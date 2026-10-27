
/* Generic object operations; and implementation of None */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_context.h"       // _PyContextTokenMissing_Type
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION
#include "pycore_descrobject.h"   // _PyMethodWrapper_Type
#include "pycore_dict.h"          // _PyObject_MaterializeManagedDict()
#include "pycore_function.h"      // _PyClassMethod_GetFunc()
#include "pycore_gc_trace.h"     // _PyGC_LiveAddImmortal()
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
static int object_setattr(PyObject *v, PyObject *name, PyObject *value);
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
    if (Py_REFCNT(self) != 0) {
        _PyObject_ASSERT_FAILED_MSG(self,
                                    "PyObject_CallFinalizerFromDealloc called "
                                    "on object with a non-zero refcount");
    }

    /* Temporarily resurrect the object. */
    _PyObject_ResurrectStart(self);

    PyObject_CallFinalizer(self);

    _PyObject_ASSERT_WITH_MSG(self,
                              Py_REFCNT(self) > 0,
                              "refcount is too small");

    _PyObject_ASSERT(self,
                    (!_PyType_IS_GC(Py_TYPE(self))
                    || _PyObject_GC_IS_TRACKED(self)));

    /* Undo the temporary resurrection; can't use DECREF here, it would
     * cause a recursive call. */
    if (_PyObject_ResurrectEnd(self)) {
        /* tp_finalize resurrected it!
           gh-130202: Note that the object may still be dead in the free
           threaded build in some circumstances, so it's not safe to access
           `self` after this point. For example, the last reference to the
           resurrected `self` may be held by another thread, which can
           concurrently deallocate it. */
        return -1;
    }

    /* this is the normal path out, the caller continues with deallocation. */
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
PyObject *
_PyObject_FunctionStr(PyObject *x)
{
    assert(!PyErr_Occurred());
    PyObject *qualname;
    int ret = PyObject_GetOptionalAttr(x, &_Py_ID(__qualname__), &qualname);
    if (qualname == NULL) {
        if (ret < 0) {
            return NULL;
        }
        return PyObject_Str(x);
    }
    PyObject *module;
    PyObject *result = NULL;
    ret = PyObject_GetOptionalAttr(x, &_Py_ID(__module__), &module);
    if (module != NULL && module != Py_None) {
        ret = PyObject_RichCompareBool(module, &_Py_ID(builtins), Py_NE);
        if (ret < 0) {
            // error
            goto done;
        }
        if (ret > 0) {
            result = PyUnicode_FromFormat("%S.%S()", module, qualname);
            goto done;
        }
    }
    else if (ret < 0) {
        goto done;
    }
    result = PyUnicode_FromFormat("%S()", qualname);
done:
    Py_DECREF(qualname);
    Py_XDECREF(module);
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

PyObject *
PyObject_GetAttrString(PyObject *v, const char *name)
{
    PyObject *w, *res;

    if (Py_TYPE(v)->tp_getattr != NULL)
        return (*Py_TYPE(v)->tp_getattr)(v, (char*)name);
    w = PyUnicode_FromString(name);
    if (w == NULL)
        return NULL;
    res = PyObject_GetAttr(v, w);
    Py_DECREF(w);
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

int
PyObject_SetAttrString(PyObject *v, const char *name, PyObject *w)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (w == NULL && _PyErr_Occurred(tstate)) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        _PyErr_SetString(tstate, PyExc_SystemError,
            "PyObject_SetAttrString() must not be called with NULL value "
            "and an exception set");
        _PyErr_ChainExceptions1Tstate(tstate, exc);
        return -1;
    }

    if (Py_TYPE(v)->tp_setattr != NULL) {
        return (*Py_TYPE(v)->tp_setattr)(v, (char*)name, w);
    }

    PyObject *s = PyUnicode_InternFromString(name);
    if (s == NULL) {
        return -1;
    }

    int res = PyObject_SetAttr(v, s, w);
    Py_DECREF(s);
    return res;
}

