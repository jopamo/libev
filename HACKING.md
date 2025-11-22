# Repository Guidelines

## Project layout

- `src/`
  - Core event loop implementation and backend drivers
  - Files like `ev_loop_core.c`, `ev_epoll.c`, `ev_kqueue.c`, `ev_iouring.c`, `ev_poll.c`,
    `ev_select.c`, `ev_port.c`, `ev_linuxaio.c`, `ev_win32.c`, `ev_timerfd.c` implement
    backend-specific logic
  - `ev.c` / `event.c` provide the main integration points and glue for the public API
  - `ev_vars.h` and `ev_wrap.h` are internal headers; do not install or include them from outside `src/`

- `include/`
  - Installed, public headers:
    - `ev.h` – primary libev-style API
    - `ev++.h` – C++ convenience wrappers
    - `event.h` – libevent compatibility API layer
  - All public surface changes are gated by macros from `config.h` and documented in these headers

- `tests/`
  - Meson-driven test suite
  - `unit_*.c` – focused unit tests for API behavior and invariants, e.g.:
    - `unit_backend_flags.c` – backend flag reporting
    - `unit_clock_maintenance.c` – timekeeping and drift handling
    - `unit_ev_once.c`, `unit_ev_run_control.c`, `unit_loop_lifecycle.c` – loop lifecycle and control paths
    - `unit_io_pipe.c` – I/O watcher behavior on pipes
    - `unit_refcount_pending.c` – watcher refcount and pending queue semantics
    - `unit_time_alloc_helpers.c` – time helper allocation and utility routines
    - `unit_timer_basic.c` – basic timer semantics
  - `perf_*.c` and perf scripts:
    - `perf_timer_bench.c`, `perf_idle_bench.c`, `perf_prepare_check_bench.c` plus
      `perf_bench_common.h`, `perf_compare.py`, `run_backend_perf.py`
    - Used to compare backend latency, idle behavior, and cross-backend performance
  - `validate_exported_symbols.py` checks that public ABI matches the `Symbols.*` lists
  - New tests must be wired into `tests/meson.build`

- `build/`
  - Not tracked in git
  - Holds Meson/Ninja build artifacts and can be removed at any time when reconfiguring

- Documentation and meta
  - `ev.pod` and `ev.3`
    - User-facing documentation and manual page for the event API
  - `Changes`
    - Human-readable release notes in chronological order
  - `TODO`
    - Backlog for future work, cleanups, and potential new backends or features
  - `HACKING.md`
    - Contributor notes, coding style, and guidelines for changing backends or core loop behavior
  - `Symbols.ev`, `Symbols.event`
    - Exported symbol lists used to keep ABI stable and validate the public interface

- `libev-4.33/`
  - Unmodified reference implementation of upstream libev 4.33
  - Used for:
    - Behavioral comparison when changing internals or semantics in `src/`
    - Tracking divergences and preparing migration notes
  - Not installed and not part of the public API

## Build, Test, and Development Commands
- Configure (default prefix): `meson setup build`.
- Configure with custom prefix or options: `meson setup build -Dprefix=/usr/local`.
- Build: `meson compile -C build` (gnu99, warning_level=2).
- Run all tests: `meson test -C build --print-errorlogs`.
- Run a single test: `meson test -C build unit_timer_basic`.
- Optional install: `meson install -C build` (may require elevated permissions).

## Coding Style & Naming Conventions
- Language: C (gnu99); keep warnings clean at the configured `warning_level=2`.
- Indentation: spaces, 2-space indents in the existing codebase; wrap long lines thoughtfully around macros.
- Public API uses the `ev_`/`EV_` prefixes; internal helpers stay `static` and avoid namespace collisions.
- Prefer feature-detected branches (`EV_USE_*`/`HAVE_*`) over platform-specific ifdefs; keep fallback paths working.
- Maintain minimal dependencies; avoid introducing external libraries without discussion.

## Testing Guidelines
- Add or update unit tests in `tests/` alongside new functionality; exercise edge cases (fork handling, time jumps, backend availability).
- Benchmarks (`perf_*`) are optional but useful for regression checks; do not gate CI behavior on them.
- Keep tests deterministic; mock or guard platform-dependent code paths so they pass on Linux/macOS/BSD.
- Always run `meson test -C build` before submitting; include `--print-errorlogs` output when reporting failures.

## Commit & Pull Request Guidelines
- Commits: concise, imperative subject lines (e.g., “Add perf idle bench”); group logical changes and keep noise low.
- PRs: describe behavior changes, affected platforms/backends, and testing performed (`meson test -C build` plus any perf checks).
- Link issues or mailing-list threads when relevant; call out API or ABI changes and update `Changes`/docs accordingly.
- Include repro snippets for bug fixes (e.g., watcher setup sequence) and note any new build options or env requirements.
