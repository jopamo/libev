#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>

#include "ev.h"
#include "compat_common.h"

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static volatile int syserr_local;
static const char* last_msg_local;

static volatile int syserr_base;
static const char* last_msg_base;

static void cb_local(const char* msg) EV_NOEXCEPT {
  syserr_local++;
  last_msg_local = msg;
}

static void cb_base(const char* msg) EV_NOEXCEPT {
  syserr_base++;
  last_msg_base = msg;
}

static void provoke_syserr_local(void) {
  ev_set_syserr_cb(cb_local);

  struct rlimit old_lim;
  if (getrlimit(RLIMIT_NOFILE, &old_lim) != 0)
    die_msg("getrlimit failed (local)");

  struct rlimit tight = old_lim;
  tight.rlim_cur = 4; /* very low limit to force failure */
  if (setrlimit(RLIMIT_NOFILE, &tight) != 0)
    die_msg("setrlimit failed (local)");

  /* open some FDs near limit */
  int fds[4];
  for (int i = 0; i < 4; i++) {
    fds[i] = open("/dev/null", O_RDONLY);
    if (fds[i] < 0)
      break;
  }

  struct ev_loop* loop = ev_loop_new(0);
  if (!loop)
    die_msg("ev_loop_new failed (local)");

  /* Try creating many async watchers: each needs internal resources */
  for (int i = 0; i < 1000 && syserr_local == 0; i++) {
    ev_async* w = malloc(sizeof(ev_async));
    ev_async_init(w, (void (*)(EV_P_ ev_async*, int))cb_local);
    ev_async_start(loop, w);
  }

  ev_run(loop, EVRUN_NOWAIT);

  ev_loop_destroy(loop);

  /* restore limit */
  if (setrlimit(RLIMIT_NOFILE, &old_lim) != 0)
    die_msg("setrlimit restore failed (local)");

  if (syserr_local == 0)
    die_msg("local syserr callback never invoked");
}

static void provoke_syserr_baseline(void) {
  ev_set_syserr_cb(cb_base);

  struct rlimit old_lim;
  if (getrlimit(RLIMIT_NOFILE, &old_lim) != 0)
    die_msg("getrlimit failed (baseline)");

  struct rlimit tight = old_lim;
  tight.rlim_cur = 4;
  if (setrlimit(RLIMIT_NOFILE, &tight) != 0)
    die_msg("setrlimit failed (baseline)");

  int fds[4];
  for (int i = 0; i < 4; i++) {
    fds[i] = open("/dev/null", O_RDONLY);
    if (fds[i] < 0)
      break;
  }

  struct ev_loop* loop = ev_loop_new(0);
  if (!loop)
    die_msg("ev_loop_new failed (baseline)");

  for (int i = 0; i < 1000 && syserr_base == 0; i++) {
    ev_async* w = malloc(sizeof(ev_async));
    ev_async_init(w, (void (*)(EV_P_ ev_async*, int))cb_base);
    ev_async_start(loop, w);
  }

  ev_run(loop, EVRUN_NOWAIT);
  ev_loop_destroy(loop);

  if (setrlimit(RLIMIT_NOFILE, &old_lim) != 0)
    die_msg("setrlimit restore failed (baseline)");

  if (syserr_base == 0)
    die_msg("baseline syserr callback never invoked");
}

int main(void) {
  struct ev_compat_baseline base;
  ev_compat_load_baseline(&base);

  provoke_syserr_local();
  provoke_syserr_baseline();

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