int
PyObject_DelAttrString(PyObject *v, const char *name)
{
    return PyObject_SetAttrString(v, name, NULL);
}

int
_PyObject_IsAbstract(PyObject *obj)
{
    int res;
    PyObject* isabstract;

    if (obj == NULL)
        return 0;

    res = PyObject_GetOptionalAttr(obj, &_Py_ID(__isabstractmethod__), &isabstract);
    if (res > 0) {
        res = PyObject_IsTrue(isabstract);
        Py_DECREF(isabstract);
    }
    return res;
}

PyObject *
_PyObject_GetAttrId(PyObject *v, _Py_Identifier *name)
{
    PyObject *result;
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    PyObject *oname = _PyUnicode_FromId(name); /* borrowed */
_Py_COMP_DIAG_POP
    if (!oname)
        return NULL;
    result = PyObject_GetAttr(v, oname);
    return result;
}

int
_PyObject_SetAttributeErrorContext(PyObject* v, PyObject* name)
{
    assert(PyErr_Occurred());
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)){
        return 0;
    }
    // Intercept AttributeError exceptions and augment them to offer suggestions later.
    PyObject *exc = PyErr_GetRaisedException();
    if (!PyErr_GivenExceptionMatches(exc, PyExc_AttributeError)) {
        goto restore;
    }
    PyAttributeErrorObject* the_exc = (PyAttributeErrorObject*) exc;
    // Check if this exception was already augmented
    if (the_exc->name || the_exc->obj) {
        goto restore;
    }
    // Augment the exception with the name and object
    if (PyObject_SetAttr(exc, &_Py_ID(name), name) ||
        PyObject_SetAttr(exc, &_Py_ID(obj), v)) {
        Py_DECREF(exc);
        return 1;
    }
restore:
    PyErr_SetRaisedException(exc);
    return 0;
}

/* Like PyObject_GetAttr but returns a PyValue.
   For types (tp_getattro == _Py_type_getattro), this can return
   a deferred reference to reduce reference count contention. */
static PyValue generic_getattr_value(PyObject *obj, PyObject *name,
                                     PyObject *dict, int suppress);

static int generic_setattr_value(PyObject *obj, PyObject *name, PyValue value,
                                 PyObject *dict);

// PyObject_GetAttrValue() of an object.
static PyValue
getattr_object_value(PyObject *v, PyObject *name)
{
    PyTypeObject *tp = Py_TYPE(v);
    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return PyValue_NULL;
    }

    /* Fast path for types - can return deferred references */
    if (tp->tp_getattro == _Py_type_getattro) {
        PyValue result = _Py_type_getattro_value((PyTypeObject *)v, name, NULL);
        if (PyValue_IsNull(result)) {
            _PyObject_SetAttributeErrorContext(v, name);
        }
        return result;
    }

    /* The generic attribute lookup gives the value, with no box */
    if (tp->tp_getattro == PyObject_GenericGetAttr) {
        PyValue result = generic_getattr_value(v, name, NULL, 0);
        if (PyValue_IsNull(result)) {
            _PyObject_SetAttributeErrorContext(v, name);
        }
        return result;
    }

    /* Fall back to regular PyObject_GetAttr and convert to a PyValue */
    PyObject *result = NULL;
    if (tp->tp_getattro != NULL) {
        result = (*tp->tp_getattro)(v, name);
    }
    else if (tp->tp_getattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            return PyValue_NULL;
        }
        result = (*tp->tp_getattr)(v, (char *)name_str);
    }
    else {
        PyErr_Format(PyExc_AttributeError,
                    "'%.100s' object has no attribute '%U'",
                    tp->tp_name, name);
    }

    if (result == NULL) {
        _PyObject_SetAttributeErrorContext(v, name);
        return PyValue_NULL;
    }
    return PyValue_FromObject(result);
}

