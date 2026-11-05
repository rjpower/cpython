
#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_compile.h"       // _PyCompile_GetUnaryIntrinsicName
#include "pycore_dict.h"          // PyDict_SetValue()
#include "pycore_function.h"      // _Py_set_function_type_params()
#include "pycore_genobject.h"     // _PyAsyncGenValueWrapperNew
#include "pycore_interpframe.h"   // _PyFrame_GetLocals()
#include "pycore_intrinsics.h"    // INTRINSIC_PRINT
#include "pycore_list.h"          // _PyList_AsTupleAndClear()
#include "pycore_object.h"        // _PyObject_IsUniquelyReferenced()
#include "pycore_setobject.h"     // _PySet_Freeze()
#include "pycore_pyerrors.h"      // _PyErr_SetString()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_typevarobject.h" // _Py_make_typevar()
#include "pycore_unicodeobject.h" // _PyUnicode_FromASCII()


/******** Unary functions ********/

static PyObject *
no_intrinsic1(PyThreadState* tstate, PyObject *unused)
{
    _PyErr_SetString(tstate, PyExc_SystemError, "invalid intrinsic function");
    return NULL;
}

static PyObject *
print_expr(PyThreadState* Py_UNUSED(ignored), PyObject *value)
{
    PyObject *hook = PySys_GetAttr(&_Py_ID(displayhook));
    if (hook == NULL) {
        return NULL;
    }
    PyObject *res = PyObject_CallOneArg(hook, value);
    Py_DECREF(hook);
    return res;
}

static int
import_all_from(PyThreadState *tstate, PyObject *locals, PyObject *v)
{
    PyObject *all, *dict, *name, *value;
    int skip_leading_underscores = 0;
    int pos, err;

    if (PyObject_GetOptionalAttr(v, &_Py_ID(__all__), &all) < 0) {
        return -1; /* Unexpected error */
    }
    if (all == NULL) {
        if (PyObject_GetOptionalAttr(v, &_Py_ID(__dict__), &dict) < 0) {
            return -1;
        }
        if (dict == NULL) {
            _PyErr_SetString(tstate, PyExc_ImportError,
                    "from-import-* object has no __dict__ and no __all__");
            return -1;
        }
        all = PyMapping_Keys(dict);
        Py_DECREF(dict);
        if (all == NULL)
            return -1;
        skip_leading_underscores = 1;
    }

    for (pos = 0, err = 0; ; pos++) {
        name = PySequence_GetItem(all, pos);
        if (name == NULL) {
            if (!_PyErr_ExceptionMatches(tstate, PyExc_IndexError)) {
                err = -1;
            }
            else {
                _PyErr_Clear(tstate);
            }
            break;
        }
        if (!PyUnicode_Check(name)) {
            PyObject *modname = PyObject_GetAttr(v, &_Py_ID(__name__));
            if (modname == NULL) {
                Py_DECREF(name);
                err = -1;
                break;
            }
            if (!PyUnicode_Check(modname)) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "module __name__ must be a string, not %.100s",
                              Py_TYPE(modname)->tp_name);
            }
            else {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "%s in %U.%s must be str, not %.100s",
                              skip_leading_underscores ? "Key" : "Item",
                              modname,
                              skip_leading_underscores ? "__dict__" : "__all__",
                              Py_TYPE(name)->tp_name);
            }
            Py_DECREF(modname);
            Py_DECREF(name);
            err = -1;
            break;
        }
        if (skip_leading_underscores) {
            if (PyUnicode_READ_CHAR(name, 0) == '_') {
                Py_DECREF(name);
                continue;
            }
        }
        value = PyObject_GetAttr(v, name);
        if (value == NULL)
            err = -1;
        else if (PyDict_CheckExact(locals))
            err = PyDict_SetValue(locals, PyValue_FromObjectBorrow(name), PyValue_FromObjectBorrow(value));
        else
            err = PyObject_SetItem(locals, name, value);
        Py_DECREF(name);
        Py_XDECREF(value);
        if (err < 0)
            break;
    }
    Py_DECREF(all);
    return err;
}

