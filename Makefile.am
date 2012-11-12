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

SUBDIRS = src doc                                                              \
          include/mcache/pool                                                  \
	  include/mcache/hash                                                  \
	  include/mcache/io                                                    \
	  include/mcache/proto                                                 \
	  include/mcache

pkgconfigdir = @libdir@/pkgconfig
pkgconfig_DATA = libmcache.pc

doxygen:
	$(MAKE) --directory=doc doxygen-doc

