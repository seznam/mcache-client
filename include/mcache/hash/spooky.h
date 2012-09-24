/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Bob Jenkins spooky V2 hash function.
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

#ifndef MCACHE_HASH_SPOOKY_H
#define MCACHE_HASH_SPOOKY_H

#include <stdint.h>
#include <string>

namespace mc {

/** Bob Jenkins spooky V2 hash function.
 * @param buf pointer to data buffer.
 * @param size size of data.
 * @param seed seed for hash algorithm.
 * @return calculated 32bit hash.
 */
uint32_t spooky(const void *buf, std::size_t size, uint32_t seed = 0);

/** Bob Jenkins spooky V2 hash function.
 * @param str data buffer.
 * @param seed seed for hash algorithm.
 * @return calculated 32bit hash.
 */
inline uint32_t spooky(const std::string &str, uint32_t seed = 0) {
    return spooky(str.data(), str.size(), seed);
}

/** Type wrapper for spooky hash fucntions.
 */
class spooky_t {
public:
    /** Bob Jenkins spooky V2 hash function.
     * @param str data buffer.
     * @param seed seed for hash algorithm.
     * @return calculated 32bit hash.
     */
    inline uint32_t
    operator()(const std::string &str, uint32_t seed = 0) const {
        return spooky(str.data(), str.size(), seed);
    }
};

} // namespace mc

#endif /* MCACHE_HASH_SPOOKY_H */

