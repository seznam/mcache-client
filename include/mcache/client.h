/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Client class.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-16 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_CLIENT_H
#define MCACHE_CLIENT_H

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

#include <mcache/error.h>
#include <mcache/proto/opts.h>

namespace mc {

// push options into mc namespace
using proto::opts_t;

/** Template of class for memcache clients.
 */
template <
    typename pool_t,
    typename server_proxies_t,
    typename impl
> class client_template_t: public boost::noncopyable {
public:
    // will be template arg that wraps key and data and base64 it due to
    // - whitespace problem...
    // - key size
    typedef std::string safe_string_t;

    /** C'tor.
     */
    template <typename pool_config_t, typename server_proxy_config_t>
    client_template_t(const std::vector<std::string> &addresses,
                      const pool_config_t &pcfg,
                      const server_proxy_config_t &scfg)
        : pool(addresses, pcfg), proxies(addresses, scfg)
    {}

    // there should be interface for storing ints, pod, frpc, protobuf, ...

    /** Call 'set' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     */
    void set(const safe_string_t &key,
             const std::string &data,
             const opts_t &opts = opts_t()) const
    {
        run(typename impl::set_t(key, data, opts));
    }

    /** Call 'get' command on appropriate memcache server.
     * @param key key for data.
     * @return data for given key.
     */
    const std::string &get(const safe_string_t &key) const {
        return run(typename impl::get_t(key)).body();
    }

protected:
    /** Serialize command and send it to appropriate server.
     * @param command memcache protocol command.
     * @return memcache server response.
     */
    template <typename command_t>
    typename command_t::response_t run(const command_t &command) const {
        for (typename pool_t::const_iterator
                iidx = pool.choose(command.key()),
                eidx = pool.end();
                iidx != eidx; ++iidx)
        {
            // check if choosed server node is dead
            typedef typename server_proxies_t::server_proxy_t server_proxy_t;
            const server_proxy_t &server = proxies[*iidx];
            if (!server.callable()) continue;

            // send command to server and wait till response arrive
            typename command_t::response_t response = server.send(command);
            // TODO(burlog): consistent hashing pool distribute keys after fail
            // TODO(burlog): so we can handle 404 as one `continue'
            // TODO(burlog): maybe solo fallback function will look better
            // TODO(burlog): if (404) fallback_get(++iidx);
            if (!response) throw response.exception();
            return response;
        }
        throw error_t(err::internal_error, "out of servers");
    }

    pool_t pool;                      //!< idxs that represents key distribution
    mutable server_proxies_t proxies; //!< i/o objects for memcache servers
};

} // namespace mc

#endif /* MCACHE_CLIENT_H */

