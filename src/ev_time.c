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

/* Internal time helpers split from ev.c. */

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

inline_size ev_tstamp get_clock(void) {
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
  if (delay > EV_TS_CONST(0.)) {
#if EV_USE_NANOSLEEP
    struct timespec ts;

    EV_TS_SET(ts, delay);
    nanosleep(&ts, 0);
#elif defined _WIN32
    /* maybe this should round up, as ms is very low resolution */
    /* compared to select (µs) or nanosleep (ns) */
    Sleep((unsigned long)(EV_TS_TO_MSEC(delay)));
#else
    struct timeval tv;

    /* here we rely on sys/time.h + sys/types.h + unistd.h providing select */
    /* something not guaranteed by newer posix versions, but guaranteed */
    /* by older ones */
    EV_TV_SET(tv, delay);
    select(0, 0, 0, 0, &tv);
#endif
  }
}
