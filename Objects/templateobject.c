/* t-string Template object implementation */

#include "Python.h"
#include "pycore_interpolation.h" // _PyInterpolation_CheckExact()
#include "pycore_runtime.h"       // _Py_STR()
#include "pycore_template.h"
#include "pycore_tuple.h"         // _PyTuple_GET_OBJECT()
#include "pycore_value.h"         // PyValue_TYPE(), _PyValue_RestoreException()

typedef struct {
    PyObject_HEAD
    PyObject *stringsiter;
    PyObject *interpolationsiter;
    int from_strings;
} templateiterobject;

#define templateiterobject_CAST(op) \
    (assert(_PyTemplateIter_CheckExact(op)), _Py_CAST(templateiterobject*, (op)))

static PyObject *
templateiter_next(PyObject *op)
{
    templateiterobject *self = templateiterobject_CAST(op);
    PyObject *item;
    if (self->from_strings) {
        item = PyIter_Next(self->stringsiter);
        self->from_strings = 0;
        if (item == NULL) {
            return NULL;
        }
        if (PyUnicode_GET_LENGTH(item) == 0) {
            Py_SETREF(item, PyIter_Next(self->interpolationsiter));
            self->from_strings = 1;
        }
    }
    else {
        item = PyIter_Next(self->interpolationsiter);
        self->from_strings = 1;
    }
    return item;
}

static void
templateiter_dealloc(PyObject *op)
{
    PyObject_GC_UnTrack(op);
    Py_TYPE(op)->tp_clear(op);
    Py_TYPE(op)->tp_free(op);
}

static int
templateiter_clear(PyObject *op)
{
    templateiterobject *self = templateiterobject_CAST(op);
    Py_CLEAR(self->stringsiter);
    Py_CLEAR(self->interpolationsiter);
    return 0;
}

static int
templateiter_traverse(PyObject *op, visitproc visit, void *arg)
{
    templateiterobject *self = templateiterobject_CAST(op);
    Py_VISIT(self->stringsiter);
    Py_VISIT(self->interpolationsiter);
    return 0;
}

PyTypeObject _PyTemplateIter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "string.templatelib.TemplateIter",
    .tp_doc = PyDoc_STR("Template iterator object"),
    .tp_basicsize = sizeof(templateiterobject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_alloc = PyType_GenericAlloc,
    .tp_dealloc = templateiter_dealloc,
    .tp_clear = templateiter_clear,
    .tp_free = PyObject_GC_Del,
    .tp_traverse = templateiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = templateiter_next,
};

typedef struct {
    PyObject_HEAD
    PyObject *strings;
    PyObject *interpolations;
} templateobject;

#define templateobject_CAST(op) \
    (assert(_PyTemplate_CheckExact(op)), _Py_CAST(templateobject*, (op)))

static PyObject *
template_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (kwds != NULL) {
        PyErr_SetString(PyExc_TypeError, "Template.__new__ only accepts *args arguments");
        return NULL;
    }

    Py_ssize_t argslen = PyTuple_GET_SIZE(args);
    Py_ssize_t stringslen = 0;
    Py_ssize_t interpolationslen = 0;
    int last_was_str = 0;

    for (Py_ssize_t i = 0; i < argslen; i++) {
        PyValue item_value = PyTuple_GET_VALUE(args, i);
        PyTypeObject *itemtype = PyValue_TYPE(item_value);
        if (PyType_FastSubclass(itemtype, Py_TPFLAGS_UNICODE_SUBCLASS)) {
            if (!last_was_str) {
                stringslen++;
            }
            last_was_str = 1;
        }
        else if (itemtype == &_PyInterpolation_Type) {
            if (!last_was_str) {
                stringslen++;
            }
            interpolationslen++;
            last_was_str = 0;
        }
        else {
            PyObject *invalid_type = Py_NewRef((PyObject *)itemtype);
            PyErr_Format(
                PyExc_TypeError,
                "Template.__new__ *args need to be of type 'str' or 'Interpolation', got %N",
                invalid_type);
            Py_CLEAR(invalid_type);
            return NULL;
        }
    }
    if (!last_was_str) {
        stringslen++;
    }

    PyObject *strings = PyTuple_New(stringslen);
    PyObject *interpolations = NULL;
    PyObject *concat = NULL;
    PyObject *template = NULL;
    if (strings == NULL || PyErr_Occurred()) goto error;
    interpolations = PyTuple_New(interpolationslen);
    if (interpolations == NULL) goto error;

    last_was_str = 0;
    Py_ssize_t stringsidx = 0, interpolationsidx = 0;
    for (Py_ssize_t i = 0; i < argslen; i++) {
        PyObject *item = _PyTuple_GET_OBJECT(args, i);
        if (PyUnicode_Check(item)) {
            if (last_was_str) {
                PyObject *laststring = _PyTuple_GET_OBJECT(strings, stringsidx - 1);
                concat = PyUnicode_Concat(laststring, item);
                if (concat == NULL) goto error;
                int status = PyTuple_SetItem(strings, stringsidx - 1, concat);
                concat = NULL;
                if (status < 0 || PyErr_Occurred()) goto error;
            }
            else {
                PyTuple_SET_ITEM(strings, stringsidx++, Py_NewRef(item));
            }
            last_was_str = 1;
        }
        else if (_PyInterpolation_CheckExact(item)) {
            if (!last_was_str) {
                _Py_DECLARE_STR(empty, "");
                PyTuple_SET_ITEM(strings, stringsidx++, &_Py_STR(empty));
            }
            PyTuple_SET_ITEM(interpolations, interpolationsidx++, Py_NewRef(item));
            last_was_str = 0;
        }
    }
    if (!last_was_str) {
        _Py_DECLARE_STR(empty, "");
        PyTuple_SET_ITEM(strings, stringsidx++, &_Py_STR(empty));
    }

    template = _PyTemplate_Build(strings, interpolations);
    if (template == NULL || PyErr_Occurred()) goto error;
    Py_CLEAR(strings);
    if (PyErr_Occurred()) goto error;
    Py_CLEAR(interpolations);
    if (PyErr_Occurred()) goto error;
    return template;
