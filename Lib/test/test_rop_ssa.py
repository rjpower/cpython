"""The tracer's SSA expansion of every register instruction arm
(Tools/cases_generator/rop_ssa.py), run by its evaluator (rop_ssa_eval.py)
against the C arm on the same live objects.

Every arm expands.  Each is run on the scenarios of its family: the
C arm runs once on a word of a fresh function (jit_template_run, forced to
the arm), and the evaluator runs the expansion from the same homes.  Some
path of the expansion must agree with C:
- on the exit;
- on every home the word names, where an opaque row's payload unifies with
  C's value;
- on the feedback and handler writes, which the path's rows must declare.

The path must also balance its own owners.  For C, each input object's count
must change by exactly the number of homes that hold it afterwards."""

import functools
import sys
import types
import unittest
from dataclasses import dataclass, field
from pathlib import Path

from test import support, test_tools

capi = support.import_helper.import_module("_testinternalcapi")
if not hasattr(capi, "jit_template_run"):
    raise unittest.SkipTest("needs the template probe")

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import ir_leaf_eval
    import rop_check
    import rop_generator
    import rop_ssa
    import rop_ssa_eval as ev
    import rop_syntax
    import rop_verify
    from rop_catalog import FieldKind, Region

ROOT = Path(test_tools.basepath)
FORM, TEMPLATE, STUB = 0, 1, 2
STATUS = ((4, "raise"), (16, "again"), (1, "miss"), (32, "redirect"), (8, "branch"))


def program():
    p = rop_check.load(sorted((ROOT / "Python" / "register_ops").glob("*.rop")))
    assert not p.diagnostics, p.diagnostics
    return p


PROGRAM = program()
SSA, FAILED = rop_ssa.expand_all(PROGRAM)

class ExpansionTests(unittest.TestCase):
    def test_every_arm_expands(self):
        self.assertEqual({n: d.message for n, d in FAILED.items()}, {})
        self.assertEqual(set(SSA), set(PROGRAM.arms))

    def test_exits_are_declared_and_name_their_frame_state(self):
        for name, arm in SSA.items():
            declared = set(PROGRAM.arms[name].contract.exits)
            for t in arm.exits():
                with self.subTest(arm=name, exit=t.kind):
                    self.assertIn(t.kind, declared)
                    homes = {h for h, _ in t.state.homes}
                    self.assertEqual(homes, {n for n, k in arm.operands.items()
                                             if k in (FieldKind.home, FieldKind.vector)})
                    if t.kind == "raise":
                        self.assertIsNotNone(t.state.point)
                        self.assertEqual(t.state.exc, "pending")

    def test_a_guard_exits_with_every_home_intact(self):
        for name, arm in SSA.items():
            for op in arm.guards():
                with self.subTest(arm=name):
                    state = op.attr.state
                    self.assertEqual(state.pc, "site")
                    self.assertTrue(all(h.state == "intact" for _, h in state.homes))

    def test_publish_takes_reads_old_writes_then_releases(self):
        # Add.Int's Finish block, in change 21's order
        ops = [op.kind.value for op in SSA["Add.Int"].blocks["Finish"].ops]
        self.assertEqual(ops, ["take", "take", "old", "store", "checkpoint",
                               "release", "release", "release"])

    def test_a_vector_publish_moves_then_closes_the_rest(self):
        ops = [(op.kind.value, op.attr) for op in SSA["Move.Retire"].entry.ops]
        self.assertEqual(ops, [("take", "srcs"), ("old", "dsts"), ("store", "dsts"),
                               ("checkpoint", "Move.Done"), ("rest", "dsts"),
                               ("release", rop_check.DebtKind.spent)])
        ops = [op.kind.value for op in SSA["Move.Copy"].entry.ops]
        self.assertEqual(ops, ["read", "dup", "old", "store", "checkpoint"])

    def test_an_owners_publish_stores_the_items_then_closes_the_olds(self):
        # d[] = v: count(d) is read from the word; the items of v go to the
        # homes of d, whose displaced owners old(d) closes after the checkpoint
        src = """\
family F(dsts:Span<Dst>, x:Src) exits(next) ops(NOP);
point F.Done(current, next_head, clear) keep(x) spent() defs(dsts);
extern helper split(v:B, n:Size) -> Span<O> effects(allocates) pending;
arm F.Split shared effects(allocates, writes, releases on (dsts), runs_python) feedback()
  exits(next)
{
  entry():
    let n:Size = count(dsts);
    let v:Span<O> = split(use(x), n);
    publish [dsts[] = v] retire [old(dsts)] at F.Done;
    exit next();
}
"""
        program = rop_check.check([rop_syntax.parse(src, "t.rop")])
        self.assertEqual(program.diagnostics, ())
        arm = rop_ssa.expand(program, program.arms["F.Split"])
        ops = [(op.kind.value, op.attr) for op in arm.entry.ops]
        self.assertEqual(ops, [("count", "dsts"), ("read", "x"), ("call", "split"),
                               ("old", "dsts"), ("store", "dsts"),
                               ("checkpoint", "F.Done"),
                               ("release", rop_check.DebtKind.old)])

    def test_a_transfer_row_performs_its_edges_exits(self):
        for name in ("Jump.Warm", "Return.Generator", "Return.Observed"):
            switches = [b.term for b in SSA[name].blocks.values()
                        if isinstance(b.term, rop_ssa.Switch) and b.term.transfer]
            with self.subTest(arm=name):
                self.assertEqual(len(switches), 1)
                for e in switches[0].edges:
                    target = SSA[name].blocks[e.target.block].term
                    self.assertIsInstance(target, rop_ssa.ExitTerm)
                    self.assertEqual(target.kind, e.case)

    def test_select_and_observes_are_entry_facts(self):
        self.assertEqual(SSA["LocalRead.Bound"].facts, ("nonnull(local)",))
        self.assertEqual(SSA["Return.Generator"].facts, ("generator_frame(frame)",))
        self.assertEqual(SSA["Call.Observed"].facts, ("instrumented(site)",))

    def test_a_row_with_cases_has_an_edge_per_case(self):
        for name, arm in SSA.items():
            for b in arm.blocks.values():
                if isinstance(b.term, rop_ssa.Switch):
                    x = PROGRAM.externs[b.term.row]
                    with self.subTest(arm=name, row=b.term.row):
                        self.assertEqual([e.case for e in b.term.edges],
                                         [c.name for c in x.result.cases])


