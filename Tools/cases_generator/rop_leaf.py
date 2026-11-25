"""Leaf graphs and baseline templates of register instruction arms (F2).

`leaf_of_arm` translates an inline arm's value computation into the leaf IR
of ir_leaf.py (`LeafBody`), the type the old uops produce.  A register arm's
operands are named homes, so the body reads them with `arg` ops and ends in
`out` ops, one per published value.  `publish` is not part of the leaf IR:
it stays in a `PublishPlan` that `template_of_arm` lowers, next to the body,
to the home-class-conditional detach, load, store and close that
jit_template_generator.Builder emits for a stack handler.  The graph is then
simplified and emitted by jit_template_generator unchanged.
"""

from dataclasses import dataclass, field

import ir_leaf
import jit_template_generator as jt
from rop_catalog import CONVENTIONS, DebtKind, Fact, FieldKind, Handle, Scalar
from rop_cgen import (Unsupported, add_test_facts, arm_facts, edges_of,
                      flatten, home_views)
from rop_check import Program
from rop_model import (Arm, BoolLit, Branch, Call, CallTerm, Cases, Exit,
                       Expr, Goto, Guard, HandleType, Let, Name, Nat, Publish,
                       ScalarType, Type)

LEAF_SCALAR = {
    Scalar.Bool: "bool", Scalar.U8: "u8", Scalar.U16: "u16", Scalar.U32: "u32",
    Scalar.U64: "u64", Scalar.I32: "i32", Scalar.I64: "i64", Scalar.F64: "f64",
    Scalar.I51: "i64", Scalar.Scaled51: "i64", Scalar.Size: "u64",
}
LEAF_HANDLE = {Handle.B: ir_leaf.VALUE, Handle.O: ir_leaf.VALUE}

EXIT = "EXIT"

# Primitives of a word's immediates: the template folds them in
IMMEDIATE_TESTS = frozenset({"_PyRop_BranchDrops", "_PyRop_BranchTaken",
                             "_PyRop_SameBool"})


def leaf_type(t: Type) -> str | None:
    """The leaf IR type of a graph value; None for a token (an `Err`)"""
    if isinstance(t, ScalarType):
        return LEAF_SCALAR.get(t.scalar)
    if isinstance(t, HandleType):
        return LEAF_HANDLE.get(t.handle)
    return None


@dataclass(frozen=True)
class PublishPlan:
    """The `publish` that ends every path of the arm: the value published
    to each destination (by name of its `out` op) and the debts"""
    publish: Publish


@dataclass
class ArmLeaf:
    body: ir_leaf.LeafBody
    plan: PublishPlan | None
    # Home operands the body reads, in order of first use
    inputs: tuple[str, ...]
    # Operands whose values are known inline ints on every path to the publish
    inline: frozenset[str]


