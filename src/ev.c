/* src/ev.c
 * libev event processing core, watcher management
 */

/* this big block deduces configuration from config.h */
#ifndef EV_STANDALONE
#ifdef EV_CONFIG_H
#include EV_CONFIG_H
#else
#include "config.h"
#endif

#if HAVE_FLOOR
#ifndef EV_USE_FLOOR
#define EV_USE_FLOOR 1
#endif
#endif

#if HAVE_CLOCK_SYSCALL
#ifndef EV_USE_CLOCK_SYSCALL
#define EV_USE_CLOCK_SYSCALL 1
#ifndef EV_USE_REALTIME
#define EV_USE_REALTIME 0
#endif
#ifndef EV_USE_MONOTONIC
#define EV_USE_MONOTONIC 1
#endif
#endif
#elif !defined EV_USE_CLOCK_SYSCALL
#define EV_USE_CLOCK_SYSCALL 0
#endif

#if HAVE_CLOCK_GETTIME
#ifndef EV_USE_MONOTONIC
#define EV_USE_MONOTONIC 1
#endif
#ifndef EV_USE_REALTIME
#define EV_USE_REALTIME 0
#endif
#else
#ifndef EV_USE_MONOTONIC
#define EV_USE_MONOTONIC 0
#endif
#ifndef EV_USE_REALTIME
#define EV_USE_REALTIME 0
#endif
#endif

#if HAVE_NANOSLEEP
#ifndef EV_USE_NANOSLEEP
#define EV_USE_NANOSLEEP EV_FEATURE_OS
#endif
#else
#undef EV_USE_NANOSLEEP
#define EV_USE_NANOSLEEP 0
#endif

#if HAVE_SELECT && HAVE_SYS_SELECT_H
#ifndef EV_USE_SELECT
#define EV_USE_SELECT EV_FEATURE_BACKENDS
#endif
#else
#undef EV_USE_SELECT
#define EV_USE_SELECT 0
#endif

#if HAVE_POLL && HAVE_POLL_H
#ifndef EV_USE_POLL
#define EV_USE_POLL EV_FEATURE_BACKENDS
#endif
#else
#undef EV_USE_POLL
#define EV_USE_POLL 0
#endif

#if HAVE_EPOLL_CTL && HAVE_SYS_EPOLL_H
#ifndef EV_USE_EPOLL
#define EV_USE_EPOLL EV_FEATURE_BACKENDS
#endif
#else
#undef EV_USE_EPOLL
#define EV_USE_EPOLL 0
#endif

#if HAVE_LINUX_AIO_ABI_H
#ifndef EV_USE_LINUXAIO
#define EV_USE_LINUXAIO 0 /* was: EV_FEATURE_BACKENDS, always off by default */
#endif
#else
#undef EV_USE_LINUXAIO
#define EV_USE_LINUXAIO 0
#endif

#if HAVE_LINUX_FS_H && HAVE_SYS_TIMERFD_H && HAVE_KERNEL_RWF_T
#ifndef EV_USE_IOURING
#define EV_USE_IOURING EV_FEATURE_BACKENDS
#endif
#else
#undef EV_USE_IOURING
#define EV_USE_IOURING 0
#endif

#if HAVE_KQUEUE && HAVE_SYS_EVENT_H
#ifndef EV_USE_KQUEUE
#define EV_USE_KQUEUE EV_FEATURE_BACKENDS
#endif
#else
#undef EV_USE_KQUEUE
#define EV_USE_KQUEUE 0
#endif

#if HAVE_PORT_H && HAVE_PORT_CREATE
#ifndef EV_USE_PORT
#define EV_USE_PORT EV_FEATURE_BACKENDS
#endif
#else
#undef EV_USE_PORT
#define EV_USE_PORT 0
#endif

#if HAVE_INOTIFY_INIT && HAVE_SYS_INOTIFY_H
#ifndef EV_USE_INOTIFY
#define EV_USE_INOTIFY EV_FEATURE_OS
#endif
#else
#undef EV_USE_INOTIFY
#define EV_USE_INOTIFY 0
#endif

#if HAVE_SIGNALFD && HAVE_SYS_SIGNALFD_H
#ifndef EV_USE_SIGNALFD
#define EV_USE_SIGNALFD EV_FEATURE_OS
#endif
#else
#undef EV_USE_SIGNALFD
#define EV_USE_SIGNALFD 0
#endif

#if HAVE_EVENTFD
#ifndef EV_USE_EVENTFD
#define EV_USE_EVENTFD EV_FEATURE_OS
#endif
#else
#undef EV_USE_EVENTFD
#define EV_USE_EVENTFD 0
#endif

#if HAVE_SYS_TIMERFD_H
#ifndef EV_USE_TIMERFD
#define EV_USE_TIMERFD EV_FEATURE_OS
#endif
#else
#undef EV_USE_TIMERFD
#define EV_USE_TIMERFD 0
#endif

#endif

/* OS X, in its infinite idiocy, actually HARDCODES
 * a limit of 1024 into their select. Where people have brains,
 * OS X engineers apparently have a vacuum. Or maybe they were
 * ordered to have a vacuum, or they do anything for money.
 * This might help. Or not.
 * Note that this must be defined early, as other include files
 * will rely on this define as well.
 */
#define _DARWIN_UNLIMITED_SELECT 1

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stddef.h>

#include <stdio.h>

#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>

#define EV_ASSERT_MSG(msg, cond) assert((cond) && (msg))

#include <signal.h>

#ifdef EV_H
#include EV_H
#else
#include "ev.h"
#endif

#if EV_NO_THREADS
#undef EV_NO_SMP
#define EV_NO_SMP 1
#undef ECB_NO_THREADS
#define ECB_NO_THREADS 1
#endif
#if EV_NO_SMP
#undef EV_NO_SMP
#define ECB_NO_SMP 1
#endif

#ifndef _WIN32
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#ifndef EV_SELECT_IS_WINSOCKET
#define EV_SELECT_IS_WINSOCKET 1
#endif
#undef EV_AVOID_STDIO
#endif

/* this block tries to deduce configuration from header-defined symbols and defaults */

/* try to deduce the maximum number of signals on this platform */
#if defined EV_NSIG
/* use what's provided */
#elif defined NSIG
#define EV_NSIG (NSIG)
#elif defined _NSIG
#define EV_NSIG (_NSIG)
#elif defined SIGMAX
#define EV_NSIG (SIGMAX + 1)
#elif defined SIG_MAX
#define EV_NSIG (SIG_MAX + 1)
#elif defined _SIG_MAX
#define EV_NSIG (_SIG_MAX + 1)
#elif defined MAXSIG
#define EV_NSIG (MAXSIG + 1)
#elif defined MAX_SIG
#define EV_NSIG (MAX_SIG + 1)
#elif defined SIGARRAYSIZE
#define EV_NSIG (SIGARRAYSIZE) /* Assume ary[SIGARRAYSIZE] */
#elif defined _sys_nsig
#define EV_NSIG (_sys_nsig) /* Solaris 2.5 */
#else
#define EV_NSIG (8 * sizeof(sigset_t) + 1)
#endif

#ifndef EV_USE_FLOOR
#define EV_USE_FLOOR 0
#endif

#ifndef EV_USE_CLOCK_SYSCALL
#if __linux && __GLIBC__ == 2 && __GLIBC_MINOR__ < 17
#define EV_USE_CLOCK_SYSCALL EV_FEATURE_OS
#else
#define EV_USE_CLOCK_SYSCALL 0
#endif
#endif

#if !(_POSIX_TIMERS > 0)
#ifndef EV_USE_MONOTONIC
#define EV_USE_MONOTONIC 0
#endif
#ifndef EV_USE_REALTIME
#define EV_USE_REALTIME 0
#endif
#endif

#ifndef EV_USE_MONOTONIC
#if defined _POSIX_MONOTONIC_CLOCK && _POSIX_MONOTONIC_CLOCK >= 0
#define EV_USE_MONOTONIC EV_FEATURE_OS
#else
#define EV_USE_MONOTONIC 0
#endif
#endif

#ifndef EV_USE_REALTIME
#define EV_USE_REALTIME !EV_USE_CLOCK_SYSCALL
#endif

#ifndef EV_USE_NANOSLEEP
#if _POSIX_C_SOURCE >= 199309L
#define EV_USE_NANOSLEEP EV_FEATURE_OS
#else
#define EV_USE_NANOSLEEP 0
#endif
#endif

#ifndef EV_USE_SELECT
#define EV_USE_SELECT EV_FEATURE_BACKENDS
#endif

#ifndef EV_USE_POLL
#ifdef _WIN32
#define EV_USE_POLL 0
#else
#define EV_USE_POLL EV_FEATURE_BACKENDS
#endif
#endif

#ifndef EV_USE_EPOLL
#if __linux && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 4))
#define EV_USE_EPOLL EV_FEATURE_BACKENDS
#else
#define EV_USE_EPOLL 0
#endif
#endif

#ifndef EV_USE_KQUEUE
#define EV_USE_KQUEUE 0
#endif

#ifndef EV_USE_PORT
#define EV_USE_PORT 0
#endif

#ifndef EV_USE_LINUXAIO
#if __linux               /* libev currently assumes linux/aio_abi.h is always available on linux */
#define EV_USE_LINUXAIO 0 /* was: 1, always off by default */
#else
#define EV_USE_LINUXAIO 0
#endif
#endif

#ifndef EV_USE_IOURING
#if __linux /* later checks might disable again */
#define EV_USE_IOURING 1
#else
#define EV_USE_IOURING 0
#endif
#endif

#ifndef EV_USE_INOTIFY
#if __linux && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 4))
#define EV_USE_INOTIFY EV_FEATURE_OS
#else
#define EV_USE_INOTIFY 0
#endif
#endif

#ifndef EV_PID_HASHSIZE
#define EV_PID_HASHSIZE EV_FEATURE_DATA ? 16 : 1
#endif

#ifndef EV_INOTIFY_HASHSIZE
#define EV_INOTIFY_HASHSIZE EV_FEATURE_DATA ? 16 : 1
#endif

#ifndef EV_USE_EVENTFD
#if __linux && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 7))
#define EV_USE_EVENTFD EV_FEATURE_OS
#else
#define EV_USE_EVENTFD 0
#endif
#endif

#ifndef EV_USE_SIGNALFD
#if __linux && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 7))
#define EV_USE_SIGNALFD EV_FEATURE_OS
#else
#define EV_USE_SIGNALFD 0
#endif
#endif

