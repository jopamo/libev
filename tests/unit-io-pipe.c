#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ev.h"

static int read_hits;
static int write_hits;

static void die_errno(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static void timeout_cb(EV_P_ ev_timer* w, int revents) {
  (void)revents;
  (void)w;
  fprintf(stderr, "Timeout reached in unit-io-pipe test\n");
  ev_break(EV_A_ EVBREAK_ALL);
}

static void read_cb(EV_P_ ev_io* w, int revents) {
  (void)revents;
  (void)w;
  ++read_hits;
}

static void write_cb(EV_P_ ev_io* w, int revents) {
  (void)revents;
  (void)w;
  ++write_hits;
  ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
  struct ev_loop* loop = ev_default_loop(EVFLAG_AUTO);
  if (!loop)
    die_errno("ev_default_loop");

  int pipe_fds[2];
  if (pipe(pipe_fds) < 0)
    die_errno("pipe");

  ev_io read_watcher;
  ev_io write_watcher;
  
  ev_io_init(&read_watcher, read_cb, pipe_fds[0], EV_READ);
  ev_io_init(&write_watcher, write_cb, pipe_fds[1], EV_WRITE);
  
  ev_io_start(loop, &read_watcher);
  ev_io_start(loop, &write_watcher);

  // Add a timeout watcher
  ev_timer timeout_watcher;
  ev_timer_init(&timeout_watcher, timeout_cb, 1.0, 0.);
  ev_timer_start(loop, &timeout_watcher);

  // Run the loop - it should break after processing the write event
  ev_run(loop, 0);
  
  ev_timer_stop(loop, &timeout_watcher);

  if (write_hits != 1)
    die_msg("write callback did not run exactly once");
  if (read_hits != 0)
    die_msg("read callback should not have run");

  // Cleanup
  close(pipe_fds[0]);
  close(pipe_fds[1]);
  ev_loop_destroy(loop);

  return EXIT_SUCCESS;
}
