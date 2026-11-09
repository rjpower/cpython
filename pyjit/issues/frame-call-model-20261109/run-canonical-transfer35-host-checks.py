"""Run focused canonical proofs and compare in-memory emission before regen."""
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

args = argparse.ArgumentParser()
args.add_argument('runtime', type=Path)
runtime = args.parse_args().runtime.resolve()
os.chdir(runtime)
sys.path.insert(0, str(runtime / 'Tools/cases_generator'))
import analyzer
import ir_generator
import ir_metadata_generator
import ir_numbers
import parser

selected = []
for node in ast.parse(Path('Lib/test/test_generated_cases.py').read_text()).body:
    if isinstance(node, ast.FunctionDef) and node.name == 'parse_src':
        selected.append(node)
    elif isinstance(node, ast.ClassDef) and node.name == 'SequentialNumbers':
        selected.append(node)
    elif isinstance(node, ast.ClassDef) and node.name == 'TestIRCases':
        node.body = [child for child in node.body
                     if isinstance(child, ast.FunctionDef)
                     and (child.name in {'setUp', 'tearDown', 'handler'}
                          or (child.name.startswith('test_canonical_')
                              and not child.name.startswith('test_canonical_home_')))]
        selected.append(node)
handle_stderr = contextlib.nullcontext
__file__ = str(runtime / 'Lib/test/test_generated_cases.py')
exec(compile(ast.Module(selected, type_ignores=[]), __file__, 'exec'))
suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestIRCases)
assert suite.countTestCases() == 8
result = unittest.TextTestRunner(verbosity=2).run(suite)
if not result.wasSuccessful():
    raise SystemExit(1)

analysis = analyzer.analyze_files(['Python/bytecodes.c'])
out = io.StringIO()
ir_generator.generate_ir(['Python/bytecodes.c'], analysis, out, False)

def handlers(text):
    return {match[1]: match[0] for match in re.finditer(
        r'(?m)^\s*TARGET\((\w+)\) \{.*?(?=^\s*TARGET\(|\Z)', text, re.S)}

before = handlers(Path('Python/generated_ir_cases.c.h').read_text())
after = handlers(out.getvalue())
assert before.keys() == after.keys()
changed = sorted(name for name in before if before[name] != after[name])
assert changed == ['BINARY_OP_SUBSCR_LIST_INT', 'YIELD_VALUE'], changed
assert before['BINARY_OP_SUBSCR_LIST_INT'].replace(
    'IR_STORE_DST(IR_A(ir_word), res);',
    'IR_STORE_CANONICAL_DST(IR_A(ir_word), res);') == after['BINARY_OP_SUBSCR_LIST_INT']
assert before['YIELD_VALUE'].replace(
    'IR_STORE_RETURN(value);', 'IR_STORE_CANONICAL_RETURN(value);') == after['YIELD_VALUE']
metadata = io.StringIO()
ir_metadata_generator.generate_ir_metadata(['Python/bytecodes.c'], analysis, metadata)
assert metadata.getvalue() == Path('Include/internal/pycore_ir_metadata.h').read_text()
print(json.dumps({'tests': result.testsRun, 'changed_handlers': changed,
                  'unchanged_handlers': len(before) - len(changed),
                  'metadata_unchanged': True,
                  'generated_sha256': hashlib.sha256(out.getvalue().encode()).hexdigest()},
                 indent=2))
