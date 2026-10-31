
/* Generic object operations; and implementation of None */

#include "Python.h"
#include "pycore_shape.h"       // _PyTypes_InitRootShapes()
#include "pycore_lookup.h"      // _PyShape_LookupPlan()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_context.h"       // _PyContextTokenMissing_Type
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_dict.h"          // _PyDict_GetItemValue()
#include "pycore_function.h"      // _PyClassMethod_GetFunc()
#include "pycore_gc_trace.h"     // _PyGC_LiveAddImmortal()
#include "pycore_gc_walk.h"       // Py_GC_ARGS_ANCHORED
#include "pycore_genobject.h"     // _PyAsyncGenAThrow_Type
#include "pycore_hamt.h"          // _PyHamtItems_Type
#include "pycore_initconfig.h"    // _PyStatus_OK()
#include "pycore_instruction_sequence.h" // _PyInstructionSequence_Type
#include "pycore_interpframe.h"   // _PyFrame_Stackbase()
#include "pycore_interpolation.h" // _PyInterpolation_Type
#include "pycore_lazyimportobject.h" // PyLazyImport_Type
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_memoryobject.h"  // _PyManagedBuffer_Type
#include "pycore_namespace.h"     // _PyNamespace_Type
#include "pycore_object.h"        // export _Py_SwappedOp
#include "pycore_object_deferred.h" // _PyObject_SetDeferredRefcount()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_symtable.h"      // PySTEntry_Type
#include "pycore_template.h"      // _PyTemplate_Type _PyTemplateIter_Type
#include "pycore_typeobject.h"    // _PyBufferWrapper_Type
#include "pycore_typevarobject.h" // _PyTypeAlias_Type
#include "pycore_floatobject.h"   // _PyFloat_ReprDouble()
#include "pycore_unicodeobject.h" // _PyUnicode_CheckConsistency()
#include "pycore_value.h"         // PyValue_FromObject
#include "pycore_unionobject.h"   // _PyUnion_Type
#include "pycore_weakref.h"       // _PyWeakref_ListPtr()


#ifdef Py_LIMITED_API
   // Prevent recursive call _Py_IncRef() <=> Py_INCREF()
#  error "Py_LIMITED_API macro must not be defined"
#endif

/* Defined in tracemalloc.c */
extern void _PyMem_DumpTraceback(int fd, const void *ptr);

/* The PyObject * forms of these protocols, which the value forms call for
   an object (see "The PyObject * protocol" at the end of this file). */
static PyObject *object_ascii(PyObject *v);
static int object_is_true(PyObject *v);


int
_PyObject_CheckConsistency(PyObject *op, int check_content)
{
#define CHECK(expr) \
    do { if (!(expr)) { _PyObject_ASSERT_FAILED_MSG(op, Py_STRINGIFY(expr)); } } while (0)

    CHECK(!_PyObject_IsFreed(op));
    CHECK(Py_REFCNT(op) >= 1);

    _PyType_CheckConsistency(Py_TYPE(op));

    if (PyUnicode_Check(op)) {
        _PyUnicode_CheckConsistency(op, check_content);
    }
    else if (PyAnyDict_Check(op)) {
        _PyDict_CheckConsistency(op, check_content);
    }
    return 1;

#undef CHECK
}




/* Object allocation routines used by NEWOBJ and NEWVAROBJ macros.
   These are used by the individual routines for object creation.
   Do not call them otherwise, they do not initialize the object! */



void
Py_IncRef(PyObject *o)
{
    Py_XINCREF(o);
}

/* The release barrier for compilers without GNU inline assembly (see
   Py_DECREF()).  The volatile store is observable, so the call is kept with
   op as its argument. */
Py_NO_INLINE void
_Py_KeepAlive(const void *op)
{
    const void *volatile keep = op;
    (void)keep;
}

void
Py_DecRef(PyObject *o)
{
    Py_XDECREF(o);
}

void
_Py_IncRef(PyObject *o)
{
    Py_INCREF(o);
}

void
_Py_DecRef(PyObject *o)
{
    Py_DECREF(o);
}



/**************************************/

PyObject *
PyObject_Init(PyObject *op, PyTypeObject *tp)
{
    if (op == NULL) {
        return PyErr_NoMemory();
    }

    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
PyObject_InitVar(PyVarObject *op, PyTypeObject *tp, Py_ssize_t size)
{
    if (op == NULL) {
        return (PyVarObject *) PyErr_NoMemory();
    }

    _PyObject_InitVar(op, tp, size);
    return op;
}

PyObject *
_PyObject_New(PyTypeObject *tp)
{
    PyObject *op = (PyObject *) PyObject_Malloc(_PyObject_SIZE(tp));
    if (op == NULL) {
        return PyErr_NoMemory();
    }
    _PyObject_Init(op, tp);
    return op;
}

PyVarObject *
_PyObject_NewVar(PyTypeObject *tp, Py_ssize_t nitems)
{
    PyVarObject *op;
    const size_t size = _PyObject_VAR_SIZE(tp, nitems);
    op = (PyVarObject *) PyObject_Malloc(size);
    if (op == NULL) {
        return (PyVarObject *)PyErr_NoMemory();
    }
    _PyObject_InitVar(op, tp, nitems);
    return op;
}

void
PyObject_CallFinalizer(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);

    if (tp->tp_finalize == NULL)
        return;
    /* tp_finalize should only be called once. */
    // The flag is in the object header, so it works for every type.
    if (_PyGC_FINALIZED(self)) {
        return;
    }
    _PyGC_SET_FINALIZED(self);
    tp->tp_finalize(self);
}

int
PyObject_CallFinalizerFromDealloc(PyObject *self)
{
    /* The collector ran the finalizer, from its queue, in the collection
       before the one that is sweeping self. */
    (void)self;
    return 0;
}

int
PyObject_Print(PyObject *op, FILE *fp, int flags)
{
    int ret = 0;
    int write_error = 0;
    if (PyErr_CheckSignals())
        return -1;
    if (_Py_EnterRecursiveCall(" printing an object")) {
        return -1;
    }
    clearerr(fp); /* Clear any previous error condition */
    if (op == NULL) {
        Py_BEGIN_ALLOW_THREADS
        fprintf(fp, "<nil>");
        Py_END_ALLOW_THREADS
    }
    else {
        if (Py_REFCNT(op) <= 0) {
            Py_BEGIN_ALLOW_THREADS
            fprintf(fp, "<refcnt %zd at %p>", Py_REFCNT(op), (void *)op);
            Py_END_ALLOW_THREADS
        }
        else {
            PyObject *s;
            if (flags & Py_PRINT_RAW)
                s = PyObject_Str(op);
            else
                s = PyObject_Repr(op);
            if (s == NULL) {
                ret = -1;
            }
            else {
                assert(PyUnicode_Check(s));
                const char *t;
                Py_ssize_t len;
                t = PyUnicode_AsUTF8AndSize(s, &len);
                if (t == NULL) {
                    ret = -1;
                }
                else {
                    /* Versions of Android and OpenBSD from before 2023 fail to
                       set the `ferror` indicator when writing to a read-only
                       stream, so we need to check the return value.
                       (https://github.com/openbsd/src/commit/fc99cf9338942ecd9adc94ea08bf6188f0428c15) */
                    if (fwrite(t, 1, len, fp) != (size_t)len) {
                        write_error = 1;
                    }
                }
                Py_DECREF(s);
            }
        }
    }
    if (ret == 0) {
        if (write_error || ferror(fp)) {
            PyErr_SetFromErrno(PyExc_OSError);
            clearerr(fp);
            ret = -1;
        }
    }

    _Py_LeaveRecursiveCall();
    return ret;
}

/* For debugging convenience.  Set a breakpoint here and call it from your DLL */
void
_Py_BreakPoint(void)
{
}


/* Heuristic checking if the object memory is uninitialized or deallocated.
   Rely on the debug hooks on Python memory allocators:
   see _PyMem_IsPtrFreed().

   The function can be used to prevent segmentation fault on dereferencing
   pointers like 0xDDDDDDDDDDDDDDDD. */
int
_PyObject_IsFreed(PyObject *op)
{
    // The whole header word: the debug allocator's fill covers the flags
    // too, which Py_TYPE() would mask off.
    if (_PyMem_IsPtrFreed(op) || _PyMem_IsPtrFreed((void *)op->ob_header)) {
        return 1;
    }
    return 0;
}


/* For debugging convenience. */
void
PyObject_Dump(PyObject* op)
{
    if (_PyObject_IsFreed(op)) {
        /* It seems like the object memory has been freed:
           don't access it to prevent a segmentation fault. */
        fprintf(stderr, "<object at %p is freed>\n", op);
        fflush(stderr);
        return;
    }

    /* first, write fields which are the least likely to crash */
    fprintf(stderr, "object address  : %p\n", (void *)op);
    fprintf(stderr, "object refcount : %zd\n", Py_REFCNT(op));
    fflush(stderr);

    PyTypeObject *type = Py_TYPE(op);
    fprintf(stderr, "object type     : %p\n", type);
    fprintf(stderr, "object type name: %s\n",
            type==NULL ? "NULL" : type->tp_name);

    /* the most dangerous part */
    fprintf(stderr, "object repr     : ");
    fflush(stderr);

    PyGILState_STATE gil = PyGILState_Ensure();
    PyObject *exc = PyErr_GetRaisedException();

    (void)PyObject_Print(op, stderr, 0);
    fflush(stderr);

    PyErr_SetRaisedException(exc);
    PyGILState_Release(gil);

    fprintf(stderr, "\n");
    fflush(stderr);
}

static PyObject *
object_repr(PyObject *v)
{
    PyObject *res;
    if (PyErr_CheckSignals())
        return NULL;
    if (v == NULL)
        return PyUnicode_FromString("<NULL>");
    if (Py_TYPE(v)->tp_repr == NULL)
        return PyUnicode_FromFormat("<%s object at %p>",
                                    Py_TYPE(v)->tp_name, v);

    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    /* PyObject_Repr() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

    /* It is possible for a type to have a tp_repr representation that loops
       infinitely. */
    if (_Py_EnterRecursiveCallTstate(tstate,
                                     " while getting the repr of an object")) {
        return NULL;
    }
    res = (*Py_TYPE(v)->tp_repr)(v);
    _Py_LeaveRecursiveCallTstate(tstate);

    if (res == NULL) {
        return NULL;
    }
    if (!PyUnicode_Check(res)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "%T.__repr__() must return a str, not %T", v, res);
        Py_DECREF(res);
        return NULL;
    }
    return res;
}

static PyObject *
object_str(PyObject *v)
{
    PyObject *res;
    if (PyErr_CheckSignals())
        return NULL;
    if (v == NULL)
        return PyUnicode_FromString("<NULL>");
    if (PyUnicode_CheckExact(v)) {
        return Py_NewRef(v);
    }
    if (Py_TYPE(v)->tp_str == NULL)
        return object_repr(v);

    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    /* PyObject_Str() must not be called with an exception set,
       because it can clear it (directly or indirectly) and so the
       caller loses its exception */
    assert(!_PyErr_Occurred(tstate));
#endif

    /* It is possible for a type to have a tp_str representation that loops
       infinitely. */
    if (_Py_EnterRecursiveCallTstate(tstate, " while getting the str of an object")) {
        return NULL;
    }
    res = (*Py_TYPE(v)->tp_str)(v);
    _Py_LeaveRecursiveCallTstate(tstate);

    if (res == NULL) {
        return NULL;
    }
    if (!PyUnicode_Check(res)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "%T.__str__() must return a str, not %T", v, res);
        Py_DECREF(res);
        return NULL;
    }
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;
}

