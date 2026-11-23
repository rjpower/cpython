"""The baseline JIT's inline templates: the primitive lowerings
(Python/jit_lower.c) against their C definitions, and each template
(Python/jit_templates.c.h) against its handler's function form on one word
of a frame."""

import ctypes
import math
import random
import struct
import sys
import unittest
import weakref

from test import test_tools
from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")
if not hasattr(capi, "jit_lower"):
    raise unittest.SkipTest("the JIT is not supported on this platform")
test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import ir_leaf
    import jit_template_generator

from test.test_ir_leaf import (
    FLOATS, INDEX_WORDS, INLINE_WORDS, NAN, OBJECTS, SEPARATE, TAG, WIDE_INTS,
    cases, context, refcount, same, word, Holder, LAYOUT,
)

MASK = 2**64 - 1
RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI = range(8)
R8, R9, R10, R11, R12, R13, R14, R15 = range(8, 16)
POOL = [RAX, RCX, RDX, RSI, RDI, R8, R9, R10, R11]
# Registers that no lowering may write
FIXED = [RBX, R12, R13, R14, R15]
# Tested on their own below: they change counts or allocate
COUNTS = {"PyValue_DUP", "PyValue_XDUP", "PyValue_CLOSE", "PyValue_XCLOSE",
          "PyValue_CLOSE_SPECIALIZED", "PyValue_FromInt64", "PyValue_FromDouble"}
# Tested on their own: they read the frame of the test's caller, or write
FRAME = {"GLOBALS", "BUILTINS", "FRAME_CO_CONSTS", "GETLOCAL", "IR_SLOTS"}
SLOTS = {"_PyObject_ShapeReadDup", "_PyDictValues_SetShape",
         "_PyDictValues_AddToInsertionOrder"}
APART = COUNTS | FRAME | SLOTS
HIGH = [R8, R9, R10, R11, RSI, RDI]


def double_bits(d):
    return struct.unpack("<Q", struct.pack("<d", d))[0]


def bits_double(b):
    return struct.unpack("<d", struct.pack("<Q", b))[0]


def canaries(rng):
    return ([rng.getrandbits(64) for _ in range(16)],
            [rng.getrandbits(64) for _ in range(16)])


def lowerings():
    return {name: (nres, nargs, ntemps, nxtemps, kinds, predicate)
            for name, nres, nargs, ntemps, nxtemps, kinds, predicate
            in capi.jit_lowerings()}


def assignments(kinds, nres, nargs, rng, count=3, alias=True):
    """Register assignments of a lowering's operands: distinct registers; if
    alias, also each result in the register of each argument of its kind;
    and the operands in the high registers."""
    out = []
    for _ in range(count):
        gprs, xmms = rng.sample(POOL, len(POOL)), rng.sample(range(16), 16)
        out.append([gprs.pop() if k == "g" else xmms.pop() for k in kinds])
    for r in range(nres if alias else 0):
        for a in range(nres, nres + nargs):
            if kinds[r] == kinds[a]:
                ops = list(out[0])
                ops[r] = ops[a]
                out.append(ops)
    # The operands in the high registers: r8-r11, rsi and rdi, then the rest
    gprs = rng.sample(HIGH, len(HIGH)) + rng.sample(
        [r for r in POOL if r not in HIGH], len(POOL) - len(HIGH))
    xmms = rng.sample(range(8, 16), 8) + rng.sample(range(8), 8)
    out.append([gprs.pop(0) if k == "g" else xmms.pop(0) for k in kinds])
    return out


def variants(name, rng, count=2):
    """(ops, args_live) to run a lowering with: for one that may call, also
    with its arguments live after it"""
    nres, nargs, *_, kinds, _ = lowerings()[name]
    out = [(ops, False) for ops in assignments(kinds, nres, nargs, rng, count)]
    if jit_template_generator.LOWERINGS[name].calls:
        out += [(ops, True) for ops in
                assignments(kinds, nres, nargs, rng, count, alias=False)]
    return out


def run(name, ops, args, rng, aux=None, args_live=False, leaf_out=None):
    """Run a lowering on args in the registers ops, with the arguments live
    after it if args_live.  Returns the predicate's value, the results and
    the registers before and after.  The frame primitives read the frame of
    their Python caller, so a test of one asks for the C definition's
    result here, in the same frame: leaf_out gets it."""
    nres, nargs, *_, kinds, _ = lowerings()[name]
    gprs, xmms = canaries(rng)
    for i, arg in enumerate(args):
        reg = ops[nres + i]
        if kinds[nres + i] == "x":
            xmms[reg] = double_bits(arg)
        else:
            gprs[reg] = arg & MASK
    if leaf_out is not None:
        leaf_out.append(capi.leaf_primitive(name, *args))
    cond, gprs, gout, xout = capi.jit_lower(name, tuple(ops), tuple(gprs), tuple(xmms),
                                            aux, args_live)
    results = [bits_double(xout[ops[i]]) if kinds[i] == "x" else gout[ops[i]]
               for i in range(nres)]
    return cond, results, (gprs, xmms), (gout, xout)


