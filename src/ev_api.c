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

/* Public API and loop lifecycle split from ev.c. */

ecb_cold int ev_version_major(void) EV_NOEXCEPT {
  return EV_VERSION_MAJOR;
}

ecb_cold int ev_version_minor(void) EV_NOEXCEPT {
  return EV_VERSION_MINOR;
}

/* return true if we are running with elevated privileges and should ignore env variables */
inline_size ecb_cold int enable_secure(void) {
#ifdef _WIN32
  return 0;
#else
  return getuid() != geteuid() || getgid() != getegid();
#endif
}

ecb_cold unsigned int ev_supported_backends(void) EV_NOEXCEPT {
  unsigned int flags = 0;

  if (EV_USE_PORT)
    flags |= EVBACKEND_PORT;
  if (EV_USE_KQUEUE)
    flags |= EVBACKEND_KQUEUE;
  if (EV_USE_EPOLL)
    flags |= EVBACKEND_EPOLL;
  if (EV_USE_LINUXAIO)
    flags |= EVBACKEND_LINUXAIO;
  if (EV_USE_IOURING && ev_linux_version() >= 0x050601)
    flags |= EVBACKEND_IOURING; /* 5.6.1+ */
  if (EV_USE_POLL)
    flags |= EVBACKEND_POLL;
  if (EV_USE_SELECT)
    flags |= EVBACKEND_SELECT;

  return flags;
}

ecb_cold unsigned int ev_recommended_backends(void) EV_NOEXCEPT {
  unsigned int flags = ev_supported_backends();

#ifndef __NetBSD__
  /* kqueue is borked on everything but netbsd apparently */
  /* it usually doesn't work correctly on anything but sockets and pipes */
  flags &= ~EVBACKEND_KQUEUE;
#endif
#ifdef __APPLE__
  /* only select works correctly on that "unix-certified" platform */
  flags &= ~EVBACKEND_KQUEUE; /* horribly broken, even for sockets */
  flags &= ~EVBACKEND_POLL;   /* poll is based on kqueue from 10.5 onwards */
#endif
#ifdef __FreeBSD__
  flags &=
      ~EVBACKEND_POLL; /* poll return value is unusable (http://forums.freebsd.org/archive/index.php/t-10270.html) */
#endif

  /* TODO: linuxaio is very experimental */
#if !EV_RECOMMEND_LINUXAIO
  flags &= ~EVBACKEND_LINUXAIO;
#endif
  /* TODO: linuxaio is super experimental */
#if !EV_RECOMMEND_IOURING
  flags &= ~EVBACKEND_IOURING;
#endif

  return flags;
}

ecb_cold unsigned int ev_embeddable_backends(void) EV_NOEXCEPT {
  int flags = EVBACKEND_EPOLL | EVBACKEND_KQUEUE | EVBACKEND_PORT | EVBACKEND_IOURING;

  /* epoll embeddability broken on all linux versions up to at least 2.6.23 */
  if (ev_linux_version() < 0x020620) /* disable it on linux < 2.6.32 */
    flags &= ~EVBACKEND_EPOLL;

  /* EVBACKEND_LINUXAIO is theoretically embeddable, but suffers from a performance overhead */

  return flags;
}

unsigned int ev_backend(EV_P) EV_NOEXCEPT {
  return backend;
}

#if EV_FEATURE_API
unsigned int ev_iteration(EV_P) EV_NOEXCEPT {
  return loop_count;
}

unsigned int ev_depth(EV_P) EV_NOEXCEPT {
  return loop_depth;
}

void ev_set_io_collect_interval(EV_P_ ev_tstamp interval) EV_NOEXCEPT {
  io_blocktime = interval;
}

void ev_set_timeout_collect_interval(EV_P_ ev_tstamp interval) EV_NOEXCEPT {
  timeout_blocktime = interval;
}

void ev_set_userdata(EV_P_ void* data) EV_NOEXCEPT {
  userdata = data;
}

void* ev_userdata(EV_P) EV_NOEXCEPT {
  return userdata;
}

void ev_set_invoke_pending_cb(EV_P_ ev_loop_callback invoke_pending_cb) EV_NOEXCEPT {
  invoke_cb = invoke_pending_cb;
}

void ev_set_loop_release_cb(EV_P_ void (*release)(EV_P) EV_NOEXCEPT, void (*acquire)(EV_P) EV_NOEXCEPT) EV_NOEXCEPT {
  release_cb = release;
  acquire_cb = acquire;
}
#endif

/* initialise a loop structure, must be zero-initialised */
ecb_noinline ecb_cold static void loop_init(EV_P_ unsigned int flags) EV_NOEXCEPT {
  if (!backend) {
    origflags = flags;

#if EV_USE_REALTIME
    if (!have_realtime) {
      struct timespec ts;

      if (!clock_gettime(CLOCK_REALTIME, &ts))
        have_realtime = 1;
    }
#endif

#if EV_USE_MONOTONIC
    if (!have_monotonic) {
      struct timespec ts;

      if (!clock_gettime(CLOCK_MONOTONIC, &ts))
        have_monotonic = 1;
    }
#endif

    /* pid check not overridable via env */
#ifndef _WIN32
    if (flags & EVFLAG_FORKCHECK)
      curpid = getpid();
#endif

    if (!(flags & EVFLAG_NOENV) && !enable_secure() && getenv("LIBEV_FLAGS"))
      flags = atoi(getenv("LIBEV_FLAGS"));

    ev_rt_now = ev_time();
    mn_now = get_clock();
    now_floor = mn_now;
    rtmn_diff = ev_rt_now - mn_now;
#if EV_FEATURE_API
    invoke_cb = ev_invoke_pending;
#endif

    io_blocktime = 0.;
    timeout_blocktime = 0.;
    backend = 0;
    backend_fd = -1;
    sig_pending = 0;
#if EV_ASYNC_ENABLE
    async_pending = 0;
#endif
    pipe_write_skipped = 0;
    pipe_write_wanted = 0;
    evpipe[0] = -1;
    evpipe[1] = -1;
#if EV_USE_INOTIFY
    fs_fd = flags & EVFLAG_NOINOTIFY ? -1 : -2;
#endif
#if EV_USE_SIGNALFD
    sigfd = flags & EVFLAG_SIGNALFD ? -2 : -1;
#endif
#if EV_USE_TIMERFD
    timerfd = flags & EVFLAG_NOTIMERFD ? -1 : -2;
#endif

    if (!(flags & EVBACKEND_MASK))
      flags |= ev_recommended_backends();

#if EV_USE_IOCP
    if (!backend && (flags & EVBACKEND_IOCP))
      backend = iocp_init(EV_A_ flags);
#endif
#if EV_USE_PORT
    if (!backend && (flags & EVBACKEND_PORT))
      backend = port_init(EV_A_ flags);
#endif
#if EV_USE_KQUEUE
    if (!backend && (flags & EVBACKEND_KQUEUE))
      backend = kqueue_init(EV_A_ flags);
#endif
#if EV_USE_IOURING
    if (!backend && (flags & EVBACKEND_IOURING))
      backend = iouring_init(EV_A_ flags);
#endif
#if EV_USE_LINUXAIO
    if (!backend && (flags & EVBACKEND_LINUXAIO))
      backend = linuxaio_init(EV_A_ flags);
#endif
#if EV_USE_EPOLL
    if (!backend && (flags & EVBACKEND_EPOLL))
      backend = epoll_init(EV_A_ flags);
#endif
#if EV_USE_POLL
    if (!backend && (flags & EVBACKEND_POLL))
      backend = poll_init(EV_A_ flags);
#endif
#if EV_USE_SELECT
    if (!backend && (flags & EVBACKEND_SELECT))
      backend = select_init(EV_A_ flags);
#endif

    ev_prepare_init(&pending_w, pendingcb);

#if EV_SIGNAL_ENABLE || EV_ASYNC_ENABLE
    ev_init(&pipe_w, pipecb);
    ev_set_priority(&pipe_w, EV_MAXPRI);
#endif
  }
}

