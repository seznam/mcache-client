/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Memcache server proxy responsible for handling
 *                  dead server from pool.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-18 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_SERVER_PROXY_H
#define MCACHE_SERVER_PROXY_H

#include <ctime>
#include <string>
#include <inttypes.h>

#include <mcache/lock.h>
#include <mcache/io/error.h>
#include <mcache/proto/response.h>

namespace mc {

/** Configuration object for server proxy and connection objects.
 */
class server_proxy_config_t {
public:
    time_t restoration_interval; //!< time when reconnect is scheduled
    uint64_t timeout;            //!< whole call timeout
};

/** Memcache server proxy responsible for handling dead servers.
 */
template <
    typename lock_t,
    typename connections_t
> class server_proxy_t {
public:
    // shortcuts
    typedef typename connections_t::connection_ptr_t connection_ptr_t;

    /** C'tor.
     */
    server_proxy_t(const std::string &address, const server_proxy_config_t &cfg)
        : restoration_interval(cfg.restoration_interval), restoration(),
          dead(false), connections(address, cfg.timeout), lock()
    {}

    /** Returns true if server is dead.
     */
    bool is_dead() const { return dead;}

    /** Returns true if server isn't dead or if we should try make server
     * alive. It is expected that caller call send() method immediately after
     * this method.
     */
    bool callable() {
        // if server is not marked dead return immediately
        if (!dead) return true;

        // check wether we shloud try make dead server alive
        time_t now = ::time(0x0);
        if (now < restoration) return false;

        // if we don't get lock returns immediately
        scope_guard_t<lock_t> guard(lock);
        if (!guard.try_lock()) return false;

        // we got lock, enlarge dead timeout and return true
        restoration = now + restoration_interval;
        return true;
    }

    /** Call command on server and process server connection errors.
     * @param command some command.
     * @return parsed command response.
     */
    template <typename command_t>
    typename command_t::response_t send(const command_t &command) {
        // pick connection from pool of connections
        typedef typename command_t::response_t response_t;
        connection_ptr_t connection = connections.pick();
        try {
            // if command was finished successfuly then make server alive
            response_t response = connection->send(command);
            dead = false;

            // if command does not understand repsonse does not return the
            // connection to pool (the connection will be closed)
            if (response.code() != proto::resp::unrecognized)
                connections.push_back(connection);
            return response;

        } catch (const io::error_t &e) {
            // lock, destroy whole pool of connections and mark server as dead
            scope_guard_t<lock_t> guard(lock);
            if (guard.try_lock()) {
                // TODO(burlog): add limit for count of errors that make server
                // TODO(burlog): dead
                connections.reset();
                restoration = ::time(0x0) + restoration_interval;
                dead = true;
            }
        }
        return response_t(proto::resp::io_error, "connection failed");
    }

    // TODO(burlog): make dead and restoration shared through processes...

protected:
    time_t restoration_interval; //!< timeout for dead server
    volatile time_t restoration; //!< time when reconnect is scheduled
    volatile int dead;           //!< true if server is dead
    connections_t connections;   //!< connections pool
    lock_t lock;                 //!< lock for reconnect critical sections
};

} // namespace mc

#endif /* MCACHE_SERVER_PROXY_H */