def as_type(value, type_):
    if type_ == "i64" and value >= 2**63:
        return value - 2**64
    return value


def a_function():
    pass


class LoweringTests(unittest.TestCase):
    def check_registers(self, name, ops, before, after, args_live=False):
        """Only the results and the temporaries change.  The arguments stay
        unless a result shares their register, or the lowering may call and
        they are not live after it."""
        nres, nargs, ntemps, nxtemps, kinds, _ = lowerings()[name]
        written = {(kinds[i], ops[i]) for i in range(nres)}
        written |= {(kinds[i], ops[i]) for i in range(nres + nargs, len(kinds))}
        if jit_template_generator.LOWERINGS[name].calls and not args_live:
            written |= {(kinds[i], ops[i]) for i in range(nres, nres + nargs)}
        for kind, regs_before, regs_after in (("g", before[0], after[0]),
                                              ("x", before[1], after[1])):
            for reg in range(16):
                if kind == "g" and reg in (RSP, RBP):
                    continue
                if (kind, reg) in written:
                    continue
                self.assertEqual(regs_after[reg], regs_before[reg],
                                 f"{name} wrote {kind}{reg} with ops {ops}")

    def test_every_lowering_is_tabled(self):
        self.assertEqual(set(lowerings()), set(jit_template_generator.LOWERINGS))
        for name, (nres, nargs, ntemps, nxtemps, kinds, predicate) in lowerings().items():
            prim = ir_leaf.PRIMITIVES[name]
            lowering = jit_template_generator.LOWERINGS[name]
            self.assertEqual(kinds, jit_template_generator.operand_kinds(prim, lowering))
            self.assertEqual(predicate, jit_template_generator.is_predicate(prim))

    def check_against_c(self, name, args, rng):
        nres, nargs, ntemps, nxtemps, kinds, predicate = lowerings()[name]
        prim = ir_leaf.PRIMITIVES[name]
        if prim.pre is not None and not prim.pre(context(), *args):
            return
        expected = capi.leaf_primitive(name, *args)
        for ops in assignments(kinds, nres, nargs, rng, count=2):
            with self.subTest(primitive=name, args=args, ops=ops):
                cond, results, before, after = run(name, ops, args, rng)
                self.check_registers(name, ops, before, after)
                if not predicate:
                    self.assertIsNone(cond)
                    self.assertTrue(same(as_type(results[0], prim.results[0]), expected),
                                    (results, expected))
                elif not nres:
                    self.assertIs(cond, expected)
                else:
                    self.assertIs(cond, expected[0])
                    if cond:
                        self.assertTrue(same(tuple(results), expected[1:]),
                                        (results, expected))

    def test_against_c(self):
        rng = random.Random(88)
        for name in lowerings():
            if name in APART:
                continue
            prim = ir_leaf.PRIMITIVES[name]
            for args in [()] if not prim.params else cases(prim):
                self.check_against_c(name, args, rng)

    def test_more_objects(self):
        "Objects beyond test_ir_leaf's: types of other sizes, other lengths and key sizes"
        class Wide:
            def __init__(self, n):
                for i in range(n):
                    setattr(self, f"f{i}", i)
        class Slotted(Holder):
            __slots__ = ("c", "d")
        instances = [Holder(), Slotted(), *(Wide(n) for n in (1, 5, 12)), Wide(12)]
        # Dicts of every index width, with and without deletions
        dicts = [{f"k{i}": i for i in range(n)} for n in (0, 1, 5, 6, 11, 22, 100, 300, 700, 5000)]
        dicts.append({f"d{i}": i for i in range(40)})
        del dicts[-1]["d3"]
        dicts += [vars(unittest), Wide(3).__dict__]
        keys = [ctypes.c_uint64.from_address(id(d) + LAYOUT.offsets["PyDictObject.ma_keys"]).value
                for d in dicts]
        lists = [[], [1], list(range(40)), ["a", None, 2**70, 1.5], [[]] * 3]
        tuples = [(), (1,), tuple(range(40)), ("a", None, 2**70, 1.5)]
        # Objects only: an int in the inline range has no object (False is the zero one)
        ints = [2**70, -2**70, 2**64, True, False]
        rng = random.Random(94)
        extra = {
            "Py_TYPE": [(id(o),) for o in [*instances, *dicts, *lists, *tuples, *ints]],
            "_PyObject_InlineValues": [(id(o),) for o in instances if hasattr(o, "__dict__")],
            "_PyObject_Shape": [(id(o),) for o in instances if hasattr(o, "__dict__")],
            "_PyObject_ValueSlot": [(id(o), k) for o in instances for k in (0, 8, 16, 40, 100)],
            "DK_UNICODE_ENTRIES": [(k,) for k in keys],
            "_PyLong_IsZero": [(id(i),) for i in ints],
            "PyList_GET_SIZE": [(id(o),) for o in lists],
            "_PyList_ITEMS": [(id(o),) for o in lists],
            "PyTuple_GET_SIZE": [(id(o),) for o in tuples],
            "PyList_GET_VALUE": [(id(o), i) for o in lists for i in range(len(o))],
            "PyTuple_GET_VALUE": [(id(o), i) for o in tuples for i in range(len(o))],
        }
        for name in ("PyList_CheckExact", "PyTuple_CheckExact", "PyDict_CheckExact",
                     "PyModule_CheckExact", "PyUnicode_CheckExact", "PyFunction_Check"):
            extra[name] = [(id(o),) for o in [*instances, *dicts, *lists, *tuples, "s", unittest,
                                              len, a_function, str, int]]
        for name, arg_cases in extra.items():
            for args in arg_cases:
                self.check_against_c(name, args, rng)

    def take(self, w):
        "The object that an owning word names, its count released"
        self.assertEqual(w & TAG, 0)
        obj = ctypes.cast(w, ctypes.py_object).value
        ctypes.pythonapi.Py_DecRef(ctypes.py_object(obj))
        return obj

    def test_constructors(self):
        rng = random.Random(89)
        for name, values in (("PyValue_FromInt64", WIDE_INTS),
                             ("PyValue_FromDouble", FLOATS + [NAN])):
            nres, nargs, _, _, kinds, _ = lowerings()[name]
            for value in values:
                expected = capi.leaf_primitive(name, value)
                for ops, live in variants(name, rng, 2):
                    with self.subTest(primitive=name, value=value, ops=ops):
                        _, (w,), before, after = run(name, ops, (value,), rng, args_live=live)
                        self.check_registers(name, ops, before, after, live)
                        if w >> 48:
                            self.assertEqual(w, expected)
                        else:
                            # A box, as C made: its value
                            self.assertTrue(same(self.take(w), value))
                            self.assertTrue(same(expected, value))

    def test_counts(self):
        rng = random.Random(90)
        table = lowerings()
        for obj in [*OBJECTS, 2**75]:
            w = word(obj)
            for name in ("PyValue_DUP", "PyValue_XDUP"):
                nres, nargs, _, _, kinds, _ = table[name]
                for ops, live in variants(name, rng, 2):
                    with self.subTest(primitive=name, obj=obj, ops=ops):
                        before_count = refcount(obj)
                        capi.leaf_primitive(name, w)
                        c_change = refcount(obj) - before_count
                        _, (result,), before, after = run(name, ops, (w,), rng, args_live=live)
                        self.check_registers(name, ops, before, after, live)
                        self.assertEqual(result, w)
                        self.assertEqual(refcount(obj) - before_count, 2 * c_change)
                        for _ in range(2 * c_change):
                            ctypes.pythonapi.Py_DecRef(ctypes.py_object(obj))
            closes = [("PyValue_CLOSE", None), ("PyValue_XCLOSE", None)]
            if type(obj) is int:
                closes.append(("PyValue_CLOSE_SPECIALIZED", "_PyLong_ExactDealloc"))
            if type(obj) is float:
                closes.append(("PyValue_CLOSE_SPECIALIZED", "_PyFloat_ExactDealloc"))
            for name, aux in closes:
                nres, nargs, _, _, kinds, _ = table[name]
                for ops, live in variants(name, rng, 2):
                    with self.subTest(primitive=name, obj=obj, ops=ops):
                        ctypes.pythonapi.Py_IncRef(ctypes.py_object(obj))
                        before_count = refcount(obj)
                        _, _, before, after = run(name, ops, (w,), rng, aux, args_live=live)
                        self.check_registers(name, ops, before, after, live)
                        change = refcount(obj) - before_count
                        self.assertEqual(change, -1 if w & TAG == 0 else 0)
                        if not change:
                            ctypes.pythonapi.Py_DecRef(ctypes.py_object(obj))
        for w in [0, *INLINE_WORDS[:5], *INDEX_WORDS]:
            for name in ("PyValue_XDUP", "PyValue_XCLOSE"):
                nres, nargs, _, _, kinds, _ = table[name]
                for ops, live in variants(name, rng, 1):
                    with self.subTest(primitive=name, word=w, ops=ops):
                        _, results, before, after = run(name, ops, (w,), rng, args_live=live)
                        self.check_registers(name, ops, before, after, live)
                        self.assertEqual(results, [w] if nres else [])

    def test_last_count(self):
        "The release of the last count destroys the object, through its call"
        class Box:
            pass
        rng = random.Random(91)
        for name in ("PyValue_CLOSE", "PyValue_XCLOSE"):
            nres, nargs, _, _, kinds, _ = lowerings()[name]
            for ops, live in variants(name, rng, 3):
                with self.subTest(primitive=name, ops=ops):
                    box = Box()
                    ref = weakref.ref(box)
                    w = word(box)
                    ctypes.pythonapi.Py_IncRef(ctypes.py_object(box))
                    del box
                    _, _, before, after = run(name, ops, (w,), rng, args_live=live)
                    self.check_registers(name, ops, before, after, live)
                    self.assertIsNone(ref())
        # Only a NaN has a float object; an int beyond the inline range one
        name = "PyValue_CLOSE_SPECIALIZED"
        for new, aux in ((lambda: 2**80 * rng.randint(3, 1000), "_PyLong_ExactDealloc"),
                         (lambda: float("nan"), "_PyFloat_ExactDealloc")):
            for ops, live in variants(name, rng, 3):
                with self.subTest(aux=aux, ops=ops):
                    obj = new()
                    w = word(obj)
                    ctypes.pythonapi.Py_IncRef(ctypes.py_object(obj))
                    del obj
                    _, _, before, after = run(name, ops, (w,), rng, aux, args_live=live)
                    self.check_registers(name, ops, before, after, live)

    def test_frame(self):
        "The frame primitives read the frame of run(): its globals, constants and slots"
        rng = random.Random(92)
        for name, cases_ in (("GLOBALS", [()]), ("BUILTINS", [()]), ("FRAME_CO_CONSTS", [()]),
                             ("IR_SLOTS", [()]),
                             # run()'s parameters are its first slots
                             ("GETLOCAL", [(i,) for i in range(7)])):
            nres, nargs, _, _, kinds, _ = lowerings()[name]
            for args in cases_:
                for ops in assignments(kinds, nres, nargs, rng):
                    with self.subTest(primitive=name, args=args, ops=ops):
                        expected = []
                        _, (result,), before, after = run(name, ops, args, rng,
                                                          leaf_out=expected)
                        self.check_registers(name, ops, before, after)
                        self.assertEqual(result, expected[0])
        self.assertEqual(capi.leaf_primitive("GLOBALS"), id(globals()))

    def instance_slots(self, obj):
        "The byte offsets of an instance's inline value slots, from obj"
        values = capi.leaf_primitive("_PyObject_InlineValues", id(obj))
        first = values - id(obj) + LAYOUT.offsets["PyDictValues.values"]
        return [first + 8 * i for i in range(2)]

    def test_shape_read_dup(self):
        "The slot's word, with a count of its own if it owns one"
        rng = random.Random(93)
        name = "_PyObject_ShapeReadDup"
        for a, b in ((["x"], 2**70), (3.5, None), ("str", {1: 2}), (7, 1e300)):
            holder = Holder()
            holder.a, holder.b = a, b
            for offset in self.instance_slots(holder):
                expected = capi.leaf_primitive(name, id(holder), offset)
                # An object that the slot owns, whose count C raised
                owned = expected < 2**48 and not expected & TAG
                if owned:
                    obj = ctypes.cast(expected, ctypes.py_object).value
                    ctypes.pythonapi.Py_DecRef(ctypes.py_object(obj))
                for ops, live in variants(name, rng, 3):
                    with self.subTest(slot=(a, b), offset=offset, ops=ops, live=live):
                        if owned:
                            before_count = refcount(obj)
                        _, (result,), before, after = run(name, ops, (id(holder), offset),
                                                          rng, args_live=live)
                        self.check_registers(name, ops, before, after, live)
                        self.assertEqual(result, expected)
                        if owned:
                            self.assertEqual(refcount(obj) - before_count, 1)
                            ctypes.pythonapi.Py_DecRef(ctypes.py_object(obj))

    def test_dict_values_stores(self):
        "The stores change an instance's values as C's do, run on one object and restored"
        rng = random.Random(95)
        holder = Holder()
        values = capi.leaf_primitive("_PyObject_InlineValues", id(holder))
        capacity = ctypes.c_uint8.from_address(values).value
        start = LAYOUT.offsets["PyDictValues.values"]
        size = start + 9 * capacity
        saved = ctypes.string_at(values, size)

        def restore():
            ctypes.memmove(values, saved, size)

        def snapshot():
            return ctypes.string_at(values, size)

        for name, arg_cases in (("_PyDictValues_SetShape", [0, 1, 7, 255, 0x105, 2**64 - 3]),
                                ("_PyDictValues_AddToInsertionOrder", [0, 1, 17, 29])):
            nres, nargs, _, _, kinds, _ = lowerings()[name]
            for value in arg_cases:
                restore()
                capi.leaf_primitive(name, values, value)
                expected = snapshot()
                self.assertNotEqual(expected, saved)
                for ops in assignments(kinds, nres, nargs, rng):
                    with self.subTest(primitive=name, value=value, ops=ops):
                        restore()
                        _, _, before, after = run(name, ops, (values, value), rng)
                        self.check_registers(name, ops, before, after)
                        self.assertEqual(snapshot(), expected)
        restore()


