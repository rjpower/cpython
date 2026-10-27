#ifndef Py_INTERNAL_VALUE_H
#define Py_INTERNAL_VALUE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_long.h"          // _PyLong_CheckExactAndCompact()
#include "pycore_object.h"
#include "pycore_stats.h"         // OBJECT_STAT_INC()
#include "pycore_tstate.h"        // _PyThreadStateImpl

#include <stdbool.h>              // bool


/*
  The core's view of PyValue (Include/pyvalue.h): the values that never
  reach Python code, and the helpers of specialized instructions.

  An index (_PyValue_TagIndex()) is the position of a virtual iterator in the
  FOR_ITER family (null_or_index) or the saved lasti during exception
  unwinding.  It never reaches Python code and must not be converted to a
  PyObject *.  _PyValue_ERROR is a handler's failed result.  A wrapped
  pointer (_PyValue_Wrap()) passes a C pointer from one uop to another.

  PyValue_Object() boxes an inline value for code that needs a PyObject *.
  The box lives in a C local, never in the slot the value came from, which
  stays canonical; across an escaping call a value root holds it
  (_PyValue_ObjectRooted()).  The cases generator does this for instruction
  inputs; see "Inline values" in
  Tools/cases_generator/interpreter_definition.md.
*/

#if SIZEOF_VOID_P != 8
#  error "PyValue needs 64-bit words"
#endif

static inline bool _PyValue_IsPointerSpace(PyValue v) { return v.bits < _PyValue_PTR_LIMIT; }
static inline bool _PyValue_IsTaggedIndex(PyValue v) { return v.bits - _PyValue_INDEX_BASE < _PyValue_DOUBLE_OFFSET - _PyValue_INDEX_BASE; }
// Relies on 0xFFFF being unused: only two inline ints have all of these bits set.
static inline bool _PyValue_BothInlineInt(PyValue a, PyValue b) { return (a.bits & b.bits) >= _PyValue_INT_BASE; }

/* The top 16 bits of *v, which select the region. Reading them from memory
   rather than shifting a loaded value lets the compiler fold the load into a
   compare with an immediate ("cmpw $0, 6(v)" on x86-64), so testing a slot
   costs a compare and a branch. */
static inline uint16_t
_PyValue_TopBitsAt(const PyValue *v)
{
#if PY_LITTLE_ENDIAN
    uint16_t top;
    memcpy(&top, (const char *)v + 6, sizeof(top));
    return top;
#else
    return (uint16_t)(v->bits >> 48);
#endif
}

static inline bool _PyValue_IsPointerSpaceAt(const PyValue *v) { return _PyValue_TopBitsAt(v) == 0; }
static inline bool _PyValue_IsInlineAt(const PyValue *v) { return _PyValue_TopBitsAt(v) >= (_PyValue_DOUBLE_OFFSET >> 48); }

// i is in -1 .. 2**47 - 1.
static inline PyValue
_PyValue_TagIndex(intptr_t i)
{
    assert(i >= -1 && i < ((intptr_t)1 << 47));
    return (PyValue){ _PyValue_INDEX_BASE | ((uintptr_t)i & (_PyValue_PTR_LIMIT - 1)) };
}

static inline intptr_t
_PyValue_UntagIndex(PyValue v)
{
    assert(_PyValue_IsTaggedIndex(v));
    return Py_ARITHMETIC_RIGHT_SHIFT(intptr_t, (intptr_t)(v.bits << 16), 16);
}

/* Increments a tagged index, but does not check for overflow */
static inline PyValue
_PyValue_IncrementTaggedIndexNoOverflow(PyValue v)
{
    assert(_PyValue_UntagIndex(v) >= 0);
    assert(_PyValue_UntagIndex(v) < ((intptr_t)1 << 47) - 1);
    v.bits++;
    return v;
}

// The identity of op: its canonical value's encoding (see "Identity" in
// Include/pyvalue.h).
static inline uintptr_t
_PyObject_ValueIdentity(PyObject *op)
{
    return PyValue_FromObject(op).bits;
}

static const PyValue _PyValue_ERROR = { .bits = _PyValue_ERROR_BITS };

/* Wrap a pointer in a value.
 * The result is not safe and should only be used in the interpreter to pass
 * values from one uop to another.  The GC should never see one. */
