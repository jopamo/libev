/* src/ev_loop_core.c
 * Core watcher/event logic split from ev.c
 */

#define MALLOC_ROUND 4096 /* prefer to allocate in chunks of this size, must be 2**n and >> 4 longs */

/* find a suitable new size for the given array */
/* hopefully by rounding to a nice-to-malloc size */
inline_size int array_nextsize(int elem, int cur, int cnt) EV_NOEXCEPT {
  int ncur = cur + 1;

  do
    ncur <<= 1;
  while (cnt > ncur);

  /* if size is large, round to MALLOC_ROUND - 4 * longs to accommodate malloc overhead */
  {
    size_t bytes = (size_t)elem * (size_t)ncur;

    if (bytes > (size_t)MALLOC_ROUND - sizeof(void*) * 4) {
      size_t nbytes = bytes;

      nbytes = nbytes + (size_t)elem + (MALLOC_ROUND - 1) + sizeof(void*) * 4;
      nbytes &= ~(size_t)(MALLOC_ROUND - 1);
      nbytes -= sizeof(void*) * 4;

      ncur = (int)(nbytes / (size_t)elem);
    }
  }

  return ncur;
}

ecb_noinline ecb_cold static void* array_realloc(int elem, void* base, int* cur, int cnt) EV_NOEXCEPT {
  *cur = array_nextsize(elem, *cur, cnt);
  return ev_realloc(base, elem * *cur);
}

#define array_needsize_noinit(base, offset, count)

#define array_needsize_zerofill(base, offset, count) memset((void*)(base + offset), 0, sizeof(*(base)) * (count))

#define array_needsize(type, base, cur, cnt, init)                      \
  if (ecb_expect_false((cnt) > (cur))) {                                \
    ecb_unused int ocur_ = (cur);                                       \
    (base) = (type*)array_realloc(sizeof(type), (base), &(cur), (cnt)); \
    init((base), ocur_, ((cur) - ocur_));                               \
  }

