/*
 * FILE             $Id$
 *
 * DESCRIPTION      Some defines for logger and error handling.
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

#ifndef MCACHE_SRC_ERROR_H
#define MCACHE_SRC_ERROR_H

#include <string>

#include "mcache/error.h"
#include "mcache/logger.h"

namespace mc {

#define LOG(LEVEL, ...) do {                                                   \
        if (mc::logger)                                                        \
            (*mc::logger)(LEVEL, __FILE__, __FUNCTION__, __LINE__,             \
                          __VA_ARGS__);                                        \
} while (false)

#ifdef DEBUG
#ifdef DBG
#undef DBG
#endif
#define DBG(LEVEL, ...) do {                                                   \
        if (mc::logger)                                                        \
            (*mc::logger)(LEVEL, __FILE__, __FUNCTION__, __LINE__,             \
                          __VA_ARGS__);                                        \
} while (false)
#else
#ifndef DBG
#define DBG(LEVEL, ...) {}
#endif
#endif

#define INFO1  0x0000000f
#define INFO2  0x00000007
#define INFO3  0x00000003
#define INFO4  0x00000001

#define WARN1  0x000000f0
#define WARN2  0x00000070
#define WARN3  0x00000030
#define WARN4  0x00000010

#define ERR1   0x00000f00
#define ERR2   0x00000700
#define ERR3   0x00000300
#define ERR4   0x00000100

#define FATAL1 0x0000f000
#define FATAL2 0x00007000
#define FATAL3 0x00003000
#define FATAL4 0x00001000

#define DBG1   0x0f000000
#define DBG2   0x07000000
#define DBG3   0x03000000
#define DBG4   0x01000000

namespace log {

/** Converts character to hex number stored in string.
 */
inline std::string hexize(const unsigned char ch) {
    static const char *HEX = "0123456789abcdef";
    return "%" + std::string(1, HEX[ch >> 4]).append(1, HEX[ch & 0x0f]);
}

/** Converts binary characters in string to hex numbers.
 */
inline std::string escape(const std::string &str) {
    std::string res;
    for (std::string::const_iterator
            istr = str.begin(),
            estr = str.end();
            istr != estr; ++istr)
    {
        if (::isprint(*istr)) res.append(1, *istr);
        else res.append(hexize(static_cast<unsigned char>(*istr)));
    }
    return res;
}

} // namespace log
} // namespace mc

#endif /* MCACHE_SRC_ERROR_H */