static inline PyValue
_PyValue_Wrap(void *ptr)
{
    assert(ptr != NULL);
#ifdef Py_DEBUG
    assert(((uintptr_t)ptr & _PyValue_TAG_BITS) == 0);
    return (PyValue){ .bits = ((uintptr_t)ptr) | _PyValue_ERROR_BITS };
#else
    return (PyValue){ .bits = (uintptr_t)ptr };
#endif
}

static inline void *
_PyValue_Unwrap(PyValue v)
{
#ifdef Py_DEBUG
    assert((v.bits & _PyValue_TAG_BITS) == _PyValue_ERROR_BITS);
    return (void *)(v.bits & ~(uintptr_t)_PyValue_TAG_BITS);
#else
    return (void *)(v.bits);
#endif
}

static inline bool
_PyValue_IsError(PyValue v)
{
    return v.bits == _PyValue_ERROR_BITS;
}

static inline bool
_PyValue_IsValid(PyValue v)
{
    /* Invalid values are ERROR and NULL */
    return v.bits > _PyValue_ERROR_BITS;
}

#define _PyValue_True ((PyValue){.bits = (uintptr_t)&_Py_TrueStruct })
#define _PyValue_False ((PyValue){.bits = (uintptr_t)&_Py_FalseStruct })
#define _PyValue_None ((PyValue){.bits = (uintptr_t)&_Py_NoneStruct })

#define _PyValue_IsTrue(V) ((V).bits == (uintptr_t)&_Py_TrueStruct)
#define _PyValue_IsFalse(V) ((V).bits == (uintptr_t)&_Py_FalseStruct)
#define _PyValue_IsNone(V) ((V).bits == (uintptr_t)&_Py_NoneStruct)

static inline bool
_PyValue_IsMalformed(PyValue v)
{
    return _PyValue_IsPointerSpace(v) && (v.bits & _PyValue_TAG_BITS) == _PyValue_ERROR_BITS;
}

/* Ints in specialized instructions. The int guards accept an inline int or
   a reference to an exact int with at most one digit; either way the value
   fits in int32. These accessors read both forms without converting the
   value to a PyObject *, so the cases generator does not box their
   argument (see "Inline values" in
   Tools/cases_generator/interpreter_definition.md). */
static inline bool
_PyValue_IsCompactInt(PyValue v)
{
    if (PyValue_IsInlineInt(v)) {
        return true;
    }
    return _PyValue_IsPointerSpace(v)
           && _PyLong_CheckExactAndCompact(PyValue_ObjectBorrow(v));
}

static inline int32_t
_PyValue_CompactInt(PyValue v)
{
    assert(_PyValue_IsCompactInt(v));
    if (PyValue_IsInlineInt(v)) {
        return PyValue_Int32(v);
    }
    return (int32_t)_PyLong_CompactValue((PyLongObject *)PyValue_ObjectBorrow(v));
}

// value << shift for a shift of 32 or more places, which makes a long int,
// or PyValue_NULL with an exception set.
PyAPI_FUNC(PyValue) _PyValue_LshiftInt32(int32_t value, int32_t shift);

/* Is v an exact float, inline or boxed?  The same as
   PyValue_Type(v) == &PyFloat_Type, in the order that the float paths of
   specialized instructions test it: inline first. */
static inline bool
_PyValue_FloatCheckExact(PyValue v)
{
    return PyValue_IsInlineFloat(v) ||
           (_PyValue_IsPointerSpace(v) && PyFloat_CheckExact(PyValue_ObjectBorrow(v)));
}

// A float or a compact int, inline or boxed, as a double.
static inline double
_PyValue_NumberAsDouble(PyValue v)
{
    if (PyValue_IsInlineFloat(v)) {
        return PyValue_Double(v);
    }
    if (PyValue_IsInlineInt(v)) {
        return (double)PyValue_Int32(v);
    }
    PyObject *op = PyValue_ObjectBorrow(v);
    if (PyFloat_CheckExact(op)) {
        return PyFloat_AS_DOUBLE(op);
    }
    assert(_PyLong_CheckExactAndCompact(op));
    return (double)_PyLong_CompactValue((PyLongObject *)op);
}

/* Are a and b operands of specialized float arithmetic: two floats, or a
   float and a compact int, which converts to double exactly (see
   _PyValue_IsCompactInt())? A NaN next to an int is refused, as the
   int-float forms of BINARY_OP_EXTEND that this replaces refused it; an
   inline float is never NaN, so that costs a test only for a box. */
