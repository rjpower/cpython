"""The primitives of the register language: their models, C types, effects and the leaf IR that arms lower into, with the evaluator's memory model (Box, Context, HELPER_MODELS)."""

from dataclasses import dataclass, field
import math
import struct
from typing import Callable



# The effect flags of ir_records (CR): reference counts are not heap writes
READS_HEAP = "READS_HEAP"
WRITES_HEAP = "WRITES_HEAP"
ALLOCATES = "ALLOCATES"
MAY_RAISE = "MAY_RAISE"
MAY_RELEASE = "MAY_RELEASE"
MAY_RUN_PYTHON = "MAY_RUN_PYTHON"
EFFECTS = (READS_HEAP, WRITES_HEAP, ALLOCATES, MAY_RAISE, MAY_RELEASE, MAY_RUN_PYTHON)

NONE = frozenset()
READS = frozenset((READS_HEAP,))
WRITES = frozenset((WRITES_HEAP,))
BOXES = frozenset((ALLOCATES, MAY_RAISE))
VALUE = "value"     # PyValue or PyValueBorrow: a 64-bit word
C_TYPES = {
    "PyValue": VALUE, "PyValueBorrow": VALUE,
    "bool": "bool", "int": "i32", "unsigned int": "u32", "long": "i64",
    "unsigned long": "u64", "int64_t": "i64", "uint64_t": "u64",
    "int32_t": "i32", "uint32_t": "u32", "uint16_t": "u16", "uint8_t": "u8",
    "int8_t": "i8", "char": "i8", "uintptr_t": "u64", "intptr_t": "i64",
    "Py_ssize_t": "i64", "size_t": "u64", "double": "f64",
}
# The structs whose pointers a leaf may hold.  A pointer type is written
# "T*"; casts among them, and to and from PyObject*, change nothing.
STRUCTS = (
    "PyObject", "PyTypeObject", "PyListObject", "PyTupleObject",
    "PyDictObject", "PyDictKeysObject", "PyDictUnicodeEntry", "PyDictValues",
    "PyModuleObject", "PyFunctionObject", "PyLongObject", "_PyRangeIterObject",
    "PyCodeObject",
)
POINTER_TARGETS = STRUCTS + ("PyValue",)


def is_pointer(t: str) -> bool:
    return t.endswith("*")


def bits_of(t: str) -> int:
    if t in (VALUE, "f64", "i64", "u64") or is_pointer(t):
        return 64
    if t == "bool":
        return 1
    return int(t[1:])


def is_signed(t: str) -> bool:
    return t[0] == "i"


MASK64 = (1 << 64) - 1


def u64(x: int) -> int:
    return x & MASK64


def s64(x: int) -> int:
    x &= MASK64
    return x - (1 << 64) if x >> 63 else x


def wrap(t: str, x: int) -> int:
    "x as a value of integer type t"
    if t == "bool":
        return int(bool(x))
    n = bits_of(t)
    x &= (1 << n) - 1
    if is_signed(t) and x >> (n - 1):
        x -= 1 << n
    return x


def f64_bits(d: float) -> int:
    return struct.unpack("<Q", struct.pack("<d", d))[0]


def bits_f64(w: int) -> float:
    return struct.unpack("<d", struct.pack("<Q", u64(w)))[0]
# The addresses of static objects that a leaf names
SYMBOLS = (
    "_Py_TrueStruct", "_Py_FalseStruct", "_Py_NoneStruct",
    "PyLong_Type", "PyFloat_Type", "PyBool_Type", "PyList_Type",
    "PyTuple_Type", "PyDict_Type", "PyModule_Type", "PyFunction_Type",
    "PyUnicode_Type", "PyRangeIter_Type", "PyType_Type",
)


@dataclass(frozen=True)
class Field:
    "A field that a leaf may load or store: struct.name, of C type ctype"
    struct: str
    name: str
    type: str           # a subset type


