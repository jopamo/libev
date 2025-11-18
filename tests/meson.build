bench_source = files('perf_idle_bench.c')

bench_iterations_env = '200000'
bench_timeout = 60

bench_local = executable(
  'perf_idle_local',
  bench_source,
  include_directories: all_incs,
  dependencies: libev_dep,
  c_args: system_libev_cflags,
  install: false,
)

test(
  'perf-local-smoke',
  bench_local,
  env: {'LIBEV_BENCH_ITERATIONS': bench_iterations_env},
  timeout: bench_timeout,
)

if system_libev_dep.found()
  bench_system = executable(
    'perf_idle_system',
    bench_source,
    dependencies: system_libev_dep,
    install: false,
  )

  python3 = import('python').find_installation('python3')
  test(
    'perf-compare-system',
    python3,
    args: [
      files('perf_compare.py'),
      bench_local.full_path(),
      bench_system.full_path(),
      '0.70',
    ],
    env: {'LIBEV_BENCH_ITERATIONS': bench_iterations_env},
    timeout: bench_timeout,
  )
endif
