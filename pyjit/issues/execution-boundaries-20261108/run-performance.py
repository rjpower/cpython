"""Coordinator: serial comparisons using the unchanged benchmark protocol."""

import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
ROOT = Path(__file__).resolve().parents[2]
EVIDENCE = BASE / 'pyjit-link-wave1/execution-boundaries-20261108'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', choices=['target', 'broad'])
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('registry', ROOT / 'bench/run.py')
    registry = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = registry
    spec.loader.exec_module(registry)
    roster = [item.name for item in registry.BENCHMARKS if item.group == 'kernels']
    assert len(roster) == 29
    cases = ['logging_silent', 'richards', 'spectral_norm'] if args.stage == 'target' else roster
    assert set(cases) <= set(roster)
    controls = {
        'golden': (BASE / 'pyvalue-rc-repr-release',
                   BASE / 'pyjit-link-wave1/interpreter-parity-20261108/comment-cleanup-release-identity.log'),
        'upstream': (BASE / 'upstream-release-devlibs-3a440b6',
                     BASE / 'pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log'),
    }
    env = {k: v for k, v in os.environ.items()
           if not k.startswith('PYTHON') and k != 'LD_PRELOAD'}
    env['UV_CACHE_DIR'] = '/home/power/.cache/uv'
    for role, (control, module_log) in controls.items():
        stem = f'final-{args.stage}-{role}'
        receipt = EVIDENCE / f'{stem}-wrapper.json'
        log = EVIDENCE / f'{stem}-wrapper.log'
        assert not receipt.exists() and not log.exists()
        command = [
            sys.executable, '-B', str(Path(__file__).with_name('measure.py')),
            '--control', str(control), '--control-role', role,
            '--candidate', str(BASE / 'execution-boundaries-release'),
            '--module-log', f'{role}={module_log}',
            '--module-log', f'candidate={EVIDENCE}/boundary-cache-v3-release-identity.log',
            '--output', str(EVIDENCE / stem), '--cases', ','.join(cases),
            '--rounds', '6' if args.stage == 'target' else '2',
        ]
        row = dict(command=command, cwd=str(ROOT.parent))
        receipt.write_text(json.dumps(row, indent=2))
        with log.open('x') as stream:
            result = subprocess.run(command, cwd=ROOT.parent, env=env,
                                    stdout=stream, stderr=subprocess.STDOUT)
        row['returncode'] = result.returncode
        receipt.write_text(json.dumps(row, indent=2))
        print(stem, result.returncode, flush=True)
        if result.returncode:
            raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
