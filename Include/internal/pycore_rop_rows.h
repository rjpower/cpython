#ifndef Py_INTERNAL_ROP_ROWS_H
#define Py_INTERNAL_ROP_ROWS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_cell.h"         // _PyCell_GetValue()
#include "pycore_ceval.h"         // _PyEval_LoadGlobalValue()
#include "pycore_code.h"          // _PyBinaryOp_ExtendDescr()
#include "pycore_feedback.h"      // _PyFbBinary
#include "pycore_interpframe.h"   // _PyFrame_TakeHome()
#include "pycore_value.h"         // PyValue
#include "pycore_opcode_utils.h"  // NUM_COMMON_CONSTANTS
#include "pycore_rop_metadata.h"  // ROP_RELATION_LT and the other declared enums
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_runtime_structs.h" // _PY_NSMALLPOSINTS
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()
#include "pycore_dict.h"          // _PyDict_ContainsValue()
#include "pycore_setobject.h"     // _PySet_Contains()
#include "pycore_unicodeobject.h" // _PyUnicode_Equal()
#include "pycore_value_slots.h"   // _PyType_VALUE_SLOT()

/* Binary's op operand is BINARY_OP's oparg, so the declared Numeric enum
 * numbers its members as Include/opcode.h numbers NB_*. */
static_assert(ROP_NUMERIC_NB_ADD == NB_ADD, "Numeric NB_ADD");
static_assert(ROP_NUMERIC_NB_AND == NB_AND, "Numeric NB_AND");
static_assert(ROP_NUMERIC_NB_FLOOR_DIVIDE == NB_FLOOR_DIVIDE, "Numeric NB_FLOOR_DIVIDE");
static_assert(ROP_NUMERIC_NB_LSHIFT == NB_LSHIFT, "Numeric NB_LSHIFT");
static_assert(ROP_NUMERIC_NB_MATRIX_MULTIPLY == NB_MATRIX_MULTIPLY, "Numeric NB_MATRIX_MULTIPLY");
static_assert(ROP_NUMERIC_NB_MULTIPLY == NB_MULTIPLY, "Numeric NB_MULTIPLY");
static_assert(ROP_NUMERIC_NB_REMAINDER == NB_REMAINDER, "Numeric NB_REMAINDER");
static_assert(ROP_NUMERIC_NB_OR == NB_OR, "Numeric NB_OR");
static_assert(ROP_NUMERIC_NB_POWER == NB_POWER, "Numeric NB_POWER");
static_assert(ROP_NUMERIC_NB_RSHIFT == NB_RSHIFT, "Numeric NB_RSHIFT");
static_assert(ROP_NUMERIC_NB_SUBTRACT == NB_SUBTRACT, "Numeric NB_SUBTRACT");
static_assert(ROP_NUMERIC_NB_TRUE_DIVIDE == NB_TRUE_DIVIDE, "Numeric NB_TRUE_DIVIDE");
static_assert(ROP_NUMERIC_NB_XOR == NB_XOR, "Numeric NB_XOR");
static_assert(ROP_NUMERIC_NB_INPLACE_ADD == NB_INPLACE_ADD, "Numeric NB_INPLACE_ADD");
static_assert(ROP_NUMERIC_NB_INPLACE_AND == NB_INPLACE_AND, "Numeric NB_INPLACE_AND");
static_assert(ROP_NUMERIC_NB_INPLACE_FLOOR_DIVIDE == NB_INPLACE_FLOOR_DIVIDE, "Numeric NB_INPLACE_FLOOR_DIVIDE");
static_assert(ROP_NUMERIC_NB_INPLACE_LSHIFT == NB_INPLACE_LSHIFT, "Numeric NB_INPLACE_LSHIFT");
static_assert(ROP_NUMERIC_NB_INPLACE_MATRIX_MULTIPLY == NB_INPLACE_MATRIX_MULTIPLY, "Numeric NB_INPLACE_MATRIX_MULTIPLY");
static_assert(ROP_NUMERIC_NB_INPLACE_MULTIPLY == NB_INPLACE_MULTIPLY, "Numeric NB_INPLACE_MULTIPLY");
static_assert(ROP_NUMERIC_NB_INPLACE_REMAINDER == NB_INPLACE_REMAINDER, "Numeric NB_INPLACE_REMAINDER");
static_assert(ROP_NUMERIC_NB_INPLACE_OR == NB_INPLACE_OR, "Numeric NB_INPLACE_OR");
static_assert(ROP_NUMERIC_NB_INPLACE_POWER == NB_INPLACE_POWER, "Numeric NB_INPLACE_POWER");
static_assert(ROP_NUMERIC_NB_INPLACE_RSHIFT == NB_INPLACE_RSHIFT, "Numeric NB_INPLACE_RSHIFT");
static_assert(ROP_NUMERIC_NB_INPLACE_SUBTRACT == NB_INPLACE_SUBTRACT, "Numeric NB_INPLACE_SUBTRACT");
static_assert(ROP_NUMERIC_NB_INPLACE_TRUE_DIVIDE == NB_INPLACE_TRUE_DIVIDE, "Numeric NB_INPLACE_TRUE_DIVIDE");
static_assert(ROP_NUMERIC_NB_INPLACE_XOR == NB_INPLACE_XOR, "Numeric NB_INPLACE_XOR");

/* An arm's operand (a B) is an object exactly when it is in pointer space:
 * it is never NULL, and the other words below Py_PTR_LIMIT that
 * PyValue_IsObject() rejects are a debug build's PyValue_Wrap() pointers,
 * which no operand holds.  The rows test their B operands with this, as the
 * is_object primitive does (Python/register_ops/core.rop). */
static inline bool
_PyRop_IsObject(PyValueBorrow v)
{
    assert(!PyValue_IsNull(v));
    assert(PyValue_IsPointerSpace(v) == (bool)PyValue_IsObject(v));
    return PyValue_IsPointerSpace(v);
}
#define _PyRop_IsObject(V0) (_PyRop_IsObject)(PyValue_Borrow(V0))

