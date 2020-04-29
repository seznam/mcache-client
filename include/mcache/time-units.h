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

inline seconds_t seconds_since_epoch(const time_point_t &tp) {
    return std::chrono::duration_cast<seconds_t>(tp.time_since_epoch());
}

} // namespace mc

#endif /* MCACHE_TIME_UNITS_H */

