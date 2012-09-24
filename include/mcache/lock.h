/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Atomic counters.
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

#ifndef MCACHE_LOCK_H
#define MCACHE_LOCK_H

namespace mc {

/** Scoped guard for lock.
 */
template <typename lock_t>
class scope_guard_t {
public:
    /** C'tor.
     */
    explicit inline scope_guard_t(const lock_t &lock): lock(&lock) {}

    /** D'tor.
     */
    inline ~scope_guard_t() { lock->unlock();}

    /** Tries lock the lock and return true if lock is locked.
     */
    inline bool try_lock() { return lock->try_lock();}

private:
    const lock_t *lock; //!< pointer to lock structure
};

namespace none {

/** Does not lock anything.
 */
class lock_t {
public:
    inline bool try_lock() const { return true;}
    inline void unlock() const {}
};

} // namespace none

namespace thread {

} // namespace thread

namespace ipc {

} // namespace ipc
} // namespace mc

#endif /* MCACHE_LOCK_H */

