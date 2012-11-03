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
#include <iterator>
#include <algorithm>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>

#include <mcache/error.h>
#include <mcache/proto/opts.h>
#include <mcache/proto/response.h>

namespace mc {

// push options into mc namespace
using proto::opts_t;

/** Result of all get commands.
 */
class result_t {
private:
    // private method for semi-safe bool conversion
    void unspecified_bool_method() {}
    typedef void (result_t::*unspecified_bool_type)();

public:
    /** C'tor.
     */
    explicit result_t(bool found): found(found), data(), flags(), cas() {}

    /** C'tor.
     */
    result_t(const std::string &data, uint32_t flags, uint64_t cas = 0)
        : found(true), data(data), flags(flags), cas(cas)
    {}

    /** Returns true if data was found on server.
     */
    operator unspecified_bool_type() const {
        return found? &result_t::unspecified_bool_method: 0x0;
    }

    const bool found;       //!< true if data was found on server
    const std::string data; //!< result data
    const uint32_t flags;   //!< flags for data stored on server
    const uint64_t cas;     //!< cas identifier of data
};

/** Template of class for memcache clients.
 */
template <
    typename pool_t,
    typename server_proxies_t,
    typename impl
> class client_template_t: public boost::noncopyable {
public:
    /** C'tor.
     */
    template <typename pool_config_t, typename server_proxy_config_t>
    client_template_t(const std::vector<std::string> &addresses,
                      const pool_config_t &pcfg,
                      const server_proxy_config_t &scfg)
        : pool(addresses, pcfg), proxies(addresses, scfg)
    {}

    ///////////////////// STANDARD MEMCACHE CLIENT API ////////////////////////

    /** Call 'set' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     */
    void set(const std::string &key,
             const std::string &data,
             const opts_t &opts = opts_t())
    {
        typename impl::set_t::response_t
            response = run(typename impl::set_t(key, data, opts));
        switch (response.code()) {
        case proto::resp::ok:
        case proto::resp::stored: return;
        default: throw response.exception();
        }
    }

    /** Call 'add' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was added.
     */
    bool add(const std::string &key,
             const std::string &data,
             const opts_t &opts = opts_t())
    {
        typename impl::add_t::response_t
            response = run(typename impl::add_t(key, data, opts));
        switch (response.code()) {
        case proto::resp::ok:
        case proto::resp::stored: return true;
        case proto::resp::exists:
        case proto::resp::not_stored: return false;
        default: throw response.exception();
        }
    }

    /** Call 'replace' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was replaced.
     */
    bool replace(const std::string &key,
                 const std::string &data,
                 const opts_t &opts = opts_t())
    {
        typename impl::replace_t::response_t
            response = run(typename impl::replace_t(key, data, opts));
        switch (response.code()) {
        case proto::resp::ok:
        case proto::resp::stored: return true;
        case proto::resp::not_stored: return false;
        default: throw response.exception();
        }
    }

    /** Call 'prepend' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was prepended.
     */
    bool prepend(const std::string &key,
                 const std::string &data,
                 const opts_t &opts = opts_t())
    {
        typename impl::prepend_t::response_t
            response = run(typename impl::prepend_t(key, data, opts));
        switch (response.code()) {
        case proto::resp::ok:
        case proto::resp::stored:
        case proto::resp::not_stored: return false;
        default: throw response.exception();
        }
    }

    /** Call 'append' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was appended.
     */
    bool append(const std::string &key,
                const std::string &data,
                const opts_t &opts = opts_t())
    {
        typename impl::append_t::response_t
            response = run(typename impl::append_t(key, data, opts));
        switch (response.code()) {
        case proto::resp::ok:
        case proto::resp::stored: return true;
        case proto::resp::not_stored: return false;
        default: throw response.exception();
        }
    }

    /** Call 'cas' command on appropriate memcache server.
     * If cas identifier was not valid then exception with was thrown due to
     * this code assuption:
     *
     * while (true) {
     *     try {
     *         mc::result_t res = client.gets(3);
     *         if (!res) {
     *              if (!client.add(3, init)) continue;
     *              return;
     *         }
     *    
     *         // some magic code modifying res.data
     *         ndata = magic(res.data);
     *    
     *         if (!client.cas(3, ndata, res.cas)) {
     *             if (!client.add(3, init)) continue;
     *             return;
     *         }
     *         return;
     *
     *     } catch (const mc::proto::error_t &e) {
     *         if (e.code() != mc::proto::resp::exists) throw;
     *     }
     * }
     *
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was set, false if value was not found.
     */
    bool cas(const std::string &key,
             const std::string &data,
             const opts_t &opts)
    {
        typename impl::cas_t::response_t
            response = run(typename impl::cas_t(key, data, opts));
        switch (response.code()) {
        case proto::resp::ok:
        case proto::resp::stored: return true;
        case proto::resp::not_found: return false;
        default: throw response.exception();
        }
    }

    /** Call 'cas' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param id cas identifier.
     * @return true if value was set - cas identifier was valid.
     */
    bool cas(const std::string &key,
             const std::string &data,
             uint64_t id)
    {
        return cas(key, data, opts_t(0, 0, id));
    }

    /** Call 'get' command on appropriate memcache server.
     * @param key key for data.
     * @return data for given key.
     */
    result_t get(const std::string &key) {
        typename impl::get_t::response_t
            response = run(typename impl::get_t(key));
        switch (response.code()) {
        case proto::resp::ok:
            return result_t(response.data(), response.flags);
        case proto::resp::not_found: return result_t(false);
        default: throw response.exception();
        }
    }

    /** Call 'gets' command on appropriate memcache server.
     * @param key key for data.
     * @return data and cas identifier for given key.
     */
    result_t gets(const std::string &key) {
        typename impl::gets_t::response_t
            response = run(typename impl::gets_t(key));
        switch (response.code()) {
        case proto::resp::ok:
            return result_t(response.data(), response.flags, response.cas);
        case proto::resp::not_found: return result_t(false);
        default: throw response.exception();
        }
    }

    /** Call 'incr' command on appropriate memcache server.
     * @param key key for data.
     * @param inc amount of increment.
     * @param opts incr commands options.
     * @return new value after incrementation.
     */
    std::pair<uint64_t, bool> incr(const std::string &key,
                                   uint64_t inc = 1,
                                   const opts_t &opts = opts_t())
    {
        typename impl::incr_t::response_t
            response = run(typename impl::incr_t(key, inc, opts));
        switch (response.code()) {
        //case proto::resp::ok: return std::make_pair(response.data(), true);
        case proto::resp::ok: return std::make_pair(3, true);
        case proto::resp::not_found: return std::make_pair(0, false);
        default: throw response.exception();
        }
    }

    /** Call 'decr' command on appropriate memcache server.
     * @param key key for data.
     * @param dec amount of decrement.
     * @param opts decr commands options.
     * @return new value after decrementation.
     */
    std::pair<uint64_t, bool> decr(const std::string &key,
                                   uint64_t dec = 1,
                                   const opts_t &opts = opts_t())
    {
        typename impl::decr_t::response_t
            response = run(typename impl::decr_t(key, dec, opts));
        switch (response.code()) {
        //case proto::resp::ok: return std::make_pair(response.data(), true);
        case proto::resp::ok: return std::make_pair(3, true);
        case proto::resp::not_found: return std::make_pair(0, false);
        default: throw response.exception();
        }
    }

    /** Call 'touch' command on appropriate memcache server.
     * @param key key for data.
     * @param exp new expiration time.
     * @param opts touch commands options.
     * @return true if value was touched.
     */
    bool touch(const std::string &key,
               uint64_t exp,
               const opts_t &opts = opts_t())
    {
        typename impl::touch_t::response_t
            response = run(typename impl::touch_t(key, exp, opts));
        switch (response.code()) {
        case proto::resp::ok: return true;
        case proto::resp::not_found: return false;
        default: throw response.exception();
        }
    }

    /** Call 'delete' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was deleted.
     */
    bool del(const std::string &key) {
        typename impl::delete_t::response_t
            response = run(typename impl::delete_t(key));
        switch (response.code()) {
        case proto::resp::ok:
        case proto::resp::deleted: return true;
        case proto::resp::not_found: return false;
        default: throw response.exception();
        }
    }

    //////////////// AUTO SERIALIZATION MEMCACHE CLIENT API ///////////////////

    // this code uses template magic so if you are confused with it you can
    // disabel it by MCACHE_DISABLE_SERIALIZATION_API macro

#ifndef MCACHE_DISABLE_SERIALIZATION_API


#endif // MCACHE_DISABLE_SERIALIZATION_API

    /////////////////////////// SUPPORT CLIENT API ////////////////////////////

    /** Dumps current state of pool to string.
     */
    std::string dump() const {
        typedef typename server_proxies_t::server_proxy_t server_proxy_t;
        std::vector<std::string> proxies_state;
        std::transform(proxies.begin(), proxies.end(),
                       std::back_inserter(proxies_state),
                       boost::bind(&server_proxy_t::state, _1));
        return pool.dump(proxies_state);
    }

protected:
    /** Serialize command and send it to appropriate server.
     * @param command memcache protocol command.
     * @return memcache server response.
     */
    template <typename command_t>
    typename command_t::response_t run(const command_t &command) {
        for (typename pool_t::const_iterator
                iidx = pool.choose(command.key),
                eidx = pool.end();
                iidx != eidx; ++iidx)
        {
            // TODO(burlog): iterator should not return two same values
            // TODO(burlog): consecutively

            std::cout << command.key << ": " << *iidx << std::endl;
            // check if choosed server node is dead
            typedef typename server_proxies_t::server_proxy_t server_proxy_t;
            server_proxy_t &server = proxies[*iidx];
            if (!server.callable()) continue;

            // TODO(burlog): max continue

            // send command to server and wait till response arrive
            typename command_t::response_t response = server.send(command);
            // TODO(burlog): consistent hashing pool distribute keys after fail
            // TODO(burlog): so we can handle 404 as one `continue'
            // TODO(burlog): maybe solo fallback function will look better
            // TODO(burlog): if (404) fallback_get(++iidx);
            // TODO(burlog): continue for 404 only if server is fresh live?
            // TODO(burlog): and for get and gets commands
            if (response.code() == proto::resp::io_error) continue;
            return response;
        }
        throw error_t(err::internal_error, "out of servers");
    }

    pool_t pool;              //!< idxs that represents key distribution
    server_proxies_t proxies; //!< i/o objects for memcache servers
};

} // namespace mc

#endif /* MCACHE_CLIENT_H */

