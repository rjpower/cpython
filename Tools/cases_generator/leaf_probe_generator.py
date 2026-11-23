"""Generate the leaf probes: each uop inside the leaf subset (ir_leaf.py) as
a C function that _testinternalcapi runs on given operands, so that a test
can compare the C uop with the leaf IR's evaluation (ir_leaf_eval.py).

The body is printed by the generators' Emitter, as the interpreter's is; the
probe binds the statement macros to the probe's outcomes instead of the
interpreter's transfers.  Reads ../../Python/bytecodes.c.
Writes to Modules/_testinternalcapi/leaf_probes.c.h by default.
"""

import argparse
import io

import ir_generator
import ir_leaf
from analyzer import Analysis, Instruction, Uop, analyze_files
from cwriter import CWriter
from generators_common import (
    DEFAULT_INPUT, ROOT, Emitter, TokenIterator, instruction_macro_args,
    write_header)
from lexer import Token
from stack import Stack, Storage

DEFAULT_OUTPUT = ROOT / "Modules/_testinternalcapi/leaf_probes.c.h"

# The outcomes of a probe; ir_leaf_eval.OUTCOMES names them in this order
OUTCOMES = ("NEXT", "EXIT", "DEOPT", "RAISE", "COMPLETE")


class ProbeStack(Stack):
    """A probe's stack is an array of the caller's, not a frame's: spills,
    reloads and bounds checks of the frame's stack pointer print nothing"""

    def stack_bound_check(self, out: CWriter) -> None:
        pass

    def emit_spill(self, out: CWriter) -> None:
        pass

    def emit_reload(self, out: CWriter) -> None:
        if self.physical_sp is None:
            self.physical_sp = self.frame_sp


class ProbeEmitter(Emitter):
    "The Emitter with the probe's binding of the statement macros"

    def __init__(self, out: CWriter, analysis: Analysis) -> None:
        super().__init__(out, analysis.labels)
        self.miss = "EXIT"
        for name in ("JUMPBY", "JUMPTO"):
            self._replacers[name] = self.jump
        self._replacers["IR_COMPLETE_STEP"] = self.complete_step
        self._replacers["IR_REPORT_EDGE"] = self.report_edge
        self._replacers["TAKE"] = self.take
        self._replacers["assert"] = self.drop_assert
        for name in ir_leaf.CAPABILITY_MACROS:
            self._replacers[name] = self.capability

    def emit_jump_to_predicted(self, inst: Instruction | None) -> None:
        self.emit(f"return LEAF_PROBE_{self.miss};\n")

    def goto_error(self, offset: int, storage: Storage) -> str:
        return "return LEAF_PROBE_RAISE;"

    def exit_if(self, tkn, tkn_iter, uop, storage, inst) -> bool:
        self.miss = "EXIT"
        return super().deopt_if(tkn, tkn_iter, uop, storage, inst)

    def deopt_if(self, tkn, tkn_iter, uop, storage, inst) -> bool:
        self.miss = "DEOPT"
        return super().deopt_if(tkn, tkn_iter, uop, storage, inst)

    def drop_assert(self, tkn, tkn_iter, uop, storage, inst) -> bool:
        """The leaf IR is the release body, which has no asserts; some name
        the interpreter's state (next_instr), which a probe has not"""
        depth = 0
        for token in tkn_iter:
            if token.kind == "LPAREN":
                depth += 1
            elif token.kind == "RPAREN":
                depth -= 1
            elif token.kind == "SEMI" and depth == 0:
                return True
        raise AssertionError("an assert without a ';'")

    def take(self, tkn, tkn_iter, uop, storage, inst) -> bool:
        "TAKE(x) moves x's count: on the probe's stack of words, x itself"
        next(tkn_iter)
        name = next(tkn_iter)
        next(tkn_iter)
        self.value_kill(name, storage, False)
        self.emit(name)
        return True

    def jump(self, tkn, tkn_iter, uop, storage, inst) -> bool:
        "JUMPBY and JUMPTO take the word's branch; the probe records it"
        instruction_macro_args(tkn, tkn_iter)
        assert next(tkn_iter).kind == "SEMI"
        self.emit("result->branch = 1;\n")
        return True

    def report_edge(self, tkn, tkn_iter, uop, storage, inst) -> bool:
        args, _ = instruction_macro_args(tkn, tkn_iter)
        assert next(tkn_iter).kind == "SEMI"
        self.emit("result->edge = 1 + !!(")
        for token in args[0]:
            self.emit(token)
        self.emit(");\n")
        return True

    def complete_step(self, tkn, tkn_iter, uop, storage, inst) -> bool:
        args, _ = instruction_macro_args(tkn, tkn_iter)
        assert next(tkn_iter).kind == "SEMI"
        self.emit("result->completed = ")
        for token in args[1]:
            self.emit(token)
        self.emit(";\nreturn LEAF_PROBE_COMPLETE;\n")
        return False

    def capability(self, tkn, tkn_iter, uop, storage, inst) -> bool:
        "The expansion that the interpreter and the translator use"
        args, _ = instruction_macro_args(tkn, tkn_iter)
        names = ["".join(t.text for t in arg) for arg in args]
        if tkn.text == "IR_DUP_CANONICAL_ELEMENT":
            names.append(ir_leaf.element_kind(uop))
        statement = tkn.text not in ("IR_DUP_CANONICAL_ELEMENT", "IR_DUP_SHAPE_READ")
        if statement:
            assert next(tkn_iter).kind == "SEMI"
        for item in ir_leaf.capability_expansion(tkn.text, names):
            if item[0] == "guard":
                self.emit(f"if ({item[1]}) {{ return LEAF_PROBE_EXIT; }}\n")
            elif item[0] == "stmt":
                self.emit(item[1] + "\n")
            elif item[0] == "expr":
                self.emit(item[1])
        return True


