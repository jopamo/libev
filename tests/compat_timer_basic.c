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

static double measure_sleep(ev_sleep_fn sleep_fn, double delay) {
  ev_tstamp start = ev_time();
  sleep_fn(delay);
  return ev_time() - start;
}

int main(void) {
  struct ev_compat_baseline base;
  ev_compat_load_baseline(&base);

  const double delay = 0.02; /* 20ms */
  const double slack = 0.5;

  double elapsed_local = measure_sleep(ev_sleep, delay);
  double elapsed_baseline = measure_sleep(base.ev_sleep, delay);

  if (elapsed_local < 0.0)
    die_msg("local ev_sleep negative elapsed");
  if (elapsed_local > delay + slack)
    die_msg("local ev_sleep overslept too much");

  if (elapsed_baseline < 0.0)
    die_msg("baseline ev_sleep negative elapsed");
  if (elapsed_baseline > delay + slack)
    die_msg("baseline ev_sleep overslept too much");

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
