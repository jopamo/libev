#include <stdio.h>
#include <stdlib.h>

#include "ev.h"

static void die(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static int userdata_hits;

static void userdata_idle_cb(EV_P_ ev_idle* w, int revents) {
  (void)revents;

  ++userdata_hits;
  if (ev_userdata(EV_A) != w->data)
    die("ev_userdata did not propagate to callback");

  ev_idle_stop(EV_A_ w);
}

static void test_userdata_and_pending(struct ev_loop* loop) {
  userdata_hits = 0;

  int token = 0x1234;
  ev_set_userdata(loop, &token);
  if (ev_userdata(loop) != &token)
    die("ev_set_userdata failed to store value");

  ev_idle watcher;
  ev_idle_init(&watcher, userdata_idle_cb);
  watcher.data = &token;
  ev_idle_start(loop, &watcher);

  if (ev_pending_count(loop) != 0)
    die("pending count should be zero before feeding events");

  ev_feed_event(loop, &watcher, EV_CUSTOM);

  if (ev_pending_count(loop) != 1)
    die("pending count did not reflect fed event");

  ev_run(loop, EVRUN_ONCE);

  if (userdata_hits != 1)
    die("userdata watcher should run exactly once");

  if (ev_pending_count(loop) != 0)
    die("pending count should be zero after drain");

  if (ev_is_active(&watcher))
    die("idle watcher should have stopped");

  ev_set_userdata(loop, NULL);
  if (ev_userdata(loop) != NULL)
    die("ev_set_userdata failed to clear value");
}

static int ref_idle_hits;
static int stop_ref_idle;

static void ref_idle_cb(EV_P_ ev_idle* w, int revents) {
  (void)revents;

  ++ref_idle_hits;

  if (stop_ref_idle) {
    ev_idle_stop(EV_A_ w);
  }
}

static void test_refcount_hooks(struct ev_loop* loop) {
  ev_idle watcher;
  ev_idle_init(&watcher, ref_idle_cb);
  ev_idle_start(loop, &watcher);

  ref_idle_hits = 0;
  stop_ref_idle = 0;

  ev_unref(loop);
  int run_result = ev_run(loop, 0);
  if (run_result != 0)
    die("ev_run should report zero active watchers after unref");

  if (ref_idle_hits != 1)
    die("idle watcher should run once while unreferenced");

  if (!ev_is_active(&watcher))
    die("idle watcher should remain active after unreferenced run");

  ev_ref(loop);
  stop_ref_idle = 1;
  run_result = ev_run(loop, 0);
  if (run_result != 0)
    die("ev_run should exit cleanly after watcher stops");

  if (ref_idle_hits < 2)
    die("idle watcher did not run again after ev_ref");

  if (ev_is_active(&watcher))
    die("idle watcher should stop after second run");
}

static unsigned int invoke_wrapper_calls;
static unsigned int max_pending_seen;
static int invoke_timer_hits;

static void custom_invoke_pending(EV_P) {
  unsigned int pending = ev_pending_count(EV_A);
  if (pending > max_pending_seen)
    max_pending_seen = pending;

  ++invoke_wrapper_calls;
  ev_invoke_pending(EV_A);
}

static void invoke_timer_cb(EV_P_ ev_timer* w, int revents) {
  (void)revents;

  ++invoke_timer_hits;
  ev_timer_stop(EV_A_ w);
}

static void test_invoke_pending_hook(struct ev_loop* loop) {
  invoke_wrapper_calls = 0;
  max_pending_seen = 0;
  invoke_timer_hits = 0;

  ev_set_invoke_pending_cb(loop, custom_invoke_pending);

  ev_timer timer_a;
  ev_timer timer_b;
  ev_timer_init(&timer_a, invoke_timer_cb, 0., 0.);
  ev_timer_init(&timer_b, invoke_timer_cb, 0., 0.);
  ev_timer_start(loop, &timer_a);
  ev_timer_start(loop, &timer_b);

  ev_run(loop, 0);

  if (invoke_timer_hits != 2)
    die("timers should have fired exactly twice");

  if (invoke_wrapper_calls == 0)
    die("custom invoke_pending hook was not called");

  if (max_pending_seen < 2)
    die("custom invoke_pending hook did not observe queued watchers");

  ev_set_invoke_pending_cb(loop, ev_invoke_pending);
}

static unsigned int release_calls;
static unsigned int acquire_calls;
static int release_timer_hits;

static void release_hook(EV_P) {
  (void)loop;
  ++release_calls;
}

static void acquire_hook(EV_P) {
  (void)loop;
  ++acquire_calls;
}

static void release_timer_cb(EV_P_ ev_timer* w, int revents) {
  (void)revents;

  ++release_timer_hits;
  ev_timer_stop(EV_A_ w);
}

static void test_release_hooks(struct ev_loop* loop) {
  release_calls = 0;
  acquire_calls = 0;
  release_timer_hits = 0;

  ev_set_loop_release_cb(loop, release_hook, acquire_hook);

  ev_timer timer;
  ev_timer_init(&timer, release_timer_cb, 0., 0.);
  ev_timer_start(loop, &timer);

  ev_run(loop, 0);

  if (release_timer_hits != 1)
    die("release test timer should have fired once");

  if (release_calls == 0 || acquire_calls == 0)
    die("release/acquire hooks were not invoked");

  if (release_calls != acquire_calls)
    die("release/acquire hooks should run in pairs");

  ev_set_loop_release_cb(loop, NULL, NULL);
}

int main(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop)
    die("ev_loop_new failed");

  test_userdata_and_pending(loop);
  test_refcount_hooks(loop);
  test_invoke_pending_hook(loop);
  test_release_hooks(loop);

  ev_loop_destroy(loop);
  return EXIT_SUCCESS;
}
