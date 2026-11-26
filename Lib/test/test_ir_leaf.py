"""The value primitives' specs (Tools/cases_generator/rop_primitives.py)
against their C definitions."""

import ctypes
import math
import random
import types
import unittest

from test import test_tools
from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")
test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import rop_primitives

LAYOUT = rop_primitives.Layout.from_c(capi)
K = LAYOUT.constants
TAG = K["Py_TAG_REFCNT"]


def context(mem=None, frame=0):
    if mem is None:
        mem = rop_primitives.Overlay(rop_primitives.RealMemory())
    return rop_primitives.Context(LAYOUT, mem, frame, capi.leaf_thread())


def word(obj):
    """The canonical word of an object held in a home.  Not for an int or
    float in the inline range, which has no object."""
    return id(obj) | (TAG if _is_immortal(obj) else 0)


def _is_immortal(obj):
    # An immortal object's count is at least 2**31 (Include/refcount.h)
    return ctypes.c_int64.from_address(id(obj)).value >= (1 << 31)


def refcount(obj):
    return ctypes.c_int64.from_address(id(obj)).value


class Holder:
    "An instance with inline values and a shape"
    def __init__(self):
        self.a = 1
        self.b = "two"


rng = random.Random(87)
INT_MIN, INT_MAX = K["Py_INT_MIN"], K["Py_INT_MAX"]
INTS = sorted({0, 1, -1, 2, -2, 255, 2**31, -2**31, 2**50 - 1, -2**50,
               INT_MIN, INT_MAX, INT_MIN + 1, INT_MAX - 1,
               *(rng.randint(INT_MIN, INT_MAX) for _ in range(20))})
WIDE_INTS = sorted({*INTS, INT_MIN - 1, INT_MAX + 1, 2**62, -2**63, 2**63 - 1,
                    *(rng.randint(-2**63, 2**63 - 1) for _ in range(10))})
FLOATS = [0.0, -0.0, 1.0, -1.5, 0.1, 1e300, -1e-300, 5e-324, -5e-324,
          1.7976931348623157e308, math.inf, -math.inf, 2.0**52, 3.0,
          *(rng.uniform(-1e6, 1e6) for _ in range(10))]
NAN = math.nan

# Objects whose words the primitives may read through
OBJECTS = [[1, 2, 3], [], (4, "five", 6.5), (), {"k": 1}, "str", types,
           Holder(), 2**70, -2**80, NAN, True, False, None, len,
           iter(range(3)), unittest.TestCase.run]


def encode_int(i):
    return capi.leaf_primitive("PyValue_FromInlineInt", i)


def encode_float(d):
    return capi.leaf_primitive("PyValue_FromDoubleNotNaN", d)


INLINE_WORDS = [*(encode_int(i) for i in INTS), *(encode_float(d) for d in FLOATS)]
INDEX_WORDS = [capi.leaf_primitive("PyValue_TagIndex", i) for i in (-1, 0, 1, 2**40, 2**47 - 1)]
OBJECT_WORDS = [word(o) for o in OBJECTS]
# Words that a primitive may read through: NULL and every live kind
LIVE_WORDS = [0, *INLINE_WORDS, *INDEX_WORDS, *OBJECT_WORDS]
# Any bits: tag tests never read memory
RANDOM_WORDS = [rng.getrandbits(64) for _ in range(40)] + [
    rng.getrandbits(48) for _ in range(10)] + [
    (0xFFF3 + i) << 48 for i in range(5)] + [1, 2, 3, K["Py_PTR_LIMIT"] - 1,
    K["Py_DOUBLE_OFFSET"] - 1, K["Py_INT_BASE"] - 1, 2**64 - 1]
ANY_WORDS = LIVE_WORDS + RANDOM_WORDS


def objects_of(*types_):
    return [id(o) for o in OBJECTS if type(o) in types_]


# The arguments of each primitive: an iterable of tuples.  Primitives not
# named take the pools of their parameter types.
TYPE_POOLS = {rop_primitives.VALUE: LIVE_WORDS, "i64": WIDE_INTS, "f64": FLOATS + [NAN],
              "PyObject*": [id(o) for o in OBJECTS]}
