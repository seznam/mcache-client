#!/usr/bin/env python

import sys
from distutils.core import setup
from distutils.core import Extension

version = str(sys.version_info.major) + '.' + str(sys.version_info.minor)

setup(name='mcache',
      version='0.1.0',
      description='Python wrapper around libmcache - memcache client library',
      author='Michal Bukovsky',
      author_email='michal.bukovsky@firma.seznam.cz',
      url="http://cml.kancelar.seznam.cz/email",
      ext_modules=[Extension('mcache',
                             ['mcache.cc'],
                             libraries=['boost_python-' + version,
                                        'boost_system',
                                        'boost_thread',
                                        'mcache'],
                             extra_compile_args=['-W',
                                                 '-Wall',
                                                 '-Wextra',
                                                 '-Wconversion'])])

