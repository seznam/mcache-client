/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Test program for libmcache: bin protocol test.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-11-07 (bukovsky)
 *                  First draft.
 */

#include <ctime>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/if.hpp>
#include <cxxabi.h>
#include <endian.h>
#include <arpa/inet.h>

#include <mcache/init.h>
#include <mcache/proto/binary.h>
#include <mcache/proto/response.h>
#include <mcache/proto/parser.h>

namespace test {
namespace helper {

static const char *HEX = "0123456789abcdef";

} // namespace helper

/** Converts character to hex number stored in string.
 * @param ch binary character.
 * @return hex number.
 */
inline std::string hexize(const unsigned char ch) {
    return std::string(1, helper::HEX[ch >> 4])
               .append(1, helper::HEX[ch & 0x0f]);
}

/** Converts binary string to sequence of hex numbers.
 * @param str binary data.
 * @return hexized binary data.
 */
inline std::string hexize(const std::string &str) {
    std::string res;
    int i = 0;
    for (std::string::const_iterator
            istr = str.begin(),
            estr = str.end();
            istr != estr; ++istr, ++i)
    {
        if (i % 4 == 0) { i = 0; res.append("|");}
        res.append(hexize(static_cast<unsigned char>(*istr)));
         if (i % 4 == 3) continue;
         res.append(1, '.');
    }
    return res;
}

/** 
 *   |0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
 *   +---------------+---------------+---------------+---------------+
 *  0| Magic         | Opcode        | Key length                    |
 *   +---------------+---------------+---------------+---------------+
 *  4| Extras length | Data type     | Reserved / Status             |
 *   +---------------+---------------+---------------+---------------+
 *  8| Total body length                                             |
 *   +---------------+---------------+---------------+---------------+
 * 12| Opaque                                                        |
 *   +---------------+---------------+---------------+---------------+
 * 16| CAS                                                           |
 *   |                                                               |
 *   +---------------+---------------+---------------+---------------+
 * 24|                                                               |
 *   . Extras body                                                   .
 *   |                                                               |
 *   +---------------+---------------+---------------+---------------+
 * xx|                                                               |
 *   . Key body                                                      .
 *   |                                                               |
 *   +---------------+---------------+---------------+---------------+
 * xx|                                                               |
 *   . Value body                                                    .
 *   |                                                               |
 *   +---------------+---------------+---------------+---------------+
 *
 * Magic:
 *  0x80    Request
 *  0x81    Response
 *
 * Opcode:
 *  0x00    Get
 *  0x01    Set
 *  0x02    Add
 *  0x03    Replace
 *  0x04    Delete
 *  0x05    Increment
 *  0x06    Decrement
 *  0x07    Quit
 *  0x08    Flush
 *  0x09    GetQ
 *  0x0A    No-op
 *  0x0B    Version
 *  0x0C    GetK
 *  0x0D    GetKQ
 *  0x0E    Append
 *  0x0F    Prepend
 *  0x10    Stat
 *  0x11    SetQ
 *  0x12    AddQ
 *  0x13    ReplaceQ
 *  0x14    DeleteQ
 *  0x15    IncrementQ
 *  0x16    DecrementQ
 *  0x17    QuitQ
 *  0x18    FlushQ
 *  0x19    AppendQ
 *  0x1A    PrependQ
 *
 * Response Status:
 *  0x0000  No error
 *  0x0001  Key not found
 *  0x0002  Key exists
 *  0x0003  Value too large
 *  0x0004  Invalid arguments
 *  0x0005  Item not stored
 *  0x0006  Incr/Decr on non-numeric value.
 *  0x0081  Unknown command
 *  0x0082  Out of memory
 *
 * Data types:
 *  0x00    Raw bytes
 */
struct packet_t {
    /** Request.
     */
    packet_t(uint8_t opcode,
             uint64_t cas,
             const std::string &key,
             const std::string &extra = std::string(),
             const std::string &body = std::string())
    {
        // packet header size
        data.resize(24);

        // magic
        data[0] = uint16_t(0x80);
        // opcode
        data[1] = opcode;
        // key len
        as<uint16_t>(&data[2]) = htons(static_cast<uint16_t>(key.size()));
        // extras len
        data[4] = static_cast<uint8_t>(extra.size());
        // data type
        data[5] = 0;
        // reserved
        data[6] = 0;
        data[7] = 0;
        // body len
        std::string::size_type size = key.size() + extra.size() + body.size();
        as<uint32_t>(&data[8]) = htonl(static_cast<uint32_t>(size));
        // opaque
        as<uint32_t>(&data[12]) = 0;
        // cas
        as<uint64_t>(&data[16]) = htobe64(cas);

        // body of the packet
        data.append(extra);
        data.append(key);
        data.append(body);
    }