CASES = {
    "PyValue_IsNull": [(w,) for w in ANY_WORDS],
    "PyValue_IsPointerSpace": [(w,) for w in ANY_WORDS],
    "PyValue_IsObject": [(w,) for w in ANY_WORDS],
    "PyValue_IsInline": [(w,) for w in ANY_WORDS],
    "PyValue_IsInlineInt": [(w,) for w in ANY_WORDS],
    "PyValue_IsInlineFloat": [(w,) for w in ANY_WORDS],
    "PyValue_IsTaggedIndex": [(w,) for w in ANY_WORDS],
    "PyValue_IsTrue": [(w,) for w in ANY_WORDS],
    "PyValue_IsFalse": [(w,) for w in ANY_WORDS],
    "PyValue_IsNone": [(w,) for w in ANY_WORDS],
    "PyValue_BothInlineInt": [(a, b) for a in ANY_WORDS[::3] for b in ANY_WORDS[::4]],
    "PyValue_Is": [(a, b) for a in LIVE_WORDS[::2] for b in LIVE_WORDS[::3]] +
                  [(w, w) for w in LIVE_WORDS] + [(w, w | TAG) for w in OBJECT_WORDS],
    "_PyValue_NamesObject": [(w, id(o)) for w in LIVE_WORDS[::2] for o in OBJECTS[::3]] +
                            [(w | tag, id(o)) for o, w in zip(OBJECTS, OBJECT_WORDS)
                             for tag in (0, TAG)],
    "PyValue_FloatOperandValues": [(a, b) for a in LIVE_WORDS[::2] for b in LIVE_WORDS[::3]],
    "_PyValue_FromIntScaled": [(i << K["Py_INT_SHIFT"],) for i in INTS],
    # The int51 boundaries: sums that fit 64 bits when scaled, and sums that overflow
    "_PyValue_AddCheckedScaled": [
        (a << K["Py_INT_SHIFT"], b << K["Py_INT_SHIFT"])
        for a in INTS for b in INTS],
    "COMPARISON_BIT": [(a, b) for a in INTS[::3] for b in INTS[::3]] +
                      [(a, b) for a in FLOATS[::2] + [NAN] for b in FLOATS[::3] + [NAN]],
    "PyValue_TagIndex": [(i,) for i in (-1, 0, 1, 7, 2**40, 2**47 - 1)],
    "PyList_GET_VALUE": [(id(o), i) for o in OBJECTS if type(o) is list for i in range(len(o))],
    "PyTuple_GET_VALUE": [(id(o), i) for o in OBJECTS if type(o) is tuple for i in range(len(o))],
    "PyList_GET_SIZE": [(a,) for a in objects_of(list)],
    "_PyList_ITEMS": [(a,) for a in objects_of(list)],
    "_PyTuple_ITEMS": [(a,) for a in objects_of(tuple)],
    "PyTuple_GET_SIZE": [(a,) for a in objects_of(tuple)],
    "_PyObject_InlineValues": [(a,) for a in objects_of(Holder)],
    "_PyObject_Shape": [(a,) for a in objects_of(Holder)],
    "_PyObject_ValueSlot": [(a, k) for a in objects_of(Holder) for k in (0, 8, 48)],
    "DK_UNICODE_ENTRIES": [(id(o.__dict__ if isinstance(o, types.ModuleType) else o),)
                           for o in OBJECTS if type(o) in (dict, types.ModuleType)],
    "_PyLong_IsZero": [(a,) for a in objects_of(int)],
    "_PyValueGuarded_Load": [(w,) for w in ANY_WORDS],
    "_PyValue_CodeConstant": [(w,) for w in ANY_WORDS],
    "_PyValue_BoundLocal": [(w,) for w in ANY_WORDS],
    "_PyValue_CellLocal": [(w,) for w in ANY_WORDS],
    "_PyValue_CommonConstant": [(w,) for w in ANY_WORDS],
    "_PyValue_PublishedItem": [(w,) for w in ANY_WORDS],
    "GETLOCAL": [(0,)],
}
# The dict keys: its me_value read goes through DK_UNICODE_ENTRIES
for args in list(CASES["DK_UNICODE_ENTRIES"]):
    keys = ctypes.c_uint64.from_address(args[0] + LAYOUT.offsets["PyDictObject.ma_keys"]).value
    CASES["DK_UNICODE_ENTRIES"].remove(args)
    CASES["DK_UNICODE_ENTRIES"].append((keys,))

