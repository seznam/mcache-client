project('libmcache', 'cpp',
       version: '1.0.5',
       default_options: [
         'cpp_std=c++14',
          'warning_level=3',
          'buildtype=debugoptimized'
       ])

pkg = import('pkgconfig')

lib_deps = [
  dependency('boost-system'),
  dependency('boost-thread'),
  dependency('readline'),
  dependency('zlib'),
  dependency('tbb')
]

git = find_program('git', required : false)
git_rev = 'no revision'
if git.found()
    git_rev = run_command('git', 'describe', '--all', '--dirty', '--long',
        '--abbrev=40', check: true).stdout().strip()
endif

add_project_arguments('-DGIT_REVISION="' + git_rev + '"', language: 'cpp')
add_project_arguments('-DHAVE_READLINE_H', language: 'cpp')
add_project_arguments('-DHAVE_LIBTBB', language: 'cpp')

headers = files('''
    include/mcache/client.h
    include/mcache/error.h
    include/mcache/hash.h
    include/mcache/init.h
    include/mcache/lock.h
    include/mcache/logger.h
    include/mcache/mcache.h
    include/mcache/server-proxies.h
    include/mcache/server-proxy.h
    include/mcache/conversion.h
    include/mcache/has-member.h
    include/mcache/time-units.h
    include/mcache/fallthrough.h
'''.split())

sources = files('''
    src/mcache.cc
    src/logger.cc
    src/error.cc
    src/init.cc
    src/server-proxy.cc
    src/pool/consistent-hashing.cc
    src/io/connection.cc
    src/proto/txt.cc
    src/proto/binary.cc
    src/proto/zlib.cc
    src/hash/city.cc
    src/hash/jenkins.cc
    src/hash/murmur3.cc
    src/hash/spooky.cc
'''.split())

inc = include_directories('src', 'include')
lib = static_library('mcache', sources,
                     dependencies: lib_deps,
                     install: true,
                     include_directories: inc)

pkg.generate(lib, name: 'libmcache')

libmcache_dep = declare_dependency(
  include_directories : include_directories('include'),
  link_with : lib)
meson.override_dependency('libmcache', libmcache_dep)

#### TESTS ####
test_inc = include_directories('include')

# outdated?
# test_mcache = executable('test-mcache', 'src/test-mcache.cc',
#                          link_with: lib,
#                          dependencies: lib_deps,
#                          include_directories: test_inc)

test_pool = executable('test-pool', 'src/pool/test-pool.cc',
                       link_with: lib,
                       dependencies: lib_deps,
                       include_directories: test_inc)

# outdated?
# test_consistent_hashing = executable('test-constistent-hashing',
#                                      'src/pool/test-consistent-hashing.cc',
#                                      link_with: lib,
#                                      dependencies: lib_deps,
#                                      include_directories: test_inc)

test_server_proxy = executable('test-server-proxy',
                               'src/test-server-proxy.cc',
                               link_with: lib,
                               dependencies: lib_deps,
                               include_directories: test_inc)

test_connections = executable('test-connections',
                              'src/io/test-connections.cc',
                              link_with: lib,
                              dependencies: lib_deps,
                              include_directories: test_inc)

test_connection = executable('test-connection',
                             'src/io/test-connection.cc',
                             link_with: lib,
                             dependencies: lib_deps,
                             include_directories: test_inc)

test_txt = executable('test-txt',
                      'src/proto/test-txt.cc',
                      link_with: lib,
                      dependencies: lib_deps,
                      include_directories: test_inc)

test_synchronization = executable('test-synchronization',
                                  'src/test-synchronization.cc',
                                  link_with: lib,
                                  dependencies: lib_deps,
                                  include_directories: test_inc)


test_binary = executable('test-binary',
                         'src/proto/test-binary.cc',
                         link_with: lib,
                         dependencies: lib_deps,
                         include_directories: test_inc)

test('test-pool', test_pool)
test('test-server-proxy', test_server_proxy)
test('test-connections', test_connections)
test('test-txt', test_txt)
test('test-synchronization', test_synchronization)
test('test-binary', test_binary)