FIELDS = {f"{f.struct}.{f.name}": f for f in (
    Field("PyObject", "ob_refcnt", "i64"),
    Field("PyObject", "ob_type", "PyTypeObject*"),
    Field("PyVarObject", "ob_size", "i64"),
    Field("PyTypeObject", "tp_version_tag", "u32"),
    Field("PyTypeObject", "tp_basicsize", "i64"),
    Field("PyTypeObject", "tp_flags", "u64"),
    Field("PyListObject", "ob_item", "PyValue*"),
    Field("PyListObject", "allocated", "i64"),
    Field("PyTupleObject", "ob_item", "PyValue*"),   # an array in place
    Field("PyDictObject", "ma_keys", "PyDictKeysObject*"),
    Field("PyDictKeysObject", "dk_version", "u32"),
    Field("PyDictKeysObject", "dk_nentries", "i64"),
    Field("PyDictKeysObject", "dk_log2_index_bytes", "u8"),
    Field("PyDictKeysObject", "dk_indices", "i8*"),  # an array in place
    Field("PyDictUnicodeEntry", "me_value", VALUE),
    Field("PyDictValues", "capacity", "u8"),
    Field("PyDictValues", "size", "u8"),
    Field("PyDictValues", "valid", "u8"),
    Field("PyDictValues", "shape", "u8"),
    Field("PyDictValues", "values", "PyValue*"),     # an array in place
    Field("PyModuleObject", "md_dict", "PyObject*"),
    Field("PyFunctionObject", "func_version", "u32"),
    Field("_PyRangeIterObject", "start", "i64"),
    Field("_PyRangeIterObject", "step", "i64"),
    Field("_PyRangeIterObject", "len", "i64"),
    Field("PyLongObject", "long_value.lv_tag", "u64"),
    Field("PyASCIIObject", "length", "i64"),
    Field("_PyInterpreterFrame", "f_globals", "PyObject*"),
    Field("_PyInterpreterFrame", "f_builtins", "PyObject*"),
    Field("_PyInterpreterFrame", "f_executable", "PyCodeObject*"),
    Field("_PyInterpreterFrame", "base", "PyValue*"),
    Field("PyCodeObject", "co_consts", "PyObject*"),
    Field("PyThreadState", "eval_breaker", "u64"),
    Field("PyCodeObject", "_co_instrumentation_version", "u64"),
)}










def _is_null(cx, w):
    return w == 0


def _is_pointer_space(cx, w):
    return u64(w) < cx.k("Py_PTR_LIMIT")


def _is_inline(cx, w):
    return u64(w) >= cx.k("Py_DOUBLE_OFFSET")


def _is_inline_int(cx, w):
    return u64(w) >= u64(cx.k("Py_INT_BASE"))


def _is_inline_float(cx, w):
    return (u64(w - cx.k("Py_DOUBLE_OFFSET")) <
            u64(cx.k("Py_INT_BASE")) - cx.k("Py_DOUBLE_OFFSET"))


def _is_tagged_index(cx, w):
    return (u64(w - cx.k("Py_INDEX_BASE")) <
            cx.k("Py_DOUBLE_OFFSET") - cx.k("Py_INDEX_BASE"))


def _object(cx, w):
    "The object of a pointer-space word: the no-count tag cleared"
    return u64(w) & ~cx.k("Py_TAG_REFCNT")


def _type_of(cx, w):
    return cx.load(_object(cx, w) + cx.off("PyObject.ob_type"), "PyTypeObject*")


def _int_scaled(cx, w):
    return s64(w << cx.k("Py_INT_SHIFT"))


def _from_int_scaled(cx, s):
    return (u64(s) >> cx.k("Py_INT_SHIFT")) | u64(cx.k("Py_INT_BASE"))


def _inline_int64(cx, w):
    return _int_scaled(cx, w) >> cx.k("Py_INT_SHIFT")


def _fits_inline(cx, i):
    return cx.k("Py_INT_MIN") <= i <= cx.k("Py_INT_MAX")


def _from_inline_int(cx, i):
    base = u64(cx.k("Py_INT_BASE"))
    return base | (u64(i) & ~base & MASK64)


def _inline_float(cx, w):
    return bits_f64(w - cx.k("Py_DOUBLE_OFFSET"))


def _from_double_not_nan(cx, d):
    return u64(f64_bits(d) + cx.k("Py_DOUBLE_OFFSET"))


def _number_as_double(cx, w):
    if _is_inline_float(cx, w):
        return _inline_float(cx, w)
    return float(_inline_int64(cx, w))


def _float_operands_check(cx, a, b):
    if _is_inline_float(cx, a):
        return _is_inline_float(cx, b) or _is_inline_int(cx, b)
    return _is_inline_int(cx, a) and _is_inline_float(cx, b)


def _float_operand_values(cx, a, b):
    if not _float_operands_check(cx, a, b):
        return False, None, None     # the out-parameters are not written
    return True, _number_as_double(cx, a), _number_as_double(cx, b)


def _from_int64(cx, i):
    if _fits_inline(cx, i):
        return _from_inline_int(cx, i)
    return cx.call("_PyValue_BoxInt64", i)


