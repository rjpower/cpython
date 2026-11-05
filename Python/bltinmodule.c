/* Built-in functions */

#include "Python.h"
#include "pycore_ast.h"           // _PyAST_Validate()
#include "pycore_audit.h"         // _PySys_AuditEnabled()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_ceval.h"         // _PyEval_Vector()
#include "pycore_long_value.h"    // _PyValue_AsLongAndOverflow()
#include "pycore_compile.h"       // _PyAST_Compile()
#include "pycore_dict.h"          // PyDict_SetValue()
#include "pycore_fileutils.h"     // _PyFile_Flush
#include "pycore_floatobject.h"   // _PyFloat_ExactDealloc()
#include "pycore_genobject.h"     // _PyGen_SetStopIterationValue()
#include "pycore_interp.h"        // _PyInterpreterState_GetConfig()
#include "pycore_import.h"        // _PyImport_LazyImportModuleLevelObject  ()
#include "pycore_iterobject.h"    // _PyCallIter_NewEx()
#include "pycore_list.h"          // PyList_AppendValue()
#include "pycore_long.h"          // _PyLong_CompactValue
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_object.h"        // _Py_AddToAllObjects()
#include "pycore_pyerrors.h"      // _PyErr_NoMemory()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_pythonrun.h"     // _Py_SourceAsString()
#include "pycore_value.h"      // _PyObject_ValueIdentity()
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
#include "pycore_value_ops.h"     // _PyValue_IterStep()
#include "pycore_value_slots.h"   // _Py_VALUE_THUNK_ITERNEXT()
#include "pycore_tuple.h"         // _PyTuple_Recycle()
#include "pycore_value_api.h"     // _PyValue_FromObjectKnown()

#include "clinic/bltinmodule.c.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>             // isatty()
#endif


static PyObject*
update_bases(PyObject *bases, PyObject *const *args, Py_ssize_t nargs)
{
    Py_ssize_t i, j;
    PyObject *base, *meth, *new_base, *result, *new_bases = NULL;
    assert(PyTuple_Check(bases));

    for (i = 0; i < nargs; i++) {
        base  = args[i];
        if (PyType_Check(base)) {
            if (new_bases) {
                /* If we already have made a replacement, then we append every normal base,
                   otherwise just skip it. */
                if (PyList_AppendValue(new_bases, PyValue_FromObjectBorrow(base)) < 0) {
                    goto error;
                }
            }
            continue;
        }
        if (PyObject_GetOptionalAttr(base, &_Py_ID(__mro_entries__), &meth) < 0) {
            goto error;
        }
        if (!meth) {
            if (new_bases) {
                if (PyList_AppendValue(new_bases, PyValue_FromObjectBorrow(base)) < 0) {
                    goto error;
                }
            }
            continue;
        }
        new_base = PyObject_CallOneArg(meth, bases);
        Py_DECREF(meth);
        if (!new_base) {
            goto error;
        }
        if (!PyTuple_Check(new_base)) {
            PyErr_SetString(PyExc_TypeError,
                            "__mro_entries__ must return a tuple");
            Py_DECREF(new_base);
            goto error;
        }
        if (!new_bases) {
            /* If this is a first successful replacement, create new_bases list and
               copy previously encountered bases. */
            if (!(new_bases = PyList_New(i))) {
                Py_DECREF(new_base);
                goto error;
            }
            for (j = 0; j < i; j++) {
                base = args[j];
                PyList_SET_VALUE(new_bases, j, PyValue_DUP(PyValue_FromObjectBorrow(base)));
            }
        }
        j = PyList_GET_SIZE(new_bases);
        if (PyList_SetSlice(new_bases, j, j, new_base) < 0) {
            Py_DECREF(new_base);
            goto error;
        }
        Py_DECREF(new_base);
    }
    if (!new_bases) {
        return bases;
    }
    result = PyList_AsTuple(new_bases);
    Py_DECREF(new_bases);
    return result;

error:
    Py_XDECREF(new_bases);
    return NULL;
}

/* AC: cannot convert yet, waiting for *args support */
static PyObject *
builtin___build_class__(PyObject *self, PyObject *const *args, Py_ssize_t nargs,
                        PyObject *kwnames)
{
    PyObject *func, *name, *winner, *prep;
    PyObject *cls = NULL, *cell = NULL, *ns = NULL, *meta = NULL, *orig_bases = NULL;
    PyObject *mkw = NULL, *bases = NULL;
    int isclass = 0;   /* initialize to prevent gcc warning */

    if (nargs < 2) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: not enough arguments");
        return NULL;
    }
    func = args[0];   /* Better be callable */
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: func must be a function");
        return NULL;
    }
    name = args[1];
    if (!PyUnicode_Check(name)) {
        PyErr_SetString(PyExc_TypeError,
                        "__build_class__: name is not a string");
        return NULL;
    }
    orig_bases = PyTuple_FromArray(args + 2, nargs - 2);
    if (orig_bases == NULL)
        return NULL;

    bases = update_bases(orig_bases, args + 2, nargs - 2);
    if (bases == NULL) {
        Py_DECREF(orig_bases);
        return NULL;
    }

    if (kwnames == NULL) {
        meta = NULL;
        mkw = NULL;
    }
    else {
        mkw = _PyStack_AsDict(args + nargs, kwnames);
        if (mkw == NULL) {
            goto error;
        }

        PyValue meta_v;
        int pop_rc = PyDict_PopValue(mkw, _PyValue_FromObjectKnown(&_Py_ID(metaclass)),
                                     &meta_v);
        if (pop_rc < 0) {
            goto error;
        }
        meta = NULL;
        if (pop_rc > 0) {
            meta = _PyValue_AsObjectAt(meta_v, BOX_SITE_DICT);
            PyValue_CLOSE(meta_v);
            if (meta == NULL) {
                goto error;
            }
        }
        if (meta != NULL) {
            /* metaclass is explicitly given, check if it's indeed a class */
            isclass = PyType_Check(meta);
        }
    }
    if (meta == NULL) {
        /* if there are no bases, use type: */
        if (PyTuple_GET_SIZE(bases) == 0) {
            meta = Py_NewRef((PyObject *) (&PyType_Type));
        }
        /* else get the type of the first base */
        else {
            PyValue base0_v = PyTuple_GetValue(bases, 0);
            PyObject *base0 = PyValue_IsNull(base0_v) ? NULL
                : _PyValue_AsObjectAt(base0_v, BOX_SITE_TUPLE);
            PyValue_XCLOSE(base0_v);
            if (base0 == NULL) {
                goto error;
            }
            meta = Py_NewRef((PyObject *)Py_TYPE(base0));
            Py_DECREF(base0);
        }
        isclass = 1;  /* meta is really a class */
    }

    if (isclass) {
        /* meta is really a class, so check for a more derived
           metaclass, or possible metaclass conflicts: */
        winner = (PyObject *)_PyType_CalculateMetaclass((PyTypeObject *)meta,
                                                        bases);
        if (winner == NULL) {
            goto error;
        }
        if (winner != meta) {
            Py_SETREF(meta, Py_NewRef(winner));
        }
    }
    /* else: meta is not a class, so we cannot do the metaclass
       calculation, so we will use the explicitly given object as it is */
    if (PyObject_GetOptionalAttr(meta, &_Py_ID(__prepare__), &prep) < 0) {
        ns = NULL;
    }
    else if (prep == NULL) {
        ns = PyDict_New();
    }
    else {
        PyObject *pargs[2] = {name, bases};
        ns = PyObject_VectorcallDict(prep, pargs, 2, mkw);
        Py_DECREF(prep);
    }
    if (ns == NULL) {
        goto error;
    }
    if (!PyMapping_Check(ns)) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s.__prepare__() must return a mapping, not %.200s",
                     isclass ? ((PyTypeObject *)meta)->tp_name : "<metaclass>",
                     Py_TYPE(ns)->tp_name);
        goto error;
    }
    PyThreadState *tstate = _PyThreadState_GET();
    EVAL_CALL_STAT_INC(EVAL_CALL_BUILD_CLASS);
    cell = _PyEval_Vector(tstate, (PyFunctionObject *)func, ns, NULL, 0, NULL);
    if (cell != NULL) {
        if (bases != orig_bases) {
            if (PyMapping_SetItemString(ns, "__orig_bases__", orig_bases) < 0) {
                goto error;
            }
        }
        PyObject *margs[3] = {name, bases, ns};
        cls = PyObject_VectorcallDict(meta, margs, 3, mkw);
        if (cls != NULL && PyType_Check(cls) && PyCell_Check(cell)) {
            PyValue cell_cls_v = PyCell_GetValue(cell);
            PyObject *cell_cls = PyValue_IsNull(cell_cls_v) ? NULL
                : _PyValue_AsObjectAt(cell_cls_v, BOX_SITE_CELL);
            PyValue_XCLOSE(cell_cls_v);
            if (cell_cls == NULL && PyErr_Occurred()) {
                Py_CLEAR(cls);
                goto error;
            }
            if (cell_cls != cls) {
                if (cell_cls == NULL) {
                    const char *msg =
                        "__class__ not set defining %.200R as %.200R. "
                        "Was __classcell__ propagated to type.__new__?";
                    PyErr_Format(PyExc_RuntimeError, msg, name, cls);
                } else {
                    const char *msg =
                        "__class__ set to %.200R defining %.200R as %.200R";
                    PyErr_Format(PyExc_TypeError, msg, cell_cls, name, cls);
                }
                Py_XDECREF(cell_cls);
                Py_SETREF(cls, NULL);
                goto error;
            }
            else {
                Py_DECREF(cell_cls);
            }
        }
    }
error:
    Py_XDECREF(cell);
    Py_XDECREF(ns);
    Py_XDECREF(meta);
    Py_XDECREF(mkw);
    if (bases != orig_bases) {
        Py_DECREF(orig_bases);
    }
    Py_DECREF(bases);
    return cls;
}

PyDoc_STRVAR(build_class_doc,
"__build_class__(func, name, /, *bases, [metaclass], **kwds) -> class\n\
\n\
Internal helper function used by the class statement.");

/* Box an optional argument for a callee that takes a PyObject *.
   PyValue_NULL gives NULL in *out. Returns -1 with an exception set if the
   box fails; otherwise *out is NULL or a new reference. */
static int
box_optional(PyValueBorrow v, PyObject **out)
{
    if (PyValue_IsNull(v)) {
        *out = NULL;
        return 0;
    }
    *out = _PyValue_AsObjectAt(v, BOX_SITE_PARSER);
    return *out == NULL ? -1 : 0;
}

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module builtins
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ab21862ed6de3306]*/

/*[clinic input]
__import__ as builtin___import__

    name: object
    globals: object(c_default="NULL") = None
    locals: object(c_default="NULL") = None
    fromlist: object(c_default="NULL") = ()
    level: int = 0

Import a module.

Because this function is meant for use by the Python
interpreter and not for general use, it is better to use
importlib.import_module() to programmatically import a module.

The globals argument is only used to determine the context;
they are not modified.  The locals argument is unused.  The fromlist
should be a list of names to emulate ``from name import ...``, or an
empty list to emulate ``import name``.
When importing a module from a package, note that __import__('A.B', ...)
returns package A when fromlist is empty, but its submodule B when
fromlist is not empty.  The level argument is used to determine whether
to perform absolute or relative imports: 0 is absolute, while a positive
number is the number of parent directories to search relative to the
current module.
[clinic start generated code]*/

static PyValue
builtin___import___impl(PyObject *module, PyValueBorrow name,
                        PyValueBorrow globals, PyValueBorrow locals,
                        PyValueBorrow fromlist, int level)
/*[clinic end generated code: output=72aedbf4f72eea7e input=e3096a230383f72d]*/
{
    PyObject *name_o = NULL, *globals_o = NULL, *locals_o = NULL;
    PyObject *fromlist_o = NULL, *res = NULL;
    name_o = _PyValue_AsObjectAt(name, BOX_SITE_PARSER);
    if (name_o != NULL
        && box_optional(globals, &globals_o) == 0
        && box_optional(locals, &locals_o) == 0
        && box_optional(fromlist, &fromlist_o) == 0)
    {
        res = PyImport_ImportModuleLevelObject(name_o, globals_o, locals_o,
                                               fromlist_o, level);
    }
    Py_XDECREF(name_o);
    Py_XDECREF(globals_o);
    Py_XDECREF(locals_o);
    Py_XDECREF(fromlist_o);
    return _PyValue_FromObjectResult(res);
}


/*[clinic input]
__lazy_import__ as builtin___lazy_import__

    name: object
    globals: object(c_default="NULL") = None
    locals: object(c_default="NULL") = None
    fromlist: object(c_default="NULL") = ()
    level: int = 0

Lazily imports a module.

Returns either the module to be imported or a imp.lazy_module object
which indicates the module to be lazily imported.
[clinic start generated code]*/

static PyValue
builtin___lazy_import___impl(PyObject *module, PyValueBorrow name,
                             PyValueBorrow globals, PyValueBorrow locals,
                             PyValueBorrow fromlist, int level)
/*[clinic end generated code: output=aa9e618013736a27 input=9c85cccd6a885b9b]*/
{
    PyObject *builtins;
    PyThreadState *tstate = PyThreadState_GET();
    if (PyValue_IsNull(globals)) {
        globals = PyValue_FromObjectBorrow(PyEval_GetGlobals());
        if (PyValue_IsNull(globals)) {
            PyErr_SetString(PyExc_TypeError,
                            "__lazy_import__() missing globals "
                            "when called without a frame");
            return PyValue_NULL;
        }
    }
    if (PyValue_IsNull(locals)) {
        locals = globals;
    }

    if (!(PyValue_IsObject(globals) && PyDict_Check(PyValue_AsKnownObjectBorrow(globals)))) {
        PyErr_Format(PyExc_TypeError,
                     "expect dict for globals, got %vT", globals);
        return PyValue_NULL;
    }
    PyObject *globals_o = PyValue_AsKnownObjectBorrow(globals);

    PyValue builtins_v;
    int builtins_rc = PyDict_GetValue(globals_o,
                                      _PyValue_FromObjectKnown(&_Py_ID(__builtins__)),
                                      &builtins_v);
    if (builtins_rc < 0) {
        return PyValue_NULL;
    }
    builtins = NULL;
    if (builtins_rc > 0) {
        builtins = _PyValue_AsObjectAt(builtins_v, BOX_SITE_DICT);
        PyValue_CLOSE(builtins_v);
        if (builtins == NULL) {
            return PyValue_NULL;
        }
    }
    if (builtins == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "unable to get builtins for lazy import");
        return PyValue_NULL;
    }
    if (PyModule_Check(builtins)) {
        PyObject *builtins_dict = Py_XNewRef(PyModule_GetDict(builtins));
        if (builtins_dict == NULL) {
            Py_DECREF(builtins);
            PyErr_SetString(PyExc_AttributeError,
                            "builtins module has no dict");
            return PyValue_NULL;
        }
        Py_SETREF(builtins, builtins_dict);
    }

    PyObject *name_o = NULL, *locals_o = NULL, *fromlist_o = NULL;
    PyObject *res = NULL;
    name_o = _PyValue_AsObjectAt(name, BOX_SITE_PARSER);
    if (name_o != NULL
        && box_optional(locals, &locals_o) == 0
        && box_optional(fromlist, &fromlist_o) == 0)
    {
        res = _PyImport_LazyImportModuleLevelObject(
            tstate, name_o, builtins, globals_o, locals_o, fromlist_o, level);
    }
    Py_XDECREF(name_o);
    Py_XDECREF(locals_o);
    Py_XDECREF(fromlist_o);
    Py_DECREF(builtins);
    return _PyValue_FromObjectResult(res);
}

/*[clinic input]
abs as builtin_abs

    number: object
    /

Return the absolute value of the argument.
[clinic start generated code]*/

