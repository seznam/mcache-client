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

#include <inttypes.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#if HAVE_LIBTBB
#include <tbb/concurrent_queue.h>
#else /* HAVE_LIBTBB */
#endif /* HAVE_LIBTBB */

#include <mcache/io/error.h>

namespace mc {
namespace io {

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
        if (connection.empty())
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
    single_connection_pool_t(const std::string &addr, uint64_t timeout)
        : addr(addr), timeout(timeout), max(1000), queue()
    {}

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        connection_ptr_t tmp;
        if (queue.try_pop(tmp)) return tmp;
        tbb::concurrent_queue::size_type size = queue.size();
        if (size > max)
            throw error_t(err::internal_error, "pool of connections exhausted");
        return boost::make_shared<connection_t>(addr, timeout);
    }

    /** Push connection back to pool.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &tmp) {
        queue.push_back(tmp);
        tmp.reset();
    }

protected:
    std::string addr;            //!< destination address
    uint64_t timeout;            //!< 
    int64_t max;                 //!< the largest size of the queue
    tbb::concurrent_queue queue; //!< queue of available connections
};

} // namespace bbt

namespace aux {

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
    single_connection_pool_t(const std::string &addr, uint64_t timeout)
        : addr(addr), timeout(timeout), max(1000)
    {}

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        connection_ptr_t tmp;
        return boost::make_shared<connection_t>(addr, timeout);
    }

    /** Push connection back to pool.
     * XXX: Given ptr is invalid (empty) after the call.
     */
    void push_back(connection_ptr_t &) {
    }

protected:
    std::string addr;            //!< destination address
    uint64_t timeout;            //!< 
};

} // namespace aux

// push appropriate version of caching_connection_pool into io namespace
#if HAVE_LIBTBB
using bbt::caching_connection_pool_t;
#else /* HAVE_LIBTBB */
using aux::caching_connection_pool_t;
#endif /* HAVE_LIBTBB */

} // namespace io
} // namespace mc

#endif /* MCACHE_IO_CONNECTIONS_H */

