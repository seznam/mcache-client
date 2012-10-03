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
#include <mcache/proto/parser.h>

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
    typedef typename connection_ptr_t::value_type connection_t;

    /** Shared data with other threads/processes.
     */
    class shared_t {
    public:
        /** C'tor.
         */
        shared_t(): restoration(), dead(false), lock() {}

        volatile time_t restoration; //!< time when reconnect is scheduled
        volatile int dead;           //!< true if server is dead
        lock_t lock;                 //!< lock for reconnect critical sections
    };

    /** C'tor.
     */
    server_proxy_t(const std::string &address,
                   shared_t *shared,
                   const server_proxy_config_t &cfg)
        : restoration_interval(cfg.restoration_interval), shared(shared),
          connections(address, cfg.timeout)
    {}

    /** Returns true if server is dead.
     */
    bool is_dead() const { return shared->dead;}

    /** Returns true if server isn't dead or if we should try make server
     * alive. It is expected that caller call send() method immediately after
     * this method.
     */
    bool callable() {
        // if server is not marked dead return immediately
        if (!shared->dead) return true;

        // check wether we shloud try make dead server alive
        time_t now = ::time(0x0);
        if (now < shared->restoration) return false;

        // if we don't get lock returns immediately
        scope_guard_t<lock_t> guard(shared->lock);
        if (!guard.try_lock()) return false;

        // we got lock, enlarge dead timeout and return true
        shared->restoration = now + restoration_interval;
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
            proto::command_parser_t<connection_t> parser(*connection);
            response_t response = parser.send(command);
            shared->dead = false;

            // if command does not understand repsonse then does not return the
            // connection to pool (the connection will be closed)
            if (response.code() != proto::resp::unrecognized)
                connections.push_back(connection);
            return response;

        } catch (const io::error_t &e) {
            // lock, destroy whole pool of connections and mark server as dead
            scope_guard_t<lock_t> guard(shared->lock);
            if (guard.try_lock()) {
                // TODO(burlog): add limit for count of errors that make server
                // TODO(burlog): dead
                connections.reset();
                shared->restoration = ::time(0x0) + restoration_interval;
                shared->dead = true;
            }
        }
        return response_t(proto::resp::io_error, "connection failed");
    }

protected:
    time_t restoration_interval;    //!< timeout for dead server
    shared_t *shared;               //!< shared data with other threads
    connections_t connections;      //!< connections pool
};

} // namespace mc

#endif /* MCACHE_SERVER_PROXY_H */