static PyValue
builtin_abs_impl(PyObject *module, PyValueBorrow number)
/*[clinic end generated code: output=8fd99d9f045359ce input=a356196903543505]*/
{
    return _PyValue_Absolute(number);
}

/*[clinic input]
all as builtin_all

    iterable: object
    /

Return True if bool(x) is True for all values x in the iterable.

If the iterable is empty, return True.
[clinic start generated code]*/

static PyValue
builtin_all_impl(PyObject *module, PyValueBorrow iterable)
/*[clinic end generated code: output=7e7b128f0506e750 input=1a7c5d1bc3438a21]*/
{
    PyObject *it;
    _PyStatusValue item;
    int cmp;

    it = _PyValue_GetIter(iterable);
    if (it == NULL)
        return PyValue_NULL;

    for (;;) {
        item = _PyValue_IterNext(it);
        if (item.status != PYGEN_NEXT)
            break;
        cmp = _PyValue_IsTrue(item.value);
        PyValue_CLOSE(item.value);
        if (cmp < 0) {
            Py_DECREF(it);
            return PyValue_NULL;
        }
        if (cmp == 0) {
            Py_DECREF(it);
            return PyValue_False;
        }
    }
    Py_DECREF(it);
    // _PyValue_IterNext() has cleared a StopIteration that ended it
    if (item.status == PYGEN_ERROR)
        return PyValue_NULL;
    return PyValue_True;
}

/*[clinic input]
any as builtin_any

    iterable: object
    /

Return True if bool(x) is True for any x in the iterable.

If the iterable is empty, return False.
[clinic start generated code]*/

static PyValue
builtin_any_impl(PyObject *module, PyValueBorrow iterable)
/*[clinic end generated code: output=e9f411ffca5ee15c input=41d7451c23384f24]*/
{
    PyObject *it;
    _PyStatusValue item;
    int cmp;

    it = _PyValue_GetIter(iterable);
    if (it == NULL)
        return PyValue_NULL;

    for (;;) {
        item = _PyValue_IterNext(it);
        if (item.status != PYGEN_NEXT)
            break;
        cmp = _PyValue_IsTrue(item.value);
        PyValue_CLOSE(item.value);
        if (cmp < 0) {
            Py_DECREF(it);
            return PyValue_NULL;
        }
        if (cmp > 0) {
            Py_DECREF(it);
            return PyValue_True;
        }
    }
    Py_DECREF(it);
    // _PyValue_IterNext() has cleared a StopIteration that ended it
    if (item.status == PYGEN_ERROR)
        return PyValue_NULL;
    return PyValue_False;
}

/*[clinic input]
ascii as builtin_ascii

    obj: object
    /

Return an ASCII-only representation of an object.

As repr(), return a string containing a printable representation of an
object, but escape the non-ASCII characters in the string returned by
repr() using \\x, \\u or \\U escapes. This generates a string similar
to that returned by repr() in Python 2.
[clinic start generated code]*/

static PyValue
builtin_ascii_impl(PyObject *module, PyValueBorrow obj)
/*[clinic end generated code: output=c0e87d35cd6adc6c input=4c62732e1b3a3cc9]*/
{
    return _PyValue_FromObjectResult(_PyValue_ASCII(obj));
}


/*[clinic input]
bin as builtin_bin

    integer: object
    /

Return the binary representation of an integer.

   >>> bin(2796202)
   '0b1010101010101010101010'
[clinic start generated code]*/

static PyValue
builtin_bin_impl(PyObject *module, PyValueBorrow integer)
/*[clinic end generated code: output=2126be9da198846e input=d16518f148341e70]*/
{
    PyObject *integer_o = _PyValue_AsObjectAt(integer, BOX_SITE_PARSER);
    if (integer_o == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = PyNumber_ToBase(integer_o, 2);
    Py_DECREF(integer_o);
    return _PyValue_FromObjectResult(res);
}


/*[clinic input]
callable as builtin_callable

    obj: object
    /

Return whether the object is callable (i.e., some kind of function).

Note that classes are callable, as are instances of classes with a
__call__() method.
[clinic start generated code]*/

static PyValue
builtin_callable_impl(PyObject *module, PyValueBorrow obj)
/*[clinic end generated code: output=d2ba8f60035cd377 input=1423bab99cc41f58]*/
{
    return PyValue_FromBool(PyValue_IsObject(obj)
                            && PyCallable_Check(PyValue_AsKnownObjectBorrow(obj)));
}

static PyObject *
builtin_breakpoint(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *keywords)
{
    PyObject *hook = PySys_GetAttrString("breakpointhook");
    if (hook == NULL) {
        return NULL;
    }

    if (PySys_Audit("builtins.breakpoint", "O", hook) < 0) {
        Py_DECREF(hook);
        return NULL;
    }

    PyObject *retval = PyObject_Vectorcall(hook, args, nargs, keywords);
    Py_DECREF(hook);
    return retval;
}

PyDoc_STRVAR(breakpoint_doc,
"breakpoint($module, /, *args, **kws)\n\
--\n\
\n\
Call sys.breakpointhook(*args, **kws).  sys.breakpointhook() must accept\n\
whatever arguments are passed.\n\
\n\
By default, this drops you into the pdb debugger.");

typedef struct {
    PyObject_HEAD
    PyObject *func;
    PyObject *it;
} filterobject;

#define _filterobject_CAST(op)      ((filterobject *)(op))

static PyObject *
filter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *func, *seq;
    PyObject *it;
    filterobject *lz;

    if ((type == &PyFilter_Type || type->tp_init == PyFilter_Type.tp_init) &&
        !_PyArg_NoKeywords("filter", kwds))
        return NULL;

    if (!PyArg_UnpackTuple(args, "filter", 2, 2, &func, &seq))
        return NULL;

    /* Get iterator. */
    it = _PyValue_GetIter(PyValue_FromObjectBorrow(seq));
    if (it == NULL)
        return NULL;

    /* create filterobject structure */
    lz = (filterobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(it);
        return NULL;
    }

    lz->func = Py_NewRef(func);
    lz->it = it;

    return (PyObject *)lz;
}

static PyObject *
filter_vectorcall(PyObject *type, PyObject * const*args,
                size_t nargsf, PyObject *kwnames)
{
    PyTypeObject *tp = _PyType_CAST(type);
    if (tp == &PyFilter_Type && !_PyArg_NoKwnames("filter", kwnames)) {
        return NULL;
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("filter", nargs, 2, 2)) {
        return NULL;
    }

    PyObject *it = _PyValue_GetIter(PyValue_FromObjectBorrow(args[1]));
    if (it == NULL) {
        return NULL;
    }

    filterobject *lz = (filterobject *)tp->tp_alloc(tp, 0);

    if (lz == NULL) {
        Py_DECREF(it);
        return NULL;
    }

    lz->func = Py_NewRef(args[0]);
    lz->it = it;

    return (PyObject *)lz;
}

static void
filter_dealloc(PyObject *self)
{
    filterobject *lz = _filterobject_CAST(self);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->func);
    Py_XDECREF(lz->it);
    Py_TYPE(lz)->tp_free(lz);
}

static int
filter_traverse(PyObject *self, visitproc visit, void *arg)
{
    filterobject *lz = _filterobject_CAST(self);
    Py_VISIT(lz->it);
    Py_VISIT(lz->func);
    return 0;
}

/* The value iternext slot of filter (R16): the inner iterator's end, with
   its return value, or its error is filter's. */
static _PyStatusValue
filter_next_value(PyObject *self)
{
    filterobject *lz = _filterobject_CAST(self);
    _PyStatusValue item;
    PyObject *it = lz->it;
    long ok;
    int checktrue = lz->func == Py_None || lz->func == (PyObject *)&PyBool_Type;

    for (;;) {
        item = _PyValue_IterStep(it);
        if (item.status != PYGEN_NEXT)
            return item;

        if (checktrue) {
            ok = _PyValue_IsTrue(item.value);
        } else {
            PyValue good = _PyValue_CallOneArg(_PyThreadState_GET(),
                                               PyValue_FromObjectBorrow(lz->func),
                                               item.value);
            if (PyValue_IsNull(good)) {
                PyValue_CLOSE(item.value);
                return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
            }
            ok = _PyValue_IsTrue(good);
            PyValue_CLOSE(good);
        }
        if (ok > 0)
            return item;
        PyValue_CLOSE(item.value);
        if (ok < 0)
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
}

_Py_VALUE_THUNK_ITERNEXT(filter_next, filter_next_value)

static PyObject *
filter_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    filterobject *lz = _filterobject_CAST(self);
    return Py_BuildValue("O(OO)", Py_TYPE(lz), lz->func, lz->it);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef filter_methods[] = {
    {"__reduce__", filter_reduce, METH_NOARGS, reduce_doc},
    {NULL,           NULL}           /* sentinel */
};

PyDoc_STRVAR(filter_doc,
"filter(function, iterable, /)\n\
--\n\
\n\
Return an iterator yielding those items of iterable for which\n\
function(item) is true.  If function is None, return the items that\n\
are true.");

PyTypeObject PyFilter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "filter",                           /* tp_name */
    sizeof(filterobject),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    filter_dealloc,                     /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    filter_doc,                         /* tp_doc */
    filter_traverse,                    /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    filter_next,                        /* tp_iternext */
    filter_methods,                     /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    filter_new,                         /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
    .tp_vectorcall = filter_vectorcall,
    .tp_value = {
        .iternext = _Py_VALUE_SLOT_INIT(iternext, filter_next_value),
    },
};


/*[clinic input]
format as builtin_format

    value: object
    format_spec: unicode(c_default="NULL") = ''
    /

Return type(value).__format__(value, format_spec)

Many built-in types implement format_spec according to the
Format Specification Mini-language. See help('FORMATTING').

If type(value) does not supply a method named __format__
and format_spec is empty, then str(value) is returned.
See also help('SPECIALMETHODS').
[clinic start generated code]*/

static PyValue
builtin_format_impl(PyObject *module, PyValueBorrow value,
                    PyObject *format_spec)
/*[clinic end generated code: output=b92f89205b44f619 input=45ef3934b86d5624]*/
{
    PyObject *res;
    if (format_spec == NULL) {
        res = _PyValue_Format(value, PyValue_NULL);
    }
    else {
        res = _PyValue_Format(value, PyValue_FromObjectBorrow(format_spec));
    }
    return _PyValue_FromObjectResult(res);
}

/*[clinic input]
@permit_long_summary
chr as builtin_chr

    i: object
    /

Return a Unicode string of one character with ordinal i; 0 <= i <= 0x10ffff.
[clinic start generated code]*/

static PyValue
builtin_chr_impl(PyObject *module, PyValueBorrow i)
/*[clinic end generated code: output=e6570a344c3dd834 input=a9b255f2d2e503f0]*/
{
    int overflow;
    long v = _PyValue_AsLongAndOverflow(i, &overflow);
    if (v == -1 && PyErr_Occurred()) {
        return PyValue_NULL;
    }
    if (overflow) {
        v = overflow < 0 ? INT_MIN : INT_MAX;
        /* Allow PyUnicode_FromOrdinal() to raise an exception */
    }
#if SIZEOF_INT < SIZEOF_LONG
    else if (v < INT_MIN) {
        v = INT_MIN;
    }
    else if (v > INT_MAX) {
        v = INT_MAX;
    }
#endif
    return _PyValue_FromObjectResult(PyUnicode_FromOrdinal(v));
}


/*[clinic input]
@permit_long_summary
compile as builtin_compile

    source: object
    filename: unicode_fs_decoded
    mode: str
    flags: int = 0
    dont_inherit: bool = False
    optimize: int = -1
    *
    module as modname: object = None
    _feature_version as feature_version: int = -1

Compile source into a code object that can be executed by exec() or eval().

The source code may represent a Python module, statement or
expression.
The filename will be used for run-time error messages.
The mode must be 'exec' to compile a module, 'single' to compile a
single (interactive) statement, or 'eval' to compile an expression.
The flags argument, if present, controls which future statements
influence the compilation of the code.
The dont_inherit argument, if true, stops the compilation inheriting
the effects of any future statements in effect in the code calling
compile; if absent or false these statements do influence the
compilation, in addition to any features explicitly specified.
[clinic start generated code]*/

static PyValue
builtin_compile_impl(PyObject *module, PyValueBorrow source,
                     PyObject *filename, const char *mode, int flags,
                     int dont_inherit, int optimize, PyValueBorrow modname,
                     int feature_version)
/*[clinic end generated code: output=7f6bbe1b42a2e62b input=444c4fe466a97279]*/
{
    PyObject *source_copy;
    PyObject *source_o = NULL;
    PyObject *modname_o = NULL;
    const char *str;
    int compile_mode = -1;
    int is_ast;
    int start[] = {Py_file_input, Py_eval_input, Py_single_input, Py_func_type_input};
    PyObject *result;

    PyCompilerFlags cf = _PyCompilerFlags_INIT;
    cf.cf_flags = flags | PyCF_SOURCE_IS_UTF8;
    if (feature_version >= 0 && (flags & PyCF_ONLY_AST)) {
        cf.cf_feature_version = feature_version;
    }

    if (flags &
        ~(PyCF_MASK | PyCF_MASK_OBSOLETE | PyCF_COMPILE_MASK))
    {
        PyErr_SetString(PyExc_ValueError,
                        "compile(): unrecognised flags");
        goto error;
    }
    /* XXX Warn if (supplied_flags & PyCF_MASK_OBSOLETE) != 0? */

    if (optimize < -1 || optimize > 2) {
        PyErr_SetString(PyExc_ValueError,
                        "compile(): invalid optimize value");
        goto error;
    }
    if (PyValue_IsNone(modname)) {
        modname_o = NULL;
    }
    else if (!(PyValue_IsObject(modname) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(modname)))) {
        PyErr_Format(PyExc_TypeError,
                     "compile() argument 'module' must be str or None, not %vT",
                     modname);
        goto error;
    }
    else {
        modname_o = PyValue_AsKnownObjectBorrow(modname);
    }

    if (!dont_inherit) {
        PyEval_MergeCompilerFlags(&cf);
    }

    if (strcmp(mode, "exec") == 0)
        compile_mode = 0;
    else if (strcmp(mode, "eval") == 0)
        compile_mode = 1;
    else if (strcmp(mode, "single") == 0)
        compile_mode = 2;
    else if (strcmp(mode, "func_type") == 0) {
        if (!(flags & PyCF_ONLY_AST)) {
            PyErr_SetString(PyExc_ValueError,
                            "compile() mode 'func_type' requires flag PyCF_ONLY_AST");
            goto error;
        }
        compile_mode = 3;
    }
    else {
        const char *msg;
        if (flags & PyCF_ONLY_AST)
            msg = "compile() mode must be 'exec', 'eval', 'single' or 'func_type'";
        else
            msg = "compile() mode must be 'exec', 'eval' or 'single'";
        PyErr_SetString(PyExc_ValueError, msg);
        goto error;
    }

    source_o = _PyValue_AsObjectAt(source, BOX_SITE_PARSER);
    if (source_o == NULL) {
        goto error;
    }
    is_ast = PyAST_Check(source_o);
    if (is_ast == -1)
        goto error;
    if (is_ast) {
        PyArena *arena = _PyArena_New();
        if (arena == NULL) {
            goto error;
        }

        if (flags & PyCF_ONLY_AST) {
            mod_ty mod = PyAST_obj2mod(source_o, arena, compile_mode);
            if (mod == NULL || !_PyAST_Validate(mod)) {
                _PyArena_Free(arena);
                goto error;
            }
            int syntax_check_only = ((flags & PyCF_OPTIMIZED_AST) == PyCF_ONLY_AST); /* unoptiomized AST */
            if (_PyCompile_AstPreprocess(mod, filename, &cf, optimize, arena,
                                         syntax_check_only, modname_o) < 0)
            {
                _PyArena_Free(arena);
                goto error;
            }
            result = PyAST_mod2obj(mod);
        }
        else {
            mod_ty mod = PyAST_obj2mod(source_o, arena, compile_mode);
            if (mod == NULL || !_PyAST_Validate(mod)) {
                _PyArena_Free(arena);
                goto error;
            }
            result = (PyObject*)_PyAST_Compile(mod, filename,
                                               &cf, optimize, arena, modname_o);
        }
        _PyArena_Free(arena);
        goto finally;
    }

    str = _Py_SourceAsString(source_o, "compile", "string, bytes or AST", &cf, &source_copy);
    if (str == NULL)
        goto error;

