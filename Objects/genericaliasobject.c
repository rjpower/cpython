// types.GenericAlias -- used to represent e.g. list[int].

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_object.h"
#include "pycore_tuple.h"         // PyTuple_GET_VALUE()
#include "pycore_typevarobject.h" // _Py_typing_type_repr
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()
#include "pycore_unionobject.h"   // _Py_union_type_or, _PyGenericAlias_Check
#include "pycore_value.h"
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()


#include <stdbool.h>

typedef struct {
    PyObject_HEAD
    PyObject *origin;
    PyObject *args;
    PyObject *parameters;
    PyObject *weakreflist;
    // Whether we're a starred type, e.g. *tuple[int].
    bool starred;
    vectorcallfunc vectorcall;
} gaobject;

typedef struct {
    PyObject_HEAD
    PyObject *obj;  /* Set to NULL when iterator is exhausted */
} gaiterobject;

static void
ga_dealloc(PyObject *self)
{
    gaobject *alias = (gaobject *)self;

    _PyObject_GC_UNTRACK(self);
    FT_CLEAR_WEAKREFS(self, alias->weakreflist);
    Py_XDECREF(alias->origin);
    Py_XDECREF(alias->args);
    Py_XDECREF(alias->parameters);
    Py_TYPE(self)->tp_free(self);
}

static int
ga_traverse(PyObject *self, visitproc visit, void *arg)
{
    gaobject *alias = (gaobject *)self;
    Py_VISIT(alias->origin);
    Py_VISIT(alias->args);
    Py_VISIT(alias->parameters);
    return 0;
}

static int
ga_repr_items_list(PyUnicodeWriter *writer, PyObject *p)
{
    assert(PyList_CheckExact(p));

    Py_ssize_t len = PyList_GET_SIZE(p);

    if (PyUnicodeWriter_WriteChar(writer, '[') < 0) {
        return -1;
    }

    for (Py_ssize_t i = 0; i < len; i++) {
        if (i > 0) {
            if (PyUnicodeWriter_WriteASCII(writer, ", ", 2) < 0) {
                return -1;
            }
        }
        PyObject *item = PyList_GetItemRef(p, i);
        if (item == NULL) {
            return -1;  // list can be mutated in a callback
        }
        if (_Py_typing_type_repr(writer, item) < 0) {
            Py_DECREF(item);
            return -1;
        }
        Py_DECREF(item);
    }

    if (PyUnicodeWriter_WriteChar(writer, ']') < 0) {
        return -1;
    }

    return 0;
}

