# Unit Test Checklist

We need to port coverage expectations from the original libev 4.33 implementation and run them against this fork. Below is the actionable list of unit/functional test modules to author under `tests/` (Meson targets prefixed `unit_*`). Each item includes the behavior to validate and relevant code references for both implementation files and current test gaps.

## Loop Lifecycle & Flags
- [x] **Backend flags & selection** — Assert `ev_supported_backends`, `ev_recommended_backends`, and `ev_embeddable_backends` return the same bitmasks as `libev-4.33`, and that `EVFLAG_NOENV`/`LIBEV_FLAGS` override backend selection (src/ev_api.c:59-225, include/ev.h:484-517). Covered by `tests/unit_backend_flags.c`.
- [x] **Default loop & alternates** — Cover creation/destruction (`ev_default_loop`, `ev_loop_new`, `ev_loop_destroy`) ensuring only the default loop handles signals/children and that cleanup watchers run on destroy (src/ev_api.c:269-425, 595-617). Covered by `tests/unit_loop_lifecycle.c`.
- [x] **`ev_run` control flow** — Validate `EVRUN_NOWAIT`, `EVRUN_ONCE`, `ev_break` cancel/one/all modes, and `loop_count`/`loop_depth` accounting exposed via `EV_FEATURE_API` (src/ev_api.c:879-1104).
- [x] **Refcount & pending hooks** — Exercise `ev_ref`, `ev_unref`, `ev_pending_count`, `ev_set_invoke_pending_cb`, `ev_set_loop_release_cb`, and `ev_set_userdata` to ensure refcounts and callback plumbing match upstream (src/ev_api.c:135-158, 630-657, 1140-1179).
- [x] **Clock maintenance** — Verify `ev_now_update`, `ev_suspend`, `ev_resume`, and large time jumps adjust timers/periodics without duplicate firings via `timers_reschedule`/`periodics_reschedule` (src/ev_api.c:768-876, 1101-1117). Covered by `tests/unit_clock_maintenance.c`.
- [ ] **Time & allocation helpers** — Mirror upstream behavior for `ev_time`, `ev_sleep` (including EINTR loops), `ev_set_allocator`, and `ev_set_syserr_cb` (src/ev_time.c:4-89, src/ev.c:2128-2187).

## Watcher Primitives
- [ ] **Watcher init & priority** — Confirm `ev_init`, `ev_set_cb`, `ev_set_priority`, and `ev_clear_pending` clamp invalid priorities and merge events correctly when `ev_feed_event` fires multiple times (include/ev.h:675-820, src/ev_loop_core.c:71-114).
- [ ] **FD bookkeeping** — Stress `fd_event`, `ev_feed_fd_event`, and `fd_reify` by mutating watchers mid-callback and ensuring reify skips fds with `reify` set (src/ev_loop_core.c:115-221).
- [ ] **I/O watchers** — Add suites for `ev_io_start/stop`: clamping invalid masks, stacking multiple watchers per fd, `EV__IOFDSET` bookkeeping, and `fd_change` propagation (src/ev_api.c:1184-1238, include/ev.h:682-695).
- [ ] **Timers** — Test repeating vs one-shot timers, `ev_timer_again`, `ev_timer_remaining`, and heap ordering from `timers_reify` (src/ev_api.c:1241-1314, 677-707).
- [ ] **Periodics** — Cover `offset`/`interval`, user `reschedule_cb`, `ev_periodic_again`, and timerfd initialization when `EV_USE_TIMERFD` is enabled (src/ev_api.c:1317-1396, src/ev_timerfd.c:1-40).
- [ ] **`ev_once`** — Expand beyond timeout-only to include fd-only and combined cases, ensuring watchers free themselves and revents contain the right mask (src/ev_api.c:2207-2254).

## Specialized Watchers
- [ ] **Signals** — Default-loop-only constraint, `EVFLAG_SIGNALFD` vs `sigaction` fallback, `ev_feed_signal`, and signal mask toggling per watcher (src/ev_api.c:1405-1515, src/ev_loop_core.c:518-620).
- [ ] **Child watchers** — Validate default-loop restriction, `trace` flag handling for `WIFSTOPPED`/`WIFCONTINUED`, and hashing in `child_reap` (src/ev_child.c:1-56, src/ev_api.c:1522-1546, 603-611).
- [ ] **Stat watchers** — Cover `ev_stat_stat`, min interval clamping, inotify path add/remove, ENOENT parent fallback, and fork re-registration (src/ev_api.c:1553-1867).
- [ ] **Idle watchers** — Ensure per-priority slots, `idleall`/`idlecnt` accounting, and delayed callbacks while higher-priority events pending (src/ev_api.c:1870-1910, 657-674).
- [ ] **Prepare/Check** — Verify ordering around backend polling, self-stop behavior, and interaction with manual `time_update` (src/ev_api.c:1913-1978, 903-950).
- [ ] **Fork/Cleanup** — Confirm `ev_fork_start/stop` queues `EV_FORK` once per fork, and cleanup watchers only run during loop destroy without holding refs (src/ev_api.c:2092-2159, 269-315).