/* str() and repr() of an inline value, which are ASCII. */
static PyObject *
inline_value_str(PyValue v)
{
    if (PyValue_IsInlineFloat(v)) {
        return _PyFloat_ReprDouble(PyValue_Double(v));
    }
    int32_t i = PyValue_Int32(v);
    char buf[12];
    char *end = buf + sizeof(buf);
    char *p = end;
    uint32_t u = i < 0 ? 0u - (uint32_t)i : (uint32_t)i;
    do {
        *--p = (char)('0' + u % 10);
        u /= 10;
    } while (u);
    if (i < 0) {
        *--p = '-';
    }
    return _PyUnicode_FromASCII(p, end - p);
}

static inline PyValue
str_value_result(PyObject *res)
{
    return res == NULL ? PyValue_NULL : PyValue_FromObject(res);
}

PyValue
PyObject_StrValue(PyValue v)
{
    if (PyValue_IsInline(v)) {
        return str_value_result(inline_value_str(v));
    }
    return str_value_result(object_str(PyValue_ObjectBorrow(v)));
}

PyValue
PyObject_ReprValue(PyValue v)
{
    if (PyValue_IsInline(v)) {
        return str_value_result(inline_value_str(v));
    }
    return str_value_result(object_repr(PyValue_ObjectBorrow(v)));
}

PyValue
PyObject_ASCIIValue(PyValue v)
{
    if (PyValue_IsInline(v)) {
        return str_value_result(inline_value_str(v));
    }
    return str_value_result(object_ascii(PyValue_ObjectBorrow(v)));
}

static PyObject *
object_ascii(PyObject *v)
{
    PyObject *repr, *ascii, *res;

    repr = object_repr(v);
    if (repr == NULL)
        return NULL;

    if (PyUnicode_IS_ASCII(repr))
        return repr;

    /* repr is guaranteed to be a PyUnicode object by PyObject_Repr */
    ascii = _PyUnicode_AsASCIIString(repr, "backslashreplace");
    Py_DECREF(repr);
    if (ascii == NULL)
        return NULL;

    res = PyUnicode_DecodeASCII(
        PyBytes_AS_STRING(ascii),
        PyBytes_GET_SIZE(ascii),
        NULL);

    Py_DECREF(ascii);
    return res;
}

PyObject *
PyObject_Bytes(PyObject *v)
{
    PyObject *result, *func;

    if (v == NULL)
        return PyBytes_FromString("<NULL>");

    if (PyBytes_CheckExact(v)) {
        return Py_NewRef(v);
    }

    func = _PyObject_LookupSpecial(v, &_Py_ID(__bytes__));
    if (func != NULL) {
        result = _PyObject_CallNoArgs(func);
        Py_DECREF(func);
        if (result == NULL)
            return NULL;
        if (!PyBytes_Check(result)) {
            PyErr_Format(PyExc_TypeError,
                         "%T.__bytes__() must return a bytes, not %T",
                         v, result);
            Py_DECREF(result);
            return NULL;
        }
        return result;
    }
    else if (PyErr_Occurred())
        return NULL;
    return PyBytes_FromObject(v);
}

/*
def _PyObject_FunctionStr(x):
    try:
        qualname = x.__qualname__
    except AttributeError:
        return str(x)
    try:
        mod = x.__module__
        if mod is not None and mod != 'builtins':
            return f"{x.__module__}.{qualname}()"
    except AttributeError:
        pass
    return qualname
*/
/* The span: x, then the object slots of its __qualname__ and __module__ */
Py_GC_PRECISE PyObject *
_PyObject_FunctionStr(PyObject *x)
{
    assert(!PyErr_Occurred());
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, x);
    PyObject **qualnamep = _PyValueSpan_ObjectSlot(&span);
    PyObject **modulep = _PyValueSpan_ObjectSlot(&span);
    PyObject *result = NULL;
    int ret = PyObject_GetOptionalAttr(x, &_Py_ID(__qualname__), qualnamep);
    if (*qualnamep == NULL) {
        if (ret >= 0) {
            result = PyObject_Str(x);
        }
        goto done;
    }
    ret = PyObject_GetOptionalAttr(x, &_Py_ID(__module__), modulep);
    if (*modulep != NULL && *modulep != Py_None) {
        ret = PyObject_RichCompareBool(*modulep, &_Py_ID(builtins), Py_NE);
        if (ret < 0) {
            // error
            goto done;
        }
        if (ret > 0) {
            result = PyUnicode_FromFormat("%S.%S()", *modulep, *qualnamep);
            goto done;
        }
    }
    else if (ret < 0) {
        goto done;
    }
    result = PyUnicode_FromFormat("%S()", *qualnamep);
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return result;
}

/* For Python 3.0.1 and later, the old three-way comparison has been
   completely removed in favour of rich comparisons.  PyObject_Compare() and
   PyObject_Cmp() are gone, and the builtin cmp function no longer exists.
   The old tp_compare slot has been renamed to tp_as_async, and should no
   longer be used.  Use tp_richcompare instead.

   See (*) below for practical amendments.

   tp_richcompare gets called with a first argument of the appropriate type
   and a second object of an arbitrary type.  We never do any kind of
   coercion.

   The tp_richcompare slot should return an object, as follows:

    NULL if an exception occurred
    NotImplemented if the requested comparison is not implemented
    any other false value if the requested comparison is false
    any other true value if the requested comparison is true

  The PyObject_RichCompare[Bool]() wrappers raise TypeError when they get
  NotImplemented.

  (*) Practical amendments:

  - If rich comparison returns NotImplemented, == and != are decided by
    comparing the object pointer (i.e. falling back to the base object
    implementation).

*/

/* Map rich comparison operators to their swapped version, e.g. LT <--> GT */
int _Py_SwappedOp[] = {Py_GT, Py_GE, Py_EQ, Py_NE, Py_LT, Py_LE};

static const char * const opstrings[] = {"<", "<=", "==", "!=", ">", ">="};

/* Perform a rich comparison, raising TypeError when the requested comparison
   operator is not supported. */
static PyObject *
do_richcompare(PyThreadState *tstate, PyObject *v, PyObject *w, int op)
{
    richcmpfunc f;
    PyObject *res;
    int checked_reverse_op = 0;

    if (!Py_IS_TYPE(v, Py_TYPE(w)) &&
        PyType_IsSubtype(Py_TYPE(w), Py_TYPE(v)) &&
        (f = Py_TYPE(w)->tp_richcompare) != NULL) {
        checked_reverse_op = 1;
        res = (*f)(w, v, _Py_SwappedOp[op]);
        if (res != Py_NotImplemented)
            return res;
        Py_DECREF(res);
    }
    if ((f = Py_TYPE(v)->tp_richcompare) != NULL) {
        res = (*f)(v, w, op);
        if (res != Py_NotImplemented)
            return res;
        Py_DECREF(res);
    }
    if (!checked_reverse_op && (f = Py_TYPE(w)->tp_richcompare) != NULL) {
        res = (*f)(w, v, _Py_SwappedOp[op]);
        if (res != Py_NotImplemented)
            return res;
        Py_DECREF(res);
    }
    /* If neither object implements it, provide a sensible default
       for == and !=, but raise an exception for ordering. */
    switch (op) {
    case Py_EQ:
        res = (v == w) ? Py_True : Py_False;
        break;
    case Py_NE:
        res = (v != w) ? Py_True : Py_False;
        break;
    default:
        _PyErr_Format(tstate, PyExc_TypeError,
                      "'%s' not supported between instances of '%.100s' and '%.100s'",
                      opstrings[op],
                      Py_TYPE(v)->tp_name,
                      Py_TYPE(w)->tp_name);
        return NULL;
    }
    return Py_NewRef(res);
}

/* Perform a rich comparison with object result.  This wraps do_richcompare()
   with a check for NULL arguments and a recursion check. */

static PyObject *
object_richcompare(PyObject *v, PyObject *w, int op)
{
    PyThreadState *tstate = _PyThreadState_GET();

    assert(Py_LT <= op && op <= Py_GE);
    if (v == NULL || w == NULL) {
        if (!_PyErr_Occurred(tstate)) {
            PyErr_BadInternalCall();
        }
        return NULL;
    }
    if (_Py_EnterRecursiveCallTstate(tstate, " in comparison")) {
        return NULL;
    }
    PyObject *res = do_richcompare(tstate, v, w, op);
    _Py_LeaveRecursiveCallTstate(tstate);
    return res;
}

/* Perform a rich comparison with integer result.  This wraps
   PyObject_RichCompare(), returning -1 for error, 0 for false, 1 for true. */
static int
object_richcompare_bool(PyObject *v, PyObject *w, int op)
{
    PyObject *res;
    int ok;

    /* Quick result when objects are the same.
       Guarantees that identity implies equality. */
    if (v == w) {
        if (op == Py_EQ)
            return 1;
        else if (op == Py_NE)
            return 0;
    }

    res = object_richcompare(v, w, op);
    if (res == NULL)
        return -1;
    if (PyBool_Check(res)) {
        ok = (res == Py_True);
        assert(_Py_IsImmortal(res));
    }
    else {
        ok = PyObject_IsTrue(res);
        Py_DECREF(res);
    }
    return ok;
}

Py_hash_t
PyObject_HashNotImplemented(PyObject *v)
{
    PyErr_Format(PyExc_TypeError, "unhashable type: '%.200s'",
                 Py_TYPE(v)->tp_name);
    return -1;
}

static Py_hash_t
object_hash(PyObject *v)
{
    PyTypeObject *tp = Py_TYPE(v);
    if (tp->tp_hash != NULL)
        return (*tp->tp_hash)(v);
    /* To keep to the general practice that inheriting
     * solely from object in C code should work without
     * an explicit call to PyType_Ready, we implicitly call
     * PyType_Ready here and then check the tp_hash slot again
     */
    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0)
            return -1;
        if (tp->tp_hash != NULL)
            return (*tp->tp_hash)(v);
    }
    /* Otherwise, the object can't be hashed */
    return PyObject_HashNotImplemented(v);
}

/* The span: v, across the name's creation */
Py_GC_PRECISE PyObject *
PyObject_GetAttrString(PyObject *v, const char *name)
{
    if (Py_TYPE(v)->tp_getattr != NULL)
        return (*Py_TYPE(v)->tp_getattr)(v, (char*)name);
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, v);
    PyObject *res = NULL;
    PyObject *w = PyUnicode_FromString(name);
    if (w != NULL) {
        res = PyObject_GetAttr(v, w);
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

int
PyObject_HasAttrStringWithError(PyObject *obj, const char *name)
{
    PyObject *res;
    int rc = PyObject_GetOptionalAttrString(obj, name, &res);
    Py_XDECREF(res);
    return rc;
}


int
PyObject_HasAttrString(PyObject *obj, const char *name)
{
    int rc = PyObject_HasAttrStringWithError(obj, name);
    if (rc < 0) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyObject_HasAttrString(); consider using "
            "PyObject_HasAttrStringWithError(), "
            "PyObject_GetOptionalAttrString() or PyObject_GetAttrString()");
        return 0;
    }
    return rc;
}

/* The span: v and w, across the name's creation, or the raised exception */
Py_GC_PRECISE int
PyObject_SetAttrString(PyObject *v, const char *name, PyObject *w)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[2];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    int res = -1;
    if (w == NULL && _PyErr_Occurred(tstate)) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        _PyValueSpan_AppendObject(&span, exc);
        _PyErr_SetString(tstate, PyExc_SystemError,
            "PyObject_SetAttrString() must not be called with NULL value "
            "and an exception set");
        _PyErr_ChainExceptions1Tstate(tstate, exc);
        goto done;
    }

    if (Py_TYPE(v)->tp_setattr != NULL) {
        res = (*Py_TYPE(v)->tp_setattr)(v, (char*)name, w);
        goto done;
    }

    _PyValueSpan_AppendObject(&span, v);
    _PyValueSpan_AppendObject(&span, w);
    PyObject *s = PyUnicode_InternFromString(name);
    if (s != NULL) {
        res = PyObject_SetAttr(v, s, w);
    }
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

Py_GC_PRECISE int
PyObject_DelAttrString(PyObject *v, const char *name)
{
    return PyObject_SetAttrString(v, name, NULL);
}

