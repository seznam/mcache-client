#
# FILE             $Id$
#
# DESCRIPTION      Project configure.
#
# PROJECT          Seznam memcache client.
#
# AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
#
# Copyright (C) Seznam.cz a.s. 2012
# All Rights Reserved
#
# HISTORY
#       2012-09-16 (bukovsky)
#                  First draft.
#

# initialize autoconf
AC_INIT([libmcache], [0.2.17], [michal.bukovsky@firma.seznam.cz])
VERSION_INFO="-version-info 0:0:0"

# Enable silent builds (disable with --disable-silent-rules)
# or simply do "make V=1"
AM_SILENT_RULES([yes])

# install autotools scripts to build-aux dir
AC_CONFIG_AUX_DIR([build-aux])

# macros from http://www.gnu.org/software/autoconf-archive/The-Macros.html
AC_CONFIG_MACRO_DIR([m4])

# initialize automake(use AC_INIT's arguments)
AM_INIT_AUTOMAKE(-Wall -Wno-portability foreign color-tests)

# tell autoconf where to find sources
AC_CONFIG_SRCDIR([src/mcache.cc])

dnl This forces debian autoconf to use version 2.53
AC_PREREQ(2.50)

# replace autotools original defaults
: ${CXXFLAGS="-W -Wall -Wextra -Wconversion -Wno-sign-conversion -fPIC -fno-omit-frame-pointer -g -ggdb -std=c++14"}
: ${LDFLAGS="-pthread -rdynamic"}

# disable so
AC_DISABLE_SHARED

# initialize libtool
AC_PROG_LIBTOOL

# Add a test for a compiler.
AC_PROG_CC
AC_PROG_CXX

# Check for gfile for solaris
AC_CHECK_PROG([file], [gfile], [gfile], [file])

AC_ARG_ENABLE(package-version, AS_HELP_STRING([--enable-package-version], [specify package version]), [
    case "${enableval}" in
        yes)
            AC_MSG_ERROR([You must specify package version.])
        ;;
        no)
            AC_MSG_NOTICE([disabling Debian package version.])
        ;;
        *)
            AC_MSG_NOTICE([setting Debian package to "${enableval}."])
            CPPFLAGS="${CPPFLAGS} -DDEBIAN_PACKAGE_VERSION=\"\\\"${enableval}\\\"\""
        ;;
    esac
])

AC_ARG_ENABLE(optimization, AS_HELP_STRING([--enable-optimization], [compile optimized without debug logging]), [
    case "${enableval}" in
        no)
            AC_MSG_NOTICE([disabling optimization.])
            CXXFLAGS="${CXXFLAGS} -O0 -g -ggdb"
        ;;
        yes)
            AC_MSG_NOTICE([enabling optimization.])
            CXXFLAGS="${CXXFLAGS} -O2 -g -ggdb -fno-omit-frame-pointer"
        ;;
        *)
            AC_MSG_ERROR([Say yes or no to --enable-optimization.])
        ;;
    esac
], [
   CXXFLAGS="${CXXFLAGS} -O0 -g -ggdb"
])

AC_ARG_ENABLE([debug], AS_HELP_STRING([--enable-debug], [Enable the debuging logging]), [
    case "${enableval}" in
        no)
            AC_MSG_NOTICE([disabling debug log.])
            CPPFLAGS="${CPPFLAGS} -UDEBUG -DNDEBUG=1"
        ;;
        yes)
            AC_MSG_NOTICE([enabling debug log.])
            CPPFLAGS="${CPPFLAGS} -DDEBUG=1 -UNDEBUG"
        ;;
        *)
            AC_MSG_ERROR([Say yes or no to --enable-debug.])
        ;;
    esac
],
    [AS_IF([test x$enable_optimization == xyes],
        [CPPFLAGS="${CPPFLAGS} -UDEBUG -DNDEBUG=1"],
        [CPPFLAGS="${CPPFLAGS} -DDEBUG=1 -UNDEBUG"])
])

# check pkgconfig
PKG_PROG_PKG_CONFIG

# Doxygen support.
DX_INIT_DOXYGEN([$PACKAGE_NAME], [$(top_srcdir)/doc/Doxyfile.in])
DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)

# check boost system
AC_LANG_CPLUSPLUS
AC_CHECK_HEADER([boost/system/config.hpp], [], AC_MSG_WARN([boost system headers missing]))
AC_CHECK_LIB(boost_system-mt, main, BOOST_SYSTEM=-lboost_system-mt, [
    AC_CHECK_LIB(boost_system, main, BOOST_SYSTEM=-lboost_system,
                 [AC_MSG_WARN([boost system library missing])])])

# check for boost threads
AC_LANG_CPLUSPLUS
AC_CHECK_HEADER([boost/thread.hpp], [], AC_MSG_WARN([boost thread headers missing]))
AC_CHECK_LIB(boost_thread-mt, main, BOOST_THREAD=-lboost_thread-mt, [
    AC_CHECK_LIB(boost_thread, main, BOOST_THREAD=-lboost_thread,
                 [AC_MSG_WARN([boost thread library missing])])])

# check fore readline
AC_LANG_C
AC_CHECK_HEADER([readline/readline.h], [AC_DEFINE([HAVE_READLINE_H], [1])], [])
AC_CHECK_LIB(readline, main, [READLINE_LIB="-lreadline"], [])

# check fore zlib
AC_LANG_C
AC_CHECK_HEADER([zlib.h], [], AC_MSG_WARN([zlib headers missing]))
AC_CHECK_LIB(z, main, [ZLIB_LIBS=-lz], [AC_MSG_WARN([zlib library missing])])

## Check for LIBTBB
AC_LANG_CPLUSPLUS
PKG_CHECK_MODULES([LIBTBB], [tbb], [has_tbb=yes AC_DEFINE([HAVE_LIBTBB])], [has_tbb=no])
# for debian system with older tbb which does not support pkg_config
AS_IF([test "x$has_tbb" = "xno"], [
       AC_CHECK_HEADERS([tbb/concurrent_queue.h], [has_tbb_include=yes], [])
       AS_IF([test "x$has_tbb_include" = "xyes"], [
           AC_CHECK_LIB(tbb, main, [AC_DEFINE([HAVE_LIBTBB])], [])
       ], [])
], [])

LIBMCACHE_EXTRA_LIBS="$BOOST_SYSTEM $BOOST_THREAD $ZLIB_LIBS $LIBTBB_LIBS"

GIT_REVISION="`git describe --all --dirty --long --abbrev=40`"

AC_SUBST(GIT_REVISION)
AC_SUBST(VERSION_INFO)
AC_SUBST(BOOST_SYSTEM)
AC_SUBST(BOOST_THREAD)
AC_SUBST(READLINE_LIB)
AC_SUBST(LIBMCACHE_EXTRA_LIBS)
AC_SUBST(ZLIB_LIBS)

# for tests
AC_SEARCH_LIBS(dlopen, dl)

# Write config.status and the Makefile
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 doc/Doxyfile
                 include/mcache/Makefile
                 include/mcache/pool/Makefile
                 include/mcache/hash/Makefile
                 include/mcache/io/Makefile
                 include/mcache/proto/Makefile
                 version
                 libmcache.pc])

AC_OUTPUT()
