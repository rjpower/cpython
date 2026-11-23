"""The baseline JIT's inline templates: the primitive lowerings
(Python/jit_lower.c) against their C definitions."""

import ctypes
import random
import struct
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
SLOTS = {"_PyObject_ShapeReadDup"}
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


def test_more_objects_function():
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
                                              len, test_more_objects_function, str, int]]
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


if __name__ == "__main__":
    unittest.main()
