#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "ev.h"
#include "compat_common.h"

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static volatile int cb_local;
static volatile int cb_base;

static void sig_cb_local(struct ev_loop* loop, ev_signal* w, int revents) {
  (void)loop;
  (void)revents;
  cb_local++;
  ev_signal_stop(loop, w);
  ev_break(loop, EVBREAK_ONE);
}

static void sig_cb_base(struct ev_loop* loop, ev_signal* w, int revents) {
  (void)loop;
  (void)revents;
  cb_base++;
  ev_signal_stop(loop, w);
  ev_break(loop, EVBREAK_ONE);
}

int main(void) {
  struct ev_compat_baseline base;
  ev_compat_load_baseline(&base);

  struct ev_loop* loop_local = ev_loop_new(0);
  cb_local = 0;
  ev_signal sig_local;
  ev_signal_init(&sig_local, sig_cb_local, SIGUSR1);
  ev_signal_start(loop_local, &sig_local);
  kill(getpid(), SIGUSR1);
  ev_run(loop_local, 0);
  if (cb_local != 1)
    die_msg("local signal watcher callback count !=1");
  ev_loop_destroy(loop_local);

  struct ev_loop* loop_base = ev_loop_new(0);
  cb_base = 0;
  ev_signal sig_base;
  ev_signal_init(&sig_base, sig_cb_base, SIGUSR1);
  ev_signal_start(loop_base, &sig_base);
  kill(getpid(), SIGUSR1);
  ev_run(loop_base, 0);
  if (cb_base != 1)
    die_msg("baseline signal watcher callback count !=1");
  ev_loop_destroy(loop_base);

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
