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

  struct ev_loop* loop_local = ev_loop_new(0);
  ev_watcher w_local;
  ev_init(&w_local, dummy_cb);
  w_local.data = NULL;

  /* feed twice before run */
  ev_feed_event(loop_local, &w_local, EV_READ);
  ev_feed_event(loop_local, &w_local, EV_WRITE);
  int pending_local = ev_clear_pending(loop_local, &w_local);
  if ((pending_local & EV_READ) == 0 || (pending_local & EV_WRITE) == 0)
    die_msg("local ev_clear_pending missing proper mask");

  ev_loop_destroy(loop_local);

  struct ev_loop* loop_base = ev_loop_new(0);
  ev_watcher w_base;
  ev_init(&w_base, dummy_cb);
  w_base.data = NULL;
  ev_feed_event(loop_base, &w_base, EV_READ);
  ev_feed_event(loop_base, &w_base, EV_WRITE);
  int pending_base = ev_clear_pending(loop_base, &w_base);
  if ((pending_base & EV_READ) == 0 || (pending_base & EV_WRITE) == 0)
    die_msg("baseline ev_clear_pending missing proper mask");

  ev_loop_destroy(loop_base);

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
