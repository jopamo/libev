#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ev.h"

static int io_fired;
static int timer_fired;
static int pipe_fds[2] = {-1, -1};

static void die(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    die("fcntl");
}

static void io_cb(EV_P_ ev_io* w, int revents) {
  char buf[8];
  ssize_t n = read(pipe_fds[0], buf, sizeof buf);
  if (n <= 0)
    die("read");

  io_fired++;
  ev_break(EV_A_ EVBREAK_ALL);
}

static void timer_cb(EV_P_ ev_timer* w, int revents) {
  timer_fired++;
  if (write(pipe_fds[1], "x", 1) != 1)
    die("write");
}

int main(void) {
  if (pipe(pipe_fds) < 0)
    die("pipe");

  set_nonblock(pipe_fds[0]);
  set_nonblock(pipe_fds[1]);

  struct ev_loop* loop = ev_default_loop(EVFLAG_AUTO);
  if (!loop)
    die("ev_default_loop");

  ev_io io;
  ev_timer timer;

  ev_io_init(&io, io_cb, pipe_fds[0], EV_READ);
  ev_timer_init(&timer, timer_cb, 0.005, 0.0);

  ev_io_start(loop, &io);
  ev_timer_start(loop, &timer);

  ev_run(loop, 0);

  close(pipe_fds[0]);
  close(pipe_fds[1]);

  if (timer_fired != 1 || io_fired != 1)
    die("unexpected counters");

  return EXIT_SUCCESS;
}
