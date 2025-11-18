#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ev.h"

static int once_called;
static int once_revents;

static void die(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void once_cb(int revents, void* arg) {
  once_called++;
  once_revents = revents;

  /* ev_once does not expose the loop, so use the stored pointer */
  ev_break((struct ev_loop*)arg, EVBREAK_ALL);
}

int main(void) {
  struct ev_loop* loop = ev_default_loop(EVFLAG_AUTO);
  if (!loop)
    die("ev_default_loop");

  /* use timeout only; fd is unused */
  ev_once(loop, -1, 0, 0.01, once_cb, loop);
  ev_run(loop, 0);

  if (once_called != 1 || !(once_revents & EV_TIMEOUT))
    die("ev_once");

  return EXIT_SUCCESS;
}
