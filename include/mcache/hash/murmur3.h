/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Austin Appleby hash function.
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

#ifndef MCACHE_HASH_MURMUR3_H
#define MCACHE_HASH_MURMUR3_H

#include <stdint.h>
#include <string>

namespace mc {

/** Austin Appleby hash function.
 * @param buf pointer to data buffer.
 * @param size size of data.
 * @param seed seed for hash algorithm.
 * @return calculated 32bit hash.
 */
uint32_t murmur3(const void *buf, std::size_t size, uint32_t seed = 0);

/** Austin Appleby hash function.
 * @param str data buffer.
 * @param seed seed for hash algorithm.
 * @return calculated 32bit hash.
 */
inline uint32_t murmur3(const std::string &str, uint32_t seed = 0) {
    return murmur3(str.data(), str.size(), seed);
}

/** Type wrapper for mumur3 hash fucntions.
 */
class murmur3_t {
public:
    /** Austin Appleby hash function.
     * @param str data buffer.
     * @param seed seed for hash algorithm.
     * @return calculated 32bit hash.
     */
    inline uint32_t
    operator()(const std::string &str, uint32_t seed = 0) const {
        return murmur3(str.data(), str.size(), seed);
    }
};

} // namespace mc

#endif /* MCACHE_HASH_MURMUR3_H */

