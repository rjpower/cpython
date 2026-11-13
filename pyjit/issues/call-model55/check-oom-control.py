"""Record unchanged dict OOM checks on the immutable selected54 builds."""

import json
import os
from pathlib import Path
import subprocess
import time


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
SOURCE = BASE / 'worktrees/frame54-cold-construction'
EVIDENCE = BASE / 'pyjit-link-wave1/call-model55'


def main():
    manifest = EVIDENCE / 'm3-oom-selected54-commands.json'
    if manifest.exists():
        raise FileExistsError(manifest)
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env['PYTHONHASHSEED'] = '0'
    rows = []
    for role in ('release', 'debug'):
        build = BASE / f'frame54-cold-construction-{role}'
        command = [str(build / 'python'), '-B', '-m', 'test', '-j1', '-v',
                   '-m', '*test_item_iterator_oom*', 'test_dict']
        row = dict(role=role, cwd=str(build), command=command,
                   source_head=subprocess.check_output(
                       ['git', '-C', str(SOURCE), 'rev-parse', 'HEAD'],
                       text=True).strip(), started=time.time())
        rows.append(row)
        manifest.write_text(json.dumps(rows, indent=2))
        log = EVIDENCE / f'm3-oom-selected54-{role}.log'
        with log.open('x') as output:
            result = subprocess.run(command, cwd=build, env=env,
                                    stdout=output, stderr=subprocess.STDOUT)
        row.update(returncode=result.returncode, finished=time.time(), log=str(log))
        manifest.write_text(json.dumps(rows, indent=2))
        print(json.dumps(row), flush=True)


if __name__ == '__main__':
    main()
