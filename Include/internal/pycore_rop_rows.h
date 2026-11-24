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

/* Python/specialize.c: the adaptation rows */
bool _PyRop_FindAddExtension(_PyFbBinary *fb, PyValueBorrow a, PyValueBorrow b);
void _PyRop_Backoff(_PyFbHeader *fb);
int _PyRop_Install(_PyInstr *site, _PyFbHeader *fb, int handler);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ROP_ROWS_H */
