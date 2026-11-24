/* The value primitives of leaf uops (Tools/cases_generator/ir_leaf.py),
 * called through their C definitions, so that Lib/test/test_ir_leaf.py can
 * check each primitive's spec against them on generated values. */

#include "parts.h"

#include "pycore_ceval.h"         // for ceval_macros.h
#include "pycore_code.h"          // COMPARISON_BIT()
#include "pycore_emscripten_signal.h" // for ceval_macros.h
#include "pycore_floatobject.h"   // _PyFloat_ExactDealloc()
#include "pycore_dict.h"          // _PyObject_ShapeReadDup(), DK_UNICODE_ENTRIES()
#include "pycore_frame.h"         // _PyInterpreterFrame
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_list.h"          // PyList_GET_VALUE()
#include "pycore_long.h"          // _PyLong_IsZero()
#include "pycore_object.h"        // _PyObject_ValueSlot()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()
#include "pycore_value.h"
/* The evaluator's macros give the frame primitives their definitions; the
   header's out-of-line unpack helpers have no use here. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "../../Python/ceval_macros.h" // GLOBALS(), GETLOCAL()
#pragma GCC diagnostic pop

#include <stddef.h>               // offsetof


/* Arguments: words, integers and addresses are Python ints (taken modulo
 * 2**64), doubles are floats. */
static uint64_t
arg_word(PyObject *const *args, int i)
{
    return (uint64_t)PyLong_AsUnsignedLongLongMask(args[i]);
}

#define WORD(i) ((uint64_t)arg_word(args, (i)))
#define INT(i) ((int64_t)arg_word(args, (i)))
#define VAL(i) ((PyValue){ .bits = (uintptr_t)WORD(i) })
#define PTR(T, i) ((T *)(uintptr_t)WORD(i))
#define DBL(i) PyFloat_AsDouble(args[i])

#define RET_WORD(x) return PyLong_FromUnsignedLongLong((unsigned long long)(x))
#define RET_INT(x) return PyLong_FromLongLong((long long)(x))
#define RET_BOOL(x) return PyBool_FromLong((long)(x))
#define RET_VAL(v) RET_WORD((v).bits)

typedef PyObject *(*leaf_fn)(PyObject *const *args);

#define PREDICATE(NAME) \
    static PyObject *lp_##NAME(PyObject *const *args) { RET_BOOL(NAME(VAL(0))); }
#define PREDICATE2(NAME) \
    static PyObject *lp_##NAME(PyObject *const *args) { RET_BOOL(NAME(VAL(0), VAL(1))); }
#define OBJECT_PREDICATE(NAME) \
    static PyObject *lp_##NAME(PyObject *const *args) { RET_BOOL(NAME(PTR(PyObject, 0))); }

PREDICATE(PyValue_IsNull)
PREDICATE(PyValue_IsPointerSpace)
PREDICATE(PyValue_IsObject)
PREDICATE(PyValue_IsInline)
PREDICATE(PyValue_IsInlineInt)
PREDICATE(PyValue_IsInlineFloat)
PREDICATE2(PyValue_BothInlineInt)
PREDICATE(PyValue_IsTaggedIndex)
PREDICATE2(PyValue_Is)
static PyObject *lp__PyValue_NamesObject(PyObject *const *args) { RET_BOOL(_PyValue_NamesObject(VAL(0), PTR(PyObject, 1))); }
PREDICATE(PyValue_IsTrue)
PREDICATE(PyValue_IsFalse)
PREDICATE(PyValue_IsNone)
PREDICATE(PyValue_BoolCheck)
PREDICATE(PyValue_LongCheckExact)
OBJECT_PREDICATE(PyList_CheckExact)
OBJECT_PREDICATE(PyTuple_CheckExact)
OBJECT_PREDICATE(PyDict_CheckExact)
OBJECT_PREDICATE(PyModule_CheckExact)
OBJECT_PREDICATE(PyUnicode_CheckExact)
OBJECT_PREDICATE(PyFunction_Check)