static PyObject *
ga_repr(PyObject *self)
{
    gaobject *alias = (gaobject *)self;
    Py_ssize_t len = PyTuple_GET_SIZE(alias->args);

    // Estimation based on the shortest format: "int[int, int, int]"
    Py_ssize_t estimate = (len <= PY_SSIZE_T_MAX / 5) ? len * 5 : len;
    estimate = 3 + 1 + estimate + 1;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(estimate);
    if (writer == NULL) {
        return NULL;
    }

    if (alias->starred) {
        if (PyUnicodeWriter_WriteChar(writer, '*') < 0) {
            goto error;
        }
    }
    if (_Py_typing_type_repr(writer, alias->origin) < 0) {
        goto error;
    }
    if (PyUnicodeWriter_WriteChar(writer, '[') < 0) {
        goto error;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        if (i > 0) {
            if (PyUnicodeWriter_WriteASCII(writer, ", ", 2) < 0) {
                goto error;
            }
        }
        PyObject *p = PyTuple_GetItemRef(alias->args, i);
        if (p == NULL) {
            goto error;
        }
        int status;
        if (PyList_CheckExact(p)) {
            // Looks like we are working with ParamSpec's list of type args:
            status = ga_repr_items_list(writer, p);
        }
        else {
            status = _Py_typing_type_repr(writer, p);
        }
        Py_DECREF(p);
        if (status < 0) {
            goto error;
        }
    }
    if (len == 0) {
        // for something like tuple[()] we should print a "()"
        if (PyUnicodeWriter_WriteASCII(writer, "()", 2) < 0) {
            goto error;
        }
    }
    if (PyUnicodeWriter_WriteChar(writer, ']') < 0) {
        goto error;
    }
    return PyUnicodeWriter_Finish(writer);

error:
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

// Index of item in self[:len], or -1 if not found (self is a tuple)
static Py_ssize_t
tuple_index(PyObject *self, Py_ssize_t len, PyObject *item)
{
    for (Py_ssize_t i = 0; i < len; i++) {
        if (PyValue_Is(PyTuple_GET_VALUE(self, i),
                       PyValue_FromPyObjectBorrow(item))) {
            return i;
        }
    }
    return -1;
}

static int
tuple_add(PyObject *self, Py_ssize_t len, PyObject *item)
{
    if (tuple_index(self, len, item) < 0) {
        PyTuple_SET_ITEM(self, len, Py_NewRef(item));
        return 1;
    }
    return 0;
}

static Py_ssize_t
tuple_extend(PyObject **dst, Py_ssize_t dstindex,
             PyObject *source)
{
    PyObject *source_owner = NULL;
    PyValue value = PyValue_NULL;

    if (PyErr_Occurred()) {
        goto failure;
    }
    source_owner = Py_NewRef(source);
    if (PyErr_Occurred()) {
        goto failure;
    }
    Py_ssize_t count = PyTuple_GET_SIZE(source_owner);
    if (_PyTuple_Resize(dst, PyTuple_GET_SIZE(*dst) + count - 1) != 0 ||
            PyErr_Occurred()) {
        goto failure;
    }
    assert(dstindex + count <= PyTuple_GET_SIZE(*dst));
    for (Py_ssize_t i = 0; i < count; ++i) {
        value = PyTuple_GetValue(source_owner, i);
        if (PyValue_IsNull(value) || PyErr_Occurred()) {
            goto failure;
        }
        PyTuple_SET_VALUE(*dst, dstindex + i, value);
        value = PyValue_NULL;
        if (PyErr_Occurred()) {
            goto failure;
        }
    }

    Py_CLEAR(source_owner);
    if (PyErr_Occurred()) {
        goto failure;
    }
    return dstindex + count;

failure:
    PyValue_CLEAR(value);
    Py_CLEAR(source_owner);
    Py_CLEAR(*dst);
    return -1;
}

PyObject *
_Py_make_parameters(PyObject *args)
{
    PyObject *input_owner = NULL;
    PyObject *tuple_args = NULL;
    PyObject *parameters = NULL;
    PyObject *t = NULL;
    PyObject *subparams = NULL;
    PyObject *t2 = NULL;
    PyObject *result = NULL;
    int entered = 0;
    int completed = 0;
    Py_ssize_t nargs = 0;
    Py_ssize_t len = 0;
    Py_ssize_t iparam = 0;

    if (PyErr_Occurred()) {
        return NULL;
    }
    assert(PyTuple_Check(args) || PyList_Check(args));
    input_owner = Py_NewRef(args);
    if (Py_EnterRecursiveCall(" in __parameter__ calculation")) {
        goto done;
    }
    entered = 1;
    if (PyErr_Occurred()) {
        goto done;
    }

    const bool is_args_list = PyList_Check(input_owner);
    if (is_args_list) {
        tuple_args = PySequence_Tuple(input_owner);
        if (tuple_args == NULL) {
            goto done;
        }
    }
    PyObject *source = is_args_list ? tuple_args : input_owner;
    nargs = PyTuple_GET_SIZE(source);
    len = nargs;
    parameters = PyTuple_New(len);
    if (parameters == NULL || PyErr_Occurred()) {
        goto done;
    }
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        t = PyTuple_GetItemRef(source, iarg);
        if (t == NULL) {
            goto done;
        }
        // We don't want __parameters__ descriptor of a bare Python class.
        if (PyType_Check(t)) {
            Py_CLEAR(t);
            if (PyErr_Occurred()) {
                goto done;
            }
            continue;
        }
        int rc = PyObject_HasAttrWithError(t, &_Py_ID(__typing_subst__));
        if (rc < 0) {
            goto done;
        }
        if (rc) {
            int delta = tuple_add(parameters, iparam, t);
            if (PyErr_Occurred()) {
                goto done;
            }
            iparam += delta;
        }
        else {
            rc = PyObject_GetOptionalAttr(t, &_Py_ID(__parameters__),
                                          &subparams);
            if (rc < 0) {
                goto done;
            }
            if (!subparams && (PyTuple_Check(t) || PyList_Check(t))) {
                // Recursively call _Py_make_parameters for lists/tuples and
                // add the results to the current parameters.
                subparams = _Py_make_parameters(t);
                if (subparams == NULL) {
                    goto done;
                }
            }
            if (subparams && PyTuple_Check(subparams)) {
                Py_ssize_t len2 = PyTuple_GET_SIZE(subparams);
                Py_ssize_t needed = len2 - 1 - (iarg - iparam);
                if (needed > 0) {
                    len += needed;
                    if (_PyTuple_Resize(&parameters, len) < 0 || PyErr_Occurred()) {
                        Py_CLEAR(subparams);
                        Py_CLEAR(tuple_args);
                        goto done;
                    }
                }
                for (Py_ssize_t j = 0; j < len2; j++) {
                    t2 = PyTuple_GetItemRef(subparams, j);
                    if (t2 == NULL) {
                        goto done;
                    }
                    int delta = tuple_add(parameters, iparam, t2);
                    if (PyErr_Occurred()) {
                        goto done;
                    }
                    iparam += delta;
                    Py_CLEAR(t2);
                    if (PyErr_Occurred()) {
                        goto done;
                    }
                }
            }
            Py_CLEAR(subparams);
            if (PyErr_Occurred()) {
                goto done;
            }
        }
        Py_CLEAR(t);
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    if (iparam < len) {
        if (_PyTuple_Resize(&parameters, iparam) < 0 || PyErr_Occurred()) {
            goto done;
        }
    }
    result = parameters;
    parameters = NULL;
    completed = 1;

done:
    Py_CLEAR(t2);
    Py_CLEAR(t);
    Py_CLEAR(subparams);
    Py_CLEAR(parameters);
    Py_CLEAR(tuple_args);
    Py_CLEAR(input_owner);
    if (entered) {
        Py_LeaveRecursiveCall();
    }
    if (!completed || PyErr_Occurred()) {
        Py_CLEAR(result);
        return NULL;
    }
    return result;
}

/* If obj is a generic alias, substitute type variables params
   with substitutions argitems.  For example, if obj is list[T],
   params is (T, S), and argitems is (str, int), return list[str].
   If obj doesn't have a __parameters__ attribute or that's not
   a non-empty tuple, return a new reference to obj. */
static PyObject *
subs_tvars(PyObject *obj, PyObject *params, PyObject *argitems,
          int argitems_is_tuple, Py_ssize_t nargs)
{
    PyObject *obj_owner = NULL;
    PyObject *params_owner = NULL;
    PyObject *items_owner = NULL;
    PyObject *subparams = NULL;
    PyObject *subargs = NULL;
    PyObject *arg = NULL;
    PyObject *param = NULL;
    PyObject *replacement = NULL;
    PyObject *result = NULL;
    int completed = 0;

    if (PyErr_Occurred()) {
        goto done;
    }

    obj_owner = Py_NewRef(obj);
    params_owner = Py_NewRef(params);
    items_owner = Py_NewRef(argitems);

    if (PyObject_GetOptionalAttr(obj_owner, &_Py_ID(__parameters__),
                                 &subparams) < 0 || PyErr_Occurred()) {
        goto done;
    }
    if (subparams != NULL && PyTuple_Check(subparams) &&
        PyTuple_GET_SIZE(subparams) != 0)
    {
        Py_ssize_t nparams = PyTuple_GET_SIZE(params_owner);
        Py_ssize_t nsubargs = PyTuple_GET_SIZE(subparams);
        subargs = PyTuple_New(nsubargs);
        if (subargs == NULL) {
            goto done;
        }
        Py_ssize_t j = 0;
        for (Py_ssize_t i = 0; i < nsubargs; ++i) {
            arg = PyTuple_GetItemRef(subparams, i);
            if (arg == NULL) {
                goto done;
            }
            Py_ssize_t iparam = tuple_index(params_owner, nparams, arg);
            if (iparam >= 0) {
                param = PyTuple_GetItemRef(params_owner, iparam);
                if (param == NULL || PyErr_Occurred()) {
                    goto done;
                }
                replacement = argitems_is_tuple
                    ? PyTuple_GetItemRef(items_owner, iparam)
                    : Py_NewRef(items_owner);
                if (replacement == NULL || PyErr_Occurred()) {
                    goto done;
                }
                if (Py_TYPE(param)->tp_iter && PyTuple_Check(replacement)) {
                    j = tuple_extend(&subargs, j, replacement);
                    if (j < 0) {
                        goto done;
                    }
                    Py_CLEAR(replacement);
                    if (PyErr_Occurred()) {
                        goto done;
                    }
                    Py_CLEAR(param);
                    if (PyErr_Occurred()) {
                        goto done;
                    }
                    Py_CLEAR(arg);
                    if (PyErr_Occurred()) {
                        goto done;
                    }
                    continue;
                }
                PyTuple_SET_ITEM(subargs, j, Py_NewRef(replacement));
                if (PyErr_Occurred()) {
                    goto done;
                }
                j++;
                Py_CLEAR(replacement);
                if (PyErr_Occurred()) {
                    goto done;
                }
                Py_CLEAR(param);
                if (PyErr_Occurred()) {
                    goto done;
                }
                Py_CLEAR(arg);
                if (PyErr_Occurred()) {
                    goto done;
                }
            }
            else {
                PyTuple_SET_ITEM(subargs, j, Py_NewRef(arg));
                if (PyErr_Occurred()) {
                    goto done;
                }
                j++;
                Py_CLEAR(arg);
                if (PyErr_Occurred()) {
                    goto done;
                }
            }
        }
        assert(j == PyTuple_GET_SIZE(subargs));

        result = PyObject_GetItem(obj_owner, subargs);
        if (result == NULL || PyErr_Occurred()) {
            goto done;
        }
        Py_CLEAR(subargs);
        if (PyErr_Occurred()) {
            goto done;
        }
    }
    else {
        result = Py_NewRef(obj_owner);
    }
    completed = 1;

done:
    Py_CLEAR(replacement);
    Py_CLEAR(param);
    Py_CLEAR(arg);
    Py_CLEAR(subargs);
    Py_CLEAR(subparams);
    Py_CLEAR(items_owner);
    Py_CLEAR(params_owner);
    Py_CLEAR(obj_owner);
    if (!completed || PyErr_Occurred()) {
        Py_CLEAR(result);
        return NULL;
    }
    return result;
}

static int
_is_unpacked_typevartuple(PyObject *arg)
{
    PyObject *tmp;
    if (PyType_Check(arg)) { // TODO: Add test
        return 0;
    }
    int res = PyObject_GetOptionalAttr(arg, &_Py_ID(__typing_is_unpacked_typevartuple__), &tmp);
    if (res > 0) {
        res = PyObject_IsTrue(tmp);
        Py_DECREF(tmp);
    }
    return res;
}

static PyObject *
_unpacked_tuple_args(PyObject *arg)
{
    PyObject *result;
    assert(!PyType_Check(arg));
    // Fast path
    if (_PyGenericAlias_Check(arg) &&
            ((gaobject *)arg)->starred &&
            ((gaobject *)arg)->origin == (PyObject *)&PyTuple_Type)
    {
        result = ((gaobject *)arg)->args;
        return Py_NewRef(result);
    }

    if (PyObject_GetOptionalAttr(arg, &_Py_ID(__typing_unpacked_tuple_args__), &result) > 0) {
        if (result == Py_None) {
            Py_DECREF(result);
            return NULL;
        }
        return result;
    }
    return NULL;
}

static PyObject *
_unpack_args(PyObject *item)
{
    PyObject *newargs = PyList_New(0);
    if (newargs == NULL) {
        return NULL;
    }
    int is_tuple = PyTuple_Check(item);
    Py_ssize_t nitems = is_tuple ? PyTuple_GET_SIZE(item) : 1;
    for (Py_ssize_t i = 0; i < nitems; i++) {
        PyObject *arg = is_tuple ? PyTuple_GetItemRef(item, i) : Py_NewRef(item);
        if (arg == NULL) {
            Py_DECREF(newargs);
            return NULL;
        }
        if (!PyType_Check(arg)) {
            PyObject *subargs = _unpacked_tuple_args(arg);
            if (subargs != NULL &&
                PyTuple_Check(subargs) &&
                !(PyTuple_GET_SIZE(subargs) &&
                  PyValue_Is(PyTuple_GET_VALUE(subargs, PyTuple_GET_SIZE(subargs)-1),
                             PyValue_FromPyObjectBorrow(Py_Ellipsis))))
            {
                int rc = PyList_SetSlice(newargs, PY_SSIZE_T_MAX, PY_SSIZE_T_MAX, subargs);
                Py_DECREF(subargs);
                Py_DECREF(arg);
                if (rc < 0) {
                    Py_DECREF(newargs);
                    return NULL;
                }
                continue;
            }
            Py_XDECREF(subargs);
            if (PyErr_Occurred()) {
                Py_DECREF(arg);
                Py_DECREF(newargs);
                return NULL;
            }
        }
        int rc = PyList_Append(newargs, arg);
        Py_DECREF(arg);
        if (rc < 0) {
            Py_DECREF(newargs);
            return NULL;
        }
    }
    Py_SETREF(newargs, PySequence_Tuple(newargs));
    return newargs;
}

PyObject *
_Py_subs_parameters(PyObject *self, PyObject *args, PyObject *parameters, PyObject *item_arg)
{
    PyObject *self_owner = NULL;
    PyObject *args_owner = NULL;
    PyObject *params_owner = NULL;
    PyObject *input_owner = NULL;
    PyObject *item = NULL;
    PyObject *param = NULL;
    PyObject *prepare = NULL;
    PyObject *tmp = NULL;
    PyObject *old_item = NULL;
    PyObject *tuple_args = NULL;
    PyObject *newargs = NULL;
    PyObject *arg = NULL;
    PyObject *subst = NULL;
    PyObject *subargs = NULL;
    PyObject *subargs_list = NULL;
    PyObject *original = NULL;
    PyObject *replacement = NULL;
    PyObject *result = NULL;
    PyObject *source = NULL;
    Py_ssize_t nparams;
    Py_ssize_t nitems;
    Py_ssize_t nargs;
    int is_tuple;
    int completed = 0;

    if (PyErr_Occurred()) {
        goto done;
    }

    self_owner = Py_NewRef(self);
    args_owner = Py_NewRef(args);
    params_owner = Py_NewRef(parameters);
    input_owner = Py_NewRef(item_arg);

    nparams = PyTuple_GET_SIZE(params_owner);
    if (nparams == 0) {
        result = PyErr_Format(PyExc_TypeError,
                              "%R is not a generic class",
                              self_owner);
        if (result == NULL) {
            goto done;
        }
        completed = 1;
        goto done;
    }

    item = _unpack_args(input_owner);
    if (item == NULL) {
        goto done;
    }
    Py_CLEAR(input_owner);
    if (PyErr_Occurred()) {
        goto done;
    }

    for (Py_ssize_t i = 0; i < nparams; i++) {
        param = PyTuple_GetItemRef(params_owner, i);
        if (param == NULL) {
            goto done;
        }
        if (PyObject_GetOptionalAttr(param, &_Py_ID(__typing_prepare_subst__),
                                     &prepare) < 0 || PyErr_Occurred()) {
            goto done;
        }
        if (prepare != NULL && prepare != Py_None) {
            if (PyTuple_Check(item)) {
                tmp = PyObject_CallFunction(prepare, "OO", self_owner, item);
            }
            else {
                tmp = PyObject_CallFunction(prepare, "O(O)", self_owner, item);
            }
            Py_CLEAR(prepare);
            if (tmp == NULL || PyErr_Occurred()) {
                goto done;
            }
            old_item = item;
            item = tmp;
            tmp = NULL;
            Py_CLEAR(old_item);
            if (PyErr_Occurred()) {
                goto done;
            }
        }
        else {
            Py_CLEAR(prepare);
            if (PyErr_Occurred()) {
                goto done;
            }
        }
        Py_CLEAR(param);
        if (PyErr_Occurred()) {
            goto done;
        }
    }

    is_tuple = PyTuple_Check(item);
    nitems = is_tuple ? PyTuple_GET_SIZE(item) : 1;
    if (nitems != nparams) {
        Py_CLEAR(item);
        if (PyErr_Occurred()) {
            goto done;
        }
        result = PyErr_Format(PyExc_TypeError,
                              "Too %s arguments for %R; actual %zd, expected %zd",
                              nitems > nparams ? "many" : "few",
                              self_owner, nitems, nparams);
        if (result == NULL) {
            goto done;
        }
        completed = 1;
        goto done;
    }

    assert(PyTuple_Check(args_owner) || PyList_Check(args_owner));
    if (PyList_Check(args_owner)) {
        tuple_args = PySequence_Tuple(args_owner);
        if (tuple_args == NULL) {
            goto done;
        }
        source = tuple_args;
    }
    else {
        source = args_owner;
    }
    nargs = PyTuple_GET_SIZE(source);
    newargs = PyTuple_New(nargs);
    if (newargs == NULL || PyErr_Occurred()) {
        goto done;
    }

    for (Py_ssize_t iarg = 0, jarg = 0; iarg < nargs; iarg++) {
        arg = PyTuple_GetItemRef(source, iarg);
        if (arg == NULL) {
            goto done;
        }
        if (PyType_Check(arg)) {
            PyTuple_SET_ITEM(newargs, jarg, arg);
            arg = NULL;
            if (PyErr_Occurred()) {
                goto done;
            }
            jarg++;
            continue;
        }
        if (PyTuple_Check(arg) || PyList_Check(arg)) {
            subargs = _Py_subs_parameters(self_owner, arg, params_owner, item);
            if (subargs == NULL) {
                goto done;
            }
            if (PyTuple_Check(arg)) {
                PyTuple_SET_ITEM(newargs, jarg, subargs);
                subargs = NULL;
                if (PyErr_Occurred()) {
                    goto done;
                }
            }
            else {
                subargs_list = PySequence_List(subargs);
                if (subargs_list == NULL) {
                    goto done;
                }
                Py_CLEAR(subargs);
                if (PyErr_Occurred()) {
                    goto done;
                }
                PyTuple_SET_ITEM(newargs, jarg, subargs_list);
                subargs_list = NULL;
                if (PyErr_Occurred()) {
                    goto done;
                }
            }
            Py_CLEAR(arg);
            if (PyErr_Occurred()) {
                goto done;
            }
            jarg++;
            continue;
        }

        int unpack = _is_unpacked_typevartuple(arg);
        if (unpack < 0) {
            goto done;
        }
        if (PyObject_GetOptionalAttr(arg, &_Py_ID(__typing_subst__), &subst) < 0 ||
            PyErr_Occurred()) {
            goto done;
        }
        if (subst != NULL) {
            Py_ssize_t iparam = tuple_index(params_owner, nparams, arg);
            if (iparam < 0) {
                PyErr_Format(PyExc_TypeError,
                             "argument %R with __typing_subst__ was not found "
                             "in __parameters__",
                             arg);
                goto done;
            }
            replacement = is_tuple
                ? PyTuple_GetItemRef(item, iparam)
                : Py_NewRef(item);
            if (replacement == NULL || PyErr_Occurred()) {
                goto done;
            }
            tmp = PyObject_CallOneArg(subst, replacement);
            if (tmp == NULL) {
                goto done;
            }
            Py_CLEAR(replacement);
            if (PyErr_Occurred()) {
                goto done;
            }
            Py_CLEAR(subst);
            if (PyErr_Occurred()) {
                goto done;
            }
        }
        else {
            tmp = subs_tvars(arg, params_owner, item, is_tuple, nitems);
            if (tmp == NULL) {
                goto done;
            }
        }
        if (unpack) {
            if (!PyTuple_Check(tmp)) {
                original = PyTuple_GetItemRef(source, iarg);
                if (original == NULL || PyErr_Occurred()) {
                    goto done;
                }
                PyErr_Format(PyExc_TypeError,
                             "expected __typing_subst__ of %T objects to return a tuple, not %T",
                             original, tmp);
                goto done;
            }
            jarg = tuple_extend(&newargs, jarg, tmp);
            if (jarg < 0) {
                goto done;
            }
            Py_CLEAR(tmp);
            if (PyErr_Occurred()) {
                goto done;
            }
        }
        else {
            PyTuple_SET_ITEM(newargs, jarg, tmp);
            tmp = NULL;
            if (PyErr_Occurred()) {
                goto done;
            }
            jarg++;
        }
        Py_CLEAR(arg);
        if (PyErr_Occurred()) {
            goto done;
        }
    }

    Py_CLEAR(item);
    if (PyErr_Occurred()) {
        goto done;
    }
    Py_CLEAR(tuple_args);
    if (PyErr_Occurred()) {
        goto done;
    }
    result = newargs;
    newargs = NULL;
    completed = 1;

done:
    Py_CLEAR(replacement);
    Py_CLEAR(original);
    Py_CLEAR(subargs_list);
    Py_CLEAR(subargs);
    Py_CLEAR(subst);
    Py_CLEAR(tmp);
    Py_CLEAR(old_item);
    Py_CLEAR(prepare);
    Py_CLEAR(param);
    Py_CLEAR(arg);
    Py_CLEAR(newargs);
    Py_CLEAR(item);
    Py_CLEAR(tuple_args);
    Py_CLEAR(input_owner);
    Py_CLEAR(params_owner);
    Py_CLEAR(args_owner);
    Py_CLEAR(self_owner);
    if (!completed || PyErr_Occurred()) {
        Py_CLEAR(result);
        return NULL;
    }
    return result;
}

PyDoc_STRVAR(genericalias__doc__,
"GenericAlias(origin, args, /)\n"
"--\n\n"
"Represent a PEP 585 generic type\n"
"\n"
"For example, for t = list[int], t.__origin__ is list and t.__args__\n"
"is (int,).");

static PyObject *
ga_parameters_lock_held(PyObject *self);

static PyObject *
ga_getitem(PyObject *self, PyObject *item)
{
    gaobject *alias = (gaobject *)self;
    // Populate __parameters__ if needed.
    PyObject *parameters;
    Py_BEGIN_CRITICAL_SECTION(self);
    parameters = ga_parameters_lock_held(self);
    Py_END_CRITICAL_SECTION();
    if (parameters == NULL) {
        return NULL;
    }

    PyObject *newargs = _Py_subs_parameters(self, alias->args, parameters, item);
    Py_DECREF(parameters);
    if (newargs == NULL) {
        return NULL;
    }

    PyObject *res = Py_GenericAlias(alias->origin, newargs);
    if (res == NULL) {
        Py_DECREF(newargs);
        return NULL;
    }
    ((gaobject *)res)->starred = alias->starred;

    Py_DECREF(newargs);
    return res;
}

static PyMappingMethods ga_as_mapping = {
    .mp_subscript = ga_getitem,
};

static Py_hash_t
ga_hash(PyObject *self)
{
    gaobject *alias = (gaobject *)self;
    // TODO: Hash in the hash for the origin
    Py_hash_t h0 = PyObject_Hash(alias->origin);
    if (h0 == -1) {
        return -1;
    }
    Py_hash_t h1 = PyObject_Hash(alias->args);
    if (h1 == -1) {
        return -1;
    }
    return h0 ^ h1;
}

static inline PyObject *
set_orig_class(PyObject *obj, PyObject *self)
{
    if (obj != NULL) {
        if (PyObject_SetAttr(obj, &_Py_ID(__orig_class__), self) < 0) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError) &&
                !PyErr_ExceptionMatches(PyExc_TypeError))
            {
                Py_DECREF(obj);
                return NULL;
            }
            PyErr_Clear();
        }
    }
    return obj;
}

