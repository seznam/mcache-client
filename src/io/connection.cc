/*
 * FILE             $Id: $
 *
 * DESCRIPTION      I/O object for communication with memcache server.
 *
 * PROJECT          Seznam memcache client.
 *
 * LICENSE          See COPYING
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-21 (bukovsky)
 *                  First draft.
 */

#include <string>
#include <thread>
#include <cstdint>
#include <boost/system/system_error.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/read_until.hpp>
#if BOOST_VERSION > 107300
#else
#include <boost/asio/io_service.hpp>
#endif
#include <boost/asio/streambuf.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <arpa/inet.h>

#include "error.h"
#include "mcache/io/error.h"
#include "mcache/hash/murmur3.h"
#include "mcache/io/connection.h"

// shortcut
namespace asio = boost::asio;

namespace mc {
namespace io {
namespace {

/** Splits address string to server, port pair.
 */
std::pair<std::string, std::string>
parse_address(const std::string &addr) {
    std::vector<std::string> parts;
    boost::split(parts, addr, [] (auto c) {return c == ':';});
    if (parts.size() != 2)
        throw error_t(err::argument, "invalid destination address: " + addr);
    return std::make_pair(parts[0], parts[1]);
}

#ifdef DEBUG
/** Dumps buffer data and escape nonprinable characters.
 */
std::string dump(const asio::streambuf &buf) {
#if BOOST_VERSION > 107300
    auto size = buf.data().size();
    std::string result(static_cast<const char *>(buf.data().data()), size);
#else
    auto size = buf.size();
    std::string result(asio::buffer_cast<const char *>(buf.data()), size);
#endif /* BOOST_VERSION > 107300 */
    return log::escape(result);
}
#endif /* DEBUG */

} // namespace

namespace tcp {

/** Pimple class for tcp connection.
 */
class connection_t::pimple_connection_t: public boost::noncopyable {
public:
    // shortcut
    using this_t = pimple_connection_t;
#if BOOST_VERSION > 107300
    using io_context_t = asio::io_context;
#else
    using io_context_t = asio::io_service;
#endif /* BOOST_VERSION > 107300 */

    /** C'tor: connect the socket to server.
     */
    pimple_connection_t(const std::string &addr, const opts_t &opts)
        : addr(addr), socket(ios), deadline(ios), opts(opts)
    {
        // prepare deadline timer
        deadline.expires_at(boost::posix_time::pos_infin);
        handle_deadline();

        // resolve endpoints
        auto [host, service] = parse_address(addr);
#if BOOST_VERSION > 107300
        auto executor = ios.get_executor();
        auto addrs = asio::ip::tcp::resolver(executor).resolve(host, service);
        auto iendpoint = addrs.begin();
#else
        asio::ip::tcp::resolver::query query(dest.first, dest.second);
        auto iendpoint = asio::ip::tcp::resolver(ios).resolve(query);
        auto &addrs = iendpoint; // for compatibility with asio::async_connect
#endif

        // nice log
        DBG(DBG2, "Resolved address of memcache server: server=%s, address=%s",
                  addr.c_str(),
                  iendpoint->endpoint().address().to_string().c_str());

        // launch async connect
        set_deadline(opts.timeouts.connect);
        boost::system::error_code ec = asio::error::would_block;
        asio::async_connect(socket, addrs, [&] (auto &&v, auto &&) {ec = v;});
        do { ios.run_one();} while (ec == asio::error::would_block);

        // check whether socket is connected
        if (ec || !socket.is_open()) {
            if (!timeouted) throw io::error_t(err::io_error, ec.message());
            throw io::error_t(err::timeout, "can't connect due to timeout"
                              ": dst=" + addr);
        }
        DBG(DBG3, "Connected to memcache server: server=%s, address=%s",
                  addr.c_str(),
                  iendpoint->endpoint().address().to_string().c_str());
    }

    /** Writes data to socket. All data has been written when method returns.
     * @param data some data to sent to remote server.
     */
    void write(const std::string &data) {
        DBG(DBG1, "Send buffer with data to server: size=%zd, buffer=%s",
                  data.size(), log::escape(data).c_str());

        // launch async write
        set_deadline(opts.timeouts.write);
        boost::system::error_code ec = asio::error::would_block;
        asio::async_write(socket, asio::buffer(data),
                          [&] (auto &&v, auto &&) {ec = v;});
        do { ios.run_one();} while (ec == asio::error::would_block);

        // check whether data was written
        if (ec) {
            if (!timeouted) throw io::error_t(err::io_error, ec.message());
            throw io::error_t(err::timeout, "can't write due to timeout"
                              ": dst=" + addr);
        }
        DBG(DBG3, "Buffer has been written to server");
    }

