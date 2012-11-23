/*
 * FILE             $Id$
 *
 * DESCRIPTION      Memcache client library init.
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

#include "mcache/init.h"

namespace mc {
namespace {
bool initialized = false;
}

namespace proto {
namespace bin {

void init();

} // namespace bin
} // namespace proto

void init() {
    if (initialized) return;

    proto::bin::init();
    initialized = true;
}

} // namespace mc