static PyObject *
import_star(PyThreadState* tstate, PyObject *from)
{
    _PyInterpreterFrame *frame = tstate->current_frame;

    PyObject *locals = _PyFrame_GetLocals(frame);
    if (locals == NULL) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                            "no locals found during 'import *'");
        return NULL;
    }
    int err = import_all_from(tstate, locals, from);
    Py_DECREF(locals);
    if (err < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
stopiteration_error(PyThreadState* tstate, PyObject *exc)
{
    _PyInterpreterFrame *frame = tstate->current_frame;
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    assert(PyExceptionInstance_Check(exc));
    const char *msg = NULL;
    if (PyErr_GivenExceptionMatches(exc, PyExc_StopIteration)) {
        msg = "generator raised StopIteration";
        if (_PyFrame_GetCode(frame)->co_flags & CO_ASYNC_GENERATOR) {
            msg = "async generator raised StopIteration";
        }
        else if (_PyFrame_GetCode(frame)->co_flags & CO_COROUTINE) {
            msg = "coroutine raised StopIteration";
        }
    }
    else if ((_PyFrame_GetCode(frame)->co_flags & CO_ASYNC_GENERATOR) &&
            PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration))
    {
        /* code in `gen` raised a StopAsyncIteration error:
        raise a RuntimeError.
        */
        msg = "async generator raised StopAsyncIteration";
    }
    if (msg != NULL) {
        PyObject *message = _PyUnicode_FromASCII(msg, strlen(msg));
        if (message == NULL) {
            return NULL;
        }
        PyObject *error = PyObject_CallOneArg(PyExc_RuntimeError, message);
        if (error == NULL) {
            Py_DECREF(message);
            return NULL;
        }
        assert(PyExceptionInstance_Check(error));
        PyException_SetCause(error, Py_NewRef(exc));
        // Steal exc reference, rather than Py_NewRef+Py_DECREF
        PyException_SetContext(error, Py_NewRef(exc));
        Py_DECREF(message);
        return error;
    }
    return Py_NewRef(exc);
}

static PyObject *
unary_pos(PyThreadState* unused, PyObject *value)
{
    return PyNumber_Positive(value);
}

static PyObject *
list_to_tuple(PyThreadState* unused, PyObject *v)
{
    /* INTRINSIC_LIST_TO_TUPLE is only emitted by the compiler for a
       freshly-built, uniquely-referenced temporary list, so steal its items
       into the tuple instead of copying them. */
    assert(PyList_CheckExact(v));
    assert(_PyObject_IsUniquelyReferenced(v));
    return _PyList_AsTupleAndClear((PyListObject *)v);
}

static PyObject *
make_typevar(PyThreadState* Py_UNUSED(ignored), PyObject *v)
{
    assert(PyUnicode_Check(v));
    return _Py_make_typevar(v, NULL, NULL);
}

static PyObject *
make_frozenset(PyThreadState* Py_UNUSED(ignored), PyObject *set)
{
    assert(PySet_CheckExact(set));
    assert(_PyObject_IsUniquelyReferenced(set));
    return _PySet_Freeze(set);
}


#define INTRINSIC_FUNC_ENTRY(N, F) \
    [N] = {F, #N},
#define INTRINSIC_FUNC_NAMED(N, NAME, F) \
    [N] = {F, NAME},

const intrinsic_func1_info
_PyIntrinsics_UnaryFunctions[] = {
    INTRINSIC_FUNC_ENTRY(INTRINSIC_1_INVALID, no_intrinsic1)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_PRINT, print_expr)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_IMPORT_STAR, import_star)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_STOPITERATION_ERROR, stopiteration_error)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_ASYNC_GEN_WRAP, _PyAsyncGenValueWrapperNew)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_UNARY_POSITIVE, unary_pos)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_LIST_TO_TUPLE, list_to_tuple)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_TYPEVAR, make_typevar)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_PARAMSPEC, _Py_make_paramspec)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_TYPEVARTUPLE, _Py_make_typevartuple)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_SUBSCRIPT_GENERIC, _Py_subscript_generic)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_TYPEALIAS, _Py_make_typealias)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_BUILD_FROZENSET, make_frozenset)
};