static PyObject *lp__PyValue_IntScaled(PyObject *const *args) { RET_INT(_PyValue_IntScaled(VAL(0))); }
static PyObject *lp__PyValue_FromIntScaled(PyObject *const *args) { RET_VAL(_PyValue_FromIntScaled(INT(0))); }
static PyObject *lp_PyValue_InlineInt64(PyObject *const *args) { RET_INT(PyValue_InlineInt64(VAL(0))); }
static PyObject *lp_PyValue_FromInlineInt(PyObject *const *args) { RET_VAL(PyValue_FromInlineInt(INT(0))); }
static PyObject *lp_PyValue_InlineFloat(PyObject *const *args) { return PyFloat_FromDouble(PyValue_InlineFloat(VAL(0))); }
static PyObject *lp_PyValue_FromDoubleNotNaN(PyObject *const *args) { RET_VAL(PyValue_FromDoubleNotNaN(DBL(0))); }
static PyObject *lp_PyValue_TagIndex(PyObject *const *args) { RET_VAL(PyValue_TagIndex((intptr_t)INT(0))); }
static PyObject *lp_PyValue_UntagIndex(PyObject *const *args) { RET_INT(PyValue_UntagIndex(VAL(0))); }
static PyObject *lp_PyValue_IncrementTaggedIndexNoOverflow(PyObject *const *args) { RET_VAL(PyValue_IncrementTaggedIndexNoOverflow(VAL(0))); }

static PyObject *
lp_PyValue_FloatOperandValues(PyObject *const *args)
{
    double a, b;
    if (!PyValue_FloatOperandValues(VAL(0), VAL(1), &a, &b)) {
        return Py_BuildValue("(OOO)", Py_False, Py_None, Py_None);
    }
    return Py_BuildValue("(Odd)", Py_True, a, b);
}

/* The constructors with a slow path return a new value: an inline word is
   returned as a word, a box as its object (the caller owns it). */
static PyObject *
value_result(PyValue v)
{
    if (PyValue_IsNull(v)) {
        return NULL;
    }
    if (PyValue_IsPointerSpace(v)) {
        return _PyValue_AsObjectStealAt(v, BOX_SITE_TEST);
    }
    RET_VAL(v);
}

static PyObject *lp_PyValue_FromInt64(PyObject *const *args) { return value_result(PyValue_FromInt64(INT(0))); }
static PyObject *lp_PyValue_FromDouble(PyObject *const *args) { return value_result(PyValue_FromDouble(DBL(0))); }

static PyObject *
lp_COMPARISON_BIT(PyObject *const *args)
{
    if (PyFloat_Check(args[0])) {
        RET_INT(COMPARISON_BIT(DBL(0), DBL(1)));
    }
    RET_INT(COMPARISON_BIT(INT(0), INT(1)));
}