/* free up a loop structure */
ecb_cold void ev_loop_destroy(EV_P) {
  int i;

#if EV_MULTIPLICITY
  /* mimic free (0) */
  if (!EV_A)
    return;
#endif

#if EV_CLEANUP_ENABLE
  /* queue cleanup watchers (and execute them) */
  if (ecb_expect_false(cleanupcnt)) {
    queue_events(EV_A_(W*) cleanups, cleanupcnt, EV_CLEANUP);
    EV_INVOKE_PENDING;
  }
#endif

#if EV_CHILD_ENABLE
  if (ev_is_default_loop(EV_A) && ev_is_active(&childev)) {
    ev_ref(EV_A); /* child watcher */
    ev_signal_stop(EV_A_ & childev);
  }
#endif

  if (ev_is_active(&pipe_w)) {
    /*ev_ref (EV_A);*/
    /*ev_io_stop (EV_A_ &pipe_w);*/

    if (evpipe[0] >= 0)
      EV_WIN32_CLOSE_FD(evpipe[0]);
    if (evpipe[1] >= 0)
      EV_WIN32_CLOSE_FD(evpipe[1]);
  }

#if EV_USE_SIGNALFD
  if (ev_is_active(&sigfd_w))
    close(sigfd);
#endif

#if EV_USE_TIMERFD
  if (ev_is_active(&timerfd_w))
    close(timerfd);
#endif

#if EV_USE_INOTIFY
  if (fs_fd >= 0)
    close(fs_fd);
#endif

  if (backend_fd >= 0)
    close(backend_fd);

#if EV_USE_IOCP
  if (backend == EVBACKEND_IOCP)
    iocp_destroy(EV_A);
#endif
#if EV_USE_PORT
  if (backend == EVBACKEND_PORT)
    port_destroy(EV_A);
#endif
#if EV_USE_KQUEUE
  if (backend == EVBACKEND_KQUEUE)
    kqueue_destroy(EV_A);
#endif
#if EV_USE_IOURING
  if (backend == EVBACKEND_IOURING)
    iouring_destroy(EV_A);
#endif
#if EV_USE_LINUXAIO
  if (backend == EVBACKEND_LINUXAIO)
    linuxaio_destroy(EV_A);
#endif
#if EV_USE_EPOLL
  if (backend == EVBACKEND_EPOLL)
    epoll_destroy(EV_A);
#endif
#if EV_USE_POLL
  if (backend == EVBACKEND_POLL)
    poll_destroy(EV_A);
#endif
#if EV_USE_SELECT
  if (backend == EVBACKEND_SELECT)
    select_destroy(EV_A);
#endif

  for (i = NUMPRI; i--;) {
    array_free(pending, [i]);
#if EV_IDLE_ENABLE
    array_free(idle, [i]);
#endif
  }

  ev_free(anfds);
  anfds = 0;
  anfdmax = 0;

  /* have to use the microsoft-never-gets-it-right macro */
  array_free(rfeed, EMPTY);
  array_free(fdchange, EMPTY);
  array_free(timer, EMPTY);
#if EV_PERIODIC_ENABLE
  array_free(periodic, EMPTY);
#endif
#if EV_FORK_ENABLE
  array_free(fork, EMPTY);
#endif
#if EV_CLEANUP_ENABLE
  array_free(cleanup, EMPTY);
#endif
  array_free(prepare, EMPTY);
  array_free(check, EMPTY);
#if EV_ASYNC_ENABLE
  array_free(async, EMPTY);
#endif

  backend = 0;

#if EV_MULTIPLICITY
  if (ev_is_default_loop(EV_A))
#endif
    ev_default_loop_ptr = 0;
#if EV_MULTIPLICITY
  else
    ev_free(EV_A);
#endif
}

#if EV_USE_INOTIFY
inline_size void infy_fork(EV_P);
#endif

inline_size void loop_fork(EV_P) {
#if EV_USE_PORT
  if (backend == EVBACKEND_PORT)
    port_fork(EV_A);
#endif
#if EV_USE_KQUEUE
  if (backend == EVBACKEND_KQUEUE)
    kqueue_fork(EV_A);
#endif
#if EV_USE_IOURING
  if (backend == EVBACKEND_IOURING)
    iouring_fork(EV_A);
#endif
#if EV_USE_LINUXAIO
  if (backend == EVBACKEND_LINUXAIO)
    linuxaio_fork(EV_A);
#endif
#if EV_USE_EPOLL
  if (backend == EVBACKEND_EPOLL)
    epoll_fork(EV_A);
#endif
#if EV_USE_INOTIFY
  infy_fork(EV_A);
#endif

  if (postfork != 2) {
#if EV_USE_SIGNALFD
    /* surprisingly, nothing needs to be done for signalfd, accoridng to docs, it does the right thing on fork */
#endif

#if EV_USE_TIMERFD
    if (ev_is_active(&timerfd_w)) {
      ev_ref(EV_A);
      ev_io_stop(EV_A_ & timerfd_w);

      close(timerfd);
      timerfd = -2;

      evtimerfd_init(EV_A);
      /* reschedule periodics, in case we missed something */
      ev_feed_event(EV_A_ & timerfd_w, EV_CUSTOM);
    }
#endif

#if EV_SIGNAL_ENABLE || EV_ASYNC_ENABLE
    if (ev_is_active(&pipe_w)) {
      /* pipe_write_wanted must be false now, so modifying fd vars should be safe */

      ev_ref(EV_A);
      ev_io_stop(EV_A_ & pipe_w);

      if (evpipe[0] >= 0)
        EV_WIN32_CLOSE_FD(evpipe[0]);

      evpipe_init(EV_A);
      /* iterate over everything, in case we missed something before */
      ev_feed_event(EV_A_ & pipe_w, EV_CUSTOM);
    }
#endif
  }

  postfork = 0;
}

#if EV_MULTIPLICITY

ecb_cold struct ev_loop* ev_loop_new(unsigned int flags) EV_NOEXCEPT {
  EV_P = (struct ev_loop*)ev_malloc(sizeof(struct ev_loop));

  memset(EV_A, 0, sizeof(struct ev_loop));
  loop_init(EV_A_ flags);

  if (ev_backend(EV_A))
    return EV_A;

  ev_free(EV_A);
  return 0;
}

#endif /* multiplicity */

#if EV_VERIFY
ecb_noinline ecb_cold static void verify_watcher(EV_P_ W w) {
  EV_ASSERT_MSG("libev: watcher has invalid priority", ABSPRI(w) >= 0 && ABSPRI(w) < NUMPRI);

  if (w->pending)
    EV_ASSERT_MSG("libev: pending watcher not on pending queue", pendings[ABSPRI(w)][w->pending - 1].w == w);
}

ecb_noinline ecb_cold static void verify_heap(EV_P_ ANHE* heap, int N) {
  int i;

  for (i = HEAP0; i < N + HEAP0; ++i) {
    EV_ASSERT_MSG("libev: active index mismatch in heap", ev_active(ANHE_w(heap[i])) == i);
    EV_ASSERT_MSG("libev: heap condition violated", i == HEAP0 || ANHE_at(heap[HPARENT(i)]) <= ANHE_at(heap[i]));
    EV_ASSERT_MSG("libev: heap at cache mismatch", ANHE_at(heap[i]) == ev_at(ANHE_w(heap[i])));

    verify_watcher(EV_A_(W) ANHE_w(heap[i]));
  }
}

ecb_noinline ecb_cold static void array_verify(EV_P_ W* ws, int cnt) {
  while (cnt--) {
    EV_ASSERT_MSG("libev: active index mismatch", ev_active(ws[cnt]) == cnt + 1);
    verify_watcher(EV_A_ ws[cnt]);
  }
}
#endif

#if EV_FEATURE_API
void ecb_cold ev_verify(EV_P) EV_NOEXCEPT {
#if EV_VERIFY
  int i;
  WL w, w2;

  assert(activecnt >= -1);

  assert(fdchangemax >= fdchangecnt);
  for (i = 0; i < fdchangecnt; ++i)
    EV_ASSERT_MSG("libev: negative fd in fdchanges", fdchanges[i] >= 0);

  assert(anfdmax >= 0);
  for (i = 0; i < anfdmax; ++i) {
    int j = 0;

    for (w = w2 = anfds[i].head; w; w = w->next) {
      verify_watcher(EV_A_(W) w);

      if (j++ & 1) {
        EV_ASSERT_MSG("libev: io watcher list contains a loop", w != w2);
        w2 = w2->next;
      }

      EV_ASSERT_MSG("libev: inactive fd watcher on anfd list", ev_active(w) == 1);
      EV_ASSERT_MSG("libev: fd mismatch between watcher and anfd", ((ev_io*)w)->fd == i);
    }
  }

  assert(timermax >= timercnt);
  verify_heap(EV_A_ timers, timercnt);

#if EV_PERIODIC_ENABLE
  assert(periodicmax >= periodiccnt);
  verify_heap(EV_A_ periodics, periodiccnt);
#endif

  for (i = NUMPRI; i--;) {
    assert(pendingmax[i] >= pendingcnt[i]);
#if EV_IDLE_ENABLE
    assert(idleall >= 0);
    assert(idlemax[i] >= idlecnt[i]);
    array_verify(EV_A_(W*) idles[i], idlecnt[i]);
#endif
  }

#if EV_FORK_ENABLE
  assert(forkmax >= forkcnt);
  array_verify(EV_A_(W*) forks, forkcnt);
#endif

#if EV_CLEANUP_ENABLE
  assert(cleanupmax >= cleanupcnt);
  array_verify(EV_A_(W*) cleanups, cleanupcnt);
#endif

#if EV_ASYNC_ENABLE
  assert(asyncmax >= asynccnt);
  array_verify(EV_A_(W*) asyncs, asynccnt);
#endif

#if EV_PREPARE_ENABLE
  assert(preparemax >= preparecnt);
  array_verify(EV_A_(W*) prepares, preparecnt);
#endif

#if EV_CHECK_ENABLE
  assert(checkmax >= checkcnt);
  array_verify(EV_A_(W*) checks, checkcnt);
#endif

#if 0
#if EV_CHILD_ENABLE
  for (w = (ev_child *)childs [chain & ((EV_PID_HASHSIZE) - 1)]; w; w = (ev_child *)((WL)w)->next)
  for (signum = EV_NSIG; signum--; ) if (signals [signum].pending)
#endif
#endif
#endif
}
#endif

