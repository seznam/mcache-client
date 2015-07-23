#
# FILE             $Id$
#
# DESCRIPTION      Project makefile.
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

gitver = $(shell git describe --all --dirty --long --abbrev=40)

AM_CPPFLAGS = -DGIT_REVISION="\"Configure=@GIT_REVISION@,Make=$(gitver)\""     \
              -DGIT_REVISION_N="\"$(gitver)\""                                 \
              -I@top_builddir@ -I@top_srcdir@/src -I$(top_srcdir)/include      \
	      -DBOOST_ASIO_DISABLE_MOVE -DBOOST_ASIO_DISABLE_STD_ARRAY         \
	      -DBOOST_ASIO_DISABLE_STD_ATOMIC -DBOOST_ASIO_DISABLE_STD_CHRONO  \
	      @LIBTBB_CFLAGS@


AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libmcache.la

SRC = mcache.cc logger.cc error.cc init.cc server-proxy.cc                     \
                                                                               \
      pool/consistent-hashing.cc                                               \
                                                                               \
      io/connection.cc                                                         \
                                                                               \
      proto/txt.cc proto/binary.cc                                             \
                                                                               \
      hash/city.cc hash/jenkins.cc hash/murmur3.cc hash/spooky.cc

libmcache_la_LIBADD = @LIBTBB_LIBS@
libmcache_la_LDFLAGS = @VERSION_INFO@
libmcache_la_SOURCES = $(SRC)

EXTRA_PROGRAMS = test-mcache test-pool test-consistent-hashing                 \
		 test-server-proxy test-connections test-connection test-txt   \
		 test-synchronization test-binary

test_mcache_SOURCES = test-mcache.cc
test_mcache_LDADD = libmcache.la @BOOST_SYSTEM@ @BOOST_THREAD@ @LIBTBB_LIBS@

test_pool_SOURCES = pool/test-pool.cc
test_pool_LDADD = libmcache.la @BOOST_SYSTEM@ @BOOST_THREAD@ @LIBTBB_LIBS@

test_consistent_hashing_SOURCES = pool/test-consistent-hashing.cc
test_consistent_hashing_LDADD = libmcache.la @BOOST_SYSTEM@ @BOOST_THREAD@     \
				@LIBTBB_LIBS@

test_server_proxy_SOURCES = test-server-proxy.cc
test_server_proxy_LDADD = libmcache.la @BOOST_SYSTEM@ @BOOST_THREAD@           \
			  @LIBTBB_LIBS@

test_connections_SOURCES = io/test-connections.cc
test_connections_LDADD = libmcache.la @BOOST_SYSTEM@ @BOOST_THREAD@            \
			 @LIBTBB_LIBS@

test_connection_SOURCES = io/test-connection.cc
test_connection_LDADD = libmcache.la @BOOST_SYSTEM@ @READLINE_LIB@             \
			@BOOST_THREAD@ @LIBTBB_LIBS@

test_txt_SOURCES = proto/test-txt.cc
test_txt_LDADD = libmcache.la @BOOST_SYSTEM@ @BOOST_THREAD@ @LIBTBB_LIBS@

test_synchronization_SOURCES = test-synchronization.cc
test_synchronization_LDADD = libmcache.la @BOOST_SYSTEM@ @BOOST_THREAD@        \
			     @LIBTBB_LIBS@

test_binary_SOURCES = proto/test-binary.cc
test_binary_LDADD = libmcache.la @BOOST_SYSTEM@ @BOOST_THREAD@ @LIBTBB_LIBS@

AM_COLOR_TESTS = always

TESTS = test-pool test-server-proxy test-connections test-txt                  \
	test-synchronization test-binary