/* The span: the object slot of __isabstractmethod__ */
Py_GC_PRECISE int
_PyObject_IsAbstract(PyObject *obj)
{
    if (obj == NULL)
        return 0;

    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    PyObject **isabstractp = _PyValueSpan_ObjectSlot(&span);
    int res = PyObject_GetOptionalAttr(obj, &_Py_ID(__isabstractmethod__),
                                       isabstractp);
    if (res > 0) {
        res = PyObject_IsTrue(*isabstractp);
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

/* The span: v, across the name's creation */
Py_GC_PRECISE PyObject *
_PyObject_GetAttrId(PyObject *v, _Py_Identifier *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, v);
    PyObject *result = NULL;
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
_Py_COMP_DIAG_POP
    if (oname != NULL) {
        result = PyObject_GetAttr(v, oname);
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    return result;
}

/* The span: the owner, the name and the raised exception */
Py_GC_PRECISE int
_PyObject_SetAttributeErrorContext(PyValue v, PyObject* name)
{
    assert(PyErr_Occurred());
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_Append(&span, v);
    _PyValueSpan_AppendObject(&span, name);
    int res = 0;
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)){
        goto done;
    }
    // Intercept AttributeError exceptions and augment them to offer suggestions later.
    PyObject *exc = PyErr_GetRaisedException();
    _PyValueSpan_AppendObject(&span, exc);
    if (!PyErr_GivenExceptionMatches(exc, PyExc_AttributeError)) {
        goto restore;
    }
    PyAttributeErrorObject* the_exc = (PyAttributeErrorObject*) exc;
    // Check if this exception was already augmented
    if (the_exc->name || the_exc->obj) {
        goto restore;
    }
    // Augment the exception with the name and object
    // AttributeError.obj is a _Py_T_OBJECT member: its setter makes the
    // one box of an inline owner.
    if (PyObject_SetAttr(exc, &_Py_ID(name), name) ||
        PyObject_SetAttrValue(PyValue_FromObject(exc), &_Py_ID(obj), v)) {
        res = 1;
        goto done;
    }
restore:
    PyErr_SetRaisedException(exc);
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

Py_GC_PRECISE Py_GC_ARGS_ANCHORED static PyValue
generic_getattr_value(PyValue owner, PyObject *name, PyObject *dict,
                      int suppress);

Py_GC_PRECISE Py_GC_ARGS_ANCHORED static int
generic_setattr_value(PyObject *obj, PyObject *name, PyValue value,
                      PyObject *dict);

static int setattr_value(PyObject *v, PyObject *name, PyValue value);

Py_GC_PRECISE Py_GC_ARGS_ANCHORED static int
optional_attr_value(PyValue v, PyObject *name, PyTypeObject *tp,
                    PyValue *result);

/* The object protocol on values (Include/cpython/abstract.h, "The PyValue
   protocols").  An inline int or float is boxed for a type slot, held by a
   value root while the slot runs, unless the operation is done here. */

/* The span: the owner, the name and the owner's type, which a reentrant
   slot may replace and which the result check names after it. */
Py_GC_PRECISE PyValue
PyObject_GetAttrValue(PyValue v, PyObject *name)
{
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return PyValue_NULL;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyTypeObject *tp = PyValue_Type(v);
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_Append(&span, v);
    _PyValueSpan_AppendObject(&span, name);
    _PyValueSpan_AppendObject(&span, (PyObject *)tp);
    PyValue result;
    if (tp->tp_getattro == PyObject_GenericGetAttr) {
        // The executor takes the owner as a value: an inline int or float
        // is boxed only to bind a descriptor.
        result = generic_getattr_value(v, name, NULL, 0);
    }
    else {
        // Only the generic getter is reached with an inline owner (note 12
        // section 2.4): int and float use it.
        assert(!PyValue_IsInline(v));
        PyObject *o = PyValue_ObjectBorrow(v);
        if (tp->tp_getattro != NULL) {
            result = (*tp->tp_getattro)(o, name);
            _PyValue_CheckSlotResult(result, tp, "tp_getattro");
        }
        else if (tp->tp_getattr != NULL) {
            const char *name_str = PyUnicode_AsUTF8(name);
            if (name_str == NULL) {
                result = PyValue_NULL;
                goto done;
            }
            result = PyValue_FromResult((*tp->tp_getattr)(o, (char *)name_str));
        }
        else {
            PyErr_Format(PyExc_AttributeError,
                         "'%.100s' object has no attribute '%U'",
                         tp->tp_name, name);
            result = PyValue_NULL;
        }
    }
    if (PyValue_IsNull(result)) {
        _PyObject_SetAttributeErrorContext(v, name);
    }
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return result;
}

Py_GC_PRECISE int
PyObject_SetAttrValue(PyValue v, PyObject *name, PyValue value)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (PyValue_IsNull(value) && _PyErr_Occurred(tstate)) {
        // The span: the raised exception, which the new one chains
        PyValue slots[1];
        _PyValueRoot span;
        _PyThreadState_PushValueSpan(tstate, &span, slots);
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        _PyValueSpan_AppendObject(&span, exc);
        _PyErr_SetString(tstate, PyExc_SystemError,
            "PyObject_SetAttr() must not be called with NULL value "
            "and an exception set");
        _PyErr_ChainExceptions1Tstate(tstate, exc);
        _PyThreadState_PopValueRoot(tstate, &span);
        return -1;
    }
    if (!PyValue_IsInline(v)) {
        return setattr_value(PyValue_ObjectBorrow(v), name, value);
    }
    // A store to an inline int or float can only fail (note 12 section 3):
    // the box is made for the error's path.
    _PyValueRoot v_root;
    PyObject *v_o = _PyValueRoot_PushObject(tstate, &v_root, v, _PY_BOX_OTHER);
    int res = v_o == NULL ? -1 : setattr_value(v_o, name, value);
    _PyThreadState_PopValueRoot(tstate, &v_root);
    return res;
}

PyValue
PyObject_RichCompareValue(PyValue v, PyValue w, int op)
{
    assert(Py_LT <= op && op <= Py_GE);
    if (PyValue_IsInline(v) && PyValue_IsInline(w)) {
        int c = _PyValue_CompareInline(v, w);
        return _PyValue_CompareResult(c, op) ? _PyValue_True : _PyValue_False;
    }
    PyObject *res;
    if (!PyValue_IsInline(v) && !PyValue_IsInline(w)) {
        res = object_richcompare(PyValue_ObjectBorrow(v),
                                 PyValue_ObjectBorrow(w), op);
    }
    else {
        PyThreadState *tstate = _PyThreadState_GET();
        _PyValueRoot v_root, w_root;
        PyObject *v_o = _PyValueRoot_PushObject(tstate, &v_root, v, _PY_BOX_OTHER);
        PyObject *w_o = _PyValueRoot_PushObject(tstate, &w_root, w, _PY_BOX_OTHER);
        res = v_o && w_o ? object_richcompare(v_o, w_o, op) : NULL;
        _PyThreadState_PopValueRoot(tstate, &w_root);
        _PyThreadState_PopValueRoot(tstate, &v_root);
    }
    return res == NULL ? PyValue_NULL : PyValue_FromObject(res);
}

int
PyObject_RichCompareBoolValue(PyValue v, PyValue w, int op)
{
    assert(Py_LT <= op && op <= Py_GE);
    if (PyValue_IsInline(v) && PyValue_IsInline(w)) {
        return _PyValue_CompareResult(_PyValue_CompareInline(v, w), op);
    }
    // Identity implies equality, as in PyObject_RichCompareBool().
    if (PyValue_Is(v, w)) {
        if (op == Py_EQ) {
            return 1;
        }
        if (op == Py_NE) {
            return 0;
        }
    }
    if (!PyValue_IsInline(v) && !PyValue_IsInline(w)) {
        return object_richcompare_bool(PyValue_ObjectBorrow(v),
                                       PyValue_ObjectBorrow(w), op);
    }
    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot v_root, w_root;
    PyObject *v_o = _PyValueRoot_PushObject(tstate, &v_root, v, _PY_BOX_OTHER);
    PyObject *w_o = _PyValueRoot_PushObject(tstate, &w_root, w, _PY_BOX_OTHER);
    int res = v_o && w_o ? object_richcompare_bool(v_o, w_o, op) : -1;
    _PyThreadState_PopValueRoot(tstate, &w_root);
    _PyThreadState_PopValueRoot(tstate, &v_root);
    return res;
}

int
PyObject_IsTrueValue(PyValue v)
{
    if (PyValue_IsInlineInt(v)) {
        return PyValue_Int32(v) != 0;
    }
    if (PyValue_IsInlineFloat(v)) {
        return PyValue_Double(v) != 0.0;
    }
    return object_is_true(PyValue_ObjectBorrow(v));
}

Py_hash_t
PyObject_HashValue(PyValue v)
{
    if (PyValue_IsInlineInt(v)) {
        // As long_hash(): hash(-1) is -2, since -1 means an error.
        Py_hash_t h = PyValue_Int32(v);
        return h == -1 ? -2 : h;
    }
    if (PyValue_IsInlineFloat(v)) {
        return _Py_HashDouble(NULL, PyValue_Double(v));
    }
    return object_hash(PyValue_ObjectBorrow(v));
}

/* The span: the owner, the name and its type, as PyObject_GetAttrValue's */
Py_GC_PRECISE int
PyObject_GetOptionalAttrValue(PyValue v, PyObject *name, PyValue *result)
{
    *result = PyValue_NULL;
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return -1;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    PyTypeObject *tp = PyValue_Type(v);
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_Append(&span, v);
    _PyValueSpan_AppendObject(&span, name);
    _PyValueSpan_AppendObject(&span, (PyObject *)tp);
    int rc = optional_attr_value(v, name, tp, result);
    _PyThreadState_PopValueRoot(tstate, &span);
    return rc;
}

/* PyObject_GetOptionalAttrValue() past its checks, whose span anchors V,
   NAME and TP. */
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static int
optional_attr_value(PyValue v, PyObject *name, PyTypeObject *tp,
                    PyValue *result)
{
    if (tp->tp_getattro == PyObject_GenericGetAttr) {
        *result = generic_getattr_value(v, name, NULL, 1);
        if (!PyValue_IsNull(*result)) {
            return 1;
        }
        return PyErr_Occurred() ? -1 : 0;
    }
    assert(!PyValue_IsInline(v));
    PyObject *o = PyValue_ObjectBorrow(v);
    if (tp->tp_getattro == _Py_type_getattro) {
        int suppress_missing_attribute_exception = 0;
        *result = _Py_type_getattro_impl((PyTypeObject *)o, name,
                                         &suppress_missing_attribute_exception);
        if (suppress_missing_attribute_exception) {
            // return 0 without having to clear the exception
            return 0;
        }
    }
    else if (tp->tp_getattro == _Py_module_getattro) {
        // optimization: suppress attribute error from module getattro method
        *result = _Py_module_getattro_impl((PyModuleObject *)o, name, 1);
        if (!PyValue_IsNull(*result)) {
            return 1;
        }
        return PyErr_Occurred() ? -1 : 0;
    }
    else if (tp->tp_getattro != NULL) {
        *result = (*tp->tp_getattro)(o, name);
        _PyValue_CheckSlotResult(*result, tp, "tp_getattro");
    }
    else if (tp->tp_getattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            return -1;
        }
        *result = PyValue_FromResult((*tp->tp_getattr)(o, (char *)name_str));
    }
    else {
        return 0;
    }

    if (!PyValue_IsNull(*result)) {
        return 1;
    }
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
        return -1;
    }
    PyErr_Clear();
    return 0;
}