# Tested apart: they change memory, need the caller's frame, or allocate
SEPARATE = {"PyValue_DUP", "PyValue_XDUP", "PyValue_CLOSE", "PyValue_XCLOSE",
            "PyValue_CLOSE_SPECIALIZED", "_PyObject_ShapeReadDup",
            "_PyDictValues_AddToInsertionOrder", "_PyDictValues_SetShape",
            "_PyValueSlot_Exchange", "_PyValueSlot_ExchangeFilled", "_PyValueSlot_Fill",
            "GLOBALS", "BUILTINS", "FRAME_CO_CONSTS", "GETLOCAL", "IR_SLOTS",
            "_Py_EvalBreakerHasEvents", "_Py_EvalBreaker",
            "FRAME_CODE_INSTRUMENTATION_VERSION",
            "PyValue_FromInt64", "PyValue_FromDouble"}


def cases(prim):
    if prim.name in CASES:
        return CASES[prim.name]
    pools = [TYPE_POOLS[t] for t in prim.params]
    if len(pools) == 1:
        return [(x,) for x in pools[0]]
    raise AssertionError(f"no cases for {prim.name}")


def same(a, b):
    if isinstance(a, float) and isinstance(b, float):
        return (math.isnan(a) and math.isnan(b)) or (a == b and math.copysign(1, a) == math.copysign(1, b))
    if isinstance(a, tuple) and isinstance(b, tuple):
        return len(a) == len(b) and all(same(x, y) for x, y in zip(a, b))
    return a == b


def run_spec(prim, args, cx=None):
    cx = cx or context()
    if prim.pre is not None and not prim.pre(cx, *args):
        return None, cx
    result = prim.spec(cx, *args)
    return result, cx


class LayoutTests(unittest.TestCase):
    def test_the_list_is_closed(self):
        self.assertEqual(set(rop_primitives.PRIMITIVES), set(capi.leaf_primitive_names()))

    def test_layout_names_every_number(self):
        self.assertEqual(set(LAYOUT.constants), set(rop_primitives.CONSTANTS))
        self.assertEqual(set(LAYOUT.offsets), set(rop_primitives.FIELDS))
        self.assertEqual(set(LAYOUT.sizes), set(rop_primitives.SIZES))
        self.assertEqual(set(LAYOUT.symbols), set(rop_primitives.SYMBOLS))

    def test_signatures_use_subset_types(self):
        known = set(rop_primitives.SCALARS) | {rop_primitives.VALUE, "destructor"}
        for prim in rop_primitives.PRIMITIVES.values():
            for t in prim.params + prim.results:
                with self.subTest(primitive=prim.name, type=t):
                    if rop_primitives.is_pointer(t):
                        self.assertIn(t[:-1], rop_primitives.POINTER_TARGETS + ("PyDictValues",))
                    else:
                        self.assertIn(t, known)
            if prim.slow:
                self.assertIn(prim.slow, rop_primitives.HELPERS)


