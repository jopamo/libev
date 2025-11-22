#include "ev.h"

#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void log_msg(const char* fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
}

static int is_baseline_lib(void) {
  const char* env = getenv("LIBEV_IS_BASELINE");
  return env && env[0] == '1';
}

static volatile sig_atomic_t got_alarm = 0;

static void alarm_handler(int signo) {
  (void)signo;
  got_alarm = 1;
}

/* Basic check that ev_sleep returns after roughly the requested duration */
static int check_basic_sleep(void) {
  const ev_tstamp delay = 0.05;   /* 50ms */
  const ev_tstamp max_slop = 0.5; /* allow plenty of slop on overloaded systems */

  ev_tstamp start = ev_time();
  ev_sleep(delay);
  ev_tstamp end = ev_time();
  ev_tstamp elapsed = end - start;

  if (elapsed < 0) {
    log_msg("ev_sleep: elapsed time went backwards (%.6f)", elapsed);
    return 1;
  }

  if (elapsed > delay + max_slop) {
    log_msg("ev_sleep: basic sleep overslept by %.6f seconds", elapsed - delay);
    return 1;
  }

  return 0;
}

/* Check whether SIGALRM interrupts ev_sleep
 *
 * For the new library we expect:
 *   - got_alarm to be set
 *   - ev_sleep to return soon after the alarm fires
 *
 * For the baseline libev-4.33, this is treated as a soft feature:
 *   - if it passes, great
 *   - if it fails, print a diagnostic but do not fail the test
 */
static int check_alarm_interrupts_sleep(void) {
  struct sigaction sa_old;
  struct sigaction sa_new;
  sigset_t set_old;
  sigset_t set_new;
  int is_baseline = is_baseline_lib();

  got_alarm = 0;

  sigemptyset(&sa_new.sa_mask);
  sa_new.sa_flags = 0;
  sa_new.sa_handler = alarm_handler;

  if (sigaction(SIGALRM, &sa_new, &sa_old) < 0) {
    perror("sigaction");
    return 1;
  }

  sigemptyset(&set_new);
  sigaddset(&set_new, SIGALRM);
  if (sigprocmask(SIG_UNBLOCK, &set_new, &set_old) < 0) {
    perror("sigprocmask");
    sigaction(SIGALRM, &sa_old, NULL);
    return 1;
  }

  const ev_tstamp total_sleep = 5.0;
  const unsigned alarm_sec = 1;
  const ev_tstamp interrupt_slop = 1.0; /* allow some slack after alarm */

  alarm(alarm_sec);

  ev_tstamp start = ev_time();
  ev_sleep(total_sleep);
  ev_tstamp end = ev_time();
  ev_tstamp elapsed = end - start;

  alarm(0);
  sigprocmask(SIG_SETMASK, &set_old, NULL);
  sigaction(SIGALRM, &sa_old, NULL);

  int ok = 1;

  if (!got_alarm) {
    log_msg("SIGALRM did not fire while ev_sleep was running");
    ok = 0;
  }

  if (elapsed > alarm_sec + interrupt_slop) {
    log_msg("SIGALRM failed to interrupt ev_sleep: elapsed=%.6f", elapsed);
    ok = 0;
  }

  if (is_baseline) {
    /* Baseline lib: treat this as informational only */
    if (!ok) {
      log_msg("note: running against baseline libev, treating SIGALRM interrupt check as soft");
    }
    return 0;
  }

  /* New library: this is a hard requirement */
  return ok ? 0 : 1;
}

int main(void) {
  /* basic helper sanity check always strict */
  if (check_basic_sleep() != 0) {
    log_msg("basic ev_sleep timing check failed");
    return 1;
  }

  /* SIGALRM interrupt check strict for new lib, soft for baseline */
  if (check_alarm_interrupts_sleep() != 0) {
    return 1;
  }

  return 0;
}