    /** Response.
     */
    packet_t(uint8_t opcode,
             uint16_t status,
             uint32_t cas,
             const std::string &key = std::string(),
             const std::string &extra = std::string(),
             const std::string &body = std::string())
    {
        // packet header size
        data.resize(24);

        // magic
        data[0] = uint16_t(0x81);
        // opcode
        data[1] = opcode;
        // key len
        as<uint16_t>(&data[2]) = htons(static_cast<uint16_t>(key.size()));
        // extras len
        data[4] = static_cast<uint8_t>(extra.size());
        // data type
        data[5] = 0;
        // status
        as<uint16_t>(&data[6]) = htons(status);
        // body len
        std::string::size_type size = key.size() + extra.size() + body.size();
        as<uint32_t>(&data[8]) = htonl(static_cast<uint32_t>(size));
        // opaque
        as<uint32_t>(&data[12]) = 0;
        // cas
        as<uint64_t>(&data[16]) = htobe64(cas);

        // body of the packet
        data.append(extra);
        data.append(key);
        data.append(body);
    }

    packet_t(): data() {}

    packet_t(bool): data("bbbbbbbbbbbbbbbbbbbbbbbb") {}

    template <typename type_t>
    type_t &as(char *ptr) const {
        return *reinterpret_cast<type_t *>(ptr);
    }

    operator const std::string &() const { return data;}

    std::string data;
};

class validation_connection_error_t: public std::runtime_error {
public:
    validation_connection_error_t(const std::string &str)
        : std::runtime_error(str)
    {}
};

class validation_connection_t {
public:
    validation_connection_t(const std::string &request,
                            const std::string &header,
                            const std::string &body)
        : request(request), responses()
    {
        responses.push_back(body);
        responses.push_back(header);
    }

    validation_connection_t(const std::string &request,
                            const std::string &header)
        : request(request), responses()
    {
        responses.push_back(header);
    }

    void write(const std::string &validate) {
        //std::cerr << std::endl << "requests(test, bin):" << std::endl
        //          << hexize(request) << std::endl
        //          << hexize(validate) << std::endl;
        if (validate != request)
            throw validation_connection_error_t("invalid request");
    }

    std::string read(std::size_t size) {
        if (responses.empty())
            throw validation_connection_error_t("empty response");
        std::string response = responses.back();
        if (size < responses.back().size()) {
            response.resize(size);
            responses.back().erase(0, size);
        } else responses.pop_back();
        //std::cerr << std::endl << "response(" << size << "):" << std::endl
        //          << hexize(response) << std::endl;
        return response;
    }

    bool empty() const { return responses.empty();}

    std::string request;
    std::vector<std::string> responses;
};

typedef mc::proto::command_parser_t<validation_connection_t> command_parser_t;
typedef mc::proto::bin::api api;

bool error_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x81, 0, "", "error desc");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool error_client_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x06, 0, "", "");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::client_error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool error_server_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x82, 0, "", "");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::server_error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool error_too_long_key() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command(std::string(251, '3'));
    packet_t request(0, 0, "3");
    packet_t response(0, 0x82, 0, "", "");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        parser.send(command);
        return false;

    } catch (const mc::error_t &e) {
        return e.code() == mc::err::bad_argument;

    } catch (const std::exception &e) {
        return false;
    }
    return false;
}

bool error_invalid_char_in_key() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3 3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x82, 0, "", "");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        parser.send(command);
        return false;

    } catch (const mc::error_t &e) {
        return e.code() == mc::err::bad_argument;

    } catch (const std::exception &e) {
        return false;
    }
    return false;
}

bool get_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response;
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(true);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x81, 0, "", "error desc");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_not_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x01, 0, "", "key not found");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_found)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x00, 0, "", "3333", "abc");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        api::get_t::response_t response = parser.send(command);
        if (!response) return false;
        if (response.data() != "abc") return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_found_without_extras() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x00, 0, "", "", "abc");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::invalid)
            return false;
    } catch (const std::exception &) { return false;}
    return true;
}