static PyObject *lp_PyValue_AsKnownObjectBorrow(PyObject *const *args) { RET_WORD(PyValue_AsKnownObjectBorrow(VAL(0))); }
static PyObject *lp__PyValue_AsKnownObjectBorrowPointer(PyObject *const *args) { RET_WORD(_PyValue_AsKnownObjectBorrowPointer(VAL(0))); }
static PyObject *lp_Py_TYPE(PyObject *const *args) { RET_WORD(Py_TYPE(PTR(PyObject, 0))); }
static PyObject *lp_PyList_GET_SIZE(PyObject *const *args) { RET_INT(PyList_GET_SIZE(PTR(PyObject, 0))); }
static PyObject *lp_PyTuple_GET_SIZE(PyObject *const *args) { RET_INT(PyTuple_GET_SIZE(PTR(PyObject, 0))); }
static PyObject *lp_PyList_GET_VALUE(PyObject *const *args) { RET_VAL(PyList_GET_VALUE(PTR(PyObject, 0), INT(1))); }
static PyObject *lp_PyTuple_GET_VALUE(PyObject *const *args) { RET_VAL(PyTuple_GET_VALUE(PTR(PyObject, 0), INT(1))); }
static PyObject *lp__PyList_ITEMS(PyObject *const *args) { RET_WORD(_PyList_ITEMS(PTR(PyObject, 0))); }
static PyObject *lp__PyTuple_ITEMS(PyObject *const *args) { RET_WORD(_PyTuple_ITEMS(PTR(PyObject, 0))); }
static PyObject *lp__PyObject_InlineValues(PyObject *const *args) { RET_WORD(_PyObject_InlineValues(PTR(PyObject, 0))); }
static PyObject *lp__PyObject_Shape(PyObject *const *args) { RET_INT(_PyObject_Shape(PTR(PyObject, 0))); }
static PyObject *lp__PyObject_ValueSlot(PyObject *const *args) { RET_WORD(_PyObject_ValueSlot(PTR(PyObject, 0), (size_t)WORD(1))); }
static PyObject *lp__PyValueSlot_Exchange(PyObject *const *args) { RET_VAL(_PyValueSlot_Exchange(PTR(PyValue, 0), VAL(1))); }
static PyObject *lp__PyValueSlot_ExchangeFilled(PyObject *const *args) { RET_VAL(_PyValueSlot_ExchangeFilled(PTR(PyValue, 0), VAL(1))); }
static PyObject *lp__PyValueSlot_Fill(PyObject *const *args) { _PyValueSlot_Fill(PTR(PyValue, 0), VAL(1)); Py_RETURN_NONE; }
static PyObject *lp__PyObject_ShapeReadDup(PyObject *const *args) { RET_VAL(_PyObject_ShapeReadDup(PTR(PyObject, 0), (uint32_t)WORD(1))); }
static PyObject *lp_DK_UNICODE_ENTRIES(PyObject *const *args) { RET_WORD(DK_UNICODE_ENTRIES(PTR(PyDictKeysObject, 0))); }
static PyObject *lp__PyLong_IsZero(PyObject *const *args) { RET_BOOL(_PyLong_IsZero(PTR(PyLongObject, 0))); }
static PyObject *lp__PyValueGuarded_Load(PyObject *const *args)
{
    RET_VAL(_PyValueGuarded_Load((_PyValueGuarded){ .bits = (uintptr_t)WORD(0) }));
}
static PyObject *lp__PyValue_CodeConstant(PyObject *const *args) { RET_VAL(_PyValue_CodeConstant(VAL(0))); }
static PyObject *lp__PyValue_BoundLocal(PyObject *const *args) { RET_VAL(_PyValue_BoundLocal(VAL(0))); }
static PyObject *lp__PyValue_CellLocal(PyObject *const *args) { RET_VAL(_PyValue_CellLocal(VAL(0))); }
static PyObject *lp__PyValue_CommonConstant(PyObject *const *args) { RET_VAL(_PyValue_CommonConstant(VAL(0))); }
static PyObject *lp__PyValue_PublishedItem(PyObject *const *args) { RET_VAL(_PyValue_PublishedItem(VAL(0))); }

static PyObject *
lp__PyDictValues_AddToInsertionOrder(PyObject *const *args)
{
    _PyDictValues_AddToInsertionOrder(PTR(PyDictValues, 0), INT(1));
    Py_RETURN_NONE;
}

static PyObject *
lp__PyDictValues_SetShape(PyObject *const *args)
{
    _PyDictValues_SetShape(PTR(PyDictValues, 0), (uint8_t)WORD(1));
    Py_RETURN_NONE;
}

/* The frame primitives read the frame of their Python caller, whose
   address leaf_frame() gives. */
static _PyInterpreterFrame *
caller_frame(void)
{
    return _PyThreadState_GET()->current_frame;
}

static PyObject *lp_GLOBALS(PyObject *const *args) { _PyInterpreterFrame *frame = caller_frame(); RET_WORD(GLOBALS()); }
static PyObject *lp_BUILTINS(PyObject *const *args) { _PyInterpreterFrame *frame = caller_frame(); RET_WORD(BUILTINS()); }
static PyObject *lp_FRAME_CO_CONSTS(PyObject *const *args) { _PyInterpreterFrame *frame = caller_frame(); RET_WORD(FRAME_CO_CONSTS); }
static PyObject *lp_IR_SLOTS(PyObject *const *args) { _PyInterpreterFrame *frame = caller_frame(); RET_WORD(IR_SLOTS()); }
static PyObject *lp_GETLOCAL(PyObject *const *args) { _PyInterpreterFrame *frame = caller_frame(); RET_VAL(GETLOCAL((int)INT(0))); }
static PyObject *lp__Py_EvalBreakerHasEvents(PyObject *const *args) { RET_BOOL(_Py_EvalBreakerHasEvents(_PyThreadState_GET())); }

