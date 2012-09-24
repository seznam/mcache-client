/*
 * FILE             $Id$
 *
 * DESCRIPTION      Memcache client library error code class.
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

#ifndef MCACHE_ERROR_H
#define MCACHE_ERROR_H

#include <string>
#include <stdexcept>

namespace mc {
namespace err {

/** Error codes.
 */
enum error_code_t {
    not_found      = 404,
    internal_error = 500,
};

} // namespace err

/** Base class for all errors in libmcache.
 */
class error_t: public std::exception {
public:
    /** C'tor.
     */
    error_t(int value, const std::string &msg): value(value), msg(msg) {}

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
    int value;       //!< error code
    std::string msg; //!< error description
};

} // namespace mc

#endif /* MCACHE_ERROR_H */