/* The C bodies of the call-boundary rows of Python/register_ops/core.rop.
 * The generated handlers build the arguments from the word: a `Span<Src>`
 * is the homes and local flags of the listed operands, in order. */

#define _PY_ROP_MAX_SRCS 4

typedef struct {
    _PyInterpreterFrame *frame;
    uint8_t n;
    uint8_t local[_PY_ROP_MAX_SRCS];
    PyValue *home[_PY_ROP_MAX_SRCS];
} _PyRopSrcs;

/* A span operand of the instruction whose head is head: count homes from
 * element first of the word's spans, with the constant geometry of the
 * arm's layout (pycore_ir.h, _PyIR_SpanElementAt()), which the generated
 * handler gives as literals */
typedef struct {
    _PyInterpreterFrame *frame;
    const _PyIRWord *head;
    int first;
    int count;
    int nhead;
    int fixed;
} _PyRopSpan;

/* The frame index of home i of a span */
static inline Py_ALWAYS_INLINE int
_PyRopSpan_Slot(const _PyRopSpan *span, int i)
{
    return _PyIR_SpanElementAt(span->head, span->nhead, span->fixed,
                               span->first + i);
}

/* The rows of the Move arms, in ceval.c */
PyAPI_FUNC(void) _PyFrame_ApplyRecipe(_PyInterpreterFrame *frame,
                                      const int32_t *recipe);
PyAPI_FUNC(void) _PyRop_MoveInOrder(const _PyRopSpan *d, const _PyRopSpan *s);
PyAPI_FUNC(void) _PyRop_RotateHomes(_PyInterpreterFrame *frame,
                                    const _PyRopSpan *homes);
PyAPI_FUNC(void) _PyRop_CloseSpan(const _PyRopSpan *s);
PyAPI_FUNC(void) _PyRop_CopyHomes(const _PyRopSpan *d, const _PyRopSpan *s);

/* Build, AddElement, AddEntry, UpdateContainer and MergeKeywords, in ceval.c */
PyAPI_FUNC(PyValue) _PyRop_BuildTuple(const _PyRopSpan *items);
PyAPI_FUNC(PyValue) _PyRop_BuildList(const _PyRopSpan *items);
PyAPI_FUNC(PyValue) _PyRop_BuildSet(const _PyRopSpan *items);
PyAPI_FUNC(PyValue) _PyRop_BuildMap(const _PyRopSpan *items);
/* BuildText, in ceval.c */
PyAPI_FUNC(PyValue) _PyRop_JoinText(const _PyRopSpan *parts);
PyAPI_FUNC(PyValue) _PyRop_BuildInterpolation(const _PyRopSpan *parts, int flags);
PyAPI_FUNC(PyValue) _PyRop_BuildTemplate(const _PyRopSpan *parts);
PyAPI_FUNC(int) _PyRop_ListAppend(PyValue *list, PyValue item);
PyAPI_FUNC(int) _PyRop_SetAdd(PyValue *set, PyValue item);
PyAPI_FUNC(int) _PyRop_DictPut(PyValue *dict, PyValue key, PyValue value);
PyAPI_FUNC(int) _PyRop_ListExtend(PyValue *list, PyValue *iterable);
PyAPI_FUNC(int) _PyRop_SetUnion(PyValue *set, PyValue *iterable);
PyAPI_FUNC(int) _PyRop_DictOverlay(PyValue *dict, PyValue *update);
PyAPI_FUNC(int) _PyRop_MergeKeywords(PyValue *fn, PyValue *dict, PyValue *update);

/* A Span<O> value: the count items that a publish `d[] = v` writes into
 * the homes of d, a count of each.  The items are those of a tuple or list
 * that the arm holds while it publishes: a temporary source taken from its
 * home, a local source in its home (parked if a destination displaces it),
 * or hold, a count of the container (a tuple the row made, or a tuple
 * source too long for one park).  No code runs between the row and the last
 * write of the first park, so a list's items stay as the row found them
 * (_PyRop_PublishItems()). */
typedef struct {
    const PyValue *items;
    PyObject *hold;
    Py_ssize_t count;
} _PyRopItems;

/* The displaced values that the rest of a publish holds until every home
 * of the park is written (_PyRop_PublishItemsRest()) */
#define _PY_ROP_PARK 16

PyAPI_FUNC(int) _PyRop_UnpackIterable(PyValue *source, Py_ssize_t n, _PyRopItems *out);
PyAPI_FUNC(int) _PyRop_UnpackStar(PyValue *source, Py_ssize_t shape, _PyRopItems *out);
PyAPI_FUNC(int) _PyRop_UnpackList(PyValue *source, Py_ssize_t n, _PyRopItems *out);
PyAPI_FUNC(int) _PyRop_ClassifyUnpack(PyValueBorrow v, Py_ssize_t n);
PyAPI_FUNC(bool) _PyRop_TupleHasLength(PyValueBorrow v, Py_ssize_t n);
PyAPI_FUNC(bool) _PyRop_ListHasLength(PyValueBorrow v, Py_ssize_t n);

/* The items of the exact tuple of n items in the source home (Unpack.Tuple).
 * A span longer than a park holds a count: a park's closes may close a
 * local source that a destination displaced. */
static inline _PyRopItems
_PyRop_UnpackTuple(PyValue *source_home, size_t n)
{
    PyObject *seq_o = PyValue_AsKnownObjectBorrow(PyValue_Borrow(*source_home));
    assert(PyTuple_CheckExact(seq_o) && (size_t)PyTuple_GET_SIZE(seq_o) == n);
    return (_PyRopItems){_PyTuple_ITEMS(seq_o),
                         n > _PY_ROP_PARK ? Py_NewRef(seq_o) : NULL,
                         (Py_ssize_t)n};
}