#ifdef Py_GIL_DISABLED
    // Disable immortalization of code constants for explicit
    // compile() calls to get consistent frozen outputs between the default
    // and free-threaded builds.
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    tstate->suppress_co_const_immortalization++;
#endif

    result = _Py_CompileString(str, filename,
                               start[compile_mode], &cf,
                               optimize, modname_o);

#ifdef Py_GIL_DISABLED
    tstate->suppress_co_const_immortalization--;
#endif

    Py_XDECREF(source_copy);
    goto finally;

error:
    result = NULL;
finally:
    Py_XDECREF(source_o);
    return _PyValue_FromObjectResult(result);
}

/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
/*[clinic input]
dir as builtin_dir

    object as arg: object = NULL
    /

Return an alphabetized list of the attributes of the object.

If called without an argument, return the names in the current scope.
Else, return an alphabetized list of names comprising (some of) the
attributes of the given object, and of attributes reachable from it.
If the object supplies a method named __dir__, it will be used;
otherwise the default dir() logic is used and returns:
  for a module object: the module's attributes.
  for a class object:  its attributes, and recursively the attributes
    of its bases.
  for any other object: its attributes, its class's attributes, and
    recursively the attributes of its class's base classes.
[clinic start generated code]*/

static PyValue
builtin_dir_impl(PyObject *module, PyValueBorrow arg)
/*[clinic end generated code: output=398c560db1a4e5df input=7fe1b2a5ca9ae355]*/
{
    PyObject *arg_o;
    if (box_optional(arg, &arg_o) < 0) {
        return PyValue_NULL;
    }
    PyObject *res = PyObject_Dir(arg_o);
    Py_XDECREF(arg_o);
    return _PyValue_FromObjectResult(res);
}

/*[clinic input]
divmod as builtin_divmod

    x: object
    y: object
    /

Return the tuple (x//y, x%y).  Invariant: div*y + mod == x.
[clinic start generated code]*/

static PyValue
builtin_divmod_impl(PyObject *module, PyValueBorrow x, PyValueBorrow y)
/*[clinic end generated code: output=e107dfecffcc863a input=175ad9c84ff41a85]*/
{
    return _PyValue_Divmod(x, y);
}


/*[clinic input]
eval as builtin_eval

    source: object
    /
    globals: object = None
    locals: object = None

Evaluate the given source in the context of globals and locals.

The source may be a string representing a Python expression
or a code object as returned by compile().
The globals must be a dictionary and locals can be any mapping,
defaulting to the current globals and locals.
If only globals is given, locals defaults to it.
[clinic start generated code]*/

static PyValue
builtin_eval_impl(PyObject *module, PyValueBorrow source,
                  PyValueBorrow globals, PyValueBorrow locals)
/*[clinic end generated code: output=4dd3663bf7b3bd62 input=7c7bce5299a89062]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *result = NULL, *source_copy;
    const char *str;

    if (!PyValue_IsNone(locals)
        && !(PyValue_IsObject(locals)
             && PyMapping_Check(PyValue_AsKnownObjectBorrow(locals))))
    {
        PyErr_SetString(PyExc_TypeError, "locals must be a mapping");
        return PyValue_NULL;
    }
    if (!PyValue_IsNone(globals)
        && !(PyValue_IsObject(globals)
             && PyAnyDict_Check(PyValue_AsKnownObjectBorrow(globals))))
    {
        PyErr_SetString(PyExc_TypeError, (PyValue_IsObject(globals)
                && PyMapping_Check(PyValue_AsKnownObjectBorrow(globals))) ?
            "globals must be a real dict or a frozendict; "
            "try eval(expr, {}, mapping)"
            : "globals must be a dict or a frozendict");
        return PyValue_NULL;
    }

    /* Owned references. A non-None globals is a dict and a non-None locals
       a mapping, so both are objects. */
    PyObject *globals_o = NULL, *locals_o = NULL;
    int fromframe = 0;
    if (!PyValue_IsNone(globals)) {
        globals_o = Py_NewRef(PyValue_AsKnownObjectBorrow(globals));
    }
    else if (_PyEval_GetFrame() != NULL) {
        fromframe = 1;
        globals_o = PyEval_GetGlobals();
        assert(globals_o != NULL);
        Py_INCREF(globals_o);
    }
    else {
        globals_o = _PyEval_GetGlobalsFromRunningMain(tstate);
        if (globals_o == NULL) {
            if (!_PyErr_Occurred(tstate)) {
                PyErr_SetString(PyExc_TypeError,
                    "eval must be given globals and locals "
                    "when called without a frame");
            }
            return PyValue_NULL;
        }
        Py_INCREF(globals_o);
    }

    if (!PyValue_IsNone(locals)) {
        locals_o = Py_NewRef(PyValue_AsKnownObjectBorrow(locals));
    }
    else if (fromframe) {
        locals_o = _PyEval_GetFrameLocals();
        if (locals_o == NULL) {
            assert(PyErr_Occurred());
            Py_DECREF(globals_o);
            return PyValue_NULL;
        }
    }
    else {
        locals_o = Py_NewRef(globals_o);
    }

    if (_PyEval_EnsureBuiltins(tstate, globals_o, NULL) < 0) {
        goto error;
    }

    if ((PyValue_IsObject(source) && PyCode_Check(PyValue_AsKnownObjectBorrow(source)))) {
        PyObject *code = PyValue_AsKnownObjectBorrow(source);
        if (PySys_Audit("exec", "O", code) < 0) {
            goto error;
        }

        if (PyCode_GetNumFree((PyCodeObject *)code) > 0) {
            PyErr_SetString(PyExc_TypeError,
                "code object passed to eval() may not contain free variables");
            goto error;
        }
        result = PyEval_EvalCode(code, globals_o, locals_o);
    }
    else {
        PyCompilerFlags cf = _PyCompilerFlags_INIT;
        cf.cf_flags = PyCF_SOURCE_IS_UTF8;
        PyObject *source_o = _PyValue_AsObjectAt(source, BOX_SITE_PARSER);
        if (source_o == NULL)
            goto error;
        str = _Py_SourceAsString(source_o, "eval", "string, bytes or code", &cf, &source_copy);
        if (str == NULL) {
            Py_DECREF(source_o);
            goto error;
        }

        while (*str == ' ' || *str == '\t')
            str++;

        (void)PyEval_MergeCompilerFlags(&cf);
#ifdef Py_GIL_DISABLED
        // Don't immortalize code constants for explicit eval() calls
        // to avoid memory leaks.
        _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
        tstate->suppress_co_const_immortalization++;
#endif
        result = PyRun_StringFlags(str, Py_eval_input, globals_o, locals_o, &cf);
#ifdef Py_GIL_DISABLED
        tstate->suppress_co_const_immortalization--;
#endif
        Py_XDECREF(source_copy);
        Py_DECREF(source_o);
    }

  error:
    Py_XDECREF(globals_o);
    Py_XDECREF(locals_o);
    return _PyValue_FromObjectResult(result);
}

/*[clinic input]
exec as builtin_exec

    source: object
    /
    globals: object = None
    locals: object = None
    *
    closure: object(c_default="NULL") = None

Execute the given source in the context of globals and locals.

The source may be a string representing one or more Python statements
or a code object as returned by compile().
The globals must be a dictionary and locals can be any mapping,
defaulting to the current globals and locals.
If only globals is given, locals defaults to it.
The closure must be a tuple of cellvars, and can only be used
when source is a code object requiring exactly that many cellvars.
[clinic start generated code]*/

static PyValue
builtin_exec_impl(PyObject *module, PyValueBorrow source,
                  PyValueBorrow globals, PyValueBorrow locals,
                  PyValueBorrow closure)
/*[clinic end generated code: output=0db00e43d4c0bc32 input=25e989b6d87a3a21]*/
{
    PyThreadState *tstate = _PyThreadState_GET();
    PyObject *v;
    /* Owned references; an explicit globals or locals is boxed so that the
       checks below raise their errors for an int or float too. */
    PyObject *globals_o = NULL, *locals_o = NULL;

    int fromframe = 0;
    if (!PyValue_IsNone(globals)) {
        globals_o = _PyValue_AsObjectAt(globals, BOX_SITE_PARSER);
        if (globals_o == NULL) {
            goto error;
        }
    }
    else if (_PyEval_GetFrame() != NULL) {
        fromframe = 1;
        globals_o = PyEval_GetGlobals();
        assert(globals_o != NULL);
        Py_INCREF(globals_o);
    }
    else {
        globals_o = _PyEval_GetGlobalsFromRunningMain(tstate);
        if (globals_o == NULL) {
            if (!_PyErr_Occurred(tstate)) {
                PyErr_SetString(PyExc_SystemError,
                                "globals and locals cannot be NULL");
            }
            goto error;
        }
        Py_INCREF(globals_o);
    }

    if (!PyValue_IsNone(locals)) {
        locals_o = _PyValue_AsObjectAt(locals, BOX_SITE_PARSER);
        if (locals_o == NULL) {
            goto error;
        }
    }
    else if (fromframe) {
        locals_o = _PyEval_GetFrameLocals();
        if (locals_o == NULL) {
            assert(PyErr_Occurred());
            Py_DECREF(globals_o);
            return PyValue_NULL;
        }
    }
    else {
        locals_o = Py_NewRef(globals_o);
    }

    if (!PyAnyDict_Check(globals_o)) {
        PyErr_Format(PyExc_TypeError,
                     "exec() globals must be a dict or a frozendict, not %T",
                     globals_o);
        goto error;
    }
    if (!PyMapping_Check(locals_o)) {
        PyErr_Format(PyExc_TypeError,
            "locals must be a mapping or None, not %.100s",
            Py_TYPE(locals_o)->tp_name);
        goto error;
    }

    if (_PyEval_EnsureBuiltins(tstate, globals_o, NULL) < 0) {
        goto error;
    }

    /* A closure of None is no closure. */
    int has_closure = !PyValue_IsNone(closure) && !PyValue_IsNull(closure);

    if ((PyValue_IsObject(source) && PyCode_Check(PyValue_AsKnownObjectBorrow(source)))) {
        PyObject *code = PyValue_AsKnownObjectBorrow(source);
        PyObject *closure_o = (has_closure && PyValue_IsObject(closure))
            ? PyValue_AsKnownObjectBorrow(closure) : NULL;
        Py_ssize_t num_free = PyCode_GetNumFree((PyCodeObject *)code);
        if (num_free == 0) {
            if (has_closure) {
                PyErr_SetString(PyExc_TypeError,
                    "cannot use a closure with this code object");
                goto error;
            }
        } else {
            int closure_is_ok =
                has_closure
                && (closure_o != NULL && PyTuple_CheckExact(closure_o))
                && (PyTuple_GET_SIZE(closure_o) == num_free);
            if (closure_is_ok) {
                for (Py_ssize_t i = 0; i < num_free; i++) {
                    PyValueBorrow value = PyTuple_GET_VALUE(closure_o, i);
                    if (PyValue_TYPE(value) != &PyCell_Type) {
                        closure_is_ok = 0;
                        break;
                    }
                }
            }
            if (!closure_is_ok) {
                PyErr_Format(PyExc_TypeError,
                    "code object requires a closure of exactly length %zd",
                    num_free);
                goto error;
            }
        }

        if (PySys_Audit("exec", "O", code) < 0) {
            goto error;
        }

        if (!has_closure) {
            v = PyEval_EvalCode(code, globals_o, locals_o);
        } else {
            v = PyEval_EvalCodeEx(code, globals_o, locals_o,
                NULL, 0,
                NULL, 0,
                NULL, 0,
                NULL,
                closure_o);
        }
    }
    else {
        if (has_closure) {
            PyErr_SetString(PyExc_TypeError,
                "closure can only be used when source is a code object");
            goto error;
        }
        PyObject *source_copy;
        const char *str;
        PyCompilerFlags cf = _PyCompilerFlags_INIT;
        cf.cf_flags = PyCF_SOURCE_IS_UTF8;
        PyObject *source_o = _PyValue_AsObjectAt(source, BOX_SITE_PARSER);
        if (source_o == NULL)
            goto error;
        str = _Py_SourceAsString(source_o, "exec",
                                       "string, bytes or code", &cf,
                                       &source_copy);
        if (str == NULL) {
            Py_DECREF(source_o);
            goto error;
        }
        if (PyEval_MergeCompilerFlags(&cf))
            v = PyRun_StringFlags(str, Py_file_input, globals_o,
                                  locals_o, &cf);
        else
            v = PyRun_String(str, Py_file_input, globals_o, locals_o);
        Py_XDECREF(source_copy);
        Py_DECREF(source_o);
    }
    if (v == NULL)
        goto error;
    Py_DECREF(globals_o);
    Py_DECREF(locals_o);
    Py_DECREF(v);
    return PyValue_None;

  error:
    Py_XDECREF(globals_o);
    Py_XDECREF(locals_o);
    return PyValue_NULL;
}


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_getattr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *v, *name, *result;

    if (!_PyArg_CheckPositional("getattr", nargs, 2, 3))
        return NULL;

    v = args[0];
    name = args[1];
    if (nargs > 2) {
        if (PyObject_GetOptionalAttr(v, name, &result) == 0) {
            PyObject *dflt = args[2];
            return Py_NewRef(dflt);
        }
    }
    else {
        PyValue result_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(v), name);
        result = PyValue_IsNull(result_v) ? NULL
            : _PyValue_AsObjectStealAt(result_v, BOX_SITE_ATTRIBUTE);
    }
    return result;
}

PyDoc_STRVAR(getattr_doc,
"getattr(object, name[, default]) -> value\n\
\n\
Get a named attribute from an object.\n\
\n\
getattr(x, 'y') is equivalent to x.y.\n\
When a default argument is given, it is returned when the attribute\n\
doesn't exist; without it, an exception is raised in that case.");


/*[clinic input]
globals as builtin_globals

Return the dictionary containing the current scope's global variables.

NOTE: Updates to this dictionary *will* affect name lookups in the
current global scope and vice-versa.
[clinic start generated code]*/

static PyValue
builtin_globals_impl(PyObject *module)
/*[clinic end generated code: output=285033b6aa6822cb input=6d725a9b48d1eaeb]*/
{
    PyObject *globals;
    if (_PyEval_GetFrame() != NULL) {
        globals = PyEval_GetGlobals();
        assert(globals != NULL);
        return PyValue_DUP(PyValue_FromObjectBorrow(globals));
    }
    PyThreadState *tstate = _PyThreadState_GET();
    globals = _PyEval_GetGlobalsFromRunningMain(tstate);
    if (globals == NULL) {
        if (_PyErr_Occurred(tstate)) {
            return PyValue_NULL;
        }
        return PyValue_None;
    }
    return PyValue_DUP(PyValue_FromObjectBorrow(globals));
}


/*[clinic input]
hasattr as builtin_hasattr

    obj: object
    name: object
    /

Return whether the object has an attribute with the given name.

This is done by calling getattr(obj, name) and catching AttributeError.
[clinic start generated code]*/