/* The span: the attribute's slot */
Py_GC_PRECISE int
PyObject_GetOptionalAttr(PyObject *v, PyObject *name, PyObject **result)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_Append(&span, PyValue_NULL);
    int rc = PyObject_GetOptionalAttrValue(PyValue_FromObject(v), name,
                                           &slots[0]);
    *result = NULL;
    if (rc > 0) {
        *result = PyValue_Object(slots[0]);
        if (*result == NULL) {
            rc = -1;
        }
    }
    _PyThreadState_PopValueRoot(tstate, &span);
    return rc;
}

int
PyObject_GetOptionalAttrString(PyObject *obj, const char *name, PyObject **result)
{
    if (Py_TYPE(obj)->tp_getattr == NULL) {
        PyObject *oname = PyUnicode_FromString(name);
        if (oname == NULL) {
            *result = NULL;
            return -1;
        }
        int rc = PyObject_GetOptionalAttr(obj, oname, result);
        Py_DECREF(oname);
        return rc;
    }

    *result = (*Py_TYPE(obj)->tp_getattr)(obj, (char*)name);
    if (*result != NULL) {
        return 1;
    }
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
        return -1;
    }
    PyErr_Clear();
    return 0;
}

/* The span: the attribute's object slot */
Py_GC_PRECISE int
PyObject_HasAttrWithError(PyObject *obj, PyObject *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[1];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    PyObject **resp = _PyValueSpan_ObjectSlot(&span);
    int rc = PyObject_GetOptionalAttr(obj, name, resp);
    _PyThreadState_PopValueRoot(tstate, &span);
    return rc;
}

Py_GC_PRECISE int
PyObject_HasAttr(PyObject *obj, PyObject *name)
{
    int rc = PyObject_HasAttrWithError(obj, name);
    if (rc < 0) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyObject_HasAttr(); consider using "
            "PyObject_HasAttrWithError(), "
            "PyObject_GetOptionalAttr() or PyObject_GetAttr()");
        return 0;
    }
    return rc;
}

/* The dispatch of PyObject_SetAttrValue() on an object owner. */
/* The span: the object, its type, the value, and the name's object slot,
   which interning may replace. */
Py_GC_PRECISE static int
setattr_value(PyObject *v, PyObject *name, PyValue value)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyTypeObject *tp = Py_TYPE(v);
    int err = -1;

    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return -1;
    }
    PyValue slots[4];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, v);
    _PyValueSpan_AppendObject(&span, (PyObject *)tp);
    _PyValueSpan_Append(&span, value);
    PyObject **namep = _PyValueSpan_ObjectSlot(&span);
    *namep = name;
    _PyUnicode_InternMortal(tstate->interp, namep);
    name = *namep;
    if (tp->tp_setattro == PyObject_GenericSetAttr) {
        err = generic_setattr_value(v, name, value, NULL);
        goto done;
    }
    if (tp->tp_setattro != NULL) {
        err = (*tp->tp_setattro)(v, name, value);
        goto done;
    }
    if (tp->tp_setattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            goto done;
        }
        // The char * slot stays an object slot (note 12 section 2.5).
        _PyValueRoot value_root;
        PyObject *value_o = _PyValueRoot_PushObject(tstate, &value_root, value,
                                                    _PY_BOX_ATTRIBUTE_STORE);
        if (value_o != NULL || PyValue_IsNull(value)) {
            err = (*tp->tp_setattr)(v, (char *)name_str, value_o);
        }
        _PyThreadState_PopValueRoot(tstate, &value_root);
        goto done;
    }
    if (tp->tp_getattr == NULL && tp->tp_getattro == NULL)
        PyErr_Format(PyExc_TypeError,
                     "'%.100s' object has no attributes "
                     "(%s .%U)",
                     tp->tp_name,
                     PyValue_IsNull(value) ? "del" : "assign to",
                     name);
    else
        PyErr_Format(PyExc_TypeError,
                     "'%.100s' object has only read-only attributes "
                     "(%s .%U)",
                     tp->tp_name,
                     PyValue_IsNull(value) ? "del" : "assign to",
                     name);
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return err;
}

Py_GC_PRECISE int
PyObject_DelAttr(PyObject *v, PyObject *name)
{
    return PyObject_SetAttr(v, name, NULL);
}

/* The number of items of OBJ, a variable-size object, that its allocation
   holds: |ob_size|, except for an int (subclass), whose ob_size is lv_tag;
   long_subtype_new allocates its digit count, at least 1. */
static Py_ssize_t
var_item_count(PyObject *obj)
{
    if (PyLong_Check(obj)) {
        Py_ssize_t n = _PyLong_DigitCount((PyLongObject *)obj);
        return n ? n : 1;
    }
    Py_ssize_t n = Py_SIZE(obj);
    return n < 0 ? -n : n;
}

PyObject **
_PyObject_ComputedDictPointer(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    assert((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0);

    Py_ssize_t dictoffset = tp->tp_dictoffset;
    if (dictoffset == 0) {
        return NULL;
    }

    if (dictoffset < 0) {
        assert(dictoffset != -1);

        size_t size = _PyObject_VAR_SIZE(tp, var_item_count(obj));
        assert(size <= (size_t)PY_SSIZE_T_MAX);
        dictoffset += (Py_ssize_t)size;

        _PyObject_ASSERT(obj, dictoffset > 0);
        _PyObject_ASSERT(obj, dictoffset % SIZEOF_VOID_P == 0);
    }
    return (PyObject **) ((char *)obj + dictoffset);
}

int
_PyObject_IsInstanceDictEmpty(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        return _PyObject_ShapeIsEmpty(obj);
    }
    if (tp->tp_dictoffset == 0) {
        return 1;
    }
    PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
    PyObject *dict = dictptr != NULL ? *dictptr : NULL;
    return dict == NULL || PyDict_GET_SIZE(dict) == 0;
}

/* Helper to get a pointer to an object's __dict__ slot, if any.
 * An object with shape slots enters dictionary mode, and the pointer is to
 * slot 0, which holds the dict; a value that refers to an object is the
 * object's pointer, so the slot can be read and written as a PyObject *.
 * Does not set an exception: on a failure the object is unchanged and the
 * result is NULL.
 *
 * Note that the tp_dictoffset docs used to recommend this function,
 * so it should be treated as part of the public API.
 */
PyObject **
_PyObject_GetDictPtr(PyObject *obj)
{
    if ((Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) == 0) {
        return _PyObject_ComputedDictPointer(obj);
    }
    if (_PyObject_EnterDictMode(obj, _Py_DICT_MODE_ON_REQUEST) == NULL) {
        PyErr_Clear();
        return NULL;
    }
    return (PyObject **)&_PyObject_Slots(obj)[0];
}

PyObject *
PyObject_SelfIter(PyObject *obj)
{
    return Py_NewRef(obj);
}

/* Helper used when the __next__ method is removed from a type:
   tp_iternext is never NULL and can be safely called without checking
   on every iteration.
 */

PyValue
_PyObject_NextNotImplemented(PyObject *self)
{
    PyErr_Format(PyExc_TypeError,
                 "'%.200s' object is not iterable",
                 Py_TYPE(self)->tp_name);
    return PyValue_NULL;
}


/* The result of a descriptor's __get__ for OWNER as a value, or
   PyValue_NULL with an exception set, or with none if suppress is 1 and the
   exception was an AttributeError.  The callers anchor the descriptor and
   the owner.  An inline int or float owner is boxed for the descriptor,
   and the box is in this frame's span until the call returns. */
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static PyValue
descr_get_value(PyObject *descr, PyValue owner, int suppress)
{
    descrgetfunc f = Py_TYPE(descr)->tp_descr_get;
    PyValue res;
    if (!PyValue_IsInline(owner)) {
        PyObject *obj = PyValue_ObjectBorrow(owner);
        res = f(descr, obj, (PyObject *)Py_TYPE(obj));
        _PyValue_CheckSlotResult(res, Py_TYPE(descr), "tp_descr_get");
    }
    else {
        PyThreadState *tstate = _PyThreadState_GET();
        PyValue held[1];
        _PyValueRoot span;
        _PyThreadState_PushValueSpan(tstate, &span, held);
        PyObject *box = _PyValue_BoxInline(owner);
        _PyValueSpan_AppendObject(&span, box);
        res = PyValue_NULL;
        if (box != NULL) {
            VALUE_BOX_STAT_INC(_PY_BOX_OTHER);
            _PyGC_INJECT(tstate, _PyGC_SITE_SLOT_BOX);
            res = f(descr, box, (PyObject *)Py_TYPE(box));
            _PyValue_CheckSlotResult(res, Py_TYPE(descr), "tp_descr_get");
        }
        _PyThreadState_PopValueRoot(tstate, &span);
    }
    if (!PyValue_IsNull(res)) {
        return res;
    }
    if (suppress && PyErr_ExceptionMatches(PyExc_AttributeError)) {
        PyErr_Clear();
    }
    return PyValue_NULL;
}

/* The generic attribute protocol (boil/design/09-object-protocol.md
   section 4): a lookup plan for the name on the object's shape
   (Objects/lookupplan.c, the cache in Python/typecache.c), executed here.
   Nothing here dispatches through tp_getattro or tp_setattro: the callers
   that are slots, or that call slots, do that first. */

/* Precedence step 2 for a plan that is not a data descriptor: the
   attribute from the instance's storage, or from DICT if the caller gives
   one.  1 with *res set if found, 0 if not, -1 with an exception set.  The
   callers anchor obj, name and dict, and *res is a slot of their spans. */
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static int
load_from_storage(PyObject *obj, PyObject *name, const _PyLookupPlan *plan,
                  PyObject *dict, PyValue *res)
{
    PyTypeObject *tp = Py_TYPE(obj);
    *res = PyValue_NULL;
    if (dict == NULL) {
        if (plan->kind == _Py_LOOKUP_SLOT) {
            *res = *(PyValue *)((char *)obj + plan->offset);
            assert(!PyValue_IsNull(*res));
            return 1;
        }
        if (plan->kind != _Py_LOOKUP_DICT) {
            return 0;
        }
        if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
            if (!_PyShape_IsDictMode(_PyObject_SHAPE(obj))) {
                /* DICT on a shape: the name is not an exact str, and only
                   the dict compares it by its own __hash__ and __eq__. */
                assert(!PyUnicode_CheckExact(name));
                if (_PyObject_EnterDictMode(obj, _Py_DICT_MODE_STR_NAME)
                    == NULL)
                {
                    return -1;
                }
            }
            dict = (PyObject *)_PyObject_SlotDict(obj);
        }
        else {
            PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
            dict = dictptr != NULL ? FT_ATOMIC_LOAD_PTR_ACQUIRE(*dictptr) : NULL;
        }
        if (dict == NULL) {
            return 0;
        }
    }
    int rc = _PyDict_GetItemValue(dict, PyValue_FromObject(name), res);
    if (!PyValue_IsNull(*res)) {
        return 1;
    }
    return rc < 0 ? -1 : 0;
}

/* The callers anchor the owner and the name. */
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static void
set_no_attribute_error(PyValue owner, PyObject *name)
{
    PyErr_Format(PyExc_AttributeError,
                 "'%.100s' object has no attribute '%U'",
                 PyValue_Type(owner)->tp_name, name);
    _PyObject_SetAttributeErrorContext(owner, name);
}