/* The count that ref owns is dropped, as PyValue_XCLOSE() drops it, but an
 * object that this leaves with no count is not deallocated: the result is
 * true, and the caller gives the object its last count back
 * (_PyValue_RestoreLast()) before any code runs, then closes it once no
 * code that the close could run may observe a half-done write.  The count
 * must be back before any code runs: a finalizer may collect garbage, which
 * must not find an object with no count.  The fast path is the decrement
 * and test of PyValue_XCLOSE().  The free-threaded build closes at once. */
static inline Py_ALWAYS_INLINE bool
_PyValue_DropNoDealloc(PyValue ref)
{
#ifdef Py_GIL_DISABLED
    PyValue_XCLOSE(ref);
    return false;
#else
    if (!PyValue_RefcountOnObject(ref)) {
        return false;
    }
    PyObject *op = BITS_TO_PTR(ref);
    assert(!_Py_IsStaticImmortal(op));
#ifdef Py_REF_DEBUG
    if (op->ob_refcnt <= 0) {
        _Py_NegativeRefcount(__FILE__, __LINE__, op);
    }
#endif
    if (Py_UNLIKELY(--op->ob_refcnt == 0)) {
        return true;
    }
#ifdef Py_REF_DEBUG
    if (!_Py_IsImmortal(op)) {
        _Py_DECREF_DecRefTotal();
    }
#endif
    _Py_DECREF_STAT_INC();
    return false;
#endif
}

/* The last count of an object that _PyValue_DropNoDealloc() left with none,
 * which ref owns again */
static inline void
_PyValue_RestoreLast(PyValue ref)
{
    PyObject *op = BITS_TO_PTR(ref);
    assert(op->ob_refcnt == 0);
    op->ob_refcnt = 1;
}

/* The rest of a publish whose element i displaced last, a value that
 * _PyValue_DropNoDealloc() left with no count; it takes the hold (in
 * ceval.c).  The span and the items come by value, so that no address of
 * the arm's frame escapes and the arm needs no stack protector. */
PyAPI_FUNC(void) _PyRop_PublishItemsRest(_PyRopSpan d, const PyValue *items,
                                         PyObject *hold, int i, PyValue last);

/* The publish `d[] = v` of a Span<O>: item i goes to home i of d, straight
 * through the word's element fields with the arm's constant layout (the
 * head's elements, then four to an operand word), and the value each home
 * held loses its count at once.  A value that would lose its last count
 * stops the publish: the result is the item's position, and that value is
 * written through last.  The caller hands the rest of the publish to
 * _PyRop_PublishItemsRest(), which parks it and the values the later homes
 * displace and closes them after the last write: no code runs before every
 * home is written, so a finalizer sees the publish done and the items, a
 * tuple's or a list's, cannot change under it (a span longer than a park
 * holds a count: a park's closes may close a local source that a
 * destination displaced).  Otherwise the result is -1, with every home
 * written and the hold closed.  A vacant home displaces NULL.  An arm
 * hands the rest over in its outlined cold region (rop_cgen.py,
 * lower_publish). */
static inline Py_ALWAYS_INLINE int
_PyRop_PublishItemsFast(const _PyRopSpan *d, _PyRopItems *v, PyValue *last)
{
    assert(v->count == d->count);
    assert(d->count <= _PY_ROP_PARK || v->hold != NULL);
    PyValue *homes = d->frame->base;
    int i = 0;
    int j = d->first;
    /* The head's fields, the last nhead of its four */
    for (; i < d->count && j < d->nhead; i++, j++) {
        PyValue *home = &homes[_PyIR_SpanElementAt(d->head, d->nhead, d->fixed, j)];
        PyValue old = *home;
        _PyFrame_WriteHome(d->frame, home, PyValue_DUP(PyValue_Borrow(v->items[i])));
        if (_PyValue_DropNoDealloc(old)) {
            *last = old;
            return i;
        }
    }
    /* The operand words: one load for four fields */
    int k = j - d->nhead;
    const _PyIRWord *w = d->head + 1 + d->fixed + k / IR_OPERANDS_PER_WORD;
    int left = 0;
    uint64_t fields = 0;
    if (i < d->count && k % IR_OPERANDS_PER_WORD) {
        left = IR_OPERANDS_PER_WORD - k % IR_OPERANDS_PER_WORD;
        fields = *w++ >> (16 + 12 * (k % IR_OPERANDS_PER_WORD));
    }
    for (; i < d->count; i++) {
        if (left == 0) {
            fields = *w++ >> 16;
            left = IR_OPERANDS_PER_WORD;
        }
        PyValue *home = &homes[fields & IR_FIELD_MAX];
        fields >>= 12;
        left--;
        PyValue old = *home;
        _PyFrame_WriteHome(d->frame, home, PyValue_DUP(PyValue_Borrow(v->items[i])));
        if (_PyValue_DropNoDealloc(old)) {
            *last = old;
            return i;
        }
    }
    Py_XDECREF(v->hold);
    v->hold = NULL;
    return -1;
}

/* The whole publish (_PyRop_PublishItemsFast()), for the completions and
 * the C rows */
static inline Py_ALWAYS_INLINE void
_PyRop_PublishItems(const _PyRopSpan *d, _PyRopItems *v)
{
    PyValue last;
    int i = _PyRop_PublishItemsFast(d, v, &last);
    if (i >= 0) {
        _PyRop_PublishItemsRest(*d, v->items, v->hold, i, last);
        v->hold = NULL;
    }
}

/* The values a call reads.  A local source is held by an owner of its own,
 * because the call can reassign the local; a temporary is held by its
 * home. */
typedef struct {
    uint8_t n;
    PyValueBorrow view[_PY_ROP_MAX_SRCS];
    PyValue owned[_PY_ROP_MAX_SRCS];
} _PyRopAnchors;

/* The sources of a span operand (at most four), for a call that reads them */
PyAPI_FUNC(_PyRopSrcs) _PyRop_SrcsOfSpan(const _PyRopSpan *span);