#ifndef EV_USE_TIMERFD
#if __linux && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8))
#define EV_USE_TIMERFD EV_FEATURE_OS
#else
#define EV_USE_TIMERFD 0
#endif
#endif

#if 0 /* debugging */
#define EV_VERIFY 3
#define EV_USE_4HEAP 1
#define EV_HEAP_CACHE_AT 1
#endif

#ifndef EV_VERIFY
#define EV_VERIFY (EV_FEATURE_API ? 1 : 0)
#endif

#ifndef EV_USE_4HEAP
#define EV_USE_4HEAP EV_FEATURE_DATA
#endif

#ifndef EV_HEAP_CACHE_AT
#define EV_HEAP_CACHE_AT EV_FEATURE_DATA
#endif

#ifdef __ANDROID__
/* supposedly, android doesn't typedef fd_mask */
#undef EV_USE_SELECT
#define EV_USE_SELECT 0
/* supposedly, we need to include syscall.h, not sys/syscall.h, so just disable */
#undef EV_USE_CLOCK_SYSCALL
#define EV_USE_CLOCK_SYSCALL 0
#endif

/* aix's poll.h seems to cause lots of trouble */
#ifdef _AIX
/* AIX has a completely broken poll.h header */
#undef EV_USE_POLL
#define EV_USE_POLL 0
#endif

/* on linux, we can use a (slow) syscall to avoid a dependency on pthread, */
/* which makes programs even slower. might work on other unices, too. */
#if EV_USE_CLOCK_SYSCALL
#include <sys/syscall.h>
#ifdef SYS_clock_gettime
#define clock_gettime(id, ts) syscall(SYS_clock_gettime, (id), (ts))
#undef EV_USE_MONOTONIC
#define EV_USE_MONOTONIC 1
#define EV_NEED_SYSCALL 1
#else
#undef EV_USE_CLOCK_SYSCALL
#define EV_USE_CLOCK_SYSCALL 0
#endif
#endif

/* this block fixes any misconfiguration where we know we run into trouble otherwise */

#ifndef CLOCK_MONOTONIC
#undef EV_USE_MONOTONIC
#define EV_USE_MONOTONIC 0
#endif

#ifndef CLOCK_REALTIME
#undef EV_USE_REALTIME
#define EV_USE_REALTIME 0
#endif

#if !EV_STAT_ENABLE
#undef EV_USE_INOTIFY
#define EV_USE_INOTIFY 0
#endif

#if __linux && EV_USE_IOURING
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
#undef EV_USE_IOURING
#define EV_USE_IOURING 0
#endif
#endif

#if !EV_USE_NANOSLEEP
/* hp-ux has it in sys/time.h, which we unconditionally include above */
#if !defined _WIN32 && !defined __hpux
#include <sys/select.h>
#endif
#endif

#if EV_USE_LINUXAIO
#include <sys/syscall.h>
#if SYS_io_getevents && EV_USE_EPOLL /* linuxaio backend requires epoll backend */
#define EV_NEED_SYSCALL 1
#else
#undef EV_USE_LINUXAIO
#define EV_USE_LINUXAIO 0
#endif
#endif

#if EV_USE_IOURING
#include <sys/syscall.h>
#if !SYS_io_uring_setup && __linux && !__alpha
#define SYS_io_uring_setup 425
#define SYS_io_uring_enter 426
#define SYS_io_uring_wregister 427
#endif
#if SYS_io_uring_setup && EV_USE_EPOLL /* iouring backend requires epoll backend */
#define EV_NEED_SYSCALL 1
#else
#undef EV_USE_IOURING
#define EV_USE_IOURING 0
#endif
#endif

#if EV_USE_INOTIFY
#include <sys/statfs.h>
#include <sys/inotify.h>
/* some very old inotify.h headers don't have IN_DONT_FOLLOW */
#ifndef IN_DONT_FOLLOW
#undef EV_USE_INOTIFY
#define EV_USE_INOTIFY 0
#endif
#endif

#if EV_USE_EVENTFD
/* our minimum requirement is glibc 2.7 which has the stub, but not the full header */
#include <stdint.h>
#ifndef EFD_NONBLOCK
#define EFD_NONBLOCK O_NONBLOCK
#endif
#ifndef EFD_CLOEXEC
#ifdef O_CLOEXEC
#define EFD_CLOEXEC O_CLOEXEC
#else
#define EFD_CLOEXEC 02000000
#endif
#endif
extern int eventfd(unsigned int initval, int flags);
#endif

#if EV_USE_SIGNALFD
/* our minimum requirement is glibc 2.7 which has the stub, but not the full header */
#include <stdint.h>
#ifndef SFD_NONBLOCK
#define SFD_NONBLOCK O_NONBLOCK
#endif
#ifndef SFD_CLOEXEC
#ifdef O_CLOEXEC
#define SFD_CLOEXEC O_CLOEXEC
#else
#define SFD_CLOEXEC 02000000
#endif
#endif
extern int signalfd(int fd, const sigset_t* mask, int flags);

struct signalfd_siginfo {
  uint32_t ssi_signo;
  char pad[128 - sizeof(uint32_t)];
};
#endif

/* for timerfd, libev core requires TFD_TIMER_CANCEL_ON_SET &c */
#if EV_USE_TIMERFD
#include <sys/timerfd.h>
/* timerfd is only used for periodics */
#if !(defined(TFD_TIMER_CANCEL_ON_SET) && defined(TFD_CLOEXEC) && defined(TFD_NONBLOCK)) || !EV_PERIODIC_ENABLE
#undef EV_USE_TIMERFD
#define EV_USE_TIMERFD 0
#endif
#endif

/*****************************************************************************/

#if EV_VERIFY >= 3
#define EV_FREQUENT_CHECK ev_verify(EV_A)
#else
#define EV_FREQUENT_CHECK \
  do {                    \
  } while (0)
#endif

/*
 * This is used to work around floating point rounding problems.
 * This value is good at least till the year 4000.
 */
#define MIN_INTERVAL 0.0001220703125 /* 1/2**13, good till 4000 */
/* #define MIN_INTERVAL  0.00000095367431640625 -- 1/2**20, good till 2200 */

#define MIN_TIMEJUMP 1.      /* minimum timejump that gets detected (if monotonic clock available) */
#define MAX_BLOCKTIME 59.743 /* never wait longer than this time (to detect time jumps) */
#define MAX_BLOCKTIME2                                                                          \
  1500001.07 /* same, but when timerfd is used to detect jumps, also safe delay to not overflow \
              */

/* find a portable timestamp that is "always" in the future but fits into time_t.
 * this is quite hard, and we are mostly guessing - we handle 32 bit signed/unsigned time_t,
 * and sizes larger than 32 bit, and maybe the unlikely floating point time_t */
#define EV_TSTAMP_HUGE (sizeof(time_t) >= 8 ? 10000000000000. : 0 < (time_t)4294967295 ? 4294967295. : 2147483647.)

#ifndef EV_TS_CONST
#define EV_TS_CONST(nv) nv
#define EV_TS_TO_MSEC(a) a * 1e3 + 0.9999
#define EV_TS_FROM_USEC(us) us * 1e-6
#define EV_TV_SET(tv, t)                            \
  do {                                              \
    (tv).tv_sec = (long)t;                          \
    (tv).tv_usec = (long)((t - (tv).tv_sec) * 1e6); \
  } while (0)
#define EV_TS_SET(ts, t)                            \
  do {                                              \
    (ts).tv_sec = (long)t;                          \
    (ts).tv_nsec = (long)((t - (ts).tv_sec) * 1e9); \
  } while (0)
#define EV_TV_GET(tv) ((tv).tv_sec + (tv).tv_usec * 1e-6)
#define EV_TS_GET(ts) ((ts).tv_sec + (ts).tv_nsec * 1e-9)
#endif

/* the following is ecb.h embedded into libev - use update_ev_c to update from an external copy */
/* ECB.H BEGIN */
/*
 * libecb - http://software.schmorp.de/pkg/libecb
 *
 * Copyright (©) 2009-2015,2018-2020 Marc Alexander Lehmann <libecb@schmorp.de>
 * Copyright (©) 2011 Emanuele Giaquinta
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

#ifndef ECB_H
#define ECB_H

/* 16 bits major, 16 bits minor */
#define ECB_VERSION 0x00010008

#include <string.h> /* for memcpy */

#if defined(_WIN32) && !defined(__MINGW32__)
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed char int_fast8_t;
typedef unsigned char uint_fast8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int_fast16_t;
typedef unsigned int uint_fast16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed int int_fast32_t;
typedef unsigned int uint_fast32_t;
#if __GNUC__
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#else /* _MSC_VER || __BORLANDC__ */
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
typedef int64_t int_fast64_t;
typedef uint64_t uint_fast64_t;
#ifdef _WIN64
#define ECB_PTRSIZE 8
typedef uint64_t uintptr_t;
typedef int64_t intptr_t;
#else
#define ECB_PTRSIZE 4
typedef uint32_t uintptr_t;
typedef int32_t intptr_t;
#endif
#else
#include <inttypes.h>
#if (defined INTPTR_MAX ? INTPTR_MAX : (defined ULONG_MAX ? ULONG_MAX : UINT_MAX)) > 0xffffffffU
#define ECB_PTRSIZE 8
#else
#define ECB_PTRSIZE 4
#endif
#endif

#define ECB_GCC_AMD64 (__amd64 || __amd64__ || __x86_64 || __x86_64__)
#define ECB_MSVC_AMD64 (_M_AMD64 || _M_X64)

#ifndef ECB_OPTIMIZE_SIZE
#if __OPTIMIZE_SIZE__
#define ECB_OPTIMIZE_SIZE 1
#else
#define ECB_OPTIMIZE_SIZE 0
#endif
#endif

/* work around x32 idiocy by defining proper macros */
#if ECB_GCC_AMD64 || ECB_MSVC_AMD64
#if _ILP32
#define ECB_AMD64_X32 1
#else
#define ECB_AMD64 1
#endif
#endif

/* many compilers define _GNUC_ to some versions but then only implement
 * what their idiot authors think are the "more important" extensions,
 * causing enormous grief in return for some better fake benchmark numbers.
 * or so.
 * we try to detect these and simply assume they are not gcc - if they have
 * an issue with that they should have done it right in the first place.
 */
#if !defined __GNUC_MINOR__ || defined __INTEL_COMPILER || defined __SUNPRO_C || defined __SUNPRO_CC || \
    defined __llvm__ || defined __clang__
