/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Various hash functions.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-09-17 (bukovsky)
 *                  First draft.
 */

#ifndef MCACHE_HASH_H
#define MCACHE_HASH_H

#include <mcache/hash/jenkins.h>
#include <mcache/hash/murmur3.h>
#include <mcache/hash/city.h>
#include <mcache/hash/spooky.h>

namespace mc {

// TODO(burlog): move single hash function to Hash_t class?

/// type of builtin hash functions
typedef uint32_t (* hash_function_t )(const std::string &);

} // namespace mc

#endif /* MCACHE_HASH_H */