static inline Py_ALWAYS_INLINE _PyRopAnchors
_PyRop_AnchorForSrcs(const _PyRopSrcs *s)
{
    assert(s->n <= _PY_ROP_MAX_SRCS);
    _PyRopAnchors a = {.n = s->n};
    for (int i = 0; i < s->n; i++) {
        a.view[i] = PyValue_Borrow(*s->home[i]);
        a.owned[i] = s->local[i] ? PyValue_DUP(a.view[i]) : PyValue_NULL;
    }
    return a;
}

/* Closes the owners an anchor holds (the `roots` debt) */
static inline void
_PyRop_CloseAnchors(_PyRopAnchors *a)
{
    for (int i = 0; i < a->n; i++) {
        PyValue owner = a->owned[i];
        a->owned[i] = PyValue_NULL;
        PyValue_XCLOSE(owner);
    }
}

/* The failure of a call: the anchors' owners, then each temporary source
 * in the order listed, are closed with the pending exception set aside. */
static inline void
_PyRop_RetireFailedSrcs(_PyRopAnchors *a, const _PyRopSrcs *s)
{
    PyObject *exc = PyErr_GetRaisedException();
    _PyRop_CloseAnchors(a);
    for (int i = 0; i < s->n; i++) {
        if (!s->local[i]) {
            PyValue_XCLOSE(_PyFrame_TakeHome(s->frame, s->home[i]));
        }
    }
    PyErr_SetRaisedException(exc);
}

/* The temporary sources of a span, taken out of their homes and closed */
static inline void
_PyRop_RetireSources(const _PyRopSpan *span)
{
    _PyRopSrcs s = _PyRop_SrcsOfSpan(span);
    for (int i = 0; i < s.n; i++) {
        if (!s.local[i]) {
            PyValue_XCLOSE(_PyFrame_TakeHome(s.frame, s.home[i]));
        }
    }
}

/* The sources of a call are the listed operands or a span */
#define _PyRop_AnchorForCall(...) \
    _Generic((__VA_ARGS__), \
        const _PyRopSpan *: _PyRop_AnchorForSpan, \
        _PyRopSpan *: _PyRop_AnchorForSpan, \
        default: _PyRop_AnchorForSrcs)(__VA_ARGS__)
#define _PyRop_RetireFailedCall(A, ...) \
    _Generic((__VA_ARGS__), \
        const _PyRopSpan *: _PyRop_RetireFailedSpan, \
        _PyRopSpan *: _PyRop_RetireFailedSpan, \
        default: _PyRop_RetireFailedSrcs)(A, __VA_ARGS__)

static inline _PyRopAnchors
_PyRop_AnchorForSpan(const _PyRopSpan *span)
{
    _PyRopSrcs s = _PyRop_SrcsOfSpan(span);
    return _PyRop_AnchorForSrcs(&s);
}

static inline void
_PyRop_RetireFailedSpan(_PyRopAnchors *a, const _PyRopSpan *span)
{
    _PyRopSrcs s = _PyRop_SrcsOfSpan(span);
    _PyRop_RetireFailedSrcs(a, &s);
}

static inline void
_PyRop_RetireAbandoned(_PyRopAnchors *a)
{
    _PyRop_CloseAnchors(a);
}

static inline PyValueBorrow
_PyRop_AnchorArg(const _PyRopAnchors *a, size_t i)
{
    assert(i < a->n);
    return a->view[i];
}

/* The poll's fast path (truth.rop, events_pending): an event is pending for
 * _Py_HandlePending(), by the one relaxed load and test of the eval breaker
 * that check_periodics() makes first */
static inline bool
_PyRop_EventsPending(PyThreadState *tstate)
{
    return (_Py_atomic_load_uintptr_relaxed(&tstate->eval_breaker)
            & _PY_EVAL_EVENTS_MASK) != 0;
}

static inline bool
_PyRop_RedirectPending(_PyInterpreterFrame *frame)
{
    return frame->pending_redirect;
}

static inline void
_PyRop_DiscardResult(PyValue z)
{
    PyValue_XCLOSE(z);
}

/* GlobalRead: the name in the frame's globals, then its builtins; an owned
 * value, or PyValue_NULL with an exception set */
static inline PyValue
_PyRop_LoadGlobal(_PyInterpreterFrame *frame, PyObject *name)
{
    PyValue v = PyValue_NULL;
    _PyEval_LoadGlobalValue(frame->f_globals, frame->f_builtins, name, &v);
    return v;
}

/* Add: floats, text and the extension descriptor */

static inline bool
_PyRop_FloatOperands(PyValueBorrow a, PyValueBorrow b)
{
    return PyValue_FloatOperandsCheck(a, b);
}

static inline double
_PyRop_AddDoubles(double x, double y)
{
    return x + y;
}

static inline bool
_PyRop_BothExactStr(PyValueBorrow a, PyValueBorrow b)
{
    return _PyRop_IsObject(a) && _PyRop_IsObject(b) &&
           PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(a)) &&
           PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(b));
}