def word_of(func, handler, nth=-1):
    "The index and fields of func's nth word whose handler is handler"
    words = capi.get_ir(func.__code__, True)
    found = [(i, w) for i, w in enumerate(words) if w[0] == handler]
    if not found:
        raise AssertionError(f"no {handler} in {[w[0] for w in words]}")
    return found[nth]


def nfixed(func):
    return capi.get_ir_raw(func.__code__)[2]


def big(n):
    "A new int object beyond the inline range"
    return 2**60 + n


class Plain:
    pass


# (source, warm-up arguments, handler, fields, cases).  The function's code
# is warmed with the arguments so that its last word with the handler is in
# the executing copy; each case gives the values of the slots that the
# fields name, in order (a dst field gets the old value that the store
# releases).  The template and the form run from the same slots.
INTS = [0, 1, -1, 7, -2**50, 2**50 - 1, 2**25, -2**25 - 3]
NOT_INTS = ["x", big(1), 2.5, None, []]
INT_PAIRS = ([(a, b) for a in INTS[::2] for b in INTS[1::2]] +
             [(a, b) for a in INTS[:3] for b in NOT_INTS[:2]] +
             [(b, a) for a in INTS[:3] for b in NOT_INTS])
FLOATS_ = [0.0, -0.0, 1.5, -2.25, 1e300, -1e-300, math.inf, -math.inf]
FLOAT_PAIRS = ([(a, b) for a in FLOATS_[::2] for b in FLOATS_[1::2]] +
               [(1.5, 2), (2, 1.5), (1.5, "x"), ("x", 1.5), (math.nan, 1.0),
                (1.0, math.nan), (math.nan, math.nan), (1.5, 1.5), (0.0, -0.0),
                (1.0, 0.0), (1e308, 1e308), (big(3), 1.0)])