static inline bool
_PyValue_FloatOperandsCheck(PyValue a, PyValue b)
{
    if (PyValue_IsInlineFloat(a) && PyValue_IsInlineFloat(b)) {
        return true;
    }
    if (_PyValue_FloatCheckExact(a)) {
        return _PyValue_FloatCheckExact(b) ||
               (_PyValue_IsCompactInt(b) && !isnan(_PyValue_NumberAsDouble(a)));
    }
    return _PyValue_IsCompactInt(a) && _PyValue_FloatCheckExact(b) &&
           !isnan(_PyValue_NumberAsDouble(b));
}

/* If _PyValue_FloatOperandsCheck(a, b), store a and b as doubles in *da
   and *db and return true. Two inline floats are tested first, which lets
   the compiler give them a path of their own. */
static inline bool
_PyValue_FloatOperandValues(PyValue a, PyValue b, double *da, double *db)
{
    if (PyValue_IsInlineFloat(a) && PyValue_IsInlineFloat(b)) {
        *da = PyValue_Double(a);
        *db = PyValue_Double(b);
        return true;
    }
    if (!_PyValue_FloatOperandsCheck(a, b)) {
        return false;
    }
    *da = _PyValue_NumberAsDouble(a);
    *db = _PyValue_NumberAsDouble(b);
    return true;
}

/* Container operations on values.  Containers hold canonical values, so
   an int or float item is usually inline: these work on it without a box,
   and box only to call the object's own method. */

/* The order of two inline values, <0, 0 or >0.  An int32 converts to a
   double exactly, and neither is NaN, so this is the order of the numbers. */
static inline int
_PyValue_CompareInline(PyValue a, PyValue b)
{
    assert(PyValue_IsInline(a) && PyValue_IsInline(b));
    if (_PyValue_BothInlineInt(a, b)) {
        int32_t x = PyValue_Int32(a), y = PyValue_Int32(b);
        return (x > y) - (x < y);
    }
    double x = PyValue_IsInlineInt(a) ? (double)PyValue_Int32(a) : PyValue_Double(a);
    double y = PyValue_IsInlineInt(b) ? (double)PyValue_Int32(b) : PyValue_Double(b);
    return (x > y) - (x < y);
}

static inline int
_PyValue_CompareResult(int c, int op)
{
    switch (op) {
    case Py_LT: return c < 0;
    case Py_LE: return c <= 0;
    case Py_EQ: return c == 0;
    case Py_NE: return c != 0;
    case Py_GT: return c > 0;
    default:    return c >= 0;
    }
}

/* Could the collector need to traverse a container holding v?  As
   _PyObject_GC_MAY_BE_TRACKED(), and false for NULL and inline values. */
static inline bool
_PyValue_MayBeTracked(PyValue v)
{
    return PyValue_IsObject(v)
           && _PyObject_GC_MAY_BE_TRACKED(PyValue_ObjectBorrow(v));
}

/* Store the canonical values of the n objects src in dst. */
static inline void
_PyValue_FromObjectArray(PyValue *dst, PyObject *const *src, Py_ssize_t n)
{
    for (Py_ssize_t i = 0; i < n; i++) {
        dst[i] = PyValue_FromObject(src[i]);
    }
}

extern PyTypeObject _PyObjectArray_Type;

/* The n values items as an array of PyObject *, for C code that takes one:
   items itself if none is inline, since a value that refers to an object
   is the object's pointer, and otherwise a new array of the objects, with
   the inline values boxed, which the collector frees once the caller no
   longer points into it.  NULL only with an exception set, if boxing
   fails.  An item may be NULL, and items may be NULL if n is 0. */
PyAPI_FUNC(PyObject **) _PyValue_ObjectArray(PyValue *items, Py_ssize_t n);

/* A C local's value root (_PyValueRoot): pushed, it is visited by the
   collector until it is popped. */
static inline void
_PyThreadState_PushValueRoot(PyThreadState *tstate, _PyValueRoot *root)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    root->value = PyValue_NULL;
    root->base = &root->value;
    root->n = 1;
    root->next = ts->gc_trace.value_roots;
    ts->gc_trace.value_roots = root;
}

/* Push root as a span over base, a buffer of the caller's, holding nothing
   yet.  Popped with _PyThreadState_PopValueRoot(). */
static inline void
_PyThreadState_PushValueSpan(PyThreadState *tstate, _PyValueRoot *root,
                             PyValue *base)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    root->value = PyValue_NULL;
    root->base = base;
    root->n = 0;
    root->next = ts->gc_trace.value_roots;
    ts->gc_trace.value_roots = root;
}

