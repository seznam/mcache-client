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
 *       2012-09-24 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_PROTO_BINARY_H
#define MCACHE_PROTO_BINARY_H

#include <string>

#include <mcache/proto/opts.h>
#include <mcache/proto/response.h>

namespace mc {
namespace proto {
namespace bin {

/** 
 */
class test_t {
public:
    typedef single_retrival_response_t response_t;

    test_t(const std::string &key): key(key) {}

    std::size_t header_delimiter() const { return 24;}

    std::string serialize() const;

    response_t deserialize_header(const std::string &) const;

    std::string key;
};

/** Used as namespace with class behaviour for protocol api.
 */
class api {
public:
    // protocol api table
    typedef test_t get_t;
};

} // namespace bin
} // namespace proto
} // namespace mc

#endif /* MCACHE_PROTO_BINARY_H */

