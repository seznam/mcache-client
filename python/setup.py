#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# FILE             $Id: $
# 
# DESCRIPTION      Python module.
# 
# PROJECT          Seznam memcache client.
# 
# LICENSE          See COPYING
# 
# AUTHOR           Michal Bukovsky <michal.bukovsky@firma.seznam.cz>
# 
# Copyright (C) Seznam.cz a.s. 2012
# All Rights Reserved
# 
# HISTORY
#       2012-11-19 (bukovsky)
#                  First draft.
# 

import os
import sys
from distutils.core import setup
from distutils.core import Extension

# detect python version
version = []
if hasattr(sys.version_info, 'major'):
    version.append(sys.version_info.major)
    version.append(sys.version_info.minor)
else:
    version = sys.version_info[0:2]

# detect boost_python library name
pattern = 'ld -o /dev/null --allow-shlib-undefined -lXXX > /dev/null 2>&1'
boost_python = 'boost_python%d%d' % (version[0], version[1])
if os.system(pattern.replace('XXX', boost_python)) != 0:
    boost_python = 'boost_python-py%d%d' % (version[0], version[1])
    if os.system(pattern.replace('XXX', boost_python)) != 0:
        boost_python = 'boost_python-%d.%d' % (version[0], version[1])
        if os.system(pattern.replace('XXX', boost_python)) != 0:
            boost_python = 'boost_python-%d.%d' % (version[0], version[1])
            if os.system(pattern.replace('XXX', boost_python)) != 0:
                print('can\'t find boost_python library')
                sys.exit(1)
print('checking boost_python library name: ' + boost_python)

# initialize setup
setup(name='mcache',
      version='1.0.5',
      description='Python wrapper around libmcache - memcache client library',
      author='Michal Bukovsky',
      author_email='michal.bukovsky@firma.seznam.cz',
      url='http://cml.kancelar.seznam.cz/email',
      ext_modules=[Extension('mcache',
                             ['mcache.cc'],
                             libraries=[boost_python,
                                        'boost_system',
                                        'boost_thread',
                                        'z',
                                        'mcache'],
                             extra_compile_args=['-W',
                                                 '-Wall',
                                                 '-Wextra',
                                                 '-Wconversion',
                                                 '-std=c++14'])])