/* The ownership primitives change the count of the object they are given;
   the caller balances it. */
static PyObject *lp_PyValue_DUP(PyObject *const *args) { RET_VAL(PyValue_DUP(VAL(0))); }
static PyObject *lp_PyValue_XDUP(PyObject *const *args) { RET_VAL(PyValue_XDUP(VAL(0))); }
static PyObject *lp_PyValue_CLOSE(PyObject *const *args) { PyValue_CLOSE(VAL(0)); Py_RETURN_NONE; }
static PyObject *lp_PyValue_XCLOSE(PyObject *const *args) { PyValue_XCLOSE(VAL(0)); Py_RETURN_NONE; }

static PyObject *
lp_PyValue_CLOSE_SPECIALIZED(PyObject *const *args)
{
    const char *name = PyUnicode_AsUTF8(args[1]);
    if (name == NULL) {
        return NULL;
    }
    destructor d;
    if (strcmp(name, "_PyLong_ExactDealloc") == 0) {
        d = _PyLong_ExactDealloc;
    }
    else if (strcmp(name, "_PyFloat_ExactDealloc") == 0) {
        d = _PyFloat_ExactDealloc;
    }
    else {
        PyErr_SetString(PyExc_ValueError, "unknown destructor");
        return NULL;
    }
    PyValue_CLOSE_SPECIALIZED(VAL(0), d);
    Py_RETURN_NONE;
}

static const struct {
    const char *name;
    int nargs;
    leaf_fn fn;
} primitives[] = {
#define P(NAME, N) {#NAME, N, lp_##NAME}
    P(PyValue_IsNull, 1), P(PyValue_IsPointerSpace, 1), P(PyValue_IsObject, 1),
    P(PyValue_IsInline, 1), P(PyValue_IsInlineInt, 1), P(PyValue_IsInlineFloat, 1),
    P(PyValue_BothInlineInt, 2), P(PyValue_IsTaggedIndex, 1), P(PyValue_Is, 2),
    P(_PyValue_NamesObject, 2),
    P(PyValue_IsTrue, 1), P(PyValue_IsFalse, 1), P(PyValue_IsNone, 1),
    P(PyValue_BoolCheck, 1), P(PyValue_LongCheckExact, 1),
    P(_PyValue_IntScaled, 1), P(_PyValue_FromIntScaled, 1),
    P(PyValue_InlineInt64, 1), P(PyValue_FromInlineInt, 1),
    P(PyValue_InlineFloat, 1), P(PyValue_FromDoubleNotNaN, 1),
    P(PyValue_FloatOperandValues, 2), P(PyValue_FromInt64, 1),
    P(PyValue_FromDouble, 1), P(PyValue_TagIndex, 1), P(PyValue_UntagIndex, 1),
    P(PyValue_IncrementTaggedIndexNoOverflow, 1), P(COMPARISON_BIT, 2),
    P(PyValue_AsKnownObjectBorrow, 1), P(_PyValue_AsKnownObjectBorrowPointer, 1),
    P(Py_TYPE, 1), P(PyList_CheckExact, 1), P(PyTuple_CheckExact, 1),
    P(PyDict_CheckExact, 1), P(PyModule_CheckExact, 1),
    P(PyUnicode_CheckExact, 1), P(PyFunction_Check, 1),
    P(PyList_GET_SIZE, 1), P(PyTuple_GET_SIZE, 1), P(PyList_GET_VALUE, 2),
    P(PyTuple_GET_VALUE, 2), P(_PyList_ITEMS, 1), P(_PyTuple_ITEMS, 1), P(_PyObject_InlineValues, 1),
    P(_PyObject_Shape, 1), P(_PyObject_ValueSlot, 2), P(_PyObject_ShapeReadDup, 2),
    P(_PyValueSlot_Exchange, 2), P(_PyValueSlot_ExchangeFilled, 2), P(_PyValueSlot_Fill, 2),
    P(DK_UNICODE_ENTRIES, 1), P(_PyDictValues_AddToInsertionOrder, 2),
    P(_PyDictValues_SetShape, 2), P(_PyLong_IsZero, 1), P(_PyValueGuarded_Load, 1),
    P(_PyValue_CodeConstant, 1), P(_PyValue_BoundLocal, 1), P(_PyValue_CellLocal, 1),
    P(_PyValue_CommonConstant, 1),
    P(_PyValue_PublishedItem, 1),
    P(GLOBALS, 0), P(BUILTINS, 0), P(FRAME_CO_CONSTS, 0), P(GETLOCAL, 1), P(IR_SLOTS, 0),
    P(_Py_EvalBreakerHasEvents, 0),
    P(PyValue_DUP, 1), P(PyValue_XDUP, 1), P(PyValue_CLOSE, 1),
    P(PyValue_XCLOSE, 1), P(PyValue_CLOSE_SPECIALIZED, 2),
#undef P
};

