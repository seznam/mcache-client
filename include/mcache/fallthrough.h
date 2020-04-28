/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Fallthrough macro.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2020
 * All Rights Reserved
 *
 * HISTORY
 *       2020-04-28 (burlog)
 *                  First draft.
 */

#ifndef MCACHE_FALLTHROUGH_H
#define MCACHE_FALLTHROUGH_H 1

#if __cplusplus <= 201402L

#ifdef __clang_major__

#if __clang_major__ < 4
#define MCACHE_FALLTHROUGH
#else /* __clang_major__ < 4 */
#define MCACHE_FALLTHROUGH [[clang::fallthrough]]
#endif /* __clang_major__ < 4 */

#else /* __clang_major__ */

#if __GNUC__ < 7
#define MCACHE_FALLTHROUGH
#else /* __GNUC__ < 7 */
#define MCACHE_FALLTHROUGH [[gnu::fallthrough]]
#endif /* __GNUC__ < 7 */

#endif /* __clang_major__ */

#else /*  __cplusplus <= 201402L */
#define MCACHE_FALLTHROUGH [[fallthrough]]
#endif /* __cplusplus <= 201402L */

#endif  /* MCACHE_FALLTHROUGH_H */