#if 0
#define array_slim(type, stem)                                          \
  if (stem##max < array_roundsize(stem##cnt >> 2)) {                    \
    stem##max = array_roundsize(stem##cnt >> 1);                        \
    base = (type*)ev_realloc(base, sizeof(type) * (stem##max));         \
    fprintf(stderr, "slimmed down " #stem " to %d\n", stem##max); /*D*/ \
  }
#endif

#define array_free(stem, idx)        \
  ev_free(stem##s idx);              \
  stem##cnt idx = stem##max idx = 0; \
  stem##s idx = 0

/*****************************************************************************/

/* dummy callback for pending events */
ecb_noinline static void pendingcb(EV_P_ ev_prepare* w, int revents) {
  (void)loop;
  (void)w;
  (void)revents;
}

ecb_noinline void ev_feed_event(EV_P_ void* w, int revents) EV_NOEXCEPT {
  W w_ = (W)w;
  int pri = ABSPRI(w_);

  if (ecb_expect_false(w_->pending))
    pendings[pri][w_->pending - 1].events |= revents;
  else {
    w_->pending = ++pendingcnt[pri];
    array_needsize(ANPENDING, pendings[pri], pendingmax[pri], w_->pending, array_needsize_noinit);
    pendings[pri][w_->pending - 1].w = w_;
    pendings[pri][w_->pending - 1].events = revents;
  }

  if (pri > pendingpri)
    pendingpri = pri;
}

inline_speed void feed_reverse(EV_P_ W w) {
  array_needsize(W, rfeeds, rfeedmax, rfeedcnt + 1, array_needsize_noinit);
  rfeeds[rfeedcnt++] = w;
}

inline_size void feed_reverse_done(EV_P_ int revents) {
  do
    ev_feed_event(EV_A_ rfeeds[--rfeedcnt], revents);
  while (rfeedcnt);
}

inline_speed void queue_events(EV_P_ W* events, int eventcnt, int type) {
  int i;

  for (i = 0; i < eventcnt; ++i)
    ev_feed_event(EV_A_ events[i], type);
}

/*****************************************************************************/

inline_speed void fd_event_nocheck(EV_P_ int fd, int revents) {
  ANFD* anfd = anfds + fd;
  ev_io* w;

  for (w = (ev_io*)anfd->head; w; w = (ev_io*)((WL)w)->next) {
    int ev = w->events & revents;

    if (ev)
      ev_feed_event(EV_A_(W) w, ev);
  }
}

/* do not submit kernel events for fds that have reify set */
/* because that means they changed while we were polling for new events */
inline_speed void fd_event(EV_P_ int fd, int revents) {
  ANFD* anfd = anfds + fd;

  if (ecb_expect_true(!anfd->reify))
    fd_event_nocheck(EV_A_ fd, revents);
}

void ev_feed_fd_event(EV_P_ int fd, int revents) EV_NOEXCEPT {
  if (fd >= 0 && fd < anfdmax)
    fd_event_nocheck(EV_A_ fd, revents);
}

/* make sure the external fd watch events are in-sync */
/* with the kernel/libev internal state */
inline_size void fd_reify(EV_P) {
  int i;

  /* most backends do not modify the fdchanges list in backend_modfiy
   * except io_uring, which has fixed-size buffers which might force us
   * to handle events in backend_modify, causing fdchanges to be amended,
   * which could result in an endless loop
   * to avoid this, we do not dynamically handle fds that were added
   * during fd_reify. that means that for those backends, fdchangecnt
   * might be non-zero during poll, which must cause them to not block
   * to not put too much of a burden on other backends, this detail
   * needs to be handled in the backend
   */
  int changecnt = fdchangecnt;

#if EV_SELECT_IS_WINSOCKET || EV_USE_IOCP
  for (i = 0; i < changecnt; ++i) {
    int fd = fdchanges[i];
    ANFD* anfd = anfds + fd;

    if (anfd->reify & EV__IOFDSET && anfd->head) {
      SOCKET handle = EV_FD_TO_WIN32_HANDLE(fd);
      unsigned long arg = 0;

      EV_ASSERT_MSG("libev: only socket fds supported in this configuration", ioctlsocket(handle, FIONREAD, &arg) == 0);

      /* handle changed, but fd didn't - we need to do it in two steps */
      backend_modify(EV_A_ fd, anfd->events, 0);
      anfd->events = 0;
      anfd->handle = handle;
    }
  }
#endif

  for (i = 0; i < changecnt; ++i) {
    int fd = fdchanges[i];
    ANFD* anfd = anfds + fd;
    ev_io* w;

    unsigned char o_events = anfd->events;
    unsigned int o_reify = anfd->reify;

    anfd->reify = 0;

    /*if (ecb_expect_true (o_reify & EV_ANFD_REIFY)) probably a deoptimisation */
    {
      anfd->events = 0;

      for (w = (ev_io*)anfd->head; w; w = (ev_io*)((WL)w)->next)
        anfd->events |= (unsigned char)w->events;

      if (o_events != anfd->events)
        o_reify = EV__IOFDSET; /* actually |= */
    }

    if (o_reify & EV__IOFDSET)
      backend_modify(EV_A_ fd, o_events, anfd->events);
  }

  /* normally, fdchangecnt hasn't changed. if it has, then new fds have been added.
   * this is a rare case (see beginning comment in this function), so we copy them to the
   * front and hope the backend handles this case
   */
  if (ecb_expect_false(fdchangecnt != changecnt))
    memmove(fdchanges, fdchanges + changecnt, (fdchangecnt - changecnt) * sizeof(*fdchanges));

  fdchangecnt -= changecnt;
}

/* something about the given fd changed */
inline_size void fd_change(EV_P_ int fd, int flags) {
  unsigned int reify = anfds[fd].reify;
  anfds[fd].reify = reify | flags;

  if (ecb_expect_true(!reify)) {
    ++fdchangecnt;
    array_needsize(int, fdchanges, fdchangemax, fdchangecnt, array_needsize_noinit);
    fdchanges[fdchangecnt - 1] = fd;
  }
}

/* the given fd is invalid/unusable, so make sure it doesn't hurt us anymore */
inline_speed ecb_cold void fd_kill(EV_P_ int fd) {
  ev_io* w;

  while ((w = (ev_io*)anfds[fd].head)) {
    ev_io_stop(EV_A_ w);
    ev_feed_event(EV_A_(W) w, EV_ERROR | EV_READ | EV_WRITE);
  }
}

/* check whether the given fd is actually valid, for error recovery */
inline_size ecb_cold int fd_valid(int fd) {
#ifdef _WIN32
  return EV_FD_TO_WIN32_HANDLE(fd) != -1;
#else
  return fcntl(fd, F_GETFD) != -1;
#endif
}

/* called on EBADF to verify fds */
ecb_noinline ecb_cold static void fd_ebadf(EV_P) {
  int fd;

  for (fd = 0; fd < anfdmax; ++fd)
    if (anfds[fd].events)
      if (!fd_valid(fd) && errno == EBADF)
        fd_kill(EV_A_ fd);
}

/* called on ENOMEM in select/poll to kill some fds and retry */
ecb_noinline ecb_cold static void fd_enomem(EV_P) {
  int fd;

  for (fd = anfdmax; fd--;)
    if (anfds[fd].events) {
      fd_kill(EV_A_ fd);
      break;
    }
}

/* usually called after fork if backend needs to re-arm all fds from scratch */
ecb_noinline static void fd_rearm_all(EV_P) {
  int fd;

  for (fd = 0; fd < anfdmax; ++fd)
    if (anfds[fd].events) {
      anfds[fd].events = 0;
      anfds[fd].emask = 0;
      fd_change(EV_A_ fd, EV__IOFDSET | EV_ANFD_REIFY);
    }
}

/* used to prepare libev internal fd's */
/* this is not fork-safe */
inline_speed void fd_intern(int fd) {
#ifdef _WIN32
  unsigned long arg = 1;
  ioctlsocket(EV_FD_TO_WIN32_HANDLE(fd), FIONBIO, &arg);
#else
  fcntl(fd, F_SETFD, FD_CLOEXEC);
  fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
}

/*****************************************************************************/

/*
 * the heap functions want a real array index. array index 0 is guaranteed to not
 * be in-use at any time. the first heap entry is at array [HEAP0]. DHEAP gives
 * the branching factor of the d-tree.
 */

/*
 * at the moment we allow libev the luxury of two heaps,
 * a small-code-size 2-heap one and a ~1.5kb larger 4-heap
 * which is more cache-efficient.
 * the difference is about 5% with 50000+ watchers.
 */
#if EV_USE_4HEAP

#define DHEAP 4
#define HEAP0 (DHEAP - 1) /* index of first element in heap */
#define HPARENT(k) ((((k) - HEAP0 - 1) / DHEAP) + HEAP0)
#define UPHEAP_DONE(p, k) ((p) == (k))

/* away from the root */
inline_speed void downheap(ANHE* heap, int N, int k) {
  ANHE he = heap[k];
  ANHE* E = heap + N + HEAP0;

  for (;;) {
    ev_tstamp minat;
    ANHE* minpos;
    ANHE* pos = heap + DHEAP * (k - HEAP0) + HEAP0 + 1;

    /* find minimum child */
    if (ecb_expect_true(pos + DHEAP - 1 < E)) {
      /* fast path */ (minpos = pos + 0), (minat = ANHE_at(*minpos));
      if (minat > ANHE_at(pos[1]))
        (minpos = pos + 1), (minat = ANHE_at(*minpos));
      if (minat > ANHE_at(pos[2]))
        (minpos = pos + 2), (minat = ANHE_at(*minpos));
      if (minat > ANHE_at(pos[3]))
        (minpos = pos + 3), (minat = ANHE_at(*minpos));
    }
    else if (pos < E) {
      /* slow path */ (minpos = pos + 0), (minat = ANHE_at(*minpos));
      if (pos + 1 < E && minat > ANHE_at(pos[1]))
        (minpos = pos + 1), (minat = ANHE_at(*minpos));
      if (pos + 2 < E && minat > ANHE_at(pos[2]))
        (minpos = pos + 2), (minat = ANHE_at(*minpos));
      if (pos + 3 < E && minat > ANHE_at(pos[3]))
        (minpos = pos + 3), (minat = ANHE_at(*minpos));
    }
    else
      break;

    if (ANHE_at(he) <= minat)
      break;

    heap[k] = *minpos;
    ev_active(ANHE_w(*minpos)) = k;

    k = minpos - heap;
  }

  heap[k] = he;
  ev_active(ANHE_w(he)) = k;
}

#else /* not 4HEAP */

#define HEAP0 1
#define HPARENT(k) ((k) >> 1)
#define UPHEAP_DONE(p, k) (!(p))

/* away from the root */
inline_speed void downheap(ANHE* heap, int N, int k) {
  ANHE he = heap[k];

  for (;;) {
    int c = k << 1;

    if (c >= N + HEAP0)
      break;

    c += c + 1 < N + HEAP0 && ANHE_at(heap[c]) > ANHE_at(heap[c + 1]) ? 1 : 0;

    if (ANHE_at(he) <= ANHE_at(heap[c]))
      break;

    heap[k] = heap[c];
    ev_active(ANHE_w(heap[k])) = k;

    k = c;
  }

  heap[k] = he;
  ev_active(ANHE_w(he)) = k;
}
#endif

/* towards the root */
inline_speed void upheap(ANHE* heap, int k) {
  ANHE he = heap[k];

  for (;;) {
    int p = HPARENT(k);

    if (UPHEAP_DONE(p, k) || ANHE_at(heap[p]) <= ANHE_at(he))
      break;

    heap[k] = heap[p];
    ev_active(ANHE_w(heap[k])) = k;
    k = p;
  }

  heap[k] = he;
  ev_active(ANHE_w(he)) = k;
}

/* move an element suitably so it is in a correct place */
inline_size void adjustheap(ANHE* heap, int N, int k) {
  if (k > HEAP0 && ANHE_at(heap[k]) <= ANHE_at(heap[HPARENT(k)]))
    upheap(heap, k);
  else
    downheap(heap, N, k);
}

/* rebuild the heap: this function is used only once and executed rarely */
inline_size void reheap(ANHE* heap, int N) {
  int i;

  /* we don't use floyds algorithm, upheap is simpler and is more cache-efficient */
  /* also, this is easy to implement and correct for both 2-heaps and 4-heaps */
  for (i = 0; i < N; ++i)
    upheap(heap, i + HEAP0);
}

/*****************************************************************************/

/* associate signal watchers to a signal */
typedef struct {
  EV_ATOMIC_T pending;
#if EV_MULTIPLICITY
  EV_P;
#endif
  WL head;
} ANSIG;

static ANSIG signals[EV_NSIG - 1];

/*****************************************************************************/

#if EV_SIGNAL_ENABLE || EV_ASYNC_ENABLE

ecb_noinline ecb_cold static void evpipe_init(EV_P) {
  if (!ev_is_active(&pipe_w)) {
    int fds[2];

#if EV_USE_EVENTFD
    fds[0] = -1;
    fds[1] = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (fds[1] < 0 && errno == EINVAL)
      fds[1] = eventfd(0, 0);

    if (fds[1] < 0)
#endif
    {
      while (pipe(fds))
        ev_syserr("(libev) error creating signal/async pipe");

      fd_intern(fds[0]);
    }

    evpipe[0] = fds[0];

    if (evpipe[1] < 0)
      evpipe[1] = fds[1]; /* first call, set write fd */
    else {
      /* on subsequent calls, do not change evpipe [1] */
      /* so that evpipe_write can always rely on its value */
      /* this branch does not do anything sensible on windows */
      /* so must not be executed on windows */

      dup2(fds[1], evpipe[1]);
      close(fds[1]);
    }

    fd_intern(evpipe[1]);

    ev_io_set(&pipe_w, evpipe[0] < 0 ? evpipe[1] : evpipe[0], EV_READ);
    ev_io_start(EV_A_ & pipe_w);
    ev_unref(EV_A); /* watcher should not keep loop alive */
  }
}

inline_speed void evpipe_write(EV_P_ EV_ATOMIC_T* flag) {
  ECB_MEMORY_FENCE; /* push out the write before this function was called, acquire flag */

  if (ecb_expect_true(*flag))
    return;

  *flag = 1;
  ECB_MEMORY_FENCE_RELEASE; /* make sure flag is visible before the wakeup */

  pipe_write_skipped = 1;

  ECB_MEMORY_FENCE; /* make sure pipe_write_skipped is visible before we check pipe_write_wanted */

  if (pipe_write_wanted) {
    int old_errno;

    pipe_write_skipped = 0;
    ECB_MEMORY_FENCE_RELEASE;

    old_errno = errno; /* save errno because write will clobber it */

#if EV_USE_EVENTFD
    if (evpipe[0] < 0) {
      uint64_t counter = 1;
      write(evpipe[1], &counter, sizeof(uint64_t));
    }
    else
#endif
    {
#ifdef _WIN32
      WSABUF buf;
      DWORD sent;
      buf.buf = (char*)&buf;
      buf.len = 1;
      WSASend(EV_FD_TO_WIN32_HANDLE(evpipe[1]), &buf, 1, &sent, 0, 0, 0);
#else
      write(evpipe[1], &(evpipe[1]), 1);
#endif
    }

    errno = old_errno;
  }
}

/* called whenever the libev signal pipe */
/* got some events (signal, async) */
static void pipecb(EV_P_ ev_io* iow, int revents) {
  int i;

  (void)iow;

  if (revents & EV_READ) {
#if EV_USE_EVENTFD
    if (evpipe[0] < 0) {
      uint64_t counter;
      read(evpipe[1], &counter, sizeof(uint64_t));
    }
    else
#endif
    {
      char dummy[4];
#ifdef _WIN32
      WSABUF buf;
      DWORD recvd;
      DWORD flags = 0;
      buf.buf = dummy;
      buf.len = sizeof(dummy);
      WSARecv(EV_FD_TO_WIN32_HANDLE(evpipe[0]), &buf, 1, &recvd, &flags, 0, 0);
#else
      read(evpipe[0], &dummy, sizeof(dummy));
#endif
    }
  }

  pipe_write_skipped = 0;

  ECB_MEMORY_FENCE; /* push out skipped, acquire flags */

#if EV_SIGNAL_ENABLE
  if (sig_pending) {
    sig_pending = 0;

    ECB_MEMORY_FENCE;

    for (i = EV_NSIG - 1; i--;)
      if (ecb_expect_false(signals[i].pending))
        ev_feed_signal_event(EV_A_ i + 1);
  }
#endif

#if EV_ASYNC_ENABLE
  if (async_pending) {
    async_pending = 0;

    ECB_MEMORY_FENCE;

    for (i = asynccnt; i--;)
      if (asyncs[i]->sent) {
        asyncs[i]->sent = 0;
        ECB_MEMORY_FENCE_RELEASE;
        ev_feed_event(EV_A_ asyncs[i], EV_ASYNC);
      }
  }
#endif
}

/*****************************************************************************/

void ev_feed_signal(int signum) EV_NOEXCEPT {
#if EV_MULTIPLICITY
  EV_P;
  ECB_MEMORY_FENCE_ACQUIRE;
  EV_A = signals[signum - 1].loop;

  if (!EV_A)
    return;
#endif

  signals[signum - 1].pending = 1;
  evpipe_write(EV_A_ & sig_pending);
}

static void ev_sighandler(int signum) {
#ifdef _WIN32
  signal(signum, ev_sighandler);
#endif

  ev_feed_signal(signum);
}

ecb_noinline void ev_feed_signal_event(EV_P_ int signum) EV_NOEXCEPT {
  WL w;

  if (ecb_expect_false(signum <= 0 || signum >= EV_NSIG))
    return;

  --signum;

#if EV_MULTIPLICITY
  /* it is permissible to try to feed a signal to the wrong loop */
  /* or, likely more useful, feeding a signal nobody is waiting for */

  if (ecb_expect_false(signals[signum].loop != EV_A))
    return;
#endif

  signals[signum].pending = 0;
  ECB_MEMORY_FENCE_RELEASE;

  for (w = signals[signum].head; w; w = w->next)
    ev_feed_event(EV_A_(W) w, EV_SIGNAL);
}

#if EV_USE_SIGNALFD
static void sigfdcb(EV_P_ ev_io* iow, int revents) {
  struct signalfd_siginfo si[2], *sip; /* these structs are big */

  (void)iow;
  (void)revents;

  for (;;) {
    ssize_t res;
    char* end;

    res = read(sigfd, si, sizeof(si));
    if (res <= 0)
      break;

    end = (char*)si + res;

    for (sip = si; (char*)sip + (ptrdiff_t)sizeof(*sip) <= end; ++sip)
      ev_feed_signal_event(EV_A_ sip->ssi_signo);

    if (res < (ssize_t)sizeof(si))
      break;
  }
}
#endif

#endif
