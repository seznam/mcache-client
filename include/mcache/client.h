/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Client class.
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
 *       2012-09-16 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_CLIENT_H
#define MCACHE_CLIENT_H

#include <string>
#include <vector>
#include <limits>
#include <iterator>
#include <algorithm>
#include <boost/noncopyable.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include <mcache/error.h>
#include <mcache/proto/opts.h>
#include <mcache/proto/response.h>
#include <mcache/conversion.h>

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
    explicit result_t(bool found, const std::string &data = std::string())
        : found(found), data(data), flags(), cas()
    {}

    /** C'tor.
     */
    result_t(const std::string &data, uint32_t flags, uint64_t cas = 0)
        : found(true), data(data), flags(flags), cas(cas)
    {}

    //////////////// AUTO SERIALIZATION MEMCACHE CLIENT API ///////////////////

#ifndef MCACHE_DISABLE_SERIALIZATION_API

    /** Converts data to given type.
     */
    template <typename type_t>
    type_t as() const { return aux::cnv<type_t>::as(data);}

    /** Converts data to given type.
     */
    template <typename type_t, typename aux_t>
    type_t as(aux_t aux) const { return aux::cnv<type_t>::as(aux, data);}

#endif // MCACHE_DISABLE_SERIALIZATION_API

    ///////////////////// STANDARD MEMCACHE CLIENT API ////////////////////////

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

#ifndef MCACHE_DISABLE_SERIALIZATION_API

/** Specialization for string.
 */
template <> inline std::string result_t::as<std::string>() const { return data;}

#endif // MCACHE_DISABLE_SERIALIZATION_API

/** Configuration of the client class.
 */
class client_config_t {
public:
    client_config_t(uint32_t max_continues = 3, int64_t h404_duration = 300)
        : max_continues(max_continues), h404_duration(h404_duration)
    {}

    uint32_t max_continues; //!< max continues in client loop
    int64_t h404_duration;  //!< duration limit for handlig 404 for get
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
    client_template_t(const std::vector<std::string> &addresses,
                      const client_config_t ccfg = client_config_t())
        : pool(addresses), proxies(addresses),
          max_continues(ccfg.max_continues), h404_duration(ccfg.h404_duration)
    {}

    /** C'tor.
     */
    template <typename server_proxy_config_t>
    client_template_t(const std::vector<std::string> &addresses,
                      const server_proxy_config_t &scfg,
                      const client_config_t ccfg = client_config_t())
        : pool(addresses), proxies(addresses, scfg),
          max_continues(ccfg.max_continues), h404_duration(ccfg.h404_duration)
    {}

    /** C'tor.
     */
    template <typename server_proxy_config_t, typename pool_config_t>
    client_template_t(const std::vector<std::string> &addresses,
                      const server_proxy_config_t &scfg,
                      const pool_config_t &pcfg,
                      const client_config_t ccfg = client_config_t())
        : pool(addresses, pcfg), proxies(addresses, scfg),
          max_continues(ccfg.max_continues), h404_duration(ccfg.h404_duration)
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
        case proto::resp::stored: return true;
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
     * @throws mc::proto::error_t with code == mc::proto::resp::exists if cas
     * @throws identifier is invalid.
     */
    bool cas(const std::string &key,
             const std::string &data,
             const opts_t &opts)
    {
        if (!opts.cas) throw error_t(err::bad_argument, "invalid cas");
        typename impl::cas_t::response_t
            response = run(typename impl::cas_t(key, data, opts));
        switch (response.code()) {
        case proto::resp::ok:
        case proto::resp::stored: return true;
        case proto::resp::not_found: return false;
        default: throw response.exception();
        }
    }