static PyObject *
ga_call(PyObject *self, PyObject *args, PyObject *kwds)
{
    gaobject *alias = (gaobject *)self;
    PyObject *obj = PyObject_Call(alias->origin, args, kwds);
    return set_orig_class(obj, self);
}

static PyObject *
ga_vectorcall(PyObject *self, PyObject *const *args,
              size_t nargsf, PyObject *kwnames)
{
    gaobject *alias = (gaobject *) self;
    PyObject *obj = PyObject_Vectorcall(alias->origin, args, nargsf, kwnames);
    return set_orig_class(obj, self);
}

static const char* const attr_exceptions[] = {
    "__class__",
    "__origin__",
    "__args__",
    "__unpacked__",
    "__parameters__",
    "__typing_unpacked_tuple_args__",
    "__mro_entries__",
    "__reduce_ex__",  // needed so we don't look up object.__reduce_ex__
    "__reduce__",
    NULL,
};

static const char* const attr_blocked[] = {
    "__bases__",
    "__copy__",
    "__deepcopy__",
    NULL,
};

static PyObject *
ga_getattro(PyObject *self, PyObject *name)
{
    gaobject *alias = (gaobject *)self;
    if (PyUnicode_Check(name)) {
        // When we check blocked attrs, we don't allow to proxy them to `__origin__`.
        // Otherwise, we can break existing code.
        for (const char * const *p = attr_blocked; ; p++) {
            if (*p == NULL) {
                break;
            }
            if (_PyUnicode_EqualToASCIIString(name, *p)) {
                goto generic_getattr;
            }
        }

        // When we see own attrs, it has a priority over `__origin__`'s attr.
        for (const char * const *p = attr_exceptions; ; p++) {
            if (*p == NULL) {
                return PyObject_GetAttr(alias->origin, name);
            }
            if (_PyUnicode_EqualToASCIIString(name, *p)) {
                goto generic_getattr;
            }
        }
    }

generic_getattr:
    return PyObject_GenericGetAttr(self, name);
}