    /** Reads all data from socket until it contains delimiter. Some data may
     * wait in input buffer for next async read operation.
     * @param delimiter data delimiter.
     * @return read data (delimiter is included).
     */
    std::string read_until(const std::string &delimiter) {
        DBG(DBG1, "Schedule receiving buffer of data from server: "
                  "delimiter=%s, buffered-size=%zd, buffered=%s",
                  log::escape(delimiter).c_str(),
                  input.size(), dump(input).c_str());

        // launch async read
        std::size_t size = 0;
        set_deadline(opts.timeouts.read);
        boost::system::error_code ec = asio::error::would_block;
        asio::async_read_until(socket, input, delimiter,
                               [&] (auto &&v, auto &&s) {ec = v, size = s;});
        do { ios.run_one();} while (ec == asio::error::would_block);

        // check whether data was read
        if (ec) {
            if (!timeouted) throw io::error_t(err::io_error, ec.message());
            throw io::error_t(err::timeout, "can't read due to timeout"
                              ": dst=" + addr);
        }
        DBG(DBG3, "New buffer with data has been read from server: size=%zd",
                  input.size());
        return copy_n(input, size);
    }

    /** Reads count bytes from socket. Some data may wait in input buffer for
     * next async read operation.
     */
    std::string read(std::size_t count) {
        DBG(DBG1, "Schedule receiving buffer of data from server: "
                  "count=%zd, buffered-size=%zd, buffered=%s",
                  count, input.size(), dump(input).c_str());

        // if there is sufficient bytes in buffer return immediately
        if (count <= input.size()) return copy_n(input, count);

        // launch async read
        set_deadline(opts.timeouts.read);
        std::size_t transfer = count - input.size();
        boost::system::error_code ec = asio::error::would_block;
        asio::async_read(socket, input, asio::transfer_at_least(transfer),
                         [&] (auto &&v, auto &&) {ec = v;});
        do { ios.run_one();} while (ec == asio::error::would_block);

        // check whether data was read
        if (ec) {
            if (!timeouted) throw io::error_t(err::io_error, ec.message());
            throw io::error_t(err::timeout, "can't read due to timeout"
                              ": dst=" + addr);
        }
        DBG(DBG3, "New buffer with data has been read from server: size=%zd",
                  input.size());
        return copy_n(input, count);
    }

private:
    /** Returns first bytes from input stream up to given count.
     */
    static std::string copy_n(asio::streambuf &stream, std::size_t count) {
#if BOOST_VERSION > 107300
         const char *ptr = static_cast<const char *>(stream.data().data());
#else
         const char *ptr = asio::buffer_cast<const char *>(stream.data());
#endif
         std::string result(ptr, ptr + count);
         stream.consume(count);
         DBG(DBG1, "Read data from input stream: count=%zd, buffer=%s",
                   count, log::escape(result).c_str());
         return result;
    }

    /** Set new deadline timeout.
     * @param milli timeout in milliseconds.
     */
    void set_deadline(milliseconds_t timeout) {
        timeouted = false;
        deadline.expires_from_now(
            boost::posix_time::milliseconds(timeout.count())
        );
    }

    /** Checks whether the deadline has passed and if not schedule new async
     * wait.
     */
    void handle_deadline() {
        // check whether timeout expired
        if (deadline.expires_at() <= asio::deadline_timer::traits_type::now()) {
            DBG(DBG3, "Connection timeouted: server=%s", addr.c_str());

            // close the socket due to timeout
            boost::system::error_code ec;
            socket.close(ec);
            if (ec) {
                LOG(ERR3, "Can't close socket: dest=%s, code=%d, error=%s",
                          addr.c_str(), ec.value(), ec.message().c_str());
            }

            // disable timeout till new timeout will be set
            deadline.expires_at(boost::posix_time::pos_infin);
            timeouted = true;
        }
        // schedule new async op
        deadline.async_wait([&] (auto &&) {this->handle_deadline();});
    }

    std::string addr;              //!< destination address
    io_context_t ios;              //!< i/o context
    asio::ip::tcp::socket socket;  //!< i/o socket
    asio::deadline_timer deadline; //!< timeout timer
    asio::streambuf input;         //!< input buffer for incoming data
    bool timeouted = false;        //!< true if socket timeouted
    opts_t opts;                   //!< connection options
};

connection_t::connection_t(const std::string &addr, opts_t opts)
    : socket(std::make_shared<pimple_connection_t>(addr, opts))
{}

void connection_t::write(const std::string &data) {
    socket->write(data);
}

std::string connection_t::read(const std::string &delimiter) {
    return socket->read_until(delimiter);
}

std::string connection_t::read(std::size_t bytes) {
    return socket->read(bytes);
}

} // namespace tcp

namespace udp {

/** Header of the each udp packet.
 */
class packet_t {
public:
    /** C'tor for one packet.
     */
    packet_t()
        : id(generate_id(std::this_thread::get_id())),
          seq(htons(1)), count(htons(1)), reserved()
    {}