    /** Apply functor to variable in memcache
     * @param key key for data
     * @param fn transformation functor
     * @param iters number of iterations limit
     * @return the value after update
     */
    template<typename fn_t>
    std::pair<std::string, uint32_t> atomic_update(const std::string &key,
            fn_t fn, const opts_t &opts = opts_t())
    {
        typedef std::pair<std::string, uint32_t> cbres_t;

        uint64_t iters = opts.iters;
        if (iters == 0) iters = 64;

        for (;iters;--iters) {
            mc::result_t res = gets(key);
            if (!res) {
                mc::proto::opts_t oadd = opts;
                cbres_t cbres = boost::unwrap_ref(fn)(std::string(), 0);
                oadd.flags = cbres.second;
                if (!add(key, cbres.first, oadd)) {
                    continue;
                }
                return cbres;
            }

            mc::proto::opts_t ocas = opts;
            ocas.cas = res.cas;

            try {
                cbres_t cbres = boost::unwrap_ref(fn)(res.data, res.flags);
                ocas.flags = cbres.second;

                if (cas(key, cbres.first, ocas))
                    return cbres;
            } catch(const mc::proto::error_t &e) {
                if (e.code() != mc::proto::resp::exists) throw;
            }
        }
        throw error_t(err::unable_cas, "max iterations reached");
    }

