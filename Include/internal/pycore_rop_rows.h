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
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_runtime_structs.h" // _PY_NSMALLPOSINTS
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()
#include "pycore_dict.h"          // _PyDict_ContainsValue()
#include "pycore_setobject.h"     // _PySet_Contains()
#include "pycore_unicodeobject.h" // _PyUnicode_Equal()
#include "pycore_value_slots.h"   // _PyType_VALUE_SLOT()

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

/* A span operand of the instruction whose head is head, which runs as
 * handler (pycore_ir.h, _PyIR_SpanElement()): count homes from element
 * first of the word's spans */
typedef struct {
    _PyInterpreterFrame *frame;
    const _PyIRWord *head;
    int handler;
    int first;
    int count;
} _PyRopSpan;

/* The rows of the Move arms, in ceval.c */
PyAPI_FUNC(void) _PyFrame_ParallelMove(_PyInterpreterFrame *frame, int n,
                                       const int32_t *src, const int32_t *dst,
                                       int stride);
PyAPI_FUNC(void) _PyFrame_ApplyRecipe(_PyInterpreterFrame *frame,
                                      const int32_t *recipe);
PyAPI_FUNC(void) _PyRop_ParallelMove(const _PyRopSpan *d, const _PyRopSpan *s);
PyAPI_FUNC(void) _PyRop_CloseRest(const _PyRopSpan *d, const _PyRopSpan *s);
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

/* A Span<O> value: count owners in storage from PyMem_Malloc (NULL when
 * count is 0), which the row that makes it allocates.  A publish
 * `d[] = v` exchanges each item with the owner its home displaces
 * (_PyRop_StoreOwners); old(d) closes those and frees the storage
 * (_PyRop_CloseOwners). */
typedef struct {
    PyValue *items;
    Py_ssize_t count;
} _PyRopOwners;

PyAPI_FUNC(void) _PyRop_StoreOwners(const _PyRopSpan *d, _PyRopOwners *v);
PyAPI_FUNC(void) _PyRop_CloseOwners(_PyRopOwners *v);

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

static inline _PyRopAnchors
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

static inline PyValue
_PyRop_AddFloats(PyValueBorrow a, PyValueBorrow b)
{
    double x, y;
    bool admitted = PyValue_FloatOperandValues(a, b, &x, &y);
    assert(admitted);
    (void)admitted;
    return PyValue_FromDouble(x + y);
}