class LeafBuilder:
    def __init__(self, program: Program, arm: Arm) -> None:
        self.program = program
        self.arm = arm
        self.info = program.graphs[arm.name]
        self.blocks = {b.label: b for b in arm.graph.blocks}
        self.types: dict[int, str] = {}
        self.out: dict[str, ir_leaf.Block] = {}
        self.ids: dict[str, int] = {}
        self.env: dict[str, int] = {}
        self.inputs: list[str] = []
        self.plan: PublishPlan | None = None
        self.count = 0
        self.block_list: list[ir_leaf.Block] = []
        self.effects: set[str] = set()
        self.helpers: list[str] = []
        self.cur: ir_leaf.Block

    # Values and ops

    def value(self, t: str) -> int:
        self.types[self.count] = t
        self.count += 1
        return self.count - 1

    def new_block(self) -> ir_leaf.Block:
        b = ir_leaf.Block(len(self.block_list))
        self.block_list.append(b)
        return b

    def emit(self, kind: str, types: tuple[str, ...], args: tuple[int, ...] = (),
             attr=None) -> tuple[int, ...]:
        results = tuple(self.value(t) for t in types)
        self.cur.ops.append(ir_leaf.Op(kind, results, args, attr))
        return results

    def emit1(self, kind: str, t: str, args: tuple[int, ...] = (), attr=None) -> int:
        return self.emit(kind, (t,), args, attr)[0]

    # Expressions

    def expr(self, e: Expr) -> int:
        if isinstance(e, Name):
            if e.name not in self.env:
                raise Unsupported(f"{self.arm.name}: {e.name} is not a leaf value")
            return self.env[e.name]
        if isinstance(e, Nat):
            return self.emit1("const", "u64", (), e.value)
        if isinstance(e, BoolLit):
            return self.emit1("const", "bool", (), int(e.value))
        if isinstance(e, Call):
            return self.call(e)
        raise Unsupported(f"{self.arm.name}: expression {e}")

    def call(self, e: Call) -> int:
        if e.callee == "use":
            (home,) = e.args
            assert isinstance(home, Name)
            if home.name not in self.inputs:
                self.inputs.append(home.name)
            return self.emit1("arg", ir_leaf.VALUE, (), home.name)
        x = self.program.externs.get(e.callee)
        if x is None or x.c is None:
            raise Unsupported(f"{self.arm.name}: {e.callee} has no C body (pending)")
        if x.c.symbol in IMMEDIATE_TESTS:
            return self.immediate_test(x.c.symbol, e)
        prim = ir_leaf.PRIMITIVES.get(x.c.symbol)
        if prim is None or len(prim.results) != 1:
            raise Unsupported(f"{self.arm.name}: {e.callee} has no leaf lowering")
        args = tuple(self.expr(a) for a in e.args)
        return self.emit1("prim", prim.results[0], args, x.c.symbol)

    def immediate(self, e: Expr) -> str:
        """The C expression, over the word, of an immediate operand of the
        arm: the word is known when its template is made, so the tests of
        immediates are not code"""
        if isinstance(e, Name):
            for p in self.program.encodings[self.arm.family].placements:
                if (p.operand == e.name and p.kind is FieldKind.imm
                        and len(p.slots) == 1 and p.slots[0].word == 0):
                    return f"IR_{p.slots[0].name}(ir_word)"
        raise Unsupported(f"{self.arm.name}: {e} is not an immediate of the word")

    def immediate_test(self, symbol: str, e: Call) -> int:
        """A branch's test of its mode or sense: a bool that is a function of
        the word, or a test result negated or not by the word"""
        imm = self.immediate(e.args[0])
        if symbol == "_PyRop_BranchDrops":
            return self.emit1("jconst", "bool", (), f"({imm}) < 2")
        hit = self.expr(e.args[1])
        if symbol == "_PyRop_BranchTaken":
            return self.emit1("xorb", "bool", (hit,), f"!(({imm}) & 1)")
        assert symbol == "_PyRop_SameBool"
        return self.emit1("xorb", "bool", (hit,), f"!({imm})")

    # Statements and terminators

    def build(self) -> ArmLeaf:
        for b in self.arm.graph.blocks:
            self.ids[b.label] = self.new_block().id
        for b in self.arm.graph.blocks:
            self.cur = self.block_list[self.ids[b.label]]
            for p in b.params:
                t = leaf_type(p.type)
                if t is not None:
                    self.cur.params.append(self.value(t))
                    self.env[p.name] = self.cur.params[-1]
        for b in self.arm.graph.blocks:
            self.cur = self.block_list[self.ids[b.label]]
            for s in b.stmts:
                self.stmt(s)
            self.term(b.term)
        flat = flatten(self.program, self.arm)
        facts = arm_facts(self.program, flat)
        views = home_views(flat)
        inline = frozenset(
            home for home, view in views.items()
            if all(f.has(view, Fact.inline_int) for lbl, f in facts.items()
                   if self.publishes(lbl)))
        body = ir_leaf.LeafBody(
            self.arm.name, self.block_list, self.types, tuple(self.inputs),
            tuple(self.outputs()), frozenset(self.effects), tuple(self.helpers))
        return ArmLeaf(body, self.plan, tuple(self.inputs), inline)

    def publishes(self, label: str) -> bool:
        return any(isinstance(s, Publish) for s in self.blocks[label].stmts)

    def outputs(self) -> list[str]:
        if self.plan is None:
            return []
        return [b.target for b in self.plan.publish.bindings]

    def stmt(self, s) -> None:
        if isinstance(s, Let):
            t = leaf_type(s.type)
            if t is None:
                raise Unsupported(f"{self.arm.name}: let {s.name} has no leaf type")
            self.env[s.name] = self.expr(s.value)
        elif isinstance(s, Guard):
            ok = self.expr(s.cond)
            bad = self.emit1("lnot", "bool", (ok,))
            self.emit("exit_if", (), (bad,), EXIT)
        elif isinstance(s, Publish):
            if self.plan is not None and (
                    self.plan.publish.bindings != s.bindings
                    or self.plan.publish.debts != s.debts):
                raise Unsupported(f"{self.arm.name}: two different publications")
            self.plan = PublishPlan(s)
            for b in s.bindings:
                self.emit("out", (), (self.expr(b.value),), b.target)
        else:
            raise Unsupported(f"{self.arm.name}: statement {s}")

    def jump(self, target, payload: tuple[int, ...] = ()):
        params = self.blocks[target.label].params
        values = list(payload) + [self.expr(a) for a in target.args]
        args = tuple(v for p, v in zip(params, values) if leaf_type(p.type) is not None)
        return ("jmp", self.ids[target.label], args)

    def term(self, t) -> None:
        if isinstance(t, Goto):
            self.cur.term = self.jump(t.target)
        elif isinstance(t, Branch):
            cond = self.expr(t.cond)
            self.cur.term = ("br", cond, self.edge_block(t.if_true),
                             self.edge_block(t.if_false))
        elif isinstance(t, CallTerm):
            self.call_term(t)
        elif isinstance(t, Exit):
            self.exit(t)
        else:
            raise Unsupported(f"{self.arm.name}: terminator {t}")

    def edge_block(self, target, payload: tuple[int, ...] = ()) -> int:
        """A block that jumps to target, so that a branch edge can carry
        arguments"""
        here = self.cur
        edge = self.new_block()
        self.cur = edge
        edge.term = self.jump(target, payload)
        self.cur = here
        return edge.id

    def exit(self, t: Exit) -> None:
        if t.kind == "next":
            self.cur.term = ("next", None)
        elif t.kind == "raise":
            self.cur.term = ("raise",)
        elif t.kind == "branch":
            self.cur.term = ("branch",)
        elif t.kind == "jump":
            self.cur.term = ("jump",)
        elif t.kind == "miss":
            self.cur.term = ("exit", EXIT, None)
        else:
            raise Unsupported(f"{self.arm.name}: exit {t.kind} has no leaf lowering")

    def call_term(self, t: CallTerm) -> None:
        x = self.program.externs.get(t.callee)
        if x is None or x.c is None:
            raise Unsupported(f"{self.arm.name}: {t.callee} has no C body (pending)")
        edges = {e.case: e for e in t.edges}
        if x.c.symbol == "_PyRop_Poll":
            # The thread is implicit in the template's registers
            return self.poll(edges)
        args = tuple(self.expr(a) for a in t.args)
        if x.c.convention == "bool_out" and x.c.symbol == "_PyValue_AddCheckedScaled":
            self.add_checked_scaled(args, edges)
        elif x.c.convention == "value_or_null" and x.c.symbol in ir_leaf.HELPERS:
            self.helpers.append(x.c.symbol)
            self.effects |= set(ir_leaf.HELPERS[x.c.symbol].effects)
            result = self.emit1("call", ir_leaf.VALUE, args, x.c.symbol)
            failed = self.emit1("prim", "bool", (result,), "PyValue_IsNull")
            err = self.blocks[edges["err"].target.label]
            if err.stmts or not (isinstance(err.term, Exit) and err.term.kind == "raise"):
                raise Unsupported(f"{self.arm.name}: the error edge of {t.callee} is not a raise")
            self.emit("raise_if", (), (failed,))
            self.cur.term = self.jump(edges["ok"].target, (result,))
        else:
            raise Unsupported(f"{self.arm.name}: {t.callee} has no leaf lowering")

    def poll(self, edges) -> None:
        """The periodic check: a load and a test of the eval breaker, and
        only when an event is pending the call that handles it"""
        err = self.blocks[edges["err"].target.label]
        if err.stmts or not (isinstance(err.term, Exit) and err.term.kind == "raise"):
            raise Unsupported(f"{self.arm.name}: the error edge of poll is not a raise")
        self.helpers.append("_Py_HandlePending")
        self.effects |= set(ir_leaf.HELPERS["_Py_HandlePending"].effects)
        events = self.emit1("prim", "bool", (), "_Py_EvalBreakerHasEvents")
        here = self.cur
        handle = self.new_block()
        quiet = self.edge_block(edges["ok"].target)
        here.term = ("br", events, handle.id, quiet)
        self.cur = handle
        status = self.emit1("call", "i32", (), "_Py_HandlePending")
        zero = self.emit1("const", "i32", (), 0)
        failed = self.emit1("cmp", "bool", (status, zero), "ne")
        self.emit("raise_if", (), (failed,))
        self.cur.term = self.jump(edges["ok"].target)

    def add_checked_scaled(self, args: tuple[int, ...], edges) -> None:
        """The checked add of the scaled words, a primitive of the closed
        set: the bool says which case, the i64 is the payload of both"""
        overflowed, out = self.emit(
            "prim", ("bool", "i64"), args, "_PyValue_AddCheckedScaled")
        here = self.cur
        over = self.edge_block(edges["overflow"].target, (out,))
        fits = self.edge_block(edges["fits"].target, (out,))
        here.term = ("br", overflowed, over, fits)


