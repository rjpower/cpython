"""Build a task-owned frozen control with the existing release configuration."""

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/deltablue-call-model-20261110'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('name')
    parser.add_argument('commit')
    args = parser.parse_args()
    assert re.fullmatch(r'deltablue-[a-z0-9-]+', args.name)
    assert re.fullmatch(r'[0-9a-f]{40}', args.commit)
    source = BASE / 'worktrees' / args.name
    build = BASE / (args.name + '-release')
    original_source = BASE / 'worktrees/execution-boundaries-20261108'
    original_build = BASE / 'execution-boundaries-release'
    assert not source.exists() and not build.exists()
    env = {k: v for k, v in os.environ.items() if not k.startswith('PYTHON')}

    def run(kind, command, cwd):
        log = EVIDENCE / f'{args.name}-{kind}.log'
        with log.open('x') as stream:
            subprocess.run(command, cwd=cwd, env=env, stdout=stream,
                           stderr=subprocess.STDOUT, check=True)
        print(kind, 'completed', flush=True)

    run('worktree', ['git', 'worktree', 'add', '--detach', str(source), args.commit],
        original_source)
    build.mkdir()
    config = subprocess.check_output([str(original_build / 'config.status'), '--config'],
                                     text=True, env=env)
    run('configure', [str(source / 'configure'), *shlex.split(config)], build)
    assert (build / 'pyconfig.h').read_bytes() == (original_build / 'pyconfig.h').read_bytes()

    # Existing module flags include relocated development-library sysroot paths
    # that configure's pkg-config files otherwise spell as /usr/include and /usr/lib.
    original = (original_build / 'Makefile').read_text()
    original = original.replace(str(original_source), str(source))
    original = original.replace(str(original_build), str(build))
    pattern = re.compile(r'^(MODULE_\w+_(?:CFLAGS|LDFLAGS))=(.*)$', re.M)
    flags = dict(pattern.findall(original))
    makefile = build / 'Makefile'
    makefile.write_text(pattern.sub(lambda m: m[1] + '=' + flags[m[1]],
                                    makefile.read_text()))
    run('build', ['make', '-s', '-j10'], build)


if __name__ == '__main__':
    main()