# ---------------------------------------------------------------------------
# The C arm against the evaluator


class Old:
    "The value a destination held before the word"


class Bump:
    def __init__(self, n):
        self.n = n

    def __add__(self, other):
        return Bump(self.n + (other.n if isinstance(other, Bump) else other))

    def __radd__(self, other):
        return Bump(other + self.n)

    def __eq__(self, other):
        return isinstance(other, Bump) and other.n == self.n

    __hash__ = None


class RaisesAdd:
    def __add__(self, other):
        raise ZeroDivisionError("add")


class Slotted:
    def __init__(self, n):
        self.n = n

    def __bool__(self):
        return self.n != 0


class Sized:
    def __init__(self, n):
        self.n = n

    def __len__(self):
        return self.n


class Plain:
    pass


class RaisesBool:
    def __bool__(self):
        raise ZeroDivisionError("bool")


class Point:
    def __init__(self):
        self.x = 1
        self.y = 2

    def norm(self):
        return self.x


class WithSlots:
    __slots__ = ("x",)

    def __init__(self):
        self.x = 7

    def norm(self):
        return self.x


class ClassAttr:
    x = 5

    def norm(self):
        return 1

    @classmethod
    def make(cls):
        return cls


class Many:
    "An instance whose dict is too large for inline values"

    def __init__(self):
        for i in range(40):
            setattr(self, f"a{i}", i)
        self.x = 9


class Swapped:
    "Point's attributes in the other order: another class and shape"

    def __init__(self):
        self.y = 0
        self.x = 3

    def norm(self):
        return 2


class SlotsClassAttr:
    __slots__ = ()
    x = 5


class LazyNorm(Exception):
    "An instance of a type with a dict offset, whose dict is not yet made"

    def norm(self):
        return 1


def materialised():
    "A Point whose dict has been made, so it no longer has inline values"
    o = Point()
    o.__dict__["z"] = 1
    return o


TRUTHS = [True, False, 0, 1, -1, 2**50, 2**60, 0.0, 1.5, None, "", "a", [], [1], (), {1: 2},
          Plain(), Slotted(0), Slotted(3), Sized(0), Sized(2), RaisesBool()]
PAIRS = [(3, 4), (2**50, 2**50), (2**60, 5), (-2**50, -2**50), (1.5, 2), (2, 0.25),
         ("a", "b"), ([1], [2]), ((1,), (2,)), (Bump(1), 2), (2, Bump(5)),
         (RaisesAdd(), 1), (1, "a"), (b"x", b"y")]


@dataclass
class Scenario:
    """A family's words: one function per source (the operands are locals
    in one and temporaries in another), the inputs placed in its homes, and
    calls that warm the word first"""
    family: str
    sources: dict[str, str]
    inputs: list[dict[str, object]]
    warm: tuple = ()
    warmups: int = 0
    # The operands an arm's static evidence (`select`) requires present
    requires: dict[str, str] = field(default_factory=dict)
    # Objects that warm a source's word in turn, `warmups` calls each (two
    # receiver types in turn make a polymorphic word)
    warm_by_source: dict[str, tuple] = field(default_factory=dict)
    # A specialised arm runs only on a word whose entry was installed for
    # it: its admission rows trust the entry's kind (the report's finding)
    coherent: bool = False
    # An arm whose static evidence is weaker than another's runs on that
    # arm's words: `live_scratch` admits the vacant scratch of `Return.Empty`
    words_of: dict[str, str] = field(default_factory=dict)
    # The sources whose code has local events (EVENTS).  An arm
    # that `observes` runs only on them: its event rows require
    # instrumented(site), which its declaration grants on the instrumented
    # twin's words only
    monitored: tuple[str, ...] = ()
    # Arms the probe cannot run on these words, and why (the report lists them)
    skip: dict[str, str] = field(default_factory=dict)
    # Each arm runs on its own first word, which the translator chose by
    # handler (Move's arms have no select)
    per_arm: bool = False
    # Inputs for a source whose word wants other values than `inputs`
    # (an arm whose operand is of one kind: a namespace, an exception)
    inputs_by_source: dict[str, list] = field(default_factory=dict)
    # The result holds an input (a function holds its code), which the
    # count check cannot see
    holds: bool = False
    # Run before each run of the arm on a word: the arm's store needs an
    # empty field (the compiler sets each field once)
    before_run: object = None

    def inputs_of(self, source):
        return self.inputs_by_source.get(source, self.inputs)


def truth_inputs(operand):
    return [{operand: v} for v in TRUTHS]


class Getter:
    "A container of Python's own: every item protocol call runs its methods"
    def __getitem__(self, key):
        return key

    def __setitem__(self, key, value):
        pass

    def __delitem__(self, key):
        pass


def identity(x):
    return x


def two(x, y):
    return x


class Holder:
    def __init__(self, x):
        self.x = x


STOPITERATION_WHY = (
    "stopiteration_error asserts that the frame is a generator's, which the "
    "probe's synthetic frame is not; test_rop_functions runs the arm")


