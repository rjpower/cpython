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
lowerings' prototypes and table) and Python/jit_templates.c.h."""

import argparse
from dataclasses import replace
import ir_generator
import rop_primitives
import ir_contract
import ir_arms
from analyzer import analyze_files
from generators_common import DEFAULT_INPUT, ROOT
from jit_backend import (
    lowering_name,
    operand_kinds,
    register_results,
    write_lowerings,
    COUNT_PRIMS,
    Emitter,
    FbEntry,
    Graph,
    LOWERINGS,
    Lowering,
    PURE_PRIMS,
    Refused,
    TEMP_ONLY,
    TOp,
    is_predicate,
    local_test,
    simplify,
    temp_test,
)



def row(name):
    "The component row of a primitive or helper, or None"
    return ir_contract.rows().get(name) if isinstance(name, str) else None


DEFAULT_LOWERINGS = ROOT / "Python/jit_lowerings.h"
DEFAULT_TEMPLATES = ROOT / "Python/jit_templates.c.h"


# Ops with an effect: a guard may not follow one, nor may a raise
EFFECT_OPS = {"detach", "slot_store", "store", "take"}
CLOSES = {"PyValue_CLOSE", "PyValue_XCLOSE", "PyValue_CLOSE_SPECIALIZED"}


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
                gv = g.value(rop_primitives.VALUE)
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
                if (fact.kind == "ENCODING" and fact.subject[1:].isdigit()
                        and set(fact.arg.split("|")) <= ir_contract.INLINE_WORDS):
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
                     else rop_primitives.VALUE for o in outputs]
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
            self.uses_fb |= self.record.oparg == "FBARG_ENTRY"
            self.add(gb, "jconst", results, [], oparg_expr(self.record))
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
            # A local's input that a primitive consumes needs a count of its
            # own, as one that a store moves elsewhere does
            prim_row = row(name)
            for arg, row_arg in zip(args, prim_row.args if prim_row else ()):
                if row_arg.mode == "consumed" and arg in self.views:
                    operand = self.views[arg]
                    if operand.may_name != TEMP_ONLY:
                        self.add(gb, "take", [], [arg], None, local_test(operand))
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
        o = self.g.value(rop_primitives.VALUE)
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
                o = self.g.value(rop_primitives.VALUE)
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
PURE_PRIMS.add("COMPARISON_BIT")


def load_records():
    analysis = analyze_files([DEFAULT_INPUT])
    formats = ir_generator.compute_formats(analysis)
    return analysis, ir_generator.build_records(analysis, formats)


def fb_entry(record) -> FbEntry:
    "The entry of a stack handler's record"
    fmt = record.format
    return FbEntry(ir_generator.feedback_type(fmt), ir_generator.field_expr(fmt.fb))


def template_of(name: str, record) -> list[str]:
    b = Builder(record)
    g = b.build()
    simplify(g)
    return Emitter(name, g, fb_entry(record) if b.uses_fb else None).emit()


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
    # The register instruction arms' templates (generated_rop_templates.c.h)
    # take the ids that follow the old handlers
    import rop_generator
    arm_templates = rop_generator.template_arm_names()
    out.write(f"const _PyJitTemplate _PyJit_Templates[{ir_arms.IR_LIMIT}] = {{\n")
    for name, i in sorted(made.items(), key=lambda x: x[1]):
        out.write(f"    [IR_H_{name}] = jit_template_{name},\n")
    for name in arm_templates:
        out.write(f"    [ROP_HANDLER_{name}] = jit_template_{name},\n")
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
