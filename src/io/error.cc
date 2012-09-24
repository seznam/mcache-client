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
 *       2012-09-16 (bukovsky)
 *                  First draft.
 */

#include <string>

#include "mcache/logger.h"
#include "error.h"

namespace mc {
namespace io {

std::string ErrorCategory_t::message(int ev) const {
    switch (ev) {
    case SYNTAX_ERROR:
        return "syntax error";
    case NOT_FOUND:
        return "not found";
    case TIMEOUT:
        return "request timeouted";
    case INTERNAL_ERROR:
        return "internal error";
    // no default g++ provide us with useful warning if someone add next error
    }
    return "unknown error";
}

/** Return mcache error category
 */
const serr::ErrorCategory_t &getErrorCategory() {
    static const ErrorCategory_t category;
    return category;
}

} // namespace io
} // namespace mc