static inline bool
_PyRop_BothExactStr(PyValueBorrow a, PyValueBorrow b)
{
    return PyValue_IsObject(a) && PyValue_IsObject(b) &&
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
    return d != NULL && PyValue_IsObject(a) && PyValue_IsObject(b) &&
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
PyAPI_FUNC(_PyStatusValue) _PyRop_LoadFromDictCell(
    PyThreadState *, _PyInterpreterFrame *, PyValueBorrow, PyValue *);

/* Python/ceval.c: UnboundLocalError for the local in home */
void _PyRop_UnboundLocal(_PyInterpreterFrame *frame, PyValue *home);
/* The local's binding taken out of its home, or NULL with UnboundLocalError
 * set when it is unbound */
PyValue _PyRop_DeleteLocal(_PyInterpreterFrame *frame, PyValue *home);


/* The members of ImportKind in declaration order */
enum { IMPORT_EAGER, IMPORT_POLICY, IMPORT_LAZY };

/* Python/ceval.c: the namespace rows (Python/register_ops/namespaces.rop).
 * A name is passed as its index in co_names, which an operand holds in 16
 * bits, wider than a name field. */
static inline PyObject *
_PyRop_NameAt(_PyInterpreterFrame *frame, uint32_t index)
{
    return _PyTuple_GET_OBJECT(_PyFrame_GetCode(frame)->co_names, index);
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

/* Truth, BranchTruth and BranchNone (Python/register_ops/truth.rop).  The
 * members of BranchMode in declaration order, the numbers the generated
 * handlers use for the enum's names: the jump is taken when the test is
 * true in the odd modes, false in the even ones, and the KEEP modes leave
 * the owner in its home on the jump edge. */
enum {
    BRANCH_IF_FALSE,
    BRANCH_IF_TRUE,
    BRANCH_KEEP_IF_FALSE,
    BRANCH_KEEP_IF_TRUE,
};

static inline bool
_PyRop_BranchTaken(int mode, bool test)
{
    return test == (bool)(mode & 1);
}

static inline bool
_PyRop_BranchKeeps(int mode)
{
    return mode >= BRANCH_KEEP_IF_FALSE;
}

static inline bool
_PyRop_BranchDrops(int mode)
{
    return mode < BRANCH_KEEP_IF_FALSE;
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
    return PyValue_IsObject(v) && PyList_CheckExact(PyValue_AsKnownObjectBorrow(v));
}

static inline bool
_PyRop_ListNonempty(PyValueBorrow v)
{
    return PyList_GET_SIZE(PyValue_AsKnownObjectBorrow(v)) != 0;
}

static inline bool
_PyRop_ExactStr(PyValueBorrow v)
{
    return PyValue_IsObject(v) && PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(v));
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
    if (!PyValue_IsObject(v) || fb->type_version == 0) {
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

/* The enums of tests.rop, in declaration order */
enum {
    ROP_RELATION_LT, ROP_RELATION_LE, ROP_RELATION_EQ, ROP_RELATION_NE,
    ROP_RELATION_GT, ROP_RELATION_GE,
};
enum { ROP_RESULT_RICH, ROP_RESULT_TRUTH };
enum {
    ROP_TEST_IDENTITY, ROP_TEST_CONTAINS, ROP_TEST_LT, ROP_TEST_LE, ROP_TEST_EQ,
    ROP_TEST_NE, ROP_TEST_GT, ROP_TEST_GE,
};

static inline bool
_PyRop_TestIsIdentity(int test)
{
    return test == ROP_TEST_IDENTITY;
}

static inline bool
_PyRop_TestIsContains(int test)
{
    return test == ROP_TEST_CONTAINS;
}

static inline int
_PyRop_RelationOf(int test)
{
    return test >= ROP_TEST_LT ? test - ROP_TEST_LT : ROP_RELATION_LT;
}

static inline bool
_PyRop_ResultIsTruth(int result)
{
    return result == ROP_RESULT_TRUTH;
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
    return PyValue_IsObject(c) && PyAnySet_CheckExact(PyValue_AsKnownObjectBorrow(c));
}

static inline bool
_PyRop_ExactAnyDict(PyValueBorrow c)
{
    return PyValue_IsObject(c) && PyAnyDict_CheckExact(PyValue_AsKnownObjectBorrow(c));
}

static inline bool
_PyRop_ExactSequence(PyValueBorrow c)
{
    if (!PyValue_IsObject(c)) {
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
bool _PyRop_FindAddExtension(_PyFbBinary *fb, PyValueBorrow a, PyValueBorrow b);
void _PyRop_Backoff(_PyFbHeader *fb);
int _PyRop_Install(_PyInstr *site, _PyFbHeader *fb, int handler);
void _PyRop_BeginTruth(_PyFbUnary *fb, PyValueBorrow v);
bool _PyRop_ValidatePlain(_PyFbUnary *fb, PyValueBorrow v);
bool _PyRop_ValidateSlot(_PyFbUnary *fb, PyValueBorrow v);
int _PyRop_ClassifyGetItem(_PyFbBinary *fb, PyValueBorrow v, PyValueBorrow k);
int _PyRop_ClassifySetItem(_PyFbBinary *fb, PyValueBorrow v, PyValueBorrow k);
int _PyRop_ClassifyCall(_PyInstr *site, _PyFbCall *fb, PyValueBorrow fn,
                        const _PyIRWord *head);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ROP_ROWS_H */
