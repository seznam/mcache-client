/*
 * FILE             $Id: $
 *
 * DESCRIPTION      List of server proxies.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-10-02 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_SERVER_PROXIES_H
#define MCACHE_SERVER_PROXIES_H

#include <string>
#include <vector>
#include <inttypes.h>
#include <boost/noncopyable.hpp>
#include <boost/shared_array.hpp>
#include <boost/interprocess/anonymous_shared_memory.hpp>

#include <mcache/error.h>

namespace mc {
namespace thread {

/** Wrapper around boost::shared_array.
 */
template <typename type_t>
class shared_array_t {
public:
    /** C'tor.
     */
    shared_array_t(std::size_t count): array(new type_t[count]()) {}

    /** Const element access operator.
     */
    const type_t &operator[](std::size_t i) const { return array[i];}

    /** Non-const element access operator.
     */
    type_t &operator[](std::size_t i) { return array[i];}

private:
    boost::shared_array<type_t> array; //!< array of elements
};

} // namespace thread

namespace ipc {

// push boost::interprocess into current namespace
namespace bip = boost::interprocess;

/** Wrapper around annonymous shared memory region.
 */
template <typename type_t>
class shared_array_t  {
public:
    /** C'tor.
     */
    shared_array_t(std::size_t count)
        : region(bip::anonymous_shared_memory(sizeof(type_t) * count))
    {
        // placement new: initialize all entries in array
        new (array()) type_t[count]();
    }

    /** Const element access operator.
     */
    const type_t &operator[](std::size_t i) const { return array()[i];}

    /** Non-const element access operator.
     */
    type_t &operator[](std::size_t i) { return array()[i];}

private:
    /** Casts region to array of type_t.
     */
    type_t *array() { return reinterpret_cast<type_t *>(region.get_address());}

    /** Casts region to array of type_t.
     */
    const type_t *array() const {
        return reinterpret_cast<const type_t *>(region.get_address());
    }

    bip::mapped_region region; //!< anonymous shared memory region
};

} // namespace ipc

/** Vector of server proxies.
 */
template <
    template <typename> class shared_templ_t,
    typename server_proxy_t
> class server_proxies_t: public boost::noncopyable {
public:
    /** C'tor.
     */
    template <typename server_proxy_config_t>
    server_proxies_t(const std::vector<std::string> &addresses,
                     const server_proxy_config_t &cfg)
        : shared(addresses.size()), proxies()
    {
        for (std::vector<std::string>::const_iterator
                iaddr = addresses.begin(),
                saddr = addresses.begin(),
                eaddr = addresses.end();
                iaddr != eaddr; ++iaddr)
        {
            std::size_t i = std::distance(saddr, iaddr);
            proxies.push_back(server_proxy_t(*iaddr, &shared[i], cfg));
        }
    }

    /** Returns server proxy at index i.
     */
    server_proxy_t &operator[](std::size_t i) { return proxies[i];}

private:
    // shortcuts
    typedef std::vector<server_proxy_t> proxies_t;
    typedef shared_templ_t<typename server_proxy_t::shared_t> shared_array_t;

    shared_array_t shared; //!< shared data for proxies
    proxies_t proxies;     //!< server proxies vector
};

} // namespace mc

#endif /* MCACHE_SERVER_PROXIES_H */

