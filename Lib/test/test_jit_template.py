"""The baseline JIT's inline templates: the primitive lowerings
(Python/jit_lower.c) against their C definitions, and each template
(Python/jit_templates.c.h) against its handler's function form on one word
of a frame."""

import contextlib
import ctypes
import dataclasses
import math
import random
import struct
import sys
import unittest
import weakref

from test import test_tools
from test import support
from test.support import import_helper
from test.support.script_helper import assert_python_ok

capi = import_helper.import_module("_testinternalcapi")
if not hasattr(capi, "jit_lower"):
    raise unittest.SkipTest("the JIT is not supported on this platform")
test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import ir_contract
    import ir_leaf
    import jit_template_generator

RECORDS = jit_template_generator.load_records()[1]

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
FRAME = {"GLOBALS", "BUILTINS", "FRAME_CO_CONSTS", "GETLOCAL", "IR_SLOTS",
         "FRAME_CODE_INSTRUMENTATION_VERSION"}
SLOTS = {"_PyObject_ShapeReadDup", "_PyDictValues_SetShape",
         "_PyDictValues_AddToInsertionOrder", "_PyValueSlot_Exchange",
         "_PyValueSlot_ExchangeFilled", "_PyValueSlot_Fill"}
# Tested on their own: they leave by a label of their own, with results on both paths
LEAVES = {"_PyValue_AddCheckedScaled"}
APART = COUNTS | FRAME | SLOTS | LEAVES
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
                if predicate and jit_template_generator.LOWERINGS[name].exits:
                    # With an exit, the lowering leaves by it where the predicate is false
                    holds = expected if not nres else expected[0]
                    cond, results, before, after = run(name, ops, args, rng, "exit")
                    self.check_registers(name, ops, before, after)
                    self.assertIs(cond, not holds)
                    if holds and nres:
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

    def test_add_checked_scaled(self):
        """The lowering leaves by its label exactly where the C definition
        overflows, and writes the C definition's out on both paths"""
        name = "_PyValue_AddCheckedScaled"
        nres, nargs, ntemps, nxtemps, kinds, predicate = lowerings()[name]
        self.assertEqual((nres, nargs, ntemps, kinds), (1, 2, 2, "ggggg"))
        rng = random.Random(90)
        for args in cases(ir_leaf.PRIMITIVES[name]):
            overflowed, out = capi.leaf_primitive(name, *args)
            assignments_ = assignments(kinds, nres, nargs, rng, count=2)
            # Both arguments in one register
            same_arg = list(assignments_[0])
            same_arg[2] = same_arg[1]
            for ops in (*assignments_, same_arg):
                same_args = ops[1] == ops[2]
                with self.subTest(args=args, ops=ops):
                    if same_args and args[0] != args[1]:
                        continue
                    cond, (got,), before, after = run(name, ops, args, rng, "exit")
                    self.check_registers(name, ops, before, after)
                    self.assertIs(cond, overflowed)
                    self.assertEqual(as_type(got, "i64"), out)

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
            "_PyTuple_ITEMS": [(id(o),) for o in tuples],
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
                    # With an exit for a NULL result, which these never give:
                    # the lowering never leaves by it
                    for aux in (None, "exit"):
                        with self.subTest(primitive=name, value=value, ops=ops, aux=aux):
                            cond, (w,), before, after = run(name, ops, (value,), rng, aux,
                                                            args_live=live)
                            self.check_registers(name, ops, before, after, live)
                            self.assertIs(cond, None if aux is None else False)
                            if w >> 48:
                                self.assertEqual(w, expected)
                            else:
                                # A box, as C made: its value
                                self.assertTrue(same(self.take(w), value))
                                self.assertTrue(same(expected, value))

    def test_failed_box_leaves_by_the_exit(self):
        "A box that fails gives NULL, and the lowering with an exit leaves by it"
        rng = random.Random(91)
        for name, failing, inline in (("PyValue_FromInt64", (2**60, -2**60, 2**50, -2**50 - 1), 7),
                                      ("PyValue_FromDouble", (NAN, -NAN), 1.5)):
            # A boxed float comes from a freelist unless it is empty: hold
            # more NaNs than it keeps, so that the box allocates
            held = [NAN * x for x in (2.0,) * 300]
            for value in failing:
                for ops, live in variants(name, rng, 2):
                    with self.subTest(primitive=name, value=value, ops=ops):
                        cond, (w,), before, after = run(name, ops, (value,), rng, "exit_fail",
                                                        args_live=live)
                        self.check_registers(name, ops, before, after, live)
                        self.assertIs(cond, True)
                        self.assertEqual(w, 0)
            # Inline values need no box
            for ops, live in variants(name, rng, 2):
                cond, (w,), before, after = run(name, ops, (inline,), rng, "exit_fail",
                                                args_live=live)
                self.assertIs(cond, False)
                self.assertEqual(w, capi.leaf_primitive(name, inline))
            del held

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
                             ("IR_SLOTS", [()]), ("FRAME_CODE_INSTRUMENTATION_VERSION", [()]),
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

    def test_value_slot_stores(self):
        "The slot's word moves out and the new one in, as C's do, in every register assignment"
        rng = random.Random(96)
        slot = (ctypes.c_uint64 * 1)()
        address = ctypes.addressof(slot)
        held_words = [0, 1 << 40 | 3, 0xFFFF_FFFF_FFFF_FFF0]
        new = 0x1234_5678_9ABC_DEF0
        for name, helds in (("_PyValueSlot_Exchange", held_words),
                            ("_PyValueSlot_ExchangeFilled", held_words[1:]),
                            ("_PyValueSlot_Fill", held_words[:1])):
            nres, nargs, _, _, kinds, _ = lowerings()[name]
            for held in helds:
                for ops in assignments(kinds, nres, nargs, rng):
                    with self.subTest(primitive=name, held=held, ops=ops):
                        slot[0] = held
                        _, results, before, after = run(name, ops, (address, new), rng)
                        self.check_registers(name, ops, before, after)
                        self.assertEqual(slot[0], new)
                        if nres:
                            self.assertEqual(results[0], held)