#define ECB_GCC_VERSION(major, minor) 0
#else
#define ECB_GCC_VERSION(major, minor) (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#endif

#define ECB_CLANG_VERSION(major, minor) \
  (__clang_major__ > (major) || (__clang_major__ == (major) && __clang_minor__ >= (minor)))

#if __clang__ && defined __has_builtin
#define ECB_CLANG_BUILTIN(x) __has_builtin(x)
#else
#define ECB_CLANG_BUILTIN(x) 0
#endif

#if __clang__ && defined __has_extension
#define ECB_CLANG_EXTENSION(x) __has_extension(x)
#else
#define ECB_CLANG_EXTENSION(x) 0
#endif

#define ECB_CPP (__cplusplus + 0)
#define ECB_CPP11 (__cplusplus >= 201103L)
#define ECB_CPP14 (__cplusplus >= 201402L)
#define ECB_CPP17 (__cplusplus >= 201703L)

#if ECB_CPP
#define ECB_C 0
#define ECB_STDC_VERSION 0
#else
#define ECB_C 1
#define ECB_STDC_VERSION __STDC_VERSION__
#endif

#define ECB_C99 (ECB_STDC_VERSION >= 199901L)
#define ECB_C11 (ECB_STDC_VERSION >= 201112L)
#define ECB_C17 (ECB_STDC_VERSION >= 201710L)

#if ECB_CPP
#define ECB_EXTERN_C extern "C"
#define ECB_EXTERN_C_BEG ECB_EXTERN_C {
#define ECB_EXTERN_C_END }
#else
#define ECB_EXTERN_C extern
#define ECB_EXTERN_C_BEG
#define ECB_EXTERN_C_END
#endif

/*****************************************************************************/

/* ECB_NO_THREADS - ecb is not used by multiple threads, ever */
/* ECB_NO_SMP     - ecb might be used in multiple threads, but only on a single cpu */

#if ECB_NO_THREADS
#define ECB_NO_SMP 1
#endif

#if ECB_NO_SMP
#define ECB_MEMORY_FENCE \
  do {                   \
  } while (0)
#endif

/* http://www-01.ibm.com/support/knowledgecenter/SSGH3R_13.1.0/com.ibm.xlcpp131.aix.doc/compiler_ref/compiler_builtins.html
 */
#if __xlC__ && ECB_CPP
#include <builtins.h>
#endif

#if 1400 <= _MSC_VER
#include <intrin.h> /* fence functions _ReadBarrier, also bit search functions _BitScanReverse */
#endif

#ifndef ECB_MEMORY_FENCE
#if ECB_GCC_VERSION(2, 5) || defined __INTEL_COMPILER || (__llvm__ && __GNUC__) || __SUNPRO_C >= 0x5110 || \
    __SUNPRO_CC >= 0x5110
#define ECB_MEMORY_FENCE_RELAXED __asm__ __volatile__("" : : : "memory")
#if __i386 || __i386__
#define ECB_MEMORY_FENCE __asm__ __volatile__("lock; orb $0, -1(%%esp)" : : : "memory")
#define ECB_MEMORY_FENCE_ACQUIRE __asm__ __volatile__("" : : : "memory")
#define ECB_MEMORY_FENCE_RELEASE __asm__ __volatile__("" : : : "memory")
#elif ECB_GCC_AMD64
#define ECB_MEMORY_FENCE __asm__ __volatile__("mfence" : : : "memory")
#define ECB_MEMORY_FENCE_ACQUIRE __asm__ __volatile__("" : : : "memory")
#define ECB_MEMORY_FENCE_RELEASE __asm__ __volatile__("" : : : "memory")
#elif __powerpc__ || __ppc__ || __powerpc64__ || __ppc64__
#define ECB_MEMORY_FENCE __asm__ __volatile__("sync" : : : "memory")
#elif defined __ARM_ARCH_2__ || defined __ARM_ARCH_3__ || defined __ARM_ARCH_3M__ || defined __ARM_ARCH_4__ || \
    defined __ARM_ARCH_4T__ || defined __ARM_ARCH_5__ || defined __ARM_ARCH_5E__ || defined __ARM_ARCH_5T__ || \
    defined __ARM_ARCH_5TE__ || defined __ARM_ARCH_5TEJ__
/* should not need any, unless running old code on newer cpu - arm doesn't support that */
#elif defined __ARM_ARCH_6__ || defined __ARM_ARCH_6J__ || defined __ARM_ARCH_6K__ || defined __ARM_ARCH_6ZK__ || \
    defined __ARM_ARCH_6T2__
#define ECB_MEMORY_FENCE __asm__ __volatile__("mcr p15,0,%0,c7,c10,5" : : "r"(0) : "memory")
#elif defined __ARM_ARCH_7__ || defined __ARM_ARCH_7A__ || defined __ARM_ARCH_7R__ || defined __ARM_ARCH_7M__
#define ECB_MEMORY_FENCE __asm__ __volatile__("dmb" : : : "memory")
#elif __aarch64__
#define ECB_MEMORY_FENCE __asm__ __volatile__("dmb ish" : : : "memory")
#elif (__sparc || __sparc__) && !(__sparc_v8__ || defined __sparcv8)
#define ECB_MEMORY_FENCE __asm__ __volatile__("membar #LoadStore | #LoadLoad | #StoreStore | #StoreLoad" : : : "memory")
#define ECB_MEMORY_FENCE_ACQUIRE __asm__ __volatile__("membar #LoadStore | #LoadLoad" : : : "memory")
#define ECB_MEMORY_FENCE_RELEASE __asm__ __volatile__("membar #LoadStore             | #StoreStore")
#elif defined __s390__ || defined __s390x__
#define ECB_MEMORY_FENCE __asm__ __volatile__("bcr 15,0" : : : "memory")
#elif defined __mips__
/* GNU/Linux emulates sync on mips1 architectures, so we force its use */
/* anybody else who still uses mips1 is supposed to send in their version, with detection code. */
#define ECB_MEMORY_FENCE __asm__ __volatile__(".set mips2; sync; .set mips0" : : : "memory")
#elif defined __alpha__
#define ECB_MEMORY_FENCE __asm__ __volatile__("mb" : : : "memory")
#elif defined __hppa__
#define ECB_MEMORY_FENCE __asm__ __volatile__("" : : : "memory")
#define ECB_MEMORY_FENCE_RELEASE __asm__ __volatile__("")
#elif defined __ia64__
#define ECB_MEMORY_FENCE __asm__ __volatile__("mf" : : : "memory")
#elif defined __m68k__
#define ECB_MEMORY_FENCE __asm__ __volatile__("" : : : "memory")
#elif defined __m88k__
#define ECB_MEMORY_FENCE __asm__ __volatile__("tb1 0,%%r0,128" : : : "memory")
#elif defined __sh__
#define ECB_MEMORY_FENCE __asm__ __volatile__("" : : : "memory")
#endif
#endif
#endif

#ifndef ECB_MEMORY_FENCE
#if ECB_GCC_VERSION(4, 7)
/* see comment below (stdatomic.h) about the C11 memory model. */
#define ECB_MEMORY_FENCE __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define ECB_MEMORY_FENCE_ACQUIRE __atomic_thread_fence(__ATOMIC_ACQUIRE)
#define ECB_MEMORY_FENCE_RELEASE __atomic_thread_fence(__ATOMIC_RELEASE)
#define ECB_MEMORY_FENCE_RELAXED __atomic_thread_fence(__ATOMIC_RELAXED)

#elif ECB_CLANG_EXTENSION(c_atomic)
/* see comment below (stdatomic.h) about the C11 memory model. */
#define ECB_MEMORY_FENCE __c11_atomic_thread_fence(__ATOMIC_SEQ_CST)
#define ECB_MEMORY_FENCE_ACQUIRE __c11_atomic_thread_fence(__ATOMIC_ACQUIRE)
#define ECB_MEMORY_FENCE_RELEASE __c11_atomic_thread_fence(__ATOMIC_RELEASE)
#define ECB_MEMORY_FENCE_RELAXED __c11_atomic_thread_fence(__ATOMIC_RELAXED)

#elif ECB_GCC_VERSION(4, 4) || defined __INTEL_COMPILER || defined __clang__
#define ECB_MEMORY_FENCE __sync_synchronize()
#elif _MSC_VER >= 1500 /* VC++ 2008 */
/* apparently, microsoft broke all the memory barrier stuff in Visual Studio 2008... */
#pragma intrinsic(_ReadBarrier, _WriteBarrier, _ReadWriteBarrier)
#define ECB_MEMORY_FENCE \
  _ReadWriteBarrier();   \
  MemoryBarrier()
#define ECB_MEMORY_FENCE_ACQUIRE \
  _ReadWriteBarrier();           \
  MemoryBarrier() /* according to msdn, _ReadBarrier is not a load fence */
#define ECB_MEMORY_FENCE_RELEASE \
  _WriteBarrier();               \
  MemoryBarrier()
#elif _MSC_VER >= 1400 /* VC++ 2005 */
#pragma intrinsic(_ReadBarrier, _WriteBarrier, _ReadWriteBarrier)
#define ECB_MEMORY_FENCE _ReadWriteBarrier()
#define ECB_MEMORY_FENCE_ACQUIRE _ReadWriteBarrier() /* according to msdn, _ReadBarrier is not a load fence */
#define ECB_MEMORY_FENCE_RELEASE _WriteBarrier()
#elif defined _WIN32
#include <WinNT.h>
#define ECB_MEMORY_FENCE MemoryBarrier() /* actually just xchg on x86... scary */
#elif __SUNPRO_C >= 0x5110 || __SUNPRO_CC >= 0x5110
#include <mbarrier.h>
#define ECB_MEMORY_FENCE __machine_rw_barrier()
#define ECB_MEMORY_FENCE_ACQUIRE __machine_acq_barrier()
#define ECB_MEMORY_FENCE_RELEASE __machine_rel_barrier()
#define ECB_MEMORY_FENCE_RELAXED __compiler_barrier()
#elif __xlC__
#define ECB_MEMORY_FENCE __sync()
#endif
#endif

#ifndef ECB_MEMORY_FENCE
#if ECB_C11 && !defined __STDC_NO_ATOMICS__
/* we assume that these memory fences work on all variables/all memory accesses, */
/* not just C11 atomics and atomic accesses */
#include <stdatomic.h>
#define ECB_MEMORY_FENCE atomic_thread_fence(memory_order_seq_cst)
#define ECB_MEMORY_FENCE_ACQUIRE atomic_thread_fence(memory_order_acquire)
#define ECB_MEMORY_FENCE_RELEASE atomic_thread_fence(memory_order_release)
#endif
#endif

