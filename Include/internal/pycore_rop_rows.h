#ifndef Py_INTERNAL_ROP_ROWS_H
#define Py_INTERNAL_ROP_ROWS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_code.h"          // _PyBinaryOp_ExtendDescr()
#include "pycore_feedback.h"      // _PyFbBinary
#include "pycore_interpframe.h"   // _PyFrame_TakeHome()
#include "pycore_value.h"         // PyValue

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

/* The values a call reads.  A local source is held by an owner of its own,
 * because the call can reassign the local; a temporary is held by its
 * home. */
typedef struct {
    uint8_t n;
    PyValueBorrow view[_PY_ROP_MAX_SRCS];
    PyValue owned[_PY_ROP_MAX_SRCS];
} _PyRopAnchors;

static inline _PyRopAnchors
_PyRop_AnchorForCall(const _PyRopSrcs *s)
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
_PyRop_RetireFailedCall(_PyRopAnchors *a, const _PyRopSrcs *s)
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

/* Python/specialize.c: the adaptation rows */
bool _PyRop_FindAddExtension(_PyFbBinary *fb, PyValueBorrow a, PyValueBorrow b);
void _PyRop_Backoff(_PyFbHeader *fb);
int _PyRop_Install(_PyInstr *site, _PyFbHeader *fb, int handler);
void _PyRop_BeginTruth(_PyFbUnary *fb, PyValueBorrow v);
bool _PyRop_ValidatePlain(_PyFbUnary *fb, PyValueBorrow v);
bool _PyRop_ValidateSlot(_PyFbUnary *fb, PyValueBorrow v);
int _PyRop_ClassifyCall(_PyInstr *site, _PyFbCall *fb, PyValueBorrow fn,
                        const _PyIRWord *head);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ROP_ROWS_H */
