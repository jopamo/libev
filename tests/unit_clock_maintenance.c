#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ev.h"

static void failf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

static ev_tstamp ts_abs(ev_tstamp value) {
  return value < 0. ? -value : value;
}

static void check_range(const char* label, ev_tstamp value, ev_tstamp min, ev_tstamp max) {
  if (value < min || value > max) {
    failf("%s out of range [%f, %f]: %f", label, min, max, value);
  }
}

static void test_ev_now_update(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop)
    failf("ev_loop_new failed");

  ev_now_update(loop);
  ev_tstamp start = ev_now(loop);

  ev_sleep(0.05);
  ev_tstamp stale = ev_now(loop);
  if (ts_abs(stale - start) > 1e-7)
    failf("ev_now changed without ev_now_update: delta=%f", stale - start);

  ev_now_update(loop);
  ev_tstamp updated = ev_now(loop);
  if (updated - start < 0.04)
    failf("ev_now_update failed to advance time sufficiently: delta=%f", updated - start);

  ev_loop_destroy(loop);
}

static ev_tstamp timer_resume_at;
static ev_tstamp timer_fired_at;
static int timer_fired;

static void frozen_timer_cb(EV_P_ ev_timer* w, int revents) {
  (void)w;
  (void)revents;
  timer_fired++;
  timer_fired_at = ev_now(EV_A);
  ev_break(EV_A_ EVBREAK_ALL);
}

static void test_suspend_resume_timer(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop)
    failf("ev_loop_new failed for timer test");

  const ev_tstamp delay = 0.05;
  const ev_tstamp suspend_len = 0.12;

  ev_timer t;
  ev_timer_init(&t, frozen_timer_cb, delay, 0.);

  ev_now_update(loop);
  timer_fired = 0;
  ev_timer_start(loop, &t);

  ev_suspend(loop);
  ev_sleep(suspend_len);
  ev_resume(loop);
  timer_resume_at = ev_now(loop);

  while (timer_fired == 0)
    ev_run(loop, 0);

  if (timer_fired != 1)
    failf("timer fired unexpected number of times: %d", timer_fired);

  ev_timer_stop(loop, &t);

  ev_tstamp delta = timer_fired_at - timer_resume_at;
  check_range("timer resume latency", delta, delay - 0.02, delay + 0.25);

  ev_loop_destroy(loop);
}

#if EV_PERIODIC_ENABLE
static ev_tstamp periodic_times[2];
static int periodic_hits;

static void periodic_cb(EV_P_ ev_periodic* w, int revents) {
  (void)w;
  (void)revents;
  if (periodic_hits < 2)
    periodic_times[periodic_hits] = ev_now(EV_A);
  periodic_hits++;
  ev_break(EV_A_ EVBREAK_ALL);
}

static void test_suspend_resume_periodic(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop)
    failf("ev_loop_new failed for periodic test");

  const ev_tstamp interval = 0.05;
  const ev_tstamp suspend_len = 0.15;

  ev_periodic p;
  ev_now_update(loop);
  ev_tstamp now = ev_now(loop);
  periodic_hits = 0;

  ev_periodic_init(&p, periodic_cb, now + interval, interval, 0);
  ev_periodic_start(loop, &p);

  while (periodic_hits < 1)
    ev_run(loop, 0);

  ev_suspend(loop);
  ev_sleep(suspend_len);
  ev_resume(loop);
  ev_tstamp resume_time = ev_now(loop);

  while (periodic_hits < 2)
    ev_run(loop, 0);

  ev_periodic_stop(loop, &p);
  if (periodic_hits != 2)
    failf("periodic fired unexpected number of times: %d", periodic_hits);

  ev_tstamp post_resume_delay = periodic_times[1] - resume_time;
  check_range("periodic resume latency", post_resume_delay, 0., 0.02);

  ev_tstamp between = periodic_times[1] - periodic_times[0];
  if (between < suspend_len - 0.01 || between > suspend_len + 0.05)
    failf("periodic callbacks not spaced by suspend duration: %f", between);

  ev_loop_destroy(loop);
}
#else
static void test_suspend_resume_periodic(void) {
  /* Periodic watchers are disabled in this build; nothing to test. */
}
#endif

int main(void) {
  test_ev_now_update();
  test_suspend_resume_timer();
  test_suspend_resume_periodic();
  return EXIT_SUCCESS;
}