COMPARE_OPS = ("<", "<=", "==", "!=", ">", ">=")
ANY = [None, True, False, 0, 1, "s", [], big(4), 2.5]
INDICES = [0, 1, 2, -1, -3, 3, 2**40, "x", True, big(1)]
SEQUENCES = [[10, "s", big(2)], [], [None]]


class Shape:
    k = 5

    def __init__(self, q=2):
        self.p = 1
        self.q = q

    def m(self):
        return self


class Slots:
    __slots__ = ("p", "q")
    k = 6

    def __init__(self, q=2):
        self.p = 1
        self.q = q

    def m(self):
        return self


class Other:
    k = 7

    def __init__(self):
        self.p = 1
        self.q = 3

    def m(self):
        return self


class Fresh:
    "A slot value made anew for each run, for an object that a run changes"
    def __init__(self, make):
        self.make = make


def same_slot(x, y):
    if type(x).__name__ == "range_iterator":
        return x.__reduce__()[1:] == y.__reduce__()[1:]
    if x is not y and type(x) is type(y) and type(x) in (Shape, Slots, Other):
        # Fresh instances: the same attributes, added in the same order.
        # vars() makes an instance's dict, which leaves the shape paths, so
        # only instances that no later case uses are read this way.
        if type(x) is Slots:
            return all(getattr(x, n, None) == getattr(y, n, None) for n in Slots.__slots__)
        return list(vars(x).items()) == list(vars(y).items())
    return same(x, y)


