#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "ev.h"

static void die_errno(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static void test_ev_time_samples(void) {
  ev_tstamp before = ev_time();
  if (before <= 0.)
    die_msg("ev_time returned non-positive timestamp");

  ev_sleep(0.01);

  ev_tstamp after = ev_time();
  if (after + 1e-9 < before)
    die_msg("ev_time moved backwards");

  ev_tstamp delta = after - before;
  if (delta < 0.004)
    die_msg("ev_time did not advance enough");

  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0)
    die_errno("gettimeofday");

  ev_tstamp wall = (ev_tstamp)tv.tv_sec + (ev_tstamp)tv.tv_usec * 1e-6;
  ev_tstamp drift = fabs(wall - after);
  if (drift > 0.25)
    die_msg("ev_time drifted too far from gettimeofday");
}

#ifndef LIBEV_IS_BASELINE

static volatile sig_atomic_t alarm_hits;

static void alarm_handler(int signum) {
  (void)signum;
  ++alarm_hits;
}

/* Check that ev_sleep is interruptible by SIGALRM and returns early
 * New implementation does a single nanosleep and does not retry on EINTR
 */
static void test_ev_sleep_retries(void) {
  alarm_hits = 0;

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = alarm_handler;
  sigemptyset(&sa.sa_mask);
  struct sigaction old_sa;

  if (sigaction(SIGALRM, &sa, &old_sa) != 0)
    die_errno("sigaction");

  sigset_t newset;
  sigset_t oldset;
  sigemptyset(&newset);
  sigaddset(&newset, SIGALRM);
  if (sigprocmask(SIG_UNBLOCK, &newset, &oldset) != 0)
    die_errno("sigprocmask");

  const ev_tstamp requested = 5.0;
  const unsigned alarm_sec = 1;
  const ev_tstamp interrupt_slop = 1.0;

  alarm(alarm_sec);

  ev_tstamp start = ev_time();
  ev_sleep(requested);
  ev_tstamp elapsed = ev_time() - start;

  alarm(0);

  if (sigprocmask(SIG_SETMASK, &oldset, NULL) != 0)
    die_errno("sigprocmask restore");
  if (sigaction(SIGALRM, &old_sa, NULL) != 0)
    die_errno("sigaction restore");

  if (!alarm_hits)
    die_msg("SIGALRM did not fire while ev_sleep was running");

  if (elapsed < 0.1)
    die_msg("ev_sleep returned too early before alarm could fire");

  if (elapsed > alarm_sec + interrupt_slop)
    die_msg("SIGALRM failed to interrupt ev_sleep");
}

#else

/* Baseline libev may retry nanosleep on EINTR
 * For the reference library we keep this as a smoke test only
 */
static void test_ev_sleep_retries(void) {
  ev_sleep(0.05);
}

#endif

static int alloc_call_count;
static int free_call_count;

static void noop_timer_cb(EV_P_ ev_timer* w, int revents) {
  (void)EV_A;
  (void)w;
  (void)revents;
}

static void* counting_allocator(void* ptr, long size) EV_NOEXCEPT {
  if (size)
    ++alloc_call_count;
  else
    ++free_call_count;

  if (size)
    return realloc(ptr, size);

  free(ptr);
  return 0;
}

static void* default_allocator(void* ptr, long size) EV_NOEXCEPT {
  if (size)
    return realloc(ptr, size);

  free(ptr);
  return 0;
}

static void test_allocator_hooks(void) {
  alloc_call_count = 0;
  free_call_count = 0;
  ev_set_allocator(counting_allocator);

  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop)
    die_msg("ev_loop_new failed under counting allocator");

  ev_timer timer;
  ev_timer_init(&timer, noop_timer_cb, 0., 0.);
  ev_timer_start(loop, &timer);
  ev_timer_stop(loop, &timer);

  ev_loop_destroy(loop);

  if (alloc_call_count == 0)
    die_msg("counting allocator never handled allocations");
  if (free_call_count == 0)
    die_msg("counting allocator never handled frees");

  ev_set_allocator(default_allocator);
}