def function_of(source):
    namespace = {}
    exec(source, namespace)
    return namespace["f"]


FUNCTION = function_of("def f(x=1):\n    pass\n")

# A function whose code is the module's, so that its words are the module's
MODULE_SOURCE = ("import types\n"
                 "f = types.FunctionType(compile(%r, 'm', 'exec'), globals())\n")

# The code of a generic function's scope, which makes its type parameters
PAIR_SOURCE = (
    "import types\n"
    "m = compile('def g[T: int](): pass', 'm', 'exec')\n"
    "scope = next(c for c in m.co_consts if hasattr(c, 'co_code'))\n"
    "f = types.FunctionType(scope, globals())\n")


SCENARIOS = {
    "Add": Scenario("Add", {
        "locals": "def f(a, b):\n    return a + b\n",
        "temporaries": "def f(a, b):\n    return (a * 1) + (b * 1)\n",
        "alias": "def f(a, b):\n    a = a + b\n    return a\n"},
        [{"x": x, "y": y} for x, y in PAIRS], warm=([1], [2])),
    "Truth": Scenario("Truth", {
        "locals": "def f(a):\n    return not a\n",
        "temporaries": "def f(a):\n    return not (a * 1)\n"},
        truth_inputs("value"), warm=(0,)),
    "BranchTruth": Scenario("BranchTruth", {
        "locals": "def f(a):\n    if a:\n        return 1\n    return 2\n",
        "temporaries": "def f(a):\n    if a * 1:\n        return 1\n    return 2\n",
        "keep": "def f(a, b):\n    return a or b\n",
        "monitored": "def f(a):\n    if a:\n        return 1\n    return 2\n"},
        truth_inputs("value"), warm=(0, 0), monitored=("monitored",)),
    "BranchNone": Scenario("BranchNone", {
        "locals": "def f(a):\n    if a is None:\n        return 1\n    return 2\n",
        "not": "def f(a):\n    if a is not None:\n        return 1\n    return 2\n",
        "monitored": "def f(a):\n    if a is None:\n        return 1\n    return 2\n"},
        [{"value": v} for v in (None, 0, 1, "a", Plain(), [])], warm=(0,),
        monitored=("monitored",)),
    "Const": Scenario("Const", {
        "pool": "def f():\n    x = 300000\n    return x\n",
        "small": "def f():\n    x = 3\n    return x\n",
        "common": "def f():\n    x = None\n    return x\n"},
        [{}], warm=()),
    "LocalRead": Scenario("LocalRead", {
        "bound": "def f(a):\n    b = a\n    return (a, b)\n",
        "checked": "def f(a):\n    if a:\n        b = 1\n    return b\n"},
        [{"local": v} for v in (1, "s", Plain(), [2**70])] + [{}], warm=(1,),
        requires={"LocalRead.Bound": "local"}),
    "LocalWrite": Scenario("LocalWrite", {
        "store": "def f(a):\n    b = a\n    return b\n",
        "restore": "def f(a):\n    x = 1\n    y = [x for x in a]\n    return x\n"},
        [{"value": v} for v in (1, "s", Plain(), 2**70)] + [{}], warm=([1],),
        requires={"LocalWrite.Store": "value"}),
    "SaveLocal": Scenario("SaveLocal", {
        "comprehension": "def f(a):\n    return [x for x in a]\n"},
        [{"local": v} for v in (1, "s", Plain(), 2**70)] + [{}], warm=([1],)),
    "LocalDelete": Scenario("LocalDelete", {
        "delete": "def f(a):\n    b = a\n    del b\n    return a\n"},
        [{"local": v} for v in (1, "s", Plain(), 2**70)] + [{}], warm=(1,)),
    "CellRead": Scenario("CellRead", {
        "get": "def f(a):\n    def g():\n        return a\n    return (g, a)\n"},
        [{"cell": types.CellType(v)} for v in (1, "s", Plain(), 2**70)]
        + [{"cell": types.CellType()}], warm=(1,)),
    "CellStore": Scenario("CellStore", {
        "set": "def f(a, b):\n    def g():\n        return a\n    a = b\n    return g\n"},
        [{"cell": types.CellType(v), "value": w}
         for v in (None, 1, Plain()) for w in (1, "s", Plain(), 2**70)]
        + [{"cell": types.CellType(), "value": 1}], warm=(1, 2), holds=True),
    "CellDelete": Scenario("CellDelete", {
        "delete": "def f(a):\n    def g():\n        return a\n    del a\n    return g\n"},
        [{"cell": types.CellType(v)} for v in (1, "s", Plain(), 2**70)]
        + [{"cell": types.CellType()}], warm=(1,)),
    "CellMake": Scenario("CellMake", {
        "wrap": "def f(a):\n    return lambda: a\n"},
        [{"local": v} for v in (1, "s", Plain(), 2**70)] + [{}], warm=(1,), holds=True),
    "Move": Scenario("Move", {
        "join": "def f(a):\n    return (len(a) > 4 and\n            a[:2] == a[-2:] == '__'"
                " and\n            a[2] != '_')\n",
        "or": "def f(a, b):\n    return (a and b) or a\n"},
        [{"srcs": [v]} for v in (1, "s", Plain(), 2**70)], warm=("__x__", 1),
        per_arm=True),
    "NamespaceRead": Scenario("NamespaceRead", {
        "build_class": "def f(a):\n    class C:\n        pass\n    return a\n"},
        [{}], warm=(1,)),
    "NamespaceWrite": Scenario("NamespaceWrite", {
        "global": "def f(a):\n    global g\n    g = a\n    return a\n"},
        [{"value": v} for v in (1, "s", Plain(), 2**70)] + [{}], warm=(1,),
        holds=True),
    "Import": Scenario("Import", {
        "eager": "def f(a):\n    import os\n    return a\n"},
        [{"level": 0, "fromlist": None}], warm=(1,)),
    "ImportFrom": Scenario("ImportFrom", {
        "from": "def f(a):\n    from os import path, sep\n    return a\n"},
        [{"module": __import__("os")}], warm=(1,)),
    "Jump": Scenario("Jump", {
        "loop": "def f(n):\n    while n:\n        n -= 1\n    return n\n",
        "forward": "def f(a):\n    x = 1 if a else 2\n    y = (x, x)\n    return y\n",
        "monitored": "def f(n):\n    while n:\n        n -= 1\n    return n\n"},
        [{}], warm=(0,), monitored=("monitored",)),
    "GetAttr": Scenario("GetAttr", {
        "shape": "def f(a):\n    return a.x\n",
        "slot": "def f(a):\n    return a.x\n",
        "class": "def f(a):\n    return a.x\n",
        "nondescriptor": "def f(a):\n    return a.x\n",
        "module": "def f(a):\n    return a.maxsize\n",
        "hint": "def f(a):\n    return a.x\n",
        "getset": "def f(a):\n    return a.x\n",
        "poly": "def f(a):\n    return a.x\n",
        "instance": "def f(a):\n    return a.x\n",
        "nodict": "def f(a):\n    return a.x\n",
        "pointer": "def f(a):\n    return a.func\n"},
        [{"obj": v} for v in (Point(), WithSlots(), ClassAttr, ClassAttr(), sys, Many(),
                              Plain(), 1, len, Swapped(), materialised(), SlotsClassAttr(),
                              capi.VGetSet(), functools.partial(identity))],
        warm_by_source={"shape": (Point(),), "slot": (WithSlots(),), "class": (ClassAttr,),
                        "nondescriptor": (ClassAttr(),), "module": (sys,), "hint": (Many(),),
                        "getset": (capi.VGetSet(),), "poly": (Point(), Swapped()),
                        "instance": (materialised(),), "nodict": (SlotsClassAttr(),),
                        "pointer": (functools.partial(identity),)},
        warmups=200, coherent=True),
    "GetItem": Scenario("GetItem", {
        "list": "def f(a, k=1):\n    return a[k]\n",
        "tuple": "def f(a, k=1):\n    return a[k]\n",
        "ascii": "def f(a, k=1):\n    return a[k]\n",
        "unicode": "def f(a, k=1):\n    return a[k]\n",
        "dict": "def f(a, k=1):\n    return a[k]\n",
        "slice": "def f(a, k=slice(0, 2)):\n    return a[k]\n",
        "generic": "def f(a, k=1):\n    return a[k]\n"},
        [{"obj": o, "key": k} for o in ([10, 20, 30], (10, 20, 30), "abc", "a\u00e9c",
                                        {1: 2, 2**70: 3}, Getter(), Plain())
         for k in (0, 1, -1, 7, -9, 1, 2**70, "x", slice(0, 2), 1.5)],
        warm_by_source={"list": ([1, 2],), "tuple": ((1, 2),), "ascii": ("ab",),
                        "unicode": ("a\u00e9",), "dict": ({1: 2},),
                        "slice": ([1, 2, 3],), "generic": (Getter(),)},
        warmups=200, coherent=True),
    "SetItem": Scenario("SetItem", {
        "list": "def f(a, k=1, v=5):\n    a[k] = v\n",
        "dict": "def f(a, k=1, v=5):\n    a[k] = v\n",
        "generic": "def f(a, k=1, v=5):\n    a[k] = v\n"},
        [{"obj": o, "key": k, "value": v}
         for o in ([10, 20, 30], {1: 2}, Getter(), (1, 2))
         for k in (0, 1, -1, 7, "x") for v in (5, None)],
        warm_by_source={"list": ([1, 2],), "dict": ({1: 2},), "generic": (Getter(),)},
        warmups=200, coherent=True),
    "DelItem": Scenario("DelItem", {
        "generic": "def f(a, k=0):\n    del a[k]\n"},
        [{"obj": o, "key": k} for o in ([10, 20, 30], {1: 2, 0: 1}, Getter(), (1, 2))
         for k in (0, 1, -1, 7, "x")], warm=([1, 2],)),
    "SliceRead": Scenario("SliceRead", {
        "generic": "def f(a, i=0, j=1):\n    return a[i:j]\n"},
        [{"obj": o, "start": i, "stop": j}
         for o in ([1, 2, 3], (1, 2, 3), "abc", Getter(), 5)
         for i, j in ((0, 1), (None, None), (-2, 5), ("x", 1))], warm=([1, 2],)),
    "SliceWrite": Scenario("SliceWrite", {
        "generic": "def f(a, i=0, j=1, v=()):\n    a[i:j] = v\n"},
        [{"obj": o, "start": i, "stop": j, "value": v}
         for o in ([1, 2, 3], Getter(), 5) for i, j in ((0, 1), (None, None), ("x", 1))
         for v in ([9], 7)], warm=([1, 2],)),
    "BuildSlice": Scenario("BuildSlice", {
        "generic": "def f(a, i=0, j=1, k=2):\n    return a[i:j:k]\n"},
        [{"parts": parts} for parts in ([0, 1], [0, 1, 2], [None, None, None],
                                        [Plain(), 1], [1, 2, 0])], warm=([1, 2],)),
    "GetMethod": Scenario("GetMethod", {
        "values": "def f(a):\n    return a.norm()\n",
        "nodict": "def f(a):\n    return a.norm()\n",
        "classmethod": "def f(a):\n    return a.make()\n",
        "poly": "def f(a):\n    return a.norm()\n",
        "lazy": "def f(a):\n    return a.norm()\n"},
        [{"obj": v} for v in (Point(), WithSlots(), ClassAttr(), ClassAttr, Plain(), 1,
                              Swapped(), LazyNorm())],
        warm_by_source={"values": (Point(),), "nodict": (WithSlots(),),
                        "classmethod": (ClassAttr,), "poly": (Point(), Swapped()),
                        "lazy": (LazyNorm(),)},
        warmups=200, coherent=True),
    "GlobalRead": Scenario("GlobalRead", {
        "module": "G = 3\ndef f():\n    return G\n",
        "builtin": "def f():\n    return len\n",
        "missing": "def f():\n    return no_such_global\n"},
        [{}], warm_by_source={"module": ((),), "builtin": ((),), "missing": ((),)},
        warmups=200, coherent=True),
    "Return": Scenario("Return", {
        "empty": "def f(a):\n    return a\n",
        "monitored": "def f(a):\n    return a\n"},
        [{"value": v} for v in (1, "s", Plain(), [1])], warm=([1],),
        words_of={"Return.Ordinary": "Return.Empty"}, monitored=("monitored",),
        skip={"Return.External": "chosen only for the evaluator's entry frame, which "
                                 "no function's word runs in"}),
    "Call": Scenario("Call", {
        "native": "def f(a):\n    return len(a)\n",
        "monitored": "def f(a):\n    return len(a)\n"},
        [{"fn": len, "args": [v]} for v in ([1, 2], "abc", 5)]
        + [{"fn": abs, "args": [v]} for v in (-3, "x")]
        + [{"fn": fn, "args": [v]} for fn in (identity, Point().norm, Holder, two)
           for v in (1, "s")], warm=([],),
        monitored=("monitored",)),
    "Build": Scenario("Build", {
        "tuple": "def f(a, b):\n    return (a, b)\n",
        "list": "def f(a, b):\n    return [a, b]\n",
        "set": "def f(a, b):\n    return {a, b}\n",
        "map": "def f(a, b):\n    return {a: b}\n"},
        [{"items": [x, y]} for x, y in ((1, 2), ("s", 2.5), (2**70, "t"), ((), None))],
        warm=(1, 2), holds=True),
    "AddElement": Scenario("AddElement", {
        "append": "def f(a):\n    return [x for x in a]\n",
        "add": "def f(a):\n    return {x for x in a}\n"},
        [{"container": c, "item": x} for c, x in (([], 1), ([3], "s"), ([], 2.5))],
        warm=([1],), holds=True,
        # (a set takes only a set: the word's arm is chosen by the display)
        inputs_by_source={"add": [{"container": c, "item": x}
                                  for c, x in ((set(), 1), ({3}, "s"), (set(), 2.5))]}),
    "AddEntry": Scenario("AddEntry", {
        "put": "def f(a):\n    return {x: x for x in a}\n"},
        [{"container": {}, "key": 1, "value": "v"}, {"container": {2: 3}, "key": "s",
                                                      "value": None}],
        warm=([1],), holds=True),
    "UpdateContainer": Scenario("UpdateContainer", {
        "extend": "def f(a):\n    return [*a]\n",
        "union": "def f(a):\n    return {*a}\n",
        "overlay": "def f(a):\n    return {**a}\n"},
        [{"container": [], "source": [1]}],
        # (each arm takes only its own kind of container)
        inputs_by_source={
            "extend": [{"container": c, "source": x} for c, x in (
                ([], [1]), ([0], (2, 3)), ([], "ab"), ([], 5), ([1], iter([2])))],
            "union": [{"container": c, "source": x} for c, x in (
                (set(), [1]), ({0}, (2, 3)), (set(), "ab"), (set(), 5), ({1}, {2: 3}))],
            "overlay": [{"container": c, "source": x} for c, x in (
                ({}, {1: 2}), ({"a": 1}, {"a": 2}), ({}, 5), ({}, [1]))]},
        warm=([1],), holds=True),
    "MergeKeywords": Scenario("MergeKeywords", {
        "merge": "def f(g, a, b):\n    return g(**a, **b)\n"},
        [{"fn": identity, "dict": {}, "source": {"k": 1}},
         {"fn": identity, "dict": {"k": 1}, "source": {"j": 2}},
         {"fn": identity, "dict": {"k": 1}, "source": {"k": 2}},
         {"fn": identity, "dict": {}, "source": 5}],
        warm=(identity, {}, {}), holds=True),
    "Function": Scenario("Function", {
        "def": "def f():\n    def g():\n        pass\n    return g\n",
        "defaults": "def f(a):\n    def g(x=a):\n        pass\n    return g\n",
        "all": "def f(a, b):\n    def g(x=a, *, k=b) -> int:\n        return a\n"
               "    return g\n"},
        # The function keeps the code and the attributes, which the count
        # check cannot see
        [{"code": code} for code in (identity.__code__, Holder.__init__.__code__)],
        warm=(1, 2), holds=True,
        inputs_by_source={
            "defaults": [{"code": identity.__code__, "fields": [v]}
                         for v in ((1,), (), (None, 2))],
            "all": [{"code": identity.__code__, "fields": [(1,), {"k": 2}, {"k": int}]}]}),
    "Intrinsic": Scenario("Intrinsic", {
        "unary": "def f(a):\n    return +a\n",
        "stopiteration": "def f(a):\n    yield a\n",
        "import_star": MODULE_SOURCE % "from sys import *"},
        [{"x": v} for v in (1, 1.5, 2**70, "s", Plain(), [1])], warm=(1,),
        inputs_by_source={
            # (A StopIteration becomes a new error that holds it, which the
            # count check cannot see; test_rop_functions runs that case)
            "stopiteration": [{"x": v} for v in (ValueError(1), KeyError("k"))],
            "import_star": [{"x": v} for v in (sys, capi, {"a": 1})]},
        skip={"Intrinsic.StopIteration": STOPITERATION_WHY}),
    "IntrinsicPair": Scenario("IntrinsicPair", {
        "binary": PAIR_SOURCE,
        "compare": "def f(a, b):\n" + "    a < b\n" * 1100},
        [{"x": x, "y": y} for x, y in PAIRS], warm=(1, 2), per_arm=True,
        inputs_by_source={
            # TYPEVAR_WITH_BOUND (a name, the bound's evaluator): the new
            # typevar holds the evaluator, so it is immortal
            "binary": [{"x": x, "y": None} for x in ("T", "U")]}),
}


