/*
 * FILE             $Id$
 *
 * DESCRIPTION      Memcache client library error code class for io.
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

#ifndef MCACHE_IO_ERROR_H
#define MCACHE_IO_ERROR_H

#include <mcache/error.h>

namespace mc {
namespace io {
namespace err {

/** Error codes.
 */
enum error_code_t {
    syntax_error   = 400,
    not_found      = 404,
    timeout        = 408,
    argument       = 420,
    internal_error = 500,
    io_error       = 501,
};

} // namespace err

/** Base class for all errors in libmcache.
 */
class error_t: public mc::error_t {
public:
    /** C'tor.
     */
    error_t(int value, const std::string &msg): mc::error_t(value, msg) {}
};

} // namespace io
} // namespace mc

#endif /* MCACHE_IO_ERROR_H */