static PyObject *
ga_richcompare(PyObject *a, PyObject *b, int op)
{
    if (!_PyGenericAlias_Check(b) ||
        (op != Py_EQ && op != Py_NE))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (op == Py_NE) {
        PyObject *eq = ga_richcompare(a, b, Py_EQ);
        if (eq == NULL)
            return NULL;
        Py_DECREF(eq);
        if (eq == Py_True) {
            Py_RETURN_FALSE;
        }
        else {
            Py_RETURN_TRUE;
        }
    }

    gaobject *aa = (gaobject *)a;
    gaobject *bb = (gaobject *)b;
    if (aa->starred != bb->starred) {
        Py_RETURN_FALSE;
    }
    int eq = PyObject_RichCompareBool(aa->origin, bb->origin, Py_EQ);
    if (eq < 0) {
        return NULL;
    }
    if (!eq) {
        Py_RETURN_FALSE;
    }
    return PyObject_RichCompare(aa->args, bb->args, Py_EQ);
}

static PyObject *
ga_mro_entries(PyObject *self, PyObject *args)
{
    gaobject *alias = (gaobject *)self;
    return PyTuple_Pack(1, alias->origin);
}

static PyObject *
ga_instancecheck(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyErr_SetString(PyExc_TypeError,
                    "isinstance() argument 2 cannot be a parameterized generic");
    return NULL;
}

