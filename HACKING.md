# Repository Guidelines

## Project Structure & Module Organization
- `src/`: core event loop, backends, and platform conditionals; avoid leaking internal helpers outside this tree.
- `include/`: public headers exported to consumers; keep API changes documented and guarded by feature macros from `config.h`.
- `tests/`: Meson-driven unit and perf cases (`unit_*`, `perf_*`); extend `tests/meson.build` when adding coverage.
- `build/`: generated Meson/Ninja artifacts; safe to remove when reconfiguring.
- Docs and meta: `ev.pod`/`ev.3` for user-facing docs, `Changes` for release notes, `TODO` for backlog, `Symbols.*` for exported symbol lists.
- `libev*/`: original implementation used for comparison.

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
