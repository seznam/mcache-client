/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Simple pool using mod without key traveling.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-17 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_POOL_MOD_H
#define MCACHE_POOL_MOD_H

#include <vector>
#include <string>
#include <iterator>
#include <stdint.h>

namespace mc {

/** Non template base class for mod pool.
 */
class mod_pool_base_t {
public:
    /// value type
    typedef uint32_t value_type;
};

/** Parent type for const iterator of the mod_pool_t.
 */
typedef std::iterator<
            std::forward_iterator_tag,
            const mod_pool_base_t::value_type
        > mod_pool_const_iterator_parent_t;

/** Const 'iterator' that returns only one entry.
 */
class mod_pool_const_iterator: public mod_pool_const_iterator_parent_t {
public:
    // shortcuts and standard iterators stuff
    typedef mod_pool_const_iterator_parent_t parent_t;
    typedef parent_t::reference reference;
    typedef parent_t::pointer pointer;
    typedef parent_t::value_type value_type;

    /** C'tor.
     */
    explicit inline mod_pool_const_iterator(value_type value)
        : value(value), stop(false)
    {}

    /** C'tor.
     */
    inline mod_pool_const_iterator(): value(), stop(true) {}

    /** Returns reference to current value.
     */
    inline reference operator*() const { return dereference();}

    /** Returns pointer to current value.
     */
    inline pointer operator->() const { return &dereference();}

    /** Moves to next item.
     */
    inline mod_pool_const_iterator &operator++() {
        increment();
        return *this;
    }

    /** Returns iterator to current item and moves to next one.
     */
    inline mod_pool_const_iterator operator++(int) {
        mod_pool_const_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    /** Returns true if iterators points to same place.
     */
    inline bool equal(const mod_pool_const_iterator &other) const {
        if (stop && other.stop) return true;
        if (other.stop) return false;
        return value == other.value;
    }

protected:
    /** Returns reference to current value.
     */
    inline reference dereference() const { return value;}

    /** Stops iteration.
     */
    inline void increment() { stop = true;}

    value_type value; //!< iterator value
    bool stop;        //!< sentinel
};

/** Pool that using mod without key traveling. If some server is dead, than
 * theese keys is not cached.
 */
template <typename hash_function_t>
class mod_pool_t: public mod_pool_base_t {
public:
    /// value type
    using mod_pool_base_t::value_type;
    /// const iterator type
    typedef mod_pool_const_iterator const_iterator;

    /** C'tor.
     */
    mod_pool_t(const std::vector<std::string> &addresses, uint32_t)
        : max(static_cast<uint32_t>(addresses.size())), hashf()
    {
        // at least one address must be supplied
        if (addresses.empty()) throw std::out_of_range(__PRETTY_FUNCTION__);
    }

    /* Returns iterator that points to the first index of usable server.
     */
    const_iterator choose(const std::string &key) const {
        return const_iterator(hashf(key) % max);
    }

    /* Returns iterator that points to the first entry.
     */
    const_iterator begin() const { return const_iterator(0);}

    /** Returns iterator one past last entry.
     */
    const_iterator end() const { return const_iterator();}

protected:
    value_type max;        //!< count of available servers
    hash_function_t hashf; //!< hash functor
};

/** Comparison operator==.
 */
inline bool operator==(const mod_pool_const_iterator &lhs,
                       const mod_pool_const_iterator &rhs)
{
    return lhs.equal(rhs);
}

/** Comparison operator!=.
 */
inline bool operator!=(const mod_pool_const_iterator &lhs,
                       const mod_pool_const_iterator &rhs)
{
    return !lhs.equal(rhs);
}

} // namespace mc

#endif /* MCACHE_POOL_MOD_H */

