/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Memcache server proxy responsible for handling
 *                  dead server from pool.
 *
 * PROJECT          Seznam memcache client.
 *
 * AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
 *
 * Copyright (C) Seznam.cz a.s. 2012
 * All Rights Reserved
 *
 * HISTORY
 *       2012-11-02 (bukovsky)
 *                  First draft.
 */

#include "error.h"
#include "mcache/server-proxy.h"

namespace mc {

void log_server_raise_zombie(const std::string &srv, time_t restoration) {
    LOG(INFO3, "Restoration timeout expired - trying connect to server: "
               "name=%s, new-restoration-attempt=%ld",
               srv.c_str(), restoration);
}

void log_server_is_dead(const std::string &srv, uint32_t fail_limit,
                        time_t restoration)
{
    LOG(WARN2, "Server is marked as dead - restoration in a few seconds: "
               "name=%s, fails=%d, restoration=%ld",
               srv.c_str(), fail_limit, restoration);
}

} // namespace mc