def _add_checked_scaled(cx, a, b):
    """The scaled sum of two int51s, or on 64-bit overflow their exact sum.
    Both results are written, so the overflow case has a payload."""
    exact = a + b
    if -(1 << 63) <= exact < (1 << 63):
        return False, exact
    shift = cx.k("Py_INT_SHIFT")
    return True, (a >> shift) + (b >> shift)


def _from_double(cx, d):
    if not math.isnan(d):
        return _from_double_not_nan(cx, d)
    return cx.call("_PyValue_BoxDouble", d)


def _tag_index(cx, i):
    return cx.k("Py_INDEX_BASE") | (u64(i) & (cx.k("Py_PTR_LIMIT") - 1))


def _untag_index(cx, w):
    x = u64(w << 16)
    return s64(x) >> 16


def _comparison_bit(cx, x, y):
    return 1 << (2 * (x >= y) + (x <= y))


def _owns_count(cx, w):
    return w != 0 and (u64(w) & cx.k("Py_OWNED_MASK")) == 0


def _dup(cx, w):
    if _owns_count(cx, w):
        addr = w + cx.off("PyObject.ob_refcnt")
        cx.store(addr, "i64", cx.load(addr, "i64") + 1)
    return w


def _close_with(dealloc):
    def close(cx, w, destructor=None):
        if _owns_count(cx, w):
            addr = w + cx.off("PyObject.ob_refcnt")
            count = cx.load(addr, "i64") - 1
            cx.store(addr, "i64", count)
            if count == 0:
                cx.call(destructor or dealloc, w)
    return close


def _check_exact(symbol):
    return lambda cx, o: cx.load(o + cx.off("PyObject.ob_type"), "PyTypeObject*") == cx.addr(symbol)


def _value_check_exact(symbol):
    "PyValue_*Check: pointer space and an object of exactly this type"
    return lambda cx, w: (_is_pointer_space(cx, w) and w != 0 and
                          _type_of(cx, w) == cx.addr(symbol))


def _inline_values(cx, o):
    tp = cx.load(o + cx.off("PyObject.ob_type"), "PyTypeObject*")
    return o + cx.load(tp + cx.off("PyTypeObject.tp_basicsize"), "i64")


def _list_item(cx, o, i):
    items = cx.load(o + cx.off("PyListObject.ob_item"), "PyValue*")
    return cx.load(items + i * cx.size("PyValue"), VALUE)


def _tuple_item(cx, o, i):
    return cx.load(o + cx.off("PyTupleObject.ob_item") + i * cx.size("PyValue"), VALUE)


def _value_slot(cx, o, offset):
    return o + offset


def _slot_exchange(cx, slot, w):
    old = cx.load(slot, VALUE)
    cx.store(slot, VALUE, w)
    return old


def _slot_fill(cx, slot, w):
    cx.store(slot, VALUE, w)


def _unicode_entries(cx, keys):
    log2 = cx.load(keys + cx.off("PyDictKeysObject.dk_log2_index_bytes"), "u8")
    return keys + cx.off("PyDictKeysObject.dk_indices") + (1 << log2)


def _add_to_insertion_order(cx, values, ix):
    size = cx.load(values + cx.off("PyDictValues.size"), "u8")
    capacity = cx.load(values + cx.off("PyDictValues.capacity"), "u8")
    array = values + cx.off("PyDictValues.values") + capacity * cx.size("PyValue")
    cx.store(array + size, "u8", ix)
    cx.store(values + cx.off("PyDictValues.size"), "u8", size + 1)


def _set_shape(cx, values, shape):
    cx.store(values + cx.off("PyDictValues.shape"), "u8", shape)


def _long_is_zero(cx, o):
    # SIGN_MASK is 3 and SIGN_ZERO 1 (Include/internal/pycore_long.h)
    return (cx.load(o + cx.off("PyLongObject.long_value.lv_tag"), "u64") & 3) == 1


def _frame_field(name, t):
    return lambda cx: cx.load(cx.frame + cx.off(f"_PyInterpreterFrame.{name}"), t)


def _getlocal(cx, i):
    base = cx.load(cx.frame + cx.off("_PyInterpreterFrame.base"), "PyValue*")
    return cx.load(base + i * cx.size("PyValue"), VALUE)


def _frame_code_instrumentation_version(cx):
    code = cx.load(cx.frame + cx.off("_PyInterpreterFrame.f_executable"), "PyCodeObject*")
    return cx.load(code + cx.off("PyCodeObject._co_instrumentation_version"), "u64")


def _frame_co_consts(cx):
    code = cx.load(cx.frame + cx.off("_PyInterpreterFrame.f_executable"), "PyCodeObject*")
    return cx.load(code + cx.off("PyCodeObject.co_consts"), "PyObject*")






