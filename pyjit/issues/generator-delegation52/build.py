"""Configure isolated delegation builds using the selected GCC configuration."""

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/generator-delegation52'
SELECTED = BASE / 'worktrees/round51-golden'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('name')
    parser.add_argument('--commit', default='0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254')
    parser.add_argument('--role', choices=['release', 'debug', 'stats'], default='release')
    parser.add_argument('--configure-only', action='store_true')
    args = parser.parse_args()
    assert re.fullmatch(r'delegation52(?:-[a-z0-9]+)*', args.name)
    source = BASE / 'worktrees' / args.name
    build = BASE / f'{args.name}-{args.role}'
    reference = BASE / ('execution-boundaries-stats' if args.role == 'stats'
                        else f'round51-golden-{args.role}')
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env['UV_CACHE_DIR'] = '/home/power/.cache/uv'
    EVIDENCE.mkdir(parents=True, exist_ok=True)

    def run(kind, command, cwd):
        log = EVIDENCE / f'{args.name}-{args.role}-{kind}.log'
        with log.open('x') as stream:
            subprocess.run(command, cwd=cwd, env=env, stdout=stream,
                           stderr=subprocess.STDOUT, check=True)
        print(f'{args.name} {args.role} {kind} complete', flush=True)

    if not source.exists():
        run('worktree', ['git', 'worktree', 'add', '--detach', str(source), args.commit],
            SELECTED)
    build.mkdir()
    config = subprocess.check_output([str(reference / 'config.status'), '--config'],
                                     text=True, env=env)
    run('configure', [str(source / 'configure'), *shlex.split(config)], build)
    assert (build / 'pyconfig.h').read_bytes() == (reference / 'pyconfig.h').read_bytes()
    original = (reference / 'Makefile').read_text()
    pattern = re.compile(r'^(MODULE_\w+_(?:CFLAGS|LDFLAGS))=(.*)$', re.M)
    flags = dict(pattern.findall(original))
    makefile = build / 'Makefile'
    makefile.write_text(pattern.sub(lambda match: match[1] + '=' + flags[match[1]],
                                    makefile.read_text()))
    if not args.configure_only:
        run('build', ['make', '-s', '-j10', '-W', str(source / 'Modules/getbuildinfo.c')], build)


if __name__ == '__main__':
    main()