/* The object protocol on values (Include/cpython/abstract.h, "The PyValue
   protocols").  An inline int or float is boxed for a type slot, held by a
   value root while the slot runs, unless the operation is done here. */

PyValue
PyObject_GetAttrValue(PyValue v, PyObject *name)
{
    if (!PyValue_IsInline(v)) {
        return getattr_object_value(PyValue_ObjectBorrow(v), name);
    }
    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot root;
    PyObject *v_o = _PyValueRoot_PushObject(tstate, &root, v, _PY_BOX_OTHER);
    PyValue res = v_o == NULL ? PyValue_NULL : getattr_object_value(v_o, name);
    _PyThreadState_PopValueRoot(tstate, &root);
    return res;
}

int
PyObject_SetAttrValue(PyValue v, PyObject *name, PyValue value)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (PyValue_IsObject(v) && PyUnicode_Check(name)
        && Py_TYPE(PyValue_ObjectBorrow(v))->tp_setattro == PyObject_GenericSetAttr)
    {
        // As PyObject_SetAttr() does, intern the name.
        Py_INCREF(name);
        _PyUnicode_InternMortal(tstate->interp, &name);
        int res = generic_setattr_value(PyValue_ObjectBorrow(v), name, value, NULL);
        Py_DECREF(name);
        return res;
    }
    _PyValueRoot v_root, value_root;
    int res = -1;
    PyObject *v_o = _PyValueRoot_PushObject(tstate, &v_root, v, _PY_BOX_OTHER);
    PyObject *value_o = _PyValueRoot_PushObject(tstate, &value_root, value,
                                                _PY_BOX_ATTRIBUTE_STORE);
    if (v_o != NULL && (value_o != NULL || PyValue_IsNull(value))) {
        res = object_setattr(v_o, name, value_o);
    }
    _PyThreadState_PopValueRoot(tstate, &value_root);
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

int
PyObject_GetOptionalAttr(PyObject *v, PyObject *name, PyObject **result)
{
    PyTypeObject *tp = Py_TYPE(v);

    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        *result = NULL;
        return -1;
    }

    if (tp->tp_getattro == PyObject_GenericGetAttr) {
        *result = _PyObject_GenericGetAttrWithDict(v, name, NULL, 1);
        if (*result != NULL) {
            return 1;
        }
        if (PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }
    if (tp->tp_getattro == _Py_type_getattro) {
        int suppress_missing_attribute_exception = 0;
        *result = _Py_type_getattro_impl((PyTypeObject*)v, name, &suppress_missing_attribute_exception);
        if (suppress_missing_attribute_exception) {
            // return 0 without having to clear the exception
            return 0;
        }
    }
    else if (tp->tp_getattro == (getattrofunc)_Py_module_getattro) {
        // optimization: suppress attribute error from module getattro method
        *result = _Py_module_getattro_impl((PyModuleObject*)v, name, 1);
        if (*result != NULL) {
            return 1;
        }
        if (PyErr_Occurred()) {
            return -1;
        }
        return 0;
    }
    else if (tp->tp_getattro != NULL) {
        *result = (*tp->tp_getattro)(v, name);
    }
    else if (tp->tp_getattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            *result = NULL;
            return -1;
        }
        *result = (*tp->tp_getattr)(v, (char *)name_str);
    }
    else {
        *result = NULL;
        return 0;
    }

    if (*result != NULL) {
        return 1;
    }
    if (!PyErr_ExceptionMatches(PyExc_AttributeError)) {
        return -1;
    }
    PyErr_Clear();
    return 0;
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

int
PyObject_HasAttrWithError(PyObject *obj, PyObject *name)
{
    PyObject *res;
    int rc = PyObject_GetOptionalAttr(obj, name, &res);
    Py_XDECREF(res);
    return rc;
}

int
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

