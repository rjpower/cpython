"""Compile the two NULL-join witnesses without calling either function."""

import ast
import dis
import json
from pathlib import Path
import sys

import _testinternalcapi


source = Path(sys.argv[1])
tree = ast.parse(source.read_text(), filename=str(source))
method = next(node for node in ast.walk(tree) if isinstance(node, ast.FunctionDef)
              and node.name == "test_settrace_null_requirements_keep_both_join_obligations")
rows = []
for node in method.body:
    if not isinstance(node, ast.FunctionDef):
        continue
    namespace = {}
    exec(compile(ast.Module(body=[node], type_ignores=[]), str(source), "exec"), namespace)
    code = namespace[node.name].__code__
    words = _testinternalcapi.get_ir(code)
    positions = {instruction.offset: instruction.positions.lineno
                 for instruction in dis.get_instructions(code)}
    rows.append({"name": node.name, "first": code.co_firstlineno,
                 "nlocals": code.co_nlocals,
                 "words": [{"pc": pc, "line": positions.get(pc), "word": word}
                           for pc, word in enumerate(words)]})
print(json.dumps(rows, indent=2))
