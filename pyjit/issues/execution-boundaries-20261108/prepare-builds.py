"""Reuse configured build products without modifying the golden controls."""

import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
SOURCE = BASE / 'worktrees/execution-boundaries-20261108'
EVIDENCE = BASE / 'pyjit-link-wave1/execution-boundaries-20261108'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--retry', action='store_true')
    args = parser.parse_args()
    tag = 'prepare-retry' if args.retry else 'prepare'
    receipt = EVIDENCE / f'{tag}-builds.json'
    assert not receipt.exists(), receipt
    env = {k: v for k, v in os.environ.items()
           if not k.startswith('PYTHON') and k != 'LD_PRELOAD'}
    env['UV_CACHE_DIR'] = '/home/power/.cache/uv'
    env['PKG_CONFIG_SYSROOT_DIR'] = str(BASE / 'devlibs/root')
    rows = []
    for role in ('release', 'debug', 'stats'):
        original = BASE / f'pyvalue-rc-repr-{role}'
        build = BASE / f'execution-boundaries-{role}'
        if build.exists():
            assert args.retry, build
            assert str(SOURCE) in (build / 'Makefile').read_text(), build
        else:
            shutil.copytree(original, build, symlinks=True)
        (build / 'compile_commands.json').unlink(missing_ok=True)
        config = subprocess.check_output(
            [str(original / 'config.status'), '--config'], text=True)
        commands = [
            [str(SOURCE / 'configure'), *shlex.split(config)],
            ['make', '-s', '-j10', '-W', str(SOURCE / 'Modules/getbuildinfo.c')],
        ]
        for index, command in enumerate(commands):
            log = EVIDENCE / f'{tag}-{role}-{index}.log'
            row = dict(role=role, command=command, cwd=str(build),
                       log=str(log), started=time.time(),
                       pkg_config_sysroot=env['PKG_CONFIG_SYSROOT_DIR'])
            rows.append(row)
            receipt.write_text(json.dumps(rows, indent=2) + '\n')
            with log.open('x') as stream:
                result = subprocess.run(command, cwd=build, env=env,
                                        stdout=stream, stderr=subprocess.STDOUT)
            row.update(returncode=result.returncode, finished=time.time())
            receipt.write_text(json.dumps(rows, indent=2) + '\n')
            print(json.dumps(row), flush=True)
            if result.returncode:
                raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