static inline PyValue
_PyRop_ConcatText(PyValueBorrow a, PyValueBorrow b)
{
    PyObject *res = PyUnicode_Concat(PyValue_AsKnownObjectBorrow(a),
                                     PyValue_AsKnownObjectBorrow(b));
    if (res == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromNonNumberSteal(res);
}

static inline bool
_PyRop_ExtensionApplies(_PyFbBinary *fb, PyValueBorrow a, PyValueBorrow b)
{
    const _PyBinaryOpSpecializationDescr *d =
        _PyBinaryOp_ExtendDescr((PyObject *)fb->descr);
    // The descriptors guard objects: inline operands go to the generic arm
    return d != NULL && _PyRop_IsObject(a) && _PyRop_IsObject(b) &&
           Py_TYPE(PyValue_AsKnownObjectBorrow(a)) == d->lhs_type &&
           Py_TYPE(PyValue_AsKnownObjectBorrow(b)) == d->rhs_type;
}

static inline PyValue
_PyRop_RunExtension(_PyFbBinary *fb, PyValueBorrow a, PyValueBorrow b)
{
    const _PyBinaryOpSpecializationDescr *d =
        _PyBinaryOp_ExtendDescr((PyObject *)fb->descr);
    PyObject *res = d->action(PyValue_AsKnownObjectBorrow(a),
                              PyValue_AsKnownObjectBorrow(b));
    if (res == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(res);
}

/* Binary: the operations other than ordinary addition (numeric.rop).  An
 * arm tests the operand kinds, decodes the operands once, and tests that
 * the operator admits them (`op_admits`); the compute takes the decoded
 * values.  The adaptation macro samples with `applies`, the same two tests,
 * so a site never installs an arm it would miss at once. */

/* b is the scaled right operand: zero or negative exactly when its int is */
static inline bool
_PyRop_IntOpAdmits(uint32_t op, int64_t b)
{
    switch (op) {
        case NB_INPLACE_ADD:
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
        case NB_AND:
        case NB_INPLACE_AND:
        case NB_OR:
        case NB_INPLACE_OR:
        case NB_XOR:
        case NB_INPLACE_XOR:
            return true;
        case NB_FLOOR_DIVIDE:
        case NB_INPLACE_FLOOR_DIVIDE:
        case NB_REMAINDER:
        case NB_INPLACE_REMAINDER:
            return b != 0;
        case NB_LSHIFT:
        case NB_INPLACE_LSHIFT:
        case NB_RSHIFT:
        case NB_INPLACE_RSHIFT:
            return b >= 0;
        default:
            return false;
    }
}

static inline bool
_PyRop_IntBinaryApplies(uint32_t op, PyValueBorrow a, PyValueBorrow b)
{
    return PyValue_BothInlineInt(a, b) && _PyRop_IntOpAdmits(op, _PyValue_IntScaled(b));
}

/* The result of an admitted pair: inline unless it leaves the int51 range,
 * where it is boxed.  Python's // rounds towards minus infinity and % takes
 * the sign of the divisor, where C truncates; int51 operands do not trap in
 * int64 division. */
static inline PyValue
_PyRop_IntBinary(uint32_t op, int64_t a, int64_t b)
{
    assert(_PyRop_IntOpAdmits(op, b));
    int64_t x = _PyValue_UnscaleInt(a);
    int64_t y = _PyValue_UnscaleInt(b);
    switch (op) {
        case NB_INPLACE_ADD: {
            int64_t sum;
            if (__builtin_add_overflow(a, b, &sum)) {
                return _PyValue_BoxInt64(x + y);
            }
            return _PyValue_FromIntScaled(sum);
        }
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT: {
            int64_t difference;
            if (__builtin_sub_overflow(a, b, &difference)) {
                return _PyValue_BoxInt64(x - y);
            }
            return _PyValue_FromIntScaled(difference);
        }
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY: {
            int64_t product;
            if (__builtin_mul_overflow(a, y, &product)) {
                return _PyValue_MultiplyInt64(x, y);
            }
            return _PyValue_FromIntScaled(product);
        }
        case NB_FLOOR_DIVIDE:
        case NB_INPLACE_FLOOR_DIVIDE: {
            int64_t quotient = x / y;
            if (quotient * y != x && (x ^ y) < 0) {
                quotient--;
            }
            return PyValue_FromInt64(quotient);
        }
        case NB_REMAINDER:
        case NB_INPLACE_REMAINDER: {
            int64_t remainder = x % y;
            if (remainder != 0 && (remainder ^ y) < 0) {
                remainder += y;
            }
            return PyValue_FromInlineInt(remainder);
        }
        /* Ints behave as infinite two's complement under &, | and ^, so the
         * result for int51 operands is the int51 result. */
        case NB_AND:
        case NB_INPLACE_AND:
            return PyValue_FromInlineInt(x & y);
        case NB_OR:
        case NB_INPLACE_OR:
            return PyValue_FromInlineInt(x | y);
        case NB_XOR:
        case NB_INPLACE_XOR:
            return PyValue_FromInlineInt(x ^ y);
        case NB_LSHIFT:
        case NB_INPLACE_LSHIFT: {
            /* A shift whose result fits in int64 is made there; a longer
             * shift of anything but 0 makes a long int. */
            int64_t shifted = y < 63 ? (int64_t)((uint64_t)x << y) : 0;
            if (x == 0) {
                return PyValue_FromInlineInt(0);
            }
            if (y < 63 && Py_ARITHMETIC_RIGHT_SHIFT(int64_t, shifted, y) == x) {
                return PyValue_FromInt64(shifted);
            }
            return _PyValue_LshiftInt64(x, y);
        }
        case NB_RSHIFT:
        case NB_INPLACE_RSHIFT:
            /* An int51 shifted by 63 places or more keeps its sign. */
            return PyValue_FromInlineInt(
                Py_ARITHMETIC_RIGHT_SHIFT(int64_t, x, y < 63 ? y : 63));
        default:
            Py_UNREACHABLE();
    }
}

static inline double
_PyRop_FloatValue(uint32_t op, double x, double y)
{
    switch (op) {
        case NB_INPLACE_ADD:
            return x + y;
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
            return x * y;
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            return x - y;
        case NB_TRUE_DIVIDE:
        case NB_INPLACE_TRUE_DIVIDE:
            return x / y;
        case NB_POWER:
        case NB_INPLACE_POWER:
            return pow(x, y);
        default:
            return 0.0;
    }
}

/* float_pow returns pow(x, y) itself for a positive finite base and a finite
 * exponent unless the result overflows (OverflowError) or underflows to a
 * nonzero subnormal, where errno decides; every other case is special-cased
 * there or raises, and is the generic arm's. */
static inline bool
_PyRop_FloatOpAdmits(uint32_t op, double x, double y, double z)
{
    switch (op) {
        case NB_INPLACE_ADD:
        case NB_MULTIPLY:
        case NB_INPLACE_MULTIPLY:
        case NB_SUBTRACT:
        case NB_INPLACE_SUBTRACT:
            return true;
        case NB_TRUE_DIVIDE:
        case NB_INPLACE_TRUE_DIVIDE:
            return y != 0.0;
        case NB_POWER:
        case NB_INPLACE_POWER:
            return x > 0.0 && isfinite(x) && isfinite(y)
                   && (z == 0.0 || isnormal(z));
        default:
            return false;
    }
}

static inline bool
_PyRop_FloatBinaryApplies(uint32_t op, PyValueBorrow a, PyValueBorrow b)
{
    double x, y;
    return PyValue_FloatOperandValues(a, b, &x, &y)
           && _PyRop_FloatOpAdmits(op, x, y, _PyRop_FloatValue(op, x, y));
}

/* Text: only the in-place add of two exact strings reaches Binary; the
 * plain add is Add's. */
static inline bool
_PyRop_TextBinaryApplies(uint32_t op, PyValueBorrow a, PyValueBorrow b)
{
    return op == NB_INPLACE_ADD && _PyRop_BothExactStr(a, b);
}

/* The rows of the constants and locals (Python/register_ops/values.rop).
 * A home operand is passed as the address of its home. */
static inline PyValue
_PyRop_CodeConstant(_PyInterpreterFrame *frame, uint32_t i)
{
    PyValueBorrow c = _PyValue_CodeConstant(
        PyTuple_GET_VALUE(_PyFrame_GetCode(frame)->co_consts, i));
    assert(!PyValue_IsNull(c));
    return PyValue_DUP(c);
}

static inline PyValue
_PyRop_SmallInt(uint32_t i)
{
    assert(i < _PY_NSMALLPOSINTS);
    return PyValue_FromInlineInt((int)i);
}

static inline PyValue
_PyRop_CommonConstant(uint32_t i)
{
    assert(i < NUM_COMMON_CONSTANTS);
    PyValueBorrow c = _PyValue_CommonConstant(
        _PyInterpreterState_GET()->common_consts[i]);
    return PyValue_DUP(c);
}

static inline PyValueBorrow
_PyRop_LocalView(PyValue *home)
{
    return PyValue_Borrow(*home);
}

/* Whether an optional source operand's home holds a value */
static inline bool
_PyRop_SrcPresent(PyValue *home)
{
    return !PyValue_IsNull(*home);
}

/* The saved binding in a temporary, absent or not, taken out of its home;
 * the arm publishes it into the local */
static inline PyValue
_PyRop_RestoreLocal(PyValue *value)
{
    PyValue saved = *value;
    *value = PyValue_NULL;
    return saved;
}

/* The local's binding, absent or not, taken out of its home; the arm
 * publishes it into dst */
static inline PyValue
_PyRop_SaveLocal(PyValue *local)
{
    PyValue saved = *local;
    *local = PyValue_NULL;
    return saved;
}

/* The rows of the cell families (Python/register_ops/cells.rop).  A Cell
 * operand's home holds a cell object, which MAKE_CELL or frame entry put
 * there before any instruction reads it. */
static inline PyCellObject *
_PyRop_CellIn(PyValue *home)
{
    PyObject *cell = PyValue_AsKnownObjectBorrow(PyValue_Borrow(*home));
    assert(PyCell_Check(cell));
    return (PyCellObject *)cell;
}

/* The contents of the cell as a new owner; false when it is empty */
static inline bool
_PyRop_CellContents(PyValue *home, PyValue *out)
{
    PyValue v = _PyCell_GetValue(_PyRop_CellIn(home));
    *out = v;
    return !PyValue_IsNull(v);
}

/* v moves into the cell; false when it was empty, else the previous owner
 * comes out for the caller to close */
static inline bool
_PyRop_CellExchange(PyValue *home, PyValue v, PyValue *old)
{
    PyValue previous = _PyCell_SwapValue(_PyRop_CellIn(home), v);
    *old = previous;
    return !PyValue_IsNull(previous);
}

/* The cell emptied; false when it was already empty */
static inline bool
_PyRop_CellTake(PyValue *home, PyValue *old)
{
    return _PyRop_CellExchange(home, PyValue_NULL, old);
}

/* Python/ceval.c: the error of an empty cell */
void _PyRop_UnboundCell(_PyInterpreterFrame *frame, PyValue *home);
/* The binding taken out of the local into a new cell, or NULL with the
 * exception set (the binding is closed) */
PyValue _PyRop_MakeCell(_PyInterpreterFrame *frame, PyValue *home);
PyValue _PyRop_GetAwaitable(PyValue x, uint32_t origin);
PyValue _PyRop_GetANext(PyValue *aiter_home);
PyAPI_FUNC(_PyStatusValue) _PyRop_LoadFromDictCell(
    PyThreadState *, _PyInterpreterFrame *, PyValueBorrow, PyValue *);

/* Python/ceval.c: UnboundLocalError for the local in home */
void _PyRop_UnboundLocal(_PyInterpreterFrame *frame, PyValue *home);
/* The local's binding taken out of its home, or NULL with UnboundLocalError
 * set when it is unbound */
PyValue _PyRop_DeleteLocal(_PyInterpreterFrame *frame, PyValue *home);
PyValue _PyRop_AppendText(PyValue *home, PyValueBorrow right);


/* The name at index in the code's co_names: every name operand of an arm
 * and the namespace rows (Python/register_ops/namespaces.rop), which pass
 * the index because an operand holds it in 16 bits, wider than a name
 * field.  Code construction admits only exact str names
 * (Objects/codeobject.c, intern_strings), so the word is an object and the
 * read is checked in debug builds only. */
static inline PyObject *
_PyRop_NameAt(_PyInterpreterFrame *frame, uint32_t index)
{
    PyValueBorrow name = PyTuple_GET_VALUE(_PyFrame_GetCode(frame)->co_names, index);
    assert(PyValue_IsObject(name) &&
           PyUnicode_CheckExact(_PyValue_AsKnownObjectBorrowPointer(name)));
    return _PyValue_AsKnownObjectBorrowPointer(name);
}

PyAPI_FUNC(PyValue) _PyRop_LoadName(PyThreadState *, _PyInterpreterFrame *, uint32_t);
PyAPI_FUNC(PyValue) _PyRop_LoadLocals(PyThreadState *, _PyInterpreterFrame *);
PyAPI_FUNC(PyValue) _PyRop_LoadBuildClass(PyThreadState *, _PyInterpreterFrame *);
PyAPI_FUNC(_PyStatusValue) _PyRop_LoadFromDict(PyThreadState *, _PyInterpreterFrame *,
                                               PyValueBorrow, uint32_t);
PyAPI_FUNC(PyValue) _PyRop_LoadGlobalOrBuiltin(_PyInterpreterFrame *, uint32_t);
PyAPI_FUNC(int) _PyRop_StoreName(PyThreadState *, _PyInterpreterFrame *,
                                 uint32_t, PyValue);
PyAPI_FUNC(int) _PyRop_DeleteName(PyThreadState *, _PyInterpreterFrame *, uint32_t);
PyAPI_FUNC(int) _PyRop_StoreGlobal(PyThreadState *, _PyInterpreterFrame *,
                                   uint32_t, PyValue);
PyAPI_FUNC(int) _PyRop_DeleteGlobal(PyThreadState *, _PyInterpreterFrame *, uint32_t);
PyAPI_FUNC(int) _PyRop_SetupAnnotations(PyThreadState *, _PyInterpreterFrame *);
PyAPI_FUNC(PyValue) _PyRop_ImportName(PyThreadState *, _PyInterpreterFrame *,
                                      PyObject *, PyValue, PyValue, uint32_t);
PyAPI_FUNC(PyValue) _PyRop_ImportFrom(PyThreadState *, _PyInterpreterFrame *,
                                      PyValue, uint32_t);

/* Truth, BranchTruth and BranchNone (Python/register_ops/truth.rop).  Of
 * BranchMode's members (pycore_rop_metadata.h), the jump is taken when the
 * test is true in the odd modes, false in the even ones, and the KEEP modes
 * leave the owner in its home on the jump edge. */

static inline bool
_PyRop_BranchTaken(int mode, bool test)
{
    return test == (bool)(mode & 1);
}

static inline bool
_PyRop_BranchKeeps(int mode)
{
    return mode >= ROP_BRANCHMODE_BRANCH_KEEP_IF_FALSE;
}

static inline bool
_PyRop_SameBool(bool a, bool b)
{
    return a == b;
}

static inline PyValue
_PyRop_TruthObject(bool test, bool invert)
{
    return test != invert ? PyValue_True : PyValue_False;
}

static inline bool
_PyRop_NotNone(PyValueBorrow v)
{
    return !PyValue_IsNone(v);
}

static inline bool
_PyRop_ExactInt(PyValueBorrow v)
{
    return PyValue_LongCheckExact(v);
}

static inline bool
_PyRop_IntNonzero(PyValueBorrow v)
{
    if (PyValue_IsInlineInt(v)) {
        return PyValue_InlineInt64(v) != 0;
    }
    return !_PyLong_IsZero((PyLongObject *)PyValue_AsKnownObjectBorrow(v));
}

static inline bool
_PyRop_ExactList(PyValueBorrow v)
{
    return _PyRop_IsObject(v) && PyList_CheckExact(PyValue_AsKnownObjectBorrow(v));
}

static inline bool
_PyRop_ListNonempty(PyValueBorrow v)
{
    return PyList_GET_SIZE(PyValue_AsKnownObjectBorrow(v)) != 0;
}

static inline bool
_PyRop_ExactStr(PyValueBorrow v)
{
    return _PyRop_IsObject(v) && PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(v));
}

static inline bool
_PyRop_StrNonempty(PyValueBorrow v)
{
    return PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(v)) != 0;
}