static PyValue
builtin_hasattr_impl(PyObject *module, PyValueBorrow obj, PyValueBorrow name)
/*[clinic end generated code: output=89dcf5dddb9efd55 input=0faec9787d979542]*/
{
    PyObject *name_o = _PyValue_AsObjectAt(name, BOX_SITE_PARSER);
    if (name_o == NULL) {
        return PyValue_NULL;
    }
    _PyStatusValue r = _PyValue_GetOptionalAttr(obj, name_o);
    Py_DECREF(name_o);
    if (r.status < 0) {
        return PyValue_NULL;
    }
    if (r.status == 0) {
        return PyValue_False;
    }
    PyValue_CLOSE(r.value);
    return PyValue_True;
}


/* AC: gdb's integration with CPython relies on builtin_id having
 * the *exact* parameter names of "self" and "v", so we ensure we
 * preserve those name rather than using the AC defaults.
 */
/*[clinic input]
id as builtin_id

    self: self(type="PyModuleDef *")
    obj as v: object
    /

Return the identity of an object.

This is guaranteed to be unique among simultaneously existing objects.
(CPython uses the object's memory address, or for some ints and floats
a number derived from the value.)
[clinic start generated code]*/

static PyValue
builtin_id_impl(PyModuleDef *self, PyValueBorrow v)
/*[clinic end generated code: output=89ada1e47a4b635c input=b57e756e2c58d479]*/
{
    PyValue id = PyValue_FromUInt64(_PyValue_Identity(v));
    if (PyValue_IsNull(id)) {
        return PyValue_NULL;
    }

    /* The audit event carries the id as an object: box it only when a hook
       will see the event. */
    PyThreadState *tstate = _PyThreadState_GET();
    if (!_PySys_AuditEnabled(tstate)) {
        return id;
    }
    PyObject *id_o = _PyValue_AsObjectAt(id, BOX_SITE_PARSER);
    if (id_o == NULL) {
        PyValue_CLOSE(id);
        return PyValue_NULL;
    }
    int audit = PySys_Audit("builtins.id", "O", id_o);
    Py_DECREF(id_o);
    if (audit < 0) {
        PyValue_CLOSE(id);
        return PyValue_NULL;
    }

    return id;
}


/* map object ************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *iters;
    PyObject *func;
    int strict;
} mapobject;

#define _mapobject_CAST(op)     ((mapobject *)(op))

static PyObject *
map_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *it, *iters, *func;
    mapobject *lz;
    Py_ssize_t numargs, i;
    int strict = 0;

    if (kwds) {
        PyObject *empty = PyTuple_New(0);
        if (empty == NULL) {
            return NULL;
        }
        static char *kwlist[] = {"strict", NULL};
        int parsed = PyArg_ParseTupleAndKeywords(
                empty, kwds, "|$p:map", kwlist, &strict);
        Py_DECREF(empty);
        if (!parsed) {
            return NULL;
        }
    }

    numargs = PyTuple_Size(args);
    if (numargs < 2) {
        PyErr_SetString(PyExc_TypeError,
           "map() must have at least two arguments.");
        return NULL;
    }

    iters = PyTuple_New(numargs-1);
    if (iters == NULL)
        return NULL;

    for (i=1 ; i<numargs ; i++) {
        /* Get iterator. */
        PyValue arg_v = PyTuple_GetValue(args, i);
        PyObject *arg = PyValue_IsNull(arg_v) ? NULL
            : _PyValue_AsObjectAt(arg_v, BOX_SITE_TUPLE);
        PyValue_XCLOSE(arg_v);
        if (arg == NULL) {
            Py_DECREF(iters);
            return NULL;
        }
        it = _PyValue_GetIter(PyValue_FromObjectBorrow(arg));
        Py_DECREF(arg);
        if (it == NULL) {
            Py_DECREF(iters);
            return NULL;
        }
        PyTuple_SET_VALUE(iters, i-1, PyValue_PrepareOwned(it));
    }

    /* create mapobject structure */
    lz = (mapobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(iters);
        return NULL;
    }
    PyValue func_v = PyTuple_GetValue(args, 0);
    func = PyValue_IsNull(func_v) ? NULL
        : _PyValue_AsObjectAt(func_v, BOX_SITE_TUPLE);
    PyValue_XCLOSE(func_v);
    if (func == NULL) {
        Py_DECREF(lz);
        Py_DECREF(iters);
        return NULL;
    }
    lz->iters = iters;
    lz->func = func;
    lz->strict = strict;

    return (PyObject *)lz;
}

static PyObject *
map_vectorcall(PyObject *type, PyObject * const*args,
                size_t nargsf, PyObject *kwnames)
{
    PyTypeObject *tp = _PyType_CAST(type);

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (kwnames != NULL && PyTuple_GET_SIZE(kwnames) != 0) {
        // Fallback to map_new()
        PyThreadState *tstate = _PyThreadState_GET();
        return _PyObject_MakeTpCall(tstate, type, args, nargs, kwnames);
    }

    if (nargs < 2) {
        PyErr_SetString(PyExc_TypeError,
           "map() must have at least two arguments.");
        return NULL;
    }

    PyObject *iters = PyTuple_New(nargs-1);
    if (iters == NULL) {
        return NULL;
    }

    for (int i=1; i<nargs; i++) {
        PyObject *it = _PyValue_GetIter(PyValue_FromObjectBorrow(args[i]));
        if (it == NULL) {
            Py_DECREF(iters);
            return NULL;
        }
        PyTuple_SET_VALUE(iters, i-1, PyValue_PrepareOwned(it));
    }

    mapobject *lz = (mapobject *)tp->tp_alloc(tp, 0);
    if (lz == NULL) {
        Py_DECREF(iters);
        return NULL;
    }
    lz->iters = iters;
    lz->func = Py_NewRef(args[0]);
    lz->strict = 0;

    return (PyObject *)lz;
}

static void
map_dealloc(PyObject *self)
{
    mapobject *lz = _mapobject_CAST(self);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->iters);
    Py_XDECREF(lz->func);
    Py_TYPE(lz)->tp_free(lz);
}

static int
map_traverse(PyObject *self, visitproc visit, void *arg)
{
    mapobject *lz = _mapobject_CAST(self);
    Py_VISIT(lz->iters);
    Py_VISIT(lz->func);
    return 0;
}

/* The value iternext slot of map (R16): without strict, the end of an
   inner iterator, with its return value, or its error is map's. */
static _PyStatusValue
map_next_value(PyObject *self)
{
    mapobject *lz = _mapobject_CAST(self);
    Py_ssize_t i;
    PyValue small_stack[_PY_FASTCALL_SMALL_STACK];
    PyValue *stack;
    _PyStatusValue result = {PYGEN_ERROR, PyValue_NULL};
    PyThreadState *tstate = _PyThreadState_GET();

    const Py_ssize_t niters = PyTuple_GET_SIZE(lz->iters);
    if (niters <= (Py_ssize_t)Py_ARRAY_LENGTH(small_stack)) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc(niters * sizeof(stack[0]));
        if (stack == NULL) {
            _PyErr_NoMemory(tstate);
            return result;
        }
    }

    Py_ssize_t nargs = 0;
    _PyStatusValue val;
    PyValue call_result;
    for (i = 0; i < niters; i++) {
        // iters holds only iterators made by PyObject_GetIter(): never a scalar.
        PyObject *it = _PyTuple_GET_OBJECT(lz->iters, i);
        val = _PyValue_IterStep(it);
        if (val.status != PYGEN_NEXT) {
            if (lz->strict) {
                goto check;
            }
            result = val;
            goto exit;
        }
        stack[i] = val.value;
        nargs++;
    }

    call_result = _PyValue_VectorcallTstate(tstate, PyValue_FromObjectBorrow(lz->func), stack, nargs,
                                            NULL);
    if (!PyValue_IsNull(call_result)) {
        result = (_PyStatusValue){PYGEN_NEXT, call_result};
    }
    goto exit;

check:
    if (val.status == PYGEN_ERROR) {
        if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
            // next() on argument i raised an exception (not StopIteration)
            goto exit_no_result;
        }
        PyErr_Clear();
    }
    else {
        // Strict mode drops the return value of argument i
        PyValue_XCLOSE(val.value);
    }
    if (i) {
        // ValueError: map() argument 2 is shorter than argument 1
        // ValueError: map() argument 3 is shorter than arguments 1-2
        const char* plural = i == 1 ? " " : "s 1-";
        PyErr_Format(PyExc_ValueError,
                     "map() argument %zd is shorter than argument%s%zd",
                     i + 1, plural, i);
        goto exit_no_result;
    }
    for (i = 1; i < niters; i++) {
        // iters holds only iterators made by PyObject_GetIter(): never a scalar.
        PyObject *it = _PyTuple_GET_OBJECT(lz->iters, i);
        val = _PyValue_IterStep(it);
        if (val.status == PYGEN_NEXT) {
            PyValue_CLOSE(val.value);
            const char* plural = i == 1 ? " " : "s 1-";
            PyErr_Format(PyExc_ValueError,
                         "map() argument %zd is longer than argument%s%zd",
                         i + 1, plural, i);
            goto exit_no_result;
        }
        if (val.status == PYGEN_ERROR) {
            if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
                // next() on argument i raised an exception (not StopIteration)
                goto exit_no_result;
            }
            PyErr_Clear();
        }
        else {
            PyValue_XCLOSE(val.value);
        }
        // Argument i is exhausted. So far so good...
    }
    // All arguments are exhausted. Success!
    result = (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    goto exit;

exit_no_result:
    assert(result.status == PYGEN_ERROR && PyValue_IsNull(result.value));

exit:
    for (i = 0; i < nargs; i++) {
        PyValue_CLOSE(stack[i]);
    }
    if (stack != small_stack) {
        PyMem_Free(stack);
    }
    return result;
}

_Py_VALUE_THUNK_ITERNEXT(map_next, map_next_value)

static PyObject *
map_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    mapobject *lz = _mapobject_CAST(self);
    Py_ssize_t numargs = PyTuple_GET_SIZE(lz->iters);
    PyObject *args = PyTuple_New(numargs+1);
    Py_ssize_t i;
    if (args == NULL)
        return NULL;
    PyTuple_SET_VALUE(args, 0, PyValue_DUP(PyValue_FromObjectBorrow(lz->func)));
    for (i = 0; i<numargs; i++){
        PyValueBorrow it = PyTuple_GET_VALUE(lz->iters, i);
        PyTuple_SET_VALUE(args, i+1, PyValue_DUP(it));
    }

    if (lz->strict) {
        return Py_BuildValue("ONO", Py_TYPE(lz), args, Py_True);
    }
    return Py_BuildValue("ON", Py_TYPE(lz), args);
}

PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyObject *
map_setstate(PyObject *self, PyObject *state)
{
    int strict = _PyValue_IsTrue(PyValue_FromObjectBorrow(state));
    if (strict < 0) {
        return NULL;
    }
    mapobject *lz = _mapobject_CAST(self);
    lz->strict = strict;
    Py_RETURN_NONE;
}

static PyMethodDef map_methods[] = {
    {"__reduce__", map_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", map_setstate, METH_O, setstate_doc},
    {NULL,           NULL}           /* sentinel */
};


PyDoc_STRVAR(map_doc,
"map(function, iterable, /, *iterables, strict=False)\n\
--\n\
\n\
Make an iterator that computes the function using arguments from\n\
each of the iterables.  Stops when the shortest iterable is exhausted.\n\
\n\
If strict is true and one of the arguments is exhausted before the\n\
others, raise a ValueError.");

PyTypeObject PyMap_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "map",                              /* tp_name */
    sizeof(mapobject),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    map_dealloc,                        /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    map_doc,                            /* tp_doc */
    map_traverse,                       /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    map_next,                           /* tp_iternext */
    map_methods,                        /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    map_new,                            /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
    .tp_vectorcall = map_vectorcall,
    .tp_value = { .iternext = _Py_VALUE_SLOT_INIT(iternext, map_next_value) },
};


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
static PyObject *
builtin_next(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *it;

    if (!_PyArg_CheckPositional("next", nargs, 1, 2))
        return NULL;

    it = args[0];
    if (!PyIter_Check(it)) {
        PyErr_Format(PyExc_TypeError,
            "'%.200s' object is not an iterator",
            Py_TYPE(it)->tp_name);
        return NULL;
    }

    // The raw step (R11): an iterator's return value reaches the caller
    // as StopIteration(value), as from the iterator's own __next__().
    _PyStatusValue res = _PyValue_IterStep(it);
    if (res.status == PYGEN_NEXT) {
        // next() returns an object: an inline item is boxed here.
        if (PyValue_IsInline(res.value)) {
            return _PyValue_AsObjectAt(res.value, BOX_SITE_ITERATION);
        }
        return PyValue_AsKnownObjectSteal(res.value);
    } else if (nargs > 1) {
        PyObject *def = args[1];
        if (res.status == PYGEN_ERROR) {
            if(!PyErr_ExceptionMatches(PyExc_StopIteration))
                return NULL;
            PyErr_Clear();
        }
        else {
            PyValue_XCLOSE(res.value);
        }
        return Py_NewRef(def);
    } else if (res.status == PYGEN_ERROR) {
        return NULL;
    } else if (PyValue_IsNull(res.value)) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    } else {
        (void)_PyGen_SetStopIterationValue(res.value);
        PyValue_CLOSE(res.value);
        return NULL;
    }
}

PyDoc_STRVAR(next_doc,
"next(iterator[, default])\n\
\n\
Return the next item from the iterator.  If default is given and the\n\
iterator is exhausted, it is returned instead of raising StopIteration.");


/*[clinic input]
setattr as builtin_setattr

    obj: object
    name: object
    value: object
    /

Sets the named attribute on the given object to the specified value.

setattr(x, 'y', v) is equivalent to ``x.y = v``
[clinic start generated code]*/

static PyValue
builtin_setattr_impl(PyObject *module, PyValueBorrow obj, PyValueBorrow name,
                     PyValueBorrow value)
/*[clinic end generated code: output=46363f152b6a9818 input=5e26417f2e8598d4]*/
{
    PyObject *name_o = _PyValue_AsObjectAt(name, BOX_SITE_PARSER);
    if (name_o == NULL) {
        return PyValue_NULL;
    }
    int err = _PyValue_SetAttr(obj, name_o, value);
    Py_DECREF(name_o);
    if (err != 0)
        return PyValue_NULL;
    return PyValue_None;
}


/*[clinic input]
delattr as builtin_delattr

    obj: object
    name: object
    /

Deletes the named attribute from the given object.

delattr(x, 'y') is equivalent to ``del x.y``
[clinic start generated code]*/

static PyValue
builtin_delattr_impl(PyObject *module, PyValueBorrow obj, PyValueBorrow name)
/*[clinic end generated code: output=8df539ed9bfffcf2 input=164865623abe7216]*/
{
    PyObject *name_o = _PyValue_AsObjectAt(name, BOX_SITE_PARSER);
    if (name_o == NULL) {
        return PyValue_NULL;
    }
    int err = _PyValue_SetAttr(obj, name_o, PyValue_NULL);
    Py_DECREF(name_o);
    if (err < 0) {
        return PyValue_NULL;
    }
    return PyValue_None;
}


/*[clinic input]
hash as builtin_hash

    obj: object
    /

Return the integer hash value for the given object.

Two objects that compare equal must also have the same hash value, but
the reverse is not necessarily true.  Hash values may differ between
Python processes.  Not all objects are hashable; calling hash() on an
unhashable object raises TypeError.
[clinic start generated code]*/

static PyValue
builtin_hash_impl(PyObject *module, PyValueBorrow obj)
/*[clinic end generated code: output=b1515333926b0c3b input=70a242ff65f6717c]*/
{
    Py_hash_t x;

    x = _PyValue_Hash(obj);
    if (x == -1)
        return PyValue_NULL;
    return PyValue_FromInt64(x);
}


