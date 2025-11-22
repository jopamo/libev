#include <stdio.h>
#include <stdlib.h>
#include "ev.h"
#include "compat_common.h"

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static void dummy_cb(EV_P_ ev_watcher* w, int revents) {
  (void)w;
  (void)revents;
  (void)loop; /* Mark loop as unused to prevent warnings */
}

int main(void) {
  struct ev_compat_baseline base;
  ev_compat_load_baseline(&base);

  ev_watcher w_local;
  ev_init(&w_local, dummy_cb);
  w_local.data = NULL;

  ev_set_priority(&w_local, EV_MAXPRI + 5);
  if (ev_priority(&w_local) != EV_MAXPRI)
    die_msg("local priority clamp above max failed");

  ev_set_priority(&w_local, EV_MINPRI - 5);
  if (ev_priority(&w_local) != EV_MINPRI)
    die_msg("local priority clamp below min failed");

  ev_watcher w_base;
  ev_init(&w_base, dummy_cb);
  w_base.data = NULL;

  ev_set_priority(&w_base, EV_MAXPRI + 5);
  if (ev_priority(&w_base) != EV_MAXPRI)
    die_msg("baseline priority clamp above max failed");

  ev_set_priority(&w_base, EV_MINPRI - 5);
  if (ev_priority(&w_base) != EV_MINPRI)
    die_msg("baseline priority clamp below min failed");

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
