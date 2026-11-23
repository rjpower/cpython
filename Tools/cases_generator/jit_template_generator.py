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
from dataclasses import dataclass, replace

import ir_generator
import ir_leaf
import ir_numbers
from analyzer import analyze_files
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
    out.write("\n#ifdef _PY_JIT_LOWERINGS_TABLE\n")
    out.write("const _PyJitLoweringInfo _PyJit_Lowerings[] = {\n")
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
    out.write("#endif\n")




# ---------------------------------------------------------------------------
# The template graph: the uops' leaf IR joined into one SSA graph of blocks
# with parameters, with the skeleton's operations added.  Ops are those of
# ir_leaf.Op plus:
#   view F          the word in the slot that field F names (an input)
#   detach F        the slot that field F names becomes NULL (its input's
#                   owner moved to the handler)
#   slot_load F     the slot's old value (a dst that may name a local)
#   slot_store F v  the slot that field F names := v
#   take v          v gets a count of its own (a local's input that the
#                   handler stores elsewhere): a DUP whose result is v
#   jconst          an integer known when the word is compiled: attr is
#                   its C expression over ir_word, fb and oparg
#   cmpmask a b     COMPARISON_BIT(a, b) & attr != 0, attr a C expression
#   xorb c          c, negated when attr (a C expression) is nonzero
# An op with `when` (a C expression) is emitted only for words where it
# holds: the slot-class flags of the word are known when it is compiled.
# Terminators: ("br", c, t, f), ("jmp", b, args), ("end", c or None) (the
# next word, or the target when c), ("end", None, True) (the target
# always), ("deopt",) (the word's own form) and ("raise",).


class Refused(Exception):
    "The handler keeps the call of its form; the message says why"


@dataclass
class TOp:
    kind: str
    results: list
    args: list
    attr: object = None
    when: str | None = None


@dataclass
class TBlock:
    id: int
    params: list
    ops: list
    term: tuple | None = None


class Graph:
    def __init__(self) -> None:
        self.types: dict[int, str] = {}
        self.blocks: dict[int, TBlock] = {}
        self.count = 0
        self.entry = self.new_block().id

    def value(self, t: str) -> int:
        self.count += 1
        self.types[self.count] = t
        return self.count

    def new_block(self, param_types=()) -> TBlock:
        b = TBlock(len(self.blocks), [self.value(t) for t in param_types], [])
        self.blocks[b.id] = b
        return b

    def successors(self, b: TBlock) -> list[int]:
        term = b.term
        if term[0] == "br":
            return [term[2], term[3]]
        if term[0] == "jmp":
            return [term[1]]
        return []

    def rpo(self) -> list[int]:
        seen, order = set(), []

        def visit(i):
            seen.add(i)
            for s in self.successors(self.blocks[i]):
                if s not in seen:
                    visit(s)
            order.append(i)
        visit(self.entry)
        return order[::-1]

    def preds(self) -> dict[int, list[int]]:
        out = {i: [] for i in self.blocks}
        for i in self.rpo():
            for s in self.successors(self.blocks[i]):
                out[s].append(i)
        return out

    def dump(self) -> str:
        lines = []
        for i in self.rpo():
            b = self.blocks[i]
            ps = ", ".join(f"%{p}:{self.types[p]}" for p in b.params)
            lines.append(f"b{i}({ps}):")
            for op in b.ops:
                res = ", ".join(f"%{r}" for r in op.results)
                args = ", ".join(f"%{a}" for a in op.args)
                when = f" [{op.when}]" if op.when else ""
                attr = "" if op.attr is None else f" {op.attr}"
                lines.append(f"  {res + ' = ' if res else ''}{op.kind}{attr} {args}{when}")
            lines.append(f"  {b.term}")
        return "\n".join(lines)


TEMP_ONLY = frozenset({"TEMP"})
# Ops with an effect: a guard may not follow one, nor may a raise
EFFECT_OPS = {"detach", "slot_store", "store", "take"}
COUNT_PRIMS = {"PyValue_DUP", "PyValue_XDUP", "PyValue_CLOSE", "PyValue_XCLOSE",
               "PyValue_CLOSE_SPECIALIZED"}
CLOSES = {"PyValue_CLOSE", "PyValue_XCLOSE", "PyValue_CLOSE_SPECIALIZED"}
INLINE_FACTS = {"INLINE_INT", "INLINE_FLOAT", "NUMBER"}


def temp_test(operand) -> str | None:
    "The C test that the slot of operand is a temporary, None if always"
    return None if operand.may_name == TEMP_ONLY else f"!(ir_word & IR_F_LOCAL_{operand.field})"


def local_test(operand) -> str:
    return f"(ir_word & IR_F_LOCAL_{operand.field})"


def unused_output(o) -> bool:
    "An output array that the uop leaves unwritten (its slots keep NULL)"
    return bool(o.array) and o.name == "unused"


def window_base(record):
    "The WINDOW operand of a handler whose window starts at its field"
    for o in record.operands:
        if o.kind == "WINDOW":
            if o.position != 0:
                raise Refused("a window that starts before its field")
            return o
    return None


def window_offset(record, entries, k) -> str:
    """The C offset from a window's first slot of entry k of entries, the
    window's values in order: the sum of the sizes of those before it, an
    array's size being its C expression of oparg"""
    if any(v is None for v in entries[:k]):
        raise Refused("a window entry of unknown size")
    sizes = [f"({v.array})" if v.array else "1" for v in entries[:k]]
    expr = " + ".join(sizes) or "0"
    if "oparg" not in expr:
        return str(eval(expr))
    return expr.replace("oparg", f"({oparg_expr(record)})")


def window_inputs(record) -> list:
    return sorted((v for v in record.values if v.external >= 0), key=lambda v: v.external)


def window_outputs(record) -> list:
    """The window's output entries up to its last result, by position: an
    entry that is no result keeps the input at its position"""
    values = {v.id: v for v in record.values}
    by_position = {o.position: values[vid] for vid, o in record.results}
    inputs = window_inputs(record)
    return [by_position.get(p, inputs[p] if p < len(inputs) else None)
            for p in range(max(by_position) + 1)]


def input_operand(record, k: int):
    for o in record.operands:
        if o.kind == "SLOT" and o.detail == f"input {k}":
            return o
    window = window_base(record)
    if window is not None:
        # An input of a window is a temporary at its offset from the field's slot
        offset = window_offset(record, window_inputs(record), k)
        return replace(window, kind="SLOT", field=f"{window.field}+{offset}", may_name=TEMP_ONLY,
                       position=0, detail=f"input {k}")
    raise Refused(f"input {k} is not in a slot of its own")


def oparg_expr(record) -> str:
    fmt = record.format
    source = record.oparg
    if source == "FUSED_OP":
        return "(int)(((_PyFbHeader *)fb)->arg & IR_FUSED_OP_MASK)"
    if source == "FBARG_ENTRY":
        return "(int)((_PyFbHeader *)fb)->arg"
    if source == "SIGNED_D":
        return "IR_DS(ir_word)"
    if source == "FIELD":
        return f"(int){ir_generator.field_expr(fmt.imm)}"
    raise Refused(f"oparg from {source}")