static PyObject *
ga_subclasscheck(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyErr_SetString(PyExc_TypeError,
                    "issubclass() argument 2 cannot be a parameterized generic");
    return NULL;
}

static PyObject *
ga_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    gaobject *alias = (gaobject *)self;
    if (alias->starred) {
        PyObject *tmp = Py_GenericAlias(alias->origin, alias->args);
        if (tmp != NULL) {
            Py_SETREF(tmp, PyObject_GetIter(tmp));
        }
        if (tmp == NULL) {
            return NULL;
        }
        return Py_BuildValue("N(N)", _PyEval_GetBuiltin(&_Py_ID(next)), tmp);
    }
    return Py_BuildValue("O(OO)", Py_TYPE(alias),
                         alias->origin, alias->args);
}

static PyObject *
ga_dir(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    gaobject *alias = (gaobject *)self;
    PyObject *dir = PyObject_Dir(alias->origin);
    if (dir == NULL) {
        return NULL;
    }

    PyObject *dir_entry = NULL;
    for (const char * const *p = attr_exceptions; ; p++) {
        if (*p == NULL) {
            break;
        }
        else {
            dir_entry = PyUnicode_FromString(*p);
            if (dir_entry == NULL) {
                goto error;
            }
            int contains = PySequence_Contains(dir, dir_entry);
            if (contains < 0) {
                goto error;
            }
            if (contains == 0 && PyList_Append(dir, dir_entry) < 0) {
                goto error;
            }

            Py_CLEAR(dir_entry);
        }
    }
    return dir;

error:
    Py_DECREF(dir);
    Py_XDECREF(dir_entry);
    return NULL;
}

