/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Connection pool.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-19 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_IO_CONNECTIONS_H
#define MCACHE_IO_CONNECTIONS_H

#include <stack>
#include <inttypes.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>

#if HAVE_LIBTBB
#include <tbb/concurrent_queue.h>
#endif /* HAVE_LIBTBB */

#include <mcache/io/error.h>

namespace mc {
namespace io {

/** Pool that does not cache connection instead of it creates for each command
 * new one.
 */
template <typename connection_t>
class create_new_connection_pool_t {
public:
    // defines pointer to connection type
    typedef boost::shared_ptr<connection_t> connection_ptr_t;

    /** C'tor.
     */
    explicit inline
    create_new_connection_pool_t(const std::string &addr, uint64_t timeout)
        : addr(addr), timeout(timeout)
    {}

    /** Creates new connection.
     */
    connection_ptr_t pick() {
        return boost::make_shared<connection_t>(addr, timeout);
    }

    /** 'Push connection back to pool'.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &tmp) { tmp.reset();}

protected:
    std::string addr; //!< destination address
    uint64_t timeout; //!< 
};

/** Cripled pool of connections with single connection. If someone ask for
 * second connection the exception is raised. It is not suitable for
 * interthread usage.
 */
template <typename connection_t>
class single_connection_pool_t {
public:
    // defines pointer to connection type
    typedef boost::shared_ptr<connection_t> connection_ptr_t;

    /** C'tor.
     */
    explicit inline
    single_connection_pool_t(const std::string &addr, uint64_t timeout)
        : connection(boost::make_shared<connection_t>(addr, timeout))
    {}

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        if (!connection)
            throw error_t(err::internal_error, "pool of connections exhausted");
        connection_ptr_t tmp = connection;
        connection.reset();
        return tmp;
    }

    /** Push connection back to pool.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &tmp) {
        connection = tmp;
        tmp.reset();
    }

protected:
    connection_ptr_t connection; //!< current connection
};

namespace bbt {

#if HAVE_LIBTBB
/** Lock-free pool that are caching connections up to max count.
 */
template <typename connection_t>
class caching_connection_pool_t {
public:
    // defines pointer to connection type
    typedef boost::shared_ptr<connection_t> connection_ptr_t;

    /** C'tor.
     */
    explicit inline
    caching_connection_pool_t(const std::string &addr, uint64_t timeout)
        : addr(addr), timeout(timeout), queue()
    {
        queue.set_capacity(3);
    }

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        connection_ptr_t tmp;
        if (queue.try_pop(tmp)) return tmp;
        return boost::make_shared<connection_t>(addr, timeout);
    }

    /** Push connection back to pool.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &tmp) {
        queue.try_push(tmp);
        tmp.reset();
    }

protected:
    // shortcut
    typedef tbb::concurrent_bounded_queue<connection_ptr_t> queue_t;

    std::string addr; //!< destination address
    uint64_t timeout; //!< 
    queue_t queue;    //!< queue of available connections
};
#endif /* HAVE_LIBTBB */

} // namespace bbt

namespace lock {

/** Locking pool that are caching connections up to max count.
 */
template <typename connection_t>
class caching_connection_pool_t {
public:
    // defines pointer to connection type
    typedef boost::shared_ptr<connection_t> connection_ptr_t;

    /** C'tor.
     */
    explicit inline
    caching_connection_pool_t(const std::string &addr, uint64_t timeout)
        : addr(addr), timeout(timeout), max(3)
    {}

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        boost::mutex::scoped_lock guard(mutex);
        if (stack.empty())
            return boost::make_shared<connection_t>(addr, timeout);
        connection_ptr_t tmp = stack.top();
        stack.pop();
        return tmp;
    }

    /** Push connection back to pool.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &tmp) {
        boost::mutex::scoped_lock guard(mutex);
        if (stack.size() < max) stack.push(tmp);
        tmp.reset();
    }

protected:
    std::string addr;                   //!< destination address
    uint64_t timeout;                   //!< 
    std::size_t max;                    //!< 
    boost::mutex mutex;                 //!<
    std::stack<connection_ptr_t> stack; //!<
};

} // namespace lock

// push appropriate version of caching_connection_pool into io namespace
#if HAVE_LIBTBB
using bbt::caching_connection_pool_t;
#else /* HAVE_LIBTBB */
using lock::caching_connection_pool_t;
#endif /* HAVE_LIBTBB */

} // namespace io
} // namespace mc

#endif /* MCACHE_IO_CONNECTIONS_H */