#ifndef ECB_MEMORY_FENCE
#if !ECB_AVOID_PTHREADS
/*
 * if you get undefined symbol references to pthread_mutex_lock,
 * or failure to find pthread.h, then you should implement
 * the ECB_MEMORY_FENCE operations for your cpu/compiler
 * OR provide pthread.h and link against the posix thread library
 * of your system.
 */
#include <pthread.h>
#define ECB_NEEDS_PTHREADS 1
#define ECB_MEMORY_FENCE_NEEDS_PTHREADS 1

static pthread_mutex_t ecb_mf_lock = PTHREAD_MUTEX_INITIALIZER;
#define ECB_MEMORY_FENCE                \
  do {                                  \
    pthread_mutex_lock(&ecb_mf_lock);   \
    pthread_mutex_unlock(&ecb_mf_lock); \
  } while (0)
#endif
#endif

#if !defined ECB_MEMORY_FENCE_ACQUIRE && defined ECB_MEMORY_FENCE
#define ECB_MEMORY_FENCE_ACQUIRE ECB_MEMORY_FENCE
#endif

#if !defined ECB_MEMORY_FENCE_RELEASE && defined ECB_MEMORY_FENCE
#define ECB_MEMORY_FENCE_RELEASE ECB_MEMORY_FENCE
#endif

#if !defined ECB_MEMORY_FENCE_RELAXED && defined ECB_MEMORY_FENCE
#define ECB_MEMORY_FENCE_RELAXED ECB_MEMORY_FENCE /* very heavy-handed */
#endif

/*****************************************************************************/

#if ECB_CPP
#define ecb_inline static inline
#elif ECB_GCC_VERSION(2, 5)
#define ecb_inline static __inline__
#elif ECB_C99
#define ecb_inline static inline
#else
#define ecb_inline static
#endif

#if ECB_GCC_VERSION(3, 3)
#define ecb_restrict __restrict__
#elif ECB_C99
#define ecb_restrict restrict
#else
#define ecb_restrict
#endif

typedef int ecb_bool;

#define ECB_CONCAT_(a, b) a##b
#define ECB_CONCAT(a, b) ECB_CONCAT_(a, b)
#define ECB_STRINGIFY_(a) #a
#define ECB_STRINGIFY(a) ECB_STRINGIFY_(a)
#define ECB_STRINGIFY_EXPR(expr) ((expr), ECB_STRINGIFY_(expr))

#define ecb_function_ ecb_inline

#if ECB_GCC_VERSION(3, 1) || ECB_CLANG_VERSION(2, 8)
#define ecb_attribute(attrlist) __attribute__(attrlist)
#else
#define ecb_attribute(attrlist)
#endif

#if ECB_GCC_VERSION(3, 1) || ECB_CLANG_BUILTIN(__builtin_constant_p)
#define ecb_is_constant(expr) __builtin_constant_p(expr)
#else
/* possible C11 impl for integral types
typedef struct ecb_is_constant_struct ecb_is_constant_struct;
#define ecb_is_constant(expr)          _Generic ((1 ? (struct ecb_is_constant_struct *)0 : (void *)((expr) - (expr)),
ecb_is_constant_struct *: 0, default: 1)) */

#define ecb_is_constant(expr) 0
#endif

#if ECB_GCC_VERSION(3, 1) || ECB_CLANG_BUILTIN(__builtin_expect)
#define ecb_expect(expr, value) __builtin_expect((expr), (value))
#else
#define ecb_expect(expr, value) (expr)
#endif

#if ECB_GCC_VERSION(3, 1) || ECB_CLANG_BUILTIN(__builtin_prefetch)
#define ecb_prefetch(addr, rw, locality) __builtin_prefetch(addr, rw, locality)
#else
#define ecb_prefetch(addr, rw, locality)
#endif

/* no emulation for ecb_decltype */
#if ECB_CPP11
// older implementations might have problems with decltype(x)::type, work around it
template<class T> struct ecb_decltype_t {
  typedef T type;
};
#define ecb_decltype(x) ecb_decltype_t<decltype(x)>::type
#elif ECB_GCC_VERSION(3, 0) || ECB_CLANG_VERSION(2, 8)
#define ecb_decltype(x) __typeof__(x)
#endif

#if _MSC_VER >= 1300
#define ecb_deprecated __declspec(deprecated)
#else
#define ecb_deprecated ecb_attribute((__deprecated__))
#endif

#if _MSC_VER >= 1500
#define ecb_deprecated_message(msg) __declspec(deprecated(msg))
#elif ECB_GCC_VERSION(4, 5)
#define ecb_deprecated_message(msg) ecb_attribute ((__deprecated__ (msg))
#else
#define ecb_deprecated_message(msg) ecb_deprecated
#endif

#if _MSC_VER >= 1400
#define ecb_noinline __declspec(noinline)
#else
#define ecb_noinline ecb_attribute((__noinline__))
#endif

#define ecb_unused ecb_attribute((__unused__))
#define ecb_const ecb_attribute((__const__))
#define ecb_pure ecb_attribute((__pure__))

#if ECB_C11 || __IBMC_NORETURN
/* http://www-01.ibm.com/support/knowledgecenter/SSGH3R_13.1.0/com.ibm.xlcpp131.aix.doc/language_ref/noreturn.html */
#define ecb_noreturn _Noreturn
#elif ECB_CPP11
#define ecb_noreturn [[noreturn]]
#elif _MSC_VER >= 1200
/* http://msdn.microsoft.com/en-us/library/k6ktzx3s.aspx */
#define ecb_noreturn __declspec(noreturn)
#else
#define ecb_noreturn ecb_attribute((__noreturn__))
#endif

#if ECB_GCC_VERSION(4, 3)
#define ecb_artificial ecb_attribute((__artificial__))
#define ecb_hot ecb_attribute((__hot__))
#define ecb_cold ecb_attribute((__cold__))
#else
#define ecb_artificial
#define ecb_hot
#define ecb_cold
#endif

/* put around conditional expressions if you are very sure that the  */
/* expression is mostly true or mostly false. note that these return */
/* booleans, not the expression.                                     */
#define ecb_expect_false(expr) ecb_expect(!!(expr), 0)
#define ecb_expect_true(expr) ecb_expect(!!(expr), 1)
/* for compatibility to the rest of the world */
#define ecb_likely(expr) ecb_expect_true(expr)
#define ecb_unlikely(expr) ecb_expect_false(expr)

/* count trailing zero bits and count # of one bits */
#if ECB_GCC_VERSION(3, 4) ||                                                                                       \
    (ECB_CLANG_BUILTIN(__builtin_clz) && ECB_CLANG_BUILTIN(__builtin_clzll) && ECB_CLANG_BUILTIN(__builtin_ctz) && \
     ECB_CLANG_BUILTIN(__builtin_ctzll) && ECB_CLANG_BUILTIN(__builtin_popcount))
/* we assume int == 32 bit, long == 32 or 64 bit and long long == 64 bit */
#define ecb_ld32(x) (__builtin_clz(x) ^ 31)
#define ecb_ld64(x) (__builtin_clzll(x) ^ 63)
#define ecb_ctz32(x) __builtin_ctz(x)
#define ecb_ctz64(x) __builtin_ctzll(x)
#define ecb_popcount32(x) __builtin_popcount(x)
/* no popcountll */
#else
ecb_function_ ecb_const int ecb_ctz32(uint32_t x);
ecb_function_ ecb_const int ecb_ctz32(uint32_t x) {
#if 1400 <= _MSC_VER && (_M_IX86 || _M_X64 || _M_IA64 || _M_ARM)
  unsigned long r;
  _BitScanForward(&r, x);
  return (int)r;
#else
  int r = 0;

  x &= ~x + 1; /* this isolates the lowest bit */

#if ECB_branchless_on_i386
  r += !!(x & 0xaaaaaaaa) << 0;
  r += !!(x & 0xcccccccc) << 1;
  r += !!(x & 0xf0f0f0f0) << 2;
  r += !!(x & 0xff00ff00) << 3;
  r += !!(x & 0xffff0000) << 4;
#else
  if (x & 0xaaaaaaaa)
    r += 1;
  if (x & 0xcccccccc)
    r += 2;
  if (x & 0xf0f0f0f0)
    r += 4;
  if (x & 0xff00ff00)
    r += 8;
  if (x & 0xffff0000)
    r += 16;
#endif

  return r;
#endif
}

ecb_function_ ecb_const int ecb_ctz64(uint64_t x);
ecb_function_ ecb_const int ecb_ctz64(uint64_t x) {
#if 1400 <= _MSC_VER && (_M_X64 || _M_IA64 || _M_ARM)
  unsigned long r;
  _BitScanForward64(&r, x);
  return (int)r;
#else
  int shift = x & 0xffffffff ? 0 : 32;
  return ecb_ctz32(x >> shift) + shift;
#endif
}

ecb_function_ ecb_const int ecb_popcount32(uint32_t x);
ecb_function_ ecb_const int ecb_popcount32(uint32_t x) {
  x -= (x >> 1) & 0x55555555;
  x = ((x >> 2) & 0x33333333) + (x & 0x33333333);
  x = ((x >> 4) + x) & 0x0f0f0f0f;
  x *= 0x01010101;

  return x >> 24;
}

ecb_function_ ecb_const int ecb_ld32(uint32_t x);
ecb_function_ ecb_const int ecb_ld32(uint32_t x) {
#if 1400 <= _MSC_VER && (_M_IX86 || _M_X64 || _M_IA64 || _M_ARM)
  unsigned long r;
  _BitScanReverse(&r, x);
  return (int)r;
#else
  int r = 0;

  if (x >> 16) {
    x >>= 16;
    r += 16;
  }
  if (x >> 8) {
    x >>= 8;
    r += 8;
  }
  if (x >> 4) {
    x >>= 4;
    r += 4;
  }
  if (x >> 2) {
    x >>= 2;
    r += 2;
  }
  if (x >> 1) {
    r += 1;
  }

  return r;
#endif
}

ecb_function_ ecb_const int ecb_ld64(uint64_t x);
ecb_function_ ecb_const int ecb_ld64(uint64_t x) {
#if 1400 <= _MSC_VER && (_M_X64 || _M_IA64 || _M_ARM)
  unsigned long r;
  _BitScanReverse64(&r, x);
  return (int)r;
#else
  int r = 0;

  if (x >> 32) {
    x >>= 32;
    r += 32;
  }

  return r + ecb_ld32(x);
#endif
}
#endif

