#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ev.h"

static int child_hits;
static pid_t observed_child_pid;

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
  fprintf(stderr, "Timeout reached in compat-child-watcher test\n");
  ev_break(EV_A_ EVBREAK_ALL);
}

static void child_cb(EV_P_ ev_child* w, int revents) {
  (void)revents;

  ++child_hits;
  observed_child_pid = w->rpid;

  if (!WIFEXITED(w->rstatus) || WEXITSTATUS(w->rstatus) != 42)
    die_msg("unexpected child exit status");

  ev_break(EV_A_ EVBREAK_ALL);
}

int main(void) {
  struct ev_loop* loop = ev_default_loop(EVFLAG_AUTO);
  if (!loop)
    die_errno("ev_default_loop");

  int sync_pipe[2];
  if (pipe(sync_pipe) < 0)
    die_errno("pipe");

  ev_child watcher;
  ev_child_init(&watcher, child_cb, 0 /* any child */, 0);
  ev_child_start(loop, &watcher);

  // Add a timeout watcher
  ev_timer timeout_watcher;
  ev_timer_init(&timeout_watcher, timeout_cb, 1.0, 0.);
  ev_timer_start(loop, &timeout_watcher);

  pid_t child = fork();
  if (child < 0)
    die_errno("fork");

  if (child == 0) {
    close(sync_pipe[1]);

    char token;
    if (read(sync_pipe[0], &token, 1) < 0)
      _exit(EXIT_FAILURE);
    close(sync_pipe[0]);

    _exit(42);
  }

  close(sync_pipe[0]);

  if (write(sync_pipe[1], "x", 1) != 1)
    die_errno("write");
  close(sync_pipe[1]);

  ev_run(loop, 0);
  
  ev_timer_stop(loop, &timeout_watcher);
  ev_child_stop(loop, &watcher);

  if (child_hits != 1)
    die_msg("child callback did not run exactly once");

  if (observed_child_pid != child)
    die_msg("child callback reported unexpected pid");

  ev_loop_destroy(loop);

  return EXIT_SUCCESS;
}
