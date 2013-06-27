/*
 * FILE             $Id$
 *
 * DESCRIPTION      Memcache client library logger functions.
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

#ifndef MCACHE_LOGGER_H
#define MCACHE_LOGGER_H

namespace mc {

/// log function type
typedef void (* log_function_t)
    (int, const char *, const char *, int, const char *, ...);

/// library logger
extern log_function_t logger;

} // namespace mc

#endif /* MCACHE_LOGGER_H */

