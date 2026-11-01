"""Generate semantic helpers from the same analyzed DSL as the interpreter.

Each valid IR handler has an explicit helper or before-effect tier-down reason.
The initial set covers ordinary scalar, branch and positional-call control flow;
suspension, instrumentation and specialized frame transfers remain interpreter
operations until their Action adapters are implemented.
"""

import argparse

from analyzer import analyze_files
from cwriter import CWriter
from generators_common import DEFAULT_INPUT, ROOT, write_header
from ir_generator import (
    EXEC_ONLY, IREmitter, compute_formats, generate_handler, handler_ids,
    handler_instructions, is_specialization, num_handlers, uop_tokens,
)

# These family members are emitted by assemble.c choose_handler into pristine
# words; family membership alone does not mean an execution-only specialization.
PRISTINE_CONSTANTS = {"LOAD_CONST_INLINE_INT", "LOAD_CONST_INLINE_FLOAT", "LOAD_SMALL_INT_INLINE"}

DIRECT = {
    "NOP", "NOT_TAKEN", "RESUME", "LOAD_SMALL_INT", "LOAD_CONST",
    "LOAD_FAST", "LOAD_FAST_BORROW", "LOAD_FAST_CHECK", "STORE_FAST", "POP_TOP",
    "LOAD_FAST_BORROW_LOAD_FAST_BORROW", "LOAD_FAST_LOAD_FAST",
    "STORE_FAST_LOAD_FAST", "STORE_FAST_STORE_FAST",
    "LOAD_GLOBAL", "LOAD_GLOBAL_NOFB", "PUSH_NULL", "COPY", "SWAP",
    "BINARY_OP", "COMPARE_OP", "TO_BOOL", "UNARY_NOT", "UNARY_NEGATIVE",
    "JUMP_FORWARD", "JUMP_BACKWARD", "JUMP_BACKWARD_NO_INTERRUPT", "IR_JUMP",
    "POP_JUMP_IF_FALSE", "POP_JUMP_IF_TRUE", "POP_JUMP_IF_NONE",
    "POP_JUMP_IF_NOT_NONE", "CALL", "RETURN_VALUE",
    "LOAD_ATTR", "LOAD_ATTR_NOFB", "STORE_ATTR", "STORE_ATTR_NOFB",
    "GET_ITER", "FOR_ITER", "END_FOR", "POP_ITER", "BUILD_LIST", "BUILD_TUPLE",
    "BUILD_MAP", "UNPACK_SEQUENCE", "STORE_SUBSCR",
    "CONTAINS_OP", "CALL_KW", "COPY_FREE_VARS", "MAKE_CELL", "LOAD_DEREF", "STORE_DEREF",
}


def classify(name, inst):
    if name in EXEC_ONLY:
        return "execution-only natural admission head"
    if name == "BINARY_OP":
        return "semantic metadata base without an executable IR dispatch target"
    tokens = uop_tokens(inst)
    if is_specialization(inst) and name not in PRISTINE_CONSTANTS:
        return "executing specialization: tier down before effect"
    if name not in DIRECT and name not in PRISTINE_CONSTANTS and not name.startswith("BINARY_OP_"):
        return "Action adapter not yet implemented"
    if "return" in tokens:
        return "external evaluator return requires a dedicated adapter"
    labels = [tokens[i + 1] for i, token in enumerate(tokens[:-1]) if token == "goto"]
    if set(labels) - {"error", "exception_unwind"}:
        return "nonlocal interpreter label requires a dedicated adapter"
    return None


class HelperEmitter(IREmitter):
    helper = True

    def dispatch_same_oparg(self, tkn, tkn_iter, uop, storage, inst):
        # Cold specialization changed only feedback/the executing handler.
        # The generic body follows once, using its immutable descriptor.
        if not uop.name.startswith("_SPECIALIZE_"):
            return super().dispatch_same_oparg(tkn, tkn_iter, uop, storage, inst)
        next(tkn_iter)
        next(tkn_iter)
        self.emit(f"do {{ next_instr = this_instr + 1; goto jit_after{uop.name}; }} while (0)")
        return True