static int
object_setattr(PyObject *v, PyObject *name, PyObject *value)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (value == NULL && _PyErr_Occurred(tstate)) {
        PyObject *exc = _PyErr_GetRaisedException(tstate);
        _PyErr_SetString(tstate, PyExc_SystemError,
            "PyObject_SetAttr() must not be called with NULL value "
            "and an exception set");
        _PyErr_ChainExceptions1Tstate(tstate, exc);
        return -1;
    }

    PyTypeObject *tp = Py_TYPE(v);
    int err;

    if (!PyUnicode_Check(name)) {
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return -1;
    }
    Py_INCREF(name);

    _PyUnicode_InternMortal(tstate->interp, &name);
    if (tp->tp_setattro != NULL) {
        err = (*tp->tp_setattro)(v, name, value);
        Py_DECREF(name);
        return err;
    }
    if (tp->tp_setattr != NULL) {
        const char *name_str = PyUnicode_AsUTF8(name);
        if (name_str == NULL) {
            Py_DECREF(name);
            return -1;
        }
        err = (*tp->tp_setattr)(v, (char *)name_str, value);
        Py_DECREF(name);
        return err;
    }
    Py_DECREF(name);
    _PyObject_ASSERT(name, Py_REFCNT(name) >= 1);
    if (tp->tp_getattr == NULL && tp->tp_getattro == NULL)
        PyErr_Format(PyExc_TypeError,
                     "'%.100s' object has no attributes "
                     "(%s .%U)",
                     tp->tp_name,
                     value==NULL ? "del" : "assign to",
                     name);
    else
        PyErr_Format(PyExc_TypeError,
                     "'%.100s' object has only read-only attributes "
                     "(%s .%U)",
                     tp->tp_name,
                     value==NULL ? "del" : "assign to",
                     name);
    return -1;
}

int
PyObject_DelAttr(PyObject *v, PyObject *name)
{
    return PyObject_SetAttr(v, name, NULL);
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

        Py_ssize_t tsize = Py_SIZE(obj);
        if (tsize < 0) {
            tsize = -tsize;
        }
        size_t size = _PyObject_VAR_SIZE(tp, tsize);
        assert(size <= (size_t)PY_SSIZE_T_MAX);
        dictoffset += (Py_ssize_t)size;

        _PyObject_ASSERT(obj, dictoffset > 0);
        _PyObject_ASSERT(obj, dictoffset % SIZEOF_VOID_P == 0);
    }
    return (PyObject **) ((char *)obj + dictoffset);
}

/* Helper to get a pointer to an object's __dict__ slot, if any.
 * Creates the dict from inline attributes if necessary.
 * Does not set an exception.
 *
 * Note that the tp_dictoffset docs used to recommend this function,
 * so it should be treated as part of the public API.
 */
