#include <ev.h>

#include "perf_bench_common.h"

static int target_iterations;
static int loop_iterations;

static void idle_cb(EV_P_ ev_idle* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
}

static void prepare_cb(EV_P_ ev_prepare* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
}

static void check_cb(EV_P_ ev_check* w, int revents) {
  (void)w;
  (void)revents;

  ++loop_iterations;
  if (loop_iterations >= target_iterations) {
    ev_break(EV_A_ EVBREAK_ALL);
  }
}

int main(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop) {
    fprintf(stderr, "failed to create ev loop\n");
    return 1;
  }

  target_iterations = bench_read_iterations();
  loop_iterations = 0;

  ev_idle idle_watcher;
  ev_idle_init(&idle_watcher, idle_cb);
  ev_idle_start(loop, &idle_watcher);

  ev_prepare prepare_watcher;
  ev_prepare_init(&prepare_watcher, prepare_cb);
  ev_prepare_start(loop, &prepare_watcher);

  ev_check check_watcher;
  ev_check_init(&check_watcher, check_cb);
  ev_check_start(loop, &check_watcher);

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
  bench_print_result("prepare-check", target_iterations, seconds, ev_version_major(), ev_version_minor());

  return 0;
}
