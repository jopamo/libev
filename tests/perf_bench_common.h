#ifndef PERF_BENCH_COMMON_H
#define PERF_BENCH_COMMON_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static inline int bench_read_runs(void) {
  const char* env = getenv("LIBEV_BENCH_RUNS");

  if (!env || env[0] == '\0') {
    return 5;
  }

  char* endptr = NULL;
  long parsed = strtol(env, &endptr, 10);

  if (endptr == env || parsed <= 0) {
    return 5;
  }

  if (parsed > INT_MAX) {
    parsed = INT_MAX;
  }

  return (int)parsed;
}

static inline int bench_read_iterations(void) {
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

static inline double bench_elapsed_seconds(const struct timespec* start, const struct timespec* end) {
  const double sec = (double)(end->tv_sec - start->tv_sec);
  const double nsec = (double)(end->tv_nsec - start->tv_nsec);
  return sec + (nsec / 1e9);
}

static inline int bench_clock_now(struct timespec* ts) {
  if (clock_gettime(CLOCK_MONOTONIC, ts) != 0) {
    return -1;
  }
  return 0;
}

static inline void bench_print_result(const char* scenario,
                                      int iterations,
                                      double avg_seconds,
                                      int version_major,
                                      int version_minor,
                                      int runs) {
  if (avg_seconds <= 0.0) {
    avg_seconds = 1e-9;
  }

  const double per_second = (double)iterations / avg_seconds;

  printf("scenario=%s version=%d.%d iterations=%d runs=%d avg_seconds=%.6f per_second=%.0f\n",
         scenario ? scenario : "unknown", version_major, version_minor, iterations, runs, avg_seconds, per_second);
}

#endif
