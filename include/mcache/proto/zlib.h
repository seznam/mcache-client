/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: zlib stuff.
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
 *       2016-03-03 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_PROTO_ZLIB_H
#define MCACHE_PROTO_ZLIB_H

namespace mc {
namespace proto {
namespace zlib {

/** Returns compressed data.
 */
std::string compress(const std::string &data);

/** Returns uncompressed data.
 */
std::string
uncompress(const std::string &data,
           std::string::size_type index = 0,
           std::string::size_type count = std::string::npos);

} // namespace zlib
} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_ZLIB_H */