ecb_function_ ecb_const ecb_bool ecb_is_pot32(uint32_t x);
ecb_function_ ecb_const ecb_bool ecb_is_pot32(uint32_t x) {
  return !(x & (x - 1));
}
ecb_function_ ecb_const ecb_bool ecb_is_pot64(uint64_t x);
ecb_function_ ecb_const ecb_bool ecb_is_pot64(uint64_t x) {
  return !(x & (x - 1));
}

ecb_function_ ecb_const uint8_t ecb_bitrev8(uint8_t x);
ecb_function_ ecb_const uint8_t ecb_bitrev8(uint8_t x) {
  return ((x * 0x0802U & 0x22110U) | (x * 0x8020U & 0x88440U)) * 0x10101U >> 16;
}

ecb_function_ ecb_const uint16_t ecb_bitrev16(uint16_t x);
ecb_function_ ecb_const uint16_t ecb_bitrev16(uint16_t x) {
  x = ((x >> 1) & 0x5555) | ((x & 0x5555) << 1);
  x = ((x >> 2) & 0x3333) | ((x & 0x3333) << 2);
  x = ((x >> 4) & 0x0f0f) | ((x & 0x0f0f) << 4);
  x = (x >> 8) | (x << 8);

  return x;
}

ecb_function_ ecb_const uint32_t ecb_bitrev32(uint32_t x);
ecb_function_ ecb_const uint32_t ecb_bitrev32(uint32_t x) {
  x = ((x >> 1) & 0x55555555) | ((x & 0x55555555) << 1);
  x = ((x >> 2) & 0x33333333) | ((x & 0x33333333) << 2);
  x = ((x >> 4) & 0x0f0f0f0f) | ((x & 0x0f0f0f0f) << 4);
  x = ((x >> 8) & 0x00ff00ff) | ((x & 0x00ff00ff) << 8);
  x = (x >> 16) | (x << 16);

  return x;
}

/* popcount64 is only available on 64 bit cpus as gcc builtin */
/* so for this version we are lazy */
ecb_function_ ecb_const int ecb_popcount64(uint64_t x);
ecb_function_ ecb_const int ecb_popcount64(uint64_t x) {
  return ecb_popcount32(x) + ecb_popcount32(x >> 32);
}

ecb_inline ecb_const uint8_t ecb_rotl8(uint8_t x, unsigned int count);
ecb_inline ecb_const uint8_t ecb_rotr8(uint8_t x, unsigned int count);
ecb_inline ecb_const uint16_t ecb_rotl16(uint16_t x, unsigned int count);
ecb_inline ecb_const uint16_t ecb_rotr16(uint16_t x, unsigned int count);
ecb_inline ecb_const uint32_t ecb_rotl32(uint32_t x, unsigned int count);
ecb_inline ecb_const uint32_t ecb_rotr32(uint32_t x, unsigned int count);
ecb_inline ecb_const uint64_t ecb_rotl64(uint64_t x, unsigned int count);
ecb_inline ecb_const uint64_t ecb_rotr64(uint64_t x, unsigned int count);

ecb_inline ecb_const uint8_t ecb_rotl8(uint8_t x, unsigned int count) {
  return (x >> (8 - count)) | (x << count);
}
ecb_inline ecb_const uint8_t ecb_rotr8(uint8_t x, unsigned int count) {
  return (x << (8 - count)) | (x >> count);
}
ecb_inline ecb_const uint16_t ecb_rotl16(uint16_t x, unsigned int count) {
  return (x >> (16 - count)) | (x << count);
}
ecb_inline ecb_const uint16_t ecb_rotr16(uint16_t x, unsigned int count) {
  return (x << (16 - count)) | (x >> count);
}
ecb_inline ecb_const uint32_t ecb_rotl32(uint32_t x, unsigned int count) {
  return (x >> (32 - count)) | (x << count);
}
ecb_inline ecb_const uint32_t ecb_rotr32(uint32_t x, unsigned int count) {
  return (x << (32 - count)) | (x >> count);
}
ecb_inline ecb_const uint64_t ecb_rotl64(uint64_t x, unsigned int count) {
  return (x >> (64 - count)) | (x << count);
}
ecb_inline ecb_const uint64_t ecb_rotr64(uint64_t x, unsigned int count) {
  return (x << (64 - count)) | (x >> count);
}

#if ECB_CPP

inline uint8_t ecb_ctz(uint8_t v) {
  return ecb_ctz32(v);
}
inline uint16_t ecb_ctz(uint16_t v) {
  return ecb_ctz32(v);
}
inline uint32_t ecb_ctz(uint32_t v) {
  return ecb_ctz32(v);
}
inline uint64_t ecb_ctz(uint64_t v) {
  return ecb_ctz64(v);
}

inline bool ecb_is_pot(uint8_t v) {
  return ecb_is_pot32(v);
}
inline bool ecb_is_pot(uint16_t v) {
  return ecb_is_pot32(v);
}
inline bool ecb_is_pot(uint32_t v) {
  return ecb_is_pot32(v);
}
inline bool ecb_is_pot(uint64_t v) {
  return ecb_is_pot64(v);
}

inline int ecb_ld(uint8_t v) {
  return ecb_ld32(v);
}
inline int ecb_ld(uint16_t v) {
  return ecb_ld32(v);
}
inline int ecb_ld(uint32_t v) {
  return ecb_ld32(v);
}
inline int ecb_ld(uint64_t v) {
  return ecb_ld64(v);
}

inline int ecb_popcount(uint8_t v) {
  return ecb_popcount32(v);
}
inline int ecb_popcount(uint16_t v) {
  return ecb_popcount32(v);
}
inline int ecb_popcount(uint32_t v) {
  return ecb_popcount32(v);
}
inline int ecb_popcount(uint64_t v) {
  return ecb_popcount64(v);
}

inline uint8_t ecb_bitrev(uint8_t v) {
  return ecb_bitrev8(v);
}
inline uint16_t ecb_bitrev(uint16_t v) {
  return ecb_bitrev16(v);
}
inline uint32_t ecb_bitrev(uint32_t v) {
  return ecb_bitrev32(v);
}

inline uint8_t ecb_rotl(uint8_t v, unsigned int count) {
  return ecb_rotl8(v, count);
}
inline uint16_t ecb_rotl(uint16_t v, unsigned int count) {
  return ecb_rotl16(v, count);
}
inline uint32_t ecb_rotl(uint32_t v, unsigned int count) {
  return ecb_rotl32(v, count);
}
inline uint64_t ecb_rotl(uint64_t v, unsigned int count) {
  return ecb_rotl64(v, count);
}

inline uint8_t ecb_rotr(uint8_t v, unsigned int count) {
  return ecb_rotr8(v, count);
}
inline uint16_t ecb_rotr(uint16_t v, unsigned int count) {
  return ecb_rotr16(v, count);
}
inline uint32_t ecb_rotr(uint32_t v, unsigned int count) {
  return ecb_rotr32(v, count);
}
inline uint64_t ecb_rotr(uint64_t v, unsigned int count) {
  return ecb_rotr64(v, count);
}

#endif

#if ECB_GCC_VERSION(4, 5) || ECB_CLANG_BUILTIN(__builtin_bswap32) && ECB_CLANG_BUILTIN(__builtin_bswap64)
#if ECB_GCC_VERSION(4, 8) || ECB_CLANG_BUILTIN(__builtin_bswap16)
#define ecb_bswap16(x) __builtin_bswap16(x)
#else
#define ecb_bswap16(x) (__builtin_bswap32(x) >> 16)
#endif
#define ecb_bswap32(x) __builtin_bswap32(x)
#define ecb_bswap64(x) __builtin_bswap64(x)
#elif _MSC_VER
#include <stdlib.h>
#define ecb_bswap16(x) ((uint16_t)_byteswap_ushort((uint16_t)(x)))
#define ecb_bswap32(x) ((uint32_t)_byteswap_ulong((uint32_t)(x)))
#define ecb_bswap64(x) ((uint64_t)_byteswap_uint64((uint64_t)(x)))
#else
ecb_function_ ecb_const uint16_t ecb_bswap16(uint16_t x);
ecb_function_ ecb_const uint16_t ecb_bswap16(uint16_t x) {
  return ecb_rotl16(x, 8);
}

ecb_function_ ecb_const uint32_t ecb_bswap32(uint32_t x);
ecb_function_ ecb_const uint32_t ecb_bswap32(uint32_t x) {
  return (((uint32_t)ecb_bswap16(x)) << 16) | ecb_bswap16(x >> 16);
}

ecb_function_ ecb_const uint64_t ecb_bswap64(uint64_t x);
ecb_function_ ecb_const uint64_t ecb_bswap64(uint64_t x) {
  return (((uint64_t)ecb_bswap32(x)) << 32) | ecb_bswap32(x >> 32);
}
#endif

#if ECB_GCC_VERSION(4, 5) || ECB_CLANG_BUILTIN(__builtin_unreachable)
#define ecb_unreachable() __builtin_unreachable()
#else
/* this seems to work fine, but gcc always emits a warning for it :/ */
ecb_noreturn void ecb_unreachable(void) {
  /* Note: this may not work on all compilers, but it's better than nothing */
  /* Use __builtin_trap() if available for better code generation */
#if ECB_GCC_VERSION(4, 5) || ECB_CLANG_BUILTIN(__builtin_trap)
  __builtin_trap();
#else
  for (;;)
    ;
#endif
}
#endif

/* try to tell the compiler that some condition is definitely true */
#define ecb_assume(cond) \
  if (!(cond))           \
    ecb_unreachable();   \
  else                   \
    0

ecb_inline ecb_const uint32_t ecb_byteorder_helper(void);
ecb_inline ecb_const uint32_t ecb_byteorder_helper(void) {
  /* the union code still generates code under pressure in gcc, */
  /* but less than using pointers, and always seems to */
  /* successfully return a constant. */
  /* the reason why we have this horrible preprocessor mess */
  /* is to avoid it in all cases, at least on common architectures */
  /* or when using a recent enough gcc version (>= 4.6) */
#if (defined __BYTE_ORDER__ && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
    ((__i386 || __i386__ || _M_IX86 || ECB_GCC_AMD64 || ECB_MSVC_AMD64) && !__VOS__)
#define ECB_LITTLE_ENDIAN 1
  return 0x44332211;
#elif (defined __BYTE_ORDER__ && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || \
    ((__AARCH64EB__ || __MIPSEB__ || __ARMEB__) && !__VOS__)
#define ECB_BIG_ENDIAN 1
  return 0x11223344;
#else
  union {
    uint8_t c[4];
    uint32_t u;
  } u = {0x11, 0x22, 0x33, 0x44};
  return u.u;
#endif
}