    /** C'tor.
     */
    explicit packet_t(const char *raw)
        : id(reinterpret_cast<const uint16_t *>(raw)[0]),
          seq(ntohs(reinterpret_cast<const uint16_t *>(raw)[1])),
          count(ntohs(reinterpret_cast<const uint16_t *>(raw)[2])),
          reserved()
    {}

    /** Creates string that contains packet header and data.
     */
    std::string operator+(const std::string &data) const {
        std::string result(reinterpret_cast<const char *>(this), sizeof(*this));
        result.append(data);
        return result;
    }

    /** Generate new id from thread id.
     */
    static uint16_t generate_id(const std::thread::id &tid) {
        return uint16_t(murmur3(&tid, sizeof(tid), ::rand()));
                        //uint32_t(::time(0x0) * ::getpid())));
    }

    uint16_t id;       //!< req/resp id
    uint16_t seq;      //!< sequence number
    uint16_t count;    //!< total number of datagrams in message
    uint16_t reserved; //!< reserved
} __attribute__((packed));

/** Pimple class for udp connection.
 */
class connection_t::pimple_connection_t: public boost::noncopyable {
public:
    // shortcut
    using this_t = pimple_connection_t;
#if BOOST_VERSION > 107300
    using io_context_t = asio::io_context;
#else
    using io_context_t = asio::io_service;
#endif /* BOOST_VERSION > 107300 */

    /** C'tor: connect the socket to server.
     */
    pimple_connection_t(const std::string &addr, const opts_t &opts)
        : addr(addr), socket(ios), deadline(ios), opts(opts)
    {
        // prepare deadline timer
        deadline.expires_at(boost::posix_time::pos_infin);
        handle_deadline();

        // resolve endpoints
        auto [host, service] = parse_address(addr);
#if BOOST_VERSION > 107300
        auto executor = ios.get_executor();
        auto addrs = asio::ip::udp::resolver(executor).resolve(host, service);
        auto iendpoint = addrs.begin();
#else
        asio::ip::udp::resolver::query query(dest.first, dest.second);
        auto iendpoint = asio::ip::udp::resolver(ios).resolve(query);
        auto &addrs = iendpoint; // for compatibility with asio::async_connect
#endif

        // nice log
        DBG(DBG2, "Resolved address of memcache server: server=%s, address=%s",
                  addr.c_str(),
                  iendpoint->endpoint().address().to_string().c_str());

        // launch async connect
        set_deadline(opts.timeouts.connect);
        boost::system::error_code ec = asio::error::would_block;
        asio::async_connect(socket, addrs, [&] (auto &&v, auto &&) {ec = v;});
        do { ios.run_one();} while (ec == asio::error::would_block);

        // check whether socket is connected
        if (ec || !socket.is_open()) {
            if (!timeouted) throw io::error_t(err::io_error, ec.message());
            throw io::error_t(err::timeout, "can't connect due to timeout"
                              ": dst=" + addr);
        }
        DBG(DBG3, "Connected to memcache server: server=%s, address=%s",
                  addr.c_str(),
                  iendpoint->endpoint().address().to_string().c_str());
    }

    /** Writes data to socket. All data has been written when method returns.
     * @param data some data to sent to remote server.
     */
    void write(const std::string &data) {
        DBG(DBG1, "Send buffer with data to server: size=%zd, buffer=%s",
                  data.size(), log::escape(data).c_str());

        // launch async write
        set_deadline(opts.timeouts.write);
        boost::system::error_code ec = asio::error::would_block;
        socket.async_send(asio::buffer(data),
                          [&] (auto &&v, auto &&) {ec = v;});
        do { ios.run_one();} while (ec == asio::error::would_block);

        // check whether data was written
        if (ec) {
            if (!timeouted) throw io::error_t(err::io_error, ec.message());
            throw io::error_t(err::timeout, "can't write due to timeout"
                              ": dst=" + addr);
        }
        DBG(DBG3, "Buffer has been written to server");
    }

