/*
 * FILE             $Id: $
 *
 * DESCRIPTION      I/O object for communication with memcache server.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-19 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_IO_CONNECTIONS_H
#define MCACHE_IO_CONNECTIONS_H

#include <string>
#include <boost/shared_ptr.hpp>

#include <mcache/error.h>
#include <mcache/command/response.h>

namespace mc {

// push response into mc namespace
using cmd::response_t;

namespace io {

/** 
 */
class opts_t {
public:
};

namespace udp {
} // namespace udp

namespace tcp {

/** I/O object that holds one tcp socket to memcache server and provides
 * interface for sending messages to them.
 */
class connection_t {
public:
    /** C'tor.
     */
    connection_t(const std::string &addr, opts_t opts);

    /** Sends command to memcache server.
     */
    template <typename command_t>
    response_t send(const command_t &command) {
        // send serialized command to server
        write(command.serialize());

        // fetch response header (txt: first line of response)
        std::string header = read(command.header_delimiter());
        response_t response = command.deserialize_header(header);

        // if response contains body fetch it and return response
        //if (response.body_size())
        //    socket->fetch_body(response.body_size(), response.body);
        return response;
    }

protected:
    /** Sends all data to server.
     */
    void write(const std::string &data);

    /** Reads data till delimiter and returns it.
     */
    std::string read(const std::string &delimiter);

    /** Reads bytes count data and returns it.
     */
    std::string read(std::size_t bytes);

    /** Pimple class.
     */
    class pimple_connection_t;

    // shortcut
    typedef boost::shared_ptr<pimple_connection_t> pimple_connection_ptr_t;

    pimple_connection_ptr_t socket; //!< hides i/o implementation
};

} // namespace tcp
} // namespace io
} // namespace mc

#endif /* MCACHE_IO_CONNECTIONS_H */

