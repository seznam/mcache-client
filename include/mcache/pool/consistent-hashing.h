/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Consistent hashing implementation.
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

#ifndef POOL_CONSISTENT_HASHING_H
#define POOL_CONSISTENT_HASHING_H

#include <map>
#include <vector>
#include <string>
#include <iterator>
#include <stdint.h>
#include <stdexcept>

#include <mcache/error.h>

namespace mc {

/** Configuration for consistent hashing pool.
 */
class consistent_hashing_pool_config_t {
public:
    /** C'tor.
     */
    consistent_hashing_pool_config_t()
        : virtual_nodes(200)
    {}

    uint32_t virtual_nodes; //!< count of virtual nodes in ring
};

/** Non template base class for consistent hashing pool.
 */
class consistent_hashing_pool_base_t {
public:
    /// namespace ring value type
    typedef uint32_t value_type;
    /// namespace ring type
    typedef std::map<uint32_t, value_type> ring_t;

    /** Dumps ring to string.
     */
    std::string dump(const ring_t &ring,
                     const std::vector<std::string> &states) const;
};

/** Parent type for const iterator of the consistent_hashing_pool_t.
 */
typedef std::iterator<
            std::forward_iterator_tag,
            const consistent_hashing_pool_base_t::value_type
        > consistent_hashing_pool_const_iterator_parent_t;

/** Const iterator for namespace ring.
 */
class consistent_hashing_pool_const_iterator
    : public consistent_hashing_pool_const_iterator_parent_t
{
public:
    // shortcuts and standard iterators stuff
    typedef consistent_hashing_pool_const_iterator_parent_t parent_t;
    typedef parent_t::reference reference;
    typedef parent_t::pointer pointer;
    typedef parent_t::value_type value_type;
    typedef consistent_hashing_pool_base_t::ring_t ring_t;

    /** C'tor.
     */
    inline consistent_hashing_pool_const_iterator(ring_t::const_iterator inode,
                                                  ring_t::const_iterator snode,
                                                  ring_t::const_iterator enode)
        : inode(inode == enode? snode: inode), snode(snode), enode(enode),
          stop(false)
    {}

    /** C'tor.
     */
    inline consistent_hashing_pool_const_iterator(ring_t::const_iterator enode)
        : inode(enode), snode(enode), enode(enode), stop(true)
    {}

    /** Returns reference to current value.
     */
    inline reference operator*() const { return dereference();}

    /** Returns pointer to current value.
     */
    inline pointer operator->() const { return &dereference();}

    /** Moves to next item.
     */
    inline consistent_hashing_pool_const_iterator &operator++() {
        increment();
        return *this;
    }

    /** Returns iterator to current item and moves to next one.
     */
    inline consistent_hashing_pool_const_iterator operator++(int) {
        consistent_hashing_pool_const_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    /** Returns true if iterators points to same place.
     */
    bool equal(const consistent_hashing_pool_const_iterator &other) const {
        return inode == other.inode;
    }

protected:
    /** Returns reference to current value.
     */
    inline reference dereference() const { return inode->second;}

    /** Increments current position in the ring.
     */
    inline void increment() {
        // move to next entry in map or to begin of map
        if (++inode == enode) {
            if (stop) throw error_t(err::internal_error, "out of servers");
            inode = snode;
            stop = true;
        }
    }

    ring_t::const_iterator inode; //!< current
    ring_t::const_iterator snode; //!< begin of map
    ring_t::const_iterator enode; //!< end of map
    bool stop;                    //!< sentinel
};

/** Ketama implementation of consistent hashing.
 */
template <typename hash_function_t>
class consistent_hashing_pool_t: public consistent_hashing_pool_base_t {
public:
    /// namespace ring value type
    using consistent_hashing_pool_base_t::value_type;
    /// namespace ring type
    using consistent_hashing_pool_base_t::ring_t;
    /// const iterator
    typedef consistent_hashing_pool_const_iterator const_iterator;

    /** C'tor.
     * @param addresses list of server addresses.
     */
    consistent_hashing_pool_t(const std::vector<std::string> &addresses,
                              const consistent_hashing_pool_config_t &
                              cfg = consistent_hashing_pool_config_t())
        : ring(), hashf()
    {
        // at least one address must be supplied
        if (addresses.empty()) throw std::out_of_range(__PRETTY_FUNCTION__);

        // create namespace ring
        for (std::vector<std::string>::const_iterator
                saddr = addresses.begin(),
                iaddr = addresses.begin(),
                eaddr = addresses.end();
                iaddr != eaddr; ++iaddr)
        {
            // prepare index of server in addresses vector
            uint32_t hash = 0;
            uint32_t idx = static_cast<uint32_t>(std::distance(saddr, iaddr));

            // push server into namespace ring
            for (uint32_t i = 0; i < cfg.virtual_nodes; ++i)
                ring.insert(std::make_pair(hash = hashf(*iaddr, hash), idx));
        }
    }

    /* Returns iterator that points to the first index of usable server.
     */
    const_iterator choose(const std::string &key) const {
        ring_t::const_iterator inode = ring.lower_bound(hashf(key));
        return const_iterator(inode, ring.begin(), ring.end());
    }

    /* Returns iterator that points to the first entry in namespace ring.
     */
    const_iterator begin() const {
        return const_iterator(ring.begin(), ring.begin(), ring.end());
    }

    /** Returns iterator one past last entry in namespace ring.
     */
    const_iterator end() const { return const_iterator(ring.end());}

    /** Dumps ring to string.
     */
    std::string dump(const std::vector<std::string> &
                     states = std::vector<std::string>()) const
    {
        return consistent_hashing_pool_base_t::dump(ring, states);
    }

protected:
    ring_t ring;           //!< namespace ring
    hash_function_t hashf; //!< hash functor
};

/** Comparison operator==.
 */
inline bool operator==(const consistent_hashing_pool_const_iterator &lhs,
                       const consistent_hashing_pool_const_iterator &rhs)
{
    return lhs.equal(rhs);
}

/** Comparison operator!=.
 */
inline bool operator!=(const consistent_hashing_pool_const_iterator &lhs,
                       const consistent_hashing_pool_const_iterator &rhs)
{
    return !lhs.equal(rhs);
}

} // namespace mc

#endif /* POOL_CONSISTENT_HASHING_H */