/******** Binary functions ********/

static PyObject *
no_intrinsic2(PyThreadState* tstate, PyObject *unused1, PyObject *unused2)
{
    _PyErr_SetString(tstate, PyExc_SystemError, "invalid intrinsic function");
    return NULL;
}

static PyObject *
prep_reraise_star(PyThreadState* unused, PyObject *orig, PyObject *excs)
{
    assert(PyList_Check(excs));
    return _PyExc_PrepReraiseStar(orig, excs);
}

static PyObject *
make_typevar_with_bound(PyThreadState* Py_UNUSED(ignored), PyObject *name,
                        PyObject *evaluate_bound)
{
    assert(PyUnicode_Check(name));
    return _Py_make_typevar(name, evaluate_bound, NULL);
}

static PyObject *
make_typevar_with_constraints(PyThreadState* Py_UNUSED(ignored), PyObject *name,
                              PyObject *evaluate_constraints)
{
    assert(PyUnicode_Check(name));
    return _Py_make_typevar(name, NULL, evaluate_constraints);
}

static PyObject *
add_conditional_annotation(PyThreadState* tstate, PyObject *conditional_annotations,
                           PyObject *index)
{
    // gh-154902: user code can rebind __conditional_annotations__ to any object
    if (!PySet_CheckExact(conditional_annotations)) {
        _PyErr_Format(tstate, PyExc_TypeError,
                      "__conditional_annotations__ must be a set, not %T",
                      conditional_annotations);
        return NULL;
    }
    if (PySet_AddValue(conditional_annotations, PyValue_FromObjectBorrow(index)) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
match_class_isinstance(PyThreadState* tstate, PyObject *subject, PyObject *type)
{
    /* Fast path for class patterns with no sub-patterns, e.g. `case C():`
       Equivalent to the isinstance check performed by _PyEval_MatchClass,
       including the same TypeError when the pattern does not refer to a
       class. */
    if (!PyType_Check(type)) {
        _PyErr_SetString(tstate, PyExc_TypeError,
                         "class pattern must refer to a class");
        return NULL;
    }
    int res = PyObject_IsInstance(subject, type);
    if (res < 0) {
        return NULL;
    }
    return res ? Py_True : Py_False;
}

/* COMPARE_OP and CONTAINS_OP at a site without a feedback entry, and the
 * names that no instruction can hold. */
#define COMPARE(NAME, OP) \
static PyObject * \
compare_##NAME(PyThreadState* Py_UNUSED(ignored), PyObject *a, PyObject *b) \
{ \
    return PyObject_RichCompare(a, b, OP); \
} \
static PyObject * \
compare_##NAME##_bool(PyThreadState* Py_UNUSED(ignored), PyObject *a, PyObject *b) \
{ \
    PyObject *res = PyObject_RichCompare(a, b, OP); \
    if (res == NULL) { \
        return NULL; \
    } \
    int truth = PyObject_IsTrue(res); \
    Py_DECREF(res); \
    if (truth < 0) { \
        return NULL; \
    } \
    return truth ? Py_True : Py_False; \
}

COMPARE(lt, Py_LT)
COMPARE(le, Py_LE)
COMPARE(eq, Py_EQ)
COMPARE(ne, Py_NE)
COMPARE(gt, Py_GT)
COMPARE(ge, Py_GE)
#undef COMPARE

static PyObject *
contains_in(PyThreadState* Py_UNUSED(ignored), PyObject *item, PyObject *container)
{
    int res = PySequence_Contains(container, item);
    if (res < 0) {
        return NULL;
    }
    return res ? Py_True : Py_False;
}

static PyObject *
contains_not_in(PyThreadState* Py_UNUSED(ignored), PyObject *item, PyObject *container)
{
    int res = PySequence_Contains(container, item);
    if (res < 0) {
        return NULL;
    }
    return res ? Py_False : Py_True;
}

static PyObject *
get_attr(PyThreadState* Py_UNUSED(ignored), PyObject *owner, PyObject *name)
{
    return PyObject_GetAttr(owner, name);
}

/* value is NULL for a deletion, the tuple holds (owner, name) */
static PyObject *
set_attr(PyThreadState* Py_UNUSED(ignored), PyObject *value, PyObject *target)
{
    assert(PyTuple_CheckExact(target) && PyTuple_GET_SIZE(target) == 2);
    // The elements may be inline values, which get a box
    PyValue owner_value = PyTuple_GetValue(target, 0);
    PyValue name_value = PyTuple_GetValue(target, 1);
    PyObject *owner = PyValue_AsObject(owner_value);
    PyObject *name = PyValue_AsObject(name_value);
    PyValue_CLOSE(owner_value);
    PyValue_CLOSE(name_value);
    int err = owner == NULL || name == NULL
              || PyObject_SetAttr(owner, name, value) < 0;
    Py_XDECREF(owner);
    Py_XDECREF(name);
    if (err) {
        return NULL;
    }
    Py_RETURN_NONE;
}

const intrinsic_func2_info
_PyIntrinsics_BinaryFunctions[] = {
    INTRINSIC_FUNC_ENTRY(INTRINSIC_2_INVALID, no_intrinsic2)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_PREP_RERAISE_STAR, prep_reraise_star)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_TYPEVAR_WITH_BOUND, make_typevar_with_bound)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_TYPEVAR_WITH_CONSTRAINTS, make_typevar_with_constraints)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_SET_FUNCTION_TYPE_PARAMS, _Py_set_function_type_params)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_SET_TYPEPARAM_DEFAULT, _Py_set_typeparam_default)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_ADD_CONDITIONAL_ANNOTATION, add_conditional_annotation)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_MATCH_CLASS_ISINSTANCE, match_class_isinstance)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 0, "INTRINSIC_COMPARE_LT", compare_lt)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 1, "INTRINSIC_COMPARE_LT_BOOL", compare_lt_bool)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 2, "INTRINSIC_COMPARE_LE", compare_le)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 3, "INTRINSIC_COMPARE_LE_BOOL", compare_le_bool)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 4, "INTRINSIC_COMPARE_EQ", compare_eq)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 5, "INTRINSIC_COMPARE_EQ_BOOL", compare_eq_bool)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 6, "INTRINSIC_COMPARE_NE", compare_ne)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 7, "INTRINSIC_COMPARE_NE_BOOL", compare_ne_bool)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 8, "INTRINSIC_COMPARE_GT", compare_gt)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 9, "INTRINSIC_COMPARE_GT_BOOL", compare_gt_bool)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 10, "INTRINSIC_COMPARE_GE", compare_ge)
    INTRINSIC_FUNC_NAMED(INTRINSIC_COMPARE_OP + 11, "INTRINSIC_COMPARE_GE_BOOL", compare_ge_bool)
    INTRINSIC_FUNC_NAMED(INTRINSIC_CONTAINS_OP, "INTRINSIC_CONTAINS_IN", contains_in)
    INTRINSIC_FUNC_NAMED(INTRINSIC_CONTAINS_OP + 1, "INTRINSIC_CONTAINS_NOT_IN", contains_not_in)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_GETATTR, get_attr)
    INTRINSIC_FUNC_ENTRY(INTRINSIC_SETATTR, set_attr)
};

#undef INTRINSIC_FUNC_ENTRY

PyObject*
_PyCompile_GetUnaryIntrinsicName(int index)
{
    if (index < 0 || index > MAX_INTRINSIC_1) {
        return NULL;
    }
    return PyUnicode_FromString(_PyIntrinsics_UnaryFunctions[index].name);
}

PyObject*
_PyCompile_GetBinaryIntrinsicName(int index)
{
    if (index < 0 || index > MAX_INTRINSIC_2) {
        return NULL;
    }
    return PyUnicode_FromString(_PyIntrinsics_BinaryFunctions[index].name);
}