    /** Receive one datagram from server.
     * @param buffer space for datagram data.
     * @return datagram header.
     */
    packet_t receive(std::string &buffer) {
        DBG(DBG1, "Schedule receiving datagram from server");

        // launch async read
        char b[1 << 16];
        std::size_t size;
        set_deadline(opts.timeouts.read);
        boost::system::error_code ec = asio::error::would_block;
        socket.async_receive(boost::asio::buffer(b, sizeof(b)),
                             [&] (auto &&v, auto &&s) {ec = v, size = s;});
        do { ios.run_one();} while (ec == asio::error::would_block);

        // check whether data was read
        if (ec) {
            if (!timeouted) throw io::error_t(err::io_error, ec.message());
            throw io::error_t(err::timeout, "can't read due to timeout"
                              ": dst=" + addr);
        }

        // fill packet
        packet_t packet(b);
        DBG(DBG3, "New datagram has been read from server: "
                  "dgram.id=%u, dgram.seq=%d, dgram.count=%d, size=%zd",
                  packet.id, packet.seq, packet.count, size);
        buffer.append(b + sizeof(packet), size - sizeof(packet));
        return packet;
    }

private:
    /** Set new deadline timeout.
     * @param milli timeout in milliseconds.
     */
    void set_deadline(milliseconds_t timeout) {
        timeouted = false;
        deadline.expires_from_now(
            boost::posix_time::milliseconds(timeout.count())
        );
    }

    /** Checks whether the deadline has passed and if not schedule new async
     * wait.
     */
    void handle_deadline() {
        // check whether timeout expired
        if (deadline.expires_at() <= asio::deadline_timer::traits_type::now()) {
            DBG(DBG3, "Connection timeouted: server=%s", addr.c_str());

            // close the socket due to timeout
            boost::system::error_code ec;
            socket.close(ec);
            if (ec) {
                LOG(ERR3, "Can't close socket: dest=%s, code=%d, error=%s",
                          addr.c_str(), ec.value(), ec.message().c_str());
            }

            // disable timeout till new timeout will be set
            deadline.expires_at(boost::posix_time::pos_infin);
            timeouted = true;
        }
        // schedule new async op
        deadline.async_wait([&] (auto &&) {this->handle_deadline();});
    }

    std::string addr;              //!< destination address
    io_context_t ios;              //!< i/o controller
    asio::ip::udp::socket socket;  //!< i/o socket
    asio::deadline_timer deadline; //!< timeout timer
    asio::streambuf input;         //!< input buffer for incoming data
    bool timeouted;                //!< true if socket timeouted
    opts_t opts;                   //!< connection options
};

connection_t::connection_t(const std::string &addr, opts_t opts)
    : socket(std::make_shared<pimple_connection_t>(addr, opts)),
      buffer(), id()
{}

void connection_t::write(const std::string &data) {
    packet_t packet;
    id = packet.id;
    socket->write(packet + data);
}

std::string connection_t::read(const std::string &delimiter) {
    // if no data has been read then read datagram stream from server
    if (buffer.empty()) fill();

    // find delimiter
    std::string::iterator
        idelim = std::search(buffer.begin(), buffer.end(),
                             delimiter.begin(), delimiter.end());
    if (idelim == buffer.end())
        throw io::error_t(err::io_error, "partial input");

    // prepare result and erase header from buffer
    std::string result(buffer.begin(), idelim);
    buffer.erase(buffer.begin(), idelim + delimiter.size());
    return result;
}

std::string connection_t::read(std::size_t bytes) {
    // if no data has been read then read datagram stream from server
    if (buffer.empty()) fill();

    // prepare result and erase it from buffer
    if (bytes > buffer.size())
        throw io::error_t(err::io_error, "partial input");
    std::string result(buffer.data(), bytes);
    buffer.erase(0, bytes);
    return result;
}

void connection_t::fill() {
    // read first packet in datagram stream
    packet_t packet = socket->receive(buffer);
    if (packet.seq != 0)
        throw io::error_t(err::io_error, "first: invalid seq number");
    if (packet.id != id)
        throw io::error_t(err::io_error, "first: invalid id");

    // read all datagrams
    for (uint16_t i = 1; i < packet.count; ++i) {
        packet_t next = socket->receive(buffer);
        if (next.seq != i)
            throw io::error_t(err::io_error, "next: invalid seq number");
        if (next.count != packet.count)
            throw io::error_t(err::io_error, "next: invalid count");
        if (next.id != id)
            throw io::error_t(err::io_error, "next: invalid id");
    }

    DBG(DBG1, "Whole message: id=%d, size=%zd, buffer=%s",
              id, buffer.size(), log::escape(buffer).c_str());
}

} // namespace udp
} // namespace io
} // namespace mc