static PyMethodDef ga_methods[] = {
    {"__mro_entries__", ga_mro_entries, METH_O},
    {"__instancecheck__", ga_instancecheck, METH_O},
    {"__subclasscheck__", ga_subclasscheck, METH_O},
    {"__reduce__", ga_reduce, METH_NOARGS},
    {"__dir__", ga_dir, METH_NOARGS},
    {0}
};

static PyMemberDef ga_members[] = {
    {"__origin__", _Py_T_OBJECT, offsetof(gaobject, origin), Py_READONLY},
    {"__args__", _Py_T_OBJECT, offsetof(gaobject, args), Py_READONLY},
    {"__unpacked__", Py_T_BOOL, offsetof(gaobject, starred), Py_READONLY},
    {0}
};

static PyObject *
ga_parameters_lock_held(PyObject *self)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(self);
    gaobject *alias = (gaobject *)self;
    if (alias->parameters == NULL) {
        alias->parameters = _Py_make_parameters(alias->args);
        if (alias->parameters == NULL) {
            return NULL;
        }
    }
    return Py_NewRef(alias->parameters);
}

static PyObject *
ga_parameters(PyObject *self, void *unused)
{
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(self);
    result = ga_parameters_lock_held(self);
    Py_END_CRITICAL_SECTION();
    return result;
}