error:
    Py_CLEAR(concat);
    Py_CLEAR(strings);
    Py_CLEAR(interpolations);
    Py_CLEAR(template);
    return NULL;
}

static void
template_dealloc(PyObject *op)
{
    PyObject_GC_UnTrack(op);
    Py_TYPE(op)->tp_clear(op);
    Py_TYPE(op)->tp_free(op);
}

static int
template_clear(PyObject *op)
{
    templateobject *self = templateobject_CAST(op);
    Py_CLEAR(self->strings);
    Py_CLEAR(self->interpolations);
    return 0;
}

static int
template_traverse(PyObject *op, visitproc visit, void *arg)
{
    templateobject *self = templateobject_CAST(op);
    Py_VISIT(self->strings);
    Py_VISIT(self->interpolations);
    return 0;
}

static PyObject *
template_repr(PyObject *op)
{
    templateobject *self = templateobject_CAST(op);
    return PyUnicode_FromFormat("%s(strings=%R, interpolations=%R)",
                                _PyType_Name(Py_TYPE(self)),
                                self->strings,
                                self->interpolations);
}

static PyObject *
template_iter(PyObject *op)
{
    templateobject *self = templateobject_CAST(op);
    templateiterobject *iter = PyObject_GC_New(templateiterobject, &_PyTemplateIter_Type);
    if (iter == NULL) {
        return NULL;
    }
    iter->stringsiter = NULL;
    iter->interpolationsiter = NULL;

    PyObject *stringsiter = PyObject_GetIter(self->strings);
    if (stringsiter == NULL) {
        Py_DECREF(iter);
        return NULL;
    }

    PyObject *interpolationsiter = PyObject_GetIter(self->interpolations);
    if (interpolationsiter == NULL) {
        Py_DECREF(iter);
        Py_DECREF(stringsiter);
        return NULL;
    }

    iter->stringsiter = stringsiter;
    iter->interpolationsiter = interpolationsiter;
    iter->from_strings = 1;
    PyObject_GC_Track(iter);
    return (PyObject *)iter;
}

static PyObject *
template_strings_concat(PyObject *left, PyObject *right)
{
    Py_ssize_t left_stringslen = PyTuple_GET_SIZE(left);
    Py_ssize_t right_stringslen = PyTuple_GET_SIZE(right);
    PyObject *left_laststring = NULL;
    PyObject *right_firststring = NULL;
    PyObject *concat = NULL;
    PyObject *newstrings = NULL;

    left_laststring = PyTuple_GetItemRef(left, left_stringslen - 1);
    if (left_laststring == NULL) goto error;
    right_firststring = PyTuple_GetItemRef(right, 0);
    if (right_firststring == NULL) goto error;

    concat = PyUnicode_Concat(left_laststring, right_firststring);
    if (concat == NULL) goto error;
    Py_CLEAR(left_laststring);
    if (PyErr_Occurred()) goto error;
    Py_CLEAR(right_firststring);
    if (PyErr_Occurred()) goto error;

    newstrings = PyTuple_New(left_stringslen + right_stringslen - 1);
    if (newstrings == NULL) goto error;

    Py_ssize_t index = 0;
    for (Py_ssize_t i = 0; i < left_stringslen - 1; i++) {
        PyTuple_SET_VALUE(newstrings, index++, PyTuple_GET_VALUE(left, i));
        if (PyErr_Occurred()) goto error;
    }
    PyTuple_SET_ITEM(newstrings, index++, concat);
    concat = NULL;
    if (PyErr_Occurred()) goto error;
    for (Py_ssize_t i = 1; i < right_stringslen; i++) {
        PyTuple_SET_VALUE(newstrings, index++, PyTuple_GET_VALUE(right, i));
        if (PyErr_Occurred()) goto error;
    }

    return newstrings;
error:
    Py_CLEAR(left_laststring);
    Py_CLEAR(right_firststring);
    Py_CLEAR(concat);
    Py_CLEAR(newstrings);
    return NULL;
}