/* The call-pair form of the generic protocol (note 09 section 4.4).  1: a
   method, *method unbound, to be called with obj; 2 (only if SPLIT): a
   classmethod's function, to be called with obj's type; 0: *method is the
   attribute; -1: an exception is set.  *method must be a location the
   collector sees: it holds the MRO result while descriptors run.  OWNER may
   be inline: a method is returned unbound with no box, and the call that
   uses it boxes the receiver (method_check_args).  The callers anchor the
   owner and the name. */
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static int
get_method_value(PyValue owner, PyObject *name, PyValue *method, int split)
{
    assert(PyUnicode_CheckExact(name));
    PyThreadState *tstate = _PyThreadState_GET();
    _PyLookupPlan plan;
    _PyShape_LookupPlan(_PyValue_Shape(owner), name, &plan);
    // The span: the plan's value from the MRO, then the slot of the
    // attribute from the instance's storage.
    PyValue slots[2];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_Append(&span, plan.value);
    _PyValueSpan_Append(&span, PyValue_NULL);
    PyValue class_value = slots[0];
    *method = class_value;
    PyObject *descr = PyValue_IsObject(class_value)
                      ? PyValue_ObjectBorrow(class_value) : NULL;
    int res;
    if (plan.kind == _Py_LOOKUP_DATA_DESCR || plan.kind == _Py_LOOKUP_PROPERTY) {
        if (plan.flags & _Py_LOOKUP_MEMBER) {
            // A member is a field of an instance: never an inline owner's.
            PyObject *obj = PyValue_ObjectBorrow(owner);
            PyValue v = *(PyValue *)((char *)obj + plan.offset);
            if (!PyValue_IsNull(v)) {
                *method = v;
                res = 0;
                goto done;
            }
            // Unbound: the descriptor raises the error.
        }
        *method = descr_get_value(descr, owner, 0);
        res = PyValue_IsNull(*method) ? -1 : 0;
        goto done;
    }
    // An inline owner has no storage: SLOT and DICT plans are an object's.
    int found = PyValue_IsInline(owner)
                ? 0
                : load_from_storage(PyValue_ObjectBorrow(owner), name, &plan,
                                    NULL, &slots[1]);
    if (found != 0) {
        *method = slots[1];
        res = found > 0 ? 0 : -1;
        goto done;
    }
    switch (plan.fallback) {
        case _Py_LOOKUP_METHOD:
            res = 1;
            break;
        case _Py_LOOKUP_DESCR:
            if (split && Py_IS_TYPE(descr, &PyClassMethod_Type)) {
                *method = PyValue_FromObject(_PyClassMethod_GetFunc(descr));
                res = 2;
                break;
            }
            if (split && Py_IS_TYPE(descr, &PyStaticMethod_Type)) {
                *method = PyValue_FromObject(_PyStaticMethod_GetFunc(descr));
                res = 0;
                break;
            }
            *method = descr_get_value(descr, owner, 0);
            res = PyValue_IsNull(*method) ? -1 : 0;
            break;
        case _Py_LOOKUP_CLASS_VALUE:
            res = 0;
            break;
        default:
            assert(plan.fallback == _Py_LOOKUP_ABSENT);
            *method = PyValue_NULL;
            set_no_attribute_error(owner, name);
            res = -1;
            break;
    }
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

/* Specialized version of _PyObject_GenericGetAttrWithDict
   specifically for the LOAD_METHOD opcode.

   Return 1 if a method is found, 0 if it's a regular attribute
   from __dict__ or something returned by using a descriptor
   protocol.

   `method` will point to the resolved attribute or NULL.  In the
   latter case, an error will be set.
*/
/* The span: the object, the name, its type, and the method's slot */
Py_GC_PRECISE int
_PyObject_GetMethod(PyObject *obj, PyObject *name, PyObject **method)
{
    assert(*method == NULL);

    PyThreadState *tstate = _PyThreadState_GET();
    PyTypeObject *tp = Py_TYPE(obj);
    PyValue slots[4];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, obj);
    _PyValueSpan_AppendObject(&span, name);
    _PyValueSpan_AppendObject(&span, (PyObject *)tp);
    _PyValueSpan_Append(&span, PyValue_NULL);
    int res = 0;
    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0) {
            goto done;
        }
    }

    if (tp->tp_getattro != PyObject_GenericGetAttr || !PyUnicode_CheckExact(name)) {
        *method = PyObject_GetAttr(obj, name);
        goto done;
    }

    res = get_method_value(PyValue_FromObject(obj), name, &slots[3], 0);
    if (res < 0) {
        res = 0;
        goto done;
    }
    // NULL means an error, so a box that cannot be made aborts.
    *method = _PyValue_ObjectOrAbort(slots[3], "_PyObject_GetMethod");
done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

// Look up a method on `self` by `name`.
//
// On success, `*method` is set and the function returns 0 or 1. If the
// return value is 1, the call is an unbound method and `*self` is the
// "self" or "cls" argument to pass. If the return value is 0, the call is
// a regular function and `*self` is cleared.
//
// On error, returns -1, clears `*self`, and sets an exception.
Py_GC_PRECISE int
_PyObject_GetMethodValue(PyThreadState *ts, PyValue *self,
                            PyObject *name, PyValue *method)
{
    // The lookup runs on the value: an inline self is not boxed here.  A
    // method is returned unbound with *self unchanged, and the call boxes
    // its receiver (method_check_args); a descriptor that binds self boxes
    // it in descr_get_value().
    assert(PyValue_IsNull(*method));

    // The span: self's value, the name and self's type.  *self and *method
    // are the caller's, which the collector sees.
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(ts, &span, slots);
    _PyValueSpan_Append(&span, *self);
    _PyValueSpan_AppendObject(&span, name);
    PyTypeObject *tp = PyValue_Type(slots[0]);
    _PyValueSpan_AppendObject(&span, (PyObject *)tp);
    int res;
    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0) {
            *self = PyValue_NULL;
            res = -1;
            goto done;
        }
    }

    if (tp->tp_getattro != PyObject_GenericGetAttr || !PyUnicode_CheckExact(name)) {
        PyValue attr = PyObject_GetAttrValue(slots[0], name);
        *self = PyValue_NULL;
        if (PyValue_IsNull(attr)) {
            res = -1;
            goto done;
        }
        *method = attr;
        res = 0;
        goto done;
    }

    res = get_method_value(slots[0], name, method, 1);
    if (res == 2) {
        *self = PyValue_FromObject((PyObject *)tp);
        res = 1;
    }
    else if (res != 1) {
        *self = PyValue_NULL;
    }
done:
    _PyThreadState_PopValueRoot(ts, &span);
    return res;
}


/* Generic GetAttr functions - put these in your tp_[gs]etattro slot. */

/* _PyObject_GenericGetAttrWithDict() with the attribute as its value: an
   attribute held as an inline value in the instance, its dict or the class
   is returned with no box.  PyValue_NULL with an exception set on failure,
   or with none if suppress is 1 and the attribute is missing.  The callers
   anchor the owner, the name and the dict.  The span: the type, which a
   reentrant lookup may replace, the plan's value from the MRO, and the
   slot of the result. */
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static PyValue
generic_getattr_value(PyValue owner, PyObject *name, PyObject *dict,
                      int suppress)
{
    /* Make sure the logic of _PyObject_GetMethod is in sync with
       this method.

       When suppress=1, this function suppresses AttributeError.

       OWNER may be an inline int or float: its plans are the type's (CLASS_
       VALUE, METHOD, DESCR, DATA_DESCR, ABSENT), and it is boxed only to
       bind a descriptor.  A dict is given only with an object owner.
    */

    PyTypeObject *tp = PyValue_Type(owner);
    PyObject *obj = PyValue_IsInline(owner) ? NULL : PyValue_ObjectBorrow(owner);
    assert(dict == NULL || obj != NULL);

    if (!PyUnicode_Check(name)){
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return PyValue_NULL;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, (PyObject *)tp);
    _PyValueSpan_Append(&span, PyValue_NULL);
    PyValue *resp = &slots[1];
    PyValue res = PyValue_NULL;

    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0) {
            goto done;
        }
    }

    _PyLookupPlan plan;
    _PyShape_LookupPlan(_PyValue_Shape(owner), name, &plan);
    _PyValueSpan_Append(&span, plan.value);
    PyValue class_value = slots[2];
    // An inline value in the class is not a descriptor: like any other
    // class attribute that is not, it is the result unless the instance
    // has the name.
    PyObject *descr = PyValue_IsObject(class_value)
                      ? PyValue_ObjectBorrow(class_value) : NULL;

    if (plan.kind == _Py_LOOKUP_DATA_DESCR || plan.kind == _Py_LOOKUP_PROPERTY) {
        if (plan.flags & _Py_LOOKUP_MEMBER) {
            // A member is a field of an instance: never an inline owner's.
            assert(obj != NULL);
            res = *(PyValue *)((char *)obj + plan.offset);
            if (!PyValue_IsNull(res) || suppress) {
                // gh-157840: an unbound member under suppress allocates no
                // AttributeError.
                goto done;
            }
            // Unbound: the descriptor raises the error.
        }
        else if (suppress && obj != NULL
                 && Py_IS_TYPE(descr, &PyMemberDescr_Type)) {
            // gh-157840, for the members a plan does not load directly.
            PyMemberDef *member = ((PyMemberDescrObject *)descr)->d_member;
            if ((member->type == Py_T_OBJECT_EX
                 || member->type == _Py_T_VALUE_EX)
                && !(member->flags & Py_AUDIT_READ)
                && PyObject_TypeCheck(obj, PyDescr_TYPE(descr))) {
                // Zero bits are NULL as a PyObject * and as a PyValue.
                PyObject **addr = _PyMember_GetOffset(obj, member);
                if (FT_ATOMIC_LOAD_PTR(*addr) == NULL) {
                    goto done;
                }
            }
        }
        res = descr_get_value(descr, owner, suppress);
        goto done;
    }

    int found = obj == NULL ? 0 : load_from_storage(obj, name, &plan, dict, resp);
    if (found > 0) {
        res = *resp;
        goto done;
    }
    if (found < 0) {
        if (suppress && PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
        }
        else {
            goto done;
        }
    }
    res = PyValue_NULL;

    switch (plan.fallback) {
        case _Py_LOOKUP_METHOD:
        case _Py_LOOKUP_DESCR:
            res = descr_get_value(descr, owner, suppress);
            break;
        case _Py_LOOKUP_CLASS_VALUE:
            res = class_value;
            break;
        default:
            assert(plan.fallback == _Py_LOOKUP_ABSENT);
            if (!suppress) {
                set_no_attribute_error(owner, name);
            }
            break;
    }
  done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

/* The span: the object, the name and the dict */
Py_GC_PRECISE PyValue
_PyObject_GenericGetAttrWithDict(PyObject *obj, PyObject *name,
                                 PyObject *dict, int suppress)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, obj);
    _PyValueSpan_AppendObject(&span, name);
    _PyValueSpan_AppendObject(&span, dict);
    PyValue res = generic_getattr_value(PyValue_FromObject(obj), name, dict,
                                        suppress);
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

/* The span: the object and the name */
Py_GC_PRECISE PyValue
PyObject_GenericGetAttr(PyObject *obj, PyObject *name)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[2];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, obj);
    _PyValueSpan_AppendObject(&span, name);
    PyValue res = generic_getattr_value(PyValue_FromObject(obj), name, NULL, 0);
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

/* The generic setattr of value, or its deletion if value is PyValue_NULL,
   on obj.  An inline value is stored without a box unless a descriptor
   takes it.  The callers anchor obj, the name, the value and the dict.  The
   span: the type, which a reentrant store may replace, and the class
   attribute from the MRO. */