// leaf_primitive(name, *args): the C definition of primitive name on args
static PyObject *
leaf_primitive(PyObject *Py_UNUSED(module), PyObject *const *args, Py_ssize_t nargs)
{
    if (nargs < 1 || !PyUnicode_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError, "leaf_primitive(name, *args)");
        return NULL;
    }
    const char *name = PyUnicode_AsUTF8(args[0]);
    if (name == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < Py_ARRAY_LENGTH(primitives); i++) {
        if (strcmp(primitives[i].name, name) == 0) {
            if (nargs - 1 != primitives[i].nargs) {
                PyErr_Format(PyExc_TypeError, "%s takes %d arguments",
                             name, primitives[i].nargs);
                return NULL;
            }
            return primitives[i].fn(args + 1);
        }
    }
    PyErr_Format(PyExc_KeyError, "no primitive %s", name);
    return NULL;
}

// leaf_primitive_names(): the primitives that leaf_primitive() calls
static PyObject *
leaf_primitive_names(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(ignored))
{
    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < Py_ARRAY_LENGTH(primitives); i++) {
        PyObject *name = PyUnicode_FromString(primitives[i].name);
        if (name == NULL || PyList_Append(result, name) < 0) {
            Py_XDECREF(name);
            Py_DECREF(result);
            return NULL;
        }
        Py_DECREF(name);
    }
    return result;
}

static PyObject *
leaf_frame(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(ignored))
{
    RET_WORD(caller_frame());
}

/* leaf_thread() -> the address of the thread state, the operand that the
   thread primitives and helpers take */
static PyObject *
leaf_thread(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(ignored))
{
    RET_WORD(_PyThreadState_GET());
}

static int
add_int(PyObject *dict, const char *name, long long value)
{
    PyObject *v = PyLong_FromLongLong(value);
    if (v == NULL) {
        return -1;
    }
    int err = PyDict_SetItemString(dict, name, v);
    Py_DECREF(v);
    return err;
}

static int
add_word(PyObject *dict, const char *name, uint64_t value)
{
    PyObject *v = PyLong_FromUnsignedLongLong(value);
    if (v == NULL) {
        return -1;
    }
    int err = PyDict_SetItemString(dict, name, v);
    Py_DECREF(v);
    return err;
}

/* leaf_layout() -> (constants, offsets, sizes, symbols): the numbers that
   the specs name, from C.  The names match ir_leaf.CONSTANTS, FIELDS, SIZES
   and SYMBOLS. */
