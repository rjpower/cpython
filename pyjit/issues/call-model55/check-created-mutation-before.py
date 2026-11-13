"""Record the new immutable-code regression on the pre-fix candidate."""

import hashlib
import json
import os
from pathlib import Path
import subprocess
import time


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/call-model55'
SOURCE = BASE / 'worktrees/call55-integrated'


def main():
    manifest = EVIDENCE / 'm3-created-mutation-before-commands.json'
    if manifest.exists():
        raise FileExistsError(manifest)
    fixture_head = 'df299bc17c6'
    content = subprocess.check_output([
        'git', '-C', str(SOURCE), 'show',
        fixture_head + ':Lib/test/test_pyvalue_api.py'])
    fixture = EVIDENCE / 'created-mutation-fixture.py'
    with fixture.open('xb') as output:
        output.write(content)
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env['PYTHONHASHSEED'] = '0'
    rows = []
    for role in ('release', 'debug'):
        build = BASE / f'call55-integrated-{role}'
        command = [str(build / 'python'), '-B', str(fixture),
                   'ProviderHandoffTests.test_created_generator_uses_executing_code_after_hook_mutation',
                   '-v']
        row = dict(role=role, command=command, cwd=str(build),
                   fixture_commit=fixture_head,
                   fixture_sha256=hashlib.sha256(content).hexdigest(),
                   source_head=subprocess.check_output(
                       ['git', '-C', str(SOURCE), 'rev-parse', 'HEAD'], text=True).strip(),
                   started=time.time())
        rows.append(row)
        manifest.write_text(json.dumps(rows, indent=2))
        log = EVIDENCE / f'm3-created-mutation-before-{role}.log'
        with log.open('x') as output:
            result = subprocess.run(command, cwd=build, env=env,
                                    stdout=output, stderr=subprocess.STDOUT, timeout=60)
        row.update(returncode=result.returncode, log=str(log), finished=time.time())
        manifest.write_text(json.dumps(rows, indent=2))
        print(json.dumps(row), flush=True)


if __name__ == '__main__':
    main()
