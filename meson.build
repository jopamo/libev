project(
  'libev',
  'c',
  version: '4.33',
  meson_version: '>=0.60',
  license: ['BSD-2-Clause', 'GPL-2.0-or-later'],
  default_options: ['c_std=gnu99', 'warning_level=2'],
)

cc = meson.get_compiler('c')
host_system = host_machine.system()

linuxaio_opt = get_option('linuxaio')
iouring_opt = get_option('iouring')

m_dep = cc.find_library('m', required: false)
rt_dep = cc.find_library('rt', required: false)
lib_deps = []
use_librt = false

pkgconfig = import('pkgconfig')

conf = configuration_data()

conf.set_quoted('PACKAGE', 'libev')
conf.set_quoted('PACKAGE_NAME', 'libev')
conf.set_quoted('PACKAGE_TARNAME', 'libev')
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_STRING', 'libev ' + meson.project_version())
conf.set_quoted('PACKAGE_BUGREPORT', '')
conf.set_quoted('PACKAGE_URL', 'http://software.schmorp.de/pkg/libev')
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('LT_OBJDIR', '.libs/')
conf.set10('STDC_HEADERS', true)

header_checks = {
  'HAVE_DLFCN_H': 'dlfcn.h',
  'HAVE_INTTYPES_H': 'inttypes.h',
  'HAVE_MEMORY_H': 'memory.h',
  'HAVE_STDINT_H': 'stdint.h',
  'HAVE_STDLIB_H': 'stdlib.h',
  'HAVE_STRINGS_H': 'strings.h',
  'HAVE_STRING_H': 'string.h',
  'HAVE_SYS_STAT_H': 'sys/stat.h',
  'HAVE_SYS_TYPES_H': 'sys/types.h',
  'HAVE_UNISTD_H': 'unistd.h',
  'HAVE_SYS_INOTIFY_H': 'sys/inotify.h',
  'HAVE_SYS_EPOLL_H': 'sys/epoll.h',
  'HAVE_SYS_EVENT_H': 'sys/event.h',
  'HAVE_PORT_H': 'port.h',
  'HAVE_POLL_H': 'poll.h',
  'HAVE_SYS_TIMERFD_H': 'sys/timerfd.h',
  'HAVE_SYS_SELECT_H': 'sys/select.h',
  'HAVE_SYS_EVENTFD_H': 'sys/eventfd.h',
  'HAVE_SYS_SIGNALFD_H': 'sys/signalfd.h',
}

foreach macro, header : header_checks
  conf.set10(macro, cc.has_header(header))
endforeach

linuxaio_header = false
if not linuxaio_opt.disabled()
  linuxaio_header = cc.has_header('linux/aio_abi.h')
  if linuxaio_opt.enabled() and not linuxaio_header
    error('linuxaio backend requested, but linux/aio_abi.h not found')
  endif
endif
conf.set10('HAVE_LINUX_AIO_ABI_H', linuxaio_header)

iouring_header = false
if not iouring_opt.disabled()
  iouring_header = cc.has_header('linux/fs.h')
  if iouring_opt.enabled() and not iouring_header
    error('io_uring backend requested, but linux/fs.h not found')
  endif
endif
conf.set10('HAVE_LINUX_FS_H', iouring_header)

func_checks = {
  'HAVE_INOTIFY_INIT': ['inotify_init', '#include <sys/inotify.h>'],
  'HAVE_EPOLL_CTL': ['epoll_ctl', '#include <sys/epoll.h>'],
  'HAVE_KQUEUE': ['kqueue', '#include <sys/event.h>'],
  'HAVE_PORT_CREATE': ['port_create', '#include <port.h>'],
  'HAVE_POLL': ['poll', '#include <poll.h>'],
  'HAVE_SELECT': ['select', '#include <sys/select.h>'],
  'HAVE_EVENTFD': ['eventfd', '#include <sys/eventfd.h>'],
  'HAVE_SIGNALFD': ['signalfd', '#include <sys/signalfd.h>'],
}

