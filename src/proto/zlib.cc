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

#include <cstdint>
#include <vector>
#include <limits>
#include <zlib.h>

#include "mcache/error.h"
#include "mcache/proto/zlib.h"

namespace mc {
namespace proto {
namespace zlib {
namespace {

template <typename Int_t>
Bytef *b(Int_t *p) {
    return reinterpret_cast<Bytef *>(p);
}

template <typename Int_t>
const Bytef *b(const Int_t *p) {
    return reinterpret_cast<const Bytef *>(p);
}

} // namespace

std::string compress(const std::string &data) {
    // estimate size of compress data and allocate buffer
    auto dst_size = ::compressBound(data.size());
    std::vector<uint8_t> dst(dst_size);
    if (dst_size > std::numeric_limits<uint32_t>::max())
        throw error_t(mc::err::bad_request, "too large data");

    // compress
    if (::compress(b(&dst[0]), &dst_size, b(data.data()), data.size()) != Z_OK)
        throw error_t(mc::err::bad_request, "zlib compress error");

    // make the result string
    uint32_t src_size = static_cast<uint32_t>(data.size());
    auto ipos = reinterpret_cast<char *>(&src_size);
    std::string res(ipos, ipos + sizeof(uint32_t));
    res.append(dst.begin(), dst.begin() + dst_size);
    return res;
}

std::string
uncompress(const std::string &data,
           std::string::size_type index,
           std::string::size_type count)
{
    // fix data boundaries
    auto isrc = data.data() + index;
    auto src_size = std::min(data.size() - index, count);

    // read size of the uncompressed data and prepare result buffer
    uint64_t dst_size = *reinterpret_cast<const uint32_t *>(isrc);
    if (dst_size > std::numeric_limits<uint32_t>::max())
        throw error_t(mc::err::bad_request, "too large data");
    isrc += sizeof(uint32_t);
    src_size -= sizeof(uint32_t);
    std::vector<uint8_t> dst(dst_size);

    // uncompress
    if (::uncompress(b(&dst[0]), &dst_size, b(isrc), src_size) != Z_OK)
        throw error_t(mc::err::bad_request, "zlib uncompress error");
    return std::string(dst.begin(), dst.begin() + dst_size);
}

} // namespace zlib
} // namespace proto
} // namespace mc