def leaf_of_arm(program: Program, arm: Arm) -> ArmLeaf:
    return LeafBuilder(program, arm).build()


# The template: the leaf body joined with its publication, as
# jit_template_generator.Builder joins the uops of a stack handler

@dataclass(frozen=True)
class Home:
    """What Builder reads of an operand: its field letter and what it may
    name (any of a temporary, a local)"""
    field: str
    may_name: frozenset[str] = frozenset({"TEMP", "LOCAL"})


class TemplateBuilder:
    def __init__(self, program: Program, arm: Arm, leaf: ArmLeaf) -> None:
        self.program = program
        self.arm = arm
        self.leaf = leaf
        self.g = jt.Graph()
        self.raises: jt.TBlock | None = None
        self.deopt: jt.TBlock | None = None
        self.homes: dict[str, Home] = {}
        enc = program.encodings[arm.family]
        for p in enc.placements:
            if p.kind is FieldKind.home and p.slots[0].word == 0:
                self.homes[p.operand] = Home(p.slots[0].name)

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

    def build(self) -> jt.Graph:
        g, body = self.g, self.leaf.body
        entry = g.blocks[g.entry]
        views: dict[str, int] = {}
        for name in self.leaf.inputs:
            views[name] = g.value(ir_leaf.VALUE)
            entry.ops.append(jt.TOp("view", [views[name]], [], self.homes[name].field))
        lmap: dict[int, int] = {}
        bmap = {}
        for b in body.blocks:
            gb = g.new_block([body.types[p] for p in b.params])
            bmap[b.id] = gb
            for p, gp in zip(b.params, gb.params):
                lmap[p] = gp
        entry.term = ("jmp", bmap[0].id, [])
        outs = self.leaf.body.outputs
        # A word that can branch ends on the flag that says whether it did
        branches = any(b.term[0] == "branch" for b in body.blocks)
        if any(b.term[0] == "jump" for b in body.blocks) and self.leaf.plan is not None:
            raise jt.Refused("a jump after a publication")
        merge = g.new_block([ir_leaf.VALUE for _ in outs]
                            + (["bool"] if branches else []))
        for b in body.blocks:
            gb = bmap[b.id]
            published: dict[str, int] = {}
            for op in b.ops:
                gb = self.op(op, gb, lmap, views, published)
            term = b.term
            if term[0] == "br" and self.leaves_by_exit(gb, lmap[term[1]]):
                # The primitive branches to the overflow case itself
                gb.ops[-1].exit = bmap[term[2]].id
                gb.ops[-1].results = gb.ops[-1].results[1:]
                gb.term = ("jmp", bmap[term[3]].id, [])
            elif term[0] == "br":
                gb.term = ("br", lmap[term[1]], bmap[term[2]].id, bmap[term[3]].id)
            elif term[0] == "jmp":
                gb.term = ("jmp", bmap[term[1]].id, [lmap[a] for a in term[2]])
            elif term[0] in ("next", "branch"):
                args = [published[o] for o in outs]
                if branches:
                    flag = g.value("bool")
                    gb.ops.append(jt.TOp("const", [flag], [], int(term[0] == "branch")))
                    args.append(flag)
                gb.term = ("jmp", merge.id, args)
            elif term[0] == "jump":
                gb.term = ("end", None, True)
            elif term[0] == "raise":
                gb.term = ("jmp", self.exit_block("raise"), [])
            else:
                gb.term = ("jmp", self.exit_block("deopt"), [])
        if self.leaf.plan is None and not branches and all(
                b.term[0] != "next" for b in body.blocks):
            del g.blocks[merge.id]
        else:
            self.epilogue(merge, views, branches)
        return g

    @staticmethod
    def leaves_by_exit(gb, cond: int) -> bool:
        """Whether the block's last op is the checked add whose bool is cond:
        its lowering leaves for the overflow case by a branch of its own"""
        last = gb.ops[-1] if gb.ops else None
        return (last is not None and last.kind == "prim"
                and last.attr == "_PyValue_AddCheckedScaled" and last.results[0] == cond)

    def op(self, op, gb, lmap, views, published):
        g = self.g
        k = op.kind
        args = [lmap[a] for a in op.args]
        if k == "arg":
            lmap[op.results[0]] = views[op.attr]
            return gb
        results = []
        for res in op.results:
            results.append(g.value(self.leaf.body.types[res]))
            lmap[res] = results[-1]
        if k == "out":
            published[op.attr] = args[0]
            return gb
        if k in ("exit_if", "raise_if"):
            cont = g.new_block()
            gb.term = ("br", args[0], self.exit_block("raise" if k == "raise_if" else "deopt"),
                       cont.id)
            return cont
        if k == "prim" and op.attr not in jt.LOWERINGS:
            raise jt.Refused(f"no lowering of {op.attr}")
        gb.ops.append(jt.TOp(k, results, args, op.attr))
        return gb

    def epilogue(self, merge: jt.TBlock, views: dict[str, int],
                 branches: bool = False) -> None:
        """Detach the spent temporaries, store each result, close what the
        displaced and spent owners held: Builder's end of a handler"""
        plan = self.leaf.plan
        if plan is None:
            raise jt.Refused("an arm that publishes nothing")
        g = self.g
        publish = plan.publish
        order = list(self.homes)
        spent = sorted((d.subject for d in publish.debts if d.kind is DebtKind.spent),
                       key=order.index)
        deferred = []
        for name in spent:
            home = self.homes[name]
            merge.ops.append(jt.TOp("detach", [], [], home.field, jt.temp_test(home)))
            if name not in self.leaf.inline and name in views:
                deferred.append(jt.TOp("prim", [], [views[name]], "PyValue_XCLOSE",
                                       jt.temp_test(home)))
        olds = []
        for binding, v in zip(publish.bindings, merge.params):
            dst = self.homes[binding.target]
            if any(d.kind is DebtKind.old and d.subject == binding.target
                   for d in publish.debts):
                o = g.value(ir_leaf.VALUE)
                merge.ops.append(jt.TOp("slot_load", [o], [], dst.field, jt.local_test(dst)))
                olds.append((o, dst))
            merge.ops.append(jt.TOp("slot_store", [], [v], dst.field))
        merge.ops.extend(deferred)
        for o, dst in olds:
            merge.ops.append(jt.TOp("prim", [], [o], "PyValue_XCLOSE", jt.local_test(dst)))
        merge.term = ("end", merge.params[-1] if branches else None)


@dataclass(frozen=True)
class FamilyRecord:
    """The only thing Emitter reads of a record, for an arm that uses no
    feedback entry"""
    format: object = None


def template_graph(program: Program, arm: Arm, leaf: ArmLeaf) -> jt.Graph:
    g = TemplateBuilder(program, arm, leaf).build()
    jt.simplify(g)
    return g


def template_of_arm(program: Program, arm: Arm, name: str,
                    leaf: ArmLeaf | None = None) -> list[str]:
    """The lines of the arm's `jit_template_NAME`"""
    leaf = leaf or leaf_of_arm(program, arm)
    g = template_graph(program, arm, leaf)
    return jt.Emitter(name, FamilyRecord(), g, False).emit()