static PyObject *
ga_unpacked_tuple_args(PyObject *self, void *unused)
{
    gaobject *alias = (gaobject *)self;
    if (alias->starred && alias->origin == (PyObject *)&PyTuple_Type) {
        return Py_NewRef(alias->args);
    }
    Py_RETURN_NONE;
}

static PyGetSetDef ga_properties[] = {
    {"__parameters__", ga_parameters, NULL, PyDoc_STR("Type variables in the GenericAlias."), NULL},
    {"__typing_unpacked_tuple_args__", ga_unpacked_tuple_args, NULL, NULL},
    {0}
};

/* A helper function to create GenericAlias' args tuple and set its attributes.
 * Returns 1 on success, 0 on failure.
 */
static inline int
setup_ga(gaobject *alias, PyObject *origin, PyObject *args) {
    if (!PyTuple_Check(args)) {
        args = PyTuple_Pack(1, args);
        if (args == NULL) {
            return 0;
        }
    }
    else {
        Py_INCREF(args);
    }

    alias->origin = Py_NewRef(origin);
    alias->args = args;
    alias->parameters = NULL;
    alias->weakreflist = NULL;

    if (PyVectorcall_Function(origin) != NULL) {
        alias->vectorcall = ga_vectorcall;
    }
    else {
        alias->vectorcall = NULL;
    }

    return 1;
}

static PyObject *
ga_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *args_owner = NULL;
    PyObject *type_owner = NULL;
    PyObject *origin = NULL;
    PyObject *arguments = NULL;
    PyObject *result = NULL;
    int completed = 0;
    int status;

    if (PyErr_Occurred()) {
        return NULL;
    }

    args_owner = Py_NewRef(args);
    if (PyErr_Occurred()) {
        goto done;
    }
    type_owner = Py_NewRef((PyObject *)type);
    if (PyErr_Occurred()) {
        goto done;
    }

    if (!_PyArg_NoKeywords("GenericAlias", kwds)) {
        goto done;
    }
    if (!_PyArg_CheckPositional("GenericAlias", PyTuple_GET_SIZE(args_owner), 2, 2)
            || PyErr_Occurred()) {
        goto done;
    }

    origin = PyTuple_GetItemRef(args_owner, 0);
    if (origin == NULL) {
        goto done;
    }
    arguments = PyTuple_GetItemRef(args_owner, 1);
    if (arguments == NULL) {
        goto done;
    }

    result = ((PyTypeObject *)type_owner)->tp_alloc(
        (PyTypeObject *)type_owner, 0);
    if (result == NULL || PyErr_Occurred()) {
        goto done;
    }

    status = setup_ga((gaobject *)result, origin, arguments);
    if (!status) {
        goto done;
    }
    completed = 1;

