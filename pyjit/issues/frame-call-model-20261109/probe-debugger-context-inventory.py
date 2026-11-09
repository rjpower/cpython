"""Coordinator-run compile-only inventory for cold debugger context recipes."""
import dis
import json
import struct
import sys
import types

import _opcode
from _testinternalcapi import get_co_localskinds, get_ir


SOURCES = {
    "ordinary_try": '''def sample(divisor):
    value = 0
    try:
        value = 10 // divisor
        value += 1
    except ZeroDivisionError:
        value = -1
    return value
''',
    "caught_exception": '''def sample(divisor):
    value = 0
    try:
        value = 10 // divisor
    except ZeroDivisionError as error:
        value = 1
        value = 2
    return value
''',
    "bare_reraise_context": '''def sample(divisor):
    try:
        value = 10 // divisor
    except ZeroDivisionError:
        raise
    return value
''',
    "try_finally": '''def sample(divisor):
    value = 0
    try:
        value = 10 // divisor
    finally:
        value += 2
    return value
''',
    "caught_finally": '''def sample(divisor):
    value = 0
    try:
        try:
            value = 10 // divisor
        except ZeroDivisionError as error:
            value = 1
            value = 2
    finally:
        value += 3
    return value
''',
    "with_owner": '''def sample(manager):
    with manager as item:
        value = item
        value += 1
    return value
''',
    "comprehension_restore": '''def sample(values):
    marker = "outer"
    result = [marker for marker in values]
    return marker, result
''',
    "yield_resume": '''def sample():
    value = 1
    yield value
    value = 2
    return value
''',
    "caught_yield": '''def sample():
    try:
        raise LookupError("active")
    except LookupError:
        yield 1
        return 2
''',
}


def source_records(code):
    data = code.co_linetable
    magic, version, nlocations, nrecords = struct.unpack_from('<4sIII', data)
    assert magic == b'IRSM' and version == 1
    assert len(data) == 16 + nlocations + nrecords * 40
    keys = ('from', 'to', 'edge', 'line', 'end_line', 'column', 'end_column',
            'handler', 'exc_slot', 'lasti')
    return [dict(zip(keys, struct.unpack_from('<10i', data, 16 + nlocations + 40*i)))
            for i in range(nrecords)]


def describe(code):
    contracts = _opcode.get_ir_contract()
    raw = struct.unpack(f'<{len(code.co_code) // 8}Q', code.co_code)
    words = get_ir(code)
    positions = list(code.co_positions())
    instructions = {instruction.offset: instruction
                    for instruction in dis.get_instructions(code)}
    rows = []
    for pc, word in enumerate(words):
        row = contracts[raw[pc] & 65535]
        rows.append({
            'pc': pc, 'operation': word[0], 'fields': word[1:],
            'oparg': instructions[pc].arg, 'position': positions[pc],
            'roles': row.roles, 'reads': row.reads, 'writes': row.writes,
            'through': row.through, 'output_kind': row.output_kind,
            'flags': row.flags, 'effects': row.effects,
        })
    return {
        'qualname': code.co_qualname, 'nlocalsplus': len(get_co_localskinds(code)),
        'varnames': code.co_varnames, 'cellvars': code.co_cellvars,
        'freevars': code.co_freevars, 'stacksize': code.co_stacksize,
        'constslots': repr(code.co_constslots), 'words': rows,
        'lines': list(code.co_lines()),
        'exception_entries': [entry._asdict() for entry in dis.Bytecode(code).exception_entries],
        'source_events': source_records(code),
    }


def walk(code):
    if code.co_name != '<module>':
        yield code
    for value in code.co_consts:
        if isinstance(value, types.CodeType):
            yield from walk(value)


def main():
    result = {}
    for label, source in SOURCES.items():
        root = compile(source, f'<debugger-context:{label}>', 'exec')
        result[label] = {'source': source, 'codes': [describe(code) for code in walk(root)]}
    print(json.dumps({'python': sys.version, 'compile_only': True,
                      'unit': 'physical word and physical home indices',
                      'cases': result}, indent=2))


if __name__ == '__main__':
    main()
