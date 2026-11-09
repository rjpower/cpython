"""Compile the library corpus without executing modules; retain rejections."""

import json
import os
from pathlib import Path
import resource
import subprocess
import sys
import time


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
SOURCE = BASE / 'worktrees/execution-boundaries-20261108'
EVIDENCE = BASE / 'pyjit-link-wave1/frame-call-model-20261109'
WORKER = r'''
import json, os, sys
root = sys.argv[1]
result = dict(compiled=0, syntax_errors=[], rejected=[])
for directory, names, files in os.walk(os.path.join(root, 'Lib')):
    names[:] = sorted(name for name in names if name != '__pycache__')
    for name in sorted(files):
        if not name.endswith('.py'):
            continue
        path = os.path.join(directory, name)
        try:
            with open(path, 'rb') as stream:
                text = stream.read()
            compile(text, path, 'exec', dont_inherit=True)
        except SyntaxError as error:
            # The corpus includes intentionally invalid parser fixtures.
            result['syntax_errors'].append(dict(path=path, error=str(error)))
        except (ValueError, OverflowError, SystemError) as error:
            result['rejected'].append(dict(path=path, error=str(error)))
        else:
            result['compiled'] += 1
        if result['compiled'] and result['compiled'] % 500 == 0:
            print('compiled', result['compiled'], file=sys.stderr, flush=True)
print(json.dumps(result))
raise SystemExit(bool(result['rejected']))
'''


def main():
    tag, = sys.argv[1:]
    record = EVIDENCE / f'{tag}-construction.json'
    stdout = EVIDENCE / f'{tag}-construction-stdout.json'
    stderr = EVIDENCE / f'{tag}-construction-stderr.log'
    assert not any(path.exists() for path in (record, stdout, stderr))
    build = BASE / 'execution-boundaries-release'
    command = [str(build / 'python'), '-S', '-B', '-c', WORKER, str(SOURCE)]
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env['PYTHONHASHSEED'] = '0'
    row = dict(command=command, cwd=str(build), started=time.time(),
               source_head=subprocess.check_output(
                   ['git', '-C', str(SOURCE), 'rev-parse', 'HEAD'], text=True).strip(),
               source_diff=subprocess.check_output(
                   ['git', '-C', str(SOURCE), 'diff', '--stat'], text=True))
    record.write_text(json.dumps(row, indent=2))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    try:
        with stdout.open('x') as out, stderr.open('x') as err:
            result = subprocess.run(command, cwd=build, env=env, stdout=out,
                                    stderr=err, timeout=300)
        row['returncode'] = result.returncode
    except subprocess.TimeoutExpired:
        row['failure'] = 'timeout'
        raise
    finally:
        row['finished'] = time.time()
        record.write_text(json.dumps(row, indent=2))
    print(json.dumps({key: row[key] for key in ('returncode', 'finished')}))
    if result.returncode in (0, 1):
        data = json.loads(stdout.read_text())
        print(json.dumps(dict(compiled=data['compiled'],
                              syntax_errors=len(data['syntax_errors']),
                              rejected=data['rejected'])))
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