/* An object whose type has the version the entry holds (an entry that
 * holds none matches nothing) */
static inline bool
_PyRop_HasEntryVersion(const _PyFbUnary *fb, PyValueBorrow v)
{
    if (!_PyRop_IsObject(v) || fb->type_version == 0) {
        return false;
    }
    PyTypeObject *tp = Py_TYPE(PyValue_AsKnownObjectBorrow(v));
    return FT_ATOMIC_LOAD_UINT_RELAXED(tp->tp_version_tag) == fb->type_version;
}

/* A word's call may be to a stub that its code was compiled with, after the
 * word was adapted to another arm: the entry then holds that arm's type
 * version, so these guards test the kind of value as well as the version. */
static inline bool
_PyRop_NoneOrPlain(const _PyFbUnary *fb, PyValueBorrow v)
{
    if (PyValue_IsNone(v)) {
        return true;
    }
    return _PyRop_HasEntryVersion(fb, v)
        && PyType_HasFeature(Py_TYPE(PyValue_AsKnownObjectBorrow(v)), Py_TPFLAGS_HEAPTYPE);
}

static inline bool
_PyRop_SlotApplies(const _PyFbUnary *fb, PyValueBorrow v)
{
    return _PyRop_HasEntryVersion(fb, v) && !PyValue_IsNone(v)
        && !PyValue_BoolCheck(v);
}

