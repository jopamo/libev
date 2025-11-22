bench_iterations_env = '200000'
bench_timeout = 60
bench_env = {'LIBEV_BENCH_ITERATIONS': bench_iterations_env}
perf_compare_script = files('perf_compare.py')
bench_specs = [
  {'name': 'idle', 'source': 'perf_idle_bench.c', 'label': 'idle'},
  {'name': 'timer', 'source': 'perf_timer_bench.c', 'label': 'timer'},
  {'name': 'prepare-check', 'source': 'perf_prepare_check_bench.c', 'label': 'prepare-check'},
]
backend_perf_args = []

python3 = import('python').find_installation('python3')
nm_prog = find_program('nm', required: true)

foreach bench : bench_specs
  bench_src = files(bench['source'])

  bench_local = executable(
    'perf_@0@_local'.format(bench['name']),
    bench_src,
    include_directories: all_incs,
    dependencies: libev_dep,
    install: false,
  )

  test(
    'perf-@0@-smoke'.format(bench['name']),
    bench_local,
    env: bench_env,
    timeout: bench_timeout,
  )

  bench_baseline = executable(
    'perf_@0@_baseline'.format(bench['name']),
    bench_src,
    dependencies: orig_libev_dep,
    install: false,
  )

  test(
    'perf-compare-@0@'.format(bench['name']),
    python3,
    args: [
      perf_compare_script,
      bench_local.full_path(),
      bench_baseline.full_path(),
      '--tolerance=0.70',
      '--label=@0@'.format(bench['label']),
    ],
    env: bench_env,
    timeout: bench_timeout,
  )

  backend_perf_args += [
    '--bench',
    bench['label'],
    bench_local.full_path(),
    bench_baseline.full_path(),
  ]
endforeach

test(
  'perf-compare-all-backends',
  python3,
  args: [
    files('run_backend_perf.py'),
    '--lib',
    libev.full_path(),
    '--baseline-lib',
    orig_libev.full_path(),
    '--perf-compare-script',
    perf_compare_script,
  ] + backend_perf_args,
  env: bench_env,
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

# Unit tests: local-only semantics - only test against local implementation
unit_tests = [
  ['unit-timer-basic', 'unit_timer_basic.c'],
  ['unit-io-pipe', 'unit_io_pipe.c'],
  ['unit-ev-once', 'unit_ev_once.c'],
  ['unit-event-timer-reset', 'unit_event_timer_reset.c'],
  ['unit-loop-lifecycle', 'unit_loop_lifecycle.c'],
  ['unit-ev-run-control', 'unit_ev_run_control.c'],
  ['unit-refcount-pending', 'unit_refcount_pending.c'],
  ['unit-clock-maintenance', 'unit_clock_maintenance.c'],
  ['unit-time-alloc-helpers', 'unit_time_alloc_helpers.c'],
  ['unit-watcher-init-priority', 'unit_watcher_init_priority.c'],
]

foreach t : unit_tests
  exe = executable(
    t[0],
    files(t[1]),
    include_directories: all_incs,
    dependencies: libev_dep,
    install: false,
  )

  test(
    t[0],
    exe,
    timeout: 30,
    suite: ['unit'],
  )
endforeach

# Compat tests: compare local vs baseline on common behavior using dlopen
compat_tests = [
  ['compat-version-backends',       'compat_version_backends.c'],
  ['compat-timer-basic',            'compat_timer_basic.c'],
  ['compat-io-watcher',             'compat_io_watcher.c'],
  ['compat-signal-watcher',         'compat_signal_watcher.c'],
  ['compat-child-watcher',          'compat_child_watcher.c'],
  ['compat-priority-merging',       'compat_priority_merging.c'],
  ['compat-event-feed-clear',       'compat_event_feed_clear.c'],
  ['compat-idle-prepare-check',     'compat_idle_prepare_check_order.c'],
  ['compat-syserr-callback',        'compat_syserr_callback.c'],
  ['compat-time-sleep',             'compat_time_sleep.c'],
  ['compat-watcher-init',           'compat_watcher_init.c'],
]

foreach t : compat_tests
  exe = executable(
    t[0],
    files(t[1]),
    include_directories: all_incs,
    dependencies: libev_dep,
    install: false,
  )

  test(
    t[0],
    exe,
    env: {'LIBEV_BASELINE_LIB': orig_libev.full_path()},
    timeout: 30,
    suite: ['compat'],
  )
endforeach

backend_flags_test = executable(
  'unit-backend-flags',
  files('unit_backend_flags.c'),
  include_directories: all_incs,
  dependencies: libev_dep,
  install: false,
)

test(
  'unit-backend-flags',
  backend_flags_test,
  env: {'LIBEV_BASELINE_LIB': orig_libev.full_path()},
  timeout: 30,
  suite: ['compat'],
)
