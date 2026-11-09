"""Run the three terminal generator tests without overlaying the host stdlib."""
import argparse
import ast
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest import mock

parser_args = argparse.ArgumentParser()
parser_args.add_argument('runtime', type=Path)
args = parser_args.parse_args()
artifact_dir = Path(__file__).resolve().parent
os.chdir(args.runtime)
sys.path.insert(0, str(args.runtime / 'Tools/cases_generator'))
import analyzer
import ir_generator
import ir_metadata_generator
import ir_numbers
import parser

names = {'setUp', 'tearDown', 'analyze', 'ir_output',
         'test_terminal_observer_ownership_and_composition',
         'test_terminal_observer_defined_null_edge',
         'test_terminal_observer_dotted_owner_mutations'}
selected = []
for node in ast.parse(Path('Lib/test/test_generated_cases.py').read_text()).body:
    if isinstance(node, ast.ClassDef) and node.name == 'SequentialNumbers':
        selected.append(node)
    if isinstance(node, ast.ClassDef) and node.name == 'TestIRCases':
        node.body = [child for child in node.body
                     if isinstance(child, ast.FunctionDef) and child.name in names]
        selected.append(node)
handle_stderr = contextlib.nullcontext
__file__ = str(args.runtime / 'Lib/test/test_generated_cases.py')
exec(compile(ast.Module(selected, type_ignores=[]),
             'Lib/test/test_generated_cases.py', 'exec'))
suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestIRCases)
assert suite.countTestCases() == 3
result = unittest.TextTestRunner(verbosity=2).run(suite)
if not result.wasSuccessful():
    raise SystemExit(1)
analysis = analyzer.analyze_files(['Python/bytecodes.c'])
output = io.StringIO()
ir_generator.generate_ir(['Python/bytecodes.c'], analysis, output, False)
new = output.getvalue()
metadata = io.StringIO()
ir_metadata_generator.generate_ir_metadata(['Python/bytecodes.c'], analysis, metadata)

def handlers(text):
    return {match.group(1): match.group(0) for match in re.finditer(
        r'(?m)^\s*TARGET\((\w+)\) \{.*?(?=^\s*TARGET\(|\Z)', text, re.S)}

old_handlers = handlers(Path('Python/generated_ir_cases.c.h').read_text())
new_handlers = handlers(new)
changed = sorted(name for name in old_handlers.keys() & new_handlers.keys()
                 if old_handlers[name] != new_handlers[name])
assert all(name.startswith('INSTRUMENTED_') for name in changed), changed
ordinary = sorted(name for name in old_handlers if not name.startswith('INSTRUMENTED_'))
def digest(rows):
    return hashlib.sha256(''.join(rows[name] for name in ordinary).encode()).hexdigest()
assert digest(old_handlers) == digest(new_handlers)
for name in ('INSTRUMENTED_FOR_ITER', 'INSTRUMENTED_JUMP_IF_TRUE_OR_POP'):
    body = new_handlers[name]
    callback = body.index('_PyIR_ObserveCommittedBranch(')
    assert 'IR_PUBLISH_TEMP' in body[:callback] or 'IR_STORE_TEMP' in body[:callback]
    assert not re.search(r'IR_(?:STORE|PUBLISH|MOVE)_', body[callback:]) if name == 'INSTRUMENTED_FOR_ITER' else True
print(json.dumps({'tests': result.testsRun, 'ir_bytes': len(new),
                  'metadata_bytes': len(metadata.getvalue()),
                  'changed_handlers': changed, 'ordinary_handlers': len(ordinary),
                  'ordinary_sha256_before': digest(old_handlers),
                  'ordinary_sha256_after': digest(new_handlers)}, indent=2))

examples = []
for name in ('INSTRUMENTED_FOR_ITER', 'INSTRUMENTED_JUMP_IF_TRUE_OR_POP'):
    examples += [f'## {name}', '', 'Before:', '```c', old_handlers[name].strip(),
                 '```', '', 'After:', '```c', new_handlers[name].strip(), '```', '']
(artifact_dir / 'terminal-output-examples.md').write_text('\n'.join(examples))
