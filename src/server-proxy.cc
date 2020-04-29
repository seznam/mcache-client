/*
 * FILE             $Id: $
 *
 * DESCRIPTION      Memcache server proxy responsible for handling
 *                  dead server from pool.
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
 *       2012-11-02 (bukovsky)
 *                  First draft.
 */

#include <sstream>

#include "error.h"
#include "mcache/server-proxy.h"

namespace mc {
namespace aux {

void log_server_raise_zombie(const std::string &srv,
                             seconds_t restoration_interval)
{
    LOG(INFO3, "Restoration timeout expired - trying connect to server: "
               "name=%s, new-restoration-attempt=%ld",
               srv.c_str(), restoration_interval.count());
}

void log_server_is_dead(const std::string &srv, uint32_t fail_limit,
                        seconds_t restoration_interval,
                        const std::string &reason)
{
    LOG(WARN2, "Server is marked as dead - restoration in a few seconds: "
               "name=%s, fails=%d, restoration=%ld, reason=%s",
               srv.c_str(), fail_limit, restoration_interval.count(),
               reason.c_str());
}

std::string make_state_string(const std::string &srv,
                              std::size_t connections,
                              seconds_t restoration_interval,
                              uint32_t fails,
                              uint32_t dead)
{
    std::ostringstream os;
    os << srv
       << " [connections-in-pool=" << connections
       << ", new-restoration-attempt=" << restoration_interval.count()
       << ", fails=" << fails
       << ", dead=" << dead << "]";
    return os.str();
}

} // namespace aux
} // namespace mc

