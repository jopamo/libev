#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ev.h"

static int timer1_fired;
static int timer2_fired;
static ev_tstamp t1_at;
static ev_tstamp t2_at;

static void die(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void timer1_cb(EV_P_ ev_timer* w, int revents) {
  timer1_fired++;
  t1_at = ev_now(EV_A);
}

static void timer2_cb(EV_P_ ev_timer* w, int revents) {
  timer2_fired++;
  t2_at = ev_now(EV_A);
  ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
  struct ev_loop* loop = ev_default_loop(EVFLAG_AUTO);
  if (!loop)
    die("ev_default_loop");

  ev_timer t1, t2;
  ev_timer_init(&t1, timer1_cb, 0.01, 0.0);
  ev_timer_init(&t2, timer2_cb, 0.02, 0.0);
  ev_timer_start(loop, &t1);
  ev_timer_start(loop, &t2);

  ev_run(loop, 0);

  if (timer1_fired != 1 || timer2_fired != 1)
    die("timer fired count");

  /* guard against large scheduling jitter; timers must be ordered */
  if (t2_at <= t1_at || (t2_at - t1_at) > 0.5) {
    fprintf(stderr, "unexpected timer timings: t1=%f t2=%f\n", t1_at, t2_at);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