bool get_command_found_with_key() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x00, 0, "3", "3333", "abc");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        api::get_t::response_t response = parser.send(command);
        if (!response) return false;
        if (response.data() != "abc") return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_found_flags() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::get_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x00, 123456, "", "\xde\xad\xbe\xef", "abc");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).flags != 0xdeadbeef) return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool get_command_gets() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::gets_t command("3");
    packet_t request(0, 0, "3");
    packet_t response(0, 0x00, 333, "", "3333", "abc");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).cas != 333) return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc", mc::proto::opts_t(0x0badcafe, 0xdeadbeef));
    packet_t request(1, 0, "3", "\xde\xad\xbe\xef\x0b\xad\xca\xfe", "abc");
    packet_t response;
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc", mc::proto::opts_t(0x0badcafe, 0xdeadbeef));
    packet_t request(1, 0, "3", "\xde\xad\xbe\xef\x0b\xad\xca\xfe", "abc");
    packet_t response(true);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc", mc::proto::opts_t(0x0badcafe, 0xdeadbeef));
    packet_t request(1, 0, "3", "\xde\xad\xbe\xef\x0b\xad\xca\xfe", "abc");
    packet_t response(1, 0x81, 0, "", "error desc");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc", mc::proto::opts_t(0x0badcafe, 0xdeadbeef));
    packet_t request(1, 0, "3", "\xde\xad\xbe\xef\x0b\xad\xca\xfe", "abc");
    packet_t response(1, 0x00, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::stored)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_not_stored() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc", mc::proto::opts_t(0x0badcafe, 0xdeadbeef));
    packet_t request(1, 0, "3", "\xde\xad\xbe\xef\x0b\xad\xca\xfe", "abc");
    packet_t response(1, 0x05, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_stored)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_exists() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc", mc::proto::opts_t(0x0badcafe, 0xdeadbeef));
    packet_t request(1, 0, "3", "\xde\xad\xbe\xef\x0b\xad\xca\xfe", "abc");
    packet_t response(1, 0x02, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::exists)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool set_command_not_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::set_t command("3", "abc", mc::proto::opts_t(0x0badcafe, 0xdeadbeef));
    packet_t request(1, 0, "3", "\xde\xad\xbe\xef\x0b\xad\xca\xfe", "abc");
    packet_t response(1, 0x01, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_found)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool cas_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    char eigth_zeros[] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
    mc::proto::opts_t opts(0, 0, 0xdeadc0de);
    api::set_t command("3", "abc", opts);
    packet_t request(1, 0xdeadc0de, "3", std::string(eigth_zeros, 8), "abc");
    packet_t response(1, 0x00, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::stored)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool cas_command_exists() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    char eigth_zeros[] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};
    mc::proto::opts_t opts(0, 0, 0xdeadc0de);
    api::set_t command("3", "abc", opts);
    packet_t request(1, 0xdeadc0de, "3", std::string(eigth_zeros, 8), "abc");
    packet_t response(1, 0x02, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::exists)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool prepend_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::prepend_t command("3", "abc", mc::proto::opts_t(0x0badcafe));
    packet_t request(15, 0, "3", "", "abc");
    packet_t response(15, 0x00, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::stored)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    mc::proto::opts_t opts(0x0badcafe, 0, 0xdeadc0de);
    std::string extras("\x00\x00\x00\x00\xde\xad\xbe\xef"
                       "\x00\x00\x00\x00\xde\xad\xc0\xde"
                       "\x0b\xad\xca\xfe", 20);
    api::incr_t command("3", 0xdeadbeef, opts);
    packet_t request(5, 0, "3", extras);
    packet_t response;
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    mc::proto::opts_t opts(0x0badcafe, 0, 0xdeadc0de);
    std::string extras("\x00\x00\x00\x00\xde\xad\xbe\xef"
                       "\x00\x00\x00\x00\xde\xad\xc0\xde"
                       "\x0b\xad\xca\xfe", 20);
    api::incr_t command("3", 0xdeadbeef, opts);
    packet_t request(5, 0, "3", extras);
    packet_t response(true);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    mc::proto::opts_t opts(0x0badcafe, 0, 0xdeadc0de);
    std::string extras("\x00\x00\x00\x00\xde\xad\xbe\xef"
                       "\x00\x00\x00\x00\xde\xad\xc0\xde"
                       "\x0b\xad\xca\xfe", 20);
    api::incr_t command("3", 0xdeadbeef, opts);
    packet_t request(5, 0, "3", extras);
    packet_t response(5, 0x81, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_bad_body() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    mc::proto::opts_t opts(0x0badcafe, 0, 0xdeadc0de);
    std::string extras("\x00\x00\x00\x00\xde\xad\xbe\xef"
                       "\x00\x00\x00\x00\xde\xad\xc0\xde"
                       "\x0b\xad\xca\xfe", 20);
    api::incr_t command("3", 0xdeadbeef, opts);
    packet_t request(5, 0, "3", extras);
    packet_t response(5, 0x00, 0, "3333");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::invalid)
            return false;
    } catch (const std::exception &) { return false;}
    return true;
}

