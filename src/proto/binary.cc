/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Command serialization/deserialization: binary protocol
 *                  implementation base class.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-26 (bukovsky)
 *                  First draft.
 */

#include <sstream>
#include <arpa/inet.h>

#include "error.h"
#include "mcache/proto/binary.h"

namespace mc {
namespace proto {
namespace bin {

test_t::response_t
test_t::deserialize_header(const std::string &header) const {
    std::size_t extra_len = header[4];
    std::size_t body_len = ::ntohl(reinterpret_cast<const uint32_t *>
            (header.data())[2]);
    LOG(INFO3, "LEN.... %zd %zd", extra_len , body_len);
    return response_t(0, body_len, 0);
}

std::string test_t::serialize() const {
    std::ostringstream os;
    os << char(0x80) << char(0x00) << char(0x00) << char(0x05)
       << char(0x00) << char(0x00) << char(0x00) << char(0x00)
       << char(0x00) << char(0x00) << char(0x00) << char(0x05)
       << char(0x00) << char(0x00) << char(0x00) << char(0x00)
       << char(0x00) << char(0x00) << char(0x00) << char(0x00)
       << char(0x00) << char(0x00) << char(0x00) << char(0x00)
       << char(0x48) << char(0x65) << char(0x6c) << char(0x6c)
       << char(0x6f);
    return os.str();
}

} // namespace bin
} // namespace proto
} // namespace mc

