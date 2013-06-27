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
 *       2012-09-19 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_IO_CONNECTION_H
#define MCACHE_IO_CONNECTION_H

#include <string>
#include <boost/shared_ptr.hpp>

#include <mcache/error.h>
#include <mcache/io/opts.h>

namespace mc {
namespace io {
namespace tcp {

/** I/O object that holds one tcp socket to memcache server and provides
 * interface for sending and retrieving messages to/from them.
 */
class connection_t {
public:
    /** C'tor.
     */
    connection_t(const std::string &addr, opts_t opts);

    /** Sends all data to server.
     */
    void write(const std::string &data);

    /** Reads data till delimiter and returns it.
     */
    std::string read(const std::string &delimiter);

    /** Reads bytes count data and returns it.
     */
    std::string read(std::size_t bytes);

protected:
    /** Pimple class.
     */
    class pimple_connection_t;

    // shortcut
    typedef boost::shared_ptr<pimple_connection_t> pimple_connection_ptr_t;

    pimple_connection_ptr_t socket; //!< hides i/o implementation
};

} // namespace tcp

namespace udp {

/** I/O object that holds one udp socket to memcache server and provides
 * interface for sending and retrieving messages to/from them.
 */
class connection_t {
public:
    /** C'tor.
     */
    connection_t(const std::string &addr, opts_t opts);

    /** Sends all data to server.
     */
    void write(const std::string &data);

    /** Reads data till delimiter and returns it.
     */
    std::string read(const std::string &delimiter);

    /** Reads bytes count data and returns it.
     */
    std::string read(std::size_t bytes);

protected:
    /** Fill buffer from datagrams.
     */
    void fill();

    /** Pimple class.
     */
    class pimple_connection_t;

    // shortcut
    typedef boost::shared_ptr<pimple_connection_t> pimple_connection_ptr_t;

    pimple_connection_ptr_t socket; //!< hides i/o implementation
    std::string buffer;             //!< buffer for incoming dat
    uint16_t id;                    //!< dgram identifier
};

} // namespace udp
} // namespace io
} // namespace mc

#endif /* MCACHE_IO_CONNECTION_H */

