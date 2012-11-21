#!/usr/bin/env python

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
boost_python = 'boost_python-py%d%d' % (version[0], version[1])
if os.system(pattern.replace('XXX', boost_python)) != 0:
    boost_python = 'boost_python-%d.%d' % (version[0], version[1])
    if os.system(pattern.replace('XXX', boost_python)) != 0:
        print('can\'t find boost_python library')
print('checking boost_python library name: ' + boost_python)

# initialize setup
setup(name='mcache',
      version='0.1.0',
      description='Python wrapper around libmcache - memcache client library',
      author='Michal Bukovsky',
      author_email='michal.bukovsky@firma.seznam.cz',
      url='http://cml.kancelar.seznam.cz/email',
      ext_modules=[Extension('mcache',
                             ['mcache.cc'],
                             libraries=[boost_python,
                                        'boost_system',
                                        'boost_thread',
                                        'mcache'],
                             extra_compile_args=['-W',
                                                 '-Wall',
                                                 '-Wextra',
                                                 '-Wconversion'])])

