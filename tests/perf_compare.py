#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys
from typing import Dict, List


ITER_ENV = "LIBEV_BENCH_ITERATIONS"
DEFAULT_TOLERANCE = 0.7  # local library must be at least this fraction of baseline performance


def parse_result_line(line: str) -> Dict[str, str]:
  tokens = line.strip().split()
  result: Dict[str, str] = {}
  for token in tokens:
    if "=" in token:
      key, value = token.split("=", 1)
      result[key] = value

  if "per_second" not in result:
    raise ValueError(f"missing per_second in output: {line.strip()}")

  return result


def run_benchmark(path: str, env: Dict[str, str]) -> Dict[str, str]:
  proc = subprocess.run(
    [path],
    capture_output=True,
    text=True,
    env=env,
    check=False,
  )

  if proc.returncode != 0:
    raise RuntimeError(f"{path} exited with {proc.returncode}: {proc.stderr.strip()}")

  return parse_result_line(proc.stdout)


def parse_args(argv: List[str]) -> argparse.Namespace:
  parser = argparse.ArgumentParser(description="Compare libev benchmark performance.")
  parser.add_argument("local_bin", help="Path to the benchmark built against the local libev.")
  parser.add_argument("baseline_bin", help="Path to the benchmark built against the baseline libev.")
  parser.add_argument(
    "legacy_tolerance",
    nargs="?",
    help=argparse.SUPPRESS,
  )
  parser.add_argument(
    "--tolerance",
    type=float,
    default=None,
    help="Minimum acceptable local/baseline ratio (defaults to 0.70 or the legacy positional value).",
  )
  parser.add_argument(
    "--label",
    default=None,
    help="Scenario label included in the output to aid debugging.",
  )

  args = parser.parse_args(argv[1:])

  if args.tolerance is not None and args.legacy_tolerance is not None:
    parser.error("Specify tolerance either positionally or with --tolerance, not both.")

  if args.tolerance is not None:
    tolerance = args.tolerance
  elif args.legacy_tolerance is not None:
    tolerance = float(args.legacy_tolerance)
  else:
    tolerance = DEFAULT_TOLERANCE

  args.tolerance = tolerance
  return args


def main(argv: list[str]) -> int:
  args = parse_args(argv)

  env = os.environ.copy()
  env.setdefault(ITER_ENV, "200000")

  local_result = run_benchmark(args.local_bin, env)
  baseline_result = run_benchmark(args.baseline_bin, env)

  local_rate = float(local_result["per_second"])
  baseline_rate = float(baseline_result["per_second"])
  ratio = local_rate / baseline_rate if baseline_rate > 0 else float("inf")

  label_prefix = f"[{args.label}] " if args.label else ""

  print(f"{label_prefix}local libev v{local_result.get('version', '?')}: {local_rate:.0f} iterations/sec")
  print(f"{label_prefix}baseline libev v{baseline_result.get('version', '?')}: {baseline_rate:.0f} iterations/sec")
  print(f"{label_prefix}ratio (local/baseline): {ratio:.2f}")

  if ratio < args.tolerance:
    delta = (1.0 - ratio) * 100.0
    print(
      f"{label_prefix}local library is {delta:.1f}% slower than baseline (tolerance {args.tolerance:.2f})",
      file=sys.stderr,
    )
    return 1

  return 0


if __name__ == "__main__":
  sys.exit(main(sys.argv))