/*[clinic input]
hex as builtin_hex

    integer: object
    /

Return the hexadecimal representation of an integer.

   >>> hex(12648430)
   '0xc0ffee'
[clinic start generated code]*/

static PyValue
builtin_hex_impl(PyObject *module, PyValueBorrow integer)
/*[clinic end generated code: output=5e7c48771df0a68c input=3bef4746efc62fac]*/
{
    PyObject *integer_o = _PyValue_AsObjectAt(integer, BOX_SITE_PARSER);
    if (integer_o == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = PyNumber_ToBase(integer_o, 16);
    Py_DECREF(integer_o);
    return _PyValue_FromObjectResult(res);
}


/*[clinic input]
@text_signature "($module, object, /, [stop_value], *, stop_exception=StopIteration)"
iter as builtin_iter

    object: object
    /
    stop_value: object = NULL
    *
    stop_exception: object = NULL

Get an iterator from an object.

In the first form, the argument must supply its own iterator, or be a
sequence.  In the second form, the callable is called until it returns
the stop value or raises the specified exception.
[clinic start generated code]*/

static PyValue
builtin_iter_impl(PyObject *module, PyValueBorrow object,
                  PyValueBorrow stop_value, PyValueBorrow stop_exception)
/*[clinic end generated code: output=ef2f2fd43d4f3ca3 input=d3a2f767f29d9ae6]*/
{
    if (PyValue_IsNull(stop_value) && PyValue_IsNull(stop_exception)) {
        return _PyValue_FromObjectResult(_PyValue_GetIter(object));
    }
    if (!(PyValue_IsObject(object)
          && PyCallable_Check(PyValue_AsKnownObjectBorrow(object)))) {
        PyErr_SetString(PyExc_TypeError,
                        "iter(): the first argument must be callable");
        return PyValue_NULL;
    }
    PyObject *stop_value_o = NULL, *stop_exception_o = NULL, *res = NULL;
    if (box_optional(stop_value, &stop_value_o) == 0
        && box_optional(stop_exception, &stop_exception_o) == 0)
    {
        res = _PyCallIter_NewEx(PyValue_AsKnownObjectBorrow(object),
                                stop_value_o, stop_exception_o);
    }
    Py_XDECREF(stop_value_o);
    Py_XDECREF(stop_exception_o);
    return _PyValue_FromObjectResult(res);
}


/*[clinic input]
len as builtin_len

    obj: object
    /

Return the number of items in a container.
[clinic start generated code]*/

static PyValue
builtin_len_impl(PyObject *module, PyValueBorrow obj)
/*[clinic end generated code: output=f62bb6f5aa2d1e95 input=bc55598da9e9c9b5]*/
{
    Py_ssize_t res;

    res = _PyValue_Size(obj);
    if (res < 0) {
        assert(PyErr_Occurred());
        return PyValue_NULL;
    }
    return PyValue_FromInt64(res);
}


/*[clinic input]
locals as builtin_locals

Return a dictionary containing the current scope's local variables.

NOTE: Whether or not updates to this dictionary will affect name
lookups in the local scope and vice-versa is *implementation
dependent* and not covered by any backwards compatibility
guarantees.
[clinic start generated code]*/

static PyValue
builtin_locals_impl(PyObject *module)
/*[clinic end generated code: output=d6520028fc3bdf52 input=989cc75c22167c42]*/
{
    PyObject *locals;
    if (_PyEval_GetFrame() != NULL) {
        locals = _PyEval_GetFrameLocals();
        assert(locals != NULL || PyErr_Occurred());
        return _PyValue_FromObjectResult(locals);
    }
    PyThreadState *tstate = _PyThreadState_GET();
    locals = _PyEval_GetGlobalsFromRunningMain(tstate);
    if (locals == NULL) {
        if (_PyErr_Occurred(tstate)) {
            return PyValue_NULL;
        }
        return PyValue_None;
    }
    return PyValue_DUP(PyValue_FromObjectBorrow(locals));
}


static PyValue
min_max(const PyValue *args, Py_ssize_t nargs, PyObject *kwnames, int op)
{
    PyObject *it = NULL;
    PyValue item, val;
    PyValue maxitem = PyValue_NULL;   /* the result */
    PyValue maxval = PyValue_NULL;    /* the value associated with the result */
    PyValueBorrow keyfunc = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow defaultval = PyValue_Borrow(PyValue_NULL);
    const char *name = (op == Py_LT) ? "min" : "max";
    PyThreadState *tstate = _PyThreadState_GET();

    if (nargs == 0) {
        PyErr_Format(PyExc_TypeError, "%s expected at least 1 argument, got 0", name);
        return PyValue_NULL;
    }

    /* The keyword-only key and default follow the positional words. */
    if (kwnames != NULL) {
        Py_ssize_t nkw = PyTuple_GET_SIZE(kwnames);
        if (nkw > 2) {
            PyErr_Format(PyExc_TypeError,
                         "%s() takes at most 2 keyword arguments (%zd given)",
                         name, nkw);
            return PyValue_NULL;
        }
        for (Py_ssize_t k = 0; k < nkw; k++) {
            PyObject *kw = PyValue_AsKnownObjectBorrow(PyTuple_GET_VALUE(kwnames, k));
            if (_PyUnicode_Equal(kw, &_Py_ID(key))) {
                keyfunc = PyValue_Borrow(args[nargs + k]);
            }
            else if (_PyUnicode_Equal(kw, &_Py_ID(default))) {
                defaultval = PyValue_Borrow(args[nargs + k]);
            }
            else {
                PyErr_Format(PyExc_TypeError,
                             "%s() got an unexpected keyword argument '%S'",
                             name, kw);
                return PyValue_NULL;
            }
        }
    }

    const int positional = nargs > 1; // False iff nargs == 1
    if (positional && !PyValue_IsNull(defaultval)) {
        PyErr_Format(PyExc_TypeError,
                        "Cannot specify a default for %s() with multiple "
                        "positional arguments", name);
        return PyValue_NULL;
    }

    if (!positional) {
        it = _PyValue_GetIter(PyValue_Borrow(args[0]));
        if (it == NULL) {
            return PyValue_NULL;
        }
    }

    if (PyValue_IsNone(keyfunc)) {
        keyfunc = PyValue_Borrow(PyValue_NULL);
    }

    while (1) {
        if (it == NULL) {
            if (nargs-- <= 0) {
                break;
            }
            item = PyValue_DUP(PyValue_Borrow(*args++));
        }
        else {
            _PyStatusValue step = _PyValue_IterNext(it);
            if (step.status == PYGEN_ERROR) {
                goto Fail_it;
            }
            if (step.status == PYGEN_RETURN) {
                break;
            }
            item = step.value;
        }

        /* get the value from the key function */
        if (!PyValue_IsNull(keyfunc)) {
            val = _PyValue_CallOneArg(tstate, keyfunc, item);
            if (PyValue_IsNull(val))
                goto Fail_it_item;
        }
        /* no key function; the value is the item */
        else {
            val = PyValue_DUP(PyValue_Borrow(item));
        }

        /* maximum value and item are unset; set them */
        if (PyValue_IsNull(maxval)) {
            maxitem = item;
            maxval = val;
        }
        /* maximum value and item are set; update them as necessary */
        else {
            int cmp = _PyValue_RichCompareBool(val, maxval, op);
            if (cmp < 0)
                goto Fail_it_item_and_val;
            else if (cmp > 0) {
                PyValue_CLOSE(maxval);
                PyValue_CLOSE(maxitem);
                maxval = val;
                maxitem = item;
            }
            else {
                PyValue_CLOSE(item);
                PyValue_CLOSE(val);
            }
        }
    }
    if (PyValue_IsNull(maxval)) {
        assert(PyValue_IsNull(maxitem));
        if (!PyValue_IsNull(defaultval)) {
            maxitem = PyValue_DUP(defaultval);
        } else {
            PyErr_Format(PyExc_ValueError,
                         "%s() iterable argument is empty", name);
        }
    }
    else
        PyValue_CLOSE(maxval);
    Py_XDECREF(it);
    return maxitem;

Fail_it_item_and_val:
    PyValue_CLOSE(val);
Fail_it_item:
    PyValue_CLOSE(item);
Fail_it:
    PyValue_XCLOSE(maxval);
    PyValue_XCLOSE(maxitem);
    Py_XDECREF(it);
    return PyValue_NULL;
}

/* AC: cannot convert yet, waiting for *args support */
static PyValue
builtin_min(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    return min_max(args, nargs, kwnames, Py_LT);
}

PyDoc_STRVAR(min_doc,
"min(iterable, *[, default=obj, key=func]) -> value\n\
min(arg1, arg2, *args, *[, key=func]) -> value\n\
\n\
With a single iterable argument, return its smallest item. The\n\
default keyword-only argument specifies an object to return if\n\
the provided iterable is empty.\n\
With two or more positional arguments, return the smallest argument.");


/* AC: cannot convert yet, waiting for *args support */
static PyValue
builtin_max(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    return min_max(args, nargs, kwnames, Py_GT);
}

PyDoc_STRVAR(max_doc,
"max(iterable, *[, default=obj, key=func]) -> value\n\
max(arg1, arg2, *args, *[, key=func]) -> value\n\
\n\
With a single iterable argument, return its biggest item. The\n\
default keyword-only argument specifies an object to return if\n\
the provided iterable is empty.\n\
With two or more positional arguments, return the largest argument.");


/*[clinic input]
oct as builtin_oct

    integer: object
    /

Return the octal representation of an integer.

   >>> oct(342391)
   '0o1234567'
[clinic start generated code]*/

static PyValue
builtin_oct_impl(PyObject *module, PyValueBorrow integer)
/*[clinic end generated code: output=28c687399cc56e23 input=b97c377b15fedf8d]*/
{
    PyObject *integer_o = _PyValue_AsObjectAt(integer, BOX_SITE_PARSER);
    if (integer_o == NULL) {
        return PyValue_NULL;
    }
    PyObject *res = PyNumber_ToBase(integer_o, 8);
    Py_DECREF(integer_o);
    return _PyValue_FromObjectResult(res);
}


/*[clinic input]
ord as builtin_ord

    character as c: object
    /

Return the ordinal value of a character.

If the argument is a one-character string, return the Unicode code
point of that character.

If the argument is a bytes or bytearray object of length 1, return its
single byte value.
[clinic start generated code]*/

static PyValue
builtin_ord_impl(PyObject *module, PyValueBorrow c)
/*[clinic end generated code: output=8a62394b81b709be input=98d38480432e1177]*/
{
    long ord;
    Py_ssize_t size;

    if ((PyValue_IsObject(c) && PyBytes_Check(PyValue_AsKnownObjectBorrow(c)))) {
        size = PyBytes_GET_SIZE(PyValue_AsKnownObjectBorrow(c));
        if (size == 1) {
            ord = (long)((unsigned char)*PyBytes_AS_STRING(PyValue_AsKnownObjectBorrow(c)));
            return PyValue_FromInt64(ord);
        }
    }
    else if ((PyValue_IsObject(c) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(c)))) {
        size = PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(c));
        if (size == 1) {
            ord = (long)PyUnicode_READ_CHAR(PyValue_AsKnownObjectBorrow(c), 0);
            return PyValue_FromInt64(ord);
        }
    }
    else if ((PyValue_IsObject(c) && PyByteArray_Check(PyValue_AsKnownObjectBorrow(c)))) {
        /* XXX Hopefully this is temporary */
        size = PyByteArray_GET_SIZE(PyValue_AsKnownObjectBorrow(c));
        if (size == 1) {
            ord = (long)((unsigned char)*PyByteArray_AS_STRING(PyValue_AsKnownObjectBorrow(c)));
            return PyValue_FromInt64(ord);
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "ord() expected string of length 1, but " \
                     "%.200s found", PyValue_TYPE(c)->tp_name);
        return PyValue_NULL;
    }

    PyErr_Format(PyExc_TypeError,
                 "ord() expected a character, "
                 "but string of length %zd found",
                 size);
    return PyValue_NULL;
}


/*[clinic input]
@permit_long_summary
pow as builtin_pow

    base: object
    exp: object
    mod: object = None

Equivalent to base**exp with 2 arguments or base**exp % mod with 3 arguments

Some types, such as ints, are able to use a more efficient algorithm
when invoked using the three argument form.
[clinic start generated code]*/

static PyValue
builtin_pow_impl(PyObject *module, PyValueBorrow base, PyValueBorrow exp,
                 PyValueBorrow mod)
/*[clinic end generated code: output=bf73c67a928a9f3e input=0cd5c3ecc8003aec]*/
{
    return _PyValue_Power(base, exp, mod);
}

/*[clinic input]
print as builtin_print

    *objects: array
    sep: object(c_default="Py_None") = ' '
        string inserted between values, default a space.
    end: object(c_default="Py_None") = '\n'
        string appended after the last value, default a newline.
    file: object = None
        a file-like object (stream); defaults to the current sys.stdout.
    flush: bool = False
        whether to forcibly flush the stream.

Prints the values to a stream, or to sys.stdout by default.

[clinic start generated code]*/

static PyValue
builtin_print_impl(PyObject *module, const PyValue *objects,
                   Py_ssize_t objects_length, PyValueBorrow sep,
                   PyValueBorrow end, PyValueBorrow file, int flush)
/*[clinic end generated code: output=d777246f50a4900e input=ff35cb3d59ee8115]*/
{
    int i, err;
    PyObject *file_o;
    PyObject *sep_o = NULL, *end_o = NULL;

    if (PyValue_IsNone(file)) {
        file_o = PySys_GetAttr(&_Py_ID(stdout));
        if (file_o == NULL) {
            return PyValue_NULL;
        }

        /* sys.stdout may be None when FILE* stdout isn't connected */
        if (file_o == Py_None) {
            Py_DECREF(file_o);
            return PyValue_None;
        }
    }
    else {
        file_o = _PyValue_AsObjectAt(file, BOX_SITE_PARSER);
        if (file_o == NULL) {
            return PyValue_NULL;
        }
    }

    if (PyValue_IsNone(sep)) {
        sep_o = NULL;
    }
    else if (!PyValue_IsNull(sep)) {
        if (!(PyValue_IsObject(sep) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(sep)))) {
            PyErr_Format(PyExc_TypeError,
                         "sep must be None or a string, not %.200s",
                         PyValue_TYPE(sep)->tp_name);
            Py_DECREF(file_o);
            return PyValue_NULL;
        }
        sep_o = PyValue_AsKnownObjectBorrow(sep);
    }
    if (PyValue_IsNone(end)) {
        end_o = NULL;
    }
    else if (!PyValue_IsNull(end)) {
        if (!(PyValue_IsObject(end) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(end)))) {
            PyErr_Format(PyExc_TypeError,
                         "end must be None or a string, not %.200s",
                         PyValue_TYPE(end)->tp_name);
            Py_DECREF(file_o);
            return PyValue_NULL;
        }
        end_o = PyValue_AsKnownObjectBorrow(end);
    }

    for (i = 0; i < objects_length; i++) {
        if (i > 0) {
            if (sep_o == NULL) {
                err = PyFile_WriteString(" ", file_o);
            }
            else {
                err = PyFile_WriteObject(sep_o, file_o, Py_PRINT_RAW);
            }
            if (err) {
                Py_DECREF(file_o);
                return PyValue_NULL;
            }
        }
        PyObject *item_o = _PyValue_AsObjectAt(objects[i], BOX_SITE_PARSER);
        if (item_o == NULL) {
            Py_DECREF(file_o);
            return PyValue_NULL;
        }
        err = PyFile_WriteObject(item_o, file_o, Py_PRINT_RAW);
        Py_DECREF(item_o);
        if (err) {
            Py_DECREF(file_o);
            return PyValue_NULL;
        }
    }

    if (end_o == NULL) {
        err = PyFile_WriteString("\n", file_o);
    }
    else {
        err = PyFile_WriteObject(end_o, file_o, Py_PRINT_RAW);
    }
    if (err) {
        Py_DECREF(file_o);
        return PyValue_NULL;
    }

    if (flush) {
        if (_PyFile_Flush(file_o) < 0) {
            Py_DECREF(file_o);
            return PyValue_NULL;
        }
    }
    Py_DECREF(file_o);

    return PyValue_None;
}


