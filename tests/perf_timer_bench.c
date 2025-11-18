#include <ev.h>

#include "perf_bench_common.h"

static int target_iterations;
static int timer_hits;

static void timer_cb(EV_P_ ev_timer* w, int revents) {
  (void)w;
  (void)revents;

  ++timer_hits;

  if (timer_hits >= target_iterations) {
    ev_break(EV_A_ EVBREAK_ALL);
    return;
  }

  ev_timer_again(EV_A_ w);
}

int main(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop) {
    fprintf(stderr, "failed to create ev loop\n");
    return 1;
  }

  target_iterations = bench_read_iterations();
  timer_hits = 0;

  ev_timer timer_watcher;
  ev_timer_init(&timer_watcher, timer_cb, 0., 0.);
  ev_timer_start(loop, &timer_watcher);

  struct timespec start;
  struct timespec end;

  if (bench_clock_now(&start) != 0) {
    perror("clock_gettime(START)");
    return 2;
  }

  ev_run(loop, 0);

  if (bench_clock_now(&end) != 0) {
    perror("clock_gettime(END)");
    return 3;
  }

  ev_loop_destroy(loop);

  double seconds = bench_elapsed_seconds(&start, &end);
  bench_print_result("timer", target_iterations, seconds, ev_version_major(), ev_version_minor());

  return 0;
}
