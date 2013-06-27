/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Geoff Pike and Jyrki Alakuijala hash function 
 *                  based on murmur.
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

#ifndef MCACHE_HASH_CITY_H
#define MCACHE_HASH_CITY_H

#include <stdint.h>
#include <string>

namespace mc {

/** Geoff Pike and Jyrki Alakuijala hash function.
 * @param buf pointer to data buffer.
 * @param size size of data.
 * @param seed seed for hash algorithm.
 * @return calculated 32bit hash.
 */
uint32_t city(const void *buf, std::size_t size, uint32_t seed = 0);

/** Geoff Pike and Jyrki Alakuijala hash function.
 * @param str data buffer.
 * @param seed seed for hash algorithm.
 * @return calculated 32bit hash.
 */
inline uint32_t city(const std::string &str, uint32_t seed = 0) {
    return city(str.data(), str.size(), seed);
}

/** Type wrapper for city hash fucntions.
 */
class city_t {
public:
    /** Geoff Pike and Jyrki Alakuijala hash function.
     * @param str data buffer.
     * @return calculated 32bit hash.
     */
    inline uint32_t
    operator()(const std::string &str, uint32_t seed = 0) const {
        return city(str.data(), str.size(), seed);
    }
};

} // namespace mc

#endif /* MCACHE_HASH_CITY_H */

