"""The baseline JIT's inline templates: the primitive lowerings
(Python/jit_lower.c) against their C definitions, and each template
(Python/jit_templates.c.h) against its handler's function form on one word
of a frame."""

import ctypes
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
    cases, context, refcount, same, word,
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
    """Register assignments of a lowering's operands: distinct registers, and
    unless not alias, each result in the register of an argument of its
    kind."""
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
                break
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


def run(name, ops, args, rng, aux=None, args_live=False):
    """Run a lowering on args in the registers ops, with the arguments live
    after it if args_live.  Returns the predicate's value, the results and
    the registers before and after."""
    nres, nargs, *_, kinds, _ = lowerings()[name]
    gprs, xmms = canaries(rng)
    for i, arg in enumerate(args):
        reg = ops[nres + i]
        if kinds[nres + i] == "x":
            xmms[reg] = double_bits(arg)
        else:
            gprs[reg] = arg & MASK
    cond, gprs, gout, xout = capi.jit_lower(name, tuple(ops), tuple(gprs), tuple(xmms),
                                            aux, args_live)
    results = [bits_double(xout[ops[i]]) if kinds[i] == "x" else gout[ops[i]]
               for i in range(nres)]
    return cond, results, (gprs, xmms), (gout, xout)


def as_type(value, type_):
    if type_ == "i64" and value >= 2**63:
        return value - 2**64
    return value


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

    def test_against_c(self):
        rng = random.Random(88)
        for name, (nres, nargs, ntemps, nxtemps, kinds, predicate) in lowerings().items():
            if name in COUNTS:
                continue
            prim = ir_leaf.PRIMITIVES[name]
            arg_cases = [()] if not prim.params else cases(prim)
            for args in arg_cases:
                if prim.pre is not None and not prim.pre(context(), *args):
                    continue
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


# (name, source, warm-up arguments, word, the word's inputs as fields,
# cases).  The function's code is warmed with the arguments so that word
# `word` is in the executing copy; each case gives the values of the inputs
# in field order.  The template and the form run from the same slots.
TEMPLATE_CASES = [
    ("add_int_locals", "def f(a, b):\n    return a + b", (3, 4),
     "BINARY_OP_ADD_INT", "BC",
     [(1, 2), (-5, 7), (2**49, 2**49), (-2**50, -1), (0, 0),
      ("x", 1),                 # the second guard fails after the first passed
      (1, "y"),                 # the first guard fails
      (big(1), 3), (2.5, 1), ([], [])]),
    ("add_int_temps", "def f(a, b):\n    return (a - 1) + (b - 1)", (3, 4),
     "BINARY_OP_ADD_INT", "BC",
     [(1, 2), (2**50 - 1, 1), (-2**50, -2**50), (big(2), 1), ("x", 1)]),
]


class TemplateTests(unittest.TestCase):
    def compile(self, source, warm):
        namespace = {}
        exec(source, namespace)
        f = namespace["f"]
        for _ in range(200):
            try:
                f(*warm)
            except Exception:
                pass
        return f

    def check_same(self, f, index, slots, objects):
        "The form and the template from the same slots"
        results = []
        for use_template in (False, True):
            before = [sys.getrefcount(o) for o in objects]
            next_, slow, exc, out = capi.jit_template_run(f, index, dict(slots), use_template)
            after = [sys.getrefcount(o) for o in objects]
            results.append((next_, slow, exc, out, [b - a for a, b in zip(before, after)]))
        form, template = results
        self.assertEqual(template[0], form[0], "next word")
        self.assertEqual(type(template[2]), type(form[2]))
        self.assertEqual(str(template[2]), str(form[2]))
        self.assertEqual(len(template[3]), len(form[3]))
        for i, (x, y) in enumerate(zip(form[3], template[3])):
            self.assertEqual(type(x), type(y), f"slot {i}")
            self.assertTrue(same(x, y), f"slot {i}: {x!r} != {y!r}")
        self.assertEqual(template[4], form[4], "counts")
        return template

    def test_templates_against_forms(self):
        for name, source, warm, handler, fields, cases in TEMPLATE_CASES:
            f = self.compile(source, warm)
            index, (_, *word_fields) = word_of(f, handler)
            field_slots = dict(zip("ABCD", word_fields))
            if name.endswith("_temps"):
                self.assertTrue(all(field_slots[x] >= nfixed(f) for x in fields), name)
            for case in cases:
                with self.subTest(name=name, case=case):
                    slots = {field_slots[x]: v for x, v in zip(fields, case)}
                    objects = [v for v in case if not isinstance(v, (int, float)) or
                               type(v) is int and not -2**50 <= v < 2**50]
                    self.check_same(f, index, slots, objects)

    def test_fast_path(self):
        "Inputs that the guards accept stay on the template's own code"
        f = self.compile(TEMPLATE_CASES[0][1], (3, 4))
        index, (_, a, b, c, d) = word_of(f, "BINARY_OP_ADD_INT")
        self.assertFalse(capi.jit_template_run(f, index, {b: 1, c: 2}, True)[1])
        self.assertFalse(capi.jit_template_run(f, index, {b: 2**49, c: 2**49}, True)[1])
        self.assertTrue(capi.jit_template_run(f, index, {b: "x", c: 2}, True)[1])
        self.assertTrue(capi.jit_template_run(f, index, {b: 1, c: "y"}, True)[1])


if __name__ == "__main__":
    unittest.main()
