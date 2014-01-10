/*
 * FILE             $Id: $
 *
 * DESCRIPTION      List of server proxies.
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
 *       2012-10-02 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_SERVER_PROXIES_H
#define MCACHE_SERVER_PROXIES_H

#include <string>
#include <vector>
#include <inttypes.h>
#include <boost/noncopyable.hpp>
#include <boost/interprocess/anonymous_shared_memory.hpp>

#include <mcache/error.h>

namespace mc {
namespace thread {

/** Simple RTTI wrapper for array.
 */
template <typename type_t>
class shared_array_t {
public:
    /** C'tor.
     */
    shared_array_t(std::size_t count): array(new type_t[count]()) {}

    /** D'tor.
     */
    ~shared_array_t() { delete [] array;}

    /** Const element access operator.
     */
    const type_t &operator[](std::size_t i) const { return array[i];}

    /** Non-const element access operator.
     */
    type_t &operator[](std::size_t i) { return array[i];}

private:
    type_t *array; //!< pointer to memory pool
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

namespace aux {

/** Array of unitialized values.
 */
template <typename type_t>
class unitialized_array_t {
public:
    /** C'tor.
     */
    unitialized_array_t(std::size_t count)
        : array(reinterpret_cast<type_t *>(::malloc(count * sizeof(type_t))))
    {}

    /** D'tor.
     */
    ~unitialized_array_t() { ::free(array);}

    /** Const element access operator.
     */
    const type_t &operator[](std::size_t i) const { return array[i];}

    /** Non-const element access operator.
     */
    type_t &operator[](std::size_t i) { return array[i];}

private:
    type_t *array;     //!< pointer to memory pool
};

} // namespace aux

/** Vector of server proxies.
 */
template <
    template <typename> class shared_templ_t,
    typename server_proxy_type
> class server_proxies_t: public boost::noncopyable {
public:
    // publish server proxy type
    typedef server_proxy_type server_proxy_t;
    // publish server proxy iterator
    typedef const server_proxy_t *const_iterator;
    // publish server proxy iterator
    typedef server_proxy_t *iterator;
    // publish server proxy type
    typedef typename server_proxy_t::server_proxy_config_type
            server_proxy_config_t;

    /** C'tor.
     */
    server_proxies_t(const std::vector<std::string> &addresses,
                     const server_proxy_config_t &cfg = server_proxy_config_t())
        : shared(addresses.size()), proxies(addresses.size()),
          count(addresses.size())
    {
        for (std::vector<std::string>::const_iterator
                iaddr = addresses.begin(),
                saddr = addresses.begin(),
                eaddr = addresses.end();
                iaddr != eaddr; ++iaddr)
        {
            // initialize server proxy inplace via placement new operator
            std::size_t i = std::distance(saddr, iaddr);
            new (&proxies[i]) server_proxy_t(*iaddr, &shared[i], cfg);
        }
    }

    /** D'tor.
     */
    ~server_proxies_t() {
        // if server_proxy_t throws than bad things can happen...
        for (std::size_t i = 0; i < count; ++i) proxies[i].~server_proxy_t();
    }

    /** Returns server proxy at index i.
     */
    server_proxy_t &operator[](std::size_t i) { return proxies[i];}

    /* Returns const iterator that points to the first server proxy.
     */
    const_iterator begin() const { return &proxies[0];}

    /** Returns const iterator one past last server proxy.
     */
    const_iterator end() const { return &proxies[count - 1] + 1;}

    /* Returns iterator that points to the first server proxy.
     */
    iterator begin() { return &proxies[0];}

    /** Returns iterator one past last server proxy.
     */
    iterator end() { return &proxies[count - 1] + 1;}

private:
    // shortcuts
    typedef aux::unitialized_array_t<server_proxy_t> proxies_t;
    typedef shared_templ_t<typename server_proxy_t::shared_t> shared_array_t;

    shared_array_t shared; //!< shared data for proxies
    proxies_t proxies;     //!< server proxies vector
    std::size_t count;     //!< count of servers
};

} // namespace mc

#endif /* MCACHE_SERVER_PROXIES_H */