ecb_inline ecb_const ecb_bool ecb_big_endian(void);
ecb_inline ecb_const ecb_bool ecb_big_endian(void) {
  return ecb_byteorder_helper() == 0x11223344;
}
ecb_inline ecb_const ecb_bool ecb_little_endian(void);
ecb_inline ecb_const ecb_bool ecb_little_endian(void) {
  return ecb_byteorder_helper() == 0x44332211;
}

/*****************************************************************************/
/* unaligned load/store */

ecb_inline uint_fast16_t ecb_be_u16_to_host(uint_fast16_t v) {
  return ecb_little_endian() ? ecb_bswap16(v) : v;
}
ecb_inline uint_fast32_t ecb_be_u32_to_host(uint_fast32_t v) {
  return ecb_little_endian() ? ecb_bswap32(v) : v;
}
ecb_inline uint_fast64_t ecb_be_u64_to_host(uint_fast64_t v) {
  return ecb_little_endian() ? ecb_bswap64(v) : v;
}

ecb_inline uint_fast16_t ecb_le_u16_to_host(uint_fast16_t v) {
  return ecb_big_endian() ? ecb_bswap16(v) : v;
}
ecb_inline uint_fast32_t ecb_le_u32_to_host(uint_fast32_t v) {
  return ecb_big_endian() ? ecb_bswap32(v) : v;
}
ecb_inline uint_fast64_t ecb_le_u64_to_host(uint_fast64_t v) {
  return ecb_big_endian() ? ecb_bswap64(v) : v;
}

ecb_inline uint_fast16_t ecb_peek_u16_u(const void* ptr) {
  uint16_t v;
  memcpy(&v, ptr, sizeof(v));
  return v;
}
ecb_inline uint_fast32_t ecb_peek_u32_u(const void* ptr) {
  uint32_t v;
  memcpy(&v, ptr, sizeof(v));
  return v;
}
ecb_inline uint_fast64_t ecb_peek_u64_u(const void* ptr) {
  uint64_t v;
  memcpy(&v, ptr, sizeof(v));
  return v;
}

ecb_inline uint_fast16_t ecb_peek_be_u16_u(const void* ptr) {
  return ecb_be_u16_to_host(ecb_peek_u16_u(ptr));
}
ecb_inline uint_fast32_t ecb_peek_be_u32_u(const void* ptr) {
  return ecb_be_u32_to_host(ecb_peek_u32_u(ptr));
}
ecb_inline uint_fast64_t ecb_peek_be_u64_u(const void* ptr) {
  return ecb_be_u64_to_host(ecb_peek_u64_u(ptr));
}

ecb_inline uint_fast16_t ecb_peek_le_u16_u(const void* ptr) {
  return ecb_le_u16_to_host(ecb_peek_u16_u(ptr));
}
ecb_inline uint_fast32_t ecb_peek_le_u32_u(const void* ptr) {
  return ecb_le_u32_to_host(ecb_peek_u32_u(ptr));
}
ecb_inline uint_fast64_t ecb_peek_le_u64_u(const void* ptr) {
  return ecb_le_u64_to_host(ecb_peek_u64_u(ptr));
}

ecb_inline uint_fast16_t ecb_host_to_be_u16(uint_fast16_t v) {
  return ecb_little_endian() ? ecb_bswap16(v) : v;
}
ecb_inline uint_fast32_t ecb_host_to_be_u32(uint_fast32_t v) {
  return ecb_little_endian() ? ecb_bswap32(v) : v;
}
ecb_inline uint_fast64_t ecb_host_to_be_u64(uint_fast64_t v) {
  return ecb_little_endian() ? ecb_bswap64(v) : v;
}

ecb_inline uint_fast16_t ecb_host_to_le_u16(uint_fast16_t v) {
  return ecb_big_endian() ? ecb_bswap16(v) : v;
}
ecb_inline uint_fast32_t ecb_host_to_le_u32(uint_fast32_t v) {
  return ecb_big_endian() ? ecb_bswap32(v) : v;
}
ecb_inline uint_fast64_t ecb_host_to_le_u64(uint_fast64_t v) {
  return ecb_big_endian() ? ecb_bswap64(v) : v;
}

ecb_inline void ecb_poke_u16_u(void* ptr, uint16_t v) {
  memcpy(ptr, &v, sizeof(v));
}
ecb_inline void ecb_poke_u32_u(void* ptr, uint32_t v) {
  memcpy(ptr, &v, sizeof(v));
}
ecb_inline void ecb_poke_u64_u(void* ptr, uint64_t v) {
  memcpy(ptr, &v, sizeof(v));
}

ecb_inline void ecb_poke_be_u16_u(void* ptr, uint_fast16_t v) {
  ecb_poke_u16_u(ptr, ecb_host_to_be_u16(v));
}
ecb_inline void ecb_poke_be_u32_u(void* ptr, uint_fast32_t v) {
  ecb_poke_u32_u(ptr, ecb_host_to_be_u32(v));
}
ecb_inline void ecb_poke_be_u64_u(void* ptr, uint_fast64_t v) {
  ecb_poke_u64_u(ptr, ecb_host_to_be_u64(v));
}

ecb_inline void ecb_poke_le_u16_u(void* ptr, uint_fast16_t v) {
  ecb_poke_u16_u(ptr, ecb_host_to_le_u16(v));
}
ecb_inline void ecb_poke_le_u32_u(void* ptr, uint_fast32_t v) {
  ecb_poke_u32_u(ptr, ecb_host_to_le_u32(v));
}
ecb_inline void ecb_poke_le_u64_u(void* ptr, uint_fast64_t v) {
  ecb_poke_u64_u(ptr, ecb_host_to_le_u64(v));
}

#if ECB_CPP

inline uint8_t ecb_bswap(uint8_t v) {
  return v;
}
inline uint16_t ecb_bswap(uint16_t v) {
  return ecb_bswap16(v);
}
inline uint32_t ecb_bswap(uint32_t v) {
  return ecb_bswap32(v);
}
inline uint64_t ecb_bswap(uint64_t v) {
  return ecb_bswap64(v);
}

template<typename T> inline T ecb_be_to_host(T v) {
  return ecb_little_endian() ? ecb_bswap(v) : v;
}
template<typename T> inline T ecb_le_to_host(T v) {
  return ecb_big_endian() ? ecb_bswap(v) : v;
}
template<typename T> inline T ecb_peek(const void* ptr) {
  return *(const T*)ptr;
}
template<typename T> inline T ecb_peek_be(const void* ptr) {
  return ecb_be_to_host(ecb_peek<T>(ptr));
}
template<typename T> inline T ecb_peek_le(const void* ptr) {
  return ecb_le_to_host(ecb_peek<T>(ptr));
}
template<typename T> inline T ecb_peek_u(const void* ptr) {
  T v;
  memcpy(&v, ptr, sizeof(v));
  return v;
}
template<typename T> inline T ecb_peek_be_u(const void* ptr) {
  return ecb_be_to_host(ecb_peek_u<T>(ptr));
}
template<typename T> inline T ecb_peek_le_u(const void* ptr) {
  return ecb_le_to_host(ecb_peek_u<T>(ptr));
}

template<typename T> inline T ecb_host_to_be(T v) {
  return ecb_little_endian() ? ecb_bswap(v) : v;
}
template<typename T> inline T ecb_host_to_le(T v) {
  return ecb_big_endian() ? ecb_bswap(v) : v;
}
template<typename T> inline void ecb_poke(void* ptr, T v) {
  *(T*)ptr = v;
}
template<typename T> inline void ecb_poke_be(void* ptr, T v) {
  return ecb_poke<T>(ptr, ecb_host_to_be(v));
}
template<typename T> inline void ecb_poke_le(void* ptr, T v) {
  return ecb_poke<T>(ptr, ecb_host_to_le(v));
}
template<typename T> inline void ecb_poke_u(void* ptr, T v) {
  memcpy(ptr, &v, sizeof(v));
}
template<typename T> inline void ecb_poke_be_u(void* ptr, T v) {
  return ecb_poke_u<T>(ptr, ecb_host_to_be(v));
}
template<typename T> inline void ecb_poke_le_u(void* ptr, T v) {
  return ecb_poke_u<T>(ptr, ecb_host_to_le(v));
}

#endif

/*****************************************************************************/

#if ECB_GCC_VERSION(3, 0) || ECB_C99
#define ecb_mod(m, n) ((m) % (n) + ((m) % (n) < 0 ? (n) : 0))
#else
#define ecb_mod(m, n) ((m) < 0 ? ((n) - 1 - ((-1 - (m)) % (n))) : ((m) % (n)))
#endif

#if ECB_CPP
template<typename T> static inline T ecb_div_rd(T val, T div) {
  return val < 0 ? -((-val + div - 1) / div) : (val) / div;
}
template<typename T> static inline T ecb_div_ru(T val, T div) {
  return val < 0 ? -((-val) / div) : (val + div - 1) / div;
}
#else
#define ecb_div_rd(val, div) ((val) < 0 ? -((-(val) + (div) - 1) / (div)) : ((val)) / (div))
#define ecb_div_ru(val, div) ((val) < 0 ? -((-(val)) / (div)) : ((val) + (div) - 1) / (div))
#endif

#if ecb_cplusplus_does_not_suck
/* does not work for local types (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2008/n2657.htm) */
template<typename T, int N> static inline int ecb_array_length(const T (&arr)[N]) {
  return N;
}
#else
#define ecb_array_length(name) (sizeof(name) / sizeof(name[0]))
#endif

/*****************************************************************************/

ecb_function_ ecb_const uint32_t ecb_binary16_to_binary32(uint32_t x);
ecb_function_ ecb_const uint32_t ecb_binary16_to_binary32(uint32_t x) {
  unsigned int s = (x & 0x8000) << (31 - 15);
  int e = (x >> 10) & 0x001f;
  unsigned int m = x & 0x03ff;

  if (ecb_expect_false(e == 31))
    /* infinity or NaN */
    e = 255 - (127 - 15);
  else if (ecb_expect_false(!e)) {
    if (ecb_expect_true(!m))
      /* zero, handled by code below by forcing e to 0 */
      e = 0 - (127 - 15);
    else {
      /* subnormal, renormalise */
      unsigned int s = 10 - ecb_ld32(m);

      m = (m << s) & 0x3ff; /* mask implicit bit */
      e -= s - 1;
    }
  }

  /* e and m now are normalised, or zero, (or inf or nan) */
  e += 127 - 15;

  return s | (e << 23) | (m << (23 - 10));
}