MONITOR = sys.monitoring.OPTIMIZER_ID


EVENTS = ("CALL", "PY_RETURN", "BRANCH_LEFT", "BRANCH_RIGHT", "JUMP")


def monitor(code):
    mon = sys.monitoring
    if mon.get_tool(MONITOR) is None:
        mon.use_tool_id(MONITOR, "test_rop_ssa")
        for e in EVENTS:
            mon.register_callback(MONITOR, getattr(mon.events, e), lambda *args: None)
    events = 0
    for e in EVENTS:
        events |= getattr(mon.events, e)
    mon.set_local_events(MONITOR, code, events)


def tearDownModule():
    if sys.monitoring.get_tool(MONITOR) is not None:
        sys.monitoring.free_tool_id(MONITOR)


def compiled(scenario, source, arm=None):
    """A fresh function of the source, run (and warmed), and its family's
    first word, or for an arm chosen by `select` its first word of that arm:
    the function, the word's index and length, and the raw words"""
    namespace = {}
    exec(scenario.sources[source], namespace)
    f = namespace["f"]
    if source in scenario.monitored:
        monitor(f.__code__)
    phases = ([(o,) for o in scenario.warm_by_source[source]]
              if source in scenario.warm_by_source else [scenario.warm])
    for warm in phases:
        for _ in range(max(1, scenario.warmups)):
            try:
                f(*warm[:f.__code__.co_argcount])
            except Exception:
                pass
    words = capi.get_ir(f.__code__, True)
    index = next((i for i, w in enumerate(words)
                  if w[0].split("+")[0] == arm
                  or arm is None and w[0].split("+")[0].partition(".")[0] == scenario.family),
                 None)
    if index is None:
        return None
    length = 1
    while index + length < len(words) and words[index + length][0] in ("IR_OPERANDS", "OPERAND"):
        length += 1
    raw = capi.get_ir_raw(f.__code__)[-2]
    exec_words = [int.from_bytes(raw[i:i + 8], "little") for i in range(0, len(raw), 8)]
    return f, index, length, exec_words


