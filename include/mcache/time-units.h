/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Type shortcuts.
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

#ifndef MCACHE_TIME_UNITS_H
#define MCACHE_TIME_UNITS_H

#include <chrono>

namespace mc {

using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using seconds_t = std::chrono::seconds;
using milliseconds_t = std::chrono::milliseconds;
using time_point_t = std::chrono::system_clock::time_point;

} // namespace mc

#endif /* MCACHE_TIME_UNITS_H */