#if EV_MULTIPLICITY
ecb_cold struct ev_loop*
#else
int
#endif
ev_default_loop(unsigned int flags) EV_NOEXCEPT {
  if (!ev_default_loop_ptr) {
#if EV_MULTIPLICITY
    EV_P = ev_default_loop_ptr = &default_loop_struct;
#else
    ev_default_loop_ptr = 1;
#endif

    loop_init(EV_A_ flags);

    if (ev_backend(EV_A)) {
#if EV_CHILD_ENABLE
      ev_signal_init(&childev, childcb, SIGCHLD);
      ev_set_priority(&childev, EV_MAXPRI);
      ev_signal_start(EV_A_ & childev);
      ev_unref(EV_A); /* child watcher should not keep loop alive */
#endif
    }
    else
      ev_default_loop_ptr = 0;
  }

  return ev_default_loop_ptr;
}

void ev_loop_fork(EV_P) EV_NOEXCEPT {
  postfork = 1;
}

/*****************************************************************************/

void ev_invoke(EV_P_ void* w, int revents) {
  EV_CB_INVOKE((W)w, revents);
}

unsigned int ev_pending_count(EV_P) EV_NOEXCEPT {
  int pri;
  unsigned int count = 0;

  for (pri = NUMPRI; pri--;)
    count += pendingcnt[pri];

  return count;
}

ecb_noinline void ev_invoke_pending(EV_P) {
  pendingpri = NUMPRI;

  do {
    --pendingpri;

    /* pendingpri possibly gets modified in the inner loop */
    while (pendingcnt[pendingpri]) {
      ANPENDING* p = pendings[pendingpri] + --pendingcnt[pendingpri];

      p->w->pending = 0;
      EV_CB_INVOKE(p->w, p->events);
      EV_FREQUENT_CHECK;
    }
  } while (pendingpri);
}

#if EV_IDLE_ENABLE
/* make idle watchers pending. this handles the "call-idle */
/* only when higher priorities are idle" logic */
inline_size void idle_reify(EV_P) {
  if (ecb_expect_false(idleall)) {
    int pri;

    for (pri = NUMPRI; pri--;) {
      if (pendingcnt[pri])
        break;

      if (idlecnt[pri]) {
        queue_events(EV_A_(W*) idles[pri], idlecnt[pri], EV_IDLE);
        break;
      }
    }
  }
}
#endif

/* make timers pending */
inline_size void timers_reify(EV_P) {
  EV_FREQUENT_CHECK;

  if (timercnt && ANHE_at(timers[HEAP0]) < mn_now) {
    do {
      ev_timer* w = (ev_timer*)ANHE_w(timers[HEAP0]);

      /*assert (("libev: inactive timer on timer heap detected", ev_is_active (w)));*/

      /* first reschedule or stop timer */
      if (w->repeat) {
        ev_at(w) += w->repeat;
        if (ev_at(w) < mn_now)
          ev_at(w) = mn_now;

        EV_ASSERT_MSG("libev: negative ev_timer repeat value found while processing timers",
                      w->repeat > EV_TS_CONST(0.));

        ANHE_at_cache(timers[HEAP0]);
        downheap(timers, timercnt, HEAP0);
      }
      else
        ev_timer_stop(EV_A_ w); /* nonrepeating: stop timer */

      EV_FREQUENT_CHECK;
      feed_reverse(EV_A_(W) w);
    } while (timercnt && ANHE_at(timers[HEAP0]) < mn_now);

    feed_reverse_done(EV_A_ EV_TIMER);
  }
}

#if EV_PERIODIC_ENABLE

ecb_noinline static void periodic_recalc(EV_P_ ev_periodic* w) {
  ev_tstamp interval = w->interval > MIN_INTERVAL ? w->interval : MIN_INTERVAL;
  ev_tstamp at = w->offset + interval * ev_floor((ev_rt_now - w->offset) / interval);

  /* the above almost always errs on the low side */
  while (at <= ev_rt_now) {
    ev_tstamp nat = at + w->interval;

    /* when resolution fails us, we use ev_rt_now */
    if (ecb_expect_false(nat == at)) {
      at = ev_rt_now;
      break;
    }

    at = nat;
  }

  ev_at(w) = at;
}

/* make periodics pending */
inline_size void periodics_reify(EV_P) {
  EV_FREQUENT_CHECK;

  while (periodiccnt && ANHE_at(periodics[HEAP0]) < ev_rt_now) {
    do {
      ev_periodic* w = (ev_periodic*)ANHE_w(periodics[HEAP0]);

      /*assert (("libev: inactive timer on periodic heap detected", ev_is_active (w)));*/

      /* first reschedule or stop timer */
      if (w->reschedule_cb) {
        ev_at(w) = w->reschedule_cb(w, ev_rt_now);

        EV_ASSERT_MSG("libev: ev_periodic reschedule callback returned time in the past", ev_at(w) >= ev_rt_now);

        ANHE_at_cache(periodics[HEAP0]);
        downheap(periodics, periodiccnt, HEAP0);
      }
      else if (w->interval) {
        periodic_recalc(EV_A_ w);
        ANHE_at_cache(periodics[HEAP0]);
        downheap(periodics, periodiccnt, HEAP0);
      }
      else
        ev_periodic_stop(EV_A_ w); /* nonrepeating: stop timer */

      EV_FREQUENT_CHECK;
      feed_reverse(EV_A_(W) w);
    } while (periodiccnt && ANHE_at(periodics[HEAP0]) < ev_rt_now);

    feed_reverse_done(EV_A_ EV_PERIODIC);
  }
}

/* recalculate all periodics after a time jump, but keep overdue ones pending */
ecb_noinline ecb_cold static void periodics_reschedule(EV_P) {
  int i;
  int overdue = 0;

  /* adjust periodics after time jump */
  for (i = HEAP0; i < periodiccnt + HEAP0; ++i) {
    ev_periodic* w = (ev_periodic*)ANHE_w(periodics[i]);
    ev_tstamp at = ANHE_at(periodics[i]);

    /* let already overdue periodics fire once immediately after a jump */
    if (at < ev_rt_now) {
      overdue = 1;
      ANHE_at_cache(periodics[i]);
      continue;
    }

    if (w->reschedule_cb)
      ev_at(w) = w->reschedule_cb(w, ev_rt_now);
    else if (w->interval)
      periodic_recalc(EV_A_ w);

  }

  if (periodiccnt && !overdue) {
    ANHE* he = periodics + HEAP0;
    ev_periodic* w = (ev_periodic*)ANHE_w(*he);

    ev_at(w) = ev_rt_now;
    ANHE_at_cache(*he);
  }

  reheap(periodics, periodiccnt);
}
#endif

/* adjust all timers by a given offset */
ecb_noinline ecb_cold static void timers_reschedule(EV_P_ ev_tstamp adjust) {
  int i;

  for (i = 0; i < timercnt; ++i) {
    ANHE* he = timers + i + HEAP0;
    ANHE_w(*he)->at += adjust;
    ANHE_at_cache(*he);
  }
}

/* fetch new monotonic and realtime times from the kernel */
/* also detect if there was a timejump, and act accordingly */
inline_speed void time_update(EV_P_ ev_tstamp max_block) {
#if EV_USE_MONOTONIC
  if (ecb_expect_true(have_monotonic)) {
    int i;
    ev_tstamp odiff = rtmn_diff;

    mn_now = get_clock();

    /* only fetch the realtime clock every 0.5*MIN_TIMEJUMP seconds */
    /* interpolate in the meantime */
    if (ecb_expect_true(mn_now - now_floor < EV_TS_CONST(MIN_TIMEJUMP * .5))) {
      ev_rt_now = rtmn_diff + mn_now;
      return;
    }

    now_floor = mn_now;
    ev_rt_now = ev_time();

    /* loop a few times, before making important decisions.
     * on the choice of "4": one iteration isn't enough,
     * in case we get preempted during the calls to
     * ev_time and get_clock. a second call is almost guaranteed
     * to succeed in that case, though. and looping a few more times
     * doesn't hurt either as we only do this on time-jumps or
     * in the unlikely event of having been preempted here.
     */
    for (i = 4; --i;) {
      ev_tstamp diff;
      rtmn_diff = ev_rt_now - mn_now;

      diff = odiff - rtmn_diff;

      if (ecb_expect_true((diff < EV_TS_CONST(0.) ? -diff : diff) < EV_TS_CONST(MIN_TIMEJUMP)))
        return; /* all is well */

      ev_rt_now = ev_time();
      mn_now = get_clock();
      now_floor = mn_now;
    }

    /* no timer adjustment, as the monotonic clock doesn't jump */
    /* timers_reschedule (EV_A_ rtmn_diff - odiff) */
#if EV_PERIODIC_ENABLE
    periodics_reschedule(EV_A);
#endif
  }
  else
#endif
  {
    ev_rt_now = ev_time();

    if (ecb_expect_false(mn_now > ev_rt_now || ev_rt_now > mn_now + max_block + EV_TS_CONST(MIN_TIMEJUMP))) {
      /* adjust timers. this is easy, as the offset is the same for all of them */
      timers_reschedule(EV_A_ ev_rt_now - mn_now);
#if EV_PERIODIC_ENABLE
      periodics_reschedule(EV_A);
#endif
    }

    mn_now = ev_rt_now;
  }
}

