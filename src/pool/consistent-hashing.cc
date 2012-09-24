/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Consistent hashing implementation.
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

#include <algorithm>
#include <sstream>

#include "mcache/pool/consistent-hashing.h"

namespace mc {
namespace {

/** Dumps ring node to string.
 */
class dump_t {
public:
    /** C'tor.
     */
    dump_t(std::string &result): result(result) {}

    /** Dumps ring node to string.
     */
    template <typename Node_t>
    void operator()(const Node_t &node) const {
        std::ostringstream os;
        os << "[" << node.first << "] -> " << node.second << std::endl;
        result.append(os.str());
    }

private:
    std::string &result; //!< target string
};

} // namespace

std::string consistent_hashing_pool_base_t::dump(const ring_t &ring) const {
    std::string result;
    std::for_each(ring.begin(), ring.end(), dump_t(result));
    return result;
}

} // namespace mc

