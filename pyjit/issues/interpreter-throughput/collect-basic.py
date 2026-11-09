"""Run the existing kernel timing harness at a frozen integration point."""

import argparse
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[2]
BUILDS = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
CASES = ('deltablue', 'logging_silent', 'comprehensions', 'richards',
         'nbody', 'float', 'generators')


def snapshot(build):
    python = build / 'python'
    notes = subprocess.check_output(['readelf', '-n', str(python)], text=True)
    source_line = next(line for line in (build / 'Makefile').read_text().splitlines()
                       if line.startswith('srcdir='))
    source = (build / source_line.split('=', 1)[1].strip()).resolve()
    return {'build': str(build), 'source': str(source), 'elf_notes': notes,
            'head': subprocess.check_output(['git', '-C', str(source),
                                            'rev-parse', 'HEAD'], text=True).strip(),
            'status': subprocess.check_output(['git', '-C', str(source), 'status',
                                              '--porcelain=v1'], text=True)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    builds = {'upstream': BUILDS / 'upstream-release-devlibs-3a440b6',
              'candidate': BUILDS / 'pyvalue-rc-repr-release'}
    before = {role: snapshot(build) for role, build in builds.items()}
    (args.output / 'identity-before.json').write_text(json.dumps(before, indent=2))
    journal = []
    values = {role: {case: [] for case in CASES} for role in builds}
    deadline = time.monotonic() + 900
    try:
        for round_number in range(1, 4):
            roles = tuple(builds) if round_number % 2 else tuple(reversed(builds))
            for role in roles:
                while os.getloadavg()[0] >= 3:
                    if time.monotonic() >= deadline:
                        raise RuntimeError('quiet-load deadline expired')
                    time.sleep(5)
                stem = f'r{round_number}-{role}'
                output = args.output / f'{stem}.json'
                command = [sys.executable, str(ROOT / 'bench/run.py'),
                           '--build', str(builds[role]), '--label', role,
                           '--metric', 'time', '--only', ','.join(CASES),
                           '--time-runs', '7', '--max-load', '4', '-o', str(output)]
                result = subprocess.run(command, capture_output=True, text=True,
                                        timeout=180)
                (args.output / f'{stem}-stdout.txt').write_text(result.stdout)
                (args.output / f'{stem}-stderr.txt').write_text(result.stderr)
                journal.append({'command': command, 'returncode': result.returncode,
                                'load_after': os.getloadavg()})
                if result.returncode:
                    raise RuntimeError(f'{stem} exited {result.returncode}')
                table = json.loads(output.read_text())['results']['time']
                for case in CASES:
                    values[role][case].append(table[case]['value'])
                print(stem, 'complete', flush=True)
        (args.output / 'round-medians.json').write_text(json.dumps(values, indent=2))
        summary = {case: {role: statistics.median(rows[case])
                          for role, rows in values.items()} for case in CASES}
        (args.output / 'summary.json').write_text(json.dumps(summary, indent=2))
    finally:
        after = {role: snapshot(build) for role, build in builds.items()}
        (args.output / 'identity-after.json').write_text(json.dumps(after, indent=2))
        (args.output / 'commands.json').write_text(json.dumps(journal, indent=2))
    if before != after:
        raise RuntimeError('source/executable identity changed during timing')


if __name__ == '__main__':
    main()
