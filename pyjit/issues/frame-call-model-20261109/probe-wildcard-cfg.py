"""Coordinator-run frontend location witness for wildcard source coverage."""
import ast
import json
import sys
from _testinternalcapi import compiler_codegen, optimize_cfg

source = '''match command:
    case 1:
        if False:
            value = 1
    case _:
        if False:
            value = 0
'''
instructions, metadata = compiler_codegen(ast.parse(source), '<wildcard-cfg>', 0)
before = instructions.get_instructions()
optimized = optimize_cfg(instructions, metadata['consts'], 0)
print(json.dumps({'python': sys.version, 'source': source,
                  'codegen': before, 'optimized': optimized.get_instructions(),
                  'consts': [repr(value) for value in metadata['consts']]}, indent=2))
