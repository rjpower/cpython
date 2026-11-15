"""Retain one clean model55 executable and requested disassemblies."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess


BASE = Path('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build')
EVIDENCE = BASE / 'pyjit-link-wave1/call-model55'


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def engine_targets(executable, source):
    """Resolve this experiment's ELF64/x86-64 local-label table, without execution."""
    target_header = source / 'Python/ir_targets.h'
    declaration = target_header.read_text()
    count = int(re.search(r'instruction_targets\[(\d+)\]', declaration)[1])
    order = [(int(index), name) for index, name in re.findall(
        r'\[(\d+)\] = &&TARGET_(\w+),', declaration)]
    if [index for index, _ in order] != list(range(count)):
        raise ValueError('generated engine targets are not a complete ordered table')
    data = executable.read_bytes()

    def unpack(fmt, offset):
        size = struct.calcsize(fmt)
        if offset < 0 or offset + size > len(data):
            raise ValueError('ELF structure exceeds file bounds')
        return struct.unpack_from(fmt, data, offset)

    header = unpack('<16sHHIQQQIHHHHHH', 0)
    if (header[0][:6] != b'\x7fELF\x02\x01' or header[1] not in (2, 3)
            or header[2] != 62 or header[11] != 64 or not header[12]):
        raise ValueError('engine map requires ordinary ELF64 little-endian x86-64')
    sections = [unpack('<IIQQQQIIQQ', header[6] + index * 64)
                for index in range(header[12])]

    def section_bytes(section):
        offset, size = section[4:6]
        if section[1] == 8 or offset + size > len(data):
            raise ValueError('ELF section has no bounded file contents')
        return data[offset:offset + size]

    def string(table, index):
        if index >= len(table) or (end := table.find(b'\0', index)) < 0:
            raise ValueError('invalid ELF symbol name')
        return table[index:end].decode('ascii')

    symbols = []
    for section in sections:
        if section[1] != 2:  # SHT_SYMTAB: local engine/table names are required.
            continue
        if section[9] != 24 or section[5] % 24 or section[6] >= len(sections):
            raise ValueError('invalid ELF symbol table')
        strings = section_bytes(sections[section[6]])
        for entry in struct.iter_unpack('<IBBHQQ', section_bytes(section)):
            symbols.append((string(strings, entry[0]), *entry[1:]))
    engines = [s for s in symbols if s[1] & 15 == 2 and re.fullmatch(
        r'run_handlers(?:\.(?:constprop|isra)\.\d+)*', s[0])]
    tables = [s for s in symbols if s[1] & 15 == 1 and re.fullmatch(
        r'instruction_targets(?:\.\d+)?', s[0])]
    if len(engines) != 1 or len(tables) != 1:
        raise ValueError('engine map requires one actual engine and target-table symbol')
    engine, table = engines[0], tables[0]

    def symbol_section(symbol):
        if not 0 < symbol[3] < len(sections):
            raise ValueError('engine/table symbol has no defined section')
        section = sections[symbol[3]]
        if (not symbol[5] or symbol[4] < section[3]
                or symbol[4] + symbol[5] > section[3] + section[5]):
            raise ValueError('engine/table symbol exceeds section bounds')
        return section

    engine_section = symbol_section(engine)
    if not engine_section[2] & 4:  # SHF_EXECINSTR
        raise ValueError('engine symbol is not in executable code')
    section_bytes(engine_section)
    section = symbol_section(table)
    if table[5] != count * 8:
        raise ValueError('ELF target-table size disagrees with generated handler order')
    contents = section_bytes(section)
    offset = table[4] - section[3]
    addresses = list(struct.unpack_from(f'<{count}Q', contents, offset))
    relocated = set()
    for section in sections:
        if section[1] == 19:  # SHT_RELR is outside the pinned link configuration.
            raise ValueError('packed RELR requires an explicit target-map reader')
        if section[1] not in (4, 9):
            continue
        fmt = '<QQq' if section[1] == 4 else '<QQ'
        size = struct.calcsize(fmt)
        if section[9] != size or section[5] % size:
            raise ValueError('invalid ELF relocation section')
        for relocation in struct.iter_unpack(fmt, section_bytes(section)):
            address, info = relocation[:2]
            if not table[4] <= address < table[4] + table[5]:
                continue
            index, remainder = divmod(address - table[4], 8)
            if (remainder or index in relocated or section[1] != 4
                    or info != 8):  # R_X86_64_RELATIVE with symbol index zero.
                raise ValueError('unsupported or overlapping engine-target relocation')
            addresses[index] = relocation[2]
            relocated.add(index)
    if any(not engine[4] <= address < engine[4] + engine[5]
           for address in addresses):
        raise ValueError('dispatch target lies outside the actual engine')
    return {
        'engine': {'symbol': engine[0], 'address': engine[4], 'size': engine[5]},
        'table': {'symbol': table[0], 'address': table[4], 'size': table[5],
                  'relative_relocations': len(relocated)},
        'generated_header_sha256': digest(target_header),
        'targets': [{'handler': index, 'label': name, 'address': address}
                    for (index, name), address in zip(order, addresses)],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('tag')
    parser.add_argument('symbols', nargs='+')
    parser.add_argument('--variant', choices=['integrated', 'bitmap'],
                        default='integrated')
    parser.add_argument('--source', type=Path)
    parser.add_argument('--build', type=Path)
    parser.add_argument('--evidence-root', type=Path, default=EVIDENCE)
    args = parser.parse_args()
    if (args.source is None) != (args.build is None):
        parser.error('--source and --build must be supplied together')
    source = args.source or BASE / f'worktrees/call55-{args.variant}'
    if subprocess.check_output(['git', '-C', str(source), 'status', '--porcelain']):
        raise RuntimeError('native capture requires a clean source')
    build = args.build or BASE / f'call55-{args.variant}-release'
    head = subprocess.check_output(
        ['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
    native = json.loads(subprocess.check_output(
        [str(build / 'python'), '-S', '-B', '-c',
         'import json,sys,sysconfig; print(json.dumps(dict(head=sys._git[2],'
         'config={key:sysconfig.get_config_var(key) for key in '
         '("PY_INTERPRETER_BACKEND","PY_TAIL_CALL_CC","Py_IR_INTERP",'
         '"HAVE_COMPUTED_GOTOS")})))'], cwd=build, text=True))
    native_head = native['head']
    if not head.startswith(native_head):
        raise RuntimeError(f'source/native mismatch: {head}, {native_head}')
    destination = args.evidence_root / args.tag
    destination.mkdir()
    executable = destination / 'python'
    shutil.copy2(build / 'python', executable)
    rows = []
    command = ['nm', '-S', '-n', str(executable)]
    symbols = subprocess.check_output(command, text=True)
    (destination / 'symbols.txt').write_text(symbols)
    rows.append(dict(command=command, returncode=0, output='symbols.txt'))
    target_map = None
    requested = list(args.symbols)
    if native['config']['PY_INTERPRETER_BACKEND'] == 'computed_goto':
        target_map = engine_targets(executable, source)
        (destination / 'engine-targets.json').write_text(json.dumps(target_map, indent=2))
        engine = target_map['engine']['symbol']
        requested = [engine if symbol == 'run_handlers' else symbol for symbol in requested]
        if engine not in requested:
            requested.append(engine)
    for symbol in requested:
        if not any(line.endswith(' ' + symbol) for line in symbols.splitlines()):
            raise ValueError(f'missing native symbol {symbol}')
        command = ['objdump', '-dr', '--no-show-raw-insn',
                   '--disassemble=' + symbol, str(executable)]
        output = symbol + '.txt'
        with (destination / output).open('x') as stream:
            result = subprocess.run(command, stdout=stream, check=True)
        rows.append(dict(command=command, returncode=result.returncode, output=output))
    record = dict(source_head=head, native_head=native_head,
                  config=native['config'], engine_targets=target_map,
                  sha256=digest(executable), commands=rows)
    (destination / 'manifest.json').write_text(json.dumps(record, indent=2))
    print(json.dumps(record))


if __name__ == '__main__':
    main()
