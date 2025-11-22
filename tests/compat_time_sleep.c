#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "ev.h"
#include "compat_common.h"

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static void check_time_pair(ev_time_fn time_a, ev_time_fn time_b) {
  ev_tstamp a_before = time_a();
  ev_tstamp b_before = time_b();

  if (a_before <= 0. || b_before <= 0.)
    die_msg("ev_time returned non-positive timestamp in compat check");

  ev_sleep(0.01);

  ev_tstamp a_after = time_a();
  ev_tstamp b_after = time_b();

  if (a_after + 1e-9 < a_before)
    die_msg("local ev_time moved backwards in compat check");
  if (b_after + 1e-9 < b_before)
    die_msg("baseline ev_time moved backwards in compat check");

  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0)
    die_msg("gettimeofday failed");

  ev_tstamp wall = (ev_tstamp)tv.tv_sec + (ev_tstamp)tv.tv_usec * 1e-6;
  ev_tstamp drift_a = fabs(wall - a_after);
  ev_tstamp drift_b = fabs(wall - b_after);

  if (drift_a > 0.5)
    die_msg("local ev_time drifted too far from gettimeofday in compat check");
  if (drift_b > 0.5)
    die_msg("baseline ev_time drifted too far from gettimeofday in compat check");
}

static void check_sleep_pair(ev_sleep_fn sleep_local, ev_sleep_fn sleep_baseline) {
  const ev_tstamp delay = 0.02; /* 20ms */
  const ev_tstamp max_slop = 0.5;

  /* local */
  {
    ev_tstamp start = ev_time();
    sleep_local(delay);
    ev_tstamp elapsed = ev_time() - start;

    if (elapsed < 0)
      die_msg("local ev_sleep elapsed went backwards");
    if (elapsed > delay + max_slop)
      die_msg("local ev_sleep overslept too much in compat check");
  }

  /* baseline */
  {
    ev_tstamp start = ev_time();
    sleep_baseline(delay);
    ev_tstamp elapsed = ev_time() - start;

    if (elapsed < 0)
      die_msg("baseline ev_sleep elapsed went backwards");
    if (elapsed > delay + max_slop)
      die_msg("baseline ev_sleep overslept too much in compat check");
  }
}

int main(void) {
  struct ev_compat_baseline base;

  ev_compat_load_baseline(&base);

  /* versions should match major/minor exactly */
  if (ev_version_major() != base.ev_version_major())
    die_msg("ev_version_major differs between local and baseline");
  if (ev_version_minor() != base.ev_version_minor())
    die_msg("ev_version_minor differs between local and baseline");

  /* simple time sanity and drift checks for both implementations */
  check_time_pair(ev_time, base.ev_time);

  /* simple sleep sanity for both implementations
   * this only checks "does roughly the right thing", not SIGALRM semantics
   */
  check_sleep_pair(ev_sleep, base.ev_sleep);

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