@dataclass(frozen=True)
class Primitive:
    name: str
    params: tuple[str, ...]
    results: tuple[str, ...]
    header: str                 # where the C definition is
    spec: Callable
    effects: frozenset[str] = NONE
    pre: Callable | None = None # the precondition of its arguments
    slow: str = ""              # the helper its slow path calls
    frame: bool = False         # reads the frame, an implicit operand
    thread: bool = False        # takes the thread state, written `tstate`
    doc: str = ""


V, B, P = VALUE, "bool", "PyObject*"
VALUE_H = "Include/internal/pycore_value.h"
PYVALUE_H = "Include/internal/pycore_pyvalue.h"
OBJECT_H = "Include/object.h"
DICT_H = "Include/internal/pycore_dict.h"
MACROS_H = "Python/ceval_macros.h"


def _p(name, params, results, header, spec, effects=NONE, **kw):
    return Primitive(name, tuple(params), tuple(results), header, spec, effects, **kw)


def _inline_int_pre(cx, *ws):
    return all(_is_inline_int(cx, w) for w in ws)


PRIMITIVES = {p.name: p for p in (
    # Tag tests: bits in, bool out
    _p("PyValue_IsNull", [V], [B], VALUE_H, _is_null),
    _p("PyValue_IsPointerSpace", [V], [B], VALUE_H, _is_pointer_space),
    _p("PyValue_IsObject", [V], [B], PYVALUE_H,
       lambda cx, w: 2 < u64(w) < cx.k("Py_PTR_LIMIT") and not w & 2,
       doc="an object word: pointer space, not NULL, not a wrapped pointer"),
    _p("PyValue_IsInline", [V], [B], VALUE_H, _is_inline),
    _p("PyValue_IsInlineInt", [V], [B], VALUE_H, _is_inline_int),
    _p("PyValue_IsInlineFloat", [V], [B], VALUE_H, _is_inline_float),
    _p("PyValue_BothInlineInt", [V, V], [B], VALUE_H,
       lambda cx, a, b: u64(a & b) >= u64(cx.k("Py_INT_BASE"))),
    _p("PyValue_IsTaggedIndex", [V], [B], VALUE_H, _is_tagged_index),
    _p("PyValue_Is", [V, V], [B], VALUE_H,
       lambda cx, a, b: ((a ^ b) & ~int(_is_pointer_space(cx, a)) & MASK64) == 0),
    _p("_PyValue_NamesObject", [V, P], [B], VALUE_H,
       lambda cx, w, p: (u64(w) & ~1) == u64(p),
       doc="the word is the object at p, whatever its no-count tag"),
    _p("PyValue_IsTrue", [V], [B], VALUE_H,
       lambda cx, w: w == cx.addr("_Py_TrueStruct") | cx.k("Py_TAG_REFCNT")),
    _p("PyValue_IsFalse", [V], [B], VALUE_H,
       lambda cx, w: w == cx.addr("_Py_FalseStruct") | cx.k("Py_TAG_REFCNT")),
    _p("PyValue_IsNone", [V], [B], VALUE_H,
       lambda cx, w: w == cx.addr("_Py_NoneStruct") | cx.k("Py_TAG_REFCNT")),
    _p("PyValue_BoolCheck", [V], [B], VALUE_H, _value_check_exact("PyBool_Type"), READS,
       pre=lambda cx, w: w != 0),
    _p("PyValue_LongCheckExact", [V], [B], VALUE_H,
       lambda cx, w: _is_inline_int(cx, w) or _value_check_exact("PyLong_Type")(cx, w),
       READS, pre=lambda cx, w: w != 0),
    # Inline numbers
    _p("_PyValue_IntScaled", [V], ["i64"], VALUE_H, _int_scaled, pre=_inline_int_pre),
    _p("_PyValue_FromIntScaled", ["i64"], [V], VALUE_H, _from_int_scaled,
       pre=lambda cx, s: s % (1 << cx.k("Py_INT_SHIFT")) == 0),
    _p("PyValue_InlineInt64", [V], ["i64"], VALUE_H, _inline_int64, pre=_inline_int_pre),
    _p("PyValue_FromInlineInt", ["i64"], [V], VALUE_H, _from_inline_int,
       pre=lambda cx, i: _fits_inline(cx, i)),
    _p("PyValue_InlineFloat", [V], ["f64"], VALUE_H, _inline_float,
       pre=lambda cx, w: _is_inline_float(cx, w)),
    _p("PyValue_FromDoubleNotNaN", ["f64"], [V], VALUE_H, _from_double_not_nan,
       pre=lambda cx, d: not math.isnan(d)),
    _p("PyValue_FloatOperandValues", [V, V], [B, "f64", "f64"], VALUE_H,
       _float_operand_values,
       doc="the bool result and the two out-parameters, written only if true"),
    _p("_PyValue_AddCheckedScaled", ["i64", "i64"], [B, "i64"], VALUE_H,
       _add_checked_scaled,
       pre=lambda cx, a, b: all(x % (1 << cx.k("Py_INT_SHIFT")) == 0 for x in (a, b)),
       doc="overflowed, and the scaled sum or, on overflow, the exact int51 sum"),
    _p("PyValue_FromInt64", ["i64"], [V], VALUE_H, _from_int64, BOXES,
       slow="_PyValue_BoxInt64"),
    _p("PyValue_FromDouble", ["f64"], [V], VALUE_H, _from_double, BOXES,
       slow="_PyValue_BoxDouble"),
    _p("PyValue_TagIndex", ["i64"], [V], VALUE_H, _tag_index,
       pre=lambda cx, i: -1 <= i < (1 << 47)),
    _p("PyValue_UntagIndex", [V], ["i64"], VALUE_H, _untag_index,
       pre=lambda cx, w: _is_tagged_index(cx, w)),
    _p("PyValue_IncrementTaggedIndexNoOverflow", [V], [V], VALUE_H,
       lambda cx, w: w + 1,
       pre=lambda cx, w: _is_tagged_index(cx, w) and 0 <= _untag_index(cx, w) < (1 << 47) - 1),
    _p("COMPARISON_BIT", ["i64", "i64"], ["i32"], "Include/internal/pycore_code.h",
       _comparison_bit, doc="also over two f64; NaN compares give 1"),
    # Objects: a word's object, types and fields
    _p("PyValue_AsKnownObjectBorrow", [V], [P], VALUE_H, _object,
       pre=lambda cx, w: w != 0 and _is_pointer_space(cx, w)),
    _p("_PyValue_AsKnownObjectBorrowPointer", [V], [P], VALUE_H, _object,
       pre=lambda cx, w: w != 0 and _is_pointer_space(cx, w)),
    _p("Py_TYPE", [P], ["PyTypeObject*"], OBJECT_H,
       lambda cx, o: cx.load(o + cx.off("PyObject.ob_type"), "PyTypeObject*"), READS),
    _p("PyList_CheckExact", [P], [B], "Include/listobject.h",
       _check_exact("PyList_Type"), READS),
    _p("PyTuple_CheckExact", [P], [B], "Include/tupleobject.h",
       _check_exact("PyTuple_Type"), READS),
    _p("PyDict_CheckExact", [P], [B], "Include/dictobject.h",
       _check_exact("PyDict_Type"), READS),
    _p("PyModule_CheckExact", [P], [B], "Include/moduleobject.h",
       _check_exact("PyModule_Type"), READS),
    _p("PyUnicode_CheckExact", [P], [B], "Include/unicodeobject.h",
       _check_exact("PyUnicode_Type"), READS),
    _p("PyFunction_Check", [P], [B], "Include/cpython/funcobject.h",
       _check_exact("PyFunction_Type"), READS),
    _p("PyList_GET_SIZE", [P], ["i64"], "Include/cpython/listobject.h",
       lambda cx, o: cx.load(o + cx.off("PyVarObject.ob_size"), "i64"), READS),
    _p("PyTuple_GET_SIZE", [P], ["i64"], "Include/cpython/tupleobject.h",
       lambda cx, o: cx.load(o + cx.off("PyVarObject.ob_size"), "i64"), READS),
    _p("PyList_GET_VALUE", [P, "i64"], [V], "Include/internal/pycore_list.h",
       _list_item, READS),
    _p("PyTuple_GET_VALUE", [P, "i64"], [V], "Include/internal/pycore_tuple.h",
       _tuple_item, READS),
    _p("_PyList_ITEMS", [P], ["PyValue*"], "Include/internal/pycore_list.h",
       lambda cx, o: cx.load(o + cx.off("PyListObject.ob_item"), "PyValue*"), READS),
    _p("_PyTuple_ITEMS", [P], ["PyValue*"], "Include/internal/pycore_tuple.h",
       lambda cx, o: o + cx.off("PyTupleObject.ob_item"), READS),
    _p("_PyObject_InlineValues", [P], ["PyDictValues*"], "Include/internal/pycore_object.h",
       _inline_values, READS),
    _p("_PyObject_Shape", [P], ["u32"], DICT_H,
       lambda cx, o: cx.load(_inline_values(cx, o) + cx.off("PyDictValues.shape"), "u8"),
       READS),
    _p("_PyObject_ValueSlot", [P, "u64"], ["PyValue*"], "Include/internal/pycore_object.h",
       _value_slot, doc="the value slot at a byte offset inside an object"),
    _p("_PyValueSlot_Exchange", ["PyValue*", V], [V], "Include/internal/pycore_object.h",
       _slot_exchange, frozenset((READS_HEAP, WRITES_HEAP)),
       doc="the slot's value out, the word in"),
    _p("_PyValueSlot_ExchangeFilled", ["PyValue*", V], [V], "Include/internal/pycore_object.h",
       _slot_exchange, frozenset((READS_HEAP, WRITES_HEAP)),
       pre=lambda cx, slot, w: cx.load(slot, VALUE) != 0,
       doc="_PyValueSlot_Exchange for a slot that holds a value"),
    _p("_PyValueSlot_Fill", ["PyValue*", V], [], "Include/internal/pycore_object.h",
       _slot_fill, WRITES, pre=lambda cx, slot, w: cx.load(slot, VALUE) == 0,
       doc="the word into a slot that holds NULL"),
    _p("_PyObject_ShapeReadDup", [P, "u32"], [V], DICT_H,
       lambda cx, o, off: _dup(cx, cx.load(o + off, VALUE)), READS,
       doc="the filled slot at a byte offset, with a count of its own"),
    _p("DK_UNICODE_ENTRIES", ["PyDictKeysObject*"], ["PyDictUnicodeEntry*"], DICT_H,
       _unicode_entries, READS),
    _p("_PyDictValues_AddToInsertionOrder", ["PyDictValues*", "i64"], [], DICT_H,
       _add_to_insertion_order, frozenset((READS_HEAP, WRITES_HEAP))),
    _p("_PyDictValues_SetShape", ["PyDictValues*", "u8"], [], DICT_H, _set_shape, WRITES),
    _p("_PyLong_IsZero", ["PyLongObject*"], [B], "Include/internal/pycore_long.h",
       _long_is_zero, READS),
    _p("_PyValueGuarded_Load", [V], [V], VALUE_H, lambda cx, w: w,
       doc="a feedback word, a guarded borrow; the identity on its bits"),
    _p("_PyValue_CodeConstant", [V], [V], VALUE_H, lambda cx, w: w,
       doc="an item of the code's constants; the identity on its bits"),
    _p("_PyValue_BoundLocal", [V], [V], VALUE_H, lambda cx, w: w,
       doc="a local the compiler shows bound; the identity on its bits"),
    _p("_PyValue_CellLocal", [V], [V], VALUE_H, lambda cx, w: w,
       doc="the home of a cell or free variable; the identity on its bits"),
    _p("_PyValue_CommonConstant", [V], [V], VALUE_H, lambda cx, w: w,
       doc="an interpreter's common constant; the identity on its bits"),
    # The frame, an implicit operand
    _p("_PyValue_PublishedItem", [V], [V], VALUE_H, lambda cx, w: w,
       doc="an item of a published list or tuple; the identity on its bits"),
    # The frame, an implicit operand
    _p("GLOBALS", [], [P], MACROS_H, _frame_field("f_globals", P), READS, frame=True),
    _p("BUILTINS", [], [P], MACROS_H, _frame_field("f_builtins", P), READS, frame=True),
    _p("FRAME_CO_CONSTS", [], [P], MACROS_H, _frame_co_consts, READS, frame=True),
    _p("GETLOCAL", ["i32"], [V], MACROS_H, _getlocal, frame=True,
       doc="a home of the frame: an operand, not heap (CR)"),
    _p("IR_SLOTS", [], ["PyValue*"], MACROS_H,
       lambda cx: cx.load(cx.frame + cx.off("_PyInterpreterFrame.base"), "PyValue*"),
       frame=True, doc="the frame's homes; `GETLOCAL(i) = v` stores to IR_SLOTS()[i]"),
    _p("_Py_EvalBreakerHasEvents", [], [B], MACROS_H,
       lambda cx: (cx.load(cx.tstate + cx.off("PyThreadState.eval_breaker"), "u64")
                   & cx.k("_PY_EVAL_EVENTS_MASK")) != 0,
       READS, thread=True, doc="an event for _Py_HandlePending(): the periodic check"),
    _p("_Py_EvalBreaker", [], ["u64"], MACROS_H,
       lambda cx: cx.load(cx.tstate + cx.off("PyThreadState.eval_breaker"), "u64"),
       READS, thread=True, doc="the eval breaker: event bits and instrumentation version"),
    _p("FRAME_CODE_INSTRUMENTATION_VERSION", [], ["u64"], MACROS_H,
       _frame_code_instrumentation_version, READS, frame=True,
       doc="the instrumentation version of the frame's code"),
    # Ownership: counts are not heap writes (CR)
    _p("PyValue_DUP", [V], [V], VALUE_H, _dup, pre=lambda cx, w: w != 0),
    _p("PyValue_XDUP", [V], [V], VALUE_H, _dup),
    _p("PyValue_CLOSE", [V], [], VALUE_H, _close_with("_Py_Dealloc"),
       frozenset((MAY_RELEASE,)), pre=lambda cx, w: w != 0, slow="_Py_Dealloc"),
    _p("PyValue_XCLOSE", [V], [], VALUE_H, _close_with("_Py_Dealloc"),
       frozenset((MAY_RELEASE,)), slow="_Py_Dealloc"),
    _p("PyValue_CLOSE_SPECIALIZED", [V, "destructor"], [], VALUE_H,
       _close_with("_Py_Dealloc"), WRITES, pre=lambda cx, w, d=None: w != 0,
       doc="the destructor of an exact type, which runs no Python"),
)}