/*[clinic input]
input as builtin_input

    prompt: object(c_default="NULL") = ""
    /

Read a string from standard input.  The trailing newline is stripped.

The prompt string, if given, is printed to standard output without a
trailing newline before reading input.

If the user hits EOF (*nix: Ctrl-D, Windows: Ctrl-Z+Return), raise
EOFError.
On *nix systems, readline is used if available.
[clinic start generated code]*/

static PyValue
builtin_input_impl(PyObject *module, PyValueBorrow prompt)
/*[clinic end generated code: output=c9c6ceb95cacde7c input=ebb939c954639427]*/
{
    PyObject *fin = NULL;
    PyObject *fout = NULL;
    PyObject *ferr = NULL;
    PyObject *prompt_o = NULL;
    PyObject *tmp;
    long fd;
    int tty;

    /* Check that stdin/out/err are intact */
    fin = PySys_GetAttr(&_Py_ID(stdin));
    if (fin == NULL) {
        goto error;
    }
    if (fin == Py_None) {
        PyErr_SetString(PyExc_RuntimeError, "lost sys.stdin");
        goto error;
    }
    fout = PySys_GetAttr(&_Py_ID(stdout));
    if (fout == NULL) {
        goto error;
    }
    if (fout == Py_None) {
        PyErr_SetString(PyExc_RuntimeError, "lost sys.stdout");
        goto error;
    }
    ferr = PySys_GetAttr(&_Py_ID(stderr));
    if (ferr == NULL) {
        goto error;
    }
    if (ferr == Py_None) {
        PyErr_SetString(PyExc_RuntimeError, "lost sys.stderr");
        goto error;
    }

    if (box_optional(prompt, &prompt_o) < 0) {
        goto error;
    }
    if (PySys_Audit("builtins.input", "O",
                    prompt_o != NULL ? prompt_o : Py_None) < 0) {
        goto error;
    }

    /* First of all, flush stderr */
    if (_PyFile_Flush(ferr) < 0) {
        PyErr_Clear();
    }

    /* We should only use (GNU) readline if Python's sys.stdin and
       sys.stdout are the same as C's stdin and stdout, because we
       need to pass it those. */
    tmp = PyObject_CallMethodNoArgs(fin, &_Py_ID(fileno));
    if (tmp == NULL) {
        PyErr_Clear();
        tty = 0;
    }
    else {
        fd = PyLong_AsLong(tmp);
        Py_DECREF(tmp);
        if (fd < 0 && PyErr_Occurred()) {
            goto error;
        }
        tty = fd == fileno(stdin) && isatty(fd);
    }
    if (tty) {
        tmp = PyObject_CallMethodNoArgs(fout, &_Py_ID(fileno));
        if (tmp == NULL) {
            PyErr_Clear();
            tty = 0;
        }
        else {
            fd = PyLong_AsLong(tmp);
            Py_DECREF(tmp);
            if (fd < 0 && PyErr_Occurred())
                goto error;
            tty = fd == fileno(stdout) && isatty(fd);
        }
    }

    /* If we're interactive, use (GNU) readline */
    if (tty) {
        PyObject *po = NULL;
        const char *promptstr;
        char *s = NULL;
        PyObject *stdin_encoding = NULL, *stdin_errors = NULL;
        PyObject *stdout_encoding = NULL, *stdout_errors = NULL;
        const char *stdin_encoding_str, *stdin_errors_str;
        PyObject *result;
        size_t len;

        /* stdin is a text stream, so it must have an encoding. */
        PyValue stdin_encoding_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(fin),
                                                        &_Py_ID(encoding));
        stdin_encoding = PyValue_IsNull(stdin_encoding_v) ? NULL
            : _PyValue_AsObjectStealAt(stdin_encoding_v, BOX_SITE_ATTRIBUTE);
        if (stdin_encoding == NULL) {
            tty = 0;
            goto _readline_errors;
        }
        PyValue stdin_errors_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(fin),
                                                      &_Py_ID(errors));
        stdin_errors = PyValue_IsNull(stdin_errors_v) ? NULL
            : _PyValue_AsObjectStealAt(stdin_errors_v, BOX_SITE_ATTRIBUTE);
        if (stdin_errors == NULL) {
            tty = 0;
            goto _readline_errors;
        }
        if (!PyUnicode_Check(stdin_encoding) ||
            !PyUnicode_Check(stdin_errors))
        {
            tty = 0;
            goto _readline_errors;
        }
        stdin_encoding_str = PyUnicode_AsUTF8(stdin_encoding);
        if (stdin_encoding_str == NULL) {
            goto _readline_errors;
        }
        stdin_errors_str = PyUnicode_AsUTF8(stdin_errors);
        if (stdin_errors_str == NULL) {
            goto _readline_errors;
        }
        if (_PyFile_Flush(fout) < 0) {
            PyErr_Clear();
        }
        if (!PyValue_IsNull(prompt)) {
            /* We have a prompt, encode it as stdout would */
            const char *stdout_encoding_str, *stdout_errors_str;
            PyObject *stringpo;
            PyValue stdout_encoding_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(fout),
                                                             &_Py_ID(encoding));
            stdout_encoding = PyValue_IsNull(stdout_encoding_v) ? NULL
                : _PyValue_AsObjectStealAt(stdout_encoding_v, BOX_SITE_ATTRIBUTE);
            if (stdout_encoding == NULL) {
                tty = 0;
                goto _readline_errors;
            }
            PyValue stdout_errors_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(fout),
                                                           &_Py_ID(errors));
            stdout_errors = PyValue_IsNull(stdout_errors_v) ? NULL
                : _PyValue_AsObjectStealAt(stdout_errors_v, BOX_SITE_ATTRIBUTE);
            if (stdout_errors == NULL) {
                tty = 0;
                goto _readline_errors;
            }
            if (!PyUnicode_Check(stdout_encoding) ||
                !PyUnicode_Check(stdout_errors))
            {
                tty = 0;
                goto _readline_errors;
            }
            stdout_encoding_str = PyUnicode_AsUTF8(stdout_encoding);
            if (stdout_encoding_str == NULL) {
                goto _readline_errors;
            }
            stdout_errors_str = PyUnicode_AsUTF8(stdout_errors);
            if (stdout_errors_str == NULL) {
                goto _readline_errors;
            }
            stringpo = _PyValue_Str(PyValue_FromObjectBorrow(prompt_o));
            if (stringpo == NULL)
                goto _readline_errors;
            po = PyUnicode_AsEncodedString(stringpo,
                stdout_encoding_str, stdout_errors_str);
            Py_CLEAR(stdout_encoding);
            Py_CLEAR(stdout_errors);
            Py_CLEAR(stringpo);
            if (po == NULL)
                goto _readline_errors;
            assert(PyBytes_Check(po));
            promptstr = PyBytes_AS_STRING(po);
            if ((Py_ssize_t)strlen(promptstr) != PyBytes_GET_SIZE(po)) {
                PyErr_SetString(PyExc_ValueError,
                        "input: prompt string cannot contain null characters");
                goto _readline_errors;
            }
        }
        else {
            po = NULL;
            promptstr = "";
        }
        s = PyOS_Readline(stdin, stdout, promptstr);
        if (s == NULL) {
            PyErr_CheckSignals();
            if (!PyErr_Occurred())
                PyErr_SetNone(PyExc_KeyboardInterrupt);
            goto _readline_errors;
        }

        len = strlen(s);
        if (len == 0) {
            PyErr_SetNone(PyExc_EOFError);
            result = NULL;
        }
        else {
            if (len > PY_SSIZE_T_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                                "input: input too long");
                result = NULL;
            }
            else {
                len--;   /* strip trailing '\n' */
                if (len != 0 && s[len-1] == '\r')
                    len--;   /* strip trailing '\r' */
                result = PyUnicode_Decode(s, len, stdin_encoding_str,
                                                  stdin_errors_str);
            }
        }
        Py_DECREF(stdin_encoding);
        Py_DECREF(stdin_errors);
        Py_XDECREF(po);
        PyMem_Free(s);

        if (result != NULL) {
            if (PySys_Audit("builtins.input/result", "O", result) < 0) {
                goto error;
            }
        }

        Py_DECREF(fin);
        Py_DECREF(fout);
        Py_DECREF(ferr);
        Py_XDECREF(prompt_o);
        return _PyValue_FromObjectResult(result);

    _readline_errors:
        Py_XDECREF(stdin_encoding);
        Py_XDECREF(stdout_encoding);
        Py_XDECREF(stdin_errors);
        Py_XDECREF(stdout_errors);
        Py_XDECREF(po);
        if (tty)
            goto error;

        PyErr_Clear();
    }

    /* Fallback if we're not interactive */
    if (!PyValue_IsNull(prompt)) {
        if (PyFile_WriteObject(prompt_o, fout, Py_PRINT_RAW) != 0)
            goto error;
    }
    if (_PyFile_Flush(fout) < 0) {
        PyErr_Clear();
    }
    tmp = PyFile_GetLine(fin, -1);
    Py_DECREF(fin);
    Py_DECREF(fout);
    Py_DECREF(ferr);
    Py_XDECREF(prompt_o);
    return _PyValue_FromObjectResult(tmp);

error:
    Py_XDECREF(fin);
    Py_XDECREF(fout);
    Py_XDECREF(ferr);
    Py_XDECREF(prompt_o);
    return PyValue_NULL;
}


/*[clinic input]
repr as builtin_repr

    obj: object
    /

Return the canonical string representation of the object.

For many object types, including most builtins, eval(repr(obj)) == obj.
[clinic start generated code]*/

static PyValue
builtin_repr_impl(PyObject *module, PyValueBorrow obj)
/*[clinic end generated code: output=51ec443c875b57f9 input=1c9e6d66d3e3be04]*/
{
    return _PyValue_FromObjectResult(_PyValue_Repr(obj));
}


/*[clinic input]
round as builtin_round

    number: object
    ndigits: object = None

Round a number to a given precision in decimal digits.

The return value is an integer if ndigits is omitted or None.
Otherwise the return value has the same type as the number.  ndigits
may be negative.
[clinic start generated code]*/

static PyValue
builtin_round_impl(PyObject *module, PyValueBorrow number,
                   PyValueBorrow ndigits)
/*[clinic end generated code: output=3bf5f0d0ded28bd5 input=bdcb7c67bf4a4320]*/
{
    PyObject *result;
    PyObject *number_o = _PyValue_AsObjectAt(number, BOX_SITE_PARSER);
    if (number_o == NULL) {
        return PyValue_NULL;
    }
    if (PyValue_IsNone(ndigits)) {
        result = _PyObject_MaybeCallSpecialNoArgs(number_o, &_Py_ID(__round__));
    }
    else {
        PyObject *ndigits_o = _PyValue_AsObjectAt(ndigits, BOX_SITE_PARSER);
        if (ndigits_o == NULL) {
            Py_DECREF(number_o);
            return PyValue_NULL;
        }
        result = _PyObject_MaybeCallSpecialOneArg(number_o, &_Py_ID(__round__),
                                                  ndigits_o);
        Py_DECREF(ndigits_o);
    }
    Py_DECREF(number_o);
    if (result == NULL && !PyErr_Occurred()) {
        PyErr_Format(PyExc_TypeError,
                     "type %.100s doesn't define __round__ method",
                     PyValue_TYPE(number)->tp_name);
    }
    return _PyValue_FromObjectResult(result);
}


/*AC: we need to keep the kwds dict intact to easily call into the
 * list.sort method, which isn't currently supported in AC. So we just use
 * the initially generated signature with a custom implementation.
 */
/* [disabled clinic input]
sorted as builtin_sorted

    iterable as seq: object
    key as keyfunc: object = None
    reverse: object = False

Return a new list containing all items from the iterable in ascending order.

A custom key function can be supplied to customize the sort order, and
the reverse flag can be set to request the result in descending order.
[end disabled clinic input]*/

PyDoc_STRVAR(builtin_sorted__doc__,
"sorted($module, iterable, /, *, key=None, reverse=False)\n"
"--\n"
"\n"
"Return a new list containing all items from the iterable in ascending order.\n"
"\n"
"A custom key function can be supplied to customize the sort order, and the\n"
"reverse flag can be set to request the result in descending order.");

#define BUILTIN_SORTED_METHODDEF    \
    {"sorted", _PyCFunction_CAST(builtin_sorted), METH_FASTCALL | METH_KEYWORDS, builtin_sorted__doc__},

static PyObject *
builtin_sorted(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *newlist, *v, *seq, *callable;

    /* Keyword arguments are passed through list.sort() which will check
       them. */
    if (!_PyArg_UnpackStack(args, nargs, "sorted", 1, 1, &seq))
        return NULL;

    newlist = _PyValue_SequenceList(PyValue_FromObjectBorrow(seq));
    if (newlist == NULL)
        return NULL;

    PyValue callable_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(newlist),
                                              &_Py_ID(sort));
    callable = PyValue_IsNull(callable_v) ? NULL
        : _PyValue_AsObjectStealAt(callable_v, BOX_SITE_ATTRIBUTE);
    if (callable == NULL) {
        Py_DECREF(newlist);
        return NULL;
    }

    assert(nargs >= 1);
    v = PyObject_Vectorcall(callable, args + 1, nargs - 1, kwnames);
    Py_DECREF(callable);
    if (v == NULL) {
        Py_DECREF(newlist);
        return NULL;
    }
    Py_DECREF(v);
    return newlist;
}


/* AC: cannot convert yet, as needs PEP 457 group support in inspect */
/*[clinic input]
vars as builtin_vars

    object as v: object = NULL
    /

Show vars.

Without arguments, equivalent to locals().
With an argument, equivalent to object.__dict__.
[clinic start generated code]*/

static PyValue
builtin_vars_impl(PyObject *module, PyValueBorrow v)
/*[clinic end generated code: output=115ce377346f83de input=577550318240f012]*/
{
    PyObject *d;

    if (PyValue_IsNull(v)) {
        if (_PyEval_GetFrame() != NULL) {
            d = _PyEval_GetFrameLocals();
        }
        else {
            PyThreadState *tstate = _PyThreadState_GET();
            d = _PyEval_GetGlobalsFromRunningMain(tstate);
            if (d == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    d = _PyEval_GetFrameLocals();
                    assert(_PyErr_Occurred(tstate));
                }
            }
            else {
                Py_INCREF(d);
            }
        }
    }
    else {
        _PyStatusValue r = _PyValue_GetOptionalAttr(v, &_Py_ID(__dict__));
        if (r.status < 0) {
            return PyValue_NULL;
        }
        if (r.status == 0) {
            PyErr_SetString(PyExc_TypeError,
                "vars() argument must have __dict__ attribute");
            return PyValue_NULL;
        }
        return r.value;
    }
    return _PyValue_FromObjectResult(d);
}


/* Improved Kahan–Babuška algorithm by Arnold Neumaier
   Neumaier, A. (1974), Rundungsfehleranalyse einiger Verfahren
   zur Summation endlicher Summen.  Z. angew. Math. Mech.,
   54: 39-51. https://doi.org/10.1002/zamm.19740540106
   https://en.wikipedia.org/wiki/Kahan_summation_algorithm#Further_enhancements
 */

typedef struct {
    double hi;     /* high-order bits for a running sum */
    double lo;     /* a running compensation for lost low-order bits */
} CompensatedSum;

