#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "ev.h"

static void die_errno(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static int future_timer_hits;

static void future_timer_cb(EV_P_ ev_timer* w, int revents) {
  (void)EV_A;
  (void)w;
  (void)revents;

  ++future_timer_hits;
}

static unsigned int depth_seen_once;
static int once_timer_hits;

static void once_timer_cb(EV_P_ ev_timer* w, int revents) {
  (void)revents;

  ++once_timer_hits;
  depth_seen_once = ev_loop_depth(EV_A);
  if (depth_seen_once != 1)
    die_msg("EVRUN_ONCE did not report depth=1 while running");

  ev_timer_stop(EV_A_ w);
}

static int cancel_iterations;

static void cancel_break_timer_cb(EV_P_ ev_timer* w, int revents) {
  (void)revents;

  ++cancel_iterations;

  if (cancel_iterations == 1) {
    ev_break(EV_A_ EVBREAK_ONE);
    ev_break(EV_A_ EVBREAK_CANCEL);
    return;
  }

  ev_timer_stop(EV_A_ w);
  ev_break(EV_A_ EVBREAK_ONE);
}

static ev_timer nested_inner_timer;
static int nested_outer_hits;
static int nested_inner_hits;
static unsigned int depth_seen_inner;
static int outer_saw_inner_break;

static void nested_inner_cb(EV_P_ ev_timer* w, int revents) {
  (void)revents;

  ++nested_inner_hits;
  depth_seen_inner = ev_loop_depth(EV_A);
  if (depth_seen_inner != 2)
    die_msg("nested ev_run did not report depth=2");

  ev_timer_stop(EV_A_ w);
  ev_break(EV_A_ EVBREAK_ALL);
}

static void nested_outer_cb(EV_P_ ev_timer* w, int revents) {
  (void)revents;

  ++nested_outer_hits;

  if (ev_loop_depth(EV_A) != 1)
    die_msg("outer ev_run depth tracking broken");

  ev_timer_stop(EV_A_ w);

  ev_timer_init(&nested_inner_timer, nested_inner_cb, 0., 0.);
  ev_timer_start(EV_A_ &nested_inner_timer);

  int rc = ev_run(EV_A_ 0);
  if (rc != 0)
    die_msg("nested ev_run should exit with no active watchers after EVBREAK_ALL");

  outer_saw_inner_break = 1;
}

static void test_nowait_iteration(struct ev_loop* loop) {
  future_timer_hits = 0;

  ev_timer far_future;
  ev_timer_init(&far_future, future_timer_cb, 60., 0.);
  ev_timer_start(loop, &far_future);

  unsigned int before_iters = ev_loop_count(loop);
  unsigned int before_depth = ev_loop_depth(loop);
  if (before_depth != 0)
    die_msg("loop depth should be zero outside ev_run");

  int rc = ev_run(loop, EVRUN_NOWAIT);
  if (rc != 1)
    die_msg("EVRUN_NOWAIT should leave far-future timer active");

  if (future_timer_hits != 0)
    die_msg("EVRUN_NOWAIT should not fire timers that are not pending");

  unsigned int after_iters = ev_loop_count(loop);
  if (after_iters != before_iters + 1)
    die_msg("EVRUN_NOWAIT should execute exactly one iteration");

  if (ev_loop_depth(loop) != 0)
    die_msg("loop depth leaked after EVRUN_NOWAIT");

  ev_timer_stop(loop, &far_future);
}

static void test_run_once_semantics(struct ev_loop* loop) {
  once_timer_hits = 0;
  depth_seen_once = 0;

  ev_timer single_shot;
  ev_timer_init(&single_shot, once_timer_cb, 0.01, 0.);
  ev_timer_start(loop, &single_shot);

  unsigned int before_iters = ev_loop_count(loop);

  int rc = ev_run(loop, EVRUN_ONCE);
  if (rc != 0)
    die_msg("EVRUN_ONCE should drain ready watchers before exiting");

  if (once_timer_hits != 1)
    die_msg("EVRUN_ONCE should deliver timer exactly once");

  if (depth_seen_once != 1)
    die_msg("missing loop depth bookkeeping inside EVRUN_ONCE");

  if (ev_is_active(&single_shot))
    die_msg("oneshot timer should be stopped after firing");

  if (ev_loop_count(loop) != before_iters + 1)
    die_msg("EVRUN_ONCE expected to run exactly one backend iteration");

  if (ev_loop_depth(loop) != 0)
    die_msg("loop depth leaked after EVRUN_ONCE");
}

static void test_ev_break_variants(struct ev_loop* loop) {
  cancel_iterations = 0;

  ev_timer cancel_timer;
  ev_timer_init(&cancel_timer, cancel_break_timer_cb, 0., 0.02);
  ev_timer_start(loop, &cancel_timer);

  int rc = ev_run(loop, 0);
  if (rc != 0)
    die_msg("EVBREAK cancel/one flow should leave no active watchers");

  if (cancel_iterations != 2)
    die_msg("EVBREAK_CANCEL failed to resume loop before EVBREAK_ONE");

  nested_outer_hits = 0;
  nested_inner_hits = 0;
  depth_seen_inner = 0;
  outer_saw_inner_break = 0;

  ev_timer outer_timer;
  ev_timer_init(&outer_timer, nested_outer_cb, 0., 0.);
  ev_timer_start(loop, &outer_timer);

  rc = ev_run(loop, 0);
  if (rc != 0)
    die_msg("EVBREAK_ALL should unwind with no active watchers");

  if (nested_outer_hits != 1 || nested_inner_hits != 1)
    die_msg("nested timers not executed exactly once");

  if (!outer_saw_inner_break)
    die_msg("outer callback did not observe nested break");

  if (depth_seen_inner != 2)
    die_msg("loop depth not incremented for nested ev_run");

  if (ev_loop_depth(loop) != 0)
    die_msg("loop depth leaked after EVBREAK_ALL");
}

int main(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop)
    die_errno("ev_loop_new");

  test_nowait_iteration(loop);
  test_run_once_semantics(loop);
  test_ev_break_variants(loop);

  ev_loop_destroy(loop);
  return EXIT_SUCCESS;
}
