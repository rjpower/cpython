"""Retain one clean model55 executable and requested disassemblies."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/call-model55'


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tag')
    parser.add_argument('symbols', nargs='+')
    parser.add_argument('--variant', choices=['integrated', 'bitmap'],
                        default='integrated')
    args = parser.parse_args()
    source = BASE / f'worktrees/call55-{args.variant}'
    if subprocess.check_output(['git', '-C', str(source), 'status', '--porcelain']):
        raise RuntimeError('native capture requires a clean source')
    build = BASE / f'call55-{args.variant}-release'
    head = subprocess.check_output(
        ['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    native_head = subprocess.check_output(
        [str(build / 'python'), '-S', '-B', '-c',
         'import sys; print(sys._git[2])'], cwd=build, text=True).strip()
    if not head.startswith(native_head):
        raise RuntimeError(f'source/native mismatch: {head}, {native_head}')
    destination = EVIDENCE / args.tag
    destination.mkdir()
    executable = destination / 'python'
    shutil.copy2(build / 'python', executable)
    rows = []
    command = ['nm', '-S', '-n', str(executable)]
    symbols = subprocess.check_output(command, text=True)
    (destination / 'symbols.txt').write_text(symbols)
    rows.append(dict(command=command, returncode=0, output='symbols.txt'))
    for symbol in args.symbols:
        if not any(line.endswith(' ' + symbol) for line in symbols.splitlines()):
            raise ValueError(f'missing native symbol {symbol}')
        command = ['objdump', '-dr', '--no-show-raw-insn',
                   '--disassemble=' + symbol, str(executable)]
        output = symbol + '.txt'
        with (destination / output).open('x') as stream:
            result = subprocess.run(command, stdout=stream, check=True)
        rows.append(dict(command=command, returncode=result.returncode, output=output))
    record = dict(source_head=head, native_head=native_head,
                  sha256=digest(executable), commands=rows)
    (destination / 'manifest.json').write_text(json.dumps(record, indent=2))
    print(json.dumps(record))


if __name__ == '__main__':
    main()
