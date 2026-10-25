
"""
opcode module - potentially shared between dis and other modules which
operate on bytecodes (e.g. peephole optimizers).
"""


__all__ = ["cmp_op", "stack_effect", "hascompare", "opname", "opmap",
           "HAVE_ARGUMENT", "EXTENDED_ARG", "hasarg", "hasconst", "hasname",
           "hasjump", "hasjrel", "hasjabs", "hasfree", "haslocal", "hasexc"]

import builtins
import _opcode
from _opcode import stack_effect

from _opcode_metadata import (_specializations, _specialized_opmap, opmap,  # noqa: F401
                              _ir_opmap, HAVE_ARGUMENT, MIN_INSTRUMENTED_OPCODE)  # noqa: F401
EXTENDED_ARG = opmap['EXTENDED_ARG']

# The code is register IR: a word per instruction, whose handler is an
# opcode, a specialisation, or one of the handlers in _ir_opmap.  opname
# names every handler.
_ir_formats = _opcode.get_ir_formats()
opname = ['<%r>' % (op,) for op in range(len(_ir_formats))]
for m in (opmap, _specialized_opmap, _ir_opmap):
    for op, i in m.items():
        opname[i] = op

cmp_op = ('<', '<=', '==', '!=', '>', '>=')

# These lists are documented as part of the dis module's API
hasarg = [op for op in opmap.values() if _opcode.has_arg(op)]
hasconst = [op for op in opmap.values() if _opcode.has_const(op)]
hasname = [op for op in opmap.values() if _opcode.has_name(op)]
hasjump = [op for op in opmap.values() if _opcode.has_jump(op)]
hasjrel = hasjump  # for backward compatibility
hasjabs = []
hasfree = [op for op in opmap.values() if _opcode.has_free(op)]
haslocal = [op for op in opmap.values() if _opcode.has_local(op)]
hasexc = [op for op in opmap.values() if _opcode.has_exc(op)]


_intrinsic_1_descs = _opcode.get_intrinsic1_descs()
_intrinsic_2_descs = _opcode.get_intrinsic2_descs()
_special_method_names = _opcode.get_special_method_names()
_common_constants = [builtins.AssertionError, builtins.NotImplementedError,
                     builtins.tuple, builtins.all, builtins.any, builtins.list,
                     builtins.set,
                     # Append-only — must match CONSTANT_* in
                     # Include/internal/pycore_opcode_utils.h.
                     None, "", True, False, -1, builtins.frozenset, ()]
_nb_ops = _opcode.get_nb_ops()

hascompare = [opmap["COMPARE_OP"]]