ecb_function_ ecb_const uint16_t ecb_binary32_to_binary16(uint32_t x);
ecb_function_ ecb_const uint16_t ecb_binary32_to_binary16(uint32_t x) {
  unsigned int s = (x >> 16) & 0x00008000;            /* sign bit, the easy part */
  int e = (int)((x >> 23) & 0x000000ff) - (127 - 15); /* the desired exponent */
  unsigned int m = x & 0x007fffff;

  x &= 0x7fffffff;

  /* if it's within range of binary16 normals, use fast path */
  if (ecb_expect_true(0x38800000 <= x && x <= 0x477fefff)) {
    /* mantissa round-to-even */
    m += 0x00000fff + ((m >> (23 - 10)) & 1);

    /* handle overflow */
    if (ecb_expect_false(m >= 0x00800000)) {
      m >>= 1;
      e += 1;
    }

    return s | (e << 10) | (m >> (23 - 10));
  }

  /* handle large numbers and infinity */
  if (ecb_expect_true(0x477fefff < x && x <= 0x7f800000))
    return s | 0x7c00;

  /* handle zero, subnormals and small numbers */
  if (ecb_expect_true(x < 0x38800000)) {
    /* zero */
    if (ecb_expect_true(!x))
      return s;

    /* handle subnormals */

    /* too small, will be zero */
    if (e < (14 - 24)) /* might not be sharp, but is good enough */
      return s;

    m |= 0x00800000; /* make implicit bit explicit */

    /* very tricky - we need to round to the nearest e (+10) bit value */
    {
      unsigned int bits = 14 - e;
      unsigned int half = (1 << (bits - 1)) - 1;
      unsigned int even = (m >> bits) & 1;

      /* if this overflows, we will end up with a normalised number */
      m = (m + half + even) >> bits;
    }

    return s | m;
  }

  /* handle NaNs, preserve leftmost nan bits, but make sure we don't turn them into infinities */
  m >>= 13;

  return s | 0x7c00 | m | !m;
}

/*******************************************************************************/
/* floating point stuff, can be disabled by defining ECB_NO_LIBM */

/* basically, everything uses "ieee pure-endian" floating point numbers */
/* the only noteworthy exception is ancient armle, which uses order 43218765 */
#if 0 || __i386 || __i386__ || ECB_GCC_AMD64 || __powerpc__ || __ppc__ || __powerpc64__ || __ppc64__ ||                \
    defined __s390__ || defined __s390x__ || defined __mips__ || defined __alpha__ || defined __hppa__ ||              \
    defined __ia64__ || defined __m68k__ || defined __m88k__ || defined __sh__ || defined _M_IX86 ||                   \
    defined ECB_MSVC_AMD64 || defined _M_IA64 ||                                                                       \
    (defined __arm__ &&                                                                                                \
     (defined __ARM_EABI__ || defined __EABI__ || defined __VFP_FP__ || defined _WIN32_WCE || defined __ANDROID__)) || \
    defined __aarch64__
#define ECB_STDFP 1
#else
#define ECB_STDFP 0
#endif

#ifndef ECB_NO_LIBM

#include <math.h> /* for frexp*, ldexp*, INFINITY, NAN */

/* only the oldest of old doesn't have this one. solaris. */
#ifdef INFINITY
#define ECB_INFINITY INFINITY
#else
#define ECB_INFINITY HUGE_VAL
#endif

#ifdef NAN
#define ECB_NAN NAN
#else
#define ECB_NAN ECB_INFINITY
#endif

#if ECB_C99 || _XOPEN_VERSION >= 600 || _POSIX_VERSION >= 200112L
#define ecb_ldexpf(x, e) ldexpf((x), (e))
#define ecb_frexpf(x, e) frexpf((x), (e))
#else
#define ecb_ldexpf(x, e) (float)ldexp((double)(x), (e))
#define ecb_frexpf(x, e) (float)frexp((double)(x), (e))
#endif

/* convert a float to ieee single/binary32 */
ecb_function_ ecb_const uint32_t ecb_float_to_binary32(float x);
ecb_function_ ecb_const uint32_t ecb_float_to_binary32(float x) {
  uint32_t r;

#if ECB_STDFP
  memcpy(&r, &x, 4);
#else
  /* slow emulation, works for anything but -0 */
  uint32_t m;
  int e;

  if (x == 0e0f)
    return 0x00000000U;
  if (x > +3.40282346638528860e+38f)
    return 0x7f800000U;
  if (x < -3.40282346638528860e+38f)
    return 0xff800000U;
  if (x != x)
    return 0x7fbfffffU;

  m = ecb_frexpf(x, &e) * 0x1000000U;

  r = m & 0x80000000U;

  if (r)
    m = -m;

  if (e <= -126) {
    m &= 0xffffffU;
    m >>= (-125 - e);
    e = -126;
  }

  r |= (e + 126) << 23;
  r |= m & 0x7fffffU;
#endif

  return r;
}

/* converts an ieee single/binary32 to a float */
ecb_function_ ecb_const float ecb_binary32_to_float(uint32_t x);
ecb_function_ ecb_const float ecb_binary32_to_float(uint32_t x) {
  float r;

#if ECB_STDFP
  memcpy(&r, &x, 4);
#else
  /* emulation, only works for normals and subnormals and +0 */
  int neg = x >> 31;
  int e = (x >> 23) & 0xffU;

  x &= 0x7fffffU;

  if (e)
    x |= 0x800000U;
  else
    e = 1;

  /* we distrust ldexpf a bit and do the 2**-24 scaling by an extra multiply */
  r = ecb_ldexpf(x * (0.5f / 0x800000U), e - 126);

  r = neg ? -r : r;
#endif

  return r;
}

/* convert a double to ieee double/binary64 */
ecb_function_ ecb_const uint64_t ecb_double_to_binary64(double x);
ecb_function_ ecb_const uint64_t ecb_double_to_binary64(double x) {
  uint64_t r;

#if ECB_STDFP
  memcpy(&r, &x, 8);
#else
  /* slow emulation, works for anything but -0 */
  uint64_t m;
  int e;

  if (x == 0e0)
    return 0x0000000000000000U;
  if (x > +1.79769313486231470e+308)
    return 0x7ff0000000000000U;
  if (x < -1.79769313486231470e+308)
    return 0xfff0000000000000U;
  if (x != x)
    return 0X7ff7ffffffffffffU;

  m = frexp(x, &e) * 0x20000000000000U;

  r = m & 0x8000000000000000;
  ;

  if (r)
    m = -m;

  if (e <= -1022) {
    m &= 0x1fffffffffffffU;
    m >>= (-1021 - e);
    e = -1022;
  }

  r |= ((uint64_t)(e + 1022)) << 52;
  r |= m & 0xfffffffffffffU;
#endif

  return r;
}

/* converts an ieee double/binary64 to a double */
ecb_function_ ecb_const double ecb_binary64_to_double(uint64_t x);
ecb_function_ ecb_const double ecb_binary64_to_double(uint64_t x) {
  double r;

#if ECB_STDFP
  memcpy(&r, &x, 8);
#else
  /* emulation, only works for normals and subnormals and +0 */
  int neg = x >> 63;
  int e = (x >> 52) & 0x7ffU;

  x &= 0xfffffffffffffU;

  if (e)
    x |= 0x10000000000000U;
  else
    e = 1;

  /* we distrust ldexp a bit and do the 2**-53 scaling by an extra multiply */
  r = ldexp(x * (0.5 / 0x10000000000000U), e - 1022);

  r = neg ? -r : r;
#endif

  return r;
}

/* convert a float to ieee half/binary16 */
ecb_function_ ecb_const uint16_t ecb_float_to_binary16(float x);
ecb_function_ ecb_const uint16_t ecb_float_to_binary16(float x) {
  return ecb_binary32_to_binary16(ecb_float_to_binary32(x));
}

/* convert an ieee half/binary16 to float */
ecb_function_ ecb_const float ecb_binary16_to_float(uint16_t x);
ecb_function_ ecb_const float ecb_binary16_to_float(uint16_t x) {
  return ecb_binary32_to_float(ecb_binary16_to_binary32(x));
}

#endif

#endif

/* ECB.H END */

#if ECB_MEMORY_FENCE_NEEDS_PTHREADS
/* if your architecture doesn't need memory fences, e.g. because it is
 * single-cpu/core, or if you use libev in a project that doesn't use libev
 * from multiple threads, then you can define ECB_NO_THREADS when compiling
 * libev, in which cases the memory fences become nops.
 * alternatively, you can remove this #error and link against libpthread,
 * which will then provide the memory fences.
 */
#error "memory fences not defined for your architecture, please report"
#endif

#ifndef ECB_MEMORY_FENCE
#define ECB_MEMORY_FENCE \
  do {                   \
  } while (0)
#define ECB_MEMORY_FENCE_ACQUIRE ECB_MEMORY_FENCE
#define ECB_MEMORY_FENCE_RELEASE ECB_MEMORY_FENCE
#endif

#define inline_size ecb_inline

#if EV_FEATURE_CODE
#define inline_speed ecb_inline
#else
#define inline_speed ecb_noinline static
#endif

/*****************************************************************************/
/* raw syscall wrappers */

#if EV_NEED_SYSCALL

#include <sys/syscall.h>

inline_size int ev_syscall_ret(long res) {
  if (ecb_expect_true(res >= 0))
    return (int)res;

  int err = errno;
  return -err;
}

#define ev_syscall0(nr) ev_syscall_ret(syscall((nr)))
#define ev_syscall1(nr, arg1) ev_syscall_ret(syscall((nr), (arg1)))
#define ev_syscall2(nr, arg1, arg2) ev_syscall_ret(syscall((nr), (arg1), (arg2)))
#define ev_syscall3(nr, arg1, arg2, arg3) ev_syscall_ret(syscall((nr), (arg1), (arg2), (arg3)))
#define ev_syscall4(nr, arg1, arg2, arg3, arg4) ev_syscall_ret(syscall((nr), (arg1), (arg2), (arg3), (arg4)))
#define ev_syscall5(nr, arg1, arg2, arg3, arg4, arg5) \
  ev_syscall_ret(syscall((nr), (arg1), (arg2), (arg3), (arg4), (arg5)))
