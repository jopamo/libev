/*
 * libev internal split file
 *
 * Copyright (c) 2007-2019 Marc Alexander Lehmann <libev@schmorp.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

/* Child watcher utilities split from ev.c. */

#if EV_CHILD_ENABLE
static WL childs[EV_PID_HASHSIZE];

static ev_signal childev;

#ifndef WIFCONTINUED
#define WIFCONTINUED(status) 0
#endif

/* handle a single child status event */
inline_speed void child_reap(EV_P_ int chain, int pid, int status) {
  ev_child* w;
  int traced = WIFSTOPPED(status) || WIFCONTINUED(status);

  for (w = (ev_child*)childs[chain & ((EV_PID_HASHSIZE)-1)]; w; w = (ev_child*)((WL)w)->next) {
    if ((w->pid == pid || !w->pid) && (!traced || (w->flags & 1))) {
      ev_set_priority(w,
                      EV_MAXPRI); /* need to do it *now*, this *must* be the same prio as the signal watcher itself */
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

  (void)revents;

  /* some systems define WCONTINUED but then fail to support it (linux 2.4) */
  if (0 >= (pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)))
    if (!WCONTINUED || errno != EINVAL || 0 >= (pid = waitpid(-1, &status, WNOHANG | WUNTRACED)))
      return;

  /* make sure we are called again until all children have been reaped */
  /* we need to do it this way so that the callback gets called before we continue */
  ev_feed_event(EV_A_(W) sw, EV_SIGNAL);

  child_reap(EV_A_ pid, pid, status);
  if ((EV_PID_HASHSIZE) > 1)
    child_reap(EV_A_ 0, pid, status); /* this might trigger a watcher twice, but feed_event catches that */
}

#endif
