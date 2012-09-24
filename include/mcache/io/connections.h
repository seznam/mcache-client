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
    explicit inline single_connection_pool_t(const std::string &addr,
                                             uint64_t timeout)
        : connection(boost::make_shared<connection_t>(addr, timeout))
    {}

    /** Removes connection from pool or creates new one and gives it to caller.
     * The caller is responsible for returning it using push_back method as
     * soon as he stops using it.
     */
    connection_ptr_t pick() {
        if (connection.empty())
            throw error_t(INTERNAL_ERROR, "pool of connections exhausted");
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

} // namespace io
} // namespace mc

#endif /* MCACHE_IO_CONNECTIONS_H */