class Builder:
    """Join the uops of one record into a Graph"""

    def __init__(self, record) -> None:
        self.record = record
        self.g = Graph()
        self.env: dict[int, int] = {}       # record value -> graph value
        self.views: dict[int, object] = {}  # graph value -> its input's operand
        self.detached: set[int] = set()
        self.deferred: list[TOp] = []       # the owners' closes, after the stores
        self.effect = False                 # an effect has been emitted
        self.slot_effect = False            # one other than a store to memory
        self.deopt = None
        self.raises = None
        self.uses_fb = False
        self.inline: set[int] = set()       # values that own no count

    def add(self, block, kind, results, args, attr=None, when=None) -> TOp:
        op = TOp(kind, list(results), list(args), attr, when)
        block.ops.append(op)
        if kind in EFFECT_OPS or (kind == "prim" and attr in COUNT_PRIMS) or (
                kind == "prim" and isinstance(attr, tuple)):
            self.effect = True
            # A store to memory is the C's own: an error after it finds
            # the same state as the form's.  A slot's or a count's is not.
            self.slot_effect |= kind != "store"
        return op

    def exit_block(self, kind: str) -> int:
        if kind == "raise":
            if self.raises is None:
                self.raises = self.g.new_block()
                self.raises.term = ("raise",)
            return self.raises.id
        if self.deopt is None:
            self.deopt = self.g.new_block()
            self.deopt.term = ("deopt",)
        return self.deopt.id

    def build(self) -> Graph:
        r = self.record
        g = self.g
        cur = g.blocks[g.entry]
        for v in r.values:
            if v.external >= 0:
                if unused_output(v):
                    continue
                if v.array:
                    raise Refused("an array input")
                operand = input_operand(r, v.external)
                gv = g.value(ir_leaf.VALUE)
                self.add(cur, "view", [gv], [], operand.field)
                self.env[v.id] = gv
                self.views[gv] = operand
        branch = None
        for u in r.uops:
            cur, branch = self.uop(u, cur, branch)
        self.epilogue(cur, branch)
        return g

    def uop(self, u, cur, branch):
        g = self.g
        body = u.leaf
        # A value that a guard shows to be an inline number owns no count:
        # its close is nothing
        for guard in u.guards:
            for fact in guard.facts:
                if fact.kind in INLINE_FACTS and fact.subject[1:].isdigit():
                    v = self.env.get(int(fact.subject[1:]))
                    if v is not None:
                        self.inline.add(v)
        lmap: dict[int, int] = {}
        bmap = {}
        for b in body.blocks:
            gb = g.new_block([body.types[p] for p in b.params])
            bmap[b.id] = gb
            for p, gp in zip(b.params, gb.params):
                lmap[p] = gp
        outputs = [o for o in u.outputs if not unused_output(o)]
        out_types = [g.types[self.env[o.value]] if o.value in self.env and o.ownership == "THROUGH"
                     else ir_leaf.VALUE for o in outputs]
        merge = g.new_block(out_types + ["bool"])
        inputs = {i.name: i for i in u.inputs}
        # The implicit object guards, which the forms put before the body
        # (IREmitter.emit_object_inputs()): an input that the uop reads as
        # an object and that may hold an inline value
        for guard in u.guards:
            if guard.site is not None:
                continue
            if self.effect:
                raise Refused(f"{u.name}: an exit after an effect")
            is_object = g.value("bool")
            self.add(cur, "prim", [is_object], [self.env[inputs[guard.input].value]],
                     "PyValue_IsPointerSpace")
            nxt = g.new_block()
            cur.term = ("br", is_object, nxt.id, self.exit_block("deopt"))
            cur = nxt
        cur.term = ("jmp", bmap[body.blocks[0].id].id, [])
        for b in body.blocks:
            gb = bmap[b.id]
            outs: dict[str, int] = {}
            for op in b.ops:
                gb = self.op(op, body, gb, lmap, inputs, outs)
            term = b.term
            kind = term[0]
            if kind == "br":
                gb.term = ("br", lmap[term[1]], bmap[term[2]].id, bmap[term[3]].id)
            elif kind == "jmp":
                gb.term = ("jmp", bmap[term[1]].id, [lmap[a] for a in term[2]])
            elif kind == "next":
                args = []
                for o in outputs:
                    if o.array:
                        raise Refused("an array output")
                    if o.name in outs:
                        args.append(outs[o.name])
                    elif o.ownership == "THROUGH" and o.value in self.env:
                        args.append(self.env[o.value])
                    else:
                        raise Refused(f"{u.name}: output {o.name} is not written on a path")
                flag = branch
                if term[1] is not None:
                    if branch is not None:
                        raise Refused("two branches")
                    flag = lmap[term[1]]
                if flag is None:
                    flag = g.value("bool")
                    self.add(gb, "const", [flag], [], 0)
                gb.term = ("jmp", merge.id, args + [flag])
            elif kind in ("raise", "exit", "complete"):
                # A completion (a loop's end) is rare: the word's form, run
                # from the start, completes it
                if self.slot_effect if kind == "raise" else self.effect:
                    raise Refused(f"{u.name}: an exit after an effect")
                gb.term = ("jmp", self.exit_block("raise" if kind == "raise" else "deopt"), [])
            else:
                raise Refused(f"{u.name}: a {kind} end")
        any_branch = any(b.term[0] == "next" and b.term[1] is not None for b in body.blocks)
        # An output that every path gives the same value is that value: an
        # input passed through or aliased stays the view of its slot
        incoming = [set() for _ in outputs]
        for b in g.blocks.values():
            if b.term is not None and b.term[0] == "jmp" and b.term[1] == merge.id:
                for s, a in zip(incoming, b.term[2]):
                    s.add(a)
        for o, p, s in zip(outputs, merge.params, incoming):
            self.env[o.value] = s.pop() if len(s) == 1 else p
        new_branch = merge.params[-1] if (any_branch or branch is not None) else None
        # The consumed inputs that live in slots are detached after the uop
        for i in u.inputs:
            if i.ownership != "CONSUMED":
                continue
            gv = self.view_of(i.value)
            if gv is None or gv in self.detached:
                continue
            operand = self.views[gv]
            self.detached.add(gv)
            if "TEMP" in operand.may_name:
                self.add(merge, "detach", [], [], operand.field, temp_test(operand))
        return merge, new_branch

    def view_of(self, record_value: int) -> int | None:
        gv = self.env.get(record_value)
        return gv if gv in self.views else None

    def op(self, op, body, gb, lmap, inputs, outs):
        g = self.g
        k = op.kind
        args = [lmap[a] for a in op.args]
        if k == "arg":
            lmap[op.results[0]] = self.env[inputs[op.attr].value]
            return gb
        results = []
        for res in op.results:
            results.append(g.value(body.types[res]))
            lmap[res] = results[-1]
        if k == "out":
            outs[op.attr] = args[0]
            return gb
        if k == "report_edge":
            return gb
        if k == "oparg":
            self.uses_fb |= self.record.oparg in ("FUSED_OP", "FBARG_ENTRY")
            self.add(gb, "jconst", results, [], oparg_expr(self.record))
            return gb
        if k == "imm":
            if op.attr != "fused_if_false":
                raise Refused(f"immediate {op.attr}")
            self.uses_fb = True
            self.add(gb, "jconst", results, [], "((int)(((_PyFbHeader *)fb)->arg >> 8) & 1)")
            return gb
        if k == "cache":
            self.uses_fb = True
            self.add(gb, "cache", results, [], op.attr)
            return gb
        if k in ("exit_if", "raise_if"):
            if self.slot_effect if k == "raise_if" else self.effect:
                raise Refused(f"a {k} after an effect")
            cont = g.new_block()
            gb.term = ("br", args[0], self.exit_block("raise" if k == "raise_if" else "deopt"),
                       cont.id)
            return cont
        if k == "prim":
            name = op.attr[0] if isinstance(op.attr, tuple) else op.attr
            if name in CLOSES and args[0] in self.views:
                operand = self.views[args[0]]
                consumed_here = any(i.ownership == "CONSUMED" and self.env.get(i.value) == args[0]
                                    for i in inputs.values())
                if args[0] not in self.detached and not consumed_here:
                    raise Refused("a close of an input that no uop consumes")
                if "TEMP" in operand.may_name and args[0] not in self.inline:
                    self.deferred.append(TOp("prim", [], args, op.attr, temp_test(operand)))
                return gb
            if name != "COMPARISON_BIT" and name not in LOWERINGS:
                raise Refused(f"no lowering of {name}")
        if k == "store" and args[1] in self.views:
            operand = self.views[args[1]]
            if operand.may_name != TEMP_ONLY:
                self.add(gb, "take", [], [args[1]], None, local_test(operand))
        if k == "call":
            for a in args:
                if a in self.views:
                    raise Refused(f"an input passed to {op.attr}")
        self.add(gb, k, results, args, op.attr)
        return gb

    def window_result(self, cur, vid, result) -> None:
        """A result in the window's temporaries (IR_SET_TEMP), or the one
        that moves to dst when dst names a local (IR_MOVE_RESULT_TO_DST)"""
        r = self.record
        window = window_base(r)
        if window is None or result.field != window.field:
            raise Refused("a result in another window")
        v = self.env[vid]
        # An input's value may move to another of the window's slots, as
        # SWAP's do: the form rewrites every slot whose value moves
        offset = window_offset(r, window_outputs(r), result.position)
        slot = f"{window.field}+{offset}"
        if r.layout.move_dst != result.position:
            self.add(cur, "slot_store", [], [v], slot)
            return
        dst = next(o for o in r.operands if o.kind == "SLOT" and o.field == r.layout.move_dst_args.split(", ")[1])
        o = self.g.value(ir_leaf.VALUE)
        self.add(cur, "slot_load", [o], [], dst.field, local_test(dst))
        self.add(cur, "slot_store", [], [v], dst.field, local_test(dst))
        self.add(cur, "slot_store", [], [v], slot, f"!{local_test(dst)}")
        self.old_values.append((o, dst))

    def epilogue(self, cur, branch):
        r = self.record
        old = self.old_values = []
        values = {v.id: v for v in r.values}
        for vid, result in r.results:
            if unused_output(values[vid]):
                continue
            if result.kind == "WINDOW":
                self.window_result(cur, vid, result)
                continue
            if result.kind != "SLOT":
                raise Refused(f"a result in a {result.kind}")
            # The record's operand of the field says what it may name.  A
            # result after the first goes to the slots that follow dst's,
            # which are temporaries, as is dst for a handler that never
            # stores to a local.
            operand = next(o for o in r.operands if o.kind == "SLOT" and o.field == result.field)
            field = operand.field if result.position == 0 else f"{operand.field}+{result.position}"
            may_be_local = (result.position == 0 and r.store_mode != "never" and
                            ("LOCAL" in operand.may_name or "CONST" in operand.may_name))
            if r.store_mode not in (None, "store", "never"):
                raise Refused(f"store mode {r.store_mode}")
            v = self.env[vid]
            if v in self.views:
                src = self.views[v]
                if src.may_name != TEMP_ONLY:
                    self.add(cur, "take", [], [v], None, local_test(src))
            if may_be_local:
                o = self.g.value(ir_leaf.VALUE)
                self.add(cur, "slot_load", [o], [], field, local_test(operand))
                old.append((o, operand))
            self.add(cur, "slot_store", [], [v], field)
        # A slot that the end stores needs no detach: every path from a
        # detach reaches the end, as no exit follows an effect
        stored = {op.attr for op in cur.ops if op.kind == "slot_store" and op.when is None}
        for b in self.g.blocks.values():
            b.ops = [op for op in b.ops if not (op.kind == "detach" and op.attr in stored)]
        cur.ops.extend(self.deferred)
        for o, operand in old:
            self.add(cur, "prim", [], [o], "PyValue_XCLOSE", local_test(operand))
        cur.term = ("end", branch)