def tagged(i):
    "The raw word of a tagged index, as a FOR_ITER window holds it"
    return ("raw", capi.leaf_primitive("PyValue_TagIndex", i))


def reversed_shape():
    "A Shape whose attributes were set in the other order, so its shape differs"
    s = Shape.__new__(Shape)
    s.q = 2
    s.p = 1
    return s


def owners_with_z():
    s = Shape()
    s.z = 1
    return s


def owners():
    "Instances on and off the paths that the attribute templates take"
    extra = Shape()
    extra.z = 1
    missing = Shape()
    del missing.q
    unset = Slots()
    del unset.q
    return [Shape(), Shape(big(3)), Shape(2.5), Shape("s"), Slots(), Slots(None), extra,
            missing, unset, Other(), None, 4]


OWNERS = owners()
TRUTHS = ANY + [[1], -5, big(0), Plain(), "", Shape()]
BINARY = "def f(a, b):\n    x = 1\n    x = a {op} b\n    return x"
TEMPS = "def f(a, b):\n    return (a {op} b) {op} (b {op} a)"
TEMPLATE_CASES = [
    *[(BINARY.format(op=op), (6, 3), handler, "ABC",
       [(Plain(), a, b) for a, b in INT_PAIRS])
      for op, handler in (("+", "BINARY_OP_ADD_INT"), ("-", "BINARY_OP_SUBTRACT_INT"),
                          ("*", "BINARY_OP_MULTIPLY_INT"), ("&", "BINARY_OP_AND_INT"),
                          ("|", "BINARY_OP_OR_INT"), ("^", "BINARY_OP_XOR_INT"))],
    *[(TEMPS.format(op=op), (6, 3), handler, "BC", INT_PAIRS)
      for op, handler in (("+", "BINARY_OP_ADD_INT"), ("-", "BINARY_OP_SUBTRACT_INT"),
                          ("*", "BINARY_OP_MULTIPLY_INT"))],
    ("def f(a, b):\n    return (a * 2**30) * (b * 2**30)", (1, 1), "BINARY_OP_MULTIPLY_INT",
     "BC", [(2**30, 2**30), (2**25, -2**25), (1, 2**50 - 1), (-2**50, -1), (3, 5)]),
    *[(BINARY.format(op=op), (6.0, 3.0), handler, "ABC",
       [(Plain(), a, b) for a, b in FLOAT_PAIRS])
      for op, handler in (("+", "BINARY_OP_ADD_FLOAT"), ("-", "BINARY_OP_SUBTRACT_FLOAT"),
                          ("*", "BINARY_OP_MULTIPLY_FLOAT"),
                          ("/", "BINARY_OP_TRUEDIV_FLOAT"))],
    *[(f"def f(a, b):\n    x = 1\n    x = a {op} b\n    return x", warm, handler, "ABC",
       [(Plain(), a, b) for a, b in pairs])
      for op in COMPARE_OPS
      for warm, handler, pairs in (((1, 2), "COMPARE_OP_INT", INT_PAIRS),
                                   ((1.0, 2.0), "COMPARE_OP_FLOAT", FLOAT_PAIRS))],
    *[(f"def f(a, b):\n    if a {op} b:\n        return 1\n    return 0", warm, handler,
       "AB", pairs)
      for op in COMPARE_OPS
      for warm, handler, pairs in (((1, 2), "COMPARE_OP_JUMP_INT", INT_PAIRS),
                                   ((1.0, 2.0), "COMPARE_OP_JUMP_FLOAT", FLOAT_PAIRS))],
    *[(f"def f(a, b):\n    x = 1\n    x = a {op} b\n    return x", (None, None), "IS_OP",
       "ABC", [(Plain(), a, b) for a in ANY for b in ANY[::2]])
      for op in ("is", "is not")],
    *[(f"def f(a, b):\n    if a {op} b:\n        return 1\n    return 0", (None, None),
       "IS_OP_JUMP", "AB", [(a, b) for a in ANY for b in ANY[::2]])
      for op in ("is", "is not")],
    ("def f(a, b):\n    if a:\n        return 1\n    return 0", (True, 0),
     "POP_JUMP_IF_FALSE_BOOL", "A", [(x,) for x in ANY]),
    ("def f(a, b):\n    if not a:\n        return 1\n    return 0", (True, 0),
     "POP_JUMP_IF_TRUE_BOOL", "A", [(x,) for x in ANY]),
    ("def f(a, b):\n    if a is None:\n        return 1\n    return 0", (None, 0),
     "POP_JUMP_IF_NOT_NONE", "A", [(x,) for x in ANY]),
    ("def f(a, b):\n    if a is not None:\n        return 1\n    return 0", (None, 0),
     "POP_JUMP_IF_NONE", "A", [(x,) for x in ANY]),
    ("def f(a, b):\n    a = 5\n    return a", (1, 2), "LOAD_SMALL_INT_INLINE", "A",
     [(Plain(),), (None,), (big(5),), (3,)]),
    *[("def f(a, b):\n    x = 1\n    x = a[b]\n    return x", (seq_type([1, 2, 3]), 1),
       handler, "ABC", [(Plain(), seq, i) for seq in seqs for i in INDICES])
      for seq_type, handler, seqs in (
          (list, "BINARY_OP_SUBSCR_LIST_INT", SEQUENCES + [(1, 2), "ab"]),
          (tuple, "BINARY_OP_SUBSCR_TUPLE_INT", [tuple(s) for s in SEQUENCES] + [[1, 2]]))],
    # The list holds inline numbers: the template and the form store the
    # same value in turn, which releases only inline values
    ("def f(a, b):\n    a[b] = 5\n    return a", ([1, 2, 3], 1), "STORE_SUBSCR_LIST_INT", "ABC",
     [(seq, i, v) for seq in ([1, 2, 3], [], (1, 2)) for i in INDICES for v in (7, 2.5)]),
    ("def f(a, b):\n    x = 1\n    x = a\n    return x", (1, 2), ("LOAD_FAST", 0),
     "AB", [(old, v) for old in (Plain(), None, 3) for v in ANY]),
    ("def f(a, b):\n    return [a, b]", (1, 2), ("LOAD_FAST_BORROW", 0), "B",
     [(v,) for v in ANY]),
    ("def f(a, b):\n    a, b = b, a\n    return a", (1, 2), "STORE_FAST", "AB",
     [(v, old) for v in ANY for old in (Plain(), None, 3)]),
    ("def f(a, b):\n    return [x for x in a]", ([1], 2), "LOAD_FAST_AND_CLEAR", "B",
     [(), *[(v,) for v in ANY]]),
    *[("def f(a, b):\n    x = 'abc'\n    y = 2.5\n    z = 10**9\n    return x, y, z", (1, 2),
       handler, "A", [(Plain(),), (None,), (3,), ()])
      for handler in ("LOAD_CONST", "LOAD_CONST_INLINE_FLOAT", "LOAD_CONST_INLINE_INT")],
    *[(f"def f(a, b):\n    x = 1\n    x = a.{name}\n    return x", (cls(), 1), handler,
       "AB", [(old, o) for old in (Plain(), None) for o in OWNERS])
      for cls, name, handler in ((Shape, "q", "LOAD_ATTR_SHAPE"), (Slots, "q", "LOAD_ATTR_SLOT"),
                                 (Shape, "k", "LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES"),
                                 (Slots, "k", "LOAD_ATTR_NONDESCRIPTOR_NO_DICT"))],
    *[("def f(a, b):\n    return a.m()", (cls(), 1), handler, "B", [(o,) for o in OWNERS])
      for cls, handler in ((Shape, "LOAD_ATTR_METHOD_WITH_VALUES"),
                           (Slots, "LOAD_ATTR_METHOD_NO_DICT"))],
    # Inline values only, as for the list store
    *[("def f(a, b):\n    a.p = b\n    return a", (cls(), 1), handler, "AB",
       [(o, v) for o in OWNERS for v in (7, 2.5)])
      for cls, handler in ((Shape, "STORE_ATTR_SHAPE"), (Slots, "STORE_ATTR_SLOT"))],
    # Each run adds the attribute to a new instance
    ("def f(a, b):\n    a.z = b\n    return a", Fresh(lambda: (Shape(), 0)), "STORE_ATTR_SHAPE_ADD",
     "AB", [(Fresh(o), v) for o in (Shape, owners_with_z, reversed_shape, Slots, Other)
            for v in (7, 2.5, "s")] + [(None, 7), (4, 7)]),
    *[("G = 5\ndef f(a, b):\n    x = 1\n    x = G + len(a)\n    return x", ([1], 0), handler,
       "", [()]) for handler in ("LOAD_GLOBAL_MODULE", "LOAD_GLOBAL_BUILTIN")],
    ("def f(a, b):\n    while a:\n        a -= 1\n    return a", (3, 0), "JUMP_BACKWARD", "",
     [()]),
    # A site that has seen two shapes of one type reads the instance's values
    ("def f(a, b):\n    x = 1\n    x = a.q\n    return x",
     [(Shape(), 0), (reversed_shape(), 0)], "LOAD_ATTR_INSTANCE_VALUE",
     "AB", [(old, o) for old in (Plain(), None) for o in OWNERS + [reversed_shape()]]),
    *[("def f(a, b):\n    s = 0\n    for x in a:\n        s = x\n    return s", (warm, 0), handler,
       ("A", "B", "B+1"), [(old, it, index) for old in (Plain(), 3) for it, index in iters])
      for warm, handler, iters in (
          ([1, 2], "FOR_ITER_LIST",
           [(seq, tagged(i)) for seq in SEQUENCES + [(1, 2)] for i in (0, 1, 3)]),
          ((1, 2), "FOR_ITER_TUPLE",
           [(tuple(seq), tagged(i)) for seq in SEQUENCES for i in (0, 1, 3)] +
           [([1, 2], tagged(0))]),
          (range(3), "FOR_ITER_RANGE",
           [(Fresh(lambda r=r: iter(r)), tagged(0))
            for r in (range(3), range(0), range(2**60, 2**60 + 2), range(5, -5, -3))] +
           [([1], tagged(0))]))],
    *[(f"def f(a, b):\n    if {neg}a:\n        return 1\n    return 0", (warm, 0),
       f"POP_JUMP_IF_{sense}_{kind}", "A", [(x,) for x in TRUTHS])
      for neg, sense in (("", "FALSE"), ("not ", "TRUE"))
      for warm, kind in ((5, "INT"), ([], "LIST"), (None, "NONE"), (Shape(), "ALWAYS_TRUE"))],
]


