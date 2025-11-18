#include <ev.h>

#include "perf_bench_common.h"

static int target_iterations;
static int loop_iterations;

static void idle_cb(EV_P_ ev_idle* w, int revents);
static void prepare_cb(EV_P_ ev_prepare* w, int revents);
static void check_cb(EV_P_ ev_check* w, int revents);

static int run_prepare_check_bench(double* seconds_out) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop) {
    fprintf(stderr, "failed to create ev loop\n");
    return 1;
  }

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
    ev_loop_destroy(loop);
    return 2;
  }

  ev_run(loop, 0);

  if (bench_clock_now(&end) != 0) {
    perror("clock_gettime(END)");
    ev_loop_destroy(loop);
    return 3;
  }

  ev_loop_destroy(loop);

  *seconds_out = bench_elapsed_seconds(&start, &end);
  return 0;
}

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
  target_iterations = bench_read_iterations();
  const int runs = bench_read_runs();

  double total_seconds = 0.0;
  for (int i = 0; i < runs; ++i) {
    double seconds = 0.0;
    int rc = run_prepare_check_bench(&seconds);
    if (rc != 0) {
      return rc;
    }
    total_seconds += seconds;
  }

  const double avg_seconds = total_seconds / runs;
  bench_print_result("prepare-check", target_iterations, avg_seconds, ev_version_major(), ev_version_minor(), runs);

  return 0;
}
