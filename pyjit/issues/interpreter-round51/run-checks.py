"""Run native focus/full qualification for an explicit source/build subtree."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time


GDB_PRETTY_PRINT = 'test.test_gdb.test_pretty_print.PrettyPrintTests.*'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tag')
    parser.add_argument('stage', choices=['focus', 'full'])
    parser.add_argument('tests', nargs='*')
    parser.add_argument('--source', required=True, type=Path)
    parser.add_argument('--build', required=True, action='append', metavar='ROLE=PATH')
    parser.add_argument('--evidence', required=True, type=Path)
    parser.add_argument('--verbose', action='store_true')
    parser.add_argument('--match', action='append', default=[],
                        help='run only matching test cases (repeatable regrtest glob)')
    parser.add_argument('--seed', type=int)
    parser.add_argument('--include-gdb-pretty-print', action='store_true',
                        help='include the quarantined GDB display diagnostics')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]*', args.tag):
        parser.error('tag must contain only letters, digits, underscores or hyphens')
    if (args.stage == 'focus') != bool(args.tests):
        parser.error('focus requires test names; full takes no test names')
    builds = {}
    for value in args.build:
        role, separator, directory = value.partition('=')
        if not separator or not directory or role not in ('release', 'debug') or role in builds:
            parser.error('--build requires a unique release=PATH or debug=PATH')
        builds[role] = Path(directory).resolve()
    source = args.source.resolve()
    args.evidence.mkdir(parents=True, exist_ok=True)
    journal = args.evidence / f'{args.tag}-{args.stage}-checks.json'
    # Reserve the tag before any native execution.
    with journal.open('x') as stream:
        stream.write('[]\n')
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PYTHON') and key != 'LD_PRELOAD'}
    env.update(PYTHONHASHSEED='0', PYTHON_JIT='0', UV_CACHE_DIR='/home/power/.cache/uv')
    ignored = [] if args.include_gdb_pretty_print else [GDB_PRETTY_PRINT]
    head = subprocess.check_output(
        ['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    diff = subprocess.check_output(
        ['git', '-C', str(source), 'diff', 'HEAD', '--stat'], text=True)
    magic = int(re.search(r'^#define PYC_MAGIC_NUMBER (\d+)$',
                         (source / 'Include/internal/pycore_magic_number.h').read_text(), re.M)[1])
    rows = []
    for role, build in builds.items():
        python = build / 'python'
        log = args.evidence / f'{args.tag}-{role}-{args.stage}.log'
        command = [str(python), '-B', '-m', 'test', '-j8']
        if args.verbose:
            command.append('-v')
        if args.seed is not None:
            command += ['--randseed', str(args.seed)]
        for pattern in args.match:
            command += ['--match', pattern]
        for pattern in ignored:
            command += ['--ignore', pattern]
        command += args.tests
        row = dict(role=role, kind=args.stage, command=command, cwd=str(build),
                   source=str(source), source_head=head, source_diff=diff,
                   log=str(log), started=time.time(), ignored_diagnostics=ignored,
                   qualification='with skipped GDB display diagnostics' if ignored else 'unfiltered')
        rows.append(row)
        journal.write_text(json.dumps(rows, indent=2))
        try:
            probe = subprocess.check_output(
                [str(python), '-S', '-B', '-c',
                 'import _imp,json,sys; print(json.dumps(dict(git=sys._git,paths=sys.path,magic=_imp.pyc_magic_number_token & 65535)))'],
                cwd=build, env=env, text=True)
            row['native_preflight'] = native = json.loads(probe)
            row['expected_magic'] = magic
            journal.write_text(json.dumps(rows, indent=2))
            revision = native['git'][2]
            assert re.fullmatch(r'[0-9a-fA-F]{7,40}', revision), row
            assert head.startswith(revision), row
            assert native['magic'] == magic, row
            assert any(Path(path).resolve() == source / 'Lib' for path in native['paths']), row
            with log.open('x') as stream:
                result = subprocess.run(command, cwd=build, env=env,
                                        stdout=stream, stderr=subprocess.STDOUT)
            row['returncode'] = result.returncode
        except Exception as error:
            row['failure'] = f'{type(error).__name__}: {error}'
            raise
        finally:
            row['finished'] = time.time()
            journal.write_text(json.dumps(rows, indent=2))
        print(json.dumps({key: row[key] for key in ['role', 'kind', 'returncode', 'log']}), flush=True)
        if result.returncode:
            raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