bool incr_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    mc::proto::opts_t opts(0x0badcafe, 0, 0xdeadc0de);
    std::string extras("\x00\x00\x00\x00\xde\xad\xbe\xef"
                       "\x00\x00\x00\x00\xde\xad\xc0\xde"
                       "\x0b\xad\xca\xfe", 20);
    api::incr_t command("3", 0xdeadbeef, opts);
    packet_t request(5, 0, "3", extras);
    packet_t response(5, 0x00, 0, "33333333");
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).data() != "3689348814741910323")
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool incr_command_not_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    mc::proto::opts_t opts(0x0badcafe, 0, 0xdeadc0de);
    std::string extras("\x00\x00\x00\x00\xde\xad\xbe\xef"
                       "\x00\x00\x00\x00\xde\xad\xc0\xde"
                       "\x0b\xad\xca\xfe", 20);
    api::incr_t command("3", 0xdeadbeef, opts);
    packet_t request(5, 0, "3", extras);
    packet_t response(5, 0x01, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_found)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    packet_t request(4, 0, "3");
    packet_t response;
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    packet_t request(4, 0, "3");
    packet_t response(true);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    packet_t request(4, 0, "3");
    packet_t response(4, 0x81, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    packet_t request(4, 0, "3");
    packet_t response(4, 0x00, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::deleted)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool del_command_not_found() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::delete_t command("3");
    packet_t request(4, 0, "3");
    packet_t response(4, 0x01, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::not_found)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool touch_command_empty() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::touch_t command("3", 0xdeadbeef);
    packet_t request(28, 0, "3", "\xde\xad\xbe\xef");
    packet_t response;
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::empty)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool touch_command_unrecognized() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::touch_t command("3", 0xdeadbeef);
    packet_t request(28, 0, "3", "\xde\xad\xbe\xef");
    packet_t response(true);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::unrecognized)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool touch_command_error() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::touch_t command("3", 0xdeadbeef);
    packet_t request(28, 0, "3", "\xde\xad\xbe\xef");
    packet_t response(28, 0x81, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::error)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

bool touch_command_ok() {
    std::cout << __PRETTY_FUNCTION__ << ": ";

    // prepare request and response
    api::touch_t command("3", 0xdeadbeef);
    packet_t request(28, 0, "3", "\xde\xad\xbe\xef");
    packet_t response(28, 0x00, 0);
    validation_connection_t connection(request, response);

    // execute command
    try {
        command_parser_t parser(connection);
        if (parser.send(command).code() != mc::proto::resp::touched)
            return false;
    } catch (const std::exception &) { return false;}
    return connection.empty();
}

class Checker_t {
public:
    Checker_t(): fails() {}

    void operator()(bool result) {
        fails += !result;
        if (result)
            std::cout << "[01;32m" << "ok" << "[01;0m" << std::endl;
        else
            std::cout << "[01;31m" << "fail" << "[01;0m" << std::endl;
    }

    int fails;
};

} // namespace test

int main(int, char **) {
    mc::init();
    test::Checker_t check;

    // error
    check(test::error_error());
    check(test::error_client_error());
    check(test::error_server_error());
    check(test::error_too_long_key());
    check(test::error_invalid_char_in_key());

    // retrieval
    check(test::get_command_empty());
    check(test::get_command_unrecognized());
    check(test::get_command_error());
    check(test::get_command_not_found());
    check(test::get_command_found());
    check(test::get_command_found_without_extras());
    check(test::get_command_found_with_key());
    check(test::get_command_found_flags());
    check(test::get_command_gets());

    // storage
    check(test::set_command_empty());
    check(test::set_command_unrecognized());
    check(test::set_command_error());
    check(test::set_command_ok());
    check(test::set_command_not_stored());
    check(test::set_command_not_found());
    check(test::set_command_exists());
    check(test::cas_command_ok());
    check(test::cas_command_exists());
    check(test::prepend_command_ok());

    // incr/decr
    check(test::incr_command_empty());
    check(test::incr_command_unrecognized());
    check(test::incr_command_error());
    check(test::incr_command_bad_body());
    check(test::incr_command_ok());
    check(test::incr_command_not_found());

    // delete
    check(test::del_command_empty());
    check(test::del_command_unrecognized());
    check(test::del_command_error());
    check(test::del_command_ok());
    check(test::del_command_not_found());

    // touch
    check(test::touch_command_empty());
    check(test::touch_command_unrecognized());
    check(test::touch_command_error());
    check(test::touch_command_ok());

    return check.fails;
}


