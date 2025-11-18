import argparse
import pathlib
import subprocess
import sys
from typing import Iterable, Set


def read_symbols(paths: Iterable[pathlib.Path]) -> Set[str]:
    symbols: Set[str] = set()
    for path in paths:
        for line in path.read_text().splitlines():
            name = line.strip()
            if not name or name.startswith('#'):
                continue
            symbols.add(name)
    return symbols


def exported_symbols(nm_path: pathlib.Path, lib_path: pathlib.Path) -> Set[str]:
    # Use the dynamic table for shared objects, otherwise fall back to global symbols.
    flags = ['-g', '--defined-only']
    cmd = [str(nm_path)]
    if lib_path.suffix != '.a':
        cmd.append('-D')
    cmd.extend(flags)
    cmd.append(str(lib_path))

    result = subprocess.run(cmd, check=True, capture_output=True, text=True)

    exports: Set[str] = set()
    for line in result.stdout.splitlines():
        parts = line.split()
        if not parts:
            continue
        name = parts[-1].split('@', 1)[0]  # drop version suffixes such as @@LIBEV_4.0
        if name.startswith(('ev_', 'event_')):
            exports.add(name)
    return exports


def main() -> int:
    parser = argparse.ArgumentParser(description='Validate exported libev symbols match the reference lists.')
    parser.add_argument('nm', type=pathlib.Path, help='Path to the nm executable')
    parser.add_argument('library', type=pathlib.Path, help='Built libev library to inspect')
    parser.add_argument('symbols', nargs='+', type=pathlib.Path, help='Expected symbol list files')
    args = parser.parse_args()

    expected = read_symbols(args.symbols)
    actual = exported_symbols(args.nm, args.library)

    missing = sorted(expected - actual)
    unexpected = sorted(actual - expected)

    if missing or unexpected:
        if missing:
            print('Missing symbols:')
            for name in missing:
                print(f'  {name}')
        if unexpected:
            print('Unexpected symbols:')
            for name in unexpected:
                print(f'  {name}')
        return 1

    print(f'Found {len(actual)} exported symbols; all match reference data.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
