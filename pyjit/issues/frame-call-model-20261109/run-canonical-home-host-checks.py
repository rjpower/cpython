"""Check the two local-load changes against the pre-regeneration snapshot.

Run after applying the source patch and before regenerating the checked-in
cases header. This script loads only the selected generator tests.
"""

import argparse
import ast
import contextlib
import io
import json
import os
from pathlib import Path
import re
import sys
import tempfile
import unittest
from unittest import mock


runtime = argparse.ArgumentParser()
runtime.add_argument("runtime", type=Path)
runtime = runtime.parse_args().runtime.resolve()
os.chdir(runtime)
sys.path.insert(0, str(runtime / "Tools/cases_generator"))
import analyzer
import ir_generator
import ir_numbers
import parser


methods = {
    "setUp",
    "tearDown",
    "handler",
    "test_canonical_numeric_constructor_proof",
    "test_canonical_numeric_constructor_mutations_fail_closed",
    "test_canonical_numeric_direct_int64_boundary",
    "test_canonical_local_duplicate_source",
}
selected = []
for node in ast.parse(Path("Lib/test/test_generated_cases.py").read_text()).body:
    if isinstance(node, ast.FunctionDef) and node.name == "parse_src":
        selected.append(node)
    elif isinstance(node, ast.ClassDef) and node.name == "SequentialNumbers":
        selected.append(node)
    elif isinstance(node, ast.ClassDef) and node.name == "TestIRCases":
        node.body = [
            child for child in node.body
            if isinstance(child, ast.FunctionDef) and child.name in methods
        ]
        selected.append(node)
handle_stderr = contextlib.nullcontext
__file__ = str(runtime / "Lib/test/test_generated_cases.py")
exec(compile(ast.Module(selected, type_ignores=[]), __file__, "exec"))
suite = unittest.defaultTestLoader.loadTestsFromTestCase(TestIRCases)
assert suite.countTestCases() == 4
result = unittest.TextTestRunner(verbosity=2).run(suite)
if not result.wasSuccessful():
    raise SystemExit(1)

analysis = analyzer.analyze_files(["Python/bytecodes.c"])
new = io.StringIO()
ir_generator.generate_ir(["Python/bytecodes.c"], analysis, new, False)


def handlers(text):
    return {
        match.group(1): match.group(0)
        for match in re.finditer(
            r"(?m)^\s*TARGET\((\w+)\) \{.*?(?=^\s*TARGET\(|\Z)",
            text, re.S
        )
    }


before = handlers(Path("Python/generated_ir_cases.c.h").read_text())
after = handlers(new.getvalue())
assert before.keys() == after.keys()
changed = sorted(name for name in before if before[name] != after[name])
assert changed == ["LOAD_FAST", "LOAD_FAST_BORROW"], changed
for name in ("BINARY_OP_MULTIPLY_FLOAT", "BINARY_OP_ADD_FLOAT",
             "BINARY_OP_SUBTRACT_FLOAT", "BINARY_OP_TRUEDIV_FLOAT"):
    assert before[name] == after[name], name
for name in changed:
    assert "IR_STORE_CANONICAL_DST(IR_A(ir_word), value);" in after[name]
    assert "IR_STORE_DST(IR_A(ir_word), value);" in before[name]
print(json.dumps({"host_tests": result.testsRun,
                  "changed_handlers": changed,
                  "unchanged_handlers": len(before) - len(changed)},
                 indent=2))
