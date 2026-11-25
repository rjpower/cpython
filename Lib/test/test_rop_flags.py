"""The flag table of the stack ops against regions89's final head.

A stack op has no semantics of its own: the translator's rules lower it, and
its register instructions declare what happens.  Its row of the opcode
table therefore holds transport facts only: how it is encoded (ARG, CONST,
NAME, JUMP, FREE, LOCAL) and EVAL_BREAK, which marks the stack ops that can
reach an eval-breaker check in every lowering and serves the compiler's
line-number propagation only (flowgraph.c).  ERROR and ESCAPES describe what
a stack op does, so they are no longer compared (a stack-level reader such as
the deleted copy_local_as_load needed them).

The table below is Include/internal/pycore_opcode_metadata.h at
e571a8717e6, restricted to those flags; each difference is an entry of
EXCEPTIONS, with its reason.
"""

import re
import tempfile
import unittest
from pathlib import Path

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import rop_check
    import rop_verify

ROOT = Path(test_tools.basepath)
METADATA = ROOT / "Include" / "internal" / "pycore_opcode_metadata.h"

TRANSPORT = frozenset({"ARG", "CONST", "NAME", "JUMP", "FREE", "LOCAL", "EVAL_BREAK"})

ORACLE_TEXT = """
    BINARY_OP ARG
    BINARY_OP_ADD_FLOAT
    BINARY_OP_ADD_INT
    BINARY_OP_ADD_UNICODE
    BINARY_OP_AND_INT
    BINARY_OP_EXTEND
    BINARY_OP_FLOOR_DIVIDE_INT
    BINARY_OP_INPLACE_ADD_UNICODE
    BINARY_OP_LSHIFT_INT
    BINARY_OP_MULTIPLY_FLOAT
    BINARY_OP_MULTIPLY_INT
    BINARY_OP_OR_INT
    BINARY_OP_REMAINDER_INT
    BINARY_OP_RSHIFT_INT
    BINARY_OP_SUBTRACT_FLOAT
    BINARY_OP_SUBTRACT_INT
    BINARY_OP_TRUEDIV_FLOAT
    BINARY_OP_XOR_INT
    BINARY_SLICE
    BUILD_INTERPOLATION ARG
    BUILD_LIST ARG
    BUILD_MAP ARG
    BUILD_SET ARG
    BUILD_SLICE ARG
    BUILD_STRING ARG
    BUILD_TEMPLATE
    BUILD_TUPLE ARG
    CACHE
    CALL ARG EVAL_BREAK
    CALL_EX_NON_PY_GENERAL EVAL_BREAK
    CALL_EX_PY
    CALL_FUNCTION_EX EVAL_BREAK
    CALL_INTRINSIC_1 ARG
    CALL_INTRINSIC_2 ARG
    CALL_KW ARG
    CALL_KW_BOUND_METHOD ARG
    CALL_KW_NON_PY ARG
    CALL_KW_PY ARG
    CHECK_EG_MATCH
    CHECK_EXC_MATCH
    CLEANUP_THROW
    COMPARE_OP ARG
    CONTAINS_OP ARG
    CONVERT_VALUE ARG
    COPY ARG
    COPY_FREE_VARS ARG
    DELETE_DEREF ARG LOCAL
    DELETE_FAST ARG LOCAL
    DELETE_SUBSCR
    DICT_MERGE ARG
    DICT_UPDATE ARG
    END_ASYNC_FOR ARG JUMP
    END_FOR
    END_SEND
    EXIT_INIT_CHECK
    EXTENDED_ARG ARG
    FORMAT_SIMPLE
    FORMAT_WITH_SPEC
    FOR_ITER ARG JUMP
    FOR_ITER_GEN
    FOR_ITER_LIST ARG JUMP
    FOR_ITER_RANGE ARG JUMP
    FOR_ITER_TUPLE ARG JUMP
    FOR_ITER_VIRTUAL ARG JUMP
    GET_AITER
    GET_ANEXT
    GET_AWAITABLE ARG
    GET_ITER ARG
    GET_ITER_SELF
    GET_ITER_VIRTUAL
    GET_LEN
    IMPORT_FROM ARG NAME
    IMPORT_NAME ARG NAME
    INSTRUMENTED_CALL ARG EVAL_BREAK
    INSTRUMENTED_CALL_FUNCTION_EX EVAL_BREAK
    INSTRUMENTED_CALL_KW ARG
    INSTRUMENTED_END_ASYNC_FOR ARG JUMP
    INSTRUMENTED_END_FOR
    INSTRUMENTED_END_SEND
    INSTRUMENTED_FOR_ITER ARG JUMP
    INSTRUMENTED_INSTRUCTION
    INSTRUMENTED_JUMP_BACKWARD ARG EVAL_BREAK
    INSTRUMENTED_JUMP_FORWARD ARG
    INSTRUMENTED_JUMP_IF_FALSE_OR_POP ARG
    INSTRUMENTED_JUMP_IF_TRUE_OR_POP ARG
    INSTRUMENTED_LINE
    INSTRUMENTED_LOAD_SUPER_ATTR ARG NAME
    INSTRUMENTED_POP_ITER
    INSTRUMENTED_POP_JUMP_IF_FALSE ARG
    INSTRUMENTED_POP_JUMP_IF_NONE ARG
    INSTRUMENTED_POP_JUMP_IF_NOT_NONE ARG
    INSTRUMENTED_POP_JUMP_IF_TRUE ARG
    INSTRUMENTED_RESUME ARG EVAL_BREAK
    INSTRUMENTED_RETURN_VALUE
    INSTRUMENTED_YIELD_VALUE ARG
    INTERPRETER_EXIT
    IS_OP ARG
    JUMP_BACKWARD ARG JUMP EVAL_BREAK
    JUMP_BACKWARD_NO_INTERRUPT ARG JUMP
    JUMP_FORWARD ARG JUMP
    JUMP_IF_FALSE_OR_POP ARG JUMP
    JUMP_IF_TRUE_OR_POP ARG JUMP
    LIST_APPEND ARG
    LIST_EXTEND ARG
    LOAD_ATTR ARG NAME
    LOAD_BUILD_CLASS
    LOAD_COMMON_CONSTANT ARG
    LOAD_CONST ARG CONST
    LOAD_DEREF ARG FREE
    LOAD_FAST ARG LOCAL
    LOAD_FAST_AND_CLEAR ARG LOCAL
    LOAD_FAST_BORROW ARG LOCAL
    LOAD_FAST_BORROW_LOAD_FAST_BORROW ARG
    LOAD_FAST_CHECK ARG LOCAL
    LOAD_FAST_LOAD_FAST ARG
    LOAD_FROM_DICT_OR_DEREF ARG FREE
    LOAD_FROM_DICT_OR_GLOBALS ARG NAME
    LOAD_GLOBAL ARG NAME
    LOAD_LOCALS
    LOAD_NAME ARG NAME
    LOAD_SMALL_INT ARG
    LOAD_SPECIAL ARG
    LOAD_SUPER_ATTR ARG NAME
    LOAD_SUPER_ATTR_ATTR ARG NAME
    LOAD_SUPER_ATTR_METHOD ARG NAME
    MAKE_CELL ARG LOCAL
    MAKE_FUNCTION
    MAP_ADD ARG
    MATCH_CLASS ARG
    MATCH_KEYS
    MATCH_MAPPING
    MATCH_SEQUENCE
    MATERIALIZE_KWARGS ARG
    NOP
    NOT_TAKEN
    POP_EXCEPT
    POP_ITER
    POP_JUMP_IF_FALSE ARG JUMP
    POP_JUMP_IF_NONE ARG JUMP
    POP_JUMP_IF_NOT_NONE ARG JUMP
    POP_JUMP_IF_TRUE ARG JUMP
    POP_TOP
    PUSH_EXC_INFO
    PUSH_NULL
    RAISE_VARARGS ARG
    RERAISE ARG
    RESERVED
    RESUME ARG EVAL_BREAK
    RETURN_GENERATOR
    RETURN_VALUE
    SEND ARG JUMP
    SEND_ASYNC_GEN ARG JUMP
    SEND_GEN
    SEND_VIRTUAL ARG JUMP
    SETUP_ANNOTATIONS
    SET_ADD ARG
    SET_FUNCTION_ATTRIBUTE ARG
    SET_UPDATE ARG
    STORE_ATTR ARG NAME
    STORE_ATTR_INSTANCE_VALUE
    STORE_ATTR_SHAPE
    STORE_ATTR_SHAPE_ADD
    STORE_ATTR_SLOT
    STORE_ATTR_WITH_HINT ARG NAME
    STORE_DEREF ARG LOCAL
    STORE_FAST ARG LOCAL
    STORE_FAST_LOAD_FAST ARG
    STORE_FAST_STORE_FAST ARG
    STORE_GLOBAL ARG NAME
    STORE_NAME ARG NAME
    STORE_SLICE
    STORE_SUBSCR
    SWAP ARG
    TO_BOOL
    UNARY_INVERT
    UNARY_NEGATIVE
    UNARY_NOT
    UNPACK_EX ARG
    UNPACK_SEQUENCE ARG
    WITH_EXCEPT_START
    YIELD_VALUE ARG
"""