#define MAX_FILLER_FDS 128
static int filler_fds[MAX_FILLER_FDS];
static int filler_fd_count;

static void drain_fds_handler(int signum) {
  (void)signum;
  for (int i = 0; i < filler_fd_count; ++i) {
    if (filler_fds[i] >= 0) {
      close(filler_fds[i]);
      filler_fds[i] = -1;
    }
  }
}

static volatile sig_atomic_t syserr_hits;
static const char* volatile last_syserr_msg;

static void record_syserr(const char* msg) EV_NOEXCEPT {
  ++syserr_hits;
  last_syserr_msg = msg;
}

static void close_filler_fds(void) {
  for (int i = 0; i < filler_fd_count; ++i) {
    if (filler_fds[i] >= 0) {
      close(filler_fds[i]);
      filler_fds[i] = -1;
    }
  }
  filler_fd_count = 0;
}

static void stub_signal_cb(EV_P_ ev_signal* w, int revents) {
  (void)EV_A;
  (void)w;
  (void)revents;
}

static void test_syserr_callback(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop)
    die_msg("ev_loop_new failed under fd pressure");

  struct rlimit old_limit;
  if (getrlimit(RLIMIT_NOFILE, &old_limit) != 0)
    die_errno("getrlimit");

  struct rlimit tight_limit = old_limit;
  rlim_t target = old_limit.rlim_cur;
  if (target > 64)
    target = 64;
  if (target < 8)
    target = old_limit.rlim_cur;
  tight_limit.rlim_cur = target;

  if (setrlimit(RLIMIT_NOFILE, &tight_limit) != 0)
    die_errno("setrlimit");

  filler_fd_count = 0;
  int last_errno = 0;
  while (filler_fd_count < MAX_FILLER_FDS) {
    int fd = open("/dev/null", O_RDONLY);
    if (fd < 0) {
      last_errno = errno;
      break;
    }
    filler_fds[filler_fd_count++] = fd;
  }

  if (last_errno != EMFILE)
    die_msg("failed to exhaust file descriptor limit");
  if (filler_fd_count == 0)
    die_msg("could not open filler file descriptors");

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = drain_fds_handler;
  sigemptyset(&sa.sa_mask);
  struct sigaction old_sa;
  if (sigaction(SIGALRM, &sa, &old_sa) != 0)
    die_errno("sigaction");

  struct itimerval timer;
  memset(&timer, 0, sizeof(timer));
  timer.it_value.tv_usec = 20000;
  if (setitimer(ITIMER_REAL, &timer, NULL) != 0)
    die_errno("setitimer");

  syserr_hits = 0;
  last_syserr_msg = NULL;
  ev_set_syserr_cb(record_syserr);

  ev_signal sigusr;
  ev_signal_init(&sigusr, stub_signal_cb, SIGUSR1);

  ev_signal_start(loop, &sigusr);
  ev_signal_stop(loop, &sigusr);

  struct itimerval disarm;
  memset(&disarm, 0, sizeof(disarm));
  setitimer(ITIMER_REAL, &disarm, NULL);
  sigaction(SIGALRM, &old_sa, NULL);

  ev_set_syserr_cb(NULL);
  close_filler_fds();
  if (setrlimit(RLIMIT_NOFILE, &old_limit) != 0)
    die_errno("setrlimit restore");
  ev_loop_destroy(loop);

  if (!syserr_hits)
    die_msg("syserr callback was never invoked");
  if (!last_syserr_msg || strcmp(last_syserr_msg, "(libev) error creating signal/async pipe"))
    die_msg("unexpected syserr message");
}

int main(void) {
  test_ev_time_samples();
  test_ev_sleep_retries();
  test_allocator_hooks();
  test_syserr_callback();
  return EXIT_SUCCESS;
}
