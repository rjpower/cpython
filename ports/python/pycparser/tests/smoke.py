"""Smoke test for pycparser: parse C, visit the AST, regenerate source."""

import importlib.metadata

import pycparser
from pycparser import c_ast, c_generator, c_parser

assert pycparser.__version__ == "3.12", pycparser.__version__
assert importlib.metadata.version("pycparser") == "3.12"

SOURCE = """
typedef struct { int x; double y[4]; } point;
static int add(int a, int b) { return a + b; }
int main(void) { point p; p.x = add(1, 2); return p.x > 2 ? 0 : 1; }
"""


class Calls(c_ast.NodeVisitor):
    def __init__(self):
        self.names = []

    def visit_FuncCall(self, node):
        self.names.append(node.name.name)
        self.generic_visit(node)


tree = c_parser.CParser().parse(SOURCE, filename="<smoke>")
calls = Calls()
calls.visit(tree)
assert calls.names == ["add"], calls.names
functions = [node.decl.name for node in tree.ext
             if isinstance(node, c_ast.FuncDef)]
assert functions == ["add", "main"], functions

# Generating C from a reparse of generated C must reach a fixed point.
generated = c_generator.CGenerator().visit(tree)
reparsed = c_parser.CParser().parse(generated)
assert c_generator.CGenerator().visit(reparsed) == generated
assert "return a + b;" in generated