/* The truth of any value, and of an object whose type's slots decide */
static inline int
_PyRop_ValueTruth(PyValueBorrow v)
{
    return _PyValue_IsTrue(v);
}

static inline int
_PyRop_SlotTruth(PyValueBorrow v)
{
    return _PyValue_IsTrueSlow(v);
}

/* Python/register_ops/tests.rop: Compare, Identity, Contains, TestBranch */

static inline bool
_PyRop_TestIsIdentity(int test)
{
    return test == ROP_BRANCHTEST_IDENTITY;
}

static inline bool
_PyRop_TestIsContains(int test)
{
    return test == ROP_BRANCHTEST_CONTAINS;
}

static inline int
_PyRop_RelationOf(int test)
{
    return test >= ROP_BRANCHTEST_TEST_LT ? test - ROP_BRANCHTEST_TEST_LT : ROP_RELATION_LT;
}

static inline bool
_PyRop_ResultIsTruth(int result)
{
    return result == ROP_COMPARERESULT_TRUTH;
}

static inline bool
_PyRop_EqualityRelation(int relation)
{
    return relation == ROP_RELATION_EQ || relation == ROP_RELATION_NE;
}

static inline bool
_PyRop_BothInlineFloat(PyValueBorrow a, PyValueBorrow b)
{
    return PyValue_IsInlineFloat(a) && PyValue_IsInlineFloat(b);
}

