#!/usr/bin/env python3

import argparse
import ctypes
import os
import subprocess
import sys
from typing import List, Sequence, Set, Tuple

EVFLAG_NOENV = 0x01000000
BACKEND_PROBE_TIMEOUT = 5.0
BACKENDS: Sequence[Tuple[str, int]] = (
    ("select", 0x01),
    ("poll", 0x02),
    ("epoll", 0x04),
    ("kqueue", 0x08),
    ("devpoll", 0x10),
    ("port", 0x20),
    ("linuxaio", 0x40),
    ("iouring", 0x80),
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run perf_compare for every available libev backend."
    )
    parser.add_argument("--lib", required=True, help="Path to libev shared library.")
    parser.add_argument(
        "--baseline-lib",
        help="Optional path to the baseline libev shared library used by the perf benchmarks.",
    )
    parser.add_argument(
        "--perf-compare-script",
        required=True,
        help="Path to the perf_compare.py helper.",
    )
    parser.add_argument(
        "--bench",
        nargs=3,
        action="append",
        metavar=("LABEL", "LOCAL_BIN", "BASELINE_BIN"),
        help="Benchmark tuple (label, local binary, baseline binary).",
    )
    parser.add_argument(
        "--tolerance",
        default="0.70",
        help="Tolerance forwarded to perf_compare.py (default: 0.70).",
    )
    args = parser.parse_args()
    if not args.bench:
        parser.error("at least one --bench entry is required")
    return args


def probe_backend(lib_path: str, flag: int) -> bool:
    helper = f"""
import ctypes
import sys
EVFLAG_NOENV = {EVFLAG_NOENV}
lib = ctypes.CDLL({lib_path!r})
lib.ev_loop_new.argtypes = [ctypes.c_uint]
lib.ev_loop_new.restype = ctypes.c_void_p
lib.ev_loop_destroy.argtypes = [ctypes.c_void_p]
lib.ev_loop_destroy.restype = None
lib.ev_backend.argtypes = [ctypes.c_void_p]
lib.ev_backend.restype = ctypes.c_uint
loop = lib.ev_loop_new({flag} | EVFLAG_NOENV)
if not loop:
    sys.exit(1)
backend = lib.ev_backend(loop)
lib.ev_loop_destroy(loop)
if backend & {flag}:
    sys.exit(0)
sys.exit(1)
"""
    try:
        subprocess.run(
            [sys.executable, "-c", helper], check=True, timeout=BACKEND_PROBE_TIMEOUT
        )
    except subprocess.TimeoutExpired:
        return False
    except subprocess.CalledProcessError:
        return False
    return True


def discover_available_backends(lib_path: str) -> List[Tuple[str, int]]:
    lib = ctypes.CDLL(lib_path)
    lib.ev_supported_backends.restype = ctypes.c_uint
    supported_mask = lib.ev_supported_backends()

    available: List[Tuple[str, int]] = []
    for name, flag in BACKENDS:
        if not (supported_mask & flag):
            continue
        if probe_backend(lib_path, flag):
            available.append((name, flag))
        else:
            print(
                f"[{os.path.basename(lib_path)}] skipping backend {name} (probe failed)",
                flush=True,
            )
    return available


def run_perf_compare(
    backend_name: str,
    flag: int,
    bench_label: str,
    local_bin: str,
    baseline_bin: str,
    perf_compare_script: str,
    tolerance: str,
) -> None:
    label = f"{bench_label}-{backend_name}"
    cmd = [
        sys.executable,
        perf_compare_script,
        local_bin,
        baseline_bin,
        f"--tolerance={tolerance}",
        f"--label={label}",
    ]
    env = os.environ.copy()
    env["LIBEV_FLAGS"] = str(flag)
    print(f"[backend {backend_name}] running {bench_label}...", flush=True)
    subprocess.run(cmd, env=env, check=True)


def main() -> None:
    args = parse_args()
    local_backends = discover_available_backends(args.lib)
    available_backends = list(local_backends)

    if args.baseline_lib:
        baseline_backends = discover_available_backends(args.baseline_lib)
        baseline_flags: Set[int] = {flag for _, flag in baseline_backends}
        available_backends = [
            (name, flag) for name, flag in local_backends if flag in baseline_flags
        ]
        missing = [name for name, flag in local_backends if flag not in baseline_flags]
        if missing:
            print(
                "Skipping backends unavailable in baseline: " + ", ".join(missing),
                flush=True,
            )

    if not available_backends:
        print("No libev backends available; nothing to run.")
        return

    print(
        "Available backends: "
        + ", ".join(name for name, _ in available_backends),
        flush=True,
    )

    for backend_name, flag in available_backends:
        for bench_label, local_bin, baseline_bin in args.bench:
            run_perf_compare(
                backend_name,
                flag,
                bench_label,
                local_bin,
                baseline_bin,
                args.perf_compare_script,
                args.tolerance,
            )


if __name__ == "__main__":
    main()