Py_GC_PRECISE Py_GC_ARGS_ANCHORED static int
generic_setattr_value(PyObject *obj, PyObject *name, PyValue value,
                      PyObject *dict)
{
    PyTypeObject *tp = Py_TYPE(obj);
    PyObject *descr;
    descrsetfunc f;
    int res = -1;

    assert(!PyType_IsSubtype(tp, &PyType_Type));
    if (!PyUnicode_Check(name)){
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return -1;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[2];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, (PyObject *)tp);
    _PyValueSpan_Append(&span, PyValue_NULL);
    if (!_PyType_IsReady(tp) && PyType_Ready(tp) < 0) {
        goto done;
    }

    // Phase 1, then the store plan.  Nothing is cached here, so whether
    // the plan could be is not asked.
    _PyType_LookupValueAndVersion(tp, name, &slots[1]);
    PyValue class_value = slots[1];
    // A class attribute held inline is not a descriptor, but it makes the
    // name read-only below for an object with no storage.
    descr = PyValue_IsObject(class_value) ? PyValue_ObjectBorrow(class_value)
                                          : NULL;
    if (PyValue_IsNull(class_value) && PyErr_Occurred()) {
        goto done;
    }
    _PyStorePlan plan;
    _PyShape_ResolveStore(_PyObject_SHAPE(obj), name, class_value, 0,
                          PyValue_IsNull(value) ? _Py_STORE_DELETE : _Py_STORE_SET,
                          PyValue_IsNull(value) ? _Py_SLOT_BOXED
                                                : _PyValue_SlotKind(value),
                          &plan);

    if (plan.outcome == _Py_STORE_DATA_DESCR) {
        if (plan.flags & _Py_LOOKUP_MEMBER) {
            PyValue *addr = (PyValue *)((char *)obj + plan.offset);
            if (!PyValue_IsNull(value) || !PyValue_IsNull(*addr)) {
                *addr = value;
                res = 0;
                goto done;
            }
            // Deleting an unbound member: the descriptor raises the error.
        }
        f = Py_TYPE(descr)->tp_descr_set;
        res = f(descr, obj, value);
        goto done;
    }

    if (dict != NULL) {
        Py_BEGIN_CRITICAL_SECTION(dict);
        res = _PyDict_SetItem_LockHeld((PyDictObject *)dict, name, value);
        Py_END_CRITICAL_SECTION();
        goto error_check;
    }

    PyObject **dictptr;
    switch (plan.outcome) {
        case _Py_STORE_GENERALISE:
            // The field's kind does not fit the value: generalise it in
            // place, then write the slot (note 09 section 5).
            _PyShape_Generalise(_PyObject_SHAPE(obj), plan.slot);
            _Py_FALLTHROUGH;
        case _Py_STORE_REPLACE:
            // A shape-mode object: the field's slot.
            *(PyValue *)((char *)obj + plan.offset) = value;
            res = 0;
            goto done;
        case _Py_STORE_MISSING:
            set_no_attribute_error(PyValue_FromObject(obj), name);
            goto done;
        case _Py_STORE_ADD:
        case _Py_STORE_REMOVE:
            // Prepare the edge, then commit; a limit converts.
            res = _PyObject_ShapeStoreAttr(obj, name, value);
            goto done;
        case _Py_STORE_DICT:
            if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
                // Slot 0's dict, or a non-exact name converting first.
                res = _PyObject_ShapeStoreAttr(obj, name, value);
                goto done;
            }
            dictptr = _PyObject_ComputedDictPointer(obj);
            if (dictptr != NULL) {
                res = _PyObjectDict_SetItem(tp, obj, dictptr, name, value);
                goto error_check;
            }
            break;
        default:
            assert(plan.outcome == _Py_STORE_NO_STORAGE);
            break;
    }
    // No instance storage.
    if (PyValue_IsNull(class_value)) {
        if (tp->tp_setattro == PyObject_GenericSetAttr) {
            PyErr_Format(PyExc_AttributeError,
                        "'%.100s' object has no attribute '%U' and no "
                        "__dict__ for setting new attributes",
                        tp->tp_name, name);
        }
        else {
            PyErr_Format(PyExc_AttributeError,
                        "'%.100s' object has no attribute '%U'",
                        tp->tp_name, name);
        }
        _PyObject_SetAttributeErrorContext(PyValue_FromObject(obj), name);
    }
    else {
        PyErr_Format(PyExc_AttributeError,
                    "'%.100s' object attribute '%U' is read-only",
                    tp->tp_name, name);
    }
    goto done;

  error_check:
    if (res < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
        set_no_attribute_error(PyValue_FromObject(obj), name);
    }
  done:
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

/* The span: the object, the name, the value and the dict */
Py_GC_PRECISE int
_PyObject_GenericSetAttrWithDict(PyObject *obj, PyObject *name,
                                 PyValue value, PyObject *dict)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[4];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, obj);
    _PyValueSpan_AppendObject(&span, name);
    _PyValueSpan_Append(&span, value);
    _PyValueSpan_AppendObject(&span, dict);
    int res = generic_setattr_value(obj, name, value, dict);
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

/* The span: the object, the name and the value */
Py_GC_PRECISE int
PyObject_GenericSetAttr(PyObject *obj, PyObject *name, PyValue value)
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyValue slots[3];
    _PyValueRoot span;
    _PyThreadState_PushValueSpan(tstate, &span, slots);
    _PyValueSpan_AppendObject(&span, obj);
    _PyValueSpan_AppendObject(&span, name);
    _PyValueSpan_Append(&span, value);
    int res = generic_setattr_value(obj, name, value, NULL);
    _PyThreadState_PopValueRoot(tstate, &span);
    return res;
}

int
PyObject_GenericSetDict(PyObject *obj, PyValue value, void *context)
{
    if (PyValue_IsNull(value)) {
        PyErr_SetString(PyExc_TypeError, "cannot delete __dict__");
        return -1;
    }
    return _PyObject_SetDict(obj, value);
}


/* Test a value used as condition, e.g., in a while or if statement.
   Return -1 if an error occurred */

static int
object_is_true(PyObject *v)
{
    Py_ssize_t res;
    if (v == Py_True)
        return 1;
    if (v == Py_False)
        return 0;
    if (v == Py_None)
        return 0;
    else if (Py_TYPE(v)->tp_as_number != NULL &&
             Py_TYPE(v)->tp_as_number->nb_bool != NULL)
        res = (*Py_TYPE(v)->tp_as_number->nb_bool)(v);
    else if (Py_TYPE(v)->tp_as_mapping != NULL &&
             Py_TYPE(v)->tp_as_mapping->mp_length != NULL)
        res = (*Py_TYPE(v)->tp_as_mapping->mp_length)(v);
    else if (Py_TYPE(v)->tp_as_sequence != NULL &&
             Py_TYPE(v)->tp_as_sequence->sq_length != NULL)
        res = (*Py_TYPE(v)->tp_as_sequence->sq_length)(v);
    else
        return 1;
    /* if it is negative, it should be either -1 or -2 */
    return (res > 0) ? 1 : Py_SAFE_DOWNCAST(res, Py_ssize_t, int);
}

/* equivalent of 'not v'
   Return -1 if an error occurred */

int
PyObject_Not(PyObject *v)
{
    int res;
    res = PyObject_IsTrue(v);
    if (res < 0)
        return res;
    return res == 0;
}

/* Test whether an object can be called */

int
PyCallable_Check(PyObject *x)
{
    if (x == NULL)
        return 0;
    return Py_TYPE(x)->tp_call != NULL;
}


/* Helper for PyObject_Dir without arguments: returns the local scope. */
static PyObject *
_dir_locals(void)
{
    PyObject *names;
    PyObject *locals;

    if (_PyEval_GetFrame() != NULL) {
        locals = _PyEval_GetFrameLocals();
    }
    else {
        PyThreadState *tstate = _PyThreadState_GET();
        locals = _PyEval_GetGlobalsFromRunningMain(tstate);
        if (locals == NULL) {
            if (!_PyErr_Occurred(tstate)) {
                locals = _PyEval_GetFrameLocals();
                assert(_PyErr_Occurred(tstate));
            }
        }
        else {
            Py_INCREF(locals);
        }
    }
    if (locals == NULL) {
        return NULL;
    }

    names = PyMapping_Keys(locals);
    Py_DECREF(locals);
    if (!names) {
        return NULL;
    }
    if (!PyList_Check(names)) {
        PyErr_Format(PyExc_TypeError,
            "dir(): expected keys() of locals to be a list, "
            "not '%.200s'", Py_TYPE(names)->tp_name);
        Py_DECREF(names);
        return NULL;
    }
    if (PyList_Sort(names)) {
        Py_DECREF(names);
        return NULL;
    }
    return names;
}

/* Helper for PyObject_Dir: object introspection. */
static PyObject *
_dir_object(PyObject *obj)
{
    PyObject *result, *sorted;
    PyObject *dirfunc = _PyObject_LookupSpecial(obj, &_Py_ID(__dir__));

    assert(obj != NULL);
    if (dirfunc == NULL) {
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_TypeError, "object does not provide __dir__");
        return NULL;
    }
    /* use __dir__ */
    result = _PyObject_CallNoArgs(dirfunc);
    Py_DECREF(dirfunc);
    if (result == NULL)
        return NULL;
    /* return sorted(result) */
    sorted = PySequence_List(result);
    Py_DECREF(result);
    if (sorted == NULL)
        return NULL;
    if (PyList_Sort(sorted)) {
        Py_DECREF(sorted);
        return NULL;
    }
    return sorted;
}

/* Implementation of dir() -- if obj is NULL, returns the names in the current
   (local) scope.  Otherwise, performs introspection of the object: returns a
   sorted list of attribute names (supposedly) accessible from the object
*/
PyObject *
PyObject_Dir(PyObject *obj)
{
    return (obj == NULL) ? _dir_locals() : _dir_object(obj);
}

/*
None is a non-NULL undefined value.
There is (and should be!) no way to create other objects of this type,
so there is exactly one (which is indestructible, by the way).
*/

/* ARGSUSED */
static PyObject *
none_repr(PyObject *op)
{
    return PyUnicode_FromString("None");
}

static void
none_dealloc(PyObject* none)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref None out of existence. Instead,
     * since None is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(none);
}

static PyObject *
none_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "NoneType takes no arguments");
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
none_bool(PyObject *v)
{
    return 0;
}

static Py_hash_t none_hash(PyObject *v)
{
    return 0xFCA86420;
}

static PyNumberMethods none_as_number = {
    0,                          /* nb_add */
    0,                          /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_remainder */
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* nb_positive */
    0,                          /* nb_absolute */
    none_bool,                  /* nb_bool */
    0,                          /* nb_invert */
    0,                          /* nb_lshift */
    0,                          /* nb_rshift */
    0,                          /* nb_and */
    0,                          /* nb_xor */
    0,                          /* nb_or */
    0,                          /* nb_int */
    0,                          /* nb_reserved */
    0,                          /* nb_float */
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    0,                          /* nb_floor_divide */
    0,                          /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
    0,                          /* nb_index */
};

PyDoc_STRVAR(none_doc,
"NoneType()\n"
"--\n\n"
"The type of the None singleton.");

PyTypeObject _PyNone_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "NoneType",
    0,
    0,
    none_dealloc,       /*tp_dealloc*/
    0,                  /*tp_vectorcall_offset*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    none_repr,          /*tp_repr*/
    &none_as_number,    /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    none_hash,          /*tp_hash */
    0,                  /*tp_call */
    0,                  /*tp_str */
    0,                  /*tp_getattro */
    0,                  /*tp_setattro */
    0,                  /*tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /*tp_flags */
    none_doc,           /*tp_doc */
    0,                  /*tp_traverse */
    0,                  /*tp_clear */
    _Py_BaseObject_RichCompare, /*tp_richcompare */
    0,                  /*tp_weaklistoffset */
    0,                  /*tp_iter */
    0,                  /*tp_iternext */
    0,                  /*tp_methods */
    0,                  /*tp_members */
    0,                  /*tp_getset */
    0,                  /*tp_base */
    0,                  /*tp_dict */
    0,                  /*tp_descr_get */
    0,                  /*tp_descr_set */
    0,                  /*tp_dictoffset */
    0,                  /*tp_init */
    0,                  /*tp_alloc */
    none_new,           /*tp_new */
};

PyObject _Py_NoneStruct = _PyObject_HEAD_INIT(&_PyNone_Type);

/* NotImplemented is an object that can be used to signal that an
   operation is not implemented for the given type combination. */

static PyObject *
NotImplemented_repr(PyObject *op)
{
    return PyUnicode_FromString("NotImplemented");
}

static PyObject *
NotImplemented_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("NotImplemented");
}

static PyMethodDef notimplemented_methods[] = {
    {"__reduce__", NotImplemented_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyObject *
notimplemented_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "NotImplementedType takes no arguments");
        return NULL;
    }
    Py_RETURN_NOTIMPLEMENTED;
}