# ---------------------------------------------------------------------------
# Simplification

PURE = {"const", "jconst", "view", "cache", "load", "fieldaddr", "ptradd", "ptrdiff",
        "add", "sub", "mul", "and", "or", "xor", "shl", "ashr", "lshr", "neg", "not",
        "lnot", "fadd", "fsub", "fmul", "fdiv", "fneg", "conv", "cmp", "fcmp", "select",
        "add_ovf", "sub_ovf", "mul_ovf", "cmpmask", "xorb", "slot_load"}
# Primitives without an effect: removable when unused
PURE_PRIMS = {name for name, l in LOWERINGS.items()
              if name not in COUNT_PRIMS and name not in ("PyValue_FromInt64",
                                                          "PyValue_FromDouble")}
PURE_PRIMS.add("COMPARISON_BIT")


def is_pure(op: TOp) -> bool:
    if op.kind == "prim":
        return op.attr in PURE_PRIMS
    return op.kind in PURE


def substitute(g: Graph, mapping: dict[int, int]) -> None:
    def m(v):
        while v in mapping:
            v = mapping[v]
        return v
    for b in g.blocks.values():
        for op in b.ops:
            op.args = [m(a) for a in op.args]
        t = b.term
        if t[0] == "br":
            b.term = ("br", m(t[1]), t[2], t[3])
        elif t[0] == "jmp":
            b.term = ("jmp", t[1], [m(a) for a in t[2]])
        elif t[0] == "end" and t[1] is not None:
            b.term = ("end", m(t[1]))


def prune(g: Graph) -> None:
    live = set(g.rpo())
    for i in list(g.blocks):
        if i not in live:
            del g.blocks[i]


def merge_chains(g: Graph) -> bool:
    changed = False
    while True:
        preds = g.preds()
        for i in g.rpo():
            b = g.blocks[i]
            if b.term[0] != "jmp":
                continue
            c = g.blocks[b.term[1]]
            if c.id == g.entry or preds[c.id] != [i]:
                continue
            substitute(g, dict(zip(c.params, b.term[2])))
            b.ops.extend(c.ops)
            b.term = c.term
            del g.blocks[c.id]
            changed = True
            break
        else:
            return changed


def thread_jumps(g: Graph) -> bool:
    "Branch past an empty block that only jumps on without arguments"
    def through(i: int) -> int:
        seen = set()
        while i not in seen:
            seen.add(i)
            b = g.blocks[i]
            if b.ops or b.params or b.term[0] != "jmp" or b.term[2] or i == g.entry:
                return i
            i = b.term[1]
        return i

    changed = False
    for b in g.blocks.values():
        t = b.term
        if t[0] == "br":
            new = ("br", t[1], through(t[2]), through(t[3]))
        elif t[0] == "jmp":
            new = ("jmp", through(t[1]), t[2])
        else:
            continue
        if new != t:
            b.term = new
            changed = True
    return changed


def definitions(g: Graph) -> dict[int, TOp]:
    return {r: op for b in g.blocks.values() for op in b.ops for r in op.results}


def uses(g: Graph) -> dict[int, int]:
    n: dict[int, int] = {}
    for b in g.blocks.values():
        for op in b.ops:
            for a in op.args:
                n[a] = n.get(a, 0) + 1
        t = b.term
        for a in ([t[1]] if t[0] == "br" else t[2] if t[0] == "jmp" else
                  [t[1]] if t[0] == "end" and t[1] is not None else []):
            n[a] = n.get(a, 0) + 1
    return n


def const_of(defs, v):
    op = defs.get(v)
    if op is not None and op.kind == "const" and isinstance(op.attr, int) and op.when is None:
        return op.attr
    return None


_CMP_PY = {"eq": lambda a, b: a == b, "ne": lambda a, b: a != b,
           "slt": lambda a, b: a < b, "sle": lambda a, b: a <= b,
           "sgt": lambda a, b: a > b, "sge": lambda a, b: a >= b}