def slot_of(field_slots, field):
    "The slot of a field, or of the slot n after its slot for 'F+n'"
    name, _, n = field.partition("+")
    return field_slots[name] + int(n or 0)


class TemplateTests(unittest.TestCase):
    def compile(self, source, warm):
        namespace = {}
        exec(source, namespace)
        f = namespace["f"]
        # warm is the arguments, or a list of them that the calls cycle
        # through; a Fresh makes them for each call
        warm = warm if isinstance(warm, list) else [warm]
        for i in range(200):
            args = warm[i % len(warm)]
            try:
                f(*(args.make() if isinstance(args, Fresh) else args))
            except Exception:
                pass
        return f

    def check_same(self, word, slots, objects):
        """The form and the template from the same slots, each on the word
        as compiled: word(fresh) gives the function and the word's index.
        A guard that fails counts towards the word's respecialisation, so
        after a slow path each side runs on a fresh function."""
        results = []
        fresh = False
        for use_template in (True, False):
            f, index = word(fresh)
            run_slots = {k: v.make() if isinstance(v, Fresh) else v for k, v in slots.items()}
            before = [sys.getrefcount(o) for o in objects]
            next_, slow, exc, out = capi.jit_template_run(f, index, run_slots, use_template)
            after = [sys.getrefcount(o) for o in objects]
            results.append((next_, slow, exc, out, [b - a for a, b in zip(before, after)]))
            fresh = slow
        if fresh:
            word(True)
        template, form = results
        self.assertEqual(template[0], form[0], "next word")
        self.assertEqual(type(template[2]), type(form[2]))
        self.assertEqual(str(template[2]), str(form[2]))
        self.assertEqual(len(template[3]), len(form[3]))
        for i, (x, y) in enumerate(zip(form[3], template[3])):
            self.assertEqual(type(x), type(y), f"slot {i}")
            self.assertTrue(same_slot(x, y), f"slot {i}: {x!r} != {y!r}")
        self.assertEqual(template[4], form[4], "counts")
        return template

    def test_templates_against_forms(self):
        for source, warm, handler, fields, cases in TEMPLATE_CASES:
            handler, nth = handler if isinstance(handler, tuple) else (handler, -1)
            current = None

            def word(fresh=False):
                nonlocal current
                if (fresh or current is None or
                        capi.get_ir(current[0].__code__, True)[current[1]][0] != handler):
                    f = self.compile(source, warm)
                    current = (f, word_of(f, handler, nth)[0])
                return current

            f, index = word()
            field_slots = dict(zip("ABCD", capi.get_ir(f.__code__, True)[index][1:]))
            fast = 0
            for case in cases:
                with self.subTest(source=source, handler=handler, case=case):
                    slots = {slot_of(field_slots, x): v for x, v in zip(fields, case)}
                    objects = [v for v in case if not isinstance(v, (int, float, tuple, Fresh)) or
                               type(v) is int and not -2**50 <= v < 2**50]
                    fast += not self.check_same(word, slots, objects)[1]
            with self.subTest(source=source, handler=handler):
                self.assertGreater(fast, 0, "no case stayed on the template's code")

    def test_temporaries(self):
        "TEMPS puts the inputs of its last word in temporaries"
        f = self.compile(TEMPS.format(op="+"), (6, 3))
        index, (_, a, b, c, d) = word_of(f, "BINARY_OP_ADD_INT")
        self.assertGreaterEqual(min(b, c), nfixed(f))

    def test_fast_path(self):
        "Inputs that the guards accept stay on the template's own code"
        f = self.compile("def f(a, b):\n    return a + b", (3, 4))
        index, (_, a, b, c, d) = word_of(f, "BINARY_OP_ADD_INT")
        self.assertFalse(capi.jit_template_run(f, index, {b: 1, c: 2}, True)[1])
        self.assertFalse(capi.jit_template_run(f, index, {b: 2**49, c: 2**49}, True)[1])
        self.assertTrue(capi.jit_template_run(f, index, {b: "x", c: 2}, True)[1])
        self.assertTrue(capi.jit_template_run(f, index, {b: 1, c: "y"}, True)[1])


if __name__ == "__main__":
    unittest.main()
