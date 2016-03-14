#
# $Id$
#
# DESCRIPTION      Project Makefile template.
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
#                  First version

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src                                                                  \
          include/mcache/pool                                                  \
	  include/mcache/hash                                                  \
	  include/mcache/io                                                    \
	  include/mcache/proto                                                 \
	  include/mcache

pkgconfigdir = @libdir@/pkgconfig
pkgconfig_DATA = libmcache.pc

@DX_RULES@

MOSTLYCLEANFILES = $(DX_CLEANFILES)
