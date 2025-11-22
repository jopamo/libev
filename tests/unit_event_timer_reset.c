#include <assert.h>
#include <event.h>
#include <ev.h>
#include <sys/time.h>

static struct event timer_event;
static struct event rearm_event;
static double readd_time;
static double fired_time;

static void rearm_cb(int fd, short what, void *arg) {
  (void)fd;
  (void)arg;

  assert(what & EV_TIMEOUT);

  readd_time = ev_time();

  struct timeval tv = {0, 100000}; /* 100ms */
  assert(event_add(&timer_event, &tv) == 0);
}

static void timer_cb(int fd, short what, void *arg) {
  (void)fd;

  assert(what & EV_TIMEOUT);

  fired_time = ev_time();
  event_base_loopexit((struct event_base *)arg, NULL);
}

int main(void) {
  struct event_base *base = event_init();
  assert(base);

  struct timeval tv_first = {0, 100000}; /* 100ms */
  struct timeval tv_rearm = {0, 50000};  /* 50ms */

  event_set(&timer_event, -1, EV_TIMEOUT, timer_cb, base);
  event_set(&rearm_event, -1, EV_TIMEOUT, rearm_cb, NULL);

  assert(event_add(&timer_event, &tv_first) == 0);
  assert(event_add(&rearm_event, &tv_rearm) == 0);

  event_base_dispatch(base);

  assert(readd_time > 0.);
  assert(fired_time > 0.);

  const double delta = fired_time - readd_time;
  assert(delta >= 0.07); /* expect ~100ms delay after re-add */

  event_del(&timer_event);
  event_del(&rearm_event);
  return 0;
}