int ev_run(EV_P_ int flags) {
#if EV_FEATURE_API
  ++loop_depth;
#endif

  EV_ASSERT_MSG("libev: ev_loop recursion during release detected", loop_done != EVBREAK_RECURSE);

  loop_done = EVBREAK_CANCEL;

  EV_INVOKE_PENDING; /* in case we recurse, ensure ordering stays nice and clean */

  do {
#if EV_VERIFY >= 2
    ev_verify(EV_A);
#endif

#ifndef _WIN32
    if (ecb_expect_false(curpid)) /* penalise the forking check even more */
      if (ecb_expect_false(getpid() != curpid)) {
        curpid = getpid();
        postfork = 1;
      }
#endif

#if EV_FORK_ENABLE
    /* we might have forked, so queue fork handlers */
    if (ecb_expect_false(postfork))
      if (forkcnt) {
        queue_events(EV_A_(W*) forks, forkcnt, EV_FORK);
        EV_INVOKE_PENDING;
      }
#endif

#if EV_PREPARE_ENABLE
    /* queue prepare watchers (and execute them) */
    if (ecb_expect_false(preparecnt)) {
      queue_events(EV_A_(W*) prepares, preparecnt, EV_PREPARE);
      EV_INVOKE_PENDING;
    }
#endif

    if (ecb_expect_false(loop_done))
      break;

    /* we might have forked, so reify kernel state if necessary */
    if (ecb_expect_false(postfork))
      loop_fork(EV_A);

    /* update fd-related kernel structures */
    fd_reify(EV_A);

#if EV_IDLE_ENABLE
    /* fast-path idle-only loops: skip kernel polling when nothing else is active */
    if (ecb_expect_false(idleall && !activeio && !fdchangecnt && !timercnt
#if EV_PERIODIC_ENABLE
                         && !periodiccnt
#endif
                         )) {
      time_update(EV_A_ EV_TS_CONST(EV_TSTAMP_HUGE));

#if EV_FEATURE_API
      ++loop_count;
#endif

      idle_reify(EV_A);

#if EV_CHECK_ENABLE
      if (ecb_expect_false(checkcnt))
        queue_events(EV_A_(W*) checks, checkcnt, EV_CHECK);
#endif

      EV_INVOKE_PENDING;
      continue;
    }
#endif

    /* calculate blocking time */
    {
      ev_tstamp waittime = 0.;
      ev_tstamp sleeptime = 0.;

      /* remember old timestamp for io_blocktime calculation */
      ev_tstamp prev_mn_now = mn_now;

      /* update time to cancel out callback processing overhead */
      time_update(EV_A_ EV_TS_CONST(EV_TSTAMP_HUGE));

      /* from now on, we want a pipe-wake-up */
      pipe_write_wanted = 1;

      ECB_MEMORY_FENCE; /* make sure pipe_write_wanted is visible before we check for potential skips */

      if (ecb_expect_true(!(flags & EVRUN_NOWAIT || idleall || !activecnt || pipe_write_skipped))) {
        waittime = EV_TS_CONST(MAX_BLOCKTIME);

#if EV_USE_TIMERFD
        /* sleep a lot longer when we can reliably detect timejumps */
        if (ecb_expect_true(timerfd >= 0))
          waittime = EV_TS_CONST(MAX_BLOCKTIME2);
#endif
#if !EV_PERIODIC_ENABLE
        /* without periodics but with monotonic clock there is no need */
        /* for any time jump detection, so sleep longer */
        if (ecb_expect_true(have_monotonic))
          waittime = EV_TS_CONST(MAX_BLOCKTIME2);
#endif

        if (timercnt) {
          ev_tstamp to = ANHE_at(timers[HEAP0]) - mn_now;
          if (waittime > to)
            waittime = to;
        }

#if EV_PERIODIC_ENABLE
        if (periodiccnt) {
          ev_tstamp to = ANHE_at(periodics[HEAP0]) - ev_rt_now;
          if (waittime > to)
            waittime = to;
        }
#endif

        /* don't let timeouts decrease the waittime below timeout_blocktime */
        if (ecb_expect_false(waittime < timeout_blocktime))
          waittime = timeout_blocktime;

        /* now there are two more special cases left, either we have
         * already-expired timers, so we should not sleep, or we have timers
         * that expire very soon, in which case we need to wait for a minimum
         * amount of time for some event loop backends.
         */
        if (ecb_expect_false(waittime < backend_mintime))
          waittime = waittime <= EV_TS_CONST(0.) ? EV_TS_CONST(0.) : backend_mintime;

        /* extra check because io_blocktime is commonly 0 */
        if (ecb_expect_false(io_blocktime)) {
          sleeptime = io_blocktime - (mn_now - prev_mn_now);

          if (sleeptime > waittime - backend_mintime)
            sleeptime = waittime - backend_mintime;

          if (ecb_expect_true(sleeptime > EV_TS_CONST(0.))) {
            ev_sleep(sleeptime);
            waittime -= sleeptime;
          }
        }
      }

#if EV_FEATURE_API
      ++loop_count;
#endif
      assert((loop_done = EVBREAK_RECURSE, 1)); /* assert for side effect */
      backend_poll(EV_A_ waittime);
      assert((loop_done = EVBREAK_CANCEL, 1)); /* assert for side effect */

      pipe_write_wanted = 0; /* just an optimisation, no fence needed */

      ECB_MEMORY_FENCE_ACQUIRE;
      if (pipe_write_skipped) {
        EV_ASSERT_MSG("libev: pipe_w not active, but pipe not written", ev_is_active(&pipe_w));
        ev_feed_event(EV_A_ & pipe_w, EV_CUSTOM);
      }

      /* update ev_rt_now, do magic */
      time_update(EV_A_ waittime + sleeptime);
    }

    /* queue pending timers and reschedule them */
    timers_reify(EV_A); /* relative timers called last */
#if EV_PERIODIC_ENABLE
    periodics_reify(EV_A); /* absolute timers called first */
#endif

#if EV_IDLE_ENABLE
    /* queue idle watchers unless other events are pending */
    idle_reify(EV_A);
#endif

#if EV_CHECK_ENABLE
    /* queue check watchers, to be executed first */
    if (ecb_expect_false(checkcnt))
      queue_events(EV_A_(W*) checks, checkcnt, EV_CHECK);
#endif

    EV_INVOKE_PENDING;
  } while (ecb_expect_true(activecnt && !loop_done && !(flags & (EVRUN_ONCE | EVRUN_NOWAIT))));

  if (loop_done == EVBREAK_ONE)
    loop_done = EVBREAK_CANCEL;

#if EV_FEATURE_API
  --loop_depth;
#endif

  return activecnt;
}

void ev_break(EV_P_ int how) EV_NOEXCEPT {
  loop_done = how;
}

void ev_ref(EV_P) EV_NOEXCEPT {
  ++activecnt;
}

void ev_unref(EV_P) EV_NOEXCEPT {
  --activecnt;
}

void ev_now_update(EV_P) EV_NOEXCEPT {
  time_update(EV_A_ EV_TSTAMP_HUGE);
}

void ev_suspend(EV_P) EV_NOEXCEPT {
  ev_now_update(EV_A);
}

void ev_resume(EV_P) EV_NOEXCEPT {
  ev_tstamp mn_prev = mn_now;

  ev_now_update(EV_A);
  timers_reschedule(EV_A_ mn_now - mn_prev);
#if EV_PERIODIC_ENABLE
  /* TODO: really do this? */
  periodics_reschedule(EV_A);
#endif
}

/*****************************************************************************/
/* singly-linked list management, used when the expected list length is short */

inline_size void wlist_add(WL* head, WL elem) {
  elem->next = *head;
  *head = elem;
}

inline_size void wlist_del(WL* head, WL elem) {
  while (*head) {
    if (ecb_expect_true(*head == elem)) {
      *head = elem->next;
      break;
    }

    head = &(*head)->next;
  }
}

/* internal, faster, version of ev_clear_pending */
inline_speed void clear_pending(EV_P_ W w) {
  if (w->pending) {
    pendings[ABSPRI(w)][w->pending - 1].w = (W)&pending_w;
    w->pending = 0;
  }
}

int ev_clear_pending(EV_P_ void* w) EV_NOEXCEPT {
  W w_ = (W)w;
  int pending = w_->pending;

  if (ecb_expect_true(pending)) {
    ANPENDING* p = pendings[ABSPRI(w_)] + pending - 1;
    p->w = (W)&pending_w;
    w_->pending = 0;
    return p->events;
  }
  else
    return 0;
}