PyObject **
_PyObject_GetDictPtr(PyObject *obj)
{
    if ((Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return _PyObject_ComputedDictPointer(obj);
    }
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL && Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        dict = _PyObject_MaterializeManagedDict(obj);
        if (dict == NULL) {
            PyErr_Clear();
            return NULL;
        }
    }
    return (PyObject **)&_PyObject_ManagedDictPointer(obj)->dict;
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

PyObject *
_PyObject_NextNotImplemented(PyObject *self)
{
    PyErr_Format(PyExc_TypeError,
                 "'%.200s' object is not iterable",
                 Py_TYPE(self)->tp_name);
    return NULL;
}


/* Specialized version of _PyObject_GenericGetAttrWithDict
   specifically for the LOAD_METHOD opcode.

   Return 1 if a method is found, 0 if it's a regular attribute
   from __dict__ or something returned by using a descriptor
   protocol.

   `method` will point to the resolved attribute or NULL.  In the
   latter case, an error will be set.
*/
int
_PyObject_GetMethod(PyObject *obj, PyObject *name, PyObject **method)
{
    int meth_found = 0;

    assert(*method == NULL);

    PyTypeObject *tp = Py_TYPE(obj);
    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0) {
            return 0;
        }
    }

    if (tp->tp_getattro != PyObject_GenericGetAttr || !PyUnicode_CheckExact(name)) {
        *method = PyObject_GetAttr(obj, name);
        return 0;
    }

    PyObject *descr = _PyType_LookupRef(tp, name);
    descrgetfunc f = NULL;
    if (descr != NULL) {
        if (_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR)) {
            meth_found = 1;
        }
        else {
            f = Py_TYPE(descr)->tp_descr_get;
            if (f != NULL && PyDescr_IsData(descr)) {
                *method = f(descr, obj, (PyObject *)Py_TYPE(obj));
                Py_DECREF(descr);
                return 0;
            }
        }
    }
    PyObject *dict, *attr;
    if ((tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) &&
         _PyObject_TryGetInstanceAttribute(obj, name, &attr)) {
        if (attr != NULL) {
            *method = attr;
            Py_XDECREF(descr);
            return 0;
        }
        dict = NULL;
    }
    else if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
        dict = (PyObject *)_PyObject_GetManagedDict(obj);
    }
    else {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        if (dictptr != NULL) {
            dict = FT_ATOMIC_LOAD_PTR_ACQUIRE(*dictptr);
        }
        else {
            dict = NULL;
        }
    }
    if (dict != NULL) {
        Py_INCREF(dict);
        if (PyDict_GetItemRef(dict, name, method) != 0) {
            // found or error
            Py_DECREF(dict);
            Py_XDECREF(descr);
            return 0;
        }
        // not found
        Py_DECREF(dict);
    }

    if (meth_found) {
        *method = descr;
        return 1;
    }

    if (f != NULL) {
        *method = f(descr, obj, (PyObject *)Py_TYPE(obj));
        Py_DECREF(descr);
        return 0;
    }

    if (descr != NULL) {
        *method = descr;
        return 0;
    }

    PyErr_Format(PyExc_AttributeError,
                 "'%.100s' object has no attribute '%U'",
                 tp->tp_name, name);

    _PyObject_SetAttributeErrorContext(obj, name);
    return 0;
}