foreach macro, data : func_checks
  func_name = data[0]
  prefix = data[1]
  conf.set10(macro, cc.has_function(func_name, prefix: prefix))
endforeach

clock_found = cc.has_function('clock_gettime', prefix: '#include <time.h>')

if not clock_found and host_system == 'linux'
  clock_syscall_check = '''
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <time.h>
    int main(void) {
      struct timespec ts;
      return syscall(SYS_clock_gettime, CLOCK_REALTIME, &ts);
    }
  '''
  if cc.links(clock_syscall_check, name: 'clock_gettime syscall')
    clock_found = true
    conf.set10('HAVE_CLOCK_SYSCALL', true)
  endif
endif

if not clock_found and rt_dep.found()
  if cc.has_function('clock_gettime', prefix: '#include <time.h>', dependencies: rt_dep)
    clock_found = true
    use_librt = true
  endif
endif

conf.set10('HAVE_CLOCK_GETTIME', clock_found)

nanosleep_found = cc.has_function('nanosleep', prefix: '#include <time.h>')

if not nanosleep_found and rt_dep.found()
  if cc.has_function('nanosleep', prefix: '#include <time.h>', dependencies: rt_dep)
    nanosleep_found = true
    use_librt = true
  endif
endif

conf.set10('HAVE_NANOSLEEP', nanosleep_found)

floor_found = cc.has_function('floor', prefix: '#include <math.h>')

if not floor_found and m_dep.found()
  floor_found = cc.has_function('floor', prefix: '#include <math.h>', dependencies: m_dep)
  if floor_found and m_dep.found()
    lib_deps += m_dep
  endif
endif

conf.set10('HAVE_FLOOR', floor_found)

kernel_rwf_check = '''
  #include <linux/fs.h>
  int main(void) {
    __kernel_rwf_t v = 0;
    return (int)v;
  }
'''

have_kernel_rwf = false
if iouring_header and not iouring_opt.disabled()
  have_kernel_rwf = cc.compiles(kernel_rwf_check, name: 'kernel_rwf_t')
  if iouring_opt.enabled() and not have_kernel_rwf
    error('__kernel_rwf_t missing while io_uring backend was requested')
  endif
endif

conf.set10('HAVE_KERNEL_RWF_T', have_kernel_rwf)

if use_librt and rt_dep.found()
  conf.set10('HAVE_LIBRT', true)
  lib_deps += rt_dep
endif

config_h = configure_file(
  output: 'config.h',
  configuration: conf,
  install: false,
)

root_inc = include_directories('.')
src_inc = include_directories('src')
pub_inc = include_directories('include')
all_incs = [root_inc, src_inc, pub_inc]

libev_sources = files(
  'src/ev.c',
  'src/event.c',
)

libev = library(
  'ev',
  libev_sources,
  include_directories: all_incs,
  dependencies: lib_deps,
  version: '4.0.0',
  soversion: '4',
  install: true,
)

libev_dep = declare_dependency(
  link_with: libev,
  include_directories: pub_inc,
  dependencies: lib_deps,
  version: meson.project_version(),
)

meson.override_dependency('libev', libev_dep)

install_headers(
  'include/ev.h',
  'include/ev++.h',
  'include/event.h',
)

install_man('ev.3')

pkgconfig.generate(
  name: 'libev',
  filebase: 'libev',
  description: 'High-performance full-featured event loop',
  version: meson.project_version(),
  libraries: libev,
  subdirs: '.',
)

orig_inc = include_directories('libev-4.33')
orig_sources = files(
  'libev-4.33/ev.c',
  'libev-4.33/event.c',
)

orig_libev = library(
  'ev_baseline',
  orig_sources,
  include_directories: [orig_inc, root_inc],
  dependencies: lib_deps,
  version: '4.0.0',
  soversion: '4',
  install: false,
)

orig_libev_dep = declare_dependency(
  link_with: orig_libev,
  include_directories: [orig_inc, root_inc],
  dependencies: lib_deps,
  version: '4.33',
)

subdir('tests')
