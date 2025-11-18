#!/usr/bin/env python3
import os
import subprocess
import sys
from typing import Dict


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


def main(argv: list[str]) -> int:
  if len(argv) < 3:
    print(f"usage: {argv[0]} <local-bin> <baseline-bin> [tolerance]", file=sys.stderr)
    return 2

  local_bin = argv[1]
  baseline_bin = argv[2]
  tolerance = float(argv[3]) if len(argv) > 3 else DEFAULT_TOLERANCE

  env = os.environ.copy()
  env.setdefault(ITER_ENV, "200000")

  local_result = run_benchmark(local_bin, env)
  baseline_result = run_benchmark(baseline_bin, env)

  local_rate = float(local_result["per_second"])
  baseline_rate = float(baseline_result["per_second"])
  ratio = local_rate / baseline_rate if baseline_rate > 0 else float("inf")

  print(f"local libev v{local_result.get('version', '?')}: {local_rate:.0f} iterations/sec")
  print(f"baseline libev v{baseline_result.get('version', '?')}: {baseline_rate:.0f} iterations/sec")
  print(f"ratio (local/baseline): {ratio:.2f}")

  if ratio < tolerance:
    delta = (1.0 - ratio) * 100.0
    print(
      f"local library is {delta:.1f}% slower than baseline (tolerance {tolerance:.2f})",
      file=sys.stderr,
    )
    return 1

  return 0


if __name__ == "__main__":
  sys.exit(main(sys.argv))