static PyObject *
template_concat_templates(templateobject *self, templateobject *other)
{
    PyObject *newstrings = template_strings_concat(self->strings, other->strings);
    if (newstrings == NULL) {
        return NULL;
    }

    PyObject *newinterpolations = PySequence_Concat(self->interpolations, other->interpolations);
    if (newinterpolations == NULL) {
        Py_DECREF(newstrings);
        return NULL;
    }

    PyObject *newtemplate = _PyTemplate_Build(newstrings, newinterpolations);
    Py_DECREF(newstrings);
    Py_DECREF(newinterpolations);
    return newtemplate;
}

PyObject *
_PyTemplate_Concat(PyObject *self, PyObject *other)
{
    if (_PyTemplate_CheckExact(self) && _PyTemplate_CheckExact(other)) {
        return template_concat_templates((templateobject *) self, (templateobject *) other);
    }

    PyErr_Format(PyExc_TypeError,
        "can only concatenate string.templatelib.Template (not \"%T\") to string.templatelib.Template",
        other);
    return NULL;
}

static PyObject *
template_values_get(PyObject *op, void *Py_UNUSED(data))
{
    templateobject *self = templateobject_CAST(op);

    Py_ssize_t len = PyTuple_GET_SIZE(self->interpolations);
    PyObject *values = PyTuple_New(PyTuple_GET_SIZE(self->interpolations));
    PyObject *value = NULL;
    if (values == NULL || PyErr_Occurred()) goto error;

    for (Py_ssize_t i = 0; i < len; i++) {
        PyObject *item = _PyTuple_GET_OBJECT(self->interpolations, i);
        value = _PyInterpolation_GetValueRef(item);
        if (value == NULL) goto error;
        PyTuple_SET_ITEM(values, i, value);
        value = NULL;
        if (PyErr_Occurred()) goto error;
    }

    return values;
error:
    Py_CLEAR(value);
    Py_CLEAR(values);
    return NULL;
}

static PyMemberDef template_members[] = {
    {"strings", Py_T_OBJECT_EX, offsetof(templateobject, strings), Py_READONLY, "Strings"},
    {"interpolations", Py_T_OBJECT_EX, offsetof(templateobject, interpolations), Py_READONLY, "Interpolations"},
    {NULL},
};

static PyGetSetDef template_getset[] = {
    {"values", template_values_get, NULL,
     PyDoc_STR("Values of interpolations"), NULL},
    {NULL},
};

static PySequenceMethods template_as_sequence = {
    .sq_concat = _PyTemplate_Concat,
};

static PyObject*
template_reduce(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    PyObject *mod = PyImport_ImportModule("string.templatelib");
    if (mod == NULL) {
        return NULL;
    }
    PyObject *func = PyObject_GetAttrString(mod, "_template_unpickle");
    Py_DECREF(mod);
    if (func == NULL) {
        return NULL;
    }

    templateobject *self = templateobject_CAST(op);
    PyObject *result = Py_BuildValue("O(OO)",
                                     func,
                                     self->strings,
                                     self->interpolations);

    Py_DECREF(func);
    return result;
}

static PyMethodDef template_methods[] = {
    {"__reduce__", template_reduce, METH_NOARGS, NULL},
    {"__class_getitem__", Py_GenericAlias,
     METH_O|METH_CLASS,
     // note that this is not supported in typeshed, and it is not clear if the
     // type for this is a simple TypeVar or a TypeVarTuple
     // for details, see: https://github.com/python/typeshed/issues/14878
     PyDoc_STR("Template supports [] for generic usage")},
    {NULL, NULL},
};

PyTypeObject _PyTemplate_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "string.templatelib.Template",
    .tp_doc = PyDoc_STR("Template object"),
    .tp_basicsize = sizeof(templateobject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_as_sequence = &template_as_sequence,
    .tp_new = template_new,
    .tp_alloc = PyType_GenericAlloc,
    .tp_dealloc = template_dealloc,
    .tp_clear = template_clear,
    .tp_free = PyObject_GC_Del,
    .tp_repr = template_repr,
    .tp_members = template_members,
    .tp_methods = template_methods,
    .tp_getset = template_getset,
    .tp_iter = template_iter,
    .tp_traverse = template_traverse,
};

PyObject *
_PyTemplate_Build(PyObject *strings, PyObject *interpolations)
{
    templateobject *template = PyObject_GC_New(templateobject, &_PyTemplate_Type);
    if (template == NULL) {
        return NULL;
    }

    template->strings = Py_NewRef(strings);
    template->interpolations = Py_NewRef(interpolations);
    PyObject_GC_Track(template);
    return (PyObject *) template;
}