inline_size void pri_adjust(EV_P_ W w) {
  int pri = ev_priority(w);

  (void)loop;

  pri = pri < EV_MINPRI ? EV_MINPRI : pri;
  pri = pri > EV_MAXPRI ? EV_MAXPRI : pri;
  ev_set_priority(w, pri);
}

inline_speed void ev_start(EV_P_ W w, int active) {
  pri_adjust(EV_A_ w);
  w->active = active;
  ev_ref(EV_A);
}

inline_size void ev_stop(EV_P_ W w) {
  ev_unref(EV_A);
  w->active = 0;
}

/*****************************************************************************/

ecb_noinline void ev_io_start(EV_P_ ev_io* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  EV_ASSERT_MSG("libev: ev_io_start called with illegal event mask", !(w->events & ~(EV_READ | EV_WRITE)));

  int needs_fdset = w->fd & EV__IOFDSET;
  int fd = ev_io_fd(w);

  EV_ASSERT_MSG("libev: ev_io_start called with negative fd", fd >= 0);

#if EV_VERIFY >= 2
  EV_ASSERT_MSG("libev: ev_io_start called on watcher with invalid fd", fd_valid(fd));
#endif
  EV_FREQUENT_CHECK;

  w->fd = fd;

  ev_start(EV_A_(W) w, 1);
  ++activeio;
  array_needsize(ANFD, anfds, anfdmax, fd + 1, array_needsize_zerofill);
  wlist_add(&anfds[fd].head, (WL)w);

  /* common bug, apparently */
  EV_ASSERT_MSG("libev: ev_io_start called with corrupted watcher", ((WL)w)->next != (WL)w);

  fd_change(EV_A_ fd, needs_fdset | EV_ANFD_REIFY);

  EV_FREQUENT_CHECK;
}

ecb_noinline void ev_io_stop(EV_P_ ev_io* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_ASSERT_MSG("libev: ev_io_stop called with illegal fd (must stay constant after start!)",
                w->fd >= 0 && w->fd < anfdmax);

#if EV_VERIFY >= 2
  EV_ASSERT_MSG("libev: ev_io_stop called on watcher with invalid fd", fd_valid(w->fd));
#endif
  EV_FREQUENT_CHECK;

  wlist_del(&anfds[w->fd].head, (WL)w);
  --activeio;
  ev_stop(EV_A_(W) w);

  fd_change(EV_A_ w->fd, EV_ANFD_REIFY);

  EV_FREQUENT_CHECK;
}

ecb_noinline void ev_timer_start(EV_P_ ev_timer* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  ev_at(w) += mn_now;

  EV_ASSERT_MSG("libev: ev_timer_start called with negative timer repeat value", w->repeat >= 0.);

  EV_FREQUENT_CHECK;

  ++timercnt;
  ev_start(EV_A_(W) w, timercnt + HEAP0 - 1);
  array_needsize(ANHE, timers, timermax, ev_active(w) + 1, array_needsize_noinit);
  ANHE_w(timers[ev_active(w)]) = (WT)w;
  ANHE_at_cache(timers[ev_active(w)]);
  upheap(timers, ev_active(w));

  EV_FREQUENT_CHECK;

  /*assert (("libev: internal timer heap corruption", timers [ev_active (w)] == (WT)w));*/
}

ecb_noinline void ev_timer_stop(EV_P_ ev_timer* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  {
    int active = ev_active(w);

    EV_ASSERT_MSG("libev: internal timer heap corruption", ANHE_w(timers[active]) == (WT)w);

    --timercnt;

    if (ecb_expect_true(active < timercnt + HEAP0)) {
      timers[active] = timers[timercnt + HEAP0];
      adjustheap(timers, timercnt, active);
    }
  }

  ev_at(w) -= mn_now;

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}

ecb_noinline void ev_timer_again(EV_P_ ev_timer* w) EV_NOEXCEPT {
  EV_FREQUENT_CHECK;

  clear_pending(EV_A_(W) w);

  if (ev_is_active(w)) {
    if (w->repeat) {
      ev_at(w) = mn_now + w->repeat;
      ANHE_at_cache(timers[ev_active(w)]);
      adjustheap(timers, timercnt, ev_active(w));
    }
    else
      ev_timer_stop(EV_A_ w);
  }
  else if (w->repeat) {
    ev_at(w) = w->repeat;
    ev_timer_start(EV_A_ w);
  }

  EV_FREQUENT_CHECK;
}

ev_tstamp ev_timer_remaining(EV_P_ ev_timer* w) EV_NOEXCEPT {
  return ev_at(w) - (ev_is_active(w) ? mn_now : EV_TS_CONST(0.));
}

#if EV_PERIODIC_ENABLE
ecb_noinline void ev_periodic_start(EV_P_ ev_periodic* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

#if EV_USE_TIMERFD
  if (timerfd == -2)
    evtimerfd_init(EV_A);
#endif

  if (w->reschedule_cb)
    ev_at(w) = w->reschedule_cb(w, ev_rt_now);
  else if (w->interval) {
    EV_ASSERT_MSG("libev: ev_periodic_start called with negative interval value", w->interval >= 0.);
    periodic_recalc(EV_A_ w);
  }
  else
    ev_at(w) = w->offset;

  EV_FREQUENT_CHECK;

  ++periodiccnt;
  ev_start(EV_A_(W) w, periodiccnt + HEAP0 - 1);
  array_needsize(ANHE, periodics, periodicmax, ev_active(w) + 1, array_needsize_noinit);
  ANHE_w(periodics[ev_active(w)]) = (WT)w;
  ANHE_at_cache(periodics[ev_active(w)]);
  upheap(periodics, ev_active(w));

  EV_FREQUENT_CHECK;

  /*assert (("libev: internal periodic heap corruption", ANHE_w (periodics [ev_active (w)]) == (WT)w));*/
}

ecb_noinline void ev_periodic_stop(EV_P_ ev_periodic* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  {
    int active = ev_active(w);

    EV_ASSERT_MSG("libev: internal periodic heap corruption", ANHE_w(periodics[active]) == (WT)w);

    --periodiccnt;

    if (ecb_expect_true(active < periodiccnt + HEAP0)) {
      periodics[active] = periodics[periodiccnt + HEAP0];
      adjustheap(periodics, periodiccnt, active);
    }
  }

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}

ecb_noinline void ev_periodic_again(EV_P_ ev_periodic* w) EV_NOEXCEPT {
  EV_FREQUENT_CHECK;

  clear_pending(EV_A_(W) w);

  if (ev_is_active(w)) {
    if (w->reschedule_cb)
      ev_at(w) = w->reschedule_cb(w, ev_rt_now);
    else if (w->interval) {
      EV_ASSERT_MSG("libev: ev_periodic_again called with negative interval value", w->interval >= 0.);
      periodic_recalc(EV_A_ w);
    }
    else
      ev_at(w) = w->offset;

    ANHE_at_cache(periodics[ev_active(w)]);
    adjustheap(periodics, periodiccnt, ev_active(w));
  }
  else
    ev_periodic_start(EV_A_ w);

  EV_FREQUENT_CHECK;
}
#endif

#ifndef SA_RESTART
#define SA_RESTART 0
#endif

#if EV_SIGNAL_ENABLE

ecb_noinline void ev_signal_start(EV_P_ ev_signal* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  EV_ASSERT_MSG("libev: ev_signal_start called with illegal signal number", w->signum > 0 && w->signum < EV_NSIG);

#if EV_MULTIPLICITY
  EV_ASSERT_MSG("libev: a signal must not be attached to two different loops",
                !signals[w->signum - 1].loop || signals[w->signum - 1].loop == loop);

  signals[w->signum - 1].loop = EV_A;
  ECB_MEMORY_FENCE_RELEASE;
#endif

  EV_FREQUENT_CHECK;

#if EV_USE_SIGNALFD
  if (sigfd == -2) {
    sigfd = signalfd(-1, &sigfd_set, SFD_NONBLOCK | SFD_CLOEXEC);
    if (sigfd < 0 && errno == EINVAL)
      sigfd = signalfd(-1, &sigfd_set, 0); /* retry without flags */

    if (sigfd >= 0) {
      fd_intern(sigfd); /* doing it twice will not hurt */

      sigemptyset(&sigfd_set);

      ev_io_init(&sigfd_w, sigfdcb, sigfd, EV_READ);
      ev_set_priority(&sigfd_w, EV_MAXPRI);
      ev_io_start(EV_A_ & sigfd_w);
      ev_unref(EV_A); /* signalfd watcher should not keep loop alive */
    }
  }

  if (sigfd >= 0) {
    if (!signals[w->signum - 1].head) {
      sigaddset(&sigfd_set, w->signum);
      sigprocmask(SIG_BLOCK, &sigfd_set, 0);

      signalfd(sigfd, &sigfd_set, 0);
    }
  }
#endif

  ev_start(EV_A_(W) w, 1);
  wlist_add(&signals[w->signum - 1].head, (WL)w);

  if (!((WL)w)->next) {
    int need_signal_handler = 1;
#if EV_USE_SIGNALFD
    need_signal_handler = sigfd < 0;
#endif

    if (need_signal_handler) {
#ifdef _WIN32
      evpipe_init(EV_A);

      signal(w->signum, ev_sighandler);
#else
    struct sigaction sa;

    evpipe_init(EV_A);

    sa.sa_handler = ev_sighandler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* if restarting works we save one iteration */
    sigaction(w->signum, &sa, 0);

    if (origflags & EVFLAG_NOSIGMASK) {
      sigemptyset(&sa.sa_mask);
      sigaddset(&sa.sa_mask, w->signum);
      sigprocmask(SIG_UNBLOCK, &sa.sa_mask, 0);
    }
#endif
    }
  }

  EV_FREQUENT_CHECK;
}