# (op, flag): the reason the table differs from the oracle.  None are needed
# today; a stub that lost a transport flag would be listed here.
EXCEPTIONS: dict[tuple[str, str], str] = {}


def parse_oracle() -> dict[str, frozenset[str]]:
    result = {}
    for line in ORACLE_TEXT.strip().splitlines():
        op, *flags = line.split()
        result[op] = frozenset(flags)
    return result


def parse_header() -> dict[str, frozenset[str]]:
    result = {}
    pattern = re.compile(r"^\s*\[(\w+)\] = \{ (?:true|false), \w+, ([^}]*)\},", re.M)
    for m in pattern.finditer(METADATA.read_text()):
        flags = frozenset(re.findall(r"HAS_(\w+)_FLAG", m.group(2)))
        result[m.group(1)] = flags & TRANSPORT
    return result


class TransportFlagTests(unittest.TestCase):

    def test_transport_flags_match_the_oracle(self):
        header = parse_header()
        for op, oracle in parse_oracle().items():
            with self.subTest(op=op):
                differ = {f for f in oracle ^ header[op] if (op, f) not in EXCEPTIONS}
                self.assertEqual(differ, set())

    def test_exceptions_are_needed(self):
        header = parse_header()
        oracle = parse_oracle()
        for (op, flag), reason in EXCEPTIONS.items():
            with self.subTest(op=op, flag=flag):
                self.assertNotEqual(flag in oracle[op], flag in header[op])
                self.assertTrue(reason)

    def test_stack_only_ops_declare_no_effects(self):
        # The stubs' bodies only abort; their flags come from the families'
        # arms or from nowhere
        text = (ROOT / "Python" / "bytecodes.c").read_text()
        self.assertEqual(re.findall(r"effects\([^)]*\)\s*exits\(abort\)", text), [])


class ReleaseDeclarationTests(unittest.TestCase):

    def test_releasing_a_binding_declares_runs_python(self):
        # An arm that releases a Local operand's old binding may run its
        # finalizer, and says so (rop_verify's per-class release rule)
        with tempfile.TemporaryDirectory() as d:
            for path in rop_check.ROP_DIR.glob("*.rop"):
                text = path.read_text()
                if path.name == "values.rop":
                    text = text.replace("releases on (local), runs_python)",
                                        "releases on (local))", 1)
                (Path(d) / path.name).write_text(text)
            checked = rop_verify.load(sorted(Path(d).glob("*.rop")))
        self.assertEqual(len(checked.diagnostics), 1)
        self.assertIn("(old(local) closes a local's binding) may runs_python",
                      checked.diagnostics[0].message)


if __name__ == "__main__":
    unittest.main()