# Under a JIT mode the lowering picks these forms of RESUME and
# JUMP_BACKWARD; a unit compiles them as their base handler (Python/jit.c)
JIT_FORMS = {"RESUME_JIT": "RESUME", "JUMP_BACKWARD_JIT": "JUMP_BACKWARD"}


def word_of(func, handler, nth=-1):
    "The index and fields of func's nth word that compiles as handler"
    words = capi.get_ir(func.__code__, True)
    found = [(i, w) for i, w in enumerate(words)
             if JIT_FORMS.get(w[0], w[0]) == handler]
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
# Operands that fail only the second of two fused guards, or the first, and
# sums, differences and products past the inline range on both sides
INT_PAIRS = ([(a, b) for a in INTS[::2] for b in INTS[1::2]] +
             [(a, b) for a in INTS[:3] for b in NOT_INTS[:2]] +
             [(b, a) for a in INTS[:3] for b in NOT_INTS] +
             [(2**50 - 1, 1), (-2**50, -1), (2**50 - 1, 2**50 - 1), (-2**50, -2**50),
              (2**50 - 1, -2**50), (1, 2**50 - 1), (-1, -2**50)])
# Signs, a zero divisor, the inline extremes and -2**50 // -1, whose quotient
# leaves the inline range
DIV_INTS = [0, 1, -1, 7, -7, 2**50 - 1, -2**50, 2**25, -2**25 - 3]
DIV_PAIRS = ([(a, b) for a in DIV_INTS for b in DIV_INTS] +
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
    # RESUME's words compile to RESUME_CHECK's template, whose miss runs
    # the RESUME form; a frame with cells reaches its RESUME after the
    # words that make them
    ("def f(a, b):\n    return a", (1, 2), "RESUME", "", [()]),
    ("def f(a, b):\n    def g():\n        return a\n    return g", (1, 2), "RESUME", "",
     [()]),
    *[(BINARY.format(op=op), (6, 3), handler, "ABC",
       [(Plain(), a, b) for a, b in INT_PAIRS])
      for op, handler in (("+", "Add.Int"), ("-", "BINARY_OP_SUBTRACT_INT"),
                          ("*", "BINARY_OP_MULTIPLY_INT"), ("&", "BINARY_OP_AND_INT"),
                          ("|", "BINARY_OP_OR_INT"), ("^", "BINARY_OP_XOR_INT"))],
    *[(src.format(op=op), (6, 3), handler, fields, [(Plain(), a, b) for a, b in DIV_PAIRS]
       if fields == "ABC" else DIV_PAIRS)
      for op, handler in (("//", "BINARY_OP_FLOOR_DIVIDE_INT"), ("%", "BINARY_OP_REMAINDER_INT"))
      for src, fields in ((BINARY, "ABC"), (TEMPS, "BC"))],
    # Counts below zero, inside the word, and past it
    (BINARY.format(op=">>"), (6, 3), "BINARY_OP_RSHIFT_INT", "ABC",
     [(Plain(), a, b) for a in INTS for b in (0, 1, 5, 62, 63, 64, 1000, -1)] +
     [(Plain(), a, b) for a, b in INT_PAIRS[-8:]]),
    (TEMPS.format(op=">>"), (6, 3), "BINARY_OP_RSHIFT_INT", "BC",
     [(a, b) for a in INTS for b in (0, 5, 63, 64, -1)]),
    *[(TEMPS.format(op=op), (6, 3), handler, "BC", INT_PAIRS)
      for op, handler in (("+", "Add.Int"), ("-", "BINARY_OP_SUBTRACT_INT"),
                          ("*", "BINARY_OP_MULTIPLY_INT"))],
    ("def f(a, b):\n    return (a * 2**30) * (b * 2**30)", (1, 1), "BINARY_OP_MULTIPLY_INT",
     "BC", [(2**30, 2**30), (2**25, -2**25), (1, 2**50 - 1), (-2**50, -1), (3, 5)]),
    *[(BINARY.format(op=op), (6.0, 3.0), handler, "ABC",
       [(Plain(), a, b) for a, b in FLOAT_PAIRS])
      # Add.Float is a shared arm: its stub is tested by test_rop_add
      for op, handler in (("-", "BINARY_OP_SUBTRACT_FLOAT"),
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
    # (The truth and branch arms are tested by test_rop_truth, by their form
    # and completion stub: they have no template yet.)
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
    # Inline values only, as for the list store
    *[("def f(a, b):\n    a.p = b\n    return a", (cls(), 1), handler, "AB",
       [(o, v) for o in OWNERS for v in (7, 2.5)])
      for cls, handler in ((Shape, "STORE_ATTR_SHAPE"), (Slots, "STORE_ATTR_SLOT"))],
    ("def f(a, b):\n    x = 1\n    x = a < b < 3\n    return x", (1, 2), "POP_TOP", "A",
     [(x,) for x in ANY + [Plain()]]),
    # Each run adds the attribute to a new instance
    ("def f(a, b):\n    a.z = b\n    return a", Fresh(lambda: (Shape(), 0)), "STORE_ATTR_SHAPE_ADD",
     "AB", [(Fresh(o), v) for o in (Shape, owners_with_z, reversed_shape, Slots, Other)
            for v in (7, 2.5, "s")] + [(None, 7), (4, 7)]),
    *[("G = 5\ndef f(a, b):\n    x = 1\n    x = G + len(a)\n    return x", ([1], 0), handler,
       "", [()]) for handler in ("LOAD_GLOBAL_MODULE", "LOAD_GLOBAL_BUILTIN")],
    # (Jump.Poll has no template yet: its poll row has no lowering.)
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
]


def slot_of(field_slots, field):
    "The slot of a field, or of the slot n after its slot for 'F+n'"
    name, _, n = field.partition("+")
    return field_slots[name] + int(n or 0)


class FailedBoxTests(unittest.TestCase):
    """The failure edge of a box that leaves by the exit (SLOW rows): in
    compiled code, a failed allocation raises MemoryError at the word, with
    the inputs still in their homes, and the function works after it"""

    @support.nomemtest
    def test_box_failure_raises_at_the_word_and_leaves_the_inputs(self):
        assert_python_ok("-c", """if 1:
            import gc
            import _testcapi
            import _testinternalcapi as capi

            inf = float("inf")
            nan = float("nan")
            cases = (
                # PyValue_FromInt64 in the floor division of two ints
                ("BINARY_OP_FLOOR_DIVIDE_INT", "a // b", (-2**50, -1), 2**50, (6, 3), 2),
                # PyValue_FromDouble of a NaN
                ("BINARY_OP_SUBTRACT_FLOAT", "a - b", (inf, inf), nan, (3.5, 2.5), 1.0),
            )
            for handler, expression, args, boxed, safe, safe_result in cases:
                namespace = {}
                exec(f"def f(a, b):\\n    return {expression}\\n"
                     "def g(a, b):\\n    try:\\n        x = f(a, b)\\n"
                     "    except MemoryError as e:\\n"
                     "        return ('failed', a, b, e.__traceback__.tb_next.tb_frame.f_code)\\n"
                     "    return x\\n",
                     namespace)
                f, g = namespace["f"], namespace["g"]
                for _ in range(300):
                    g(*safe)
                    g(*args)
                words = [row[0] for row in capi.get_ir(f.__code__, True)]
                assert handler in words, words
                a, b = args
                failed = 0
                for skip in range(4):
                    gc.collect()
                    gc.disable()
                    # A boxed float comes from a freelist unless it is empty:
                    # hold more NaNs than it keeps, so that the box allocates
                    held = [nan * x for x in (2.0,) * 300]
                    # skip allocations succeed, then exactly one fails
                    _testcapi.set_nomemory(skip, skip + 1)
                    try:
                        got = g(a, b)
                    finally:
                        _testcapi.remove_mem_hooks()
                        gc.enable()
                    del held
                    if isinstance(got, tuple) and got[:1] == ("failed",):
                        failed += 1
                        # Raised in f, at its word, with the inputs in their homes
                        assert got[1] is a and got[2] is b and got[3] is f.__code__, got
                    else:
                        assert got == boxed or got != got, got
                    # The function works after the failure
                    again = g(a, b)
                    assert again == boxed or again != again, again
                    assert g(*safe) == safe_result
                assert failed, (handler, "no allocation failure reached the box")
            """, PYTHON_JIT="always")


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

    def test_move_is_a_shared_stub(self):
        "A Move word calls its function form through the shared stub"
        # The resolver renames most moves away; a with statement still
        # stages its exit and value with a Move of one pair
        f = self.compile("def f(a, b):\n    with a:\n        return b",
                         (contextlib.nullcontext(), 2))
        index = word_of(f, "Move.Transfer")[0]
        self.assertTrue(capi.jit_compile(f.__code__))
        self.assertEqual(capi.jit_unit(f.__code__)["plans"][index],
                         ("Move.Transfer", "shared", "stub", "Move.Transfer"))
        self.assertEqual(f(contextlib.nullcontext(), 2), 2)

    def test_resume_check_misses_when_instrumentation_changes(self):
        "RESUME_CHECK's template stays on its code until monitoring changes the version"
        f = self.compile("def f(a, b):\n    return a", (1, 2))
        index = word_of(f, "RESUME")[0]
        self.assertFalse(capi.jit_template_run(f, index, {}, True)[1])
        sys.monitoring.use_tool_id(4, "test_jit_template")
        try:
            sys.monitoring.set_events(4, sys.monitoring.events.PY_START)
            # The code's version is behind the interpreter's: the miss runs
            # the RESUME form, which instruments the code
            next_, slow, exc, _ = capi.jit_template_run(f, index, {}, True)
            self.assertTrue(slow)
            self.assertIsNone(exc)
        finally:
            sys.monitoring.set_events(4, 0)
            sys.monitoring.free_tool_id(4)

    def test_temporaries(self):
        "TEMPS puts the inputs of its last word in temporaries"
        f = self.compile(TEMPS.format(op="+"), (6, 3))
        index, (_, a, b, c, d) = word_of(f, "Add.Int")
        self.assertGreaterEqual(min(b, c), nfixed(f))

    def test_fast_path(self):
        "Inputs that the guards accept stay on the template's own code"
        f = self.compile("def f(a, b):\n    return a + b", (3, 4))
        index, (_, a, b, c, d) = word_of(f, "Add.Int")
        self.assertFalse(capi.jit_template_run(f, index, {b: 1, c: 2}, True)[1])
        self.assertFalse(capi.jit_template_run(f, index, {b: 2**49, c: 2**49}, True)[1])
        self.assertTrue(capi.jit_template_run(f, index, {b: "x", c: 2}, True)[1])
        self.assertTrue(capi.jit_template_run(f, index, {b: 1, c: "y"}, True)[1])

    def test_division_fast_path(self):
        "Division and remainder by a nonzero inline int stay on the template's code"
        for op, handler in (("//", "BINARY_OP_FLOOR_DIVIDE_INT"),
                            ("%", "BINARY_OP_REMAINDER_INT")):
            f = self.compile(f"def f(a, b):\n    return a {op} b", (7, 2))
            index, (_, a, b, c, d) = word_of(f, handler)
            for x, y in ((7, 2), (-7, 2), (7, -2), (-2**50, -1)):
                self.assertFalse(capi.jit_template_run(f, index, {b: x, c: y}, True)[1])
            self.assertTrue(capi.jit_template_run(f, index, {b: 7, c: 0}, True)[1])


def graph_of(handler):
    "The template graph of a handler, simplified as the generator emits it"
    record = RECORDS[handler]
    graph = jit_template_generator.Builder(record).build()
    jit_template_generator.simplify(graph)
    return graph


def prims_of(graph):
    return [jit_template_generator.prim_name(op) for block in graph.blocks.values()
            for op in block.ops if op.kind == "prim"]


@contextlib.contextmanager
def declared(name, **changes):
    """The component row `name` with columns changed, for the generator that
    reads ir_contract.rows(): a declaration withdrawn or altered"""
    table = ir_contract.rows()
    saved = table[name]
    if "result" in changes:
        changes["result"] = dataclasses.replace(saved.result, **changes["result"])
    table[name] = dataclasses.replace(saved, **changes)
    try:
        yield
    finally:
        table[name] = saved


def prim_graph(*steps):
    """A graph for the passes: one word v, then each step (name, test) as a
    block that tests v (`test` is the primitive, whose false side leaves by
    a deopt) or, with test None, ends in the primitive on v.  Returns the
    graph and its ops by step"""
    g = jit_template_generator.Graph()
    TOp = jit_template_generator.TOp
    block = g.blocks[g.entry]
    v = g.value(ir_leaf.VALUE)
    block.ops.append(TOp("view", [v], [], "B"))
    deopt = g.new_block()
    deopt.term = ("deopt",)
    ops = []
    for name, test in steps:
        if test:
            c = g.value("bool")
            op = TOp("prim", [c], [v], name)
            block.ops.append(op)
            nxt = g.new_block()
            block.term = ("br", c, nxt.id, deopt.id) if test == "continue" else \
                ("br", c, deopt.id, nxt.id)
            block = nxt
        else:
            op = TOp("prim", [], [v], name)
            block.ops.append(op)
        ops.append(op)
    block.term = ("end", None)
    return g, ops


class TemplateGraphTests(unittest.TestCase):
    "What the generator reads from the component rows, and the graphs it builds with it"

    def test_tests_for_null(self):
        found = {name for name in ir_contract.rows() if jit_template_generator.is_null_test(name)}
        self.assertEqual(found, {"PyValue_IsNull"})

    def test_tests_of_both(self):
        self.assertEqual(jit_template_generator.both_test("PyValue_IsInlineInt"),
                         "PyValue_BothInlineInt")
        found = {name for name in ir_contract.rows()
                 if jit_template_generator.both_test(name) is not None}
        self.assertEqual(found, {"PyValue_IsInlineInt"})
        with declared("PyValue_BothInlineInt", conjoins=""):
            self.assertIsNone(jit_template_generator.both_test("PyValue_IsInlineInt"))

    def test_what_a_test_shows(self):
        facts = jit_template_generator.test_facts
        self.assertEqual(facts("PyValue_IsNull"), (False, True))
        self.assertEqual(facts("PyValue_IsObject"), (True, False))
        self.assertEqual(facts("PyValue_IsInlineInt"), (True, False))
        # No fact is declared of this: it shows nothing, though a word
        # outside pointer space is not NULL
        self.assertEqual(facts("PyValue_IsPointerSpace"), (False, False))
        # A tagged index is an encoding of the representation table
        self.assertEqual(facts("PyValue_IsTaggedIndex"), (True, False))
        # A primitive without a row shows nothing
        self.assertEqual(facts("no_such_primitive"), (False, False))

    def test_results_that_are_never_null(self):
        found = {name for name in ir_contract.rows() if jit_template_generator.never_null(name)}
        scalar = {name for name in found if name in ir_leaf.PRIMITIVES
                  and all(p not in (ir_leaf.VALUE, "PyObject*")
                          for p in ir_leaf.PRIMITIVES[name].params)}
        self.assertLessEqual({"_PyValue_FromIntScaled", "PyValue_FromInlineInt",
                              "PyValue_TagIndex", "PyValue_DUP"}, found)
        # The declaration against the C definition on its domain
        for name in scalar:
            prim = ir_leaf.PRIMITIVES[name]
            for args in cases(prim):
                if prim.pre is None or prim.pre(context(), *args):
                    self.assertNotEqual(capi.leaf_primitive(name, *args), 0, (name, args))
        with declared("_PyValue_FromIntScaled", result={"shape": "VALUE_OR_NULL"}):
            self.assertFalse(jit_template_generator.never_null("_PyValue_FromIntScaled"))

    def test_primitives_for_a_word_that_is_not_null(self):
        given = {name: jit_template_generator.given_not_null(name) for name in ir_contract.rows()}
        self.assertEqual({k: v for k, v in given.items() if v},
                         {"PyValue_XDUP": "PyValue_DUP"})

    def test_results_that_fail_only_in_their_slow_path(self):
        found = {name for name in ir_contract.rows()
                 if jit_template_generator.null_only_from_slow(name)}
        self.assertEqual(found, {"PyValue_FromInt64", "PyValue_FromDouble"})

    def graph_names(self, handler):
        graph = graph_of(handler)
        return graph, prims_of(graph)

    def test_without_conjoins_the_guards_stay_two(self):
        with declared("PyValue_BothInlineInt", conjoins=""):
            graph, names = self.graph_names("BINARY_OP_ADD_INT")
        self.assertEqual(names.count("PyValue_IsInlineInt"), 2)
        self.assertNotIn("PyValue_BothInlineInt", names)

    def test_without_a_nonnull_form_the_test_for_null_stays_in_the_primitive(self):
        with declared("PyValue_DUP", nonnull_form=""):
            graph, names = self.graph_names("LOAD_GLOBAL_MODULE")
        self.assertIn("PyValue_XDUP", names)
        self.assertNotIn("PyValue_DUP", names)
        # The form is chosen where the word is known not NULL
        for declare in (False, True):
            g, (test, dup) = prim_graph(("PyValue_IsNull", "leave"), ("PyValue_XDUP", None))
            with contextlib.ExitStack() as stack:
                if declare:
                    stack.enter_context(declared("PyValue_DUP", nonnull_form=""))
                jit_template_generator.fold_nulls(g)
            self.assertEqual(dup.attr, "PyValue_XDUP" if declare else "PyValue_DUP")

    def test_without_slow_the_box_keeps_its_test_and_has_no_exit(self):
        for handler, name in (("FOR_ITER_RANGE", "PyValue_FromInt64"),
                              ("BINARY_OP_ADD_FLOAT", "PyValue_FromDouble")):
            with self.subTest(handler=handler):
                def box():
                    graph = graph_of(handler)
                    return [op for b in graph.blocks.values() for op in b.ops
                            if jit_template_generator.prim_name(op) == name], graph
                (op,), graph = box()
                self.assertIsNotNone(op.exit)
                with declared(name, slow=""):
                    (op,), graph = box()
                self.assertIsNone(op.exit)
                self.assertIn("PyValue_IsNull", prims_of(graph))

    def test_without_the_declared_test_for_null_nothing_is_dropped(self):
        # After a test of IsNull that leaves where it holds, a second one is dropped
        g, (first, second) = prim_graph(("PyValue_IsNull", "leave"), ("PyValue_IsNull", "leave"))
        jit_template_generator.fold_nulls(g)
        self.assertEqual(second.kind, "const")
        with declared("PyValue_IsNull", result={"tests": ((frozenset(), frozenset()),)}):
            g, (first, second) = prim_graph(("PyValue_IsNull", "leave"),
                                            ("PyValue_IsNull", "leave"))
            jit_template_generator.fold_nulls(g)
            self.assertEqual(second.kind, "prim")

    def test_a_declared_true_side_shows_the_word_not_null(self):
        g, (first, second) = prim_graph(("PyValue_IsInlineInt", "continue"),
                                        ("PyValue_IsNull", "leave"))
        jit_template_generator.fold_nulls(g)
        self.assertEqual(second.kind, "const")
        with declared("PyValue_IsInlineInt", result={"tests": ((frozenset(), frozenset()),)}):
            g, (first, second) = prim_graph(("PyValue_IsInlineInt", "continue"),
                                            ("PyValue_IsNull", "leave"))
            jit_template_generator.fold_nulls(g)
            self.assertEqual(second.kind, "prim")

    def test_add_int_has_one_guard_and_tests_the_box_only(self):
        graph = graph_of("BINARY_OP_ADD_INT")
        names = prims_of(graph)
        self.assertEqual(names.count("PyValue_BothInlineInt"), 1)
        self.assertNotIn("PyValue_IsInlineInt", names)
        # The overflow edge keeps its test: the box can be NULL, the inline int cannot
        self.assertEqual(names.count("PyValue_IsNull"), 1)
        (block,) = [b for b in graph.blocks.values()
                    if any(jit_template_generator.prim_name(op) == "PyValue_IsNull"
                           for op in b.ops)]
        self.assertTrue(any(op.kind == "call" for op in block.ops))

    def test_guards_leave_by_their_exit_without_a_bool(self):
        "A predicate that only guards leaves by the exit: no result, no branch on one"
        for handler, name in (("BINARY_OP_ADD_FLOAT", "PyValue_FloatOperandValues"),
                              ("BINARY_OP_SUBSCR_LIST_INT", "PyValue_IsObject"),
                              ("POP_JUMP_IF_FALSE_BOOL", "PyValue_BoolCheck")):
            with self.subTest(handler=handler):
                graph = graph_of(handler)
                ops = [op for block in graph.blocks.values() for op in block.ops
                       if op.kind == "prim" and jit_template_generator.prim_name(op) == name]
                self.assertEqual(len(ops), 1)
                self.assertIsNotNone(ops[0].exit)
                self.assertEqual(len(ops[0].results),
                                 len(ir_leaf.PRIMITIVES[name].results) - 1)

    def cse_loads(self, middle):
        """A graph that loads a field of one object, runs the ops in middle
        (TOp makers given the object and a value) and loads it again; the
        number of loads of the field after CSE"""
        g = jit_template_generator.Graph()
        entry = g.blocks[g.entry]
        obj = g.value(ir_leaf.VALUE)
        entry.ops.append(jit_template_generator.TOp("view", [obj], [], "B"))
        first = g.value("i64")
        entry.ops.append(jit_template_generator.TOp("load", [first], [obj], "_PyRangeIterObject.len"))
        for make in middle:
            entry.ops.append(make(g, obj, first))
        second = g.value("i64")
        entry.ops.append(jit_template_generator.TOp("load", [second], [obj], "_PyRangeIterObject.len"))
        entry.ops.append(jit_template_generator.TOp("store", [], [obj, second], "_PyRangeIterObject.len"))
        entry.term = ("end", None)
        jit_template_generator.cse(g)
        return sum(op.kind == "load" for op in entry.ops)

    def test_a_read_is_reused_until_a_write_may_change_it(self):
        TOp = jit_template_generator.TOp
        store = lambda field: lambda g, o, v: TOp("store", [], [o, v], field)
        self.assertEqual(self.cse_loads([]), 1)
        # Another field, or a count that is not this field's, leaves it
        self.assertEqual(self.cse_loads([store("_PyRangeIterObject.start")]), 1)
        self.assertEqual(self.cse_loads([lambda g, o, v: TOp("take", [], [o], None)]), 1)
        # Its own field, a store through a type, a release and a helper that
        # runs Python change it
        self.assertEqual(self.cse_loads([store("_PyRangeIterObject.len")]), 2)
        self.assertEqual(self.cse_loads([store("i64")]), 2)
        self.assertEqual(self.cse_loads([lambda g, o, v: TOp("prim", [], [o], "PyValue_CLOSE")]), 2)
        self.assertEqual(self.cse_loads([lambda g, o, v: TOp("call", [], [], "_Py_HandlePending")]), 2)
        # Boxing allocates new objects and changes none
        self.assertEqual(self.cse_loads([lambda g, o, v: TOp("call", [g.value(ir_leaf.VALUE)],
                                                             [v], "_PyValue_BoxInt64")]), 1)

    def cse_home_loads(self, middle):
        """A graph that loads a frame home (GETLOCAL), runs the ops in middle
        (TOp makers given the loaded value) and loads it again; the number
        of home loads after CSE"""
        TOp = jit_template_generator.TOp
        g = jit_template_generator.Graph()
        entry = g.blocks[g.entry]
        index = g.value("i32")
        entry.ops.append(TOp("const", [index], [], 3))
        first = g.value(ir_leaf.VALUE)
        entry.ops.append(TOp("prim", [first], [index], "GETLOCAL"))
        for make in middle:
            entry.ops.append(make(g, first))
        second = g.value(ir_leaf.VALUE)
        entry.ops.append(TOp("prim", [second], [index], "GETLOCAL"))
        entry.term = ("end", second)
        jit_template_generator.cse(g)
        return sum(op.kind == "prim" and op.attr == "GETLOCAL" for op in entry.ops)

    def test_home_loads_are_their_own_location(self):
        # A home is not the heap (HOME): a heap store leaves a home load;
        # a home write, a release and Python change it
        TOp = jit_template_generator.TOp
        self.assertTrue(jit_template_generator.home_load("GETLOCAL"))
        self.assertFalse(jit_template_generator.home_load("IR_SLOTS"))
        self.assertEqual(self.cse_home_loads([]), 1)
        self.assertEqual(self.cse_home_loads(
            [lambda g, v: TOp("store", [], [v, v], "_PyRangeIterObject.len")]), 1)
        self.assertEqual(self.cse_home_loads([lambda g, v: TOp("slot_store", [], [v], 3)]), 2)
        self.assertEqual(self.cse_home_loads([lambda g, v: TOp("detach", [], [], 3)]), 2)
        self.assertEqual(self.cse_home_loads(
            [lambda g, v: TOp("prim", [], [v], "PyValue_CLOSE")]), 2)

    def test_swap_stores_both_homes(self):
        # SWAP's outputs are exchanged, not passed through: its template
        # loads both homes and stores each value into the other
        graph = graph_of("SWAP")
        ops = [op for block in graph.blocks.values() for op in block.ops]
        loads = [op for op in ops if op.kind == "view"]
        stores = [op for op in ops if op.kind == "slot_store"]
        self.assertEqual(len(stores), 2)
        self.assertNotEqual(stores[0].attr, stores[1].attr)
        self.assertEqual({op.attr for op in stores}, {op.attr for op in loads})
        for store in stores:
            load = next(op for op in loads if op.results[0] is store.args[0])
            self.assertNotEqual(load.attr, store.attr)

    def test_range_iterator_reads_its_length_once(self):
        graph = graph_of("FOR_ITER_RANGE")
        loads = [op.attr for b in graph.blocks.values() for op in b.ops if op.kind == "load"]
        self.assertEqual(loads.count("_PyRangeIterObject.len"), 1)

    def test_no_template_repeats_what_the_passes_remove(self):
        for handler, record in RECORDS.items():
            if record.form != "EMITTED" or not record.uops or any(u.leaf is None for u in record.uops):
                continue
            try:
                graph = graph_of(handler)
            except jit_template_generator.Refused:
                continue
            with self.subTest(handler=handler):
                self.assertFalse(jit_template_generator.fold_nulls(graph))
                self.assertFalse(jit_template_generator.fuse_guards(graph))
                self.assertFalse(jit_template_generator.thread_tests(graph))
                self.assertFalse(jit_template_generator.thread_branches(graph))

    def test_an_assumption_is_checked_only_with_assertions(self):
        # F7: an `assume` op becomes a check that traps, under #ifndef NDEBUG;
        # without assertions the template is the one with no assumption
        handler = "BINARY_OP_ADD_FLOAT"
        record = RECORDS[handler]
        plain = jit_template_generator.template_of(handler, record)
        uops = list(record.uops)
        i, u = next((i, u) for i, u in enumerate(uops) if u.leaf is not None)
        leaf = dataclasses.replace(u.leaf, blocks=list(u.leaf.blocks))
        entry = leaf.blocks[0]
        arg = next(op for op in entry.ops if op.kind == "arg")
        ops = list(entry.ops)
        ops.insert(ops.index(arg) + 1,
                   ir_leaf.Op("assume", (), arg.results, ("exact", "PyUnicode_Type")))
        leaf.blocks[0] = dataclasses.replace(entry, ops=ops)
        uops[i] = dataclasses.replace(u, leaf=leaf)
        planted = jit_template_generator.template_of(
            handler, dataclasses.replace(record, uops=uops))
        start, end = planted.index("#ifndef NDEBUG"), planted.index("#endif")
        check = "\n".join(planted[start:end])
        self.assertIn("&PyUnicode_Type", check)
        self.assertIn("0x0f, 0x0b", check)  # ud2
        self.assertEqual(planted[:start] + planted[end + 1:], plain)


if __name__ == "__main__":
    unittest.main()
