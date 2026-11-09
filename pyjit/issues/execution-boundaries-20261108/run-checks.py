"""Coordinator: serialized native checks with durable commands and logs."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/execution-boundaries-20261108'
SOURCE = BASE / 'worktrees/execution-boundaries-20261108'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('tag')
    parser.add_argument('stage', choices=['build', 'focus', 'full', 'stats', 'identity', 'regen', 'cdb', 'lint', 'stats-clean'])
    parser.add_argument('tests', nargs='*')
    parser.add_argument('--verbose', action='store_true')
    parser.add_argument('--role', choices=['release', 'debug'])
    parser.add_argument('--seed', type=int)
    args = parser.parse_args()
    path = EVIDENCE / f'{args.tag}-{args.stage}-checks.json'
    assert not path.exists(), path
    env = {k: v for k, v in os.environ.items() if not k.startswith('PYTHON') and k != 'LD_PRELOAD'}
    env.update(PYTHONHASHSEED='0', UV_CACHE_DIR='/home/power/.cache/uv')
    rows = []
    roles = (args.role,) if args.role else ('release', 'debug')

    def run(role, kind, command):
        build = BASE / f'execution-boundaries-{role}'
        log = EVIDENCE / f'{args.tag}-{role}-{kind}.log'
        assert not log.exists(), log
        row = dict(role=role, kind=kind, command=command, cwd=str(build),
                   log=str(log), started=time.time(),
                   source_head=subprocess.check_output(['git', '-C', str(SOURCE), 'rev-parse', 'HEAD'], text=True).strip(),
                   source_diff=subprocess.check_output(['git', '-C', str(SOURCE), 'diff', 'HEAD', '--stat'], text=True))
        rows.append(row)
        path.write_text(json.dumps(rows, indent=2))
        if command[0].endswith('/python'):
            assert Path(command[0]) == build / 'python', command
            probe = subprocess.check_output(
                [command[0], '-S', '-B', '-c',
                 'import _imp,json,sys; print(json.dumps(dict(git=sys._git,paths=sys.path,magic=_imp.pyc_magic_number_token & 65535)))'],
                cwd=build, env=env, text=True)
            row['native_preflight'] = json.loads(probe)
            header = (SOURCE / 'Include/internal/pycore_magic_number.h').read_text()
            row['expected_magic'] = int(re.search(
                r'^#define PYC_MAGIC_NUMBER (\d+)$', header, re.M)[1])
            path.write_text(json.dumps(rows, indent=2))
            assert row['source_head'].startswith(row['native_preflight']['git'][2])
            assert row['native_preflight']['magic'] == row['expected_magic'], row
            assert any(Path(p).resolve() == SOURCE / 'Lib'
                       for p in row['native_preflight']['paths'])
        with log.open('w') as stream:
            result = subprocess.run(command, cwd=build, env=env, stdout=stream, stderr=subprocess.STDOUT)
        row.update(returncode=result.returncode, finished=time.time())
        path.write_text(json.dumps(rows, indent=2))
        print(json.dumps({k: row[k] for k in ['role', 'kind', 'returncode', 'log']}), flush=True)
        if result.returncode:
            raise SystemExit(result.returncode)

    if args.stage == 'regen':
        run('release', 'regen', ['make', '-s', 'regen-cases'])
    elif args.stage in ('build', 'identity'):
        for role in roles:
            run(role, args.stage, ['make', '-s', '-j10', '-W', str(SOURCE / 'Modules/getbuildinfo.c')])
    elif args.stage in ('stats', 'stats-clean'):
        command = ['make', '-s', '-j10', '-W', str(SOURCE / 'Modules/getbuildinfo.c')]
        if args.stage == 'stats-clean':
            # Restoring source mtimes preserves release objects, but stats
            # objects still contain the retired diagnostic layout.
            command += ['-W', str(SOURCE / 'Include/cpython/pystats.h')]
        run('stats', 'build', command)
        run('stats', 'smoke', [str(BASE / 'execution-boundaries-stats/python'), '-B', '-m', 'test', '-j6', 'test_ir', 'test_ir_contract'])
    elif args.stage == 'cdb':
        run('release', 'cdb', ['make', '-s', '-j10',
                             'PYTHON_FOR_REGEN=uv run --no-project python -B',
                             'compile_commands.json'])
        run('release', 'relink', ['make', '-s', '-j10'])
    elif args.stage == 'lint':
        run('release', 'lint', ['uv', 'run', '--no-project', 'python', '-B', str(SOURCE / 'Tools/build/check_value_api.py'), '--cdb', 'compile_commands.json', '--cc', 'gcc', '--cxx', '/usr/bin/g++', '--jobs', '10'])
    else:
        for role in roles:
            command = [str(BASE / f'execution-boundaries-{role}/python'), '-B', '-m', 'test', '-j8']
            if args.verbose:
                command.append('-v')
            if args.seed is not None:
                command += ['--randseed', str(args.seed)]
            if args.stage == 'focus':
                assert args.tests
                command += args.tests
            run(role, args.stage, command)


if __name__ == '__main__':
    main()