# Constant words, as (symbol, tag)
VALUE_CONSTANTS = {
    "PyValue_True": ("_Py_TrueStruct", 1), "PyValue_False": ("_Py_FalseStruct", 1),
    "PyValue_None": ("_Py_NoneStruct", 1), "PyValue_NULL": (None, 0),
}


@dataclass(frozen=True)
class Helper:
    "An out-of-line function that a leaf may call"
    name: str
    params: tuple[str, ...]
    results: tuple[str, ...]
    effects: frozenset[str]
    doc: str = ""
    thread: bool = False        # takes the thread state, written `tstate`


HELPERS = {h.name: h for h in (
    Helper("_PyValue_BoxInt64", ("i64",), (V,), frozenset((ALLOCATES, MAY_RAISE)),
           "a new int outside the inline range"),
    Helper("_PyValue_BoxDouble", ("f64",), (V,), frozenset((ALLOCATES, MAY_RAISE)),
           "a new float for a NaN"),
    Helper("_PyValue_MultiplyInt64", ("i64", "i64"), (V,), frozenset((ALLOCATES, MAY_RAISE))),
    Helper("_PyValue_LshiftInt64", ("i64", "i64"), (V,), frozenset((ALLOCATES, MAY_RAISE))),
    Helper("_Py_Dealloc", (P,), (), frozenset((MAY_RELEASE,)), "the last count's release"),
    Helper("_PyLong_ExactDealloc", (P,), (), WRITES, "an exact int's release"),
    Helper("_PyFloat_ExactDealloc", (P,), (), WRITES, "an exact float's release"),
    Helper("_Py_HandlePending", (), ("i32",), frozenset(EFFECTS),
           "the events of the eval breaker: signals, GC, a drop request, ...",
           thread=True),
    # The completions that a `complete` exit calls (IREmitter.complete_step):
    # each drains the input window, which may release the iterator
    *(Helper(name, (V,), (), frozenset(EFFECTS), f"the completion of {kind}")
      for kind, name in (("_PY_IR_FOR_ITER", "_PyEval_CompleteForIter"),)),
)}