ecb_noinline void ev_signal_stop(EV_P_ ev_signal* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  wlist_del(&signals[w->signum - 1].head, (WL)w);
  ev_stop(EV_A_(W) w);

  if (!signals[w->signum - 1].head) {
#if EV_MULTIPLICITY
    signals[w->signum - 1].loop = 0; /* unattach from signal */
#endif
#if EV_USE_SIGNALFD
    if (sigfd >= 0) {
      sigset_t ss;

      sigemptyset(&ss);
      sigaddset(&ss, w->signum);
      sigdelset(&sigfd_set, w->signum);

      signalfd(sigfd, &sigfd_set, 0);
      sigprocmask(SIG_UNBLOCK, &ss, 0);
    }
    else
#endif
      signal(w->signum, SIG_DFL);
  }

  EV_FREQUENT_CHECK;
}

#endif

#if EV_CHILD_ENABLE

void ev_child_start(EV_P_ ev_child* w) EV_NOEXCEPT {
#if EV_MULTIPLICITY
  EV_ASSERT_MSG("libev: child watchers are only supported in the default loop", loop == ev_default_loop_ptr);
#endif
  if (ecb_expect_false(ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  ev_start(EV_A_(W) w, 1);
  wlist_add(&childs[w->pid & ((EV_PID_HASHSIZE)-1)], (WL)w);

  EV_FREQUENT_CHECK;
}

void ev_child_stop(EV_P_ ev_child* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  wlist_del(&childs[w->pid & ((EV_PID_HASHSIZE)-1)], (WL)w);
  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}

#endif

#if EV_STAT_ENABLE

#ifdef _WIN32
#undef lstat
#define lstat(a, b) _stati64(a, b)
#endif

#define DEF_STAT_INTERVAL 5.0074891
#define NFS_STAT_INTERVAL 30.1074891 /* for filesystems potentially failing inotify */
#define MIN_STAT_INTERVAL 0.1074891

ecb_noinline static void stat_timer_cb(EV_P_ ev_timer* w_, int revents);

#if EV_USE_INOTIFY

/* the * 2 is to allow for alignment padding, which for some reason is >> 8 */
#define EV_INOTIFY_BUFSIZE (sizeof(struct inotify_event) * 2 + NAME_MAX)

ecb_noinline static void infy_add(EV_P_ ev_stat* w) {
  w->wd = inotify_add_watch(fs_fd, w->path,
                            IN_ATTRIB | IN_DELETE_SELF | IN_MOVE_SELF | IN_MODIFY | IN_CREATE | IN_DELETE |
                                IN_MOVED_FROM | IN_MOVED_TO | IN_DONT_FOLLOW | IN_MASK_ADD);

  if (w->wd >= 0) {
    struct statfs sfs;

    /* now local changes will be tracked by inotify, but remote changes won't */
    /* unless the filesystem is known to be local, we therefore still poll */
    /* also do poll on <2.6.25, but with normal frequency */

    if (!fs_2625)
      w->timer.repeat = w->interval ? w->interval : DEF_STAT_INTERVAL;
    else if (!statfs(w->path, &sfs) && (sfs.f_type == 0x1373        /* devfs */
                                        || sfs.f_type == 0x4006     /* fat */
                                        || sfs.f_type == 0x4d44     /* msdos */
                                        || sfs.f_type == 0xEF53     /* ext2/3 */
                                        || sfs.f_type == 0x72b6     /* jffs2 */
                                        || sfs.f_type == 0x858458f6 /* ramfs */
                                        || sfs.f_type == 0x5346544e /* ntfs */
                                        || sfs.f_type == 0x3153464a /* jfs */
                                        || sfs.f_type == 0x9123683e /* btrfs */
                                        || sfs.f_type == 0x52654973 /* reiser3 */
                                        || sfs.f_type == 0x01021994 /* tmpfs */
                                        || sfs.f_type == 0x58465342 /* xfs */))
      w->timer.repeat = 0.; /* filesystem is local, kernel new enough */
    else
      w->timer.repeat = w->interval ? w->interval : NFS_STAT_INTERVAL; /* remote, use reduced frequency */
  }
  else {
    /* can't use inotify, continue to stat */
    w->timer.repeat = w->interval ? w->interval : DEF_STAT_INTERVAL;

    /* if path is not there, monitor some parent directory for speedup hints */
    /* note that exceeding the hardcoded path limit is not a correctness issue, */
    /* but an efficiency issue only */
    if ((errno == ENOENT || errno == EACCES) && strlen(w->path) < 4096) {
      char path[4096];
      strcpy(path, w->path);

      do {
        int mask =
            IN_MASK_ADD | IN_DELETE_SELF | IN_MOVE_SELF | (errno == EACCES ? IN_ATTRIB : IN_CREATE | IN_MOVED_TO);

        char* pend = strrchr(path, '/');

        if (!pend || pend == path)
          break;

        *pend = 0;
        w->wd = inotify_add_watch(fs_fd, path, mask);
      } while (w->wd < 0 && (errno == ENOENT || errno == EACCES));
    }
  }

  if (w->wd >= 0)
    wlist_add(&fs_hash[w->wd & ((EV_INOTIFY_HASHSIZE)-1)].head, (WL)w);

  /* now re-arm timer, if required */
  if (ev_is_active(&w->timer))
    ev_ref(EV_A);
  ev_timer_again(EV_A_ & w->timer);
  if (ev_is_active(&w->timer))
    ev_unref(EV_A);
}

ecb_noinline static void infy_del(EV_P_ ev_stat* w) {
  int slot;
  int wd = w->wd;

  if (wd < 0)
    return;

  w->wd = -2;
  slot = wd & ((EV_INOTIFY_HASHSIZE)-1);
  wlist_del(&fs_hash[slot].head, (WL)w);

  /* remove this watcher, if others are watching it, they will rearm */
  inotify_rm_watch(fs_fd, wd);
}

ecb_noinline static void infy_wd(EV_P_ int slot, int wd, struct inotify_event* ev) {
  if (slot < 0)
    /* overflow, need to check for all hash slots */
    for (slot = 0; slot < (EV_INOTIFY_HASHSIZE); ++slot)
      infy_wd(EV_A_ slot, wd, ev);
  else {
    WL w_;

    for (w_ = fs_hash[slot & ((EV_INOTIFY_HASHSIZE)-1)].head; w_;) {
      ev_stat* w = (ev_stat*)w_;
      w_ = w_->next; /* lets us remove this watcher and all before it */

      if (w->wd == wd || wd == -1) {
        if (ev->mask & (IN_IGNORED | IN_UNMOUNT | IN_DELETE_SELF)) {
          wlist_del(&fs_hash[slot & ((EV_INOTIFY_HASHSIZE)-1)].head, (WL)w);
          w->wd = -1;
          infy_add(EV_A_ w); /* re-add, no matter what */
        }

        stat_timer_cb(EV_A_ & w->timer, 0);
      }
    }
  }
}

static void infy_cb(EV_P_ ev_io* w, int revents) {
  char buf[EV_INOTIFY_BUFSIZE];
  int ofs;
  int len = read(fs_fd, buf, sizeof(buf));

  (void)w;
  (void)revents;

  for (ofs = 0; ofs < len;) {
    struct inotify_event* ev = (struct inotify_event*)(buf + ofs);
    infy_wd(EV_A_ ev->wd, ev->wd, ev);
    ofs += sizeof(struct inotify_event) + ev->len;
  }
}

inline_size ecb_cold void ev_check_2625(EV_P) {
  /* kernels < 2.6.25 are borked
   * http://www.ussg.indiana.edu/hypermail/linux/kernel/0711.3/1208.html
   */
  if (ev_linux_version() < 0x020619)
    return;

  fs_2625 = 1;
}

inline_size int infy_newfd(void) {
#if defined IN_CLOEXEC && defined IN_NONBLOCK
  int fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
  if (fd >= 0)
    return fd;
#endif
  return inotify_init();
}

inline_size void infy_init(EV_P) {
  if (fs_fd != -2)
    return;

  fs_fd = -1;

  ev_check_2625(EV_A);

  fs_fd = infy_newfd();

  if (fs_fd >= 0) {
    fd_intern(fs_fd);
    ev_io_init(&fs_w, infy_cb, fs_fd, EV_READ);
    ev_set_priority(&fs_w, EV_MAXPRI);
    ev_io_start(EV_A_ & fs_w);
    ev_unref(EV_A);
  }
}

inline_size void infy_fork(EV_P) {
  int slot;

  if (fs_fd < 0)
    return;

  ev_ref(EV_A);
  ev_io_stop(EV_A_ & fs_w);
  close(fs_fd);
  fs_fd = infy_newfd();

  if (fs_fd >= 0) {
    fd_intern(fs_fd);
    ev_io_set(&fs_w, fs_fd, EV_READ);
    ev_io_start(EV_A_ & fs_w);
    ev_unref(EV_A);
  }

  for (slot = 0; slot < (EV_INOTIFY_HASHSIZE); ++slot) {
    WL w_ = fs_hash[slot].head;
    fs_hash[slot].head = 0;

    while (w_) {
      ev_stat* w = (ev_stat*)w_;
      w_ = w_->next; /* lets us add this watcher */

      w->wd = -1;

      if (fs_fd >= 0)
        infy_add(EV_A_ w); /* re-add, no matter what */
      else {
        w->timer.repeat = w->interval ? w->interval : DEF_STAT_INTERVAL;
        if (ev_is_active(&w->timer))
          ev_ref(EV_A);
        ev_timer_again(EV_A_ & w->timer);
        if (ev_is_active(&w->timer))
          ev_unref(EV_A);
      }
    }
  }
}

#endif

#ifdef _WIN32
#define EV_LSTAT(p, b) _stati64(p, b)
#else
#define EV_LSTAT(p, b) lstat(p, b)
#endif

void ev_stat_stat(EV_P_ ev_stat* w) EV_NOEXCEPT {
  (void)loop;

  if (lstat(w->path, &w->attr) < 0)
    w->attr.st_nlink = 0;
  else if (!w->attr.st_nlink)
    w->attr.st_nlink = 1;
}

ecb_noinline static void stat_timer_cb(EV_P_ ev_timer* w_, int revents) {
  ev_stat* w = (ev_stat*)(((char*)w_) - offsetof(ev_stat, timer));

  (void)revents;

  ev_statdata prev = w->attr;
  ev_stat_stat(EV_A_ w);

  /* memcmp doesn't work on netbsd, they.... do stuff to their struct stat */
  if (prev.st_dev != w->attr.st_dev || prev.st_ino != w->attr.st_ino || prev.st_mode != w->attr.st_mode ||
      prev.st_nlink != w->attr.st_nlink || prev.st_uid != w->attr.st_uid || prev.st_gid != w->attr.st_gid ||
      prev.st_rdev != w->attr.st_rdev || prev.st_size != w->attr.st_size || prev.st_atime != w->attr.st_atime ||
      prev.st_mtime != w->attr.st_mtime || prev.st_ctime != w->attr.st_ctime) {
    /* we only update w->prev on actual differences */
    /* in case we test more often than invoke the callback, */
    /* to ensure that prev is always different to attr */
    w->prev = prev;

#if EV_USE_INOTIFY
    if (fs_fd >= 0) {
      infy_del(EV_A_ w);
      infy_add(EV_A_ w);
      ev_stat_stat(EV_A_ w); /* avoid race... */
    }
#endif

    ev_feed_event(EV_A_ w, EV_STAT);
  }
}

void ev_stat_start(EV_P_ ev_stat* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  ev_stat_stat(EV_A_ w);

  if (w->interval < MIN_STAT_INTERVAL && w->interval)
    w->interval = MIN_STAT_INTERVAL;

  ev_timer_init(&w->timer, stat_timer_cb, 0., w->interval ? w->interval : DEF_STAT_INTERVAL);
  ev_set_priority(&w->timer, ev_priority(w));

#if EV_USE_INOTIFY
  infy_init(EV_A);

  if (fs_fd >= 0)
    infy_add(EV_A_ w);
  else
#endif
  {
    ev_timer_again(EV_A_ & w->timer);
    ev_unref(EV_A);
  }

  ev_start(EV_A_(W) w, 1);

  EV_FREQUENT_CHECK;
}

void ev_stat_stop(EV_P_ ev_stat* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

#if EV_USE_INOTIFY
  infy_del(EV_A_ w);
#endif

  if (ev_is_active(&w->timer)) {
    ev_ref(EV_A);
    ev_timer_stop(EV_A_ & w->timer);
  }

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}
#endif

#if EV_IDLE_ENABLE
void ev_idle_start(EV_P_ ev_idle* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  pri_adjust(EV_A_(W) w);

  EV_FREQUENT_CHECK;

  {
    int active = ++idlecnt[ABSPRI(w)];

    ++idleall;
    ev_start(EV_A_(W) w, active);

    array_needsize(ev_idle*, idles[ABSPRI(w)], idlemax[ABSPRI(w)], active, array_needsize_noinit);
    idles[ABSPRI(w)][active - 1] = w;
  }

  EV_FREQUENT_CHECK;
}

void ev_idle_stop(EV_P_ ev_idle* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  {
    int active = ev_active(w);

    idles[ABSPRI(w)][active - 1] = idles[ABSPRI(w)][--idlecnt[ABSPRI(w)]];
    ev_active(idles[ABSPRI(w)][active - 1]) = active;

    ev_stop(EV_A_(W) w);
    --idleall;
  }

  EV_FREQUENT_CHECK;
}
#endif

#if EV_PREPARE_ENABLE
void ev_prepare_start(EV_P_ ev_prepare* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  ev_start(EV_A_(W) w, ++preparecnt);
  array_needsize(ev_prepare*, prepares, preparemax, preparecnt, array_needsize_noinit);
  prepares[preparecnt - 1] = w;

  EV_FREQUENT_CHECK;
}

void ev_prepare_stop(EV_P_ ev_prepare* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  {
    int active = ev_active(w);

    prepares[active - 1] = prepares[--preparecnt];
    ev_active(prepares[active - 1]) = active;
  }

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}
#endif

#if EV_CHECK_ENABLE
void ev_check_start(EV_P_ ev_check* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  ev_start(EV_A_(W) w, ++checkcnt);
  array_needsize(ev_check*, checks, checkmax, checkcnt, array_needsize_noinit);
  checks[checkcnt - 1] = w;

  EV_FREQUENT_CHECK;
}

void ev_check_stop(EV_P_ ev_check* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  {
    int active = ev_active(w);

    checks[active - 1] = checks[--checkcnt];
    ev_active(checks[active - 1]) = active;
  }

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}
#endif

#if EV_EMBED_ENABLE
ecb_noinline void ev_embed_sweep(EV_P_ ev_embed* w) EV_NOEXCEPT {
  (void)loop;

  ev_run(w->other, EVRUN_NOWAIT);
}

static void embed_io_cb(EV_P_ ev_io* io, int revents) {
  ev_embed* w = (ev_embed*)(((char*)io) - offsetof(ev_embed, io));

  (void)revents;

  if (ev_cb(w))
    ev_feed_event(EV_A_(W) w, EV_EMBED);
  else
    ev_run(w->other, EVRUN_NOWAIT);
}

static void embed_prepare_cb(EV_P_ ev_prepare* prepare, int revents) {
  ev_embed* w = (ev_embed*)(((char*)prepare) - offsetof(ev_embed, prepare));

  (void)revents;

  {
    loop = w->other;

    while (fdchangecnt) {
      fd_reify(EV_A);
      ev_run(EV_A_ EVRUN_NOWAIT);
    }
  }
}

#if EV_FORK_ENABLE
static void embed_fork_cb(EV_P_ ev_fork* fork_w, int revents) {
  ev_embed* w = (ev_embed*)(((char*)fork_w) - offsetof(ev_embed, fork));

  (void)revents;

  ev_embed_stop(EV_A_ w);

  {
    EV_P = w->other;

    ev_loop_fork(EV_A);
    ev_run(EV_A_ EVRUN_NOWAIT);
  }

  ev_embed_start(EV_A_ w);
}
#endif

#if 0
static void
embed_idle_cb (EV_P_ ev_idle *idle, int revents)
{
  ev_idle_stop (EV_A_ idle);
}
#endif

void ev_embed_start(EV_P_ ev_embed* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  {
    EV_P = w->other;
    EV_ASSERT_MSG("libev: loop to be embedded is not embeddable", backend & ev_embeddable_backends());
    ev_io_init(&w->io, embed_io_cb, backend_fd, EV_READ);
  }

  EV_FREQUENT_CHECK;

  ev_set_priority(&w->io, ev_priority(w));
  ev_io_start(EV_A_ & w->io);

  ev_prepare_init(&w->prepare, embed_prepare_cb);
  ev_set_priority(&w->prepare, EV_MINPRI);
  ev_prepare_start(EV_A_ & w->prepare);

#if EV_FORK_ENABLE
  ev_fork_init(&w->fork, embed_fork_cb);
  ev_fork_start(EV_A_ & w->fork);
#endif

  /*ev_idle_init (&w->idle, e,bed_idle_cb);*/

  ev_start(EV_A_(W) w, 1);

  EV_FREQUENT_CHECK;
}

void ev_embed_stop(EV_P_ ev_embed* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  ev_io_stop(EV_A_ & w->io);
  ev_prepare_stop(EV_A_ & w->prepare);
#if EV_FORK_ENABLE
  ev_fork_stop(EV_A_ & w->fork);
#endif

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}
#endif

#if EV_FORK_ENABLE
void ev_fork_start(EV_P_ ev_fork* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  ev_start(EV_A_(W) w, ++forkcnt);
  array_needsize(ev_fork*, forks, forkmax, forkcnt, array_needsize_noinit);
  forks[forkcnt - 1] = w;

  EV_FREQUENT_CHECK;
}

void ev_fork_stop(EV_P_ ev_fork* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  {
    int active = ev_active(w);

    forks[active - 1] = forks[--forkcnt];
    ev_active(forks[active - 1]) = active;
  }

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}
#endif

#if EV_CLEANUP_ENABLE
void ev_cleanup_start(EV_P_ ev_cleanup* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  ev_start(EV_A_(W) w, ++cleanupcnt);
  array_needsize(ev_cleanup*, cleanups, cleanupmax, cleanupcnt, array_needsize_noinit);
  cleanups[cleanupcnt - 1] = w;

  /* cleanup watchers should never keep a refcount on the loop */
  ev_unref(EV_A);
  EV_FREQUENT_CHECK;
}

void ev_cleanup_stop(EV_P_ ev_cleanup* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;
  ev_ref(EV_A);

  {
    int active = ev_active(w);

    cleanups[active - 1] = cleanups[--cleanupcnt];
    ev_active(cleanups[active - 1]) = active;
  }

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}
#endif

#if EV_ASYNC_ENABLE
void ev_async_start(EV_P_ ev_async* w) EV_NOEXCEPT {
  if (ecb_expect_false(ev_is_active(w)))
    return;

  w->sent = 0;

  evpipe_init(EV_A);

  EV_FREQUENT_CHECK;

  ev_start(EV_A_(W) w, ++asynccnt);
  array_needsize(ev_async*, asyncs, asyncmax, asynccnt, array_needsize_noinit);
  asyncs[asynccnt - 1] = w;

  EV_FREQUENT_CHECK;
}

void ev_async_stop(EV_P_ ev_async* w) EV_NOEXCEPT {
  clear_pending(EV_A_(W) w);
  if (ecb_expect_false(!ev_is_active(w)))
    return;

  EV_FREQUENT_CHECK;

  {
    int active = ev_active(w);

    asyncs[active - 1] = asyncs[--asynccnt];
    ev_active(asyncs[active - 1]) = active;
  }

  ev_stop(EV_A_(W) w);

  EV_FREQUENT_CHECK;
}

void ev_async_send(EV_P_ ev_async* w) EV_NOEXCEPT {
  w->sent = 1;
  evpipe_write(EV_A_ & async_pending);
}
#endif

/*****************************************************************************/

struct ev_once {
  ev_io io;
  ev_timer to;
  void (*cb)(int revents, void* arg);
  void* arg;
};

static void once_cb(EV_P_ struct ev_once* once, int revents) {
  void (*cb)(int revents, void* arg) = once->cb;
  void* arg = once->arg;

  ev_io_stop(EV_A_ & once->io);
  ev_timer_stop(EV_A_ & once->to);
  ev_free(once);

  cb(revents, arg);
}

static void once_cb_io(EV_P_ ev_io* w, int revents) {
  struct ev_once* once = (struct ev_once*)(((char*)w) - offsetof(struct ev_once, io));

  once_cb(EV_A_ once, revents | ev_clear_pending(EV_A_ & once->to));
}

static void once_cb_to(EV_P_ ev_timer* w, int revents) {
  struct ev_once* once = (struct ev_once*)(((char*)w) - offsetof(struct ev_once, to));

  once_cb(EV_A_ once, revents | ev_clear_pending(EV_A_ & once->io));
}

void ev_once(EV_P_ int fd, int events, ev_tstamp timeout, void (*cb)(int revents, void* arg), void* arg) EV_NOEXCEPT {
  struct ev_once* once = (struct ev_once*)ev_malloc(sizeof(struct ev_once));

  once->cb = cb;
  once->arg = arg;

  ev_init(&once->io, once_cb_io);
  if (fd >= 0) {
    ev_io_set(&once->io, fd, events);
    ev_io_start(EV_A_ & once->io);
  }

  ev_init(&once->to, once_cb_to);
  if (timeout >= 0.) {
    ev_timer_set(&once->to, timeout, 0.);
    ev_timer_start(EV_A_ & once->to);
  }
}

/*****************************************************************************/

#if EV_WALK_ENABLE
ecb_cold void ev_walk(EV_P_ int types, void (*cb)(EV_P_ int type, void* w)) EV_NOEXCEPT {
  int i, j;
  ev_watcher_list *wl, *wn;

  if (types & (EV_IO | EV_EMBED))
    for (i = 0; i < anfdmax; ++i)
      for (wl = anfds[i].head; wl;) {
        wn = wl->next;

#if EV_EMBED_ENABLE
        if (ev_cb((ev_io*)wl) == embed_io_cb) {
          if (types & EV_EMBED)
            cb(EV_A_ EV_EMBED, ((char*)wl) - offsetof(struct ev_embed, io));
        }
        else
#endif
#if EV_USE_INOTIFY
            if (ev_cb((ev_io*)wl) == infy_cb)
          ;
        else
#endif
            if ((ev_io*)wl != &pipe_w)
          if (types & EV_IO)
            cb(EV_A_ EV_IO, wl);

        wl = wn;
      }

  if (types & (EV_TIMER | EV_STAT))
    for (i = timercnt + HEAP0; i-- > HEAP0;)
#if EV_STAT_ENABLE
      /* timer may be inactive when the stat watcher relies on inotify */
      if (ev_cb((ev_timer*)ANHE_w(timers[i])) == stat_timer_cb) {
        if (types & EV_STAT)
          cb(EV_A_ EV_STAT, ((char*)ANHE_w(timers[i])) - offsetof(struct ev_stat, timer));
      }
      else
#endif
          if (types & EV_TIMER)
        cb(EV_A_ EV_TIMER, ANHE_w(timers[i]));

#if EV_STAT_ENABLE && EV_USE_INOTIFY
  if (types & EV_STAT)
    for (i = 0; i < (EV_INOTIFY_HASHSIZE); ++i)
      for (wl = fs_hash[i].head; wl; wl = wl->next) {
        ev_stat* w = (ev_stat*)wl;

        if (!ev_is_active(&w->timer))
          cb(EV_A_ EV_STAT, w);
      }
#endif

#if EV_PERIODIC_ENABLE
  if (types & EV_PERIODIC)
    for (i = periodiccnt + HEAP0; i-- > HEAP0;)
      cb(EV_A_ EV_PERIODIC, ANHE_w(periodics[i]));
#endif

#if EV_IDLE_ENABLE
  if (types & EV_IDLE)
    for (j = NUMPRI; j--;)
      for (i = idlecnt[j]; i--;)
        cb(EV_A_ EV_IDLE, idles[j][i]);
#endif

#if EV_FORK_ENABLE
  if (types & EV_FORK)
    for (i = forkcnt; i--;)
      if (ev_cb(forks[i]) != embed_fork_cb)
        cb(EV_A_ EV_FORK, forks[i]);
#endif

#if EV_ASYNC_ENABLE
  if (types & EV_ASYNC)
    for (i = asynccnt; i--;)
      cb(EV_A_ EV_ASYNC, asyncs[i]);
#endif

#if EV_PREPARE_ENABLE
  if (types & EV_PREPARE)
    for (i = preparecnt; i--;)
#if EV_EMBED_ENABLE
      if (ev_cb(prepares[i]) != embed_prepare_cb)
#endif
        cb(EV_A_ EV_PREPARE, prepares[i]);
#endif

#if EV_CHECK_ENABLE
  if (types & EV_CHECK)
    for (i = checkcnt; i--;)
      cb(EV_A_ EV_CHECK, checks[i]);
#endif

#if EV_SIGNAL_ENABLE
  if (types & EV_SIGNAL)
    for (i = 0; i < EV_NSIG - 1; ++i)
      for (wl = signals[i].head; wl;) {
        wn = wl->next;
        cb(EV_A_ EV_SIGNAL, wl);
        wl = wn;
      }
#endif

#if EV_CHILD_ENABLE
  if (types & EV_CHILD)
    for (i = (EV_PID_HASHSIZE); i--;)
      for (wl = childs[i]; wl;) {
        wn = wl->next;
        cb(EV_A_ EV_CHILD, wl);
        wl = wn;
      }
#endif
  /* EV_STAT     0x00001000 -- stat data changed */
  /* EV_EMBED    0x00010000 -- embedded event loop needs sweep */
}
#endif

#if EV_MULTIPLICITY
#include "ev_wrap.h"
#endif
