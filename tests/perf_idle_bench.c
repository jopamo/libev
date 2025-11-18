#include <ev.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int target_iterations;
static int idle_hits;

static void idle_cb(EV_P_ ev_idle* w, int revents) {
  (void)w;
  (void)revents;

  ++idle_hits;
  if (idle_hits >= target_iterations) {
    ev_break(EV_A_ EVBREAK_ALL);
  }
}

static int read_iterations(void) {
  const char* env = getenv("LIBEV_BENCH_ITERATIONS");

  if (!env || env[0] == '\0') {
    return 200000;
  }

  char* endptr = NULL;
  long parsed = strtol(env, &endptr, 10);

  if (endptr == env || parsed <= 0) {
    return 200000;
  }

  if (parsed > INT_MAX) {
    parsed = INT_MAX;
  }

  return (int)parsed;
}

static double elapsed_seconds(const struct timespec* start, const struct timespec* end) {
  const double sec = (double)(end->tv_sec - start->tv_sec);
  const double nsec = (double)(end->tv_nsec - start->tv_nsec);
  return sec + (nsec / 1e9);
}

int main(void) {
  struct ev_loop* loop = ev_loop_new(EVFLAG_AUTO);
  if (!loop) {
    fprintf(stderr, "failed to create ev loop\n");
    return 1;
  }

  target_iterations = read_iterations();
  idle_hits = 0;

  ev_idle idle_watcher;
  ev_idle_init(&idle_watcher, idle_cb);
  ev_idle_start(loop, &idle_watcher);

  struct timespec start;
  struct timespec end;
  if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
    perror("clock_gettime(START)");
    return 2;
  }

  ev_run(loop, 0);

  if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
    perror("clock_gettime(END)");
    return 3;
  }

  ev_loop_destroy(loop);

  double seconds = elapsed_seconds(&start, &end);
  if (seconds <= 0.0) {
    seconds = 1e-9;
  }

  const double per_second = (double)target_iterations / seconds;

  printf("version=%d.%d iterations=%d seconds=%.6f per_second=%.0f\n", ev_version_major(), ev_version_minor(),
         target_iterations, seconds, per_second);

  return 0;
}
