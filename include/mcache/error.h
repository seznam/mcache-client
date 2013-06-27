/*
 * FILE             $Id$
 *
 * DESCRIPTION      Memcache client library error code class.
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
 *       2012-09-16 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_ERROR_H
#define MCACHE_ERROR_H

#include <string>
#include <stdexcept>

namespace mc {
namespace err {

/** Error codes.
 */
enum error_code_t {
    bad_request        = 400,
    not_found          = 404,
    method_not_allowed = 405,
    bad_argument       = 406,
    internal_error     = 500,
};

} // namespace err

/** Base class for all errors in libmcache.
 */
class error_t: public std::exception {
public:
    /** C'tor.
     */
    error_t(err::error_code_t value, const std::string &msg)
        : value(value), msg(msg)
    {}

    /** D'tor.
     */
    virtual ~error_t() throw() {}

    /** Returns error description.
     */
    virtual const char *what() const throw() { return msg.c_str();}

    /** Returns error code.
     */
    int code() const { return value;}

protected:
    /** C'tor.
     */
    error_t(int value, const std::string &msg): value(value), msg(msg) {}

    int value;       //!< error code
    std::string msg; //!< error description
};

/** Exception for empty pool of servers.
 */
class out_of_servers_t: public error_t {
public:
    out_of_servers_t(): error_t(err::internal_error, "out of servers") {}
};

} // namespace mc

#endif /* MCACHE_ERROR_H */

