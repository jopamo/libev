bench_source = files('perf_idle_bench.c')

bench_iterations_env = '200000'
bench_timeout = 60

python3 = import('python').find_installation('python3')
nm_prog = find_program('nm', required: true)

bench_local = executable(
  'perf_idle_local',
  bench_source,
  include_directories: all_incs,
  dependencies: libev_dep,
  install: false,
)

test(
  'perf-local-smoke',
  bench_local,
  env: {'LIBEV_BENCH_ITERATIONS': bench_iterations_env},
  timeout: bench_timeout,
)

test(
  'symbols-match-reference',
  python3,
  args: [
    files('validate_exported_symbols.py'),
    nm_prog.full_path(),
    libev.full_path(),
    join_paths(meson.project_source_root(), 'Symbols.ev'),
    join_paths(meson.project_source_root(), 'Symbols.event'),
  ],
  timeout: 30,
)

# Unit-style functional checks
foreach t : [
  ['unit-timer-basic', 'unit_timer_basic.c'],
  ['unit-io-pipe', 'unit_io_pipe.c'],
  ['unit-ev-once', 'unit_ev_once.c'],
]
  exe = executable(
    t[0],
    files(t[1]),
    include_directories: all_incs,
    dependencies: libev_dep,
    install: false,
  )

  test(t[0], exe, timeout: 30)
endforeach

bench_baseline = executable(
  'perf_idle_baseline',
  bench_source,
  dependencies: orig_libev_dep,
  install: false,
)

test(
  'perf-compare-baseline',
  python3,
  args: [
    files('perf_compare.py'),
    bench_local.full_path(),
    bench_baseline.full_path(),
    '0.70',
  ],
  env: {'LIBEV_BENCH_ITERATIONS': bench_iterations_env},
  timeout: bench_timeout,
)
