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

/* timerfd integration split from ev.c. */

#if EV_USE_TIMERFD

static void periodics_reschedule(EV_P);

static void timerfdcb(EV_P_ ev_io* iow, int revents) {
  struct itimerspec its = {0};

  (void)iow;
  (void)revents;

  its.it_value.tv_sec = ev_rt_now + (int)MAX_BLOCKTIME2;
  timerfd_settime(timerfd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &its, 0);

  ev_rt_now = ev_time();
  /* periodics_reschedule only needs ev_rt_now */
  /* but maybe in the future we want the full treatment. */
  /*
  now_floor = EV_TS_CONST (0.);
  time_update (EV_A_ EV_TSTAMP_HUGE);
  */
#if EV_PERIODIC_ENABLE
  periodics_reschedule(EV_A);
#endif
}

ecb_noinline ecb_cold static void evtimerfd_init(EV_P) {
  if (!ev_is_active(&timerfd_w)) {
    timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);

    if (timerfd >= 0) {
      fd_intern(timerfd); /* just to be sure */

      ev_io_init(&timerfd_w, timerfdcb, timerfd, EV_READ);
      ev_set_priority(&timerfd_w, EV_MINPRI);
      ev_io_start(EV_A_ & timerfd_w);
      ev_unref(EV_A); /* watcher should not keep loop alive */

      /* (re-) arm timer */
      timerfdcb(EV_A_ 0, 0);
    }
  }
}

#endif