@dataclass
class Op:
    kind: str                   # see OP_KINDS
    results: tuple[int, ...]
    args: tuple[int, ...] = ()
    attr: object = None         # the op's constant part: a name, a field, ...
    site: object = None         # the token it comes from (guards: the site)

    def text(self, types: dict[int, str]) -> str:
        res = ", ".join(f"%{r}:{types[r]}" for r in self.results)
        args = ", ".join(f"%{a}" for a in self.args)
        attr = "" if self.attr is None else f" {_attr_text(self.attr)}"
        head = f"{res} = " if res else ""
        return f"{head}{self.kind}{attr} {args}".rstrip()


def _attr_text(attr) -> str:
    if isinstance(attr, tuple):
        return " ".join(_attr_text(a) for a in attr)
    if isinstance(attr, float):
        return repr(attr)
    return str(attr)


@dataclass
class Block:
    id: int
    params: list[int] = field(default_factory=list)
    ops: list[Op] = field(default_factory=list)
    # ("br", cond, then, else) | ("jmp", target, args) | ("next", branch)
    # | ("raise",) | ("exit", kind, site) | ("complete", kind, value)
    term: tuple | None = None


@dataclass
class LeafBody:
    uop: str
    blocks: list[Block]
    types: dict[int, str]
    inputs: tuple[str, ...]
    outputs: tuple[str, ...]
    effects: frozenset[str]
    helpers: tuple[str, ...]        # helpers it may call, primitives' slow paths included

    def ops(self):
        for block in self.blocks:
            yield from block.ops

    def guards(self) -> list[Op]:
        return [op for op in self.ops() if op.kind == "exit_if"]

    def dump(self, guards=()) -> list[str]:
        """The body as text.  guards are the uop's record guards: each exit
        is printed with the facts of the guard at its site."""
        by_site = {g.site.begin: g.facts for g in guards if g.site is not None}

        def facts(op):
            site = op.site[0] if isinstance(op.site, tuple) else op.site
            return by_site.get(site.begin) if site is not None else None
        lines = []
        for block in self.blocks:
            params = ", ".join(f"%{p}:{self.types[p]}" for p in block.params)
            lines.append(f"b{block.id}({params}):" if params else f"b{block.id}:")
            for op in block.ops:
                text = op.text(self.types)
                if op.kind == "exit_if":
                    found = facts(op)
                    if found:
                        text += " [" + ", ".join(str(f) for f in found) + "]"
                lines.append("  " + text)
            lines.append("  " + _term_text(block.term))
        return lines


