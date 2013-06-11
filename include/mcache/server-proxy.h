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
#include <mcache/io/opts.h>
#include <mcache/io/error.h>
#include <mcache/proto/response.h>
#include <mcache/proto/parser.h>

namespace mc {
namespace aux {

/** Just push line to log.
 */
void log_server_raise_zombie(const std::string &srv, time_t restoration);

/** Just push line to log.
 */
void log_server_is_dead(const std::string &srv,
                        uint32_t fail_limit,
                        time_t restoration,
                        const std::string &reason);

/** Composes string with info about current server proxy state.
 */
std::string make_state_string(const std::string &srv,
                              std::size_t connections,
                              time_t restoration,
                              uint32_t fails,
                              uint32_t dead);

} // namespace aux

/** Configuration object for server proxy and connection objects.
 */
class server_proxy_config_t {
public:
    /** C'tor.
     */
    server_proxy_config_t(time_t restoration_interval = 60,
                          uint32_t fail_limit = 1,
                          io::opts_t io_opts = io::opts_t())
        : restoration_interval(restoration_interval), fail_limit(fail_limit),
          io_opts(io_opts)
    {}

    time_t restoration_interval; //!< time when reconnect is scheduled
    uint32_t fail_limit;         //!< count of fails after that srv become dead
    io::opts_t io_opts;          //!< io options
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
    typedef typename connection_ptr_t::element_type connection_t;
    typedef server_proxy_config_t server_proxy_config_type;

    /** Shared data with other threads/processes.
     */
    class shared_t {
    public:
        /** C'tor.
         */
        shared_t(): restoration(), dead(false), fails(), lock() {}

        volatile time_t restoration; //!< time when reconnect is scheduled
        volatile uint32_t dead;      //!< true if server is dead
        volatile uint32_t fails;     //!< current count of fails
        lock_t lock;                 //!< lock for reconnect critical sections
    };

    /** C'tor.
     */
    server_proxy_t(const std::string &address,
                   shared_t *shared,
                   const server_proxy_config_t &cfg)
        : restoration_interval(cfg.restoration_interval),
          fail_limit(cfg.fail_limit), shared(shared),
          connections(address, cfg.io_opts)
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
        aux::log_server_raise_zombie(connections.server_name(),
                                     restoration_interval);
        return true;
    }

    /** Returns time duration since last dead mark.
     */
    time_t lifespan() const {
        time_t now = ::time(0x0);
        if (!shared->restoration) return now;
        return std::max(now - (shared->restoration - restoration_interval), 0l);
    }

    /** Call command on server and process server connection errors.
     * @param command some command.
     * @return parsed command response.
     */
    template <typename command_t>
    typename command_t::response_t send(const command_t &command) {
        // pick connection from pool of connections
        typedef typename command_t::response_t response_t;
        try {
            // if command was finished successfuly then make server alive
            connection_ptr_t connection = connections.pick();
            proto::command_parser_t<connection_t> parser(*connection);
            response_t response = parser.send(command);
            shared->dead = false;
            shared->fails = 0;

            // if command does not understand repsonse then does not return the
            // connection to pool (the connection will be closed)
            if (response.code() < proto::resp::error)
                connections.push_back(connection);
            return response;

        } catch (const io::error_t &e) {
            // lock, destroy whole pool of connections and mark server as dead
            scope_guard_t<lock_t> guard(shared->lock);
            if (guard.try_lock()) {
                if (++shared->fails >= fail_limit) {
                    connections.clear();
                    shared->restoration = ::time(0x0) + restoration_interval;
                    shared->dead = true;
                    aux::log_server_is_dead(connections.server_name(),
                                            fail_limit,
                                            restoration_interval,
                                            e.what());
                }
            }
            return response_t(proto::resp::io_error,
                              std::string("connection failed: ") + e.what());
        }
        // never reached
        throw std::runtime_error(__PRETTY_FUNCTION__);
    }

    /** Returns current state of server proxy.
     */
    std::string state() const {
        return aux::make_state_string(connections.server_name(),
                                      connections.size(),
                                      shared->restoration,
                                      shared->fails,
                                      shared->dead);
    }

protected:
    time_t restoration_interval; //!< timeout for dead server
    uint32_t fail_limit;         //!< count of fails after that srv become dead
    shared_t *shared;            //!< shared data with other threads
    connections_t connections;   //!< connections pool
};

} // namespace mc

#endif /* MCACHE_SERVER_PROXY_H */

