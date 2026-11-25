"""Leaf uops (Tools/cases_generator/ir_leaf.py): the value primitives' specs
against their C definitions, and the leaf-subset checker."""

import ctypes
import math
from pathlib import Path
import random
import tempfile
import types
import unittest
from unittest import mock

from test import test_generated_cases as generated
from test import test_tools
from test.support import import_helper

capi = import_helper.import_module("_testinternalcapi")
test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import ir_leaf
    import ir_contract
    import ir_leaf_eval

LAYOUT = ir_leaf_eval.Layout.from_c(capi)
K = LAYOUT.constants
TAG = K["Py_TAG_REFCNT"]


def context(mem=None, frame=0):
    if mem is None:
        mem = ir_leaf_eval.Overlay(ir_leaf_eval.RealMemory())
    return ir_leaf_eval.Context(LAYOUT, mem, frame, capi.leaf_thread())


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
TYPE_POOLS = {ir_leaf.VALUE: LIVE_WORDS, "i64": WIDE_INTS, "f64": FLOATS + [NAN],
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
        self.assertEqual(set(ir_leaf.PRIMITIVES), set(capi.leaf_primitive_names()))

    def test_layout_names_every_number(self):
        self.assertEqual(set(LAYOUT.constants), set(ir_leaf.CONSTANTS))
        self.assertEqual(set(LAYOUT.offsets), set(ir_leaf.FIELDS))
        self.assertEqual(set(LAYOUT.sizes), set(ir_leaf.SIZES))
        self.assertEqual(set(LAYOUT.symbols), set(ir_leaf.SYMBOLS))

    def test_signatures_use_subset_types(self):
        known = set(ir_leaf.SCALARS) | {ir_leaf.VALUE, "destructor"}
        for prim in ir_leaf.PRIMITIVES.values():
            for t in prim.params + prim.results:
                with self.subTest(primitive=prim.name, type=t):
                    if ir_leaf.is_pointer(t):
                        self.assertIn(t[:-1], ir_leaf.POINTER_TARGETS + ("PyDictValues",))
                    else:
                        self.assertIn(t, known)
            if prim.slow:
                self.assertIn(prim.slow, ir_leaf.HELPERS)


class PrimitiveTests(unittest.TestCase):
    def test_pure_and_reading_primitives(self):
        for prim in ir_leaf.PRIMITIVES.values():
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
            prim = ir_leaf.PRIMITIVES[name]
            for x in args:
                with self.subTest(primitive=name, arg=x):
                    expected, cx = run_spec(prim, (x,))
                    got = capi.leaf_primitive(name, x)
                    if isinstance(expected, ir_leaf_eval.Box):
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
                expected = ir_leaf.PRIMITIVES[name].spec(context(frame=frame))
                self.assertEqual(capi.leaf_primitive(name), expected)
        self.assertEqual(ir_leaf.PRIMITIVES["GLOBALS"].spec(context(frame=frame)), id(globals()))
        expected = ir_leaf.PRIMITIVES["GETLOCAL"].spec(context(frame=frame), 0)
        self.assertEqual(capi.leaf_primitive("GETLOCAL", 0), expected)
        self.assertEqual(expected & ~TAG, id(self))

    def test_thread_primitives(self):
        prim = ir_leaf.PRIMITIVES["_Py_EvalBreakerHasEvents"]
        self.assertEqual(capi.leaf_primitive(prim.name), prim.spec(context()))
        # An event bit, written to the overlay only
        cx = context()
        breaker = cx.tstate + LAYOUT.offsets["PyThreadState.eval_breaker"]
        for bits, expected in ((1 << 4, True), (K["_PY_EVAL_EVENTS_MASK"] + 1, False)):
            cx.store(breaker, "u64", cx.load(breaker, "u64") & ~K["_PY_EVAL_EVENTS_MASK"] | bits)
            self.assertIs(prim.spec(cx), expected)
        breaker = ir_leaf.PRIMITIVES["_Py_EvalBreaker"]
        self.assertEqual(capi.leaf_primitive(breaker.name), breaker.spec(context()))

    def check_count_change(self, name, obj, *extra):
        "The spec's write of obj's count against the change C makes"
        prim = ir_leaf.PRIMITIVES[name]
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
                    expected, cx = run_spec(ir_leaf.PRIMITIVES[name], (word_,))
                    self.assertEqual(cx.mem.writes(), [])
                    capi.leaf_primitive(name, word_)

    def test_shape_read_dup(self):
        holder = Holder()
        values = capi.leaf_primitive("_PyObject_InlineValues", id(holder))
        first = values + LAYOUT.offsets["PyDictValues.values"] - id(holder)
        for k, attr in enumerate(("a", "b")):
            offset = first + k * LAYOUT.sizes["PyValue"]
            expected, cx = run_spec(ir_leaf.PRIMITIVES["_PyObject_ShapeReadDup"], (id(holder), offset))
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
                expected, cx = run_spec(ir_leaf.PRIMITIVES[name], (address, new))
                self.assertEqual([(a, after) for a, _, _, after in cx.mem.writes()],
                                 [(address, new)])
                self.assertEqual(slot[0], held)
                got = capi.leaf_primitive(name, address, new)
                self.assertEqual(slot[0], new)
                if ir_leaf.PRIMITIVES[name].results:
                    self.assertEqual(expected, held)
                    self.assertEqual(got, held)
        # The preconditions: the filled form needs a word in the slot, the fill a NULL
        slot[0] = 0
        self.assertIsNone(run_spec(ir_leaf.PRIMITIVES["_PyValueSlot_ExchangeFilled"],
                                   (address, new))[0])
        slot[0] = new
        self.assertIsNone(run_spec(ir_leaf.PRIMITIVES["_PyValueSlot_Fill"], (address, new))[0])

    def test_dict_values_writes(self):
        holder = Holder()
        values = capi.leaf_primitive("_PyObject_InlineValues", id(holder))
        shape = capi.leaf_primitive("_PyObject_Shape", id(holder))
        # Writing the current shape changes nothing; the spec says where
        _, cx = run_spec(ir_leaf.PRIMITIVES["_PyDictValues_SetShape"], (values, shape))
        capi.leaf_primitive("_PyDictValues_SetShape", values, shape)
        self.assertEqual([(a, after) for a, _, _, after in cx.mem.writes()],
                         [(values + LAYOUT.offsets["PyDictValues.shape"], shape)])
        self.assertEqual(capi.leaf_primitive("_PyObject_Shape", id(holder)), shape)
        # An insertion: compare, then restore the two bytes it wrote
        _, cx = run_spec(ir_leaf.PRIMITIVES["_PyDictValues_AddToInsertionOrder"], (values, 1))
        writes = cx.mem.writes()
        saved = [(addr, ctypes.c_uint8.from_address(addr).value) for addr, *_ in writes]
        try:
            capi.leaf_primitive("_PyDictValues_AddToInsertionOrder", values, 1)
            for addr, t, before, after in writes:
                self.assertEqual(ctypes.c_uint8.from_address(addr).value, after)
        finally:
            for addr, byte in saved:
                ctypes.c_uint8.from_address(addr).value = byte


def analyze(source):
    "The uops of DSL source"
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "bytecodes.c"
        path.write_text(generated.parser.BEGIN_MARKER + "\n" + source + "\n" +
                        generated.parser.END_MARKER)
        with generated.handle_stderr():
            return generated.analyzer.analyze_files([str(path)]).uops


SPAM_ROWS = """
#define SPAM_ROWS(PRIMITIVE, HELPER, MACRO) \\
    MACRO(SPAM, ARGS(scalar, scalar, scalar), SCALAR, EFFECTS(reads), EXITS())
"""

# Declarations of the refusals' uops: one that reads a borrowed value and
# makes a new one, and one that passes its consumed value through; each with
# the exits of a body that guards and of one that does not
READS = ("(value: borrowed -- res: inline)", "effects(reads) exits(next)")
PASSES = ("(value: consumed -- res: moved(value))", "effects(reads) exits(next)")
PASSES_OBJECT = ("(value: consumed object -- res: moved(value))", "effects(reads) exits(next)")
GUARDS = "exits(next, miss)"

def guarded(declaration):
    "The declaration of a uop whose body can also exit by a miss"
    stack, clauses = declaration
    return stack, clauses.replace("exits(next)", GUARDS)


# Bodies outside the subset, and the construct the checker names
REFUSALS = {
    "a for loop": (guarded(READS), """
        for (int i = 0; i < 3; i++) {
            EXIT_IF(i);
        }"""),
    "a call of SPAM, which is neither a listed primitive nor a declared helper":
        (READS, "res = PyValue_FromInlineInt(SPAM(value, 0, 0));"),
    # A row without a model of its own (a HELPER row, V11) or of another build
    "a call of PyValue_TYPE, which is neither a listed primitive nor a declared helper":
        (READS, "res = PyValue_FromInlineInt(PyValue_TYPE(value)->tp_flags);"),
    "a call of _Py_IsOwnedByCurrentThread, which is neither a listed primitive nor a declared "
    "helper": (guarded(PASSES), "EXIT_IF(_Py_IsOwnedByCurrentThread(value)); res = value; DEAD(value);"),
    "untyped memory":
        (PASSES_OBJECT, "char *p = (char *)PyValue_AsKnownObjectBorrow(value); res = value; DEAD(value);"),
    "field PyTypeObject.tp_as_mapping is not listed": (guarded(PASSES_OBJECT), """
        EXIT_IF(Py_TYPE(PyValue_AsKnownObjectBorrow(value))->tp_as_mapping == NULL);
        res = value; DEAD(value);"""),
    "the evaluator's tstate": (guarded(PASSES), "EXIT_IF(tstate == NULL); res = value; DEAD(value);"),
    "_Py_EvalBreakerHasEvents's first argument is not tstate":
        (guarded(PASSES), "EXIT_IF(_Py_EvalBreakerHasEvents(frame)); res = value; DEAD(value);"),
    "a value used as a condition": (guarded(PASSES), "EXIT_IF(value); res = value; DEAD(value);"),
}


class CheckerTests(unittest.TestCase):
    def setUp(self):
        # SPAM stands for a function that is no primitive: a declared body
        # calls only rows
        rows = mock.patch.dict(ir_contract.rows(), ir_contract.parse_rows(SPAM_ROWS))
        rows.start()
        self.addCleanup(rows.stop)

    def test_refusals(self):
        for expected, ((stack, clauses), body) in REFUSALS.items():
            with self.subTest(expected):
                uop = analyze(f"op(_OP, {stack}) {clauses} {{ {body} }}")["_OP"]
                with self.assertRaises(ir_leaf.LeafError) as cm:
                    ir_leaf.translate(uop)
                self.assertIn(expected, str(cm.exception))

    def test_forms_come_from_rows(self):
        # The identities, the constant and the erased forms are INTRINSIC rows
        # of the generated build (G0d3); TAKE moves its argument
        for name, shape in (("PyValue_Borrow", "VIEW"), ("FT_ATOMIC_LOAD_PTR", "SCALAR"),
                            ("LOCK_OBJECT", "LITERAL"), ("UNLOCK_OBJECT", "NONE"),
                            ("QSBR_QUIESCENT_STATE", "NONE")):
            with self.subTest(name):
                self.assertEqual(ir_leaf.intrinsic(name).result.shape, shape)
        self.assertEqual(ir_leaf.intrinsic("TAKE").result.moved, 0)
        # Another build's form, a model's primitive and a name of no row are no form
        for name in ("_Py_IsOwnedByCurrentThread", "PyValue_IsNull", "SPAM"):
            with self.subTest(name):
                self.assertIsNone(ir_leaf.intrinsic(name))
        self.assertTrue(ir_leaf.erased("UNLOCK_OBJECT"))
        self.assertTrue(ir_leaf.erased("STAT_INC"))
        self.assertFalse(ir_leaf.erased("PyValue_Borrow"))
        for name in ("IDENTITIES", "ERASED", "CONSTANT_FORMS"):
            self.assertFalse(hasattr(ir_leaf, name))

    def test_array_operand(self):
        uop = analyze("""
            op(_OP, (values[oparg]: consumed --)) effects(releases) exits(next) {
                DECREF_INPUTS();
            }""")["_OP"]
        with self.assertRaisesRegex(ir_leaf.LeafError, r"array input values\[oparg\]"):
            ir_leaf.translate(uop)

    def test_annotated_uop_must_be_leaf(self):
        uops = analyze("""
            leaf op(_OK, (value -- value: inline_int)) effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(value));
            }
            leaf op(_BAD, (value: borrowed -- res: inline)) effects(reads) exits(next) {
                res = PyValue_FromInlineInt(SPAM(value, 0, 0));
            }""")
        records = {}
        for name, uop in uops.items():
            leaf = ir_leaf.leaf_of(uop, {})
            records[name] = types.SimpleNamespace(
                uops=[types.SimpleNamespace(name=name, feedback=(), leaf=leaf)])
        ir_leaf.check_leaf(records["_OK"], [uops["_OK"]])
        with self.assertRaisesRegex(SyntaxError, "_BAD is annotated leaf, but a call of SPAM"):
            ir_leaf.check_leaf(records["_BAD"], [uops["_BAD"]])

    def test_translation(self):
        uop = analyze("""
            op(_OP, (value: consumed -- res: inline, v: moved(value) inline_int))
                effects() exits(next, miss) {
                EXIT_IF(!PyValue_IsInlineInt(value));
                res = PyValue_IsNone(value) ? PyValue_False : PyValue_True;
                v = value;
                DEAD(value);
            }""")["_OP"]
        body = ir_leaf.translate(uop)
        self.assertEqual(body.dump(), [
            "b0:",
            "  %0:value = arg value",
            "  %1:bool = prim PyValue_IsInlineInt %0",
            "  %2:bool = lnot %1",
            "  exit_if EXIT %2",
            "  %3:bool = prim PyValue_IsNone %0",
            "  %4:value = const PyValue_False",
            "  %5:value = const PyValue_True",
            "  %6:value = select %3, %4, %5",
            "  out res %6",
            "  out v %0",
            "  next",
        ])
        self.assertEqual(body.effects, frozenset())

    def test_assumptions(self):
        # F7: the leaf IR states a declared assumption as an assume op, which
        # the templates check in a build with assertions
        source = """
            op(_OP, (value: borrowed assumed(%s) by _P -- res: inline)) effects() exits(next) {
                res = PyValue_IsNone(value) ? PyValue_False : PyValue_True;
            }"""
        body = ir_leaf.translate(analyze(source % "exact(PyUnicode_Type|PyBytes_Type)")["_OP"])
        self.assertEqual(body.dump()[:3], [
            "b0:",
            "  %0:value = arg value",
            "  assume exact PyUnicode_Type|PyBytes_Type %0",
        ])
        self.assertEqual(body.effects, frozenset())
        for fact in ("nonnull", "object", "inline_int", "inline_float"):
            with self.subTest(fact=fact):
                body = ir_leaf.translate(analyze(source % fact)["_OP"])
                self.assertIn(f"  assume {fact}  %0", body.dump())
        # Each fact with a C assertion is one the templates check
        self.assertEqual(ir_leaf.ASSUMABLE, tuple(ir_contract.ASSERTION_FORMS))


BYTECODES = Path(test_tools.basepath) / "Python" / "bytecodes.c"


class RecordTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        ir_generator = generated.ir_generator
        with generated.handle_stderr():
            analysis = generated.analyzer.analyze_files([str(BYTECODES)])
        cls.records = ir_generator.build_records(
            analysis, ir_generator.compute_formats(analysis))
        cls.uops = {part.name: part
                    for inst in ir_generator.handler_instructions(analysis).values()
                    for part in inst.parts if isinstance(part, generated.analyzer.Uop)}

    def record_uops(self):
        for record in self.records.values():
            yield from record.uops

    def test_annotated_uops_have_leaf_ir(self):
        annotated = {name for name, uop in self.uops.items() if "leaf" in uop.annotations}
        self.assertGreater(len(annotated), 50)
        for uop in self.record_uops():
            if uop.name in annotated:
                self.assertIsNotNone(uop.leaf, uop.name)

    def test_effects_agree_with_the_records(self):
        for uop in self.record_uops():
            if uop.leaf is not None:
                self.assertEqual(uop.leaf.effects, uop.effects, uop.name)

    def test_leaf_helpers_are_declared(self):
        for uop in self.record_uops():
            if uop.leaf is not None:
                for name in uop.leaf.helpers:
                    self.assertIn(name, ir_leaf.HELPERS, uop.name)

    def test_record_dump_prints_the_leaf(self):
        with test_tools.imports_under_tool("cases_generator"):
            import ir_records
        text = ir_records.format_record(self.records["BINARY_OP_ADD_INT"])
        self.assertIn("  uop 2 _BINARY_OP_ADD_INT", text)
        self.assertIn("call _PyValue_BoxInt64", text)
        self.assertRegex(text, r"exit_if EXIT %\d+ \[")


# ---------------------------------------------------------------------------
# Uops: the leaf IR's evaluation (ir_leaf_eval.evaluate) against the C uop
# (its probe, Modules/_testinternalcapi/leaf_probes.c.h) on generated inputs.

class Shaped:
    "Instances with inline values: the shape guard and read"
    def __init__(self, a, b):
        self.a = a
        self.b = b


SHAPED = [Shaped(1, "two"), Shaped(2.5, None), Shaped([3], (4,))]
SHAPED[0].a         # gives Shaped a type version
# Mortal objects: a uop's counts on them change
BIG_INTS = [2**70, -2**80, 2**63, -2**63 - 1]
LISTS = [[1, 2, 3], ["a", 2.5, None, 2**70], [], [7]]
TUPLES = [(4, "five", 6.5), (), (2**70,), (None, True)]
INT_WORDS = [encode_int(i) for i in INTS[::2]] + [word(i) for i in BIG_INTS]
FLOAT_WORDS = [encode_float(d) for d in FLOATS[::2]] + [word(NAN)]
OTHERS = ["str", None, True, 2.5j, SHAPED[0]]     # held: their words borrow
OTHER_WORDS = [word(o) for o in OTHERS]
BOOL_WORDS = [word(True), word(False)]
INDEX_OPARGS = range(0, 3)


def owns_count(w):
    return ir_leaf._owns_count(context(), w)


def object_at(w):
    "The object of an owning word, with a new reference held by Python"
    return ctypes.cast(w & ~TAG, ctypes.py_object).value


def int_domain():
    return [(a, b) for a in INT_WORDS for b in INT_WORDS[::2]]


def float_domain():
    return [(a, b) for a in FLOAT_WORDS for b in FLOAT_WORDS[::2]]


def mixed_domain():
    pool = INT_WORDS[::3] + FLOAT_WORDS[::3] + OTHER_WORDS
    return [(a, b) for a in pool for b in pool]


def subscript_domain(containers):
    return [(word(c), encode_int(i)) for c in containers
            for i in range(-len(c) - 1, len(c) + 1)] + [
            (word(containers[0]), w) for w in OTHER_WORDS[:2] + [word(2**70)]]


def iterator_domain(make):
    return [(word(it), capi.leaf_primitive("PyValue_TagIndex", i))
            for it, n in make() for i in range(n + 2)]


def shape_caches(inputs):
    owner = object_at(inputs[0])
    version = _type_version(type(owner))
    shape = capi.leaf_primitive("_PyObject_Shape", id(owner))
    values = capi.leaf_primitive("_PyObject_InlineValues", id(owner))
    first = values + LAYOUT.offsets["PyDictValues.values"] - id(owner)
    return [{"type_version": version, "shape": shape, "offset": first + k * 8}
            for k in range(2)] + [{"type_version": version + 1, "shape": shape, "offset": first},
                                  {"type_version": version, "shape": shape ^ 1, "offset": first}]


# Owners that a store changes: made afresh, and kept alive here
STORE_OWNERS = []


def store_domain():
    STORE_OWNERS.clear()
    STORE_OWNERS.extend(Shaped(k, str(k)) for k in range(3))
    values = [encode_int(5), word(None), word(BIG_INTS[0]), word(LISTS[0])]
    return [(v, word(o)) for o in STORE_OWNERS for v in values]


def store_caches(inputs):
    return shape_caches(inputs[1:])


def _type_version(tp):
    return ctypes.c_uint32.from_address(id(tp) + LAYOUT.offsets["PyTypeObject.tp_version_tag"]).value


# Handler: (inputs, a function of the inputs giving the cache operands, opargs)
HANDLERS = {
    **{name: (int_domain, None, [0]) for name in (
        "BINARY_OP_ADD_INT", "BINARY_OP_SUBTRACT_INT", "BINARY_OP_MULTIPLY_INT",
        "BINARY_OP_FLOOR_DIVIDE_INT", "BINARY_OP_REMAINDER_INT")},
    **{name: (float_domain, None, [0]) for name in (
        "BINARY_OP_ADD_FLOAT", "BINARY_OP_SUBTRACT_FLOAT", "BINARY_OP_MULTIPLY_FLOAT",
        "BINARY_OP_TRUEDIV_FLOAT")},
    "COMPARE_OP_JUMP_INT": (mixed_domain, None, [i << 5 | k for i in range(6) for k in (0, 16)]),
    "COMPARE_OP_JUMP_FLOAT": (float_domain, None, [i << 5 | k for i in range(6) for k in (0, 16)]),
    "IS_OP_JUMP": (mixed_domain, None, [0, 1]),
    "POP_JUMP_IF_FALSE_BOOL": (lambda: [(w,) for w in BOOL_WORDS + OTHER_WORDS], None, [0]),
    "POP_JUMP_IF_TRUE_BOOL": (lambda: [(w,) for w in BOOL_WORDS + OTHER_WORDS], None, [0]),
    "POP_JUMP_IF_NOT_NONE": (lambda: [(w,) for w in BOOL_WORDS + OTHER_WORDS + INT_WORDS[:3]],
                             None, [0]),
    "POP_JUMP_IF_FALSE_INT": (lambda: [(w,) for w in INT_WORDS + OTHER_WORDS], None, [0]),
    "BINARY_OP_SUBSCR_LIST_INT": (lambda: subscript_domain(LISTS), None, [0]),
    "BINARY_OP_SUBSCR_TUPLE_INT": (lambda: subscript_domain(TUPLES), None, [0]),
    "FOR_ITER_LIST": (lambda: iterator_domain(lambda: [(l, len(l)) for l in LISTS]), None, [0]),
    "FOR_ITER_TUPLE": (lambda: iterator_domain(lambda: [(t, len(t)) for t in TUPLES]), None, [0]),
    "FOR_ITER_RANGE": (lambda: [(word(it), 0) for it in
                                (iter(range(3)), iter(range(5, -2, -2)), iter(range(0)))]
                       + [(word(it), 0) for it in (iter([1]),)], None, [0]),
    "STORE_ATTR_SHAPE": (store_domain, store_caches, [0]),
    "LOAD_SMALL_INT_INLINE": (lambda: [()], None, [0, 1, 200, 255]),
    "LOAD_FAST_BORROW": (lambda: [()], None, [0, 1]),
    "LOAD_FAST": (lambda: [()], None, [0, 1]),
    "JUMP_FORWARD": (lambda: [()], None, [0, 3]),
    "JUMP_BACKWARD": (lambda: [()], None, [0, 3]),
}


def output_positions(part, oparg):
    "The stack position of each named output of a uop, and the stack's height"
    positions, height = {}, 0
    for item in part.stack.outputs:
        if item.name != "unused":
            positions[item.name] = height
        height += eval(item.size, {"oparg": oparg}) if item.is_array() else 1
    return positions, height


class LeafUopTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        RecordTests.setUpClass.__func__(cls)

    def check_uop(self, uop, part, env, caches, oparg, fused, keep):
        """Run uop on env's words in C and in the evaluator; compare.  Returns
        the outcome, or None if the inputs make the C undefined."""
        for guard in uop.guards:
            if guard.site is None:
                w = next(env[i.value] for i in uop.inputs if i.name == guard.input)
                if not ir_leaf.PRIMITIVES["PyValue_IsObject"].spec(context(), w):
                    return "EXIT"
        words = [env[i.value] for i in uop.inputs]
        consumed = [w for i, w in zip(uop.inputs, words)
                    if i.ownership == "CONSUMED" and owns_count(w)]
        for w in consumed:
            ctypes.pythonapi.Py_IncRef(ctypes.py_object(object_at(w)))
        cache_words = [caches[fb.name] for fb in uop.feedback]
        cx = context(frame=capi.leaf_frame())
        try:
            expected = ir_leaf_eval.evaluate(
                uop.leaf, cx, {i.name: w for i, w in zip(uop.inputs, words)},
                {fb.name: caches[fb.name] for fb in uop.feedback}, oparg,
                {"fused_if_false": fused})
        except ir_leaf_eval.UndefinedBehavior:
            for w in consumed:
                ctypes.pythonapi.Py_DecRef(ctypes.py_object(object_at(w)))
            return None
        got = capi.leaf_probe(uop.name, tuple(words), oparg, tuple(cache_words), fused)
        outcome, outputs, branch, edge, completed = got
        where = (uop.name, [hex(w) for w in words], oparg, caches)
        self.assertEqual(ir_leaf_eval.OUTCOMES[outcome], expected.outcome, where)
        self.assertEqual(edge, expected.edge, where)
        # Read before calling a method of self: a bound method counts self
        writes = [(hex(addr), ir_leaf_eval.RealMemory().load(addr, t), after)
                  for addr, t, before, after in cx.mem.writes()]
        for addr, real, after in writes:
            self.assertEqual(real, after, (where, addr))
        if expected.outcome != "NEXT":
            if expected.outcome == "COMPLETE":
                self.assertEqual(completed, expected.completed, where)
            for w in consumed:
                ctypes.pythonapi.Py_DecRef(ctypes.py_object(object_at(w)))
            return expected.outcome
        self.assertEqual(bool(branch), expected.branch, where)
        positions, height = output_positions(part, oparg)
        self.assertEqual(len(outputs), height, where)
        for out in uop.outputs:
            if out.name == "unused":
                continue
            got_word = outputs[positions[out.name]]
            want = expected.outputs[out.name]
            if isinstance(want, ir_leaf_eval.Box):
                obj = object_at(got_word)
                self.assertEqual(type(obj).__name__, want.type, where)
                self.assertTrue(same(obj, want.value), (where, obj, want.value))
            else:
                self.assertEqual(got_word, want, (where, out.name))
            if out.ownership in ("OWNED", "ALIAS") and owns_count(got_word):
                keep.append(got_word)
            env[out.value] = got_word
        return "NEXT"

    def test_store_fast(self):
        # _SWAP_FAST stores into a home of the frame: here, this function's
        # local `scratch`
        leaf = next(u.leaf for r in self.records.values() for u in r.uops
                    if u.name == "_SWAP_FAST")
        home = LeafUopTests.test_store_fast.__code__.co_varnames.index("scratch")
        for value in (BIG_INTS[0], "x", 5, None):
            scratch = Holder()
            old = word(scratch)
            w = encode_int(value) if type(value) is int and INT_MIN <= value <= INT_MAX \
                else word(value)
            if owns_count(w):
                ctypes.pythonapi.Py_IncRef(ctypes.py_object(value))
            cx = context(frame=capi.leaf_frame())
            expected = ir_leaf_eval.evaluate(leaf, cx, {"value": w}, {}, home)
            outcome, outputs, *_ = capi.leaf_probe("_SWAP_FAST", (w,), home, (), False)
            writes = [(ir_leaf_eval.RealMemory().load(addr, t), after)
                      for addr, t, before, after in cx.mem.writes()]
            # The old value is the probe's output now, an owned count
            self.assertEqual(outputs, (old,))
            ctypes.pythonapi.Py_DecRef(ctypes.py_object(object_at(old)))
            self.assertEqual(ir_leaf_eval.OUTCOMES[outcome], expected.outcome)
            self.assertEqual(expected.outputs, {"trash": old})
            self.assertEqual(writes, [(w, w)])
            self.assertEqual((type(scratch), scratch), (type(value), value))

    def test_handlers(self):
        parts = self.uops
        for handler, (domain, make_caches, opargs) in HANDLERS.items():
            record = self.records[handler]
            self.assertTrue(all(u.leaf is not None for u in record.uops), handler)
            externals = [v.id for v in sorted((v for v in record.values if v.external >= 0),
                                              key=lambda v: v.external)]
            outcomes = set()
            for inputs in domain():
                for caches in (make_caches(inputs) if make_caches else [{}]):
                    for oparg in opargs:
                        for fused in (False, True):
                            env = dict(zip(externals, inputs))
                            keep = []
                            try:
                                for uop in record.uops:
                                    outcome = self.check_uop(uop, parts[uop.name], env,
                                                             caches, oparg, fused, keep)
                                    outcomes.add(outcome)
                                    if outcome != "NEXT":
                                        break
                            finally:
                                for w in keep:
                                    ctypes.pythonapi.Py_DecRef(ctypes.py_object(object_at(w)))
            with self.subTest(handler=handler):
                self.assertIn("NEXT", outcomes)


if __name__ == "__main__":
    unittest.main()
