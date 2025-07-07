project(
  'libmcache',
  'cpp',
  version: '1.0.13',
  default_options : [
    'cpp_std=c++17',
    'warning_level=2',
    'buildtype=debugoptimized',
    'wrap_mode=nofallback',
  ]
)

add_project_arguments('-DBOOST_ASIO_DISABLE_MOVE', language : 'cpp')
add_project_arguments('-DBOOST_ASIO_DISABLE_STD_ARRAY', language : 'cpp')
add_project_arguments('-DBOOST_ASIO_DISABLE_STD_ATOMIC', language : 'cpp')
add_project_arguments('-DBOOST_ASIO_DISABLE_STD_CHRONO', language : 'cpp')

dependencies = [
  dependency('threads'),
  dependency('dl', required: false),
  dependency('zlib'),
  dependency('tbb', required: false),
  dependency('python3'),
  dependency('boost', modules: ['python3', 'system', 'thread']),
]

includes = include_directories(
  'include',
  'src',
)

headers = [
  'include/mcache/async.h',
  'include/mcache/client.h',
  'include/mcache/conversion.h',
  'include/mcache/error.h',
  'include/mcache/fallthrough.h',
  'include/mcache/has-member.h',
  'include/mcache/hash.h',
  'include/mcache/init.h',
  'include/mcache/lock.h',
  'include/mcache/logger.h',
  'include/mcache/mcache.h',
  'include/mcache/server-proxies.h',
  'include/mcache/server-proxy.h',
  'include/mcache/time-units.h',

  'include/mcache/hash/city.h',
  'include/mcache/hash/jenkins.h',
  'include/mcache/hash/murmur3.h',
  'include/mcache/hash/spooky.h',

  'include/mcache/io/connection.h',
  'include/mcache/io/connections.h',
  'include/mcache/io/error.h',
  'include/mcache/io/opts.h',

  'include/mcache/pool/consistent-hashing.h',
  'include/mcache/pool/mod.h',

  'include/mcache/proto/binary.h',
  'include/mcache/proto/error.h',
  'include/mcache/proto/opts.h',
  'include/mcache/proto/parser.h',
  'include/mcache/proto/response.h',
  'include/mcache/proto/txt.h',
  'include/mcache/proto/zlib.h',
]

sources = [
  'src/error.cc',
  'src/error.h',
  'src/init.cc',
  'src/logger.cc',
  'src/mcache.cc',
  'src/server-proxy.cc',

  'src/hash/city.cc',
  'src/hash/jenkins.cc',
  'src/hash/murmur3.cc',
  'src/hash/spooky.cc',

  'src/io/connection.cc',

  'src/pool/consistent-hashing.cc',

  'src/proto/binary.cc',
  'src/proto/txt.cc',
  'src/proto/zlib.cc',
]

git_revision = 'no revision'
git = find_program('git', required: false)
fs = import('fs')
if git.found() and fs.is_dir('.git')
    res = run_command('git', 'describe', '--all', '--dirty', '--long', '--abbrev=40', check: true)
    git_revision = res.stdout().strip()
endif

add_project_arguments(
  '-DPACKAGE_VERSION="' + meson.project_version() + '"',
  language: ['c', 'cpp']
)

add_project_arguments(
  '-DPACKAGE_NAME="' + meson.project_name() + '"',
  language: ['c', 'cpp']
)

add_project_arguments(
  '-DGIT_REVISION="' + git_revision + '"',
  language: ['c', 'cpp']
)

if get_option('optimization') not in ['1', '2', '3']
  add_project_arguments(
    '-DDEBUG',
    language: ['c', 'cpp']
  )
endif

if get_option('debug')
  add_project_arguments(
    '-ggdb',
    language: ['c', 'cpp']
  )
endif

if dependency('tbb', required: false).found()
  add_project_arguments(
    '-DHAVE_LIBTBB',
    language: ['c', 'cpp']
  )
endif

cxx = meson.get_compiler('cpp')
add_project_arguments(
  cxx.get_supported_arguments(
    [
      '-Wconversion',
      '-Wno-sign-conversion',
      '-Wnon-virtual-dtor',
      '-Woverloaded-virtual',
      '-Wctor-dtor-privacy',
      '-fno-omit-frame-pointer',
      '-ftemplate-backtrace-limit=0',
      '-Wextra-semi',
      '-Wuseless-cast',
      '-Wzero-as-null-pointer-constant',
      '-Wundef',
      '-Wlogical-op',
      '-Wpointer-arith',
      '-Wcast-qual',
      '-Wformat=2',
      '-Wformat-truncation',
      '-Wdisabled-optimization',
      '-Wnull-dereference',
      '-Wduplicated-cond',
      '-fdiagnostics-color=always',
      '-Werror=format',
    ]
  ),
  language: 'cpp'
)

lib = static_library(
  'mcache',
  [sources],
  dependencies: dependencies,
  install: true,
  include_directories: includes,
  link_args: '-rdynamic'
)

install_headers(
  headers,
  subdir: '..',
  preserve_path: true
)

pkg = import('pkgconfig')
pkg.generate(lib, name: 'libmcache')

libmcache_dep = declare_dependency(
  include_directories: includes,
  dependencies: dependencies,
  link_with: lib
)
meson.override_dependency('libmcache', libmcache_dep)

executable(
  'test-mcache',
  dependencies: libmcache_dep,
  sources: 'src/test-mcache.cc',
)

test(
  'test-pool',
  executable(
    'test-pool',
    dependencies: libmcache_dep,
    sources: 'src/pool/test-pool.cc',
  ),
)

test(
  'test-consistent-hashing',
  executable(
    'test-consistent-hashing',
    dependencies: libmcache_dep,
    sources: 'src/pool/test-consistent-hashing.cc',
  ),
)

test(
  'test-server-proxy',
  executable(
    'test-server-proxy',
    dependencies: libmcache_dep,
    sources: 'src/test-server-proxy.cc',
  ),
)

test(
  'test-connections',
  executable(
    'test-connections',
    dependencies: libmcache_dep,
    sources: 'src/io/test-connections.cc',
  ),
)

readline_dep = dependency('readline', required: false)
if readline_dep.found()
  executable(
    'test-connection',
    dependencies: [libmcache_dep, readline_dep],
    sources: 'src/io/test-connection.cc',
  )
endif

test(
  'test-txt',
  executable(
    'test-txt',
    dependencies: libmcache_dep,
    sources: 'src/proto/test-txt.cc',
  ),
)

test(
  'test-binary',
  executable(
    'test-binary',
    dependencies: libmcache_dep,
    sources: 'src/proto/test-binary.cc',
  ),
)

test(
  'test-synchronization',
  executable(
    'test-synchronization',
    dependencies: libmcache_dep,
    sources: 'src/test-synchronization.cc',
  ),
)

if (get_option('docs'))
  doxygen = find_program('doxygen', required: true)
  dot = find_program('dot', required: true)

  input = [meson.project_source_root() / 'doc']
  foreach file: sources + headers
    input += meson.project_source_root() / file
  endforeach

  conf_data = configuration_data()
  conf_data.set('PROJECT', meson.project_name())
  conf_data.set('VERSION', meson.project_version())
  conf_data.set_quoted('REVISION', git_revision)
  conf_data.set('SRCDIR', meson.project_source_root())
  conf_data.set('DOT_PATH', dot.full_path())
  conf_data.set('OUTPUT', meson.project_build_root())
  conf_data.set('INPUT', ' '.join(input))

  doxyfile = configure_file(
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: conf_data
  )
  run_target('docs', command: [doxygen, doxyfile])
endif
