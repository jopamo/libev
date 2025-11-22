/* Internal time helpers split from ev.c */

#ifndef EV_HAVE_EV_TIME
ev_tstamp ev_time(void) EV_NOEXCEPT {
#if EV_USE_REALTIME
  if (ecb_expect_true(have_realtime)) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return EV_TS_GET(ts);
  }
#endif

  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return EV_TV_GET(tv);
  }
}
#endif

inline_size ev_tstamp get_clock(void) EV_NOEXCEPT {
#if EV_USE_MONOTONIC
  if (ecb_expect_true(have_monotonic)) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return EV_TS_GET(ts);
  }
#endif

  return ev_time();
}

#if EV_MULTIPLICITY
ev_tstamp ev_now(EV_P) EV_NOEXCEPT {
  return ev_rt_now;
}
#endif

void ev_sleep(ev_tstamp delay) EV_NOEXCEPT {
  if (delay <= EV_TS_CONST(0.))
    return;

#if EV_USE_NANOSLEEP
  struct timespec ts;

  /* Modern Linux path: a single nanosleep
   * If a signal interrupts the sleep (EINTR), we just return early
   * This gives "sleep up to delay" semantics instead of "at least delay"
   */
  EV_TS_SET(ts, delay);
  (void)nanosleep(&ts, &ts);

#elif defined _WIN32
  /* maybe this should round up, as ms is very low resolution */
  /* compared to select (µs) or nanosleep (ns) */
  Sleep((unsigned long)(EV_TS_TO_MSEC(delay)));
#else
  struct timeval tv;

  /* Fallback path for platforms without nanosleep
   * On modern Linux this branch shouldn't be used (EV_USE_NANOSLEEP is enabled)
   * but we keep it for compatibility
   */
  EV_TV_SET(tv, delay);
  /* if select is interrupted by a signal (EINTR), we just return */
  (void)select(0, 0, 0, 0, &tv);
#endif
}
