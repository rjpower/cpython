"""Check ordinary admission using the candidate's five bootstrap images."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import time

BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/call-model55'
IMAGES = (
    ('builtins', 'Lib/_pybuiltins.py'),
    ('getpath', 'Modules/getpath.py'),
    ('importlib._bootstrap', 'Lib/importlib/_bootstrap.py'),
    ('importlib._bootstrap_external', 'Lib/importlib/_bootstrap_external.py'),
    ('zipimport', 'Lib/zipimport.py'),
)


# Compile diverse normal source before paying for another complete build.
# This does not execute the modules or replace their behavioral tests.
ADMISSION_PATHS = (
    'site.py', 'shutil.py', 'os.py', '_collections_abc.py', 'contextlib.py', 'functools.py',
    'threading.py', 'weakref.py', 'warnings.py', 'types.py', 'inspect.py',
    'enum.py', 'dataclasses.py', 'argparse.py', 'typing.py',
    'collections/__init__.py', 'logging/__init__.py', 'asyncio/tasks.py',
    'asyncio/base_events.py', 'unittest/case.py', 'pathlib/__init__.py',
    'test/test_ir_call_roles.py', 'test/test_ir_unpack_transfer.py',
    'test/test_ir_send_transfer.py', 'test/test_frame.py',
    'test/test_generators.py', 'test/test_coroutines.py',
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tag')
    parser.add_argument('--admission-matrix', action='store_true')
    parser.add_argument('--variant', choices=['integrated', 'bitmap'],
                        default='integrated')
    args = parser.parse_args()
    source = BASE / f'worktrees/call55-{args.variant}'
    build = BASE / f'call55-{args.variant}-release'
    manifest = EVIDENCE / f'{args.tag}-freezer-commands.json'
    if manifest.exists():
        raise FileExistsError(manifest)
    head = subprocess.check_output(
        ['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    diff = subprocess.check_output(
        ['git', '-C', str(source), 'status', '--porcelain'], text=True)
    if diff:
        raise RuntimeError('commit the candidate before freezer checks')
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env['PYTHONHASHSEED'] = '0'
    rows = []

    def run(name, command):
        log = EVIDENCE / f'{args.tag}-freezer-{name}.log'
        row = dict(name=name, command=command, cwd=str(build),
                   source_head=head, log=str(log), started=time.time())
        rows.append(row)
        manifest.write_text(json.dumps(rows, indent=2))
        with log.open('x') as output:
            result = subprocess.run(command, cwd=build, env=env,
                                    stdout=output, stderr=subprocess.STDOUT)
        row.update(returncode=result.returncode, finished=time.time())
        manifest.write_text(json.dumps(rows, indent=2))
        print(json.dumps(dict(name=name, returncode=result.returncode,
                              log=str(log))), flush=True)
        return result.returncode

    code = run('build', ['make', '-s', '-j10', 'Programs/_freeze_module'])
    if code:
        raise SystemExit(1)
    failed = False
    images = list(IMAGES)
    if args.admission_matrix:
        images += [(f'admission-{index}', f'Lib/{path}')
                   for index, path in enumerate(ADMISSION_PATHS)]
    for name, path in images:
        output = EVIDENCE / f'{args.tag}-{name}-probe.h'
        if output.exists():
            raise FileExistsError(output)
        code = run(name, ['./Programs/_freeze_module', name,
                          str(source / path), str(output)])
        failed |= code != 0
    after = subprocess.check_output(
        ['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    if after != head or subprocess.check_output(
            ['git', '-C', str(source), 'status', '--porcelain']):
        raise RuntimeError('candidate changed during freezer checks')
    raise SystemExit(int(failed))


if __name__ == '__main__':
    main()