TRANSFER = None   # rop_transfer_run instead of jit_template_run's modes


def flatten(x):
    if isinstance(x, tuple):
        return [y for item in x for y in flatten(item)]
    return [x]


class CRun:
    "One run of the C arm on a fresh word, and what it changed"

    def __init__(self, f, index, slots, mode, handler):
        code = f.__code__
        objects = list({id(o): o for o in slots.values()}.values())
        fb_before = capi.get_ir_raw(code)[-1]
        counts = [sys.getrefcount(o) for o in objects]
        self.kind, self.extra, self.slow = "same", None, False
        if mode is TRANSFER:
            self.kind, nxt, exc, out, self.extra = capi.rop_transfer_run(f, index, slots, handler)
        else:
            nxt, self.slow, exc, out = capi.jit_template_run(f, index, slots, mode, handler)
        self.exc_type = type(exc) if exc is not None else None
        del exc
        # Indexing, not zip: zip's reused result tuple would hold a reference
        self.counts = [sys.getrefcount(objects[i]) - counts[i] for i in range(len(objects))]
        self.objects = objects
        self.next = nxt
        self.slots = out
        raw = capi.get_ir_raw(code)[-2]
        self.handler = int.from_bytes(raw[index * 8:index * 8 + 8], "little") & 0x1FF
        self.feedback_written = capi.get_ir_raw(code)[-1] != fb_before


