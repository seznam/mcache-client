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
#include <mcache/proto/response.h>

namespace mc {

// push options into mc namespace
using proto::opts_t;
// push server response into mc namespace

/** Template of class for memcache clients.
 */
template <
    typename pool_t,
    typename server_proxy_t
> class client_template_t: public boost::noncopyable {
public:
    // will be template arg that wraps key and data and base64 it due to
    // - whitespace problem...
    // - key size
    typedef std::string safe_string_t;

    /** C'tor.
     * @param pool pool of indexes.
     * @param addresses addresses of memcache servers.
     */
    template <typename pool_config_t, typename server_proxy_config_t>
    client_template_t(const std::vector<std::string> &addresses,
                      const pool_config_t &pcfg,
                      const server_proxy_config_t &scfg)
        : pool(addresses, pcfg), proxies()
    {
        for (std::vector<std::string>::const_iterator
                iaddr = addresses.begin(),
                eaddr = addresses.end();
                iaddr != eaddr; ++iaddr)
        {
            proxies.push_back(server_proxy_t(*iaddr, scfg));
        }
    }

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
        run(cmd::set_t(key, data, opts));
    }

    /** Call 'get' command on appropriate memcache server.
     * @param key key for data.
     * @return data for given key.
     */
    const std::string &get(const safe_string_t &key) const {
        return run(cmd::get_t(key)).data;
    }

protected:
    /** Serialize command and send it to appropriate server.
     * @param command memcache protocol command.
     * @return memcache server response.
     */
    template <typename command_t>
    const response_t &run(const command_t &command) const {
        for (typename pool_t::const_iterator
                iidx = pool.choose(command.key()),
                eidx = pool.end();
                iidx != eidx; ++iidx)
        {
            // check if choosed server node is dead
            const server_proxy_t &server = proxies[*iidx];
            if (!server.callable()) continue;

            // send command to server and wait till response arrive
            response_t response = server.send(command);
            // TODO(burlog): consistent hashing pool distribute keys after fail
            // TODO(burlog): so we can handle 404 as one `continue'
            // TODO(burlog): maybe solo fallback function will look better
            // TODO(burlog): if (404) fallback_get(++iidx);
            if (!response) throw response.exception();
            return response;
        }
        throw Error_t(ErrorCategory_t::INTERNAL_ERROR, "out of servers");
    }

    // shortcuts
    typedef std::vector<server_proxy_t> server_proxies_t;

    pool_t pool;                      //!< idxs that represents key distribution
    mutable server_proxies_t proxies; //!< i/o objects for memcache servers
};

//typedef client_template_t<mc::ch_t, mc::thread_proxy> thread_client_t;
//
//class thread_client_t: public client_template_t<mc::ch_t, mc::thread_proxy>{
//    thread_client_t(x, y, z): 
//}

} // namespace mc

//int x() {
//    mc::client_t c(a, c1, c2);
//
//    c.response_storage = xy;
//    const response_t &r1 = c.get(100);
//    if (response_t::wait_for_response != r1) throw 1;
//    if (response_t::wait_for_response != c.get(101)) throw 1;
//    if (response_t::wait_for_response != c.get(102)) throw 1;
//    if (response_t::wait_for_response != c.get(103)) throw 1;
//    if (response_t::wait_for_response != c.get(104)) throw 1;
//    if (response_t::wait_for_response != c.get(105)) throw 1;
//    c.wait_for_response();
//
//}

#endif /* MCACHE_CLIENT_H */