def _term_text(term: tuple) -> str:
    kind = term[0]
    if kind == "br":
        return f"br %{term[1]}, b{term[2]}, b{term[3]}"
    if kind == "jmp":
        args = ", ".join(f"%{a}" for a in term[2])
        return f"jmp b{term[1]}({args})" if args else f"jmp b{term[1]}"
    if kind == "next":
        return "next" if term[1] is None else f"next branch_if %{term[1]}"
    if kind == "exit":
        return f"exit {term[1]}"
    if kind == "complete":
        return f"complete {term[1]} %{term[2]}"
    return kind


@dataclass(frozen=True)
class Layout:
    "The numbers that the specs name: ir_leaf.CONSTANTS, FIELDS, SIZES, SYMBOLS"
    constants: dict[str, int]
    offsets: dict[str, int]
    sizes: dict[str, int]
    symbols: dict[str, int]

    @classmethod
    def from_c(cls, capi) -> "Layout":
        "The layout of the running interpreter, from _testinternalcapi"
        return cls(*capi.leaf_layout())


class Box(int):
    """A value that a helper made: a new object, which has no address the
    evaluator knows.  Its word is a fake address in pointer space, owned and
    mortal, so that the tag tests see an object; any load through it fails.
    type and value say what object it is."""

    _next = 0x1000

    def __new__(cls, type: str, value):
        Box._next += 0x10
        self = super().__new__(cls, Box._next)
        self.type = type
        self.value = value
        return self

    def __repr__(self) -> str:
        return f"Box({self.type}, {self.value!r})"


