"""Run serialized model55 build and qualification checks with durable evidence."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
SOURCE = BASE / 'worktrees/call55-integrated'
EVIDENCE = BASE / 'pyjit-link-wave1/call-model55'
GDB_DISPLAY = 'test.test_gdb.test_pretty_print.PrettyPrintTests.*'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tag')
    parser.add_argument('stage', choices=['build', 'regen', 'focus', 'full'])
    parser.add_argument('tests', nargs='*')
    parser.add_argument('--role', choices=['release', 'debug'])
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()
    manifest = EVIDENCE / f'{args.tag}-{args.stage}-commands.json'
    if manifest.exists():
        raise FileExistsError(manifest)
    env = {k: v for k, v in os.environ.items()
           if not k.startswith('PYTHON') and k != 'LD_PRELOAD'}
    env.update(PYTHONHASHSEED='0', UV_CACHE_DIR='/home/power/.cache/uv')
    rows = []
    roles = ['release'] if args.stage == 'regen' else (
        [args.role] if args.role else ['release', 'debug'])
    for role in roles:
        build = BASE / f'call55-integrated-{role}'
        if args.stage == 'build':
            command = ['make', '-s', '-j10', '-W', str(SOURCE / 'Modules/getbuildinfo.c')]
        elif args.stage == 'regen':
            command = ['make', '-s', 'PYTHON_FOR_REGEN=uv run --no-project python -B',
                       'regen-cases']
        else:
            command = [str(build / 'python'), '-B', '-m', 'test', '-j8',
                       '--ignore', GDB_DISPLAY]
            if args.verbose:
                command.append('-v')
            if args.stage == 'focus':
                if not args.tests:
                    parser.error('focus requires test modules')
                command += args.tests
        log = EVIDENCE / f'{args.tag}-{role}-{args.stage}.log'
        row = dict(role=role, command=command, cwd=str(build), log=str(log),
                   source_head=subprocess.check_output(
                       ['git', '-C', str(SOURCE), 'rev-parse', 'HEAD'], text=True).strip(),
                   source_diff=subprocess.check_output(
                       ['git', '-C', str(SOURCE), 'diff', 'HEAD', '--stat'], text=True),
                   started=time.time())
        rows.append(row)
        manifest.write_text(json.dumps(rows, indent=2))
        if args.stage in ('focus', 'full'):
            probe = subprocess.check_output(
                [command[0], '-S', '-B', '-c',
                 'import _imp,json,sys; print(json.dumps(dict(git=sys._git,paths=sys.path,'
                 'magic=_imp.pyc_magic_number_token & 65535)))'],
                cwd=build, env=env, text=True)
            row['native_preflight'] = json.loads(probe)
            header = (SOURCE / 'Include/internal/pycore_magic_number.h').read_text()
            magic = int(re.search(r'^#define PYC_MAGIC_NUMBER (\d+)$', header, re.M)[1])
            assert row['source_head'].startswith(row['native_preflight']['git'][2])
            assert row['native_preflight']['magic'] == magic
            assert str(SOURCE / 'Lib') in row['native_preflight']['paths']
            manifest.write_text(json.dumps(rows, indent=2))
        with log.open('x') as output:
            result = subprocess.run(command, cwd=build, env=env, stdout=output,
                                    stderr=subprocess.STDOUT)
        row.update(returncode=result.returncode, finished=time.time())
        manifest.write_text(json.dumps(rows, indent=2))
        print(json.dumps(dict(role=role, stage=args.stage,
                              returncode=result.returncode, log=str(log))), flush=True)
        result.check_returncode()


if __name__ == '__main__':
    main()