def fold(g: Graph) -> bool:
    "Constants, parameters with one incoming value, and the bool rewrites"
    changed = False
    defs = definitions(g)
    # Parameters that receive one value (or themselves)
    incoming: dict[int, set] = {}
    for b in g.blocks.values():
        if b.term[0] == "jmp":
            for p, a in zip(g.blocks[b.term[1]].params, b.term[2]):
                incoming.setdefault(p, set()).add(a)
    mapping = {}
    for p, vals in incoming.items():
        vals = vals - {p}
        if len(vals) == 1:
            mapping[p] = vals.pop()
        elif all(const_of(defs, v) is not None for v in vals) and \
                len({const_of(defs, v) for v in vals}) == 1 and len(vals) > 0:
            pass
    if mapping:
        for b in g.blocks.values():
            keep = [i for i, p in enumerate(b.params) if p not in mapping]
            if len(keep) != len(b.params):
                b.params = [b.params[i] for i in keep]
                for pb in g.blocks.values():
                    if pb.term[0] == "jmp" and pb.term[1] == b.id:
                        pb.term = ("jmp", b.id, [pb.term[2][i] for i in keep])
        substitute(g, mapping)
        changed = True
        defs = definitions(g)
    for b in g.blocks.values():
        # An end whose branch is a constant goes one way
        if b.term[0] == "end" and b.term[1] is not None:
            k = const_of(defs, b.term[1])
            if k is not None:
                b.term = ("end", None, True) if k else ("end", None)
                changed = True
        for op in b.ops:
            k = op.kind
            a = op.args
            if k == "lnot" and const_of(defs, a[0]) is not None:
                op.kind, op.args, op.attr = "const", [], int(not const_of(defs, a[0]))
                changed = True
            elif k == "cmp" and op.attr in _CMP_PY and all(const_of(defs, x) is not None for x in a):
                op.kind, op.args, op.attr = "const", [], int(_CMP_PY[op.attr](
                    const_of(defs, a[0]), const_of(defs, a[1])))
                changed = True
            elif k == "conv" and const_of(defs, a[0]) is not None and \
                    g.types[op.results[0]] != "f64" and g.types[a[0]] != "f64":
                op.kind, op.args, op.attr = "const", [], ir_leaf.wrap(
                    g.types[op.results[0]], const_of(defs, a[0]))
                changed = True
            elif k == "prim" and op.attr == "PyValue_IsTrue":
                # IsTrue(c ? True : False) is c
                d = defs.get(a[0])
                if d is not None and d.kind == "select" and \
                        defs[d.args[1]].attr == "PyValue_True" and \
                        defs[d.args[2]].attr == "PyValue_False":
                    op.kind, op.args, op.attr = "xorb", [d.args[0]], "0"
                    changed = True
            elif k == "cmp" and op.attr in ("ne", "eq") and const_of(defs, a[1]) == 0:
                d = defs.get(a[0])
                negate = op.attr == "eq"
                if g.types[a[0]] == "bool":
                    rewrite = ("xorb", [a[0]], "1" if negate else "0")
                elif d is not None and d.kind == "conv" and g.types[d.args[0]] == "bool":
                    rewrite = ("xorb", [d.args[0]], "1" if negate else "0")
                elif d is not None and d.kind == "xor":
                    # (conv b) ^ k with k known when the word is compiled
                    x, kv = d.args
                    dx, dk = defs.get(x), defs.get(kv)
                    if dk is not None and dk.kind != "jconst" and dx is not None and dx.kind == "jconst":
                        x, kv, dx, dk = kv, x, dk, dx
                    if (dx is not None and dx.kind == "conv" and g.types[dx.args[0]] == "bool"
                            and dk is not None and dk.kind in ("jconst", "const")):
                        expr = str(dk.attr)
                        rewrite = ("xorb", [dx.args[0]],
                                   f"!({expr})" if negate else f"({expr})")
                    else:
                        rewrite = None
                elif d is not None and d.kind == "and":
                    # COMPARISON_BIT(x, y) & mask
                    x, m = d.args
                    dx, dm = defs.get(x), defs.get(m)
                    if dx is not None and dx.kind == "prim" and dx.attr == "COMPARISON_BIT" \
                            and dm is not None and dm.kind in ("jconst", "const"):
                        rewrite = ("cmpmask", list(dx.args),
                                   f"!!({dm.attr})" if not negate else f"!({dm.attr})")
                        if negate:
                            rewrite = None
                    else:
                        rewrite = None
                else:
                    rewrite = None
                if rewrite is not None:
                    op.kind, op.args, op.attr = rewrite
                    if op.kind == "cmpmask":
                        op.attr = str(defs[d.args[1]].attr)
                    changed = True
            elif k == "xorb":
                d = defs.get(a[0])
                if d is not None and d.kind == "xorb":
                    op.args, op.attr = list(d.args), f"(({op.attr}) ^ ({d.attr}))"
                    changed = True
                elif d is not None and d.kind == "lnot":
                    op.args, op.attr = list(d.args), f"!({op.attr})"
                    changed = True
                elif op.attr == "0" and d is not None:
                    pass
        t = b.term
        if t[0] == "br":
            c = const_of(defs, t[1])
            if c is not None:
                b.term = ("jmp", t[2] if c else t[3], [])
                changed = True
                continue
            d = defs.get(t[1])
            if d is not None and d.kind == "lnot":
                b.term = ("br", d.args[0], t[3], t[2])
                changed = True
    return changed


def thread_bool_phis(g: Graph) -> bool:
    """br c, T, F where T and F only pass constants to the same J: J's
    parameter that gets 1 and 0 is c"""
    defs = definitions(g)
    preds = g.preds()
    for i in g.rpo():
        b = g.blocks[i]
        if b.term[0] != "br":
            continue
        c, ti, fi = b.term[1], b.term[2], b.term[3]
        T, F = g.blocks[ti], g.blocks[fi]
        if ti == fi or preds[ti] != [i] or preds[fi] != [i]:
            continue
        if T.term[0] != "jmp" or F.term[0] != "jmp" or T.term[1] != F.term[1]:
            continue
        if any(op.kind != "const" for op in T.ops + F.ops):
            continue
        J = g.blocks[T.term[1]]
        if sorted(preds[J.id]) != sorted([ti, fi]):
            continue
        mapping, ok, extra = {}, True, []
        for p, x, y in zip(J.params, T.term[2], F.term[2]):
            cx, cy = const_of(defs, x), const_of(defs, y)
            if x == y:
                mapping[p] = x
            elif g.types[p] == "bool" and (cx, cy) == (1, 0):
                mapping[p] = c
            elif g.types[p] == "bool" and (cx, cy) == (0, 1):
                n = g.value("bool")
                extra.append(TOp("xorb", [n], [c], "1"))
                mapping[p] = n
            elif g.types[p] == ir_leaf.VALUE and defs.get(x) is not None and \
                    defs.get(y) is not None and defs[x].kind == "const" and \
                    defs[y].kind == "const" and isinstance(defs[x].attr, str) and \
                    isinstance(defs[y].attr, str):
                # c ? K1 : K2 of two word constants
                kx, ky, n = g.value(ir_leaf.VALUE), g.value(ir_leaf.VALUE), g.value(ir_leaf.VALUE)
                extra += [TOp("const", [kx], [], defs[x].attr), TOp("const", [ky], [], defs[y].attr),
                          TOp("select", [n], [c, kx, ky])]
                mapping[p] = n
            else:
                ok = False
        if not ok:
            continue
        J.params = []
        J.ops[:0] = extra
        substitute(g, mapping)
        b.term = ("jmp", J.id, [])
        prune(g)
        return True
    return False


def dominators(g: Graph) -> dict[int, int]:
    "The immediate dominator of each block but the entry"
    order = g.rpo()
    index = {b: n for n, b in enumerate(order)}
    preds = g.preds()
    idom = {g.entry: g.entry}

    def meet(a: int, b: int) -> int:
        while a != b:
            while index[a] > index[b]:
                a = idom[a]
            while index[b] > index[a]:
                b = idom[b]
        return a

    changed = True
    while changed:
        changed = False
        for b in order[1:]:
            done = [p for p in preds[b] if p in idom]
            new = done[0]
            for p in done[1:]:
                new = meet(p, new)
            if idom.get(b) != new:
                idom[b] = new
                changed = True
    del idom[g.entry]
    return idom


# The ops that compute the same result from the same arguments anywhere:
# not loads, which a store between may change, nor the view of a slot
CSE_KINDS = {"const", "jconst", "cache", "fieldaddr", "ptradd", "add", "sub", "mul", "and",
             "or", "xor", "shl", "ashr", "lshr", "neg", "not", "conv", "fadd", "fsub",
             "fmul", "fdiv"}


def cse(g: Graph) -> bool:
    "Reuse the result of an equal pure op in a dominating position"
    idom = dominators(g)
    children: dict[int, list[int]] = {}
    for b, d in idom.items():
        children.setdefault(d, []).append(b)
    mapping: dict[int, int] = {}

    def walk(i: int, seen: dict) -> None:
        seen = dict(seen)
        b = g.blocks[i]
        keep = []
        for op in b.ops:
            pure = (op.kind in CSE_KINDS or
                    (op.kind == "prim" and isinstance(op.attr, str) and op.attr in PURE_PRIMS
                     and op.results))
            if not pure and not is_pure(op):
                # An effect: what read memory before it may differ after
                seen = {k: v for k, v in seen.items()
                        if not (k[0] == "prim" and ir_leaf.PRIMITIVES[k[1]].effects)}
            if pure and op.when is None and len(op.results) == 1:
                key = (op.kind, op.attr, tuple(mapping.get(a, a) for a in op.args))
                if key in seen:
                    mapping[op.results[0]] = seen[key]
                    continue
                seen[key] = op.results[0]
            keep.append(op)
        b.ops = keep
        for c in children.get(i, []):
            walk(c, seen)

    walk(g.entry, {})
    if mapping:
        substitute(g, mapping)
    return bool(mapping)


def dce(g: Graph) -> bool:
    changed = False
    while True:
        n = uses(g)
        removed = False
        for b in g.blocks.values():
            keep = []
            for op in b.ops:
                if is_pure(op) and op.results and not any(n.get(r) for r in op.results):
                    removed = True
                    continue
                keep.append(op)
            b.ops = keep
        if not removed:
            return changed
        changed = True


def simplify(g: Graph) -> None:
    prune(g)
    while True:
        changed = merge_chains(g)
        changed |= thread_jumps(g)
        changed |= fold(g)
        changed |= thread_bool_phis(g)
        prune(g)
        changed |= cse(g)
        changed |= dce(g)
        if not changed:
            return


def load_records():
    analysis = analyze_files([DEFAULT_INPUT])
    formats = ir_generator.compute_formats(analysis)
    return analysis, ir_generator.build_records(analysis, formats)


# ---------------------------------------------------------------------------
# Flags, registers and emission

GPR_NAMES = ["RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI",
             "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"]