class PrimitiveTests(unittest.TestCase):
    def test_pure_and_reading_primitives(self):
        for prim in rop_primitives.PRIMITIVES.values():
            if prim.name in SEPARATE:
                continue
            checked = 0
            for args in cases(prim):
                expected, cx = run_spec(prim, args)
                if cx is None or (prim.pre is not None and not prim.pre(cx, *args)):
                    continue
                with self.subTest(primitive=prim.name, args=args):
                    got = capi.leaf_primitive(prim.name, *args)
                    self.assertTrue(same(got, expected), f"C {got!r}, spec {expected!r}")
                    self.assertEqual(cx.mem.writes(), [])
                checked += 1
            self.assertGreater(checked, 0, prim.name)

    def test_constructors_with_a_slow_path(self):
        for name, args, kind in [("PyValue_FromInt64", WIDE_INTS, int),
                                 ("PyValue_FromDouble", FLOATS + [NAN], float)]:
            prim = rop_primitives.PRIMITIVES[name]
            for x in args:
                with self.subTest(primitive=name, arg=x):
                    expected, cx = run_spec(prim, (x,))
                    got = capi.leaf_primitive(name, x)
                    if isinstance(expected, rop_primitives.Box):
                        # The slow path: a new object of the same value
                        self.assertIs(type(got), kind)
                        self.assertTrue(same(got, expected.value))
                        self.assertEqual(cx.calls, [(prim.slow, (x,))])
                    else:
                        self.assertEqual(got, expected)
                        self.assertEqual(cx.calls, [])

    def test_frame_primitives(self):
        # The spec and C read the frame of this function
        frame = capi.leaf_frame()
        for name in ("GLOBALS", "BUILTINS", "FRAME_CO_CONSTS", "IR_SLOTS",
                     "FRAME_CODE_INSTRUMENTATION_VERSION"):
            with self.subTest(primitive=name):
                expected = rop_primitives.PRIMITIVES[name].spec(context(frame=frame))
                self.assertEqual(capi.leaf_primitive(name), expected)
        self.assertEqual(rop_primitives.PRIMITIVES["GLOBALS"].spec(context(frame=frame)), id(globals()))
        expected = rop_primitives.PRIMITIVES["GETLOCAL"].spec(context(frame=frame), 0)
        self.assertEqual(capi.leaf_primitive("GETLOCAL", 0), expected)
        self.assertEqual(expected & ~TAG, id(self))

    def test_thread_primitives(self):
        prim = rop_primitives.PRIMITIVES["_Py_EvalBreakerHasEvents"]
        self.assertEqual(capi.leaf_primitive(prim.name), prim.spec(context()))
        # An event bit, written to the overlay only
        cx = context()
        breaker = cx.tstate + LAYOUT.offsets["PyThreadState.eval_breaker"]
        for bits, expected in ((1 << 4, True), (K["_PY_EVAL_EVENTS_MASK"] + 1, False)):
            cx.store(breaker, "u64", cx.load(breaker, "u64") & ~K["_PY_EVAL_EVENTS_MASK"] | bits)
            self.assertIs(prim.spec(cx), expected)
        breaker = rop_primitives.PRIMITIVES["_Py_EvalBreaker"]
        self.assertEqual(capi.leaf_primitive(breaker.name), breaker.spec(context()))

    def check_count_change(self, name, obj, *extra):
        "The spec's write of obj's count against the change C makes"
        prim = rop_primitives.PRIMITIVES[name]
        w = word(obj)
        expected, cx = run_spec(prim, (w, *extra))
        delta = {addr: after - before for addr, t, before, after in cx.mem.writes()}
        before = refcount(obj)
        capi.leaf_primitive(name, w, *extra)
        change = refcount(obj) - before
        self.assertEqual(delta.get(id(obj), 0), change)
        return expected, change

    def test_ownership(self):
        for obj in OBJECTS:
            for name in ("PyValue_DUP", "PyValue_XDUP"):
                with self.subTest(primitive=name, obj=obj):
                    expected, change = self.check_count_change(name, obj)
                    self.assertEqual(expected, word(obj))
                    if change:
                        ctypes.pythonapi.Py_DecRef(ctypes.py_object(obj))
            for name, extra in (("PyValue_CLOSE", ()), ("PyValue_XCLOSE", ()),
                                ("PyValue_CLOSE_SPECIALIZED", ("_PyLong_ExactDealloc",))):
                if name == "PyValue_CLOSE_SPECIALIZED" and type(obj) is not int:
                    continue
                with self.subTest(primitive=name, obj=obj):
                    ctypes.pythonapi.Py_IncRef(ctypes.py_object(obj))
                    _, change = self.check_count_change(name, obj, *extra)
                    if not change:
                        ctypes.pythonapi.Py_DecRef(ctypes.py_object(obj))
        for word_ in [0, *INLINE_WORDS[:5], *INDEX_WORDS]:
            for name in ("PyValue_XDUP", "PyValue_XCLOSE"):
                with self.subTest(primitive=name, word=word_):
                    expected, cx = run_spec(rop_primitives.PRIMITIVES[name], (word_,))
                    self.assertEqual(cx.mem.writes(), [])
                    capi.leaf_primitive(name, word_)

    def test_shape_read_dup(self):
        holder = Holder()
        values = capi.leaf_primitive("_PyObject_InlineValues", id(holder))
        first = values + LAYOUT.offsets["PyDictValues.values"] - id(holder)
        for k, attr in enumerate(("a", "b")):
            offset = first + k * LAYOUT.sizes["PyValue"]
            expected, cx = run_spec(rop_primitives.PRIMITIVES["_PyObject_ShapeReadDup"], (id(holder), offset))
            got = capi.leaf_primitive("_PyObject_ShapeReadDup", id(holder), offset)
            self.assertEqual(got, expected)
            obj = getattr(holder, attr)
            if got & ~TAG == id(obj) and not got & TAG:
                ctypes.pythonapi.Py_DecRef(ctypes.py_object(obj))

    def test_value_slot_stores(self):
        "A store into a slot: the word in, and the slot's word out where there is one"
        slot = (ctypes.c_uint64 * 1)()
        address = ctypes.addressof(slot)
        new = encode_int(-9)
        for name, held in (("_PyValueSlot_Exchange", 0),
                           ("_PyValueSlot_Exchange", encode_int(5)),
                           ("_PyValueSlot_Exchange", word(OBJECTS[0])),
                           ("_PyValueSlot_ExchangeFilled", encode_int(5)),
                           ("_PyValueSlot_ExchangeFilled", word(OBJECTS[0])),
                           ("_PyValueSlot_Fill", 0)):
            with self.subTest(primitive=name, held=held):
                slot[0] = held
                expected, cx = run_spec(rop_primitives.PRIMITIVES[name], (address, new))
                self.assertEqual([(a, after) for a, _, _, after in cx.mem.writes()],
                                 [(address, new)])
                self.assertEqual(slot[0], held)
                got = capi.leaf_primitive(name, address, new)
                self.assertEqual(slot[0], new)
                if rop_primitives.PRIMITIVES[name].results:
                    self.assertEqual(expected, held)
                    self.assertEqual(got, held)
        # The preconditions: the filled form needs a word in the slot, the fill a NULL
        slot[0] = 0
        self.assertIsNone(run_spec(rop_primitives.PRIMITIVES["_PyValueSlot_ExchangeFilled"],
                                   (address, new))[0])
        slot[0] = new
        self.assertIsNone(run_spec(rop_primitives.PRIMITIVES["_PyValueSlot_Fill"], (address, new))[0])

    def test_dict_values_writes(self):
        holder = Holder()
        values = capi.leaf_primitive("_PyObject_InlineValues", id(holder))
        shape = capi.leaf_primitive("_PyObject_Shape", id(holder))
        # Writing the current shape changes nothing; the spec says where
        _, cx = run_spec(rop_primitives.PRIMITIVES["_PyDictValues_SetShape"], (values, shape))
        capi.leaf_primitive("_PyDictValues_SetShape", values, shape)
        self.assertEqual([(a, after) for a, _, _, after in cx.mem.writes()],
                         [(values + LAYOUT.offsets["PyDictValues.shape"], shape)])
        self.assertEqual(capi.leaf_primitive("_PyObject_Shape", id(holder)), shape)
        # An insertion: compare, then restore the two bytes it wrote
        _, cx = run_spec(rop_primitives.PRIMITIVES["_PyDictValues_AddToInsertionOrder"], (values, 1))
        writes = cx.mem.writes()
        saved = [(addr, ctypes.c_uint8.from_address(addr).value) for addr, *_ in writes]
        try:
            capi.leaf_primitive("_PyDictValues_AddToInsertionOrder", values, 1)
            for addr, t, before, after in writes:
                self.assertEqual(ctypes.c_uint8.from_address(addr).value, after)
        finally:
            for addr, byte in saved:
                ctypes.c_uint8.from_address(addr).value = byte
