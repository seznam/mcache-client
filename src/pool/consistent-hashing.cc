/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Consistent hashing implementation.
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
    dump_t(const std::vector<std::string> &states, std::string &result)
        : result(result), states(states)
    {}

    /** Dumps ring node to string.
     */
    template <typename Node_t>
    void operator()(const Node_t &node) const {
        std::ostringstream os;
        os << "[" << node.first << "] -> " << desc(node.second) << std::endl;
        result.append(os.str());
    }

    /** Makes proxy description or dump idx only.
     */
    std::string desc(uint32_t idx) const {
        std::ostringstream os;
        os << idx;
        if (idx < states.size()) os << " {" << states[idx] << "}";
        return os.str();
    }

private:
    std::string &result;                    //!< target string
    const std::vector<std::string> &states; //!< proxies states
};

} // namespace

std::string
consistent_hashing_pool_base_t
::dump(const ring_t &ring, const std::vector<std::string> &states) const {
    std::string result;
    std::for_each(ring.begin(), ring.end(), dump_t(states, result));
    return result;
}

} // namespace mc