def generate(filenames, outfile):
    analysis = analyze_files(filenames)
    formats = compute_formats(analysis)
    ids = handler_ids(analysis)
    instructions = handler_instructions(analysis)
    # Metadata also describes the generic BINARY_OP semantic base, although
    # executable IR targets use operation-specific forms. Classify it too.
    for name in formats.keys() - instructions.keys():
        if name in analysis.instructions:
            instructions[name] = analysis.instructions[name]
            ids[name] = analysis.instructions[name].opcode
    write_header(__file__, filenames, outfile)
    outfile.write('''
/* This file is included in ceval.c: semantic helpers share its private
 * operation helpers, but never its computed-goto labels or C activation. */
#if !_Py_TAIL_CALL_INTERP && !defined(Py_STATS) && !defined(Py_GIL_DISABLED)
#pragma push_macro("IR_FIELD_A")
#pragma push_macro("IR_FIELD_B")
#pragma push_macro("IR_FIELD_C")
#pragma push_macro("IR_FIELD_D")
#pragma push_macro("IR_FIELD_CD")
#undef IR_FIELD_A
#undef IR_FIELD_B
#undef IR_FIELD_C
#undef IR_FIELD_D
#undef IR_FIELD_CD
#define IR_FIELD_A(W) (op->fields[0])
#define IR_FIELD_B(W) (op->fields[1])
#define IR_FIELD_C(W) (op->fields[2])
#define IR_FIELD_D(W) (op->fields[3])
#define IR_FIELD_CD(W) (op->wide)
#pragma push_macro("DISPATCH")
#pragma push_macro("DISPATCH_SAME_OPARG")
#pragma push_macro("JUMP_TO_LABEL")
#pragma push_macro("DISPATCH_INLINED")
#pragma push_macro("JIT_CALL_RETRY")
#undef JIT_CALL_RETRY
#define JIT_CALL_RETRY() JIT_FINISH(_PyJIT_DEOPT, _PyJIT_DEOPT_BEFORE)
#pragma push_macro("JIT_CALL_NEEDS_PREPARE")
#pragma push_macro("JIT_PREPARE_CALL")
#undef JIT_CALL_NEEDS_PREPARE
#undef JIT_PREPARE_CALL
#define JIT_CALL_NEEDS_PREPARE(I) (ctx->unit != NULL)
#define JIT_PREPARE_CALL(I,F) _PyJit_PrepareCall(ctx, (I), (F))
#pragma push_macro("JIT_LEAVE_FRAME")
#undef JIT_LEAVE_FRAME
#define JIT_LEAVE_FRAME(FRAME) do { if (ctx->cold_request != NULL) _PyJit_RetireCold(ctx->cold_request, (FRAME)); } while (0)
#undef DISPATCH
#undef DISPATCH_SAME_OPARG
#undef JUMP_TO_LABEL
#undef DISPATCH_INLINED
#define JIT_FINISH(KIND, PHASE) do { \\
    ctx->frame = frame; ctx->next_instr = next_instr; \\
    ctx->raising_instr = (KIND) == _PyJIT_RAISE ? this_instr : NULL; ctx->phase = (PHASE); \\
    return (KIND); \\
} while (0)
#define DISPATCH() JIT_FINISH(jit_action, jit_phase)
#define DISPATCH_SAME_OPARG() do { \\
    next_instr = this_instr; \\
    JIT_FINISH(_PyJIT_DEOPT, _PyJIT_DEOPT_BEFORE); \\
} while (0)
#define JIT_PHASE_error _PyJIT_ERROR_LOG_PENDING
#define JIT_PHASE_exception_unwind _PyJIT_UNWIND_PENDING
#define JUMP_TO_LABEL(LABEL) JIT_FINISH(_PyJIT_RAISE, JIT_PHASE_##LABEL)
#define DISPATCH_INLINED(NEW_FRAME) do { \\
    assert((NEW_FRAME)->previous == frame); \\
    frame = (NEW_FRAME); next_instr = frame->instr_ptr; \\
    JIT_FINISH(_PyJIT_ENTER_FRAME, _PyJIT_ENTER_PREPARED); \\
} while (0)
''')
    out = CWriter(outfile, 2, False)
    emitter = HelperEmitter(out, analysis, formats)
    reasons = {}
    for name, inst in sorted(instructions.items()):
        if name not in formats:
            continue
        reason = classify(name, inst)
        reasons[name] = reason
        if reason is None:
            emitter.handler_format = formats[name]
            generate_handler(name, inst, formats[name], emitter, out)
    outfile.write('''
#undef JIT_FINISH
#undef JIT_PHASE_error
#undef JIT_PHASE_exception_unwind
#pragma pop_macro("JIT_LEAVE_FRAME")
#pragma pop_macro("JIT_PREPARE_CALL")
#pragma pop_macro("JIT_CALL_NEEDS_PREPARE")
#pragma pop_macro("JIT_CALL_RETRY")
#pragma pop_macro("DISPATCH_INLINED")
#pragma pop_macro("JUMP_TO_LABEL")
#pragma pop_macro("DISPATCH_SAME_OPARG")
#pragma pop_macro("DISPATCH")
#pragma pop_macro("IR_FIELD_CD")
#pragma pop_macro("IR_FIELD_D")
#pragma pop_macro("IR_FIELD_C")
#pragma pop_macro("IR_FIELD_B")
#pragma pop_macro("IR_FIELD_A")
''')
    outfile.write(f"static const _PyJitHelperInfo jit_helpers[{num_handlers(analysis)}] = {{\n")
    for name, reason in sorted(reasons.items(), key=lambda item: ids[item[0]]):
        helper = "NULL" if reason else f"jit_helper_{name}"
        message = f'"{reason}"' if reason else "NULL"
        outfile.write(f'    [{ids[name]}] = {{"{name}", {helper}, {message}}},\n')
    outfile.write("};\n#endif\n")
    outfile.write("""
const _PyJitHelperInfo *
_PyJit_GetHelper(unsigned int handler)
{
#if !_Py_TAIL_CALL_INTERP && !defined(Py_STATS) && !defined(Py_GIL_DISABLED)
    if (handler < Py_ARRAY_LENGTH(jit_helpers) && jit_helpers[handler].name != NULL) {
        return &jit_helpers[handler];
    }
#endif
    return NULL;
}
""")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-o", "--output", default=ROOT / "Python/generated_jit_helpers.c.h")
    parser.add_argument("input", nargs="*", default=[DEFAULT_INPUT])
    args = parser.parse_args()
    with open(args.output, "w") as outfile:
        generate(args.input, outfile)
