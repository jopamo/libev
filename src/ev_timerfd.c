/* timerfd integration split from ev.c */

#if EV_USE_TIMERFD

static void periodics_reschedule(EV_P);

static void timerfdcb(EV_P_ ev_io *iow, int revents) {
  struct itimerspec its;
  ev_tstamp deadline;

  (void)iow;
  (void)revents;

  if (timerfd < 0)
    return;

  ev_rt_now = ev_time();

  memset(&its, 0, sizeof its);

  /* arm absolute timeout at ev_rt_now + MAX_BLOCKTIME2 */
  deadline = ev_rt_now + (ev_tstamp)(int)MAX_BLOCKTIME2;
  its.it_value.tv_sec = (time_t)deadline;
  its.it_value.tv_nsec =
    (long)((deadline - (ev_tstamp)its.it_value.tv_sec) * 1e9);

  timerfd_settime(timerfd,
                  TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET,
                  &its,
                  0);

  /* periodics_reschedule only needs ev_rt_now */
  /* but maybe in the future we want the full treatment */
  /*
  now_floor = EV_TS_CONST (0.);
  time_update (EV_A_ EV_TSTAMP_HUGE);
  */
#if EV_PERIODIC_ENABLE
  periodics_reschedule(EV_A);
#endif
}

ecb_noinline ecb_cold static void evtimerfd_init(EV_P) {
  if (ev_is_active(&timerfd_w) || timerfd >= 0)
    return;

  timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0)
    return;

  fd_intern(timerfd); /* just to be sure */

  ev_io_init(&timerfd_w, timerfdcb, timerfd, EV_READ);
  ev_set_priority(&timerfd_w, EV_MINPRI);
  ev_io_start(EV_A_ &timerfd_w);
  ev_unref(EV_A); /* watcher should not keep loop alive */

  /* (re-) arm timer */
  timerfdcb(EV_A_ &timerfd_w, EV_READ);
}

#endif