static inline CompensatedSum
cs_from_double(double x)
{
    return (CompensatedSum) {x};
}

static inline CompensatedSum
cs_add(CompensatedSum total, double x)
{
    double t = total.hi + x;
    if (fabs(total.hi) >= fabs(x)) {
        total.lo += (total.hi - t) + x;
    }
    else {
        total.lo += (x - t) + total.hi;
    }
    return (CompensatedSum) {t, total.lo};
}

static inline double
cs_to_double(CompensatedSum total)
{
    /* Avoid losing the sign on a negative result,
       and don't let adding the compensation convert
       an infinite or overflowed sum to a NaN. */
    if (total.lo && isfinite(total.lo)) {
        return total.hi + total.lo;
    }
    return total.hi;
}

/* The helpers of sum(): items are values, and an inline int or float is read
   without a box. */

/* An exact int (or, with allow_bool, a bool) as a Py_ssize_t, unless it does
   not fit. */
static int
sum_exact_int(PyValueBorrow v, int allow_bool, Py_ssize_t *out)
{
    if (PyValue_IsInlineInt(v)) {
        *out = (Py_ssize_t)PyValue_InlineInt64(v);
        return 1;
    }
    if (!PyValue_IsObject(v)) {
        return 0;
    }
    PyObject *o = PyValue_AsKnownObjectBorrow(v);
    if (!(PyLong_CheckExact(o) || (allow_bool && PyBool_Check(o)))) {
        return 0;
    }
    if (_PyLong_IsCompact((PyLongObject *)o)) {
        *out = _PyLong_CompactValue((PyLongObject *)o);
        return 1;
    }
    int overflow = 0;
    Py_ssize_t b = PyLong_AsLongAndOverflow(o, &overflow);
    if (overflow != 0) {
        return 0;
    }
    *out = b;
    return 1;
}

/* An exact float: inline, or a float object that is not a subclass. */
static int
sum_exact_float(PyValueBorrow v)
{
    return PyValue_IsInlineFloat(v)
        || (PyValue_IsObject(v)
            && PyFloat_CheckExact(PyValue_AsKnownObjectBorrow(v)));
}

static double
sum_float_value(PyValueBorrow v)
{
    return PyValue_IsInlineFloat(v) ? PyValue_InlineFloat(v)
        : PyFloat_AS_DOUBLE(PyValue_AsKnownObjectBorrow(v));
}

/* Any float, subclasses included (PyFloat_Check()). */
static double
sum_any_float_value(PyValueBorrow v)
{
    return sum_float_value(v);
}

/* An int of any size, or a subclass of int (PyLong_Check()). */
static int
sum_is_int(PyValueBorrow v)
{
    return PyValue_IsInlineInt(v)
        || (PyValue_IsObject(v)
            && PyLong_Check(PyValue_AsKnownObjectBorrow(v)));
}

/* PyLong_AsDouble() of sum_is_int(); -1.0 with an exception for an overflow. */
static double
sum_int_as_double(PyValueBorrow v)
{
    if (PyValue_IsInlineInt(v)) {
        return (double)PyValue_InlineInt64(v);
    }
    return PyLong_AsDouble(PyValue_AsKnownObjectBorrow(v));
}

/*[clinic input]
@permit_long_summary
sum as builtin_sum

    iterable: object
    /
    start: object(c_default="NULL") = 0

Return the sum of a 'start' value (default: 0) plus an iterable of numbers

When the iterable is empty, return the start value.
This function is intended specifically for use with numeric values and
may reject non-numeric types.
[clinic start generated code]*/

static PyValue
builtin_sum_impl(PyObject *module, PyValueBorrow iterable,
                 PyValueBorrow start)
/*[clinic end generated code: output=b3f2dbd80f1410e3 input=d464d57815196b73]*/
{
    PyObject *iter = _PyValue_GetIter(iterable);
    if (iter == NULL)
        return PyValue_NULL;

    PyValue result;   /* owned */
    if (PyValue_IsNull(start)) {
        result = PyValue_FromInt64(0);
    } else {
        /* reject string values for 'start' parameter */
        if (PyValue_IsObject(start)
            && PyUnicode_Check(PyValue_AsKnownObjectBorrow(start))) {
            PyErr_SetString(PyExc_TypeError,
                "sum() can't sum strings [use ''.join(seq) instead]");
            Py_DECREF(iter);
            return PyValue_NULL;
        }
        if (PyValue_IsObject(start)
            && PyBytes_Check(PyValue_AsKnownObjectBorrow(start))) {
            PyErr_SetString(PyExc_TypeError,
                "sum() can't sum bytes [use b''.join(seq) instead]");
            Py_DECREF(iter);
            return PyValue_NULL;
        }
        if (PyValue_IsObject(start)
            && PyByteArray_Check(PyValue_AsKnownObjectBorrow(start))) {
            PyErr_SetString(PyExc_TypeError,
                "sum() can't sum bytearray [use b''.join(seq) instead]");
            Py_DECREF(iter);
            return PyValue_NULL;
        }
        result = PyValue_DUP(start);
    }

#ifndef SLOW_SUM
    /* Fast addition by keeping temporary sums in C instead of new Python objects.
       Assumes all inputs are the same type.  If the assumption fails, default
       to the more general routine.  The items are values: an inline int or
       float is never boxed.
    */
    Py_ssize_t i_result;
    if (sum_exact_int(PyValue_Borrow(result), 0, &i_result)) {
        /* If this already overflowed, don't even enter the loop. */
        PyValue_CLOSE(result);
        result = PyValue_NULL;
        while (PyValue_IsNull(result)) {
            _PyStatusValue step = _PyValue_IterNext(iter);
            if (step.status == PYGEN_ERROR) {
                Py_DECREF(iter);
                return PyValue_NULL;
            }
            if (step.status == PYGEN_RETURN) {
                Py_DECREF(iter);
                return PyValue_FromInt64(i_result);
            }
            PyValue item = step.value;
            Py_ssize_t b;
            /* Single digits are common, fast, and cannot overflow on unpacking. */
            if (sum_exact_int(PyValue_Borrow(item), 1, &b)
                && (i_result >= 0 ? (b <= PY_SSIZE_T_MAX - i_result)
                                  : (b >= PY_SSIZE_T_MIN - i_result)))
            {
                i_result += b;
                PyValue_CLOSE(item);
                continue;
            }
            /* Either overflowed or is not an int. Restore real objects and process normally */
            PyValue partial = PyValue_FromInt64(i_result);
            if (PyValue_IsNull(partial)) {
                PyValue_CLOSE(item);
                Py_DECREF(iter);
                return PyValue_NULL;
            }
            result = _PyValue_Add(partial, item);
            PyValue_CLOSE(partial);
            PyValue_CLOSE(item);
            if (PyValue_IsNull(result)) {
                Py_DECREF(iter);
                return PyValue_NULL;
            }
        }
    }

    if (sum_exact_float(PyValue_Borrow(result))) {
        CompensatedSum re_sum = cs_from_double(sum_float_value(PyValue_Borrow(result)));
        PyValue_CLOSE(result);
        result = PyValue_NULL;
        while (PyValue_IsNull(result)) {
            _PyStatusValue step = _PyValue_IterNext(iter);
            if (step.status == PYGEN_ERROR) {
                Py_DECREF(iter);
                return PyValue_NULL;
            }
            if (step.status == PYGEN_RETURN) {
                Py_DECREF(iter);
                return PyValue_FromDouble(cs_to_double(re_sum));
            }
            PyValue item = step.value;
            if (sum_exact_float(PyValue_Borrow(item))) {
                re_sum = cs_add(re_sum, sum_float_value(PyValue_Borrow(item)));
                PyValue_CLOSE(item);
                continue;
            }
            if (sum_is_int(PyValue_Borrow(item))) {
                double value = sum_int_as_double(PyValue_Borrow(item));
                if (value != -1.0 || !PyErr_Occurred()) {
                    re_sum = cs_add(re_sum, value);
                    PyValue_CLOSE(item);
                    continue;
                }
                PyValue_CLOSE(item);
                Py_DECREF(iter);
                return PyValue_NULL;
            }
            PyValue partial = PyValue_FromDouble(cs_to_double(re_sum));
            result = _PyValue_Add(partial, item);
            PyValue_CLOSE(partial);
            PyValue_CLOSE(item);
            if (PyValue_IsNull(result)) {
                Py_DECREF(iter);
                return PyValue_NULL;
            }
        }
    }

    if (PyValue_IsObject(result)
        && PyComplex_CheckExact(PyValue_AsKnownObjectBorrow(result))) {
        Py_complex z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(result));
        CompensatedSum re_sum = cs_from_double(z.real);
        CompensatedSum im_sum = cs_from_double(z.imag);
        PyValue_CLOSE(result);
        result = PyValue_NULL;
        while (PyValue_IsNull(result)) {
            _PyStatusValue step = _PyValue_IterNext(iter);
            if (step.status == PYGEN_ERROR) {
                Py_DECREF(iter);
                return PyValue_NULL;
            }
            if (step.status == PYGEN_RETURN) {
                Py_DECREF(iter);
                return _PyValue_FromObjectResult(
                    PyComplex_FromDoubles(cs_to_double(re_sum),
                                          cs_to_double(im_sum)));
            }
            PyValue item = step.value;
            if (PyValue_IsObject(item)
                && PyComplex_CheckExact(PyValue_AsKnownObjectBorrow(item))) {
                z = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(item));
                re_sum = cs_add(re_sum, z.real);
                im_sum = cs_add(im_sum, z.imag);
                PyValue_CLOSE(item);
                continue;
            }
            if (sum_is_int(PyValue_Borrow(item))) {
                double value = sum_int_as_double(PyValue_Borrow(item));
                if (value != -1.0 || !PyErr_Occurred()) {
                    re_sum = cs_add(re_sum, value);
                    PyValue_CLOSE(item);
                    continue;
                }
                PyValue_CLOSE(item);
                Py_DECREF(iter);
                return PyValue_NULL;
            }
            if (PyValue_IsInlineFloat(item)
                || (PyValue_IsObject(item)
                    && PyFloat_Check(PyValue_AsKnownObjectBorrow(item)))) {
                re_sum = cs_add(re_sum, sum_any_float_value(PyValue_Borrow(item)));
                PyValue_CLOSE(item);
                continue;
            }
            PyObject *partial = PyComplex_FromDoubles(cs_to_double(re_sum),
                                                      cs_to_double(im_sum));
            if (partial == NULL) {
                PyValue_CLOSE(item);
                Py_DECREF(iter);
                return PyValue_NULL;
            }
            result = _PyValue_Add(PyValue_FromObjectBorrow(partial), item);
            Py_DECREF(partial);
            PyValue_CLOSE(item);
            if (PyValue_IsNull(result)) {
                Py_DECREF(iter);
                return PyValue_NULL;
            }
        }
    }
#endif

    for(;;) {
        _PyStatusValue step = _PyValue_IterNext(iter);
        if (step.status == PYGEN_ERROR) {
            PyValue_CLOSE(result);
            result = PyValue_NULL;
            break;
        }
        if (step.status == PYGEN_RETURN) {
            break;
        }
        PyValue item = step.value;
        /* It's tempting to use PyNumber_InPlaceAdd instead of
           PyNumber_Add here, to avoid quadratic running time
           when doing 'sum(list_of_lists, [])'.  However, this
           would produce a change in behaviour: a snippet like

             empty = []
             sum([[x] for x in range(10)], empty)

           would change the value of empty. In fact, using
           in-place addition rather that binary addition for
           any of the steps introduces subtle behavior changes:

           https://bugs.python.org/issue18305 */
        PyValue temp = _PyValue_Add(result, item);
        PyValue_CLOSE(result);
        PyValue_CLOSE(item);
        result = temp;
        if (PyValue_IsNull(result))
            break;
    }
    Py_DECREF(iter);
    return result;
}


/*[clinic input]
@permit_long_summary
isinstance as builtin_isinstance

    obj: object
    class_or_tuple: object
    /

Return whether an object is an instance of a class or of a subclass thereof.

A tuple, as in ``isinstance(x, (A, B, ...))``, may be given as the
target to check against.  This is equivalent to ``isinstance(x, A) or
isinstance(x, B) or ...`` etc.
[clinic start generated code]*/

static PyValue
builtin_isinstance_impl(PyObject *module, PyValueBorrow obj,
                        PyValueBorrow class_or_tuple)
/*[clinic end generated code: output=9c12944ffb21cef7 input=5d74d547df498f38]*/
{
    int retval;

    retval = _PyEval_IsInstanceValue(obj, class_or_tuple);
    if (retval < 0)
        return PyValue_NULL;
    return PyValue_FromBool(retval);
}


/*[clinic input]
issubclass as builtin_issubclass

    cls: object
    class_or_tuple: object
    /

Return whether 'cls' is derived from another class or is the same class.

A tuple, as in ``issubclass(x, (A, B, ...))``, may be given as the
target to check against.  This is equivalent to ``issubclass(x, A) or
issubclass(x, B) or ...``.
[clinic start generated code]*/

static PyValue
builtin_issubclass_impl(PyObject *module, PyValueBorrow cls,
                        PyValueBorrow class_or_tuple)
/*[clinic end generated code: output=ae66cc1a219f2dd1 input=a91ce96345a6705d]*/
{
    int retval;

    PyObject *cls_o = _PyValue_AsObjectAt(cls, BOX_SITE_PARSER);
    if (cls_o == NULL) {
        return PyValue_NULL;
    }
    PyObject *class_or_tuple_o = _PyValue_AsObjectAt(class_or_tuple, BOX_SITE_PARSER);
    if (class_or_tuple_o == NULL) {
        Py_DECREF(cls_o);
        return PyValue_NULL;
    }
    retval = PyObject_IsSubclass(cls_o, class_or_tuple_o);
    Py_DECREF(cls_o);
    Py_DECREF(class_or_tuple_o);
    if (retval < 0)
        return PyValue_NULL;
    return PyValue_FromBool(retval);
}

typedef struct {
    PyObject_HEAD
    Py_ssize_t tuplesize;
    PyObject *ittuple;     /* tuple of iterators */
    PyObject *result;
    int strict;
} zipobject;

#define _zipobject_CAST(op)     ((zipobject *)(op))

static PyObject *
zip_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    zipobject *lz;
    Py_ssize_t i;
    PyObject *ittuple;  /* tuple of iterators */
    PyObject *result;
    Py_ssize_t tuplesize;
    int strict = 0;

    if (kwds) {
        PyObject *empty = PyTuple_New(0);
        if (empty == NULL) {
            return NULL;
        }
        static char *kwlist[] = {"strict", NULL};
        int parsed = PyArg_ParseTupleAndKeywords(
                empty, kwds, "|$p:zip", kwlist, &strict);
        Py_DECREF(empty);
        if (!parsed) {
            return NULL;
        }
    }

    /* args must be a tuple */
    assert(PyTuple_Check(args));
    tuplesize = PyTuple_GET_SIZE(args);

    /* obtain iterators */
    ittuple = PyTuple_New(tuplesize);
    if (ittuple == NULL)
        return NULL;
    for (i=0; i < tuplesize; ++i) {
        PyValue item_v = PyTuple_GetValue(args, i);
        PyObject *item = PyValue_IsNull(item_v) ? NULL
            : _PyValue_AsObjectAt(item_v, BOX_SITE_TUPLE);
        PyValue_XCLOSE(item_v);
        if (item == NULL) {
            Py_DECREF(ittuple);
            return NULL;
        }
        PyObject *it = _PyValue_GetIter(PyValue_FromObjectBorrow(item));
        Py_DECREF(item);
        if (it == NULL) {
            Py_DECREF(ittuple);
            return NULL;
        }
        PyTuple_SET_VALUE(ittuple, i, PyValue_PrepareOwned(_PyObject_CAST(it)));
    }

    /* create a result holder */
    result = PyTuple_New(tuplesize);
    if (result == NULL) {
        Py_DECREF(ittuple);
        return NULL;
    }
    for (i=0 ; i < tuplesize ; i++) {
        PyTuple_SET_VALUE(result, i, PyValue_DUP(_PyValue_FromObjectKnown(Py_None)));
    }

    /* create zipobject structure */
    lz = (zipobject *)type->tp_alloc(type, 0);
    if (lz == NULL) {
        Py_DECREF(ittuple);
        Py_DECREF(result);
        return NULL;
    }
    lz->ittuple = ittuple;
    lz->tuplesize = tuplesize;
    lz->result = result;
    lz->strict = strict;

    return (PyObject *)lz;
}