static void
notimplemented_dealloc(PyObject *notimplemented)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref NotImplemented out of existence. Instead,
     * since Notimplemented is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(notimplemented);
}

static int
notimplemented_bool(PyObject *v)
{
    PyErr_SetString(PyExc_TypeError,
                    "NotImplemented should not be used in a boolean context");
    return -1;
}

static PyNumberMethods notimplemented_as_number = {
    .nb_bool = notimplemented_bool,
};

PyDoc_STRVAR(notimplemented_doc,
"NotImplementedType()\n"
"--\n\n"
"The type of the NotImplemented singleton.");

PyTypeObject _PyNotImplemented_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "NotImplementedType",
    0,
    0,
    notimplemented_dealloc,       /*tp_dealloc*/ /*never called*/
    0,                  /*tp_vectorcall_offset*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    NotImplemented_repr,        /*tp_repr*/
    &notimplemented_as_number,  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash */
    0,                  /*tp_call */
    0,                  /*tp_str */
    0,                  /*tp_getattro */
    0,                  /*tp_setattro */
    0,                  /*tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /*tp_flags */
    notimplemented_doc, /*tp_doc */
    0,                  /*tp_traverse */
    0,                  /*tp_clear */
    0,                  /*tp_richcompare */
    0,                  /*tp_weaklistoffset */
    0,                  /*tp_iter */
    0,                  /*tp_iternext */
    notimplemented_methods, /*tp_methods */
    0,                  /*tp_members */
    0,                  /*tp_getset */
    0,                  /*tp_base */
    0,                  /*tp_dict */
    0,                  /*tp_descr_get */
    0,                  /*tp_descr_set */
    0,                  /*tp_dictoffset */
    0,                  /*tp_init */
    0,                  /*tp_alloc */
    notimplemented_new, /*tp_new */
};

PyObject _Py_NotImplementedStruct = _PyObject_HEAD_INIT(&_PyNotImplemented_Type);


PyStatus
_PyObject_InitState(PyInterpreterState *interp)
{
    return _PyStatus_OK();
}

void
_PyObject_FiniState(PyInterpreterState *interp)
{
}


extern PyTypeObject _PyLegacyEventHandler_Type;
extern PyTypeObject _PyLineIterator;
extern PyTypeObject _PyMemoryIter_Type;
extern PyTypeObject _PyPositionsIterator;
extern PyTypeObject _Py_GenericAliasIterType;

static PyTypeObject* static_types[_Py_NUM_MANAGED_PREINITIALIZED_TYPES] = {
    // The two most important base types: must be initialized first and
    // deallocated last.
    &PyBaseObject_Type,
    &PyType_Type,

    // PyStaticMethod_Type and PyCFunction_Type are used by PyType_Ready()
    // on other types and so must be initialized first.
    &PyStaticMethod_Type,
    &PyCFunction_Type,

    // Static types with base=&PyBaseObject_Type
    &PyAsyncGen_Type,
    &PyByteArrayIter_Type,
    &PyByteArray_Type,
    &PyBytesIter_Type,
    &PyBytes_Type,
    &PyCallIter_Type,
    &PyCapsule_Type,
    &PyCell_Type,
    &PyClassMethodDescr_Type,
    &PyClassMethod_Type,
    &PyCode_Type,
    &PyComplex_Type,
    &PyContextToken_Type,
    &PyContextVar_Type,
    &PyContext_Type,
    &PyCoro_Type,
    &PyDictItems_Type,
    &PyDictIterItem_Type,
    &PyDictIterKey_Type,
    &PyDictIterValue_Type,
    &PyDictKeys_Type,
    &PyDictProxy_Type,
    &PyDictRevIterItem_Type,
    &PyDictRevIterKey_Type,
    &PyDictRevIterValue_Type,
    &PyDictValues_Type,
    &PyDict_Type,
    &PyEllipsis_Type,
    &PyEnum_Type,
    &PyFilter_Type,
    &PyFloat_Type,
    &PyFrameLocalsProxy_Type,
    &PyFrame_Type,
    &PyFrozenDict_Type,
    &PyFrozenSet_Type,
    &PyFunction_Type,
    &PyGen_Type,
    &PyGetSetDescr_Type,
    &PyInstanceMethod_Type,
    &PyLazyImport_Type,
    &PyListIter_Type,
    &PyListRevIter_Type,
    &PyList_Type,
    &PyLongRangeIter_Type,
    &PyLong_Type,
    &PyMap_Type,
    &PyMemberDescr_Type,
    &PyMemoryView_Type,
    &PyMethodDescr_Type,
    &PyMethod_Type,
    &PyModuleDef_Type,
    &PyModule_Type,
    &PyODictIter_Type,
    &PyPickleBuffer_Type,
    &PyProperty_Type,
    &PyRangeIter_Type,
    &PyRange_Type,
    &PyReversed_Type,
    &PySTEntry_Type,
    &PySentinel_Type,
    &PySeqIter_Type,
    &PySetIter_Type,
    &PySet_Type,
    &PySlice_Type,
    &PyStdPrinter_Type,
    &PySuper_Type,
    &PyTraceBack_Type,
    &PyTupleIter_Type,
    &PyTuple_Type,
    &PyUnicodeIter_Type,
    &PyUnicode_Type,
    &PyWrapperDescr_Type,
    &PyZip_Type,
    &Py_GenericAliasType,
    &_PyAsyncGenASend_Type,
    &_PyAsyncGenAThrow_Type,
    &_PyAsyncGenWrappedValue_Type,
    &_PyBufferWrapper_Type,
    &_PyContextTokenMissing_Type,
    &_PyCoroWrapper_Type,
    &_Py_GenericAliasIterType,
    &_PyHamtItems_Type,
    &_PyHamtKeys_Type,
    &_PyHamtValues_Type,
    &_PyHamt_ArrayNode_Type,
    &_PyHamt_BitmapNode_Type,
    &_PyHamt_CollisionNode_Type,
    &_PyHamt_Type,
    &_PyInstructionSequence_Type,
    &_PyInterpolation_Type,
    &_PyLegacyEventHandler_Type,
    &_PyLineIterator,
    &_PyManagedBuffer_Type,
    &_PyMemoryIter_Type,
    &_PyMethodWrapper_Type,
    &_PyNamespace_Type,
    &_PyObjectArray_Type,
    &_PyNone_Type,
    &_PyNotImplemented_Type,
    &_PyPositionsIterator,
    &_PyTemplate_Type,
    &_PyTemplateIter_Type,
    &_PyUnicodeASCIIIter_Type,
    &_PyUnion_Type,
    &_PyWeakref_CallableProxyType,
    &_PyWeakref_ProxyType,
    &_PyWeakref_RefType,
    &_PyTypeAlias_Type,
    &_PyNoDefault_Type,

    // subclasses: _PyTypes_FiniTypes() deallocates them before their base
    // class
    &PyBool_Type,         // base=&PyLong_Type
    &PyCMethod_Type,      // base=&PyCFunction_Type
    &PyODictItems_Type,   // base=&PyDictItems_Type
    &PyODictKeys_Type,    // base=&PyDictKeys_Type
    &PyODictValues_Type,  // base=&PyDictValues_Type
    &PyODict_Type,        // base=&PyDict_Type
};


/* The root shapes of the static builtin types, filled before any static
   object is used: a static object's header points to its type's root shape,
   whose type field the positional initializers of PyTypeObject leave NULL.
   PyType_Ready() fills them again. */
void
_PyTypes_InitRootShapes(void)
{
    for (size_t i=0; i < Py_ARRAY_LENGTH(static_types); i++) {
        if (static_types[i] != NULL) {
            _PyShape_InitRoot(static_types[i]);
        }
    }
    _PyExc_InitRootShapes();
}


PyStatus
_PyTypes_InitTypes(PyInterpreterState *interp)
{
    // All other static types (unless initialized elsewhere)
    for (size_t i=0; i < Py_ARRAY_LENGTH(static_types); i++) {
        PyTypeObject *type = static_types[i];
        if (type == NULL) {
            continue;
        }
        if (_PyStaticType_InitBuiltin(interp, type) < 0) {
            return _PyStatus_ERR("Can't initialize builtin type");
        }
        if (type == &PyType_Type) {
            // Sanitify checks of the two most important types
            assert(PyBaseObject_Type.tp_base == NULL);
            assert(PyType_Type.tp_base == &PyBaseObject_Type);
        }
    }

    // Cache __reduce__ from PyBaseObject_Type object
    PyObject *baseobj_dict = _PyType_GetDict(&PyBaseObject_Type);
    PyObject *baseobj_reduce = PyDict_GetItemWithError(baseobj_dict, &_Py_ID(__reduce__));
    if (baseobj_reduce == NULL && PyErr_Occurred()) {
        return _PyStatus_ERR("Can't get __reduce__ from base object");
    }
    _Py_INTERP_CACHED_OBJECT(interp, objreduce) = baseobj_reduce;

    // Must be after static types are initialized
    if (_Py_initialize_generic(interp) < 0) {
        return _PyStatus_ERR("Can't initialize generic types");
    }

    return _PyStatus_OK();
}


// Best-effort function clearing static types.
//
// Don't deallocate a type if it still has subclasses. If a Py_Finalize()
// sub-function is interrupted by CTRL+C or fails with MemoryError, some
// subclasses are not cleared properly. Leave the static type unchanged in this
// case.
void
_PyTypes_FiniTypes(PyInterpreterState *interp)
{
    // Deallocate types in the reverse order to deallocate subclasses before
    // their base classes.
    for (Py_ssize_t i=Py_ARRAY_LENGTH(static_types)-1; i>=0; i--) {
        PyTypeObject *type = static_types[i];
        if (type == NULL) {
            continue;
        }
        _PyStaticType_FiniBuiltin(interp, type);
    }
}


static inline void
new_reference(PyObject *op)
{
    // A new object has no collector flags, whatever its block held before,
    // and its shape, if the caller set it first, is kept.  The object map
    // tells the collector's heap walk that the block holds an object.
    op->ob_header &= _Py_HEADER_SHAPE_MASK;
    _PyGC_ObjectMapSet(op);
    _PyReftracerTrack(op, PyRefTracer_CREATE);
}

void
_Py_NewReference(PyObject *op)
{
    new_reference(op);
}

void
_Py_NewReferenceNoTotal(PyObject *op)
{
    new_reference(op);
}

void
_Py_SetImmortalUntracked(PyObject *op)
{
    // Check if already immortal to avoid degrading from static immortal to plain immortal
    if (_Py_IsImmortal(op)) {
        return;
    }
    // Keep the collector's bits: the object stays an object in the heap walk,
    // which never sweeps it, and its references are roots.
    op->ob_flags |= _Py_IMMORTAL_FLAGS;
    _PyGC_LiveAddImmortal(op);
}

void
_Py_SetImmortal(PyObject *op)
{
    if (PyObject_IS_GC(op) && _PyObject_GC_IS_TRACKED(op)) {
        _PyObject_GC_UNTRACK(op);
    }
    _Py_SetImmortalUntracked(op);
}

void
_PyObject_SetDeferredRefcount(PyObject *op)
{
}

int
PyUnstable_Object_EnableDeferredRefcount(PyObject *op)
{
    return 0;
}

int
PyUnstable_Object_IsUniqueReferencedTemporary(PyObject *op)
{
    // Nothing is reference counted, so no object is known to be unique.
    (void)op;
    return 0;
}

int
PyUnstable_TryIncRef(PyObject *op)
{
    return _Py_TryIncref(op);
}

void
PyUnstable_EnableTryIncRef(PyObject *op)
{
}

int
PyUnstable_SetImmortal(PyObject *op)
{
    assert(op != NULL);
    // The object must be uniquely referenced. Nothing is reference counted,
    // so that cannot be shown and no object is made immortal.
    (void)op;
    return 0;
}

void
_Py_ResurrectReference(PyObject *op)
{
}

