#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ev.h"
#include "compat_common.h"

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static volatile int cb_count_local;
static volatile int cb_count_base;

static void io_cb_local(struct ev_loop* loop, ev_io* w, int revents) {
  (void)loop;
  (void)revents;
  cb_count_local++;
  ev_io_stop(loop, w);
  ev_break(loop, EVBREAK_ONE);
}

static void io_cb_baseline(struct ev_loop* loop, ev_io* w, int revents) {
  (void)loop;
  (void)revents;
  cb_count_base++;
  ev_io_stop(loop, w);
  ev_break(loop, EVBREAK_ONE);
}

int main(void) {
  struct ev_compat_baseline base;
  ev_compat_load_baseline(&base);

  int fds[2];
  if (pipe(fds) != 0)
    die_msg("pipe() failed");

  /* local */
  {
    struct ev_loop* loop = ev_loop_new(0);
    cb_count_local = 0;
    ev_io watcher;
    ev_io_init(&watcher, io_cb_local, fds[0], EV_READ);
    ev_io_start(loop, &watcher);
    write(fds[1], "x", 1);
    ev_run(loop, 0);
    if (cb_count_local != 1)
      die_msg("local I/O watcher callback count !=1");
    ev_loop_destroy(loop);
  }

  /* baseline */
  {
    int fds2[2];
    if (pipe(fds2) != 0)
      die_msg("pipe() failed (baseline)");
    struct ev_loop* loop_b = ev_loop_new(0);
    cb_count_base = 0;
    ev_io watcher_b;
    ev_io_init(&watcher_b, io_cb_baseline, fds2[0], EV_READ);
    ev_io_start(loop_b, &watcher_b);
    write(fds2[1], "y", 1);
    ev_run(loop_b, 0);
    if (cb_count_base != 1)
      die_msg("baseline I/O watcher callback count !=1");
    ev_loop_destroy(loop_b);
    close(fds2[0]);
    close(fds2[1]);
  }

  close(fds[0]);
  close(fds[1]);

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