/* Store v at the end of the span and extend the span over it, in one step,
   so that the collector never visits a slot that is not yet filled nor
   misses one that is.  The caller sized the buffer.  v is stored as it is:
   a raw box (_PyValue_FromBits() of a box's pointer) stays the box, which
   is what keeps a box that C code points into alive. */
static inline void
_PyValueSpan_Append(_PyValueRoot *root, PyValue v)
{
    root->base[root->n] = v;
    root->n++;
}

/* The span's buffer moved to base, which holds the values appended so far,
   as PyMem_Realloc() leaves them.  Call it before any other collecting
   call. */
static inline void
_PyValueSpan_Rebase(_PyValueRoot *root, PyValue *base)
{
    root->base = base;
}

/* The collector scans most C frames conservatively, so a popped value left
   in its root would keep the object alive until the stack memory is reused.
   The barrier keeps the store, which is dead to the compiler.  Returns the
   value the root held. */
static inline PyValue
_PyThreadState_PopValueRoot(PyThreadState *tstate, _PyValueRoot *root)
{
    _PyThreadStateImpl *ts = (_PyThreadStateImpl *)tstate;
    assert(ts->gc_trace.value_roots == root);
    PyValue res = root->value;
    ts->gc_trace.value_roots = root->next;
    root->value = PyValue_NULL;
    __asm__ volatile("" : : "r"(root) : "memory");
    return res;
}


/* The object that root holds, for C code that needs a PyObject *: an
   inline value is boxed in place, and root holds the box (its object field)
   from then on, so root's value must not be used as a PyValue afterwards.
   NULL if root holds NULL, or with an exception set if boxing fails. */
static inline PyObject *
_PyValueRoot_Object(_PyValueRoot *root)
{
    if (PyValue_IsInline(root->value)) {
        root->object = _PyValue_BoxInline(root->value);
    }
    return root->object;
}

/* The object of v, for a handler to pass to an escaping call: if v holds an
   inline value, a new box, which root holds until
   _PyValue_UnrootObject(tstate, v, root).  kind (_PY_BOX_CALL and so on)
   says what the box is for, for the statistics.  NULL with an exception set
   if boxing fails, and then root is not pushed. */
static inline PyObject *
_PyValue_ObjectRooted(PyThreadState *tstate, PyValue v, _PyValueRoot *root,
                      int kind)
{
    (void)kind;
    if (PyValue_IsInline(v)) {
        PyObject *box = _PyValue_BoxInline(v);
        if (box != NULL) {
            VALUE_BOX_STAT_INC(kind);
            _PyThreadState_PushValueRoot(tstate, root);
            root->object = box;
        }
        return box;
    }
    return PyValue_ObjectBorrow(v);
}

/* Push root holding v, and return v's object for C code that calls a
   PyObject * function: an inline value is boxed in root, which keeps the
   box alive until it is popped.  NULL if v is PyValue_NULL, or with an
   exception set if boxing fails.  The root is pushed in either case, so
   that the caller pops it on every path. */
static inline PyObject *
_PyValueRoot_PushObject(PyThreadState *tstate, _PyValueRoot *root,
                        PyValue v, int kind)
{
    (void)kind;
    _PyThreadState_PushValueRoot(tstate, root);
    root->value = v;
    if (PyValue_IsInline(v)) {
        root->object = _PyValue_BoxInline(v);
        if (root->object != NULL) {
            VALUE_BOX_STAT_INC(kind);
        }
        return root->object;
    }
    return PyValue_ObjectBorrow(v);
}

static inline void
_PyValue_UnrootObject(PyThreadState *tstate, PyValue v, _PyValueRoot *root)
{
    if (PyValue_IsInline(v)) {
        _PyThreadState_PopValueRoot(tstate, root);
    }
}


// Like Py_VISIT but for PyValue fields
#define _Py_VISIT_VALUE(v)                                              \
    do {                                                                \
        if (PyValue_IsObject(v)) {                                      \
            int vret = _PyGC_VisitValue(&(v), visit, arg);              \
            if (vret)                                                   \
                return vret;                                            \
        }                                                               \
    } while (0)

/* Leave the values NULL, so that the slots keep nothing alive */
static inline void
_PyValue_ClearArray(PyValue *values, int n)
{
    for (int i = 0; i < n; i++) {
        values[i] = PyValue_NULL;
    }
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_VALUE_H */