## Advanced Integrations & Async
- [ ] **Embed watchers** — Nest loops to exercise `ev_embed_start/stop`, EV_EMBED callbacks vs direct `ev_run`, and automatic restart after forks (src/ev_api.c:1982-2079).
- [ ] **Async watchers** — Multi-thread smoke tests for `ev_async_send`, covering eventfd vs pipe paths, `async_pending` fences, and refcount neutrality (src/ev_api.c:2163-2202, src/ev_loop_core.c:518-575).
- [ ] **Release/acquire callbacks** — Hook `ev_set_loop_release_cb` and assert release/acquire happen around each `backend_poll`/`ev_sleep` path (src/ev_api.c:155-158, src/ev_epoll.c:149-151, src/ev_poll.c:94-110, src/ev_select.c:135-165).
- [ ] **`ev_walk` enumeration** — Confirm it reports the right `type` values (EV_IO, EV_TIMER, EV_STAT, EV_EMBED) while skipping internal watchers (`pipe_w`, `infy_cb`) (src/ev_api.c:2259-2341).
- [ ] **Event feeding & errors** — Ensure `ev_feed_signal_event` rejects foreign loops, `ev_feed_event` handles watchers that free themselves, and `pending_w` placeholders prevent double invokes (src/ev_loop_core.c:581-620, src/ev_api.c:1140-1159).
- [ ] **Collect intervals & userdata** — Assert `ev_set_io_collect_interval`, `ev_set_timeout_collect_interval`, and `ev_set_userdata` alter scheduling latency and callback data as expected (src/ev_api.c:135-149).
- [ ] **Custom invoke flow** — Use `ev_set_invoke_pending_cb` to substitute a shim and verify priority ordering matches `ev_invoke_pending` when restored (src/ev_api.c:151-154, 640-655).

## Backend & Platform Specific
- [ ] **Epoll backend** — Regression tests for `epoll_modify` covering ENOENT/EEXIST/EPERM, `epoll_eperms` permanent readiness, and generation-counter mismatch rebuilding kernel state (src/ev_epoll.c:74-200).
- [ ] **Kqueue backend** — Ensure `kqueue_change` re-adds watchers, `EV_ERROR` handling resubmits or `fd_kill`s, and `kqueue_fork` recreates fds only for original pids (src/ev_kqueue.c:48-200).
- [ ] **Poll/Select fallback** — Validate `pollidxs` compaction, `POLLNVAL`/ENOMEM paths, Windows EINVAL sleep, WSAENOTSOCK remapping, and `FD_SETSIZE` assertions (src/ev_poll.c:52-139, src/ev_select.c:70-199).
- [ ] **Linux advanced backends** — Add suites for linux-aio fallback to epoll when `io_submit` fails, and io_uring SQ/CQ growth, timeout ops, and kernel-version gates (src/ev_linuxaio.c:41-200, src/ev_iouring.c:1-188).
- [ ] **Flag-controlled helpers** — Confirm `EVFLAG_NOINOTIFY`, `EVFLAG_SIGNALFD`, and `EVFLAG_NOTIMERFD` toggle fs/signalfd/timerfd init paths gracefully when syscalls fail (src/ev_api.c:190-220, src/ev_timerfd.c:1-40, src/ev_api.c:1405-1515, 1553-1867).
- [ ] **Platform-specific backends** — Add Solaris `port_getn` regression (src/ev_port.c:92-151) and Windows socket handling/IOCP stubs (src/ev_win32.c:1-200).

## Libevent Compatibility Layer
- [ ] **`event_set`/`event_add`/`event_del`** — Verify IO vs EV_SIGNAL watchers, EV_PERSIST semantics, and timer arm/disarm flow (src/event.c:200-265).
- [ ] **Timer rearm via libevent** — Extend `unit_event_timer_reset` to cover overlapping timers and confirm re-add delays propagate (src/event.c:215-279, tests/unit_event_timer_reset.c:32-52).
- [ ] **`event_active`/`event_pending`** — Ensure manual activation sets `ev_res` and `event_pending` reports revents/timevals appropriately (src/event.c:267-308).
- [ ] **Loop control wrappers** — Test `event_base_loop`, `event_dispatch`, `event_loopexit`, `event_once`, and `event_base_once` glue into `ev_run`/`ev_once` correctly (src/event.c:324-381).
- [ ] **Priority helpers** — Cover `event_priority_set`, `event_priority_init`, `event_base_priority_init`, even though priorities are advisory (src/event.c:310-389).
- [ ] **Metadata getters** — Confirm `event_get_version`, `event_get_method`, and `event_set_log_callback` mirror libevent expectations (src/event.c:40-138).

---
Each checklist entry implies a dedicated `tests/unit_*.c` (or extended existing test) plus a Meson target update in `tests/meson.build`. For each new case we should:
1. Cross-check behavior against the pristine tree under `libev-4.33/` to ensure assertions reflect upstream semantics.
2. Build the fork (`meson setup build && meson compile -C build`).
3. Run the new test via `meson test -C build <test_name> --print-errorlogs` on both the original and forked sources.
4. Document coverage in the eventual PR (reference this checklist).