#define ev_syscall6(nr, arg1, arg2, arg3, arg4, arg5, arg6) \
  ev_syscall_ret(syscall((nr), (arg1), (arg2), (arg3), (arg4), (arg5), (arg6)))

#endif

/*****************************************************************************/

/* make sure configured priority bounds are sane */
#if EV_MAXPRI < EV_MINPRI
#error "EV_MAXPRI must be >= EV_MINPRI"
#endif

#define NUMPRI (EV_MAXPRI - EV_MINPRI + 1)

#if EV_MINPRI == EV_MAXPRI
#define ABSPRI(w) (((W)(w)), 0)
#else
#define ABSPRI(w) (ev_clamp_priority(((W)(w))->priority) - EV_MINPRI)
#endif

#define EMPTY /* required for microsofts broken pseudo-c compiler */

typedef ev_watcher* W;
typedef ev_watcher_list* WL;
typedef ev_watcher_time* WT;

#define ev_active(w) ((W)(w))->active
#define ev_at(w) ((WT)(w))->at

#if EV_USE_REALTIME
/* sig_atomic_t is used to avoid per-thread variables or locking but still */
/* giving it a reasonably high chance of working on typical architectures */
static EV_ATOMIC_T have_realtime; /* did clock_gettime (CLOCK_REALTIME) work? */
#endif

#if EV_USE_MONOTONIC
static EV_ATOMIC_T have_monotonic; /* did clock_gettime (CLOCK_MONOTONIC) work? */
#endif

#ifndef EV_FD_TO_WIN32_HANDLE
#define EV_FD_TO_WIN32_HANDLE(fd) _get_osfhandle(fd)
#endif
#ifndef EV_WIN32_HANDLE_TO_FD
#define EV_WIN32_HANDLE_TO_FD(handle) _open_osfhandle(handle, 0)
#endif
#ifndef EV_WIN32_CLOSE_FD
#define EV_WIN32_CLOSE_FD(fd) close(fd)
#endif

#ifdef _WIN32
#include "ev_win32.c"
#endif

/*****************************************************************************/

#if EV_USE_LINUXAIO
#include <linux/aio_abi.h> /* probably only needed for aio_context_t */
#endif

/* define a suitable floor function (only used by periodics atm) */

#if EV_USE_FLOOR
#include <math.h>
#define ev_floor(v) floor(v)
#else

#include <float.h>

/* a floor() replacement function, should be independent of ev_tstamp type */
ecb_noinline static ev_tstamp ev_floor(ev_tstamp v) {
  /* the choice of shift factor is not terribly important */
#if FLT_RADIX != 2 /* assume FLT_RADIX == 10 */
  const ev_tstamp shift = sizeof(unsigned long) >= 8 ? 10000000000000000000. : 1000000000.;
#else
  const ev_tstamp shift = sizeof(unsigned long) >= 8 ? 18446744073709551616. : 4294967296.;
#endif

  /* special treatment for negative arguments */
  if (ecb_expect_false(v < 0.)) {
    ev_tstamp f = -ev_floor(-v);

    return f - (f == v ? 0 : 1);
  }

  /* argument too large for an unsigned long? then reduce it */
  if (ecb_expect_false(v >= shift)) {
    ev_tstamp f;

    if (v == v - 1.)
      return v; /* very large numbers are assumed to be integer */

    f = shift * ev_floor(v * (1. / shift));
    return f + ev_floor(v - f);
  }

  /* fits into an unsigned long */
  return (unsigned long)v;
}

#endif

/*****************************************************************************/

#ifdef __linux
#include <sys/utsname.h>
#endif

ecb_noinline ecb_cold static unsigned int ev_linux_version(void) {
#ifdef __linux
  unsigned int v = 0;
  struct utsname buf;
  int i;
  char* p = buf.release;

  if (uname(&buf))
    return 0;

  for (i = 3 + 1; --i;) {
    unsigned int c = 0;

    for (;;) {
      if (*p >= '0' && *p <= '9')
        c = c * 10 + *p++ - '0';
      else {
        p += *p == '.';
        break;
      }
    }

    v = (v << 8) | c;
  }

  return v;
#else
  return 0;
#endif
}

/*****************************************************************************/

#if EV_AVOID_STDIO
ecb_noinline ecb_cold static void ev_printerr(const char* msg) {
  write(STDERR_FILENO, msg, strlen(msg));
}
#endif

static void (*syserr_cb)(const char* msg) EV_NOEXCEPT = 0;

ecb_cold void ev_set_syserr_cb(void (*cb)(const char* msg) EV_NOEXCEPT) EV_NOEXCEPT {
  syserr_cb = cb;
}

ecb_noinline ecb_cold static void ev_syserr(const char* msg) {
  void (*cb)(const char* msg) EV_NOEXCEPT;

  if (!msg)
    msg = "(libev) system error";

  /* Load the callback into a local variable to ensure it's not changed between check and call */
  cb = syserr_cb;

  /* Ensure the callback is invoked if set */
  if (cb) {
    cb(msg);
    return; /* Don't continue to default handling if callback is set */
  }

#if EV_AVOID_STDIO
  ev_printerr(msg);
  ev_printerr(": ");
  ev_printerr(strerror(errno));
  ev_printerr("\n");
#else
  perror(msg);
#endif
  abort();
}

static void* ev_realloc_emul(void* ptr, long size) EV_NOEXCEPT {
  /* some systems, notably openbsd and darwin, fail to properly
   * implement realloc (x, 0) (as required by both ansi c-89 and
   * the single unix specification, so work around them here.
   * recently, also (at least) fedora and debian started breaking it,
   * despite documenting it otherwise.
   */

  if (size)
    return realloc(ptr, size);

  free(ptr);
  return 0;
}

static void* (*alloc)(void* ptr, long size)EV_NOEXCEPT = ev_realloc_emul;

ecb_cold void ev_set_allocator(void* (*cb)(void* ptr, long size)EV_NOEXCEPT) EV_NOEXCEPT {
  alloc = cb;
}

inline_speed void* ev_realloc(void* ptr, long size) {
  ptr = alloc(ptr, size);

  if (!ptr && size) {
#if EV_AVOID_STDIO
    ev_printerr("(libev) memory allocation failed, aborting.\n");
#else
    fprintf(stderr, "(libev) cannot allocate %ld bytes, aborting.", size);
#endif
    abort();
  }

  return ptr;
}

#define ev_malloc(size) ev_realloc(0, (size))
#define ev_free(ptr) ev_realloc((ptr), 0)

/*****************************************************************************/

/* set in reify when reification needed */
#define EV_ANFD_REIFY 1

/* file descriptor info structure */
typedef struct {
  WL head;
  unsigned char events; /* the events watched for */
  unsigned char emask;  /* some backends store the actual kernel mask in here */
  unsigned char eflags; /* flags field for use by backends */
  unsigned int reify;   /* flag set when this ANFD needs reification (EV_ANFD_REIFY, EV__IOFDSET) */
#if EV_USE_EPOLL
  unsigned int egen; /* generation counter to counter epoll bugs */
#endif
#if EV_SELECT_IS_WINSOCKET || EV_USE_IOCP
  SOCKET handle;
#endif
#if EV_USE_IOCP
  OVERLAPPED or, ow;
#endif
} ANFD;

/* stores the pending event set for a given watcher */
typedef struct {
  W w;
  int events; /* the pending event set for the given watcher */
} ANPENDING;

#if EV_USE_INOTIFY
/* hash table entry per inotify-id */
typedef struct {
  WL head;
} ANFS;
#endif

/* Heap Entry */
#if EV_HEAP_CACHE_AT
/* a heap element */
typedef struct {
  ev_tstamp at;
  WT w;
} ANHE;

#define ANHE_w(he) (he).w                      /* access watcher, read-write */
#define ANHE_at(he) (he).at                    /* access cached at, read-only */
#define ANHE_at_cache(he) (he).at = (he).w->at /* update at from watcher */
#else
/* a heap element */
typedef WT ANHE;

#define ANHE_w(he) (he)
#define ANHE_at(he) (he)->at
#define ANHE_at_cache(he)
#endif

#if EV_MULTIPLICITY

struct ev_loop {
  ev_tstamp ev_rt_now;
#define ev_rt_now ((loop)->ev_rt_now)
#define VAR(name, decl) decl;
#include "ev_vars.h"
#undef VAR
};
#include "ev_wrap.h"

static struct ev_loop default_loop_struct;
#ifdef EV_API_STATIC
#define EV_API_DECL_DEF static
#else
#define EV_API_DECL_DEF
#endif

EV_API_DECL_DEF struct ev_loop* ev_default_loop_ptr = 0;

#else

EV_API_DECL_DEF ev_tstamp ev_rt_now = EV_TS_CONST(0.);
#define VAR(name, decl) static decl;
#include "ev_vars.h"
#undef VAR

static int ev_default_loop_ptr;

#endif

#if EV_FEATURE_API
#define EV_RELEASE_CB               \
  if (ecb_expect_false(release_cb)) \
  release_cb(EV_A)
#define EV_ACQUIRE_CB               \
  if (ecb_expect_false(acquire_cb)) \
  acquire_cb(EV_A)
#define EV_INVOKE_PENDING invoke_cb(EV_A)
#else
#define EV_RELEASE_CB (void)0
#define EV_ACQUIRE_CB (void)0
#define EV_INVOKE_PENDING ev_invoke_pending(EV_A)
#endif

#define EVBREAK_RECURSE 0x80

/*****************************************************************************/

#include "ev_time.c"

/*****************************************************************************/

#include "ev_loop_core.c"

/*****************************************************************************/

#include "ev_child.c"

/*****************************************************************************/

#include "ev_timerfd.c"

/*****************************************************************************/

#if EV_USE_IOCP
#include "ev_iocp.c"
#endif
#if EV_USE_PORT
#include "ev_port.c"
#endif
#if EV_USE_KQUEUE
#include "ev_kqueue.c"
#endif
#if EV_USE_EPOLL
#include "ev_epoll.c"
#endif
#if EV_USE_LINUXAIO
#include "ev_linuxaio.c"
#endif
#if EV_USE_IOURING
#include "ev_iouring.c"
#endif
#if EV_USE_POLL
#include "ev_poll.c"
#endif
#if EV_USE_SELECT
#include "ev_select.c"
#endif

#include "ev_api.c"
