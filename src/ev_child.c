/* Child watcher utilities split from ev.c. */

#if EV_CHILD_ENABLE
static WL childs[EV_PID_HASHSIZE];

static ev_signal childev;

#ifndef WIFCONTINUED
#define WIFCONTINUED(status) 0
#endif

/* handle a single child status event */
inline_speed void child_reap(EV_P_ int chain, int pid, int status) EV_NOEXCEPT {
  ev_child* w;
  const int traced = WIFSTOPPED(status) || WIFCONTINUED(status);

  for (w = (ev_child*)childs[chain & ((EV_PID_HASHSIZE)-1)]; w; w = (ev_child*)((WL)w)->next) {
    if ((w->pid == pid || !w->pid) && (!traced || (w->flags & 1))) {
      ev_set_priority(w, EV_MAXPRI /* need to do it *now*, this *must* be the same prio as the signal watcher itself */
      );
      w->rpid = pid;
      w->rstatus = status;
      ev_feed_event(EV_A_(W) w, EV_CHILD);
    }
  }
}

#ifndef WCONTINUED
#define WCONTINUED 0
#endif

/* called on sigchld etc., calls waitpid */
static void childcb(EV_P_ ev_signal* sw, int revents) {
  int pid, status;

  (void)sw;
  (void)revents;

  for (;;) {
    /* some systems define WCONTINUED but then fail to support it (linux 2.4) */
    pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (pid <= 0) {
      if (!WCONTINUED || errno != EINVAL)
        break;

      pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
      if (pid <= 0)
        break;
    }

    child_reap(EV_A_ pid, pid, status);
    if ((EV_PID_HASHSIZE) > 1)
      child_reap(EV_A_ 0, pid, status); /* this might trigger a watcher twice, but feed_event catches that */
  }
}

#endif
