/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Connection pool.
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

#include <mcache/io/opts.h>
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
    create_new_connection_pool_t(const std::string &addr, opts_t opts)
        : addr(addr), opts(opts)
    {}

    /** Creates new connection.
     */
    connection_ptr_t pick() {
        return boost::make_shared<connection_t>(addr, opts);
    }

    /** Push connection back to pool.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &tmp) { tmp.reset();}

    /** Just returns zero.
     */
    std::size_t size() const { return 0;}

    /** Does nothing.
     */
    void clear() {}

    /** Returns server address.
     */
    const std::string &server_name() const { return addr;}

protected:
    std::string addr; //!< destination address
    opts_t opts;      //!< io options
};

/** Cripled pool of connections with single connection. It is not suitable for
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
    single_connection_pool_t(const std::string &addr, opts_t opts)
        : addr(addr), opts(opts), connection()
    {}

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        if (!connection) return boost::make_shared<connection_t>(addr, opts);
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

    /** Returns count of connections in pool.
     */
    std::size_t size() const { return connection;}

    /** Destroy held connection.
     */
    void clear() { connection.reset();}

    /** Returns server address.
     */
    const std::string &server_name() const { return addr;}

protected:
    std::string addr;            //!< destination address
    opts_t opts;                 //!< io options
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
    caching_connection_pool_t(const std::string &addr, opts_t opts)
        : addr(addr), opts(opts), queue()
    {
        queue.set_capacity(opts.max_connections_in_pool);
    }

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        connection_ptr_t tmp;
        if (queue.try_pop(tmp)) return tmp;
        return boost::make_shared<connection_t>(addr, opts);
    }

    /** Push connection back to pool.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &tmp) {
        queue.try_push(tmp);
        tmp.reset();
    }

    /** Returns count of connections in pool.
     */
    std::size_t size() const { return std::size_t(std::max(queue.size(), 0l));}

    /** Destroys all connection found at queue.
     */
    void clear() {
        boost::mutex::scoped_lock guard(mutex);
        queue.clear();
    }

    /** Returns server address.
     */
    const std::string &server_name() const { return addr;}

protected:
    // shortcut
    typedef tbb::concurrent_bounded_queue<connection_ptr_t> queue_t;

    boost::mutex mutex; //!< pool mutex
    std::string addr;   //!< destination address
    opts_t opts;        //!< io options
    queue_t queue;      //!< queue of available connections
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
    caching_connection_pool_t(const std::string &addr, opts_t opts)
        : addr(addr), opts(opts)
    {}

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        boost::mutex::scoped_lock guard(mutex);
        if (stack.empty())
            return boost::make_shared<connection_t>(addr, opts);
        connection_ptr_t tmp = stack.top();
        stack.pop();
        return tmp;
    }

    /** Push connection back to pool.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &tmp) {
        boost::mutex::scoped_lock guard(mutex);
        if (stack.size() < opts.max_connections_in_pool) stack.push(tmp);
        tmp.reset();
    }

    /** Returns count of connections in pool.
     */
    std::size_t size() const {
        boost::mutex::scoped_lock guard(mutex);
        return stack.size();
    }

    /** Destroys all connection found at stack.
     */
    void clear() {
        boost::mutex::scoped_lock guard(mutex);
        while (!stack.empty()) stack.pop();
    }

    /** Returns server address.
     */
    const std::string &server_name() const { return addr;}

protected:
    std::string addr;                   //!< destination address
    opts_t opts;                        //!< io options
    boost::mutex mutex;                 //!< pool mutex
    std::stack<connection_ptr_t> stack; //!< stack of available connections
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

