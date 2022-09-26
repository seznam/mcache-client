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

#include <unistd.h>
#include <string>
#include <cstdarg>
#include <dlfcn.h>
#include <cstdio>
#include <ctime>

#include "error.h"
#include "mcache/logger.h"

namespace mc {

/// library logger
log_function_t logger;

/** Default logger.
 */
void stderrLogger(int,
                  const char *file,
                  const char *function,
                  int line,
                  const char *format, ...)
{
    va_list valist;
    va_start(valist, format);

    // log time and pid -- ignore error
    time_t lt = 0;
    time(&lt);
    char strtime[21] = "notime";
    struct tm ltm;
    strftime(strtime, sizeof(strtime),
             "%Y/%m/%d %H:%M:%S", localtime_r(&lt, &ltm));
    fprintf(stderr, "%s [%d] ", strtime, getpid());

    // log information
    vfprintf(stderr, format, valist);

    // log place
    fprintf(stderr, " {%s:%s():%d}\n", file, function, line);

    va_end(valist);
}

/** Initializer.
 */
static class Initializer_t {
public:
    /** Initialize library.
     */
    Initializer_t() {
        // try get dbglog symbol
        if (void *dbglog = dlsym(RTLD_DEFAULT, "__dbglog")) {
            mc::logger = (mc::log_function_t)(dbglog);
        } else {
            mc::logger = (mc::log_function_t)(stderrLogger);
        }
    }
} initializer;

} // namespace mc
