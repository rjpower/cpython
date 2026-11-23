"""Generate the baseline JIT's inline templates (tiers87.md, B2).

A template is the machine code of one handler, printed as a C function over
the JIT's assembler (Include/internal/pycore_jit_asm.h) that the compiler
calls for a word whose handler has one.  It is built from the handler's
operation record (ir_records.py) and the leaf IR of its uops (ir_leaf.py):
the uops are joined into one graph, the skeleton (operand views, owner
detach, the destination store and the closes) is bound from the record and
tested against the word's slot-class flags when the template runs, the graph
is simplified, values get registers and every op becomes an assembler call
or a call of its primitive's lowering (Python/jit_lower.c).  A handler that
the generator cannot lower keeps the B1 call of its function form; the
generated file lists each with the reason.

Reads ../../Python/bytecodes.c.  Writes Python/jit_lowerings.h (the
lowerings' prototypes and table) and Python/jit_templates.c.h.
"""

import argparse
from dataclasses import dataclass

import ir_leaf
from generators_common import DEFAULT_INPUT, ROOT

DEFAULT_LOWERINGS = ROOT / "Python/jit_lowerings.h"
DEFAULT_TEMPLATES = ROOT / "Python/jit_templates.c.h"


# ---------------------------------------------------------------------------
# The lowerings: for each primitive that Python/jit_lower.c lowers, the
# temporaries it needs and whether it may call (its slow path, or a count in
# a Py_REF_DEBUG build).  The operands follow from the primitive's signature
# in ir_leaf.PRIMITIVES; pycore_jit_lower.h gives their order.

@dataclass(frozen=True)
class Lowering:
    temps: int = 0          # general registers
    xtemps: int = 0         # xmm registers
    calls: bool = False     # may call a helper: the live registers are saved


LOWERINGS = {
    # Tags
    "PyValue_IsNull": Lowering(),
    "PyValue_IsPointerSpace": Lowering(1),
    "PyValue_IsObject": Lowering(1),
    "PyValue_IsInline": Lowering(1),
    "PyValue_IsInlineInt": Lowering(1),
    "PyValue_IsInlineFloat": Lowering(2),
    "PyValue_BothInlineInt": Lowering(1),
    "PyValue_IsTaggedIndex": Lowering(1),
    "PyValue_Is": Lowering(1),
    "_PyValue_NamesObject": Lowering(1),
    "PyValue_IsTrue": Lowering(1),
    "PyValue_IsFalse": Lowering(1),
    "PyValue_IsNone": Lowering(1),
    "PyValue_BoolCheck": Lowering(2),
    "PyValue_LongCheckExact": Lowering(2),
    # Numbers
    "_PyValue_IntScaled": Lowering(),
    "_PyValue_FromIntScaled": Lowering(1),
    "PyValue_InlineInt64": Lowering(),
    "PyValue_FromInlineInt": Lowering(1),
    "PyValue_InlineFloat": Lowering(1),
    "PyValue_FromDoubleNotNaN": Lowering(1),
    "PyValue_FloatOperandValues": Lowering(2),
    "PyValue_FromInt64": Lowering(1, calls=True),
    "PyValue_FromDouble": Lowering(1, calls=True),
    "PyValue_TagIndex": Lowering(1),
    "PyValue_UntagIndex": Lowering(),
    "PyValue_IncrementTaggedIndexNoOverflow": Lowering(),
    # Words as objects
    "PyValue_AsKnownObjectBorrow": Lowering(),
    "_PyValue_AsKnownObjectBorrowPointer": Lowering(),
    "_PyValueGuarded_Load": Lowering(),
    # Objects and types
    "Py_TYPE": Lowering(),
    "PyList_CheckExact": Lowering(1),
    "PyTuple_CheckExact": Lowering(1),
    "PyDict_CheckExact": Lowering(1),
    "PyModule_CheckExact": Lowering(1),
    "PyUnicode_CheckExact": Lowering(1),
    "PyFunction_Check": Lowering(1),
    "PyList_GET_SIZE": Lowering(),
    "PyTuple_GET_SIZE": Lowering(),
    "PyList_GET_VALUE": Lowering(1),
    "PyTuple_GET_VALUE": Lowering(),
    "_PyList_ITEMS": Lowering(),
    "_PyObject_InlineValues": Lowering(1),
    "_PyObject_Shape": Lowering(1),
    "_PyObject_ValueSlot": Lowering(),
    "_PyObject_ShapeReadDup": Lowering(1, calls=True),
    "DK_UNICODE_ENTRIES": Lowering(2),
    "_PyDictValues_AddToInsertionOrder": Lowering(2),
    "_PyDictValues_SetShape": Lowering(),
    "_PyLong_IsZero": Lowering(1),
    # The frame, in r13, and its slots, in r15
    "GLOBALS": Lowering(),
    "BUILTINS": Lowering(),
    "FRAME_CO_CONSTS": Lowering(),
    "GETLOCAL": Lowering(),
    "IR_SLOTS": Lowering(),
    # The thread
    "_Py_EvalBreakerHasEvents": Lowering(),
    # Counts
    "PyValue_DUP": Lowering(1, calls=True),
    "PyValue_XDUP": Lowering(1, calls=True),
    "PyValue_CLOSE": Lowering(1, calls=True),
    "PyValue_XCLOSE": Lowering(1, calls=True),
    "PyValue_CLOSE_SPECIALIZED": Lowering(1, calls=True),
}