// Look up a method on `self` by `name`.
//
// On success, `*method` is set and the function returns 0 or 1. If the
// return value is 1, the call is an unbound method and `*self` is the
// "self" or "cls" argument to pass. If the return value is 0, the call is
// a regular function and `*self` is cleared.
//
// On error, returns -1, clears `*self`, and sets an exception.
int
_PyObject_GetMethodValue(PyThreadState *ts, PyValue *self,
                            PyObject *name, PyValue *method)
{
    int meth_found = 0;
    // A box of an inline self: this frame, which the collector scans,
    // holds it, and *self keeps the inline value.
    PyObject *obj = PyValue_Object(*self);
    if (obj == NULL) {
        *self = PyValue_NULL;
        return -1;
    }

    assert(PyValue_IsNull(*method));

    PyTypeObject *tp = Py_TYPE(obj);
    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0) {
            *self = PyValue_NULL;
            return -1;
        }
    }

    if (tp->tp_getattro != PyObject_GenericGetAttr || !PyUnicode_CheckExact(name)) {
        PyObject *res = PyObject_GetAttr(obj, name);
        *self = PyValue_NULL;
        if (res != NULL) {
            *method = PyValue_FromObject(res);
            return 0;
        }
        return -1;
    }

    _PyType_LookupValueAndVersion(tp, name, method);
    // An inline value is neither a method nor a descriptor: like any
    // other class attribute that is not, it is the result unless the
    // instance has the name.
    int in_type = !PyValue_IsNull(*method);
    PyObject *descr = PyValue_IsInline(*method) ? NULL
                                                : PyValue_ObjectBorrow(*method);
    descrgetfunc f = NULL;
    if (descr != NULL) {
        if (_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR)) {
            meth_found = 1;
        }
        else {
            f = Py_TYPE(descr)->tp_descr_get;
            if (f != NULL && PyDescr_IsData(descr)) {
                PyObject *value = f(descr, obj, (PyObject *)Py_TYPE(obj));
                *method = PyValue_NULL;
                *self = PyValue_NULL;
                if (value != NULL) {
                    *method = PyValue_FromObject(value);
                    return 0;
                }
                return -1;
            }
        }
    }
    PyObject *dict;
    PyValue attr;
    if ((tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) &&
         _PyObject_TryGetInstanceAttributeValue(obj, name, &attr)) {
        if (!PyValue_IsNull(attr)) {
            *method = attr;
            *self = PyValue_NULL;
            return 0;
        }
        dict = NULL;
    }
    else if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
        dict = (PyObject *)_PyObject_GetManagedDict(obj);
    }
    else {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        if (dictptr != NULL) {
            dict = FT_ATOMIC_LOAD_PTR_ACQUIRE(*dictptr);
        }
        else {
            dict = NULL;
        }
    }
    if (dict != NULL) {
        assert(PyUnicode_CheckExact(name));
        int found = _PyDict_GetMethodValue((PyDictObject *)dict, name, method);
        if (found < 0) {
            assert(PyValue_IsNull(*method));
            *self = PyValue_NULL;
            return -1;
        }
        else if (found) {
            *self = PyValue_NULL;
            return 0;
        }
    }

    if (meth_found) {
        assert(!PyValue_IsNull(*method));
        return 1;
    }

    if (f != NULL) {
        if (Py_IS_TYPE(descr, &PyClassMethod_Type)) {
            PyObject *callable = _PyClassMethod_GetFunc(descr);
            *method = PyValue_FromObject(callable);
            *self = PyValue_FromObject((PyObject *)tp);
            return 1;
        }
        else if (Py_IS_TYPE(descr, &PyStaticMethod_Type)) {
            PyObject *callable = _PyStaticMethod_GetFunc(descr);
            *method = PyValue_FromObject(callable);
            *self = PyValue_NULL;
            return 0;
        }
        PyObject *value = f(descr, obj, (PyObject *)tp);
        *method = PyValue_NULL;
        *self = PyValue_NULL;
        if (value) {
            *method = PyValue_FromObject(value);
            return 0;
        }
        return -1;
    }

    if (in_type) {
        assert(!PyValue_IsNull(*method));
        *self = PyValue_NULL;
        return 0;
    }

    PyErr_Format(PyExc_AttributeError,
                 "'%.100s' object has no attribute '%U'",
                 tp->tp_name, name);

    _PyObject_SetAttributeErrorContext(obj, name);
    assert(PyValue_IsNull(*method));
    *self = PyValue_NULL;
    return -1;
}


/* Generic GetAttr functions - put these in your tp_[gs]etattro slot. */

/* The result of a descriptor's __get__ for obj as a value, or PyValue_NULL
   with an exception set, or with none if suppress is 1 and the exception
   was an AttributeError. */
static PyValue
descr_get_value(descrgetfunc f, PyObject *descr, PyObject *obj, int suppress)
{
    PyObject *res = f(descr, obj, (PyObject *)Py_TYPE(obj));
    if (res == NULL) {
        if (suppress && PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
        }
        return PyValue_NULL;
    }
    return PyValue_FromObject(res);
}

/* _PyObject_GenericGetAttrWithDict() with the attribute as its value: an
   attribute held as an inline value in the instance, its dict or the class
   is returned with no box.  PyValue_NULL with an exception set on failure,
   or with none if suppress is 1 and the attribute is missing. */