static PyObject *
leaf_layout(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(ignored))
{
    PyObject *k = PyDict_New(), *off = PyDict_New(), *size = PyDict_New();
    PyObject *sym = PyDict_New();
    if (k == NULL || off == NULL || size == NULL || sym == NULL) {
        goto error;
    }
#define K(NAME) if (add_int(k, #NAME, (long long)(NAME)) < 0) goto error
#define KW(NAME) if (add_word(k, #NAME, (uint64_t)(NAME)) < 0) goto error
    KW(Py_PTR_LIMIT); KW(Py_INDEX_BASE); KW(Py_DOUBLE_OFFSET); K(Py_INT_SHIFT);
    KW(Py_INT_BASE); K(Py_INT_MIN); K(Py_INT_MAX); KW(Py_OWNED_MASK);
    K(Py_TAG_REFCNT); KW(_PY_EVAL_EVENTS_MASK);
#define OFF(S, F) if (add_int(off, #S "." #F, (long long)offsetof(S, F)) < 0) goto error
    OFF(PyObject, ob_refcnt); OFF(PyObject, ob_type); OFF(PyVarObject, ob_size);
    OFF(PyTypeObject, tp_version_tag); OFF(PyTypeObject, tp_basicsize);
    OFF(PyListObject, ob_item); OFF(PyListObject, allocated);
    OFF(PyTupleObject, ob_item); OFF(PyDictObject, ma_keys);
    OFF(PyDictKeysObject, dk_version); OFF(PyDictKeysObject, dk_nentries);
    OFF(PyDictKeysObject, dk_log2_index_bytes); OFF(PyDictKeysObject, dk_indices);
    OFF(PyDictUnicodeEntry, me_value); OFF(PyDictValues, capacity);
    OFF(PyDictValues, size); OFF(PyDictValues, valid); OFF(PyDictValues, shape);
    OFF(PyDictValues, values); OFF(PyModuleObject, md_dict);
    OFF(PyFunctionObject, func_version); OFF(_PyRangeIterObject, start);
    OFF(_PyRangeIterObject, step); OFF(_PyRangeIterObject, len);
    OFF(PyLongObject, long_value.lv_tag);
    OFF(_PyInterpreterFrame, f_globals); OFF(_PyInterpreterFrame, f_builtins);
    OFF(_PyInterpreterFrame, f_executable); OFF(_PyInterpreterFrame, base);
    OFF(PyCodeObject, co_consts); OFF(PyThreadState, eval_breaker);
#define SIZE(T) if (add_int(size, #T, (long long)sizeof(T)) < 0) goto error
    SIZE(PyValue); SIZE(PyDictUnicodeEntry);
#define SYM(NAME) if (add_word(sym, #NAME, (uint64_t)(uintptr_t)&NAME) < 0) goto error
    SYM(_Py_TrueStruct); SYM(_Py_FalseStruct); SYM(_Py_NoneStruct);
    SYM(PyLong_Type); SYM(PyFloat_Type); SYM(PyBool_Type); SYM(PyList_Type);
    SYM(PyTuple_Type); SYM(PyDict_Type); SYM(PyModule_Type); SYM(PyFunction_Type);
    SYM(PyUnicode_Type); SYM(PyRangeIter_Type); SYM(PyType_Type);
#undef K
#undef KW
#undef OFF
#undef SIZE
#undef SYM
    return Py_BuildValue("(NNNN)", k, off, size, sym);
error:
    Py_XDECREF(k);
    Py_XDECREF(off);
    Py_XDECREF(size);
    Py_XDECREF(sym);
    return NULL;
}

/* The leaf probes: each uop inside the leaf subset as a C function on a
 * stack of words (Tools/cases_generator/leaf_probe_generator.py), printed by
 * the generators' Emitter with the probe's binding of the statement macros.
 * The interpreter's word and frame stack pointer have no part in a probe:
 * the fused flag is the caller's, and the stack is the caller's array. */
enum { LEAF_PROBE_NEXT, LEAF_PROBE_EXIT, LEAF_PROBE_DEOPT, LEAF_PROBE_RAISE,
       LEAF_PROBE_COMPLETE };

struct leaf_probe_result {
    PyValue *stack_pointer;     // after NEXT: the top of the outputs
    int branch;                 // JUMPBY or JUMPTO ran
    int edge;                   // 0, or 1 + the flag that IR_REPORT_EDGE saw
    int fused_if_false;         // IR_FUSED_IF_FALSE(), an input
    PyValue completed;          // after COMPLETE: the completion's value
};

typedef int (*leaf_probe_fn)(_PyInterpreterFrame *, PyValue *, int,
                             const uint64_t *, struct leaf_probe_result *);