GPR_POOL = [0, 1, 2, 6, 7, 8, 9, 10, 11]
RCX = 1
SHIFTS = {"shl": "SHL", "ashr": "SAR", "lshr": "SHR"}
XMM_POOL = list(range(16))
CMP_CONDS = {"eq": "E", "ne": "NE", "slt": "L", "sle": "LE", "sgt": "G", "sge": "GE",
             "ult": "B", "ule": "BE", "ugt": "A", "uge": "AE"}
ALU = {"add": "ADD", "sub": "SUB", "and": "AND", "or": "OR", "xor": "XOR"}
OVF = {"add_ovf": "ADD", "sub_ovf": "SUB"}


def is_xmm(t: str) -> bool:
    return t == "f64"


def flag_producer(g: Graph, op: TOp) -> bool:
    "op sets the flags for its bool result (the last for checked arithmetic)"
    if op.kind == "prim":
        name = op.attr[0] if isinstance(op.attr, tuple) else op.attr
        return name in LOWERINGS and is_predicate(ir_leaf.PRIMITIVES[name])
    return op.kind in ("cmp", "fcmp", "cmpmask", "xorb", "lnot", "add_ovf", "sub_ovf",
                       "mul_ovf")


def cond_result(op: TOp) -> int:
    return op.results[-1] if op.kind.endswith("_ovf") else op.results[0]


def reads_memory(op: TOp) -> bool:
    if op.kind == "prim":
        return bool(ir_leaf.PRIMITIVES[op.attr].effects)
    return op.kind not in ("cmp", "fcmp", "cmpmask", "xorb", "lnot")


def sink(g: Graph) -> None:
    """Move each flag producer with one use in its block to just before that
    use, so that the flags reach it"""
    n = uses(g)
    for b in g.blocks.values():
        moved = True
        while moved:
            moved = False
            for i, op in enumerate(b.ops):
                if not flag_producer(g, op) or op.kind.endswith("_ovf") or op.when:
                    continue
                r = op.results[0]
                if n.get(r) != 1:
                    continue
                users = [j for j, o in enumerate(b.ops) if r in o.args]
                if users:
                    j = users[0]
                else:
                    t = b.term
                    if not ((t[0] == "br" and t[1] == r) or (t[0] == "end" and t[1] == r)):
                        continue
                    j = len(b.ops)
                if j == i + 1:
                    continue
                if any(not is_pure(o) for o in b.ops[i + 1:j]) and reads_memory(op):
                    continue
                b.ops.insert(j, b.ops.pop(i))
                moved = True
                break


def flag_values(g: Graph) -> set[int]:
    "Bool results that stay in the flags: used once, by what follows them"
    n = uses(g)
    out = set()
    for b in g.blocks.values():
        for i, op in enumerate(b.ops):
            if not flag_producer(g, op):
                continue
            r = cond_result(op)
            if n.get(r) != 1:
                continue
            if i + 1 < len(b.ops):
                nxt = b.ops[i + 1]
                if r in nxt.args and nxt.kind in ("xorb", "lnot", "select") and \
                        nxt.args[0] == r and not nxt.when:
                    out.add(r)
            else:
                t = b.term
                if (t[0] in ("br", "end")) and t[1] == r:
                    out.add(r)
    return out


def term_args(t) -> list[int]:
    if t[0] == "br":
        return [t[1]]
    if t[0] == "jmp":
        return list(t[2])
    if t[0] == "end" and t[1] is not None:
        return [t[1]]
    return []


def liveness(g: Graph) -> dict[int, set]:
    live_in: dict[int, set] = {i: set() for i in g.blocks}
    order = g.rpo()
    changed = True
    while changed:
        changed = False
        for i in reversed(order):
            b = g.blocks[i]
            live = set(term_args(b.term))
            for s in g.successors(b):
                live |= live_in[s] - set(g.blocks[s].params)
            for op in reversed(b.ops):
                live -= set(op.results)
                live |= set(op.args)
            live |= set()
            new = live - set(b.params) | set()
            new = live
            if new != live_in[i]:
                live_in[i] = new
                changed = True
    return live_in


class Allocation:
    def __init__(self, g: Graph, flags: set[int]) -> None:
        self.g = g
        self.flags = flags
        self.reg: dict[int, int] = {}
        self.live_in = liveness(g)
        self.live_after: dict[int, list[set]] = {}
        for i in g.blocks:
            self.block_live(i)
        self.forbidden = self.clobbered(definitions(g))
        self.assign()

    def clobbered(self, defs) -> dict[int, set[int]]:
        """The registers each value may not take because an op that its
        live range spans writes them: a shift by a variable count takes
        its count in cl"""
        forbidden: dict[int, set[int]] = {}
        for i, b in self.g.blocks.items():
            for op, after in zip(b.ops, self.live_after[i]):
                if op.kind in SHIFTS and const_of(defs, op.args[1]) is None:
                    count = op.args[1]
                    for v in (after | set(op.results) | {op.args[0]}) - {count}:
                        forbidden.setdefault(v, set()).add(RCX)
        return forbidden

    def needs_reg(self, v: int) -> bool:
        return v not in self.flags

    def block_live(self, i: int) -> None:
        g = self.g
        b = g.blocks[i]
        live = set(term_args(b.term))
        for s in g.successors(b):
            live |= self.live_in[s] - set(g.blocks[s].params)
        after = []
        for op in reversed(b.ops):
            after.append(set(live))
            live -= set(op.results)
            live |= set(op.args)
        after.reverse()
        self.live_after[i] = after

    def kind(self, v: int) -> str:
        return "x" if is_xmm(self.g.types[v]) else "g"

    def regs_of(self, values, kind) -> set[int]:
        return {self.reg[v] for v in values if v in self.reg and self.kind(v) == kind}

    def pick(self, v: int, busy: set[int], prefer=()) -> None:
        kind = self.kind(v)
        if kind == "g":
            busy = busy | self.forbidden.get(v, set())
        pool = XMM_POOL if kind == "x" else GPR_POOL
        for r in prefer:
            if r in pool and r not in busy:
                self.reg[v] = r
                return
        for r in pool:
            if r not in busy:
                self.reg[v] = r
                return
        raise Refused("out of registers")

    def assign(self) -> None:
        g = self.g
        for i in g.rpo():
            b = g.blocks[i]
            entry_live = self.live_in[i]
            for p in b.params:
                if self.needs_reg(p):
                    k = self.kind(p)
                    self.pick(p, self.regs_of(entry_live - {p}, k))
            for op, after in zip(b.ops, self.live_after[i]):
                for j, r in enumerate(op.results):
                    if not self.needs_reg(r):
                        continue
                    k = self.kind(r)
                    busy = self.regs_of(after - {r}, k) | self.regs_of(op.args[1:], k)
                    busy |= self.regs_of(op.results[:j], k)
                    prefer = []
                    if op.args and op.args[0] in self.reg and op.args[0] not in after:
                        prefer = [self.reg[op.args[0]]]
                    if op.kind in ("select", "ptradd"):
                        busy |= self.regs_of(op.args, k)
                        prefer = []
                    self.pick(r, busy, prefer)


def c_int(x: int) -> str:
    x = ir_leaf.s64(x)
    if -2**31 <= x < 2**31:
        return str(x)
    return f"(int64_t){ir_leaf.u64(x):#x}ULL"


def const_expr(t: str, x) -> str:
    if t == ir_leaf.VALUE and isinstance(x, str):
        symbol, tag = ir_leaf.VALUE_CONSTANTS[x]
        return "0" if symbol is None else f"(int64_t)((uintptr_t)&{symbol} | {tag})"
    if isinstance(x, str) and x.startswith("&"):
        return f"(int64_t)(uintptr_t){x}"
    if isinstance(x, str):
        return f"(int64_t)({x})"
    if isinstance(x, float):
        return c_int(ir_leaf.f64_bits(x))
    return c_int(x)


def mem_size(t: str) -> tuple[int, bool]:
    "bytes and signedness of a memory access of type t"
    if t in (ir_leaf.VALUE, "f64") or ir_leaf.is_pointer(t) or t in ("i64", "u64"):
        return 8, False
    return ir_leaf.bits_of(t) // 8, ir_leaf.is_signed(t)


def field_offset(attr) -> tuple[str, str]:
    "The C offset and the type of a load or store"
    if attr in ir_leaf.FIELDS:
        f = ir_leaf.FIELDS[attr]
        return f"offsetof({f.struct}, {f.name})", f.type
    return "0", attr


