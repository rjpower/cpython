"""Serialize the regression repeat and logging hardware diagnostics."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
PACKET = Path(__file__).resolve().parent
EVIDENCE = BASE / 'pyjit-link-wave1/execution-boundaries-20261108'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--perf-only', action='store_true')
    args = parser.parse_args()
    for role in ('golden', 'upstream'):
        status = json.loads((EVIDENCE / f'final-broad-{role}/status.json').read_text())
        assert status['stable_identity'] and not status['failure']
        assert not status['postflight_error']
    commands = [('regression-repeat', [
        sys.executable, '-B', str(PACKET / 'measure.py'),
        '--control', str(BASE / 'pyvalue-rc-repr-release'),
        '--control-role', 'golden',
        '--candidate', str(BASE / 'execution-boundaries-release'),
        '--module-log', 'golden=' + str(BASE / 'pyjit-link-wave1/interpreter-parity-20261108/comment-cleanup-release-identity.log'),
        '--module-log', 'candidate=' + str(EVIDENCE / 'boundary-cache-v3-release-identity.log'),
        '--output', str(EVIDENCE / 'final-regressions-golden'),
        '--cases', 'unpack_sequence,pickle,float,deepcopy_memo,nbody,generators,logging_silent,spectral_norm',
        '--rounds', '4', '--cpu', '15',
    ])]
    if args.perf_only:
        commands = []
    for name, mode, extra in (('perf-stat-v2', 'stat', []),
                              ('perf-record-v2', 'record', []),
                              ('perf-empty-v2', 'stat', ['--empty'])):
        commands.append((name, [
            'taskset', '-c', '15', sys.executable, '-B', str(PACKET / 'perf.py'),
            '--medians', str(EVIDENCE / 'perf-calibration.json'),
            '--upstream', str(BASE / 'upstream-release-devlibs-3a440b6'),
            '--candidate', str(BASE / 'execution-boundaries-release'),
            '--identity-profiles', str(EVIDENCE / 'perf-identities'),
            '--output', str(EVIDENCE / name), '--mode', mode,
            '--case', 'logging_silent', *extra,
        ]))
    commands.append(('perf-summary', [sys.executable, '-B', str(PACKET / 'summarize-perf.py')]))
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    receipt = EVIDENCE / ('perf-v2-commands.json' if args.perf_only
                          else 'final-diagnostics-commands.json')
    assert not receipt.exists()
    rows = []
    for name, command in commands:
        log = EVIDENCE / f'{name}-wrapper.log'
        with log.open('x') as output:
            result = subprocess.run(command, env=env, stdout=output, stderr=subprocess.STDOUT)
        rows.append({'name': name, 'command': command, 'returncode': result.returncode,
                     'log': str(log)})
        receipt.write_text(json.dumps(rows, indent=2))
        print(name, result.returncode, flush=True)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