struct leaf_probe {
    const char *name;
    leaf_probe_fn run;
    int inputs;
};

#undef ASSERT_WITHIN_STACK_BOUNDS
#define ASSERT_WITHIN_STACK_BOUNDS(F, L) ((void)0)
#undef IR_FUSED_IF_FALSE
#define IR_FUSED_IF_FALSE() (result->fused_if_false)

/* A probe declares each stack item; a body may not use them all */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include "leaf_probes.c.h"
#pragma GCC diagnostic pop

#define LEAF_PROBE_STACK 16

static uint64_t
tuple_word(PyObject *tuple, Py_ssize_t i)
{
    PyObject *item = PySequence_GetItem(tuple, i);
    if (item == NULL) {
        return 0;
    }
    uint64_t word = PyLong_AsUnsignedLongLongMask(item);
    Py_DECREF(item);
    return word;
}

/* leaf_probe(name, inputs, oparg, caches, fused_if_false): run the probe of
 * uop name on the input words, in the caller's frame.  Returns (outcome,
 * outputs, branch, edge, completed); the outputs are the stack's words after
 * NEXT, else (). */
static PyObject *
leaf_probe(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *name;
    PyObject *inputs, *caches;
    int oparg, fused_if_false;
    if (!PyArg_ParseTuple(args, "sO!iO!p", &name, &PyTuple_Type, &inputs,
                          &oparg, &PyTuple_Type, &caches, &fused_if_false)) {
        return NULL;
    }
    const struct leaf_probe *probe = LEAF_PROBES;
    while (probe->name != NULL && strcmp(probe->name, name) != 0) {
        probe++;
    }
    if (probe->name == NULL) {
        PyErr_Format(PyExc_KeyError, "no leaf probe %s", name);
        return NULL;
    }
    Py_ssize_t n = PyTuple_GET_SIZE(inputs);
    if (n != probe->inputs || PyTuple_GET_SIZE(caches) > 8) {
        PyErr_SetString(PyExc_ValueError, "wrong number of inputs or caches");
        return NULL;
    }
    PyValue stack[LEAF_PROBE_STACK] = {0};
    uint64_t cache_words[8] = {0};
    for (Py_ssize_t i = 0; i < n; i++) {
        stack[i].bits = (uintptr_t)tuple_word(inputs, i);
    }
    for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(caches); i++) {
        cache_words[i] = tuple_word(caches, i);
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    struct leaf_probe_result result = {NULL, 0, 0, fused_if_false, {0}};
    _PyInterpreterFrame *frame = _PyThreadState_GET()->current_frame;
    int outcome = probe->run(frame, stack + n, oparg, cache_words, &result);
    if (outcome == LEAF_PROBE_RAISE && PyErr_Occurred()) {
        PyErr_Clear();
    }
    Py_ssize_t count = outcome == LEAF_PROBE_NEXT ? result.stack_pointer - stack : 0;
    assert(0 <= count && count < LEAF_PROBE_STACK);
    PyObject *outputs = PyTuple_New(count);
    if (outputs == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < count; i++) {
        PyObject *word = PyLong_FromUnsignedLongLong(stack[i].bits);
        if (word == NULL) {
            Py_DECREF(outputs);
            return NULL;
        }
        PyTuple_SET_ITEM(outputs, i, word);
    }
    return Py_BuildValue("iNiiK", outcome, outputs, result.branch, result.edge,
                         (unsigned long long)result.completed.bits);
}

static PyMethodDef test_methods[] = {
    {"leaf_probe", leaf_probe, METH_VARARGS},
    {"leaf_primitive", _PyCFunction_CAST(leaf_primitive), METH_FASTCALL},
    {"leaf_primitive_names", leaf_primitive_names, METH_NOARGS},
    {"leaf_frame", leaf_frame, METH_NOARGS},
    {"leaf_thread", leaf_thread, METH_NOARGS},
    {"leaf_layout", leaf_layout, METH_NOARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Leaf(PyObject *mod)
{
    return PyModule_AddFunctions(mod, test_methods);
}