def cache_binding(name: str, ctype: str, index: int) -> str:
    "A cache operand's local, from the probe's array of words"
    if ctype == "_PyValueGuarded":
        return f"_PyValueGuarded {name} = {{caches[{index}]}};"
    if ctype.endswith("*"):
        return f"{ctype} {name} = ({ctype})(uintptr_t)caches[{index}];"
    return f"{ctype} {name} = ({ctype})caches[{index}];"


def generate_probe(uop: Uop, feedback: dict[str, str], emitter: ProbeEmitter,
                   out: CWriter) -> None:
    out.emit(f"static int\nleaf_probe_{uop.name}(_PyInterpreterFrame *frame, "
             "PyValue *stack_pointer, int oparg,\n"
             "    const uint64_t *caches, struct leaf_probe_result *result)\n{\n")
    out.emit("(void)frame; (void)oparg; (void)caches;\n")
    for index, cache in enumerate(c for c in uop.caches if c.name != "unused"):
        ctype = feedback.get(cache.name) or {1: "uint16_t", 2: "uint32_t"}.get(cache.size, "uint32_t")
        out.emit(cache_binding(cache.name, ctype, index) + "\n")
    names = dict.fromkeys(item.name for item in (*uop.stack.inputs, *uop.stack.outputs)
                          if item.name != "unused" and not item.is_array())
    if names:
        out.emit(f"PyValue {', '.join(names)};\n")
    # The interpreter's emitter checks ownership (TAKE, transfers); the
    # probe runs the body on a plain stack of words
    storage = Storage.for_uop(ProbeStack(), uop, out, check_liveness=False)
    reachable, storage = emitter.emit_tokens(uop, storage, None, False)
    if reachable:
        storage.stack.flush(out)
        out.emit("result->stack_pointer = stack_pointer;\n")
        out.emit("return LEAF_PROBE_NEXT;\n")
    out.emit("}\n\n")


def leaf_uops(analysis: Analysis) -> dict[str, tuple[Uop, dict[str, str]]]:
    "The uops inside the leaf subset, with their cache operands' C types"
    formats = ir_generator.compute_formats(analysis)
    records = ir_generator.build_records(analysis, formats)
    parts = {part.name: part
             for inst in ir_generator.handler_instructions(analysis).values()
             for part in inst.parts if isinstance(part, Uop)}
    result = {}
    for record in records.values():
        for uop in record.uops:
            if uop.leaf is not None and uop.name not in result:
                result[uop.name] = (parts[uop.name],
                                    {fb.name: fb.field.ctype for fb in uop.feedback})
    return result


def generate_probes(filenames: list[str], analysis: Analysis, outfile) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    emitter = ProbeEmitter(out, analysis)
    uops = leaf_uops(analysis)
    for name in sorted(uops):
        uop, feedback = uops[name]
        generate_probe(uop, feedback, emitter, out)
    out.emit("static const struct leaf_probe LEAF_PROBES[] = {\n")
    for name in sorted(uops):
        uop, _ = uops[name]
        inputs = sum(1 for item in uop.stack.inputs if not item.is_array())
        out.emit(f'{{"{name}", leaf_probe_{name}, {inputs}}},\n')
    out.emit("{NULL, NULL, 0},\n};\n")


arg_parser = argparse.ArgumentParser(description="Generate the leaf probes")
arg_parser.add_argument("-o", "--output", type=str, help="Generated code",
                        default=DEFAULT_OUTPUT)
arg_parser.add_argument("input", nargs=argparse.REMAINDER, help="Instruction definition file(s)")


if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)
    data = analyze_files(args.input)
    with open(args.output, "w") as outfile:
        generate_probes(args.input, data, outfile)