def classify(run, mode, index, length, jump_kind):
    if run.kind != "same":
        return run.kind
    if mode == STUB:
        for bit, name in STATUS:
            if run.next & bit:
                return name
        return "next"
    if run.next is None:
        return "raise"
    if run.next == index:
        return "again"
    if run.next == index + length:
        return "next"
    return jump_kind


def transfers(arm):
    "An arm that may leave the frame: it runs under rop_transfer_run"
    return bool({"enter", "return"} & set(arm.contract.exits))


def is_generic(arm):
    return arm in rop_verify.generic_arms(PROGRAM, arm.family)


def is_mortal(o):
    return not sys._is_immortal(o)


@dataclass
class Result:
    agreed: bool
    exit: str
    why: str = ""
    c_counts: str = ""


REPEATS = 3


@dataclass
class Word:
    "A fresh word of a scenario's source for an arm, and the homes to fill"
    f: object
    index: int
    length: int
    site: object
    slots: dict
    homes: dict


def prepare(arm_name, scenario, source, inputs):
    """A fresh word that the arm runs on, with its homes filled from the
    inputs, or None if the arm does not run on the scenario's word"""
    arm = PROGRAM.arms[arm_name]
    if arm_name in scenario.skip:
        return None
    if arm.observes and source not in scenario.monitored:
        return None
    if arm_name in scenario.requires and scenario.requires[arm_name] not in inputs:
        return None
    # An arm chosen by static evidence runs only on the words it was chosen for
    built = compiled(scenario, source,
                     scenario.words_of.get(arm_name, arm_name)
                     if arm.select or scenario.per_arm else None)
    if built is None:
        return None
    f, index, length, words = built
    current = capi.get_ir(f.__code__, True)[index][0].split("+")[0]
    if scenario.coherent and current != arm_name and not is_generic(arm):
        return None
    site = ev.decode_site(PROGRAM, arm.family, words, index, length)
    ssa = SSA[arm_name]
    homes = {site.operands[n]: n for n, k in ssa.operands.items() if k is FieldKind.home}
    slots = {}
    for operand, value in inputs.items():
        if ssa.operands[operand] is FieldKind.vector:
            if len(value) != len(site.operands[operand]):
                return None
            for slot, v in zip(site.operands[operand], value):
                slots[slot] = v
                homes[slot] = operand
        else:
            slots[site.operands[operand]] = value
    # A destination span's homes are compared too
    for param in PROGRAM.families[arm.family].params:
        if rop_check.is_span_of(param.type, rop_check.DST):
            for slot in site.operands[param.name]:
                homes.setdefault(slot, param.name)
    for slot, operand in homes.items():
        if slot not in slots and operand in ("dst", "callable"):
            slots[slot] = Old()
    return Word(f, index, length, site, slots, homes)


