#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "ev.h"
#include "compat_common.h"

static void die_msg(const char* msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

static volatile int cb_local;
static volatile int cb_base;

static void child_cb_local(struct ev_loop* loop, ev_child* w, int revents) {
  (void)loop;
  (void)revents;
  cb_local++;
  ev_child_stop(loop, w);
  ev_break(loop, EVBREAK_ONE);
}

static void child_cb_base(struct ev_loop* loop, ev_child* w, int revents) {
  (void)loop;
  (void)revents;
  cb_base++;
  ev_child_stop(loop, w);
  ev_break(loop, EVBREAK_ONE);
}

int main(void) {
  struct ev_compat_baseline base;
  ev_compat_load_baseline(&base);

  pid_t pid = fork();
  if (pid < 0)
    die_msg("fork failed");
  if (pid == 0) {
    exit(EXIT_SUCCESS);
  }

  struct ev_loop* loop_local = ev_default_loop(0);
  cb_local = 0;
  ev_child child_local;
  ev_child_init(&child_local, child_cb_local, pid, 0);
  ev_child_start(loop_local, &child_local);
  ev_run(loop_local, 0);
  if (cb_local != 1)
    die_msg("local child watcher callback !=1");
  // Don't destroy the default loop

  pid = fork();
  if (pid < 0)
    die_msg("fork failed");
  if (pid == 0) {
    exit(EXIT_SUCCESS);
  }

  struct ev_loop* loop_base = ev_default_loop(0);
  cb_base = 0;
  ev_child child_base;
  ev_child_init(&child_base, child_cb_base, pid, 0);
  ev_child_start(loop_base, &child_base);
  ev_run(loop_base, 0);
  if (cb_base != 1)
    die_msg("baseline child watcher callback !=1");
  // Don't destroy the default loop

  ev_compat_unload_baseline(&base);
  return EXIT_SUCCESS;
}
