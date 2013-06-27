/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Proto utils.
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
 *       2012-11-03 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_SRC_PROTO_AUX_H
#define MCACHE_SRC_PROTO_AUX_H

#include <string>
#include <cctype>
#include <algorithm>
#include <boost/bind.hpp>

#include "error.h"
#include "mcache/error.h"

namespace mc {
namespace proto {
namespace aux {

/** Throws if key does not match constraints:
 * The length limit of a key is set at 250 characters (of course, normally
 * clients wouldn't need to use such long keys); the key must not include
 * control characters or whitespace.
 */
inline void check_key(const std::string &key) {
    // check size
    if (key.size() > 250)
        throw mc::error_t(err::bad_argument, "key is too long");

    // check chars
    std::string::const_iterator
        ierr = std::find_if(key.begin(), key.end(),
                            boost::bind(::isspace, _1)
                            || boost::bind(::iscntrl, _1));
    if (ierr != key.end())
        throw mc::error_t(err::bad_argument, "key contains invalid chars");
}

} // namespace aux
} // namespace proto
} // namespace mc

#endif /* MCACHE_SRC_PROTO_AUX_H */