def check(cx, arm_name, scenario, source, inputs):
    """Run the C arm REPEATS times on one word (its counter moves on between
    runs, so a generic arm adapts and then runs) and the expansion on the
    same homes; a Result for each run, None if the arm does not run on the
    scenario's word"""
    word = prepare(arm_name, scenario, source, inputs)
    if word is None:
        return None
    arm = PROGRAM.arms[arm_name]
    f, index, length, site, slots, homes = (word.f, word.index, word.length, word.site,
                                            word.slots, word.homes)
    ssa = SSA[arm_name]
    before = {slot: capi.list_value_bits([v], 0) for slot, v in slots.items()}
    constants = [capi.list_value_bits([c], 0) for c in f.__code__.co_consts]
    mode = (STUB if arm.region is Region.shared
            else TRANSFER if transfers(arm) else FORM)
    results = []
    for _ in range(REPEATS):
        if scenario.before_run is not None:
            scenario.before_run(inputs)
        results.append(one_run(cx, arm, ssa, f, index, length, site, slots, homes,
                               before, constants, mode, inputs, scenario.holds))
    return results


def one_run(cx, arm, ssa, f, index, length, site, slots, homes, before, constants,
            mode, inputs, holds=False):
    arm_name = arm.name
    handler = capi.arm_handlers()[arm_name]
    run = CRun(f, index, slots, mode, handler)
    jump_kind = "jump" if arm.family == "Jump" else "branch"
    exit = classify(run, mode, index, length, jump_kind)
    if arm.family == "NamespaceWrite" and "value" in inputs and exit == "next":
        # The value's new owner is the globals dict, which the count of the
        # homes does not see: the global holds the value itself
        stored = f.__globals__.get("g")
        if stored is not inputs["value"]:
            return Result(False, exit, f"C stored {stored!r} in the global")
    obs = ev.Observation(exit, run.next if mode != STUB else None, run.slots,
                         run.feedback_written, run.handler != handler,
                         (run.extra,) if exit == "return" else None)
    if exit == "enter":
        # The child runs the callee (a class's __init__, after the new
        # self), with the arguments in its first slots
        fn, child = run.extra
        callee, first = inputs["fn"], 0
        if isinstance(callee, type):
            callee, first = callee.__init__, 1
        elif hasattr(callee, "__func__"):
            callee, first = callee.__func__, 1
        args = inputs.get("args", [])
        got = child[first:first + len(args)]
        if fn is not callee or not all(
                g is a or g == capi.list_value_bits([a], 0) for g, a in zip(got, args)):
            return Result(False, exit, f"C entered {fn!r} with {got!r}")
    outs = ev.outcomes(PROGRAM, ssa, cx, site, before, constants)
    if mode != STUB:
        generic = next((a.name for a in PROGRAM.arms_of(arm.family) if is_generic(a)), None)
        if generic is not None and generic != arm_name:
            expanded = []
            for out in outs:
                if out.exit == "miss":
                    expanded.extend(ev.outcomes(PROGRAM, SSA[generic], cx, site, before,
                                                constants))
                else:
                    expanded.append(out)
            outs = expanded
    reasons = []
    for out in outs:
        why = (ev.disagreement(cx, out, obs, homes) or ev.balance(cx, out, before)
               or ("; ".join(out.problems) if out.problems else None))
        if why is None:
            break
        reasons.append(why)
    else:
        return Result(False, obs.exit, f"no path agrees; {len(outs)} paths: "
                      + " | ".join(sorted(set(reasons))[:6]))
    # The C arm's own ownership: each input's count changed by the homes
    # that hold it after the word.  Not checked after an exception (its
    # traceback holds the frame) or a feedback write (an installed recipe
    # may hold the receiver's type)
    for i, o in enumerate(run.objects):
        delta = run.counts[i]
        after = run.slots + tuple(flatten(run.extra))
        # A new instance of a heap class holds its class
        held = (sum(1 for s in after if s is o)
                + sum(1 for s in after if type(s) is o and not any(s is x for x in run.objects)))
        # (In the entered __init__ frame, the new instance is not yet a home)
        if (not holds and is_mortal(o) and run.exc_type is None and not run.feedback_written
                and not (run.kind == "enter" and isinstance(o, type))
                and delta != held):
            return Result(False, obs.exit, f"C changed the count of {o!r} by {delta:+d}, "
                          f"and {held} homes hold it", "leak")
    return Result(True, obs.exit)


def cx():
    return ir_leaf_eval.Context(ir_leaf_eval.Layout.from_c(capi), ir_leaf_eval.RealMemory())