done:
    Py_CLEAR(origin);
    Py_CLEAR(arguments);
    Py_CLEAR(args_owner);
    Py_CLEAR(type_owner);

    if (!completed || PyErr_Occurred()) {
        Py_CLEAR(result);
        return NULL;
    }
    return result;
}

static PyNumberMethods ga_as_number = {
        .nb_or = _Py_union_type_or, // Add __or__ function
};

static PyObject *
ga_iternext(PyObject *op)
{
    gaiterobject *gi = (gaiterobject*)op;
    PyObject *obj;
    Py_BEGIN_CRITICAL_SECTION(gi);
    obj = gi->obj;
    gi->obj = NULL;
    Py_END_CRITICAL_SECTION();
    if (obj == NULL) {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
    gaobject *alias = (gaobject *)obj;
    PyObject *starred_alias = Py_GenericAlias(alias->origin, alias->args);
    Py_DECREF(obj);
    if (starred_alias == NULL) {
        return NULL;
    }
    ((gaobject *)starred_alias)->starred = true;
    return starred_alias;
}

static void
ga_iter_dealloc(PyObject *op)
{
    gaiterobject *gi = (gaiterobject*)op;
    PyObject_GC_UnTrack(gi);
    Py_XDECREF(gi->obj);
    PyObject_GC_Del(gi);
}

static int
ga_iter_traverse(PyObject *op, visitproc visit, void *arg)
{
    gaiterobject *gi = (gaiterobject*)op;
    Py_VISIT(gi->obj);
    return 0;
}

static int
ga_iter_clear(PyObject *self)
{
    gaiterobject *gi = (gaiterobject *)self;
    Py_CLEAR(gi->obj);
    return 0;
}

static PyObject *
ga_iter_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *iter = _PyEval_GetBuiltin(&_Py_ID(iter));
    gaiterobject *gi = (gaiterobject *)self;

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */

    PyObject *obj;
    Py_BEGIN_CRITICAL_SECTION(gi);
    obj = Py_XNewRef(gi->obj);
    Py_END_CRITICAL_SECTION();

    if (obj) {
        PyObject *result = Py_BuildValue("N(O)", iter, obj);
        Py_DECREF(obj);
        return result;
    }
    else {
        return Py_BuildValue("N(())", iter);
    }
}

static PyMethodDef ga_iter_methods[] = {
    {"__reduce__", ga_iter_reduce, METH_NOARGS},
    {0}
};

// gh-91632: _Py_GenericAliasIterType is exported  to be cleared
// in _PyTypes_FiniTypes.
PyTypeObject _Py_GenericAliasIterType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "generic_alias_iterator",
    .tp_basicsize = sizeof(gaiterobject),
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = ga_iternext,
    .tp_traverse = ga_iter_traverse,
    .tp_methods = ga_iter_methods,
    .tp_dealloc = ga_iter_dealloc,
    .tp_clear = ga_iter_clear,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
};

static PyObject *
ga_iter(PyObject *self) {
    gaiterobject *gi = PyObject_GC_New(gaiterobject, &_Py_GenericAliasIterType);
    if (gi == NULL) {
        return NULL;
    }
    gi->obj = Py_NewRef(self);
    PyObject_GC_Track(gi);
    return (PyObject *)gi;
}

// TODO:
// - argument clinic?
// - cache?
PyTypeObject Py_GenericAliasType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "types.GenericAlias",
    .tp_doc = genericalias__doc__,
    .tp_basicsize = sizeof(gaobject),
    .tp_dealloc = ga_dealloc,
    .tp_repr = ga_repr,
    .tp_as_number = &ga_as_number,  // allow X | Y of GenericAlias objs
    .tp_as_mapping = &ga_as_mapping,
    .tp_hash = ga_hash,
    .tp_call = ga_call,
    .tp_getattro = ga_getattro,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_VECTORCALL,
    .tp_traverse = ga_traverse,
    .tp_richcompare = ga_richcompare,
    .tp_weaklistoffset = offsetof(gaobject, weakreflist),
    .tp_methods = ga_methods,
    .tp_members = ga_members,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = ga_new,
    .tp_free = PyObject_GC_Del,
    .tp_getset = ga_properties,
    .tp_iter = ga_iter,
    .tp_vectorcall_offset = offsetof(gaobject, vectorcall),
};

PyObject *
Py_GenericAlias(PyObject *origin, PyObject *args)
{
    gaobject *alias = (gaobject*) PyType_GenericAlloc(
            (PyTypeObject *)&Py_GenericAliasType, 0);
    if (alias == NULL) {
        return NULL;
    }
    if (!setup_ga(alias, origin, args)) {
        Py_DECREF(alias);
        return NULL;
    }
    return (PyObject *)alias;
}