static void
zip_dealloc(PyObject *self)
{
    zipobject *lz = _zipobject_CAST(self);
    PyObject_GC_UnTrack(lz);
    Py_XDECREF(lz->ittuple);
    Py_XDECREF(lz->result);
    Py_TYPE(lz)->tp_free(lz);
}

static int
zip_traverse(PyObject *self, visitproc visit, void *arg)
{
    zipobject *lz = _zipobject_CAST(self);
    Py_VISIT(lz->ittuple);
    Py_VISIT(lz->result);
    return 0;
}

/* The value iternext slot of zip (R16): without strict, the end of an
   inner iterator, with its return value, or its error is zip's. The result
   tuple holds the items' words. */
static _PyStatusValue
zip_next_value(PyObject *self)
{
    zipobject *lz = _zipobject_CAST(self);

    Py_ssize_t i;
    Py_ssize_t tuplesize = lz->tuplesize;
    PyObject *result = lz->result;
    PyObject *it;
    _PyStatusValue item;

    if (tuplesize == 0)
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};

    if (_PyObject_IsUniquelyReferenced(result)) {
        Py_INCREF(result);
        for (i=0 ; i < tuplesize ; i++) {
            it = _PyTuple_GET_OBJECT(lz->ittuple, i);  // iterators only
            item = _PyValue_IterStep(it);
            if (item.status != PYGEN_NEXT) {
                Py_DECREF(result);
                if (lz->strict) {
                    goto check;
                }
                return item;
            }
            _PyTuple_ReplaceTakeValue(result, i, item.value);
        }
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        _PyTuple_Recycle(result);
    } else {
        result = PyTuple_New(tuplesize);
        if (result == NULL)
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
        for (i=0 ; i < tuplesize ; i++) {
            it = _PyTuple_GET_OBJECT(lz->ittuple, i);  // iterators only
            item = _PyValue_IterStep(it);
            if (item.status != PYGEN_NEXT) {
                Py_DECREF(result);
                if (lz->strict) {
                    goto check;
                }
                return item;
            }
            PyTuple_SET_VALUE(result, i, item.value);
        }
    }
    return (_PyStatusValue){PYGEN_NEXT, PyValue_FromObjectSteal(result)};
check:
    if (item.status == PYGEN_ERROR) {
        if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
            // next() on argument i raised an exception (not StopIteration)
            return item;
        }
        PyErr_Clear();
    }
    else {
        // Strict mode drops the return value of argument i
        PyValue_XCLOSE(item.value);
    }
    if (i) {
        // ValueError: zip() argument 2 is shorter than argument 1
        // ValueError: zip() argument 3 is shorter than arguments 1-2
        const char* plural = i == 1 ? " " : "s 1-";
        PyErr_Format(PyExc_ValueError,
                     "zip() argument %zd is shorter than argument%s%zd",
                     i + 1, plural, i);
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    for (i = 1; i < tuplesize; i++) {
        it = _PyTuple_GET_OBJECT(lz->ittuple, i);  // iterators only
        item = _PyValue_IterStep(it);
        if (item.status == PYGEN_NEXT) {
            PyValue_CLOSE(item.value);
            const char* plural = i == 1 ? " " : "s 1-";
            PyErr_Format(PyExc_ValueError,
                         "zip() argument %zd is longer than argument%s%zd",
                         i + 1, plural, i);
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
        }
        if (item.status == PYGEN_ERROR) {
            if (!PyErr_ExceptionMatches(PyExc_StopIteration)) {
                // next() on argument i raised an exception (not StopIteration)
                return item;
            }
            PyErr_Clear();
        }
        else {
            PyValue_XCLOSE(item.value);
        }
        // Argument i is exhausted. So far so good...
    }
    // All arguments are exhausted. Success!
    return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
}

_Py_VALUE_THUNK_ITERNEXT(zip_next, zip_next_value)

static PyObject *
zip_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    zipobject *lz = _zipobject_CAST(self);
    /* Just recreate the zip with the internal iterator tuple */
    if (lz->strict) {
        return _PyTuple_PackValues(3, (PyValueBorrow[]){_PyValue_FromObjectKnown(_PyObject_CAST(Py_TYPE(lz))), PyValue_FromObjectBorrow(lz->ittuple), _PyValue_FromObjectKnown(Py_True)});
    }
    return _PyTuple_PackValues(2, (PyValueBorrow[]){PyValue_FromObjectBorrow((PyObject *)Py_TYPE(lz)), PyValue_FromObjectBorrow(lz->ittuple)});
}

static PyObject *
zip_setstate(PyObject *self, PyObject *state)
{
    int strict = _PyValue_IsTrue(PyValue_FromObjectBorrow(state));
    if (strict < 0) {
        return NULL;
    }
    zipobject *lz = _zipobject_CAST(self);
    lz->strict = strict;
    Py_RETURN_NONE;
}

static PyMethodDef zip_methods[] = {
    {"__reduce__", zip_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", zip_setstate, METH_O, setstate_doc},
    {NULL}  /* sentinel */
};

PyDoc_STRVAR(zip_doc,
"zip(*iterables, strict=False)\n\
--\n\
\n\
The zip object yields n-length tuples, where n is the number of\n\
iterables passed as positional arguments to zip().  The i-th element\n\
in every tuple comes from the i-th iterable argument to zip().  This\n\
continues until the shortest argument is exhausted.\n\
\n\
If strict is true and one of the arguments is exhausted before the\n\
others, raise a ValueError.\n\
\n\
   >>> list(zip('abcdefg', range(3), range(4)))\n\
   [('a', 0, 0), ('b', 1, 1), ('c', 2, 2)]");

PyTypeObject PyZip_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "zip",                              /* tp_name */
    sizeof(zipobject),                  /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    zip_dealloc,                        /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    zip_doc,                            /* tp_doc */
    zip_traverse,                       /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    PyObject_SelfIter,                  /* tp_iter */
    zip_next,                           /* tp_iternext */
    zip_methods,                        /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    zip_new,                            /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
    .tp_value = { .iternext = _Py_VALUE_SLOT_INIT(iternext, zip_next_value) },
};


static PyMethodDef builtin_methods[] = {
    {"__build_class__", _PyCFunction_CAST(builtin___build_class__),
     METH_FASTCALL | METH_KEYWORDS, build_class_doc},
    BUILTIN___IMPORT___METHODDEF
    BUILTIN___LAZY_IMPORT___METHODDEF
    BUILTIN_ABS_METHODDEF
    BUILTIN_ALL_METHODDEF
    BUILTIN_ANY_METHODDEF
    BUILTIN_ASCII_METHODDEF
    BUILTIN_BIN_METHODDEF
    {"breakpoint", _PyCFunction_CAST(builtin_breakpoint), METH_FASTCALL | METH_KEYWORDS, breakpoint_doc},
    BUILTIN_CALLABLE_METHODDEF
    BUILTIN_CHR_METHODDEF
    BUILTIN_COMPILE_METHODDEF
    BUILTIN_DELATTR_METHODDEF
    BUILTIN_DIR_METHODDEF
    BUILTIN_DIVMOD_METHODDEF
    BUILTIN_EVAL_METHODDEF
    BUILTIN_EXEC_METHODDEF
    BUILTIN_FORMAT_METHODDEF
    {"getattr", _PyCFunction_CAST(builtin_getattr), METH_FASTCALL, getattr_doc},
    BUILTIN_GLOBALS_METHODDEF
    BUILTIN_HASATTR_METHODDEF
    BUILTIN_HASH_METHODDEF
    BUILTIN_HEX_METHODDEF
    BUILTIN_ID_METHODDEF
    BUILTIN_INPUT_METHODDEF
    BUILTIN_ISINSTANCE_METHODDEF
    BUILTIN_ISSUBCLASS_METHODDEF
    BUILTIN_ITER_METHODDEF
    BUILTIN_LEN_METHODDEF
    BUILTIN_LOCALS_METHODDEF
    {"max", _PyCFunction_CAST(builtin_max), METH_VALUE | METH_KEYWORDS, max_doc},
    {"min", _PyCFunction_CAST(builtin_min), METH_VALUE | METH_KEYWORDS, min_doc},
    {"next", _PyCFunction_CAST(builtin_next), METH_FASTCALL, next_doc},
    BUILTIN_OCT_METHODDEF
    BUILTIN_ORD_METHODDEF
    BUILTIN_POW_METHODDEF
    BUILTIN_PRINT_METHODDEF
    BUILTIN_REPR_METHODDEF
    BUILTIN_ROUND_METHODDEF
    BUILTIN_SETATTR_METHODDEF
    BUILTIN_SORTED_METHODDEF
    BUILTIN_SUM_METHODDEF
    BUILTIN_VARS_METHODDEF
    {NULL,              NULL},
};

PyDoc_STRVAR(builtin_doc,
"Built-in functions, types, exceptions, and other objects.\n\
\n\
This module provides direct access to all 'built-in'\n\
identifiers of Python; for example, builtins.len is\n\
the full name for the built-in function len().\n\
\n\
This module is not normally accessed explicitly by most\n\
applications, but can be useful in modules that provide\n\
objects with the same name as a built-in value, but in\n\
which the built-in of that name is also needed.");

static struct PyModuleDef builtinsmodule = {
    PyModuleDef_HEAD_INIT,
    "builtins",
    builtin_doc,
    -1, /* multiple "initialization" just copies the module dict. */
    builtin_methods,
    NULL,
    NULL,
    NULL,
    NULL
};


/* Builtins implemented in Python.

   Lib/_pybuiltins.py is frozen into the interpreter as a bootstrap module
   (see Tools/build/freeze_modules.py), so it can be imported here before
   the import system exists.  The names in its __all__ are copied into the
   builtins dict. */

int
_PyBuiltin_InitPythonFunctions(PyObject *dict)
{
    int imported = PyImport_ImportFrozenModule("_pybuiltins");
    if (imported < 0) {
        return -1;
    }
    if (imported == 0) {
        PyErr_SetString(PyExc_ImportError,
                        "frozen module _pybuiltins not found");
        return -1;
    }
    PyObject *mod = PyImport_AddModuleRef("_pybuiltins");
    if (mod == NULL) {
        return -1;
    }

    int rc = -1;
    PyValue all_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(mod),
                                     &_Py_ID(__all__));
    PyObject *all = PyValue_IsNull(all_v) ? NULL
        : _PyValue_AsObjectStealAt(all_v, BOX_SITE_ATTRIBUTE);
    if (all == NULL) {
        goto done;
    }
    Py_ssize_t n = PyList_Size(all);
    if (n < 0) {
        goto done;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue name_v = PyList_GetValue(all, i);
        PyObject *name = PyValue_IsNull(name_v) ? NULL
            : _PyValue_AsObjectAt(name_v, BOX_SITE_LIST);
        PyValue_XCLOSE(name_v);
        if (name == NULL) {
            goto done;
        }
        PyValue func = _PyValue_GetAttr(PyValue_FromObjectBorrow(mod), name);
        if (PyValue_IsNull(func)) {
            Py_DECREF(name);
            goto done;
        }
        int r = PyDict_SetValue(dict, PyValue_FromObjectBorrow(name), func);
        PyValue_CLOSE(func);
        Py_DECREF(name);
        if (r < 0) {
            goto done;
        }
    }
    rc = 0;

done:
    Py_XDECREF(all);
    Py_DECREF(mod);
    return rc;
}

PyObject *
_PyBuiltin_Init(PyInterpreterState *interp)
{
    PyObject *mod, *dict, *debug;

    const PyConfig *config = _PyInterpreterState_GetConfig(interp);

    mod = _PyModule_CreateInitialized(&builtinsmodule, PYTHON_API_VERSION);
    if (mod == NULL)
        return NULL;
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(mod, Py_MOD_GIL_NOT_USED);
#endif
    dict = PyModule_GetDict(mod);

#ifdef Py_TRACE_REFS
    /* "builtins" exposes a number of statically allocated objects
     * that, before this code was added in 2.3, never showed up in
     * the list of "all objects" maintained by Py_TRACE_REFS.  As a
     * result, programs leaking references to None and False (etc)
     * couldn't be diagnosed by examining sys.getobjects(0).
     */
#define ADD_TO_ALL(OBJECT) _Py_AddToAllObjects((PyObject *)(OBJECT))
#else
#define ADD_TO_ALL(OBJECT) (void)0
#endif

#define SETBUILTIN(NAME, OBJECT) \
    if (PyDict_SetValueString(dict, NAME, PyValue_FromObjectBorrow((PyObject *)OBJECT)) < 0)       \
        return NULL;                                                    \
    ADD_TO_ALL(OBJECT)

    SETBUILTIN("None",                  Py_None);
    SETBUILTIN("Ellipsis",              Py_Ellipsis);
    SETBUILTIN("NotImplemented",        Py_NotImplemented);
    SETBUILTIN("False",                 Py_False);
    SETBUILTIN("True",                  Py_True);
    SETBUILTIN("bool",                  &PyBool_Type);
    SETBUILTIN("memoryview",            &PyMemoryView_Type);
    SETBUILTIN("bytearray",             &PyByteArray_Type);
    SETBUILTIN("bytes",                 &PyBytes_Type);
    SETBUILTIN("classmethod",           &PyClassMethod_Type);
    SETBUILTIN("complex",               &PyComplex_Type);
    SETBUILTIN("dict",                  &PyDict_Type);
    SETBUILTIN("enumerate",             &PyEnum_Type);
    SETBUILTIN("filter",                &PyFilter_Type);
    SETBUILTIN("float",                 &PyFloat_Type);
    SETBUILTIN("frozendict",            &PyFrozenDict_Type);
    SETBUILTIN("frozenset",             &PyFrozenSet_Type);
    SETBUILTIN("property",              &PyProperty_Type);
    SETBUILTIN("int",                   &PyLong_Type);
    SETBUILTIN("list",                  &PyList_Type);
    SETBUILTIN("map",                   &PyMap_Type);
    SETBUILTIN("object",                &PyBaseObject_Type);
    SETBUILTIN("range",                 &PyRange_Type);
    SETBUILTIN("reversed",              &PyReversed_Type);
    SETBUILTIN("sentinel",              &PySentinel_Type);
    SETBUILTIN("set",                   &PySet_Type);
    SETBUILTIN("slice",                 &PySlice_Type);
    SETBUILTIN("staticmethod",          &PyStaticMethod_Type);
    SETBUILTIN("str",                   &PyUnicode_Type);
    SETBUILTIN("super",                 &PySuper_Type);
    SETBUILTIN("tuple",                 &PyTuple_Type);
    SETBUILTIN("type",                  &PyType_Type);
    SETBUILTIN("zip",                   &PyZip_Type);
    debug = PyBool_FromLong(config->optimization_level == 0);
    if (PyDict_SetValueString(dict, "__debug__", PyValue_FromObjectBorrow(debug)) < 0) {
        Py_DECREF(debug);
        return NULL;
    }
    Py_DECREF(debug);

    return mod;
#undef ADD_TO_ALL
#undef SETBUILTIN
}