def is_predicate(prim: ir_leaf.Primitive) -> bool:
    "Its first result is a bool, which a lowering returns as a condition"
    return bool(prim.results) and prim.results[0] == "bool"


def register_results(prim: ir_leaf.Primitive) -> tuple[str, ...]:
    return prim.results[1:] if is_predicate(prim) else prim.results


def operand_kinds(prim: ir_leaf.Primitive, lowering: Lowering) -> str:
    "'g' or 'x' per operand of the lowering, in the order of pycore_jit_lower.h"
    types = [*register_results(prim), *prim.params]
    if prim.name == "PyValue_CLOSE_SPECIALIZED":
        types = types[:1]           # the destructor is aux, not an operand
    kinds = "".join("x" if t == "f64" else "g" for t in types)
    return kinds + "g" * lowering.temps + "x" * lowering.xtemps


def lowering_name(name: str) -> str:
    return f"_PyJitLower_{name}"


def write_lowerings(out) -> None:
    out.write("// This file is generated by Tools/cases_generator/jit_template_generator.py\n")
    out.write("// from Tools/cases_generator/ir_leaf.py.\n// Do not edit!\n\n")
    for name in LOWERINGS:
        out.write(f"int {lowering_name(name)}(_PyJitWordCx *cx, const uint8_t *ops, "
                  "const void *aux);\n")
    out.write("\nconst _PyJitLoweringInfo _PyJit_Lowerings[] = {\n")
    for name, lowering in LOWERINGS.items():
        prim = ir_leaf.PRIMITIVES[name]
        kinds = operand_kinds(prim, lowering)
        nres = len(register_results(prim))
        nargs = len(kinds) - nres - lowering.temps - lowering.xtemps
        out.write(f'    {{"{name}", {lowering_name(name)}, {nres}, {nargs}, '
                  f'{lowering.temps}, {lowering.xtemps}, "{kinds}", '
                  f'{int(is_predicate(prim))}}},\n')
    out.write("};\n")
    out.write("const int _PyJit_NumLowerings = "
              "(int)(sizeof(_PyJit_Lowerings) / sizeof(_PyJit_Lowerings[0]));\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--lowerings", default=str(DEFAULT_LOWERINGS))
    parser.add_argument("input", nargs="*", default=[DEFAULT_INPUT])
    args = parser.parse_args()
    with open(args.lowerings, "w") as out:
        write_lowerings(out)


if __name__ == "__main__":
    main()
