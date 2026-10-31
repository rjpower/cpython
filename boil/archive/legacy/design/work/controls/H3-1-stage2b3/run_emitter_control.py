"""Host-only copied old emitter control, using the existing generator oracle."""
import ast
import io
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

import test
import test.support
import test.support.import_helper

ROOT = Path(__file__).resolve().parents[5]
test.__path__.insert(0, str(ROOT / 'Lib/test'))
from test import test_generated_cases as cases

source = ROOT / 'Tools/cases_generator/ir_generator.py'
text = source.read_text()
current = 'out.emit(f"INSTRUCTION_STATS({name});\\n")'
old = 'out.emit(f"INSTRUCTION_STATS({inst.name});\\n")'
assert text.count(current) == 1
module = ast.parse(text.replace(current, old), filename=str(source))
function = next(node for node in module.body if isinstance(node, ast.FunctionDef)
                and node.name == 'generate_handler')
namespace = dict(vars(cases.ir_generator))
exec(compile(ast.Module(body=[function], type_ignores=[]), str(source), 'exec'), namespace)
with patch.object(cases.ir_generator, 'generate_handler', namespace['generate_handler']):
    result = unittest.TextTestRunner(stream=io.StringIO()).run(
        unittest.TestSuite([cases.TestIRCases('test_counted_handler_entry_names')]))
assert not result.errors, result.errors
assert result.failures, 'old semantic emitter must fail the actual target-name oracle'
failed = [str(test) for test, traceback in result.failures]
assert any("handler='BINARY_OP_ADD'" in name for name in failed), failed
assert any("handler='LOAD_ATTR_NOFB'" in name for name in failed), failed
print(f'Copied old semantic emitter rejected by actual per-operation/NOFB assertions ({len(failed)} subtests)')