def immediates(g: Graph, defs) -> set[int]:
    """The integer constants that fit an imm32 and that only ops taking
    their second operand as an immediate use: they are never in a register"""
    candidates = {r for r, op in defs.items() if op.kind == "const" and
                  (k := const_of(defs, r)) is not None and -2**31 <= k < 2**31 and
                  g.types[r] != "bool"}
    for b in g.blocks.values():
        for op in b.ops:
            for n, a in enumerate(op.args):
                if a in candidates and not ((op.kind == "cmp" or op.kind in ALU) and n == 1
                                            and op.args[0] != a and op.when is None):
                    candidates.discard(a)
        t = b.term
        for a in ([t[1]] if t[0] in ("br", "end") else t[2] if t[0] == "jmp" else []):
            candidates.discard(a)
    return candidates


class Emitter:
    def __init__(self, name: str, record, g: Graph, uses_fb: bool) -> None:
        self.name = name
        self.record = record
        self.g = g
        sink(g)
        self.flags = flag_values(g)
        self.alloc = Allocation(g, self.flags)
        self.lines: list[str] = []
        self.uses_fb = uses_fb
        self.defs = definitions(g)
        self.imms = immediates(g, self.defs)
        self.order: list[int] = []
        self.cold: set[int] = set()
        self.temp_count = 0

    # Text
    def w(self, line: str) -> None:
        self.lines.append("    " * self.indent + line)

    def R(self, v: int) -> str:
        r = self.alloc.reg[v]
        return f"_PyJitXmm_{r}" if self.alloc.kind(v) == "x" else f"_PyJitReg_{GPR_NAMES[r]}"

    def gpr(self, r: int) -> str:
        return f"_PyJitReg_{GPR_NAMES[r]}"

    def cond(self, v: int) -> str:
        if v in self.flags:
            return f"c{v}"
        self.w(f"_PyJitAsm_TestRR(a, {self.R(v)}, {self.R(v)});")
        return "_PyJitCond_NE"

    def label(self, i: int) -> str:
        t = self.g.blocks[i].term
        if t[0] == "deopt":
            return "cx->slow"
        if t[0] == "raise":
            return "raise"
        return f"L{i}"

    def is_exit(self, i: int) -> bool:
        return self.g.blocks[i].term[0] in ("deopt", "raise")

    # Layout: hot blocks in order, cold ones (with a helper call, or only
    # leading to cold ones) after them
    def layout(self) -> None:
        g = self.g
        order = [i for i in g.rpo() if not self.is_exit(i)]
        cold = set()
        for i in reversed(order):
            b = g.blocks[i]
            if any(op.kind == "call" for op in b.ops):
                cold.add(i)
                continue
            succ = [s for s in g.successors(b) if not self.is_exit(s)]
            if b.term[0] in ("br", "jmp") and succ and all(s in cold for s in succ):
                cold.add(i)
        cold.discard(g.entry)
        # The end is the last hot block: the template falls through from it
        # to the code after the template
        ends = [i for i in order if g.blocks[i].term[0] == "end"]
        assert len(ends) == 1 and ends[0] not in cold, ends
        hot = [i for i in order if i not in cold and i != ends[0]] + ends
        self.order = hot + [i for i in order if i in cold]
        self.cold = cold

    def temps(self, i: int, k: int, op: TOp, count: int, kind="g") -> list[int]:
        after = self.alloc.live_after[i][k]
        busy = self.alloc.regs_of(after | set(op.args) | set(op.results), kind)
        pool = XMM_POOL if kind == "x" else GPR_POOL
        free = [r for r in pool if r not in busy]
        if len(free) < count:
            raise Refused("out of registers for temporaries")
        return free[:count]

    def live_masks(self, i: int, k: int, op: TOp, written=frozenset()) -> tuple[int, int]:
        after = self.alloc.live_after[i][k] - set(op.results) - set(written)
        g = sum(1 << r for r in self.alloc.regs_of(after, "g"))
        x = sum(1 << r for r in self.alloc.regs_of(after, "x"))
        return g, x

    def emit(self) -> list[str]:
        g = self.g
        self.layout()
        self.indent = 1
        out_header = [f"static void\njit_template_{self.name}(_PyJitWordCx *cx)\n{{",
                      "    _PyJitAsm *a = cx->a;",
                      "    _PyIRWord ir_word = cx->word;",
                      "    (void)ir_word;"]
        fmt = self.record.format
        if self.uses_fb:
            ftype = ir_generator.feedback_type(fmt)
            out_header.append(f"    {ftype} *fb = ({ftype} *)_PyIR_FbEntry(cx->words, "
                              f"{ir_generator.field_expr(fmt.fb)});")
            out_header.append("    (void)fb;")
        labels = [i for i in self.order]
        for i in labels:
            out_header.append(f"    _PyJitLabel L{i} = _PyJitAsm_NewLabel(a);")
        uses_raise = any(b.term[0] == "raise" for b in g.blocks.values())
        if uses_raise:
            out_header.append("    _PyJitLabel raise = _PyJitAsm_NewLabel(a);")
        in_cold = False
        for n, i in enumerate(self.order):
            if i in self.cold and not in_cold:
                self.w("_PyJitLower_ColdBegin(cx);")
                in_cold = True
            nxt = self.order[n + 1] if n + 1 < len(self.order) else None
            if nxt is not None and (nxt in self.cold) != (i in self.cold):
                nxt = None
            self.block(i, nxt)
        if uses_raise:
            if not in_cold:
                self.w("_PyJitLower_ColdBegin(cx);")
                in_cold = True
            self.w("_PyJitTemplate_Raise(cx, raise);")
        if in_cold:
            self.w("_PyJitLower_ColdEnd(cx);")
        return out_header + self.lines + ["}"]

    def block(self, i: int, nxt) -> None:
        g = self.g
        b = g.blocks[i]
        self.w(f"_PyJitAsm_Bind(a, L{i});")
        for k, op in enumerate(b.ops):
            if op.when:
                self.w(f"if ({op.when}) {{")
                self.indent += 1
            self.op(i, k, op)
            if op.when:
                self.indent -= 1
                self.w("}")
        t = b.term
        if t[0] == "br":
            c = self.cond(t[1])
            tl, fl = self.label(t[2]), self.label(t[3])
            if nxt == t[2]:
                self.w(f"_PyJitAsm_Jcc(a, _PyJitCond_Not((_PyJitCond){c}), {fl});")
            elif nxt == t[3]:
                self.w(f"_PyJitAsm_Jcc(a, {c}, {tl});")
            else:
                self.w(f"_PyJitAsm_Jcc(a, {c}, {tl});")
                self.w(f"_PyJitAsm_Jmp(a, {fl});")
        elif t[0] == "jmp":
            self.moves(i, g.blocks[t[1]].params, t[2])
            if nxt != t[1]:
                self.w(f"_PyJitAsm_Jmp(a, {self.label(t[1])});")
        elif t[0] == "end":
            if t[2:] == (True,):
                self.w("_PyJitAsm_Jmp(a, cx->target);")
            elif t[1] is not None:
                self.w(f"_PyJitTemplate_Branch(cx, {self.cond(t[1])});")
        else:
            raise Refused(f"term {t[0]}")

    def moves(self, i: int, params, args) -> None:
        pending = []
        for p, a in zip(params, args):
            if p in self.flags or a in self.flags:
                raise Refused("a flag passed to a block")
            if self.alloc.reg[p] != self.alloc.reg[a] or self.alloc.kind(p) != self.alloc.kind(a):
                pending.append((p, a))
        while pending:
            dsts = {(self.alloc.kind(p), self.alloc.reg[p]) for p, _ in pending}
            for n, (p, a) in enumerate(pending):
                key = (self.alloc.kind(a), self.alloc.reg[a])
                if not any((self.alloc.kind(q), self.alloc.reg[q]) == key
                           for q, b in pending if b != a and (q, b) != (p, a)) or True:
                    srcs = {(self.alloc.kind(b2), self.alloc.reg[b2]) for q, b2 in pending
                            if (q, b2) != (p, a)}
                    if (self.alloc.kind(p), self.alloc.reg[p]) in srcs:
                        continue
                    if self.alloc.kind(p) == "x":
                        self.w(f"_PyJitAsm_MovsdRR(a, {self.R(p)}, {self.R(a)});")
                    else:
                        self.w(f"_PyJitAsm_MovRR(a, {self.R(p)}, {self.R(a)});")
                    pending.pop(n)
                    break
            else:
                raise Refused("a cycle of moves")

    def setcc(self, op: TOp, c: str) -> None:
        r = cond_result(op)
        if r in self.flags:
            self.w(f"int c{r} = {c};")
            return
        self.w(f"_PyJitAsm_Setcc(a, {c}, {self.R(r)});")
        self.w(f"_PyJitAsm_AluRI(a, _PyJitAlu_AND, {self.R(r)}, 0xFF);")

    def mov(self, dst: int, src: int) -> None:
        if self.alloc.reg[dst] != self.alloc.reg[src]:
            if self.alloc.kind(dst) == "x":
                self.w(f"_PyJitAsm_MovsdRR(a, {self.R(dst)}, {self.R(src)});")
            else:
                self.w(f"_PyJitAsm_MovRR(a, {self.R(dst)}, {self.R(src)});")

    def extend(self, v: int) -> None:
        "Make a C function's narrow integer result canonical in its register"
        t = self.g.types[v]
        if is_xmm(t) or t in ("i64", "u64") or ir_leaf.is_pointer(t) or t == ir_leaf.VALUE:
            return
        if t == "bool":
            self.w(f"_PyJitAsm_AluRI(a, _PyJitAlu_AND, {self.R(v)}, 0xFF);")
            return
        n = 64 - ir_leaf.bits_of(t)
        sh = "SAR" if ir_leaf.is_signed(t) else "SHR"
        self.w(f"_PyJitAsm_ShiftRI(a, _PyJitShift_SHL, {self.R(v)}, {n});")
        self.w(f"_PyJitAsm_ShiftRI(a, _PyJitShift_{sh}, {self.R(v)}, {n});")

    def canonical(self, v: int) -> None:
        "Zero-extend an unsigned narrow result"
        t = self.g.types[v]
        if t in ("u32", "u16", "u8"):
            n = 64 - ir_leaf.bits_of(t)
            self.w(f"_PyJitAsm_ShiftRI(a, _PyJitShift_SHL, {self.R(v)}, {n});")
            self.w(f"_PyJitAsm_ShiftRI(a, _PyJitShift_SHR, {self.R(v)}, {n});")

    def slot(self, field: str) -> str:
        "The slot of a field, or of the slot n after a field's for 'F+n'"
        base, _, n = field.partition("+")
        index = f"(IR_{base}(ir_word) + {n})" if n else f"IR_{base}(ir_word)"
        return f"_PyJitMem_At(_PyJit_BASE, (int32_t)(8 * {index}))"

    def op(self, i: int, k: int, op: TOp) -> None:
        g = self.g
        kd = op.kind
        res = op.results
        args = op.args
        if kd == "view" or kd == "slot_load":
            self.w(f"_PyJitAsm_MovRM(a, {self.R(res[0])}, {self.slot(op.attr)});")
        elif kd == "detach":
            self.w(f"_PyJitAsm_MovMI(a, {self.slot(op.attr)}, 0);")
        elif kd == "slot_store":
            self.w(f"_PyJitAsm_MovMR(a, {self.slot(op.attr)}, {self.R(args[0])});")
        elif kd == "take":
            # DUP returns its argument: the register is its result, which a
            # call (a count in a Py_REF_DEBUG build) writes, as it holds it
            self.lower(i, k, op, "PyValue_DUP", [args[0]], args, written={args[0]})
        elif kd == "const" and res[0] in self.imms:
            pass
        elif kd == "const" or kd == "jconst":
            t = g.types[res[0]]
            expr = f"(int64_t)({op.attr})" if kd == "jconst" else const_expr(t, op.attr)
            if res[0] in self.flags:
                raise Refused("a constant flag")
            if is_xmm(t):
                tmp = self.temps(i, k, op, 1)[0]
                self.w(f"_PyJitAsm_MovImm(a, {self.gpr(tmp)}, {expr});")
                self.w(f"_PyJitAsm_MovqXR(a, {self.R(res[0])}, {self.gpr(tmp)});")
            else:
                self.w(f"_PyJitAsm_MovImm(a, {self.R(res[0])}, {expr});")
        elif kd == "cache":
            t = g.types[res[0]]
            mem = f"_PyJitMem_At(_PyJit_WORDS, (int32_t)((char *)&fb->{op.attr} - (char *)cx->words))"
            self.load(res[0], t, mem)
        elif kd == "load":
            off, t = field_offset(op.attr)
            self.load(res[0], t, f"_PyJitMem_At({self.R(args[0])}, (int32_t){off})")
        elif kd == "store":
            off, t = field_offset(op.attr)
            size, _ = mem_size(t)
            suffix = {8: "", 4: "32", 2: "16", 1: "8"}[size]
            mem = f"_PyJitMem_At({self.R(args[0])}, (int32_t){off})"
            if is_xmm(t):
                self.w(f"_PyJitAsm_MovsdMR(a, {mem}, {self.R(args[1])});")
            else:
                self.w(f"_PyJitAsm_MovMR{suffix}(a, {mem}, {self.R(args[1])});")
        elif kd == "fieldaddr":
            off, _ = field_offset(op.attr)
            self.w(f"_PyJitAsm_Lea(a, {self.R(res[0])}, _PyJitMem_At({self.R(args[0])}, (int32_t){off}));")
        elif kd == "ptradd":
            size = op.attr if isinstance(op.attr, int) else f"sizeof({op.attr})"
            self.w(f"_PyJitTemplate_PtrAdd(cx, {self.R(res[0])}, {self.R(args[0])}, "
                   f"{self.R(args[1])}, {size});")
        elif kd == "ptrdiff":
            # The elements between two pointers into one array: the bytes
            # between them shifted by the element size's log2
            size = op.attr if isinstance(op.attr, int) else f"sizeof({op.attr})"
            self.mov(res[0], args[0])
            self.w(f"_PyJitAsm_AluRR(a, _PyJitAlu_SUB, {self.R(res[0])}, {self.R(args[1])});")
            self.w(f"_PyJitTemplate_SarLog2(cx, {self.R(res[0])}, {size});")
        elif kd in ALU and args[1] in self.imms:
            self.mov(res[0], args[0])
            self.w(f"_PyJitAsm_AluRI(a, _PyJitAlu_{ALU[kd]}, {self.R(res[0])}, "
                   f"{const_of(self.defs, args[1])});")
            if kd in ("add", "sub"):
                self.canonical(res[0])
        elif kd in ALU or kd == "mul":
            self.mov(res[0], args[0])
            if kd == "mul":
                self.w(f"_PyJitAsm_ImulRR(a, {self.R(res[0])}, {self.R(args[1])});")
            else:
                self.w(f"_PyJitAsm_AluRR(a, _PyJitAlu_{ALU[kd]}, {self.R(res[0])}, {self.R(args[1])});")
            if kd in ("add", "sub", "mul"):
                self.canonical(res[0])
        elif kd in SHIFTS:
            n = const_of(self.defs, args[1])
            sh = SHIFTS[kd]
            self.mov(res[0], args[0])
            if n is None:
                # The allocation keeps every other value out of rcx here
                if self.alloc.reg[args[1]] != RCX:
                    self.w(f"_PyJitAsm_MovRR(a, _PyJitReg_RCX, {self.R(args[1])});")
                self.w(f"_PyJitAsm_ShiftRCl(a, _PyJitShift_{sh}, {self.R(res[0])});")
            else:
                self.w(f"_PyJitAsm_ShiftRI(a, _PyJitShift_{sh}, {self.R(res[0])}, {n});")
            if kd == "shl":
                self.canonical(res[0])
        elif kd in ("neg", "not"):
            self.mov(res[0], args[0])
            self.w(f"_PyJitAsm_UnaryR(a, _PyJitUnary_{kd.upper()}, {self.R(res[0])});")
            self.canonical(res[0])
        elif kd in ("fadd", "fsub", "fmul", "fdiv"):
            self.mov(res[0], args[0])
            self.w(f"_PyJitAsm_SseRR(a, _PyJitSse_{kd[1:].upper()}SD, {self.R(res[0])}, {self.R(args[1])});")
        elif kd in OVF:
            self.mov(res[0], args[0])
            self.w(f"_PyJitAsm_AluRR(a, _PyJitAlu_{OVF[kd]}, {self.R(res[0])}, {self.R(args[1])});")
            self.setcc(op, "_PyJitCond_O")
        elif kd == "mul_ovf":
            self.mov(res[0], args[0])
            self.w(f"_PyJitAsm_ImulRR(a, {self.R(res[0])}, {self.R(args[1])});")
            self.setcc(op, "_PyJitCond_O")
        elif kd == "conv":
            self.conv(i, k, op)
        elif kd == "cmp" and args[1] in self.imms:
            self.w(f"_PyJitAsm_AluRI(a, _PyJitAlu_CMP, {self.R(args[0])}, "
                   f"{const_of(self.defs, args[1])});")
            self.setcc(op, f"_PyJitCond_{CMP_CONDS[op.attr]}")
        elif kd == "cmp":
            self.w(f"_PyJitAsm_AluRR(a, _PyJitAlu_CMP, {self.R(args[0])}, {self.R(args[1])});")
            self.setcc(op, f"_PyJitCond_{CMP_CONDS[op.attr]}")
        elif kd == "cmpmask" or kd == "fcmp":
            mask = op.attr if kd == "cmpmask" else FCMP_MASKS[op.attr]
            ops = [self.R(args[0]), self.R(args[1])]
            if is_xmm(g.types[args[0]]):
                ops += [self.gpr(t) for t in self.temps(i, k, op, 2)]
                fn = "_PyJitLower_ComparisonDouble"
            else:
                fn = "_PyJitLower_ComparisonInt"
            self.setcc(op, f"{fn}(cx, (const uint8_t[]){{{', '.join(ops)}}}, {mask})")
        elif kd in ("xorb", "lnot"):
            c = self.cond(args[0])
            neg = "1" if kd == "lnot" else op.attr
            self.setcc(op, f"(({neg}) ? (int)_PyJitCond_Not((_PyJitCond){c}) : (int){c})")
        elif kd == "select":
            c = self.cond(args[0])
            self.mov(res[0], args[2])
            self.w(f"_PyJitAsm_CmovRR(a, {c}, {self.R(res[0])}, {self.R(args[1])});")
        elif kd == "prim":
            name = op.attr[0] if isinstance(op.attr, tuple) else op.attr
            if name not in LOWERINGS:
                raise Refused(f"{name} unfused")
            prim = ir_leaf.PRIMITIVES[name]
            regs = [r for r in res if not (is_predicate(prim) and r == res[0])]
            argv = args
            aux = "NULL"
            if isinstance(op.attr, tuple):
                aux = f"(const void *){op.attr[1]}"
            self.lower(i, k, op, name, regs, argv, aux)
        elif kd == "call":
            helper = ir_leaf.HELPERS[op.attr]
            gm, xm = self.live_masks(i, k, op)
            self.w(f"cx->live = {gm:#x}; cx->live_xmm = {xm:#x};")
            argv = [f"_PyJitArg_{'XMM' if self.alloc.kind(x) == 'x' else 'GPR'}({self.R(x)})"
                    for x in args]
            if helper.thread:
                argv.insert(0, "_PyJitArg_GPR(_PyJit_TSTATE)")
            argl = ", ".join(argv)
            result = (f"_PyJitArg_{'XMM' if self.alloc.kind(res[0]) == 'x' else 'GPR'}({self.R(res[0])})"
                      if res else "_PyJitArg_NONE")
            self.w(f"_PyJitLower_Call(cx, (const void *){op.attr}, 1, {len(argv)}, "
                   f"(const _PyJitArg[]){{{argl or '_PyJitArg_NONE'}}}, {result});")
            if res:
                self.extend(res[0])
        else:
            raise Refused(f"no emission of {kd}")

    def load(self, r: int, t: str, mem: str) -> None:
        if is_xmm(t):
            self.w(f"_PyJitAsm_MovsdRM(a, {self.R(r)}, {mem});")
            return
        size, signed = mem_size(t)
        if size == 8:
            self.w(f"_PyJitAsm_MovRM(a, {self.R(r)}, {mem});")
        elif size == 4:
            self.w(f"_PyJitAsm_Mov{'sxRM32' if signed else 'RM32'}(a, {self.R(r)}, {mem});")
        else:
            self.w(f"_PyJitAsm_Mov{'sx' if signed else 'zx'}RM{size * 8}(a, {self.R(r)}, {mem});")

    def conv(self, i: int, k: int, op: TOp) -> None:
        src, dst = self.g.types[op.args[0]], self.g.types[op.results[0]]
        a, r = op.args[0], op.results[0]
        if dst == "f64" and src != "f64":
            self.w(f"_PyJitAsm_Cvtsi2sd(a, {self.R(r)}, {self.R(a)});")
        elif src == "f64":
            raise Refused("a conversion from a double")
        elif dst == "bool":
            self.w(f"_PyJitAsm_TestRR(a, {self.R(a)}, {self.R(a)});")
            self.setcc(op, "_PyJitCond_NE")
        else:
            self.mov(r, a)
            if src == "bool" or dst in ("i64", "u64") or ir_leaf.is_pointer(dst) or \
                    ir_leaf.is_pointer(src):
                return
            if ir_leaf.bits_of(dst) >= ir_leaf.bits_of(src) and \
                    ir_leaf.is_signed(dst) == ir_leaf.is_signed(src):
                return
            if ir_leaf.bits_of(dst) > ir_leaf.bits_of(src) and not ir_leaf.is_signed(src):
                return      # a zero-extended value is canonical in a wider type
            n = 64 - ir_leaf.bits_of(dst)
            sh = "SAR" if ir_leaf.is_signed(dst) else "SHR"
            self.w(f"_PyJitAsm_ShiftRI(a, _PyJitShift_SHL, {self.R(r)}, {n});")
            self.w(f"_PyJitAsm_ShiftRI(a, _PyJitShift_{sh}, {self.R(r)}, {n});")

    def lower(self, i, k, op, name, regs, argv, aux="NULL", written=frozenset()) -> None:
        lowering = LOWERINGS[name]
        prim = ir_leaf.PRIMITIVES[name]
        temps = self.temps(i, k, op, lowering.temps, "g")
        xtemps = self.temps(i, k, op, lowering.xtemps, "x")
        ops = [self.R(r) for r in regs] + [self.R(x) for x in argv] + \
              [self.gpr(t) for t in temps] + [f"_PyJitXmm_{t}" for t in xtemps]
        if lowering.calls:
            gm, xm = self.live_masks(i, k, op, written)
            self.w(f"cx->live = {gm:#x}; cx->live_xmm = {xm:#x};")
        call = (f"_PyJitLower_{name}(cx, (const uint8_t[]){{{', '.join(ops) or '0'}}}, {aux})")
        if is_predicate(prim):
            self.setcc(op, call)
        else:
            self.w(f"{call};")