static PyValue
generic_getattr_value(PyObject *obj, PyObject *name, PyObject *dict,
                      int suppress)
{
    /* Make sure the logic of _PyObject_GetMethod is in sync with
       this method.

       When suppress=1, this function suppresses AttributeError.
    */

    PyTypeObject *tp = Py_TYPE(obj);
    PyObject *descr = NULL;
    PyValue res = PyValue_NULL;
    descrgetfunc f;

    if (!PyUnicode_Check(name)){
        PyErr_Format(PyExc_TypeError,
                     "attribute name must be string, not '%.200s'",
                     Py_TYPE(name)->tp_name);
        return PyValue_NULL;
    }

    if (!_PyType_IsReady(tp)) {
        if (PyType_Ready(tp) < 0)
            return PyValue_NULL;
    }

    Py_INCREF(name);

    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot cref;
    _PyThreadState_PushValueRoot(tstate, &cref);

    _PyType_LookupValueAndVersion(tp, name, &cref.value);
    // An inline value in the class is not a descriptor: like any other
    // class attribute that is not, it is the result unless the instance
    // has the name.
    descr = PyValue_IsObject(cref.value) ? PyValue_ObjectBorrow(cref.value)
                                         : NULL;

    f = NULL;
    if (descr != NULL) {
        f = Py_TYPE(descr)->tp_descr_get;
        if (f != NULL && PyDescr_IsData(descr)) {
            // gh-157840: We special-case member descriptors here to avoid
            // allocating an extra AttributeError
            if (suppress && Py_IS_TYPE(descr, &PyMemberDescr_Type)) {
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
            res = descr_get_value(f, descr, obj, suppress);
            goto done;
        }
    }
    if (dict == NULL) {
        if ((tp->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
            if (PyUnicode_CheckExact(name) &&
                _PyObject_TryGetInstanceAttributeValue(obj, name, &res)) {
                if (!PyValue_IsNull(res)) {
                    goto done;
                }
            }
            else {
                dict = (PyObject *)_PyObject_MaterializeManagedDict(obj);
                if (dict == NULL) {
                    res = PyValue_NULL;
                    goto done;
                }
            }
        }
        else if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
            dict = (PyObject *)_PyObject_GetManagedDict(obj);
        }
        else {
            PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
            if (dictptr) {
                dict = *dictptr;
            }
        }
    }
    if (dict != NULL) {
        int rc = _PyDict_GetItemValue(dict, PyValue_FromObject(name), &res);
        if (!PyValue_IsNull(res)) {
            goto done;
        }
        else if (rc < 0) {
            if (suppress && PyErr_ExceptionMatches(PyExc_AttributeError)) {
                PyErr_Clear();
            }
            else {
                goto done;
            }
        }
    }

    if (f != NULL) {
        res = descr_get_value(f, descr, obj, suppress);
        goto done;
    }

    if (!PyValue_IsNull(cref.value)) {
        res = cref.value;
        goto done;
    }

    if (!suppress) {
        PyErr_Format(PyExc_AttributeError,
                     "'%.100s' object has no attribute '%U'",
                     tp->tp_name, name);

        _PyObject_SetAttributeErrorContext(obj, name);
    }
  done:
    _PyThreadState_PopValueRoot(tstate, &cref);
    Py_DECREF(name);
    return res;
}

PyObject *
_PyObject_GenericGetAttrWithDict(PyObject *obj, PyObject *name,
                                 PyObject *dict, int suppress)
{
    return PyValue_Object(generic_getattr_value(obj, name, dict, suppress));
}

PyObject *
PyObject_GenericGetAttr(PyObject *obj, PyObject *name)
{
    return _PyObject_GenericGetAttrWithDict(obj, name, NULL, 0);
}

/* The generic setattr of value, or its deletion if value is PyValue_NULL,
   on obj.  An inline value is stored without a box unless a descriptor
   takes it. */
static int
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

    if (!_PyType_IsReady(tp) && PyType_Ready(tp) < 0) {
        return -1;
    }

    Py_INCREF(name);
    _Py_INCREF_TYPE(tp);

    PyThreadState *tstate = _PyThreadState_GET();
    _PyValueRoot cref;
    _PyThreadState_PushValueRoot(tstate, &cref);

    _PyType_LookupValueAndVersion(tp, name, &cref.value);
    descr = _PyValueRoot_Object(&cref);
    if (descr == NULL && PyErr_Occurred()) {
        goto done;
    }

    if (descr != NULL) {
        f = Py_TYPE(descr)->tp_descr_set;
        if (f != NULL) {
            _PyValueRoot vref;
            PyObject *value_o = _PyValueRoot_PushObject(tstate, &vref, value,
                                                        _PY_BOX_ATTRIBUTE_STORE);
            if (value_o != NULL || PyValue_IsNull(value)) {
                res = f(descr, obj, value_o);
            }
            _PyThreadState_PopValueRoot(tstate, &vref);
            goto done;
        }
    }

    if (dict == NULL) {
        PyObject **dictptr;

        if ((tp->tp_flags & Py_TPFLAGS_INLINE_VALUES)) {
            res = _PyObject_StoreInstanceAttribute(obj, name, value);
            goto error_check;
        }

        if ((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT)) {
            PyManagedDictPointer *managed_dict = _PyObject_ManagedDictPointer(obj);
            dictptr = (PyObject **)&managed_dict->dict;
        }
        else {
            dictptr = _PyObject_ComputedDictPointer(obj);
        }
        if (dictptr == NULL) {
            if (descr == NULL) {
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
                _PyObject_SetAttributeErrorContext(obj, name);
            }
            else {
                PyErr_Format(PyExc_AttributeError,
                            "'%.100s' object attribute '%U' is read-only",
                            tp->tp_name, name);
            }
            goto done;
        }
        else {
            res = _PyObjectDict_SetItem(tp, obj, dictptr, name, value);
        }
    }
    else {
        Py_BEGIN_CRITICAL_SECTION(dict);
        res = _PyDict_SetItem_LockHeld((PyDictObject *)dict, name, value);
        Py_END_CRITICAL_SECTION();
    }
  error_check:
    if (res < 0 && PyErr_ExceptionMatches(PyExc_KeyError)) {
        PyErr_Format(PyExc_AttributeError,
                        "'%.100s' object has no attribute '%U'",
                        tp->tp_name, name);
        _PyObject_SetAttributeErrorContext(obj, name);
    }
  done:
    _PyThreadState_PopValueRoot(tstate, &cref);
    _Py_DECREF_TYPE(tp);
    Py_DECREF(name);
    return res;
}