static inline bool
_PyRop_CompareScaled(int relation, int64_t a, int64_t b)
{
    switch (relation) {
        case ROP_RELATION_LT: return a < b;
        case ROP_RELATION_LE: return a <= b;
        case ROP_RELATION_EQ: return a == b;
        case ROP_RELATION_NE: return a != b;
        case ROP_RELATION_GT: return a > b;
        default: return a >= b;
    }
}

/* IEEE: every relation but NE is false when either is NaN */
static inline bool
_PyRop_CompareFloats(int relation, PyValueBorrow a, PyValueBorrow b)
{
    double x = PyValue_InlineFloat(a);
    double y = PyValue_InlineFloat(b);
    switch (relation) {
        case ROP_RELATION_LT: return x < y;
        case ROP_RELATION_LE: return x <= y;
        case ROP_RELATION_EQ: return x == y;
        case ROP_RELATION_NE: return x != y;
        case ROP_RELATION_GT: return x > y;
        default: return x >= y;
    }
}

static inline bool
_PyRop_EqualStrings(int relation, PyValueBorrow a, PyValueBorrow b)
{
    assert(relation == ROP_RELATION_EQ || relation == ROP_RELATION_NE);
    int eq = _PyUnicode_Equal(PyValue_AsKnownObjectBorrow(a),
                              PyValue_AsKnownObjectBorrow(b));
    return (relation == ROP_RELATION_EQ) == (eq != 0);
}

static inline PyValue
_PyRop_RichCompare(int relation, PyValueBorrow a, PyValueBorrow b)
{
    return _PyValue_RichCompare(a, b, relation);
}

/* The comparison as a truth value: the rich result is closed before the
 * conversion's failure is reported */
static inline int
_PyRop_CompareTruth(int relation, PyValueBorrow a, PyValueBorrow b)
{
    PyValue cmp = _PyValue_RichCompare(a, b, relation);
    if (PyValue_IsNull(cmp)) {
        return -1;
    }
    int res = _PyValue_IsTrue(cmp);
    PyValue_CLOSE(cmp);
    return res;
}

static inline bool
_PyRop_ExactAnySet(PyValueBorrow c)
{
    return _PyRop_IsObject(c) && PyAnySet_CheckExact(PyValue_AsKnownObjectBorrow(c));
}

static inline bool
_PyRop_ExactAnyDict(PyValueBorrow c)
{
    return _PyRop_IsObject(c) && PyAnyDict_CheckExact(PyValue_AsKnownObjectBorrow(c));
}

static inline bool
_PyRop_ExactSequence(PyValueBorrow c)
{
    if (!_PyRop_IsObject(c)) {
        return false;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(c);
    return PyUnicode_CheckExact(o) || PyTuple_CheckExact(o) || PyList_CheckExact(o);
}

static inline int
_PyRop_SetContains(PyValueBorrow c, PyValueBorrow item)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(c);
    assert(PyAnySet_CheckExact(o));
    return _PySet_Contains((PySetObject *)o, item);
}

static inline int
_PyRop_DictContains(PyValueBorrow c, PyValueBorrow item)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(c);
    assert(PyAnyDict_CheckExact(o));
    return _PyDict_ContainsValue((PyDictObject *)o, item);
}

static inline int
_PyRop_SequenceContains(PyValueBorrow c, PyValueBorrow item)
{
    PyObject *o = PyValue_AsKnownObjectBorrow(c);
    _Py_containsvaluefunc contains_slot = _PyType_VALUE_SLOT(Py_TYPE(o), contains);
    return contains_slot(o, item);
}

static inline int
_PyRop_ValueContains(PyValueBorrow c, PyValueBorrow item)
{
    return _PyValue_Contains(c, item);
}

/* The case of cases(set, dict, sequence, none) */
static inline int
_PyRop_ClassifyContains(PyValueBorrow c)
{
    if (_PyRop_ExactAnyDict(c)) {
        return 1;
    }
    if (_PyRop_ExactAnySet(c)) {
        return 0;
    }
    return _PyRop_ExactSequence(c) ? 2 : 3;
}

/* Python/specialize.c: the adaptation rows */
bool _PyRop_FindExtension(_PyFbBinary *fb, uint32_t op, PyValueBorrow a, PyValueBorrow b);
void _PyRop_Backoff(_PyFbHeader *fb);
void _PyRop_RecordOperands(_PyFbBinary *fb, PyValueBorrow a, PyValueBorrow b);
int _PyRop_Install(_PyInstr *site, _PyFbHeader *fb, int handler);
void _PyRop_BeginTruth(_PyFbUnary *fb, PyValueBorrow v);
bool _PyRop_ValidatePlain(_PyFbUnary *fb, PyValueBorrow v);
bool _PyRop_ValidateGenMethod(PyValueBorrow v);
bool _PyRop_ValidateSlot(_PyFbUnary *fb, PyValueBorrow v);
int _PyRop_ClassifyGetItem(_PyFbBinary *fb, PyValueBorrow v, PyValueBorrow k);
int _PyRop_ClassifySetItem(_PyFbBinary *fb, PyValueBorrow v, PyValueBorrow k);
int _PyRop_ClassifyCall(_PyInstr *site, _PyFbCall *fb, PyValueBorrow fn,
                        const _PyIRWord *head);
int _PyRop_ClassifyCallKw(_PyInstr *site, _PyFbCall *fb, PyValueBorrow fn,
                          const _PyIRWord *head);
int _PyRop_ClassifyCallEx(_PyInstr *site, _PyFbCall *fb, PyValueBorrow fn,
                          const _PyIRWord *head);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ROP_ROWS_H */