# The fcmp predicates as the COMPARISON_* outcomes for which they hold
FCMP_MASKS = {"oeq": "COMPARISON_EQUALS", "une": "COMPARISON_NOT_EQUALS",
              "olt": "COMPARISON_LESS_THAN", "ogt": "COMPARISON_GREATER_THAN",
              "ole": "(COMPARISON_LESS_THAN | COMPARISON_EQUALS)",
              "oge": "(COMPARISON_GREATER_THAN | COMPARISON_EQUALS)"}


def template_of(name: str, record) -> list[str]:
    b = Builder(record)
    g = b.build()
    simplify(g)
    return Emitter(name, record, g, b.uses_fb).emit()


def write_templates(out, analysis, records) -> None:
    ids = ir_generator.handler_ids(analysis)
    made, refused = {}, {}
    bodies = []
    for name in sorted(records):
        record = records[name]
        if record.form != "EMITTED" or name not in ids:
            continue
        if not record.uops or any(u.leaf is None for u in record.uops):
            continue
        if record.reads_poly_table:
            refused[name] = "reads the site's table of cases"
            continue
        try:
            bodies.append(template_of(name, record))
            made[name] = ids[name]
        except Refused as e:
            refused[name] = str(e)
    out.write("// This file is generated by Tools/cases_generator/jit_template_generator.py\n")
    out.write("// from the leaf IR of Python/bytecodes.c.\n// Do not edit!\n//\n")
    out.write(f"// {len(made)} templates.  The handlers whose uops are all leaf but that\n")
    out.write("// keep the call of their form:\n")
    for name, why in sorted(refused.items()):
        out.write(f"//   {name}: {why}\n")
    out.write("\n")
    for body in bodies:
        out.write("\n".join(body) + "\n\n")
    out.write(f"const _PyJitTemplate _PyJit_Templates[{ir_numbers.IR_LIMIT}] = {{\n")
    for name, i in sorted(made.items(), key=lambda x: x[1]):
        out.write(f"    [{i}] = jit_template_{name},\n")
    out.write("};\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--lowerings", default=str(DEFAULT_LOWERINGS))
    parser.add_argument("--templates", default=str(DEFAULT_TEMPLATES))
    parser.add_argument("input", nargs="*", default=[DEFAULT_INPUT])
    args = parser.parse_args()
    with open(args.lowerings, "w") as out:
        write_lowerings(out)
    analysis = analyze_files(args.input)
    records = ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))
    with open(args.templates, "w") as out:
        write_templates(out, analysis, records)


if __name__ == "__main__":
    main()
