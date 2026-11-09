"""Capture native disassembly, then update the affected static fixtures once.

Run --capture with the candidate interpreter, and --apply with the host driver.
Tests continue to compare against static expectations after this migration.
"""

import argparse
import ast
import hashlib
import json
from pathlib import Path
import re


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture(path):
    from test import test_dis as fixture

    source = Path(fixture.__file__)
    case = fixture.DisTests()
    case.code_quicken(fixture.loop_test)
    targets = {
        'dis_bug708901': (fixture.bug708901, {}),
        'dis_asyncwith': (fixture._asyncwith, {}),
        'dis_loop_test_quickened_code': (fixture.loop_test, {'adaptive': True}),
        **{f'dis_nested_{depth}': (fixture._h, {'depth': depth})
           for depth in range(3)},
    }
    output = {name: case.strip_addresses(case.get_disassembly(func, **options))
              for name, (func, options) in targets.items()}
    jumpy = 'expected_opinfo_jumpy = [\n' + '\n'.join(
        fixture._stringify_instruction(instruction)
        for instruction in fixture.dis.get_instructions(
            fixture.jumpy, first_line=fixture.expected_jumpy_line)) + '\n]'
    value = {'source': str(source), 'source_sha256': digest(source),
             'firstlines': {name: func.__code__.co_firstlineno
                            for name, (func, _) in targets.items()},
             'disassembly': output, 'jumpy': jumpy}
    with path.open('x') as stream:
        json.dump(value, stream, indent=2)


def line_expression(anchor, difference):
    base = anchor + '.__code__.co_firstlineno'
    return base if difference == 0 else f'{base} + {difference}'


def static_template(name, text, data):
    arguments = []
    anchor = {'dis_bug708901': 'bug708901', 'dis_asyncwith': '_asyncwith',
              'dis_loop_test_quickened_code': 'loop_test'}.get(name, '_h')
    first = data['firstlines'][name]
    if name in ('dis_nested_1', 'dis_nested_2'):
        previous = f'dis_nested_{int(name[-1]) - 1}'
        prefix = data['disassembly'][previous]
        assert text.startswith(prefix + '\n'), name
        text = '%s' + text[len(prefix):]
        arguments.append(previous)
    pattern = re.compile(
        r'(?P<line>^ *[0-9]+)(?= {2,})|'
        r'(?P<const>LOAD_CONST +)(?P<index>[0-9]+)(?= \(<code object <genexpr>)|'
        r'file "(?P<filename>[^"]+)", line (?P<code_line>[0-9]+)', re.M)
    constant_seen = False

    def parameter(match):
        nonlocal constant_seen
        if match['line'] is not None:
            field = match['line']
            arguments.append(line_expression(anchor, int(field) - first))
            return f'%{len(field)}d'
        if match['const'] is not None:
            if name != 'dis_nested_1' or constant_seen:
                return match[0]
            constant_seen = True
            arguments.append('1 if __debug__ else 0')
            return match['const'] + '%d'
        assert Path(match['filename']).resolve() == Path(data['source']).resolve()
        arguments.extend(('__file__', line_expression(anchor, int(match['code_line']) - first)))
        return 'file "%s", line %d'

    text = pattern.sub(parameter, text)
    assert '"""' not in text
    literal = text.replace('\\', '\\\\')
    return name + ' = """\\\n' + literal + '""" % (\n    ' + ',\n    '.join(arguments) + ',\n)'


def apply(path):
    data = json.loads(path.read_text())
    source = Path(data['source'])
    assert digest(source) == data['source_sha256'], 'fixture source changed since capture'
    text = source.read_text()
    replacements = {name: static_template(name, output, data)
                    for name, output in data['disassembly'].items() if name != 'dis_nested_0'}
    replacements['expected_opinfo_jumpy'] = data['jumpy']
    offsets = [0]
    for line in text.splitlines(keepends=True):
        offsets.append(offsets[-1] + len(line))
    edits = []
    for node in ast.parse(text).body:
        if (isinstance(node, ast.Assign) and len(node.targets) == 1
                and isinstance(node.targets[0], ast.Name)
                and node.targets[0].id in replacements):
            edits.append((offsets[node.lineno - 1] + node.col_offset,
                          offsets[node.end_lineno - 1] + node.end_col_offset,
                          replacements[node.targets[0].id]))
    assert len(edits) == len(replacements), 'missing or duplicate fixture assignment'
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    ast.parse(text)
    source.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--capture', type=Path)
    mode.add_argument('--apply', type=Path)
    args = parser.parse_args()
    if args.capture is not None:
        capture(args.capture)
    else:
        apply(args.apply)


if __name__ == '__main__':
    main()