# The arms no scenario runs, and why.  Each expands.
NOT_RUN = {
    # The tracer's view of the keyword and expanded calls is deferred while the
    # interpreter ports finish (rir90 interpreter-first rule)
    "CallKw.Plan": "deferred: binds keywords by a site's plan",
    "CallKw.Generic": "deferred: the keyword call that adapts, then binds or calls",
    "CallKw.Observed": "deferred: the keyword call under monitoring",
    "CallEx.Generic": "deferred: the call with the star tuple (its owner is a block parameter)",
    "CallEx.Observed": "deferred: the star call under monitoring",
    "MaterializeKwargs.Pending": "deferred: the pending **kwargs dict of a frame",
    "InitResult.Check": "its word exists only in the init shim",
    "Return.External": "chosen only for the evaluator's entry frame",
    "Return.Generator": "returns from a generator frame, which rop_transfer_run's "
                        "synthetic caller cannot hold",
    "NamespaceRead.Name": "LOAD_NAME occurs in module and class code, which the probe "
                          "cannot call as a function",
    "NamespaceRead.Locals": "LOAD_LOCALS occurs in class code",
    "NamespaceReadFrom.Mapping": "LOAD_FROM_DICT_OR_GLOBALS occurs in class code",
    "CellMapRead.Lookup": "LOAD_FROM_DICT_OR_DEREF occurs in class code",
    "NamespaceWrite.Name": "STORE_NAME occurs in module and class code",
    "SetupAnnotations.Setup": "SETUP_ANNOTATIONS occurs in module and class code",
    "Import.Policy": "a function's imports are eager; the policy applies to module code",
    "Import.Lazy": "`lazy import` is a module-level statement",
    # select(jit_on) and select(jit_off): the word is the one the JIT's
    # state chose when the function was lowered
    "Intrinsic.StopIteration": STOPITERATION_WHY,
    "IntrinsicPair.Attr": "GETATTR and SETATTR are escapes for a name too wide for "
                          "LOAD_ATTR and STORE_ATTR; no function's source compiles "
                          "to one on a word",
    "Jump.Poll" if sys._jit.is_enabled() else "Jump.Warm": "the JIT's state selects the "
                                                           "other loop jump",
}


def evaluated_arms():
    return [n for n in SSA if PROGRAM.arms[n].family in SCENARIOS]


class AgainstCTests(unittest.TestCase):
    """Every expanded arm of a family with scenarios, on every input"""

    maxDiff = None

    @classmethod
    def setUpClass(cls):
        cls.cx = cx()
        cls.exits: dict[str, set] = {}

    def run_arm(self, name):
        scenario = SCENARIOS[PROGRAM.arms[name].family]
        seen = self.exits.setdefault(name, set())
        for source in scenario.sources:
            for inputs in scenario.inputs_of(source):
                with self.subTest(arm=name, source=source, inputs=inputs):
                    for result in check(self.cx, name, scenario, source, inputs) or ():
                        seen.add(result.exit)
                        self.assertTrue(result.agreed, result.why)

    def test_arms(self):
        for name in evaluated_arms():
            self.run_arm(name)
        ran = {n for n, seen in self.exits.items() if seen}
        self.assertEqual(set(SSA) - ran, set(NOT_RUN))


def templated_arms():
    "The arms that have a baseline template"
    p = rop_generator.lower_all(PROGRAM, rop_generator.number_arms(PROGRAM))
    made, _ = rop_generator.template_texts(p)
    return sorted(made)


def same_slot(x, y):
    "The same object, or equal values of one type (a new int or a word)"
    return x is y or (type(x) is type(y) and x == y)


class TemplateTests(unittest.TestCase):
    """Every arm's template against its form, on every scenario: each runs
    on a fresh word of its own, from the same homes, and they must agree on
    the next word, the exception, every slot, each input's count and the
    feedback and handler writes.  Each template stays on its own code for
    some input."""

    maxDiff = None

    def run_both(self, name, scenario, source, inputs):
        handler = capi.arm_handlers()[name]
        words = [prepare(name, scenario, source, inputs) for _ in (TEMPLATE, FORM)]
        if None in words:
            return None
        # One set of objects for both runs, so that their counts compare.  A
        # destination temporary is vacant, as the translator leaves it: a
        # template stores into it without a close
        nfixed = capi.get_ir_raw(words[0].f.__code__)[2]
        slots = {slot: v for slot, v in words[0].slots.items()
                 if not (isinstance(v, Old) and slot >= nfixed)}
        runs = [CRun(w.f, w.index, slots, mode, handler)
                for w, mode in zip(words, (TEMPLATE, FORM))]
        template, form = runs
        self.assertEqual(template.next, form.next, "next word")
        self.assertIs(template.exc_type, form.exc_type)
        self.assertEqual(len(template.slots), len(form.slots))
        for i, (x, y) in enumerate(zip(form.slots, template.slots)):
            self.assertTrue(same_slot(x, y), f"slot {i}: {x!r} != {y!r}")
        self.assertEqual(template.counts, form.counts, "counts")
        self.assertEqual(template.feedback_written, form.feedback_written, "feedback")
        self.assertEqual(template.handler, form.handler, "handler")
        return not template.slow

    def test_templates_against_forms(self):
        arms = templated_arms()
        self.assertTrue(arms)
        for name in arms:
            scenario = SCENARIOS.get(PROGRAM.arms[name].family)
            if scenario is None or name in NOT_RUN:
                continue
            fast = 0
            for source in scenario.sources:
                for inputs in scenario.inputs_of(source):
                    with self.subTest(arm=name, source=source, inputs=inputs):
                        fast += bool(self.run_both(name, scenario, source, inputs))
            with self.subTest(arm=name):
                self.assertGreater(fast, 0, "no input stayed on the template's code")


if __name__ == "__main__":
    unittest.main()