    /** Call 'get' command on appropriate memcache server.
     * @param key key for data.
     * @return data for given key.
     */
    result_t get(const std::string &key) {
        typename impl::get_t::response_t
            response = run(typename impl::get_t(key), true);
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
            response = run(typename impl::gets_t(key), true);
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
        case proto::resp::ok:
            return std::make_pair(aux::cnv<uint64_t>::as(response.data()), 1);
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
        case proto::resp::ok:
            return std::make_pair(aux::cnv<uint64_t>::as(response.data()), 1);
        case proto::resp::not_found: return std::make_pair(0, false);
        default: throw response.exception();
        }
    }

    /** Call 'touch' command on appropriate memcache server.
     * @param key key for data.
     * @param exp new expiration time.
     * @return true if value was touched.
     */
    bool touch(const std::string &key, uint64_t exp) {
        typename impl::touch_t::response_t
            response = run(typename impl::touch_t(key, exp));
        switch (response.code()) {
        case proto::resp::touched: return true;
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

    /** Call 'flush_all' command on all servers.
     * @param expiration when data should expire in seconds from now.
     */
    result_t flush_all(uint32_t expiration = 0) {
        typedef std::vector<typename impl::flush_all_t::response_t> responses_t;
        responses_t responses = run_all(typename impl::flush_all_t(expiration));
        uint32_t errs = 0;
        std::string descs;
        for (typename responses_t::const_iterator
                iresponse = responses.begin(),
                eresponse = responses.end();
                iresponse != eresponse; ++iresponse)
        {
            if (iresponse->code() != proto::resp::ok) {
                ++errs;
                if (!descs.empty()) descs += ", ";
                descs += "<" + iresponse->data() + ">";
            }
        }
        return result_t(!errs, descs);
    }

    //////////////// AUTO DESERIALIZATION MEMCACHE CLIENT API /////////////////

    // this code uses template magic so if you are confused with it you can
    // disabel it by MCACHE_DISABLE_SERIALIZATION_API macro

#ifndef MCACHE_DISABLE_SERIALIZATION_API

    /** Call 'set' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     */
    template <typename type_t>
    typename boost::enable_if_c<
        aux::has_as<aux::cnv<type_t> >::value,
        void
    >::type set(const std::string &key,
                const type_t &data,
                const opts_t &opts = opts_t())
    {
        typedef typename boost::remove_cv<type_t>::type bare_type_t;
        set(key, aux::cnv<bare_type_t>::as(data), opts);
    }

    /** Call 'add' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was added.
     */
    template <typename type_t>
    typename boost::enable_if_c<
        aux::has_as<aux::cnv<type_t> >::value,
        bool
    >::type add(const std::string &key,
                const type_t &data,
                const opts_t &opts = opts_t())
    {
        typedef typename boost::remove_cv<type_t>::type bare_type_t;
        return add(key, aux::cnv<bare_type_t>::as(data), opts);
    }

    /** Call 'replace' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was replaced.
     */
    template <typename type_t>
    typename boost::enable_if_c<
        aux::has_as<aux::cnv<type_t> >::value,
        bool
    >::type replace(const std::string &key,
                    const type_t &data,
                    const opts_t &opts = opts_t())
    {
        typedef typename boost::remove_cv<type_t>::type bare_type_t;
        return replace(key, aux::cnv<bare_type_t>::as(data), opts);
    }

    /** Call 'prepend' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was prepended.
     */
    template <typename type_t>
    typename boost::enable_if_c<
        aux::has_as<aux::cnv<type_t> >::value,
        bool
    >::type prepend(const std::string &key,
                    const type_t &data,
                    const opts_t &opts = opts_t())
    {
        typedef typename boost::remove_cv<type_t>::type bare_type_t;
        return prepend(key, aux::cnv<bare_type_t>::as(data), opts);
    }

    /** Call 'append' command on appropriate memcache server.
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was appended.
     */
    template <typename type_t>
    typename boost::enable_if_c<
        aux::has_as<aux::cnv<type_t> >::value,
        bool
    >::type append(const std::string &key,
                   const type_t &data,
                   const opts_t &opts = opts_t())
    {
        typedef typename boost::remove_cv<type_t>::type bare_type_t;
        return append(key, aux::cnv<bare_type_t>::as(data), opts);
    }

    /** Call 'cas' command on appropriate memcache server. For more complex
     * description see string version of this method.
     *
     * @param key key for data.
     * @param data data to store.
     * @param opts storage commands options.
     * @return true if value was set, false if value was not found.
     * @throws mc::proto::error_t with code == mc::proto::resp::exists if cas
     * @throws identifier is invalid.
     */
    template <typename type_t>
    typename boost::enable_if_c<
        aux::has_as<aux::cnv<type_t> >::value,
        bool
    >::type cas(const std::string &key,
                const type_t &data,
                const opts_t &opts = opts_t())
    {
        typedef typename boost::remove_cv<type_t>::type bare_type_t;
        return cas(key, aux::cnv<bare_type_t>::as(data), opts);
    }

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
    typename command_t::response_t
    run(const command_t &command, bool h404 = false) {
        // we will never have this count of servers
        typename pool_t::value_type
            prev = std::numeric_limits<typename pool_t::value_type>::max();
        // count of consequently continues
        uint32_t conts = 0;
        bool out_of_servers = true;

        // find callable server for given key
        for (typename pool_t::const_iterator
                iidx = pool.choose(command.key),
                eidx = pool.end();
                (iidx != eidx) && (conts < max_continues); ++iidx)
        {
            // skip previous key
            if (*iidx == prev) continue;
            prev = *iidx;

            // check if choosed server node is dead
            typedef typename server_proxies_t::server_proxy_t server_proxy_t;
            server_proxy_t &server = proxies[*iidx];
            if (server.callable()) {
                // send command to server and wait till response arrive
                typename command_t::response_t response = server.send(command);
                switch (response.code()) {
                case proto::resp::io_error: break;
                case proto::resp::not_found:
                    // if we got 404 for get command and we ask first server in
                    // the consistent hash ring and server have been recently
                    // restored so we can ask neighbour server for the data...
                    if (h404 && !conts && (server.lifespan() < h404_duration)) {
                        out_of_servers = false;
                        break;
                    }
                    // pass
                default: return response;
                }
            }

            // can't be in for statement due to "skip previous key" continue
            ++conts;
        }
        if (out_of_servers) throw out_of_servers_t();
        return typename command_t::response_t(proto::resp::not_found);
    }

    /** Serialize command and send it to all servers.
     * @param command memcache protocol command.
     * @return memcache server response.
     */
    template <typename command_t>
    std::vector<typename command_t::response_t>
    run_all(const command_t &command) {
        std::vector<typename command_t::response_t> responses;
        for (typename server_proxies_t::iterator
                iserver = proxies.begin(),
                eserver = proxies.end();
                iserver != eserver; ++iserver)
        {
            // check if choosed server node is dead
            typedef typename server_proxies_t::server_proxy_t server_proxy_t;
            server_proxy_t &server = *iserver;
            if (server.callable()) {
                // send command to server and wait till response arrive
                responses.push_back(server.send(command));

            } else {
                // for dead server create fake response
                typedef typename command_t::response_t response_t;
                responses.push_back(response_t(proto::resp::error, "dead"));
            }
        }
        return responses;
    }

    pool_t pool;                  //!< idxs that represents key distribution
    server_proxies_t proxies;     //!< i/o objects for memcache servers
    const uint32_t max_continues; //!< max continues in client loop
    const int64_t h404_duration;  //!< duration limit for handlig 404 for get
};

} // namespace mc

#endif /* MCACHE_CLIENT_H */