def _box_int64(cx, i):
    return Box("int", i)


def _box_double(cx, d):
    return Box("float", d)


def _multiply_int64(cx, a, b):
    return Box("int", a * b)


def _lshift_int64(cx, v, n):
    return Box("int", v << n)


# The language result of each helper, which is all a test of a uop can
# compare: a box's address is the allocator's.  A release is recorded.
HELPER_MODELS = {
    "_PyValue_BoxInt64": _box_int64,
    "_PyValue_BoxDouble": _box_double,
    "_PyValue_MultiplyInt64": _multiply_int64,
    "_PyValue_LshiftInt64": _lshift_int64,
    # The tests run it with no event pending, or with one that raises nothing
    "_Py_HandlePending": lambda cx: 0,
}


@dataclass
class Context:
    "What a spec reads: the layout, memory, the frame and the helpers"
    layout: Layout
    mem: object
    frame: int = 0
    tstate: int = 0
    calls: list = field(default_factory=list)

    def k(self, name: str) -> int:
        return self.layout.constants[name]

    def off(self, name: str) -> int:
        return self.layout.offsets[name]

    def size(self, name: str) -> int:
        return self.layout.sizes[name]

    def addr(self, name: str) -> int:
        return self.layout.symbols[name]

    def load(self, addr: int, t: str):
        if isinstance(addr, Box):
            raise MemoryError(f"load through {addr!r}")
        return self.mem.load(addr, t)

    def store(self, addr: int, t: str, x) -> None:
        if isinstance(addr, Box):
            raise MemoryError(f"store through {addr!r}")
        self.mem.store(addr, t, x)

    def call(self, name: str, *args):
        self.calls.append((name, args))
        model = HELPER_MODELS.get(name)
        return model(self, *args) if model is not None else None