void
_Py_ForgetReference(PyObject *op)
{
}




/* Hack to force loading of abstract.o */
Py_ssize_t (*_Py_abstract_hack)(PyObject *) = PyObject_Size;


/* These methods are used to control infinite recursion in repr, str, print,
   etc.  Container objects that may recursively contain themselves,
   e.g. builtin dictionaries and lists, should use Py_ReprEnter() and
   Py_ReprLeave() to avoid infinite recursion.

   Py_ReprEnter() returns 0 the first time it is called for a particular
   object and 1 every time thereafter.  It returns -1 if an exception
   occurred.  Py_ReprLeave() has no return value.

   See dictobject.c and listobject.c for examples of use.
*/

int
Py_ReprEnter(PyObject *obj)
{
    PyObject *dict;
    PyObject *list;
    Py_ssize_t i;

    dict = PyThreadState_GetDict();
    /* Ignore a missing thread-state, so that this function can be called
       early on startup. */
    if (dict == NULL)
        return 0;
    list = PyDict_GetItemWithError(dict, &_Py_ID(Py_Repr));
    if (list == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        list = PyList_New(0);
        if (list == NULL)
            return -1;
        if (PyDict_SetItem(dict, &_Py_ID(Py_Repr), list) < 0) {
            return -1;
        }
    }
    i = PyList_GET_SIZE(list);
    while (--i >= 0) {
        if (PyList_GET_ITEM(list, i) == obj)
            return 1;
    }
    if (PyList_Append(list, obj) < 0)
        return -1;
    return 0;
}

void
Py_ReprLeave(PyObject *obj)
{
    PyObject *dict;
    PyObject *list;
    Py_ssize_t i;

    PyObject *exc = PyErr_GetRaisedException();

    dict = PyThreadState_GetDict();
    if (dict == NULL)
        goto finally;

    list = PyDict_GetItemWithError(dict, &_Py_ID(Py_Repr));
    if (list == NULL || !PyList_Check(list))
        goto finally;

    i = PyList_GET_SIZE(list);
    /* Count backwards because we always expect obj to be list[-1] */
    while (--i >= 0) {
        if (PyList_GET_ITEM(list, i) == obj) {
            PyList_SetSlice(list, i, i + 1, NULL);
            break;
        }
    }

finally:
    /* ignore exceptions because there is no way to report them. */
    PyErr_SetRaisedException(exc);
}

void _Py_NO_RETURN
_PyObject_AssertFailed(PyObject *obj, const char *expr, const char *msg,
                       const char *file, int line, const char *function)
{
    fprintf(stderr, "%s:%d: ", file, line);
    if (function) {
        fprintf(stderr, "%s: ", function);
    }
    fflush(stderr);

    if (expr) {
        fprintf(stderr, "Assertion \"%s\" failed", expr);
    }
    else {
        fprintf(stderr, "Assertion failed");
    }
    fflush(stderr);

    if (msg) {
        fprintf(stderr, ": %s", msg);
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    if (_PyObject_IsFreed(obj)) {
        /* It seems like the object memory has been freed:
           don't access it to prevent a segmentation fault. */
        fprintf(stderr, "<object at %p is freed>\n", obj);
        fflush(stderr);
    }
    else {
        /* Display the traceback where the object has been allocated.
           Do it before dumping repr(obj), since repr() is more likely
           to crash than dumping the traceback. */
        _PyMem_DumpTraceback(fileno(stderr), obj);

        /* This might succeed or fail, but we're about to abort, so at least
           try to provide any extra info we can: */
        PyObject_Dump(obj);

        fprintf(stderr, "\n");
        fflush(stderr);
    }

    Py_FatalError("_PyObject_AssertFailed");
}


void
_Py_Dealloc(PyObject *op)
{
    // Only the collector's sweep deallocates (Python/gc_live.c).  This is
    // reached from extensions built for reference counting whose decrefs
    // reached zero.
    (void)op;
}


PyObject **
PyObject_GET_WEAKREFS_LISTPTR(PyObject *op)
{
    return (PyObject **)_PyWeakref_ListPtr(op, 1);
}


#undef Py_NewRef
#undef Py_XNewRef

// Export Py_NewRef() and Py_XNewRef() as regular functions for the stable ABI.
PyObject*
Py_NewRef(PyObject *obj)
{
    return _Py_NewRef(obj);
}

PyObject*
Py_XNewRef(PyObject *obj)
{
    return _Py_XNewRef(obj);
}

#undef Py_Is
#undef Py_IsNone
#undef Py_IsTrue
#undef Py_IsFalse

// Export Py_Is(), Py_IsNone(), Py_IsTrue(), Py_IsFalse() as regular functions
// for the stable ABI.
int Py_Is(PyObject *x, PyObject *y)
{
    return (x == y);
}

int Py_IsNone(PyObject *x)
{
    return Py_Is(x, Py_None);
}

int Py_IsTrue(PyObject *x)
{
    return Py_Is(x, Py_True);
}

int Py_IsFalse(PyObject *x)
{
    return Py_Is(x, Py_False);
}


// Py_SET_REFCNT() implementation for stable ABI
void
_Py_SetRefcnt(PyObject *ob, Py_ssize_t refcnt)
{
    Py_SET_REFCNT(ob, refcnt);
}

int PyRefTracer_SetTracer(PyRefTracer tracer, void *data) {
    _Py_AssertHoldsTstate();

    _PyEval_StopTheWorldAll(&_PyRuntime);
    if (_PyRuntime.ref_tracer.tracer_func != NULL) {
        _PyReftracerTrack(NULL, PyRefTracer_TRACKER_REMOVED);
        if (PyErr_Occurred()) {
            _PyEval_StartTheWorldAll(&_PyRuntime);
            return -1;
        }
    }
    _PyRuntime.ref_tracer.tracer_func = tracer;
    _PyRuntime.ref_tracer.tracer_data = data;
    _PyEval_StartTheWorldAll(&_PyRuntime);
    return 0;
}

PyRefTracer PyRefTracer_GetTracer(void** data) {
    _Py_AssertHoldsTstate();
    if (data != NULL) {
        *data = _PyRuntime.ref_tracer.tracer_data;
    }
    return _PyRuntime.ref_tracer.tracer_func;
}



static PyObject* constants[] = {
    &_Py_NoneStruct,                   // Py_CONSTANT_NONE
    (PyObject*)(&_Py_FalseStruct),     // Py_CONSTANT_FALSE
    (PyObject*)(&_Py_TrueStruct),      // Py_CONSTANT_TRUE
    &_Py_EllipsisObject,               // Py_CONSTANT_ELLIPSIS
    &_Py_NotImplementedStruct,         // Py_CONSTANT_NOT_IMPLEMENTED
    NULL,  // Py_CONSTANT_ZERO
    NULL,  // Py_CONSTANT_ONE
    NULL,  // Py_CONSTANT_EMPTY_STR
    NULL,  // Py_CONSTANT_EMPTY_BYTES
    NULL,  // Py_CONSTANT_EMPTY_TUPLE
};

void
_Py_GetConstant_Init(void)
{
    constants[Py_CONSTANT_ZERO] = _PyLong_GetZero();
    constants[Py_CONSTANT_ONE] = _PyLong_GetOne();
    constants[Py_CONSTANT_EMPTY_STR] = PyUnicode_New(0, 0);
    constants[Py_CONSTANT_EMPTY_BYTES] = PyBytes_FromStringAndSize("", 0);
    constants[Py_CONSTANT_EMPTY_TUPLE] = PyTuple_New(0);
#ifndef NDEBUG
    for (size_t i=0; i < Py_ARRAY_LENGTH(constants); i++) {
        assert(constants[i] != NULL);
        assert(_Py_IsImmortal(constants[i]));
    }
#endif
}

PyObject*
Py_GetConstant(unsigned int constant_id)
{
    if (constant_id < Py_ARRAY_LENGTH(constants)) {
        return constants[constant_id];
    }
    else {
        PyErr_BadInternalCall();
        return NULL;
    }
}


PyObject*
Py_GetConstantBorrowed(unsigned int constant_id)
{
    // All constants are immortal
    return Py_GetConstant(constant_id);
}

int
PyUnstable_IsImmortal(PyObject *op)
{
    /* Checking a reference count requires a thread state */
    _Py_AssertHoldsTstate();
    assert(op != NULL);
    return _Py_IsImmortal(op);
}

int
PyUnstable_Object_IsUniquelyReferenced(PyObject *op)
{
    _Py_AssertHoldsTstate();
    assert(op != NULL);
    // Nothing is reference counted, so no object is known to be unique.
    return 0;
}

int
_PyObject_VisitType(PyObject *op, visitproc visit, void *arg)
{
    assert(op != NULL);
    PyTypeObject *tp = Py_TYPE(op);
    _PyObject_ASSERT((PyObject *)tp, PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE));
    Py_VISIT(tp);
    return 0;
}

// Implementations for the stable ABI
// Keep these at the end.
#undef Py_TYPE
#undef Py_REFCNT
#undef Py_SIZE
#undef Py_IS_TYPE
#undef Py_SET_SIZE
PyTypeObject* Py_TYPE(PyObject *ob) { return _Py_TYPE_impl(ob); }
Py_ssize_t Py_REFCNT(PyObject *ob) { return _Py_REFCNT(ob); }
Py_ssize_t Py_SIZE(PyObject *o) { return _Py_SIZE_impl(o); }
int Py_IS_TYPE(PyObject *o, PyTypeObject *t) { return _Py_IS_TYPE_impl(o, t); }
void Py_SET_SIZE(PyVarObject *o, Py_ssize_t s) { _Py_SET_SIZE_impl(o, s); }


/* The PyObject * protocol

   These functions are the compatibility layer over the value forms: each
   canonicalises its arguments (an exact int that fits in int32, or an exact
   float other than NaN, becomes inline), calls the value form, and boxes an
   inline result.  The core calls the value forms. */

PyObject *
PyObject_Repr(PyObject *v)
{
    if (v == NULL) {
        return object_repr(v);
    }
    return PyValue_Object(PyObject_ReprValue(PyValue_FromObject(v)));
}

PyObject *
PyObject_Str(PyObject *v)
{
    if (v == NULL) {
        return object_str(v);
    }
    return PyValue_Object(PyObject_StrValue(PyValue_FromObject(v)));
}

PyObject *
PyObject_ASCII(PyObject *v)
{
    if (v == NULL) {
        return object_ascii(v);
    }
    return PyValue_Object(PyObject_ASCIIValue(PyValue_FromObject(v)));
}

PyObject *
PyObject_RichCompare(PyObject *v, PyObject *w, int op)
{
    if (v == NULL || w == NULL) {
        return object_richcompare(v, w, op);
    }
    return PyValue_Object(PyObject_RichCompareValue(PyValue_FromObject(v),
                                                    PyValue_FromObject(w), op));
}

int
PyObject_RichCompareBool(PyObject *v, PyObject *w, int op)
{
    if (v == NULL || w == NULL) {
        return object_richcompare_bool(v, w, op);
    }
    return PyObject_RichCompareBoolValue(PyValue_FromObject(v),
                                         PyValue_FromObject(w), op);
}

Py_hash_t
PyObject_Hash(PyObject *v)
{
    return PyObject_HashValue(PyValue_FromObject(v));
}

int
PyObject_IsTrue(PyObject *v)
{
    return PyObject_IsTrueValue(PyValue_FromObject(v));
}

Py_GC_PRECISE PyObject *
PyObject_GetAttr(PyObject *v, PyObject *name)
{
    return PyValue_Object(PyObject_GetAttrValue(PyValue_FromObject(v), name));
}

Py_GC_PRECISE int
PyObject_SetAttr(PyObject *v, PyObject *name, PyObject *value)
{
    // A NULL value deletes; PyObject_SetAttrValue() reports one passed with
    // an exception set.
    return PyObject_SetAttrValue(PyValue_FromObject(v), name,
                                 PyValue_FromResult(value));
}