int
_PyObject_GenericSetAttrWithDict(PyObject *obj, PyObject *name,
                                 PyObject *value, PyObject *dict)
{
    return generic_setattr_value(
        obj, name, value == NULL ? PyValue_NULL : PyValue_FromObject(value),
        dict);
}

int
PyObject_GenericSetAttr(PyObject *obj, PyObject *name, PyObject *value)
{
    return _PyObject_GenericSetAttrWithDict(obj, name, value, NULL);
}

int
PyObject_GenericSetDict(PyObject *obj, PyObject *value, void *context)
{
    if (value == NULL) {
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
    // and its type, if the caller set it first, is kept.  The object map
    // tells the collector's heap walk that the block holds an object.
    op->ob_header &= _Py_HEADER_TYPE_MASK;
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
        PyTypeObject *type = Py_TYPE(obj);
        const size_t presize = _PyType_PreHeaderSize(type);
        void *ptr = (void *)((char *)obj - presize);
        _PyMem_DumpTraceback(fileno(stderr), ptr);

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

PyObject *
PyObject_GetAttr(PyObject *v, PyObject *name)
{
    return PyValue_Object(PyObject_GetAttrValue(PyValue_FromObject(v), name));
}

int
PyObject_SetAttr(PyObject *v, PyObject *name, PyObject *value)
{
    if (value == NULL) {
        // Deletion, which also reports a NULL value with an exception set.
        return object_setattr(v, name, NULL);
    }
    return PyObject_SetAttrValue(PyValue_FromObject(v), name,
                                 PyValue_FromObject(value));
}
