/*
 * FILE             $Id$
 *
 * DESCRIPTION      Memcache client library error code class for proto.
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

#ifndef MCACHE_PROTO_ERROR_H
#define MCACHE_PROTO_ERROR_H

#include <mcache/error.h>

namespace mc {
namespace proto {
namespace resp {

/** Server response codes.
 */
enum response_code_t {
    ok           = 200,
    stored       = 201,
    deleted      = 202,
    touched      = 203,

    not_stored   = 400,
    exists       = 401,
    not_found    = 404,

    error        = 500,
    client_error = 501,
    server_error = 502,
    empty        = 503,
    io_error     = 504,
    syntax       = 505,
    invalid      = 506,

    unrecognized = 1000,
};

} // namespace resp

/** Base class for all errors in libmcache::proto.
 */
class error_t: public mc::error_t {
public:
    /** C'tor.
     */
    error_t(resp::response_code_t value, const std::string &msg)
        : mc::error_t(value, msg)
    {}
};

} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_ERROR_H */

