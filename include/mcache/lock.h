/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Locking primitives.
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
 *       2012-09-18 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_LOCK_H
#define MCACHE_LOCK_H

#include <boost/thread/mutex.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

namespace mc {

/** Scope guard for lock.
 */
template <typename lock_t>
class scope_guard_t {
public:
    /** C'tor.
     */
    explicit inline scope_guard_t(lock_t &lock): lock(&lock) {}

    /** D'tor.
     */
    inline ~scope_guard_t() { lock->unlock();}

    /** Tries lock the lock and return true if lock is locked.
     */
    inline bool try_lock() { return lock->try_lock();}

private:
    lock_t *lock; //!< pointer to lock structure
};

namespace none {

/** Does not lock anything.
 */
class lock_t {
public:
    inline bool try_lock() { return true;}
    inline void unlock() {}
};

} // namespace none

namespace thread {

/// boost thread mutex is suitable itself
typedef boost::mutex lock_t;

} // namespace thread

namespace ipc {

/// boost interprocess mutex is suitable itself
typedef boost::interprocess::interprocess_mutex lock_t;

} // namespace ipc
} // namespace mc

#endif /* MCACHE_LOCK_H */

