/*
 * FILE             $Id$
 *
 * DESCRIPTION      For the present, it is there only for configure.in.
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

extern "C" {

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "UNKNOWN"
#endif

#ifndef GIT_REVISION
#define GIT_REVISION "UNKNOWN"
#endif

/// compiled in package version info
const char MCACHE_PACKAGE_VERSION_SYMBOL[] = PACKAGE_VERSION;

/// compiled in revision info
const char MCACHE_GIT_REVISION_SYMBOL[] = GIT_REVISION;

/// present symbol
const char *mcache_present() {
    return "MCACHE=" GIT_REVISION "\tGIT_REVISION_FOR_STRINGS";
}

} // extern "C"

