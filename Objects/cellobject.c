/* Cell object implementation */

#include "Python.h"
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_object.h"

#define _PyCell_CAST(op) _Py_CAST(PyCellObject*, (op))

PyObject *
PyCell_New(PyObject *obj)
{
    PyCellObject *op;

    op = (PyCellObject *)PyObject_GC_New(PyCellObject, &PyCell_Type);
    if (op == NULL)
        return NULL;
    op->ob_ref = obj == NULL ? PyValue_NULL : PyValue_FromObject(obj);

    _PyObject_GC_TRACK(op);
    return (PyObject *)op;
}

PyDoc_STRVAR(cell_new_doc,
"cell([contents])\n"
"--\n"
"\n"
"Create a new cell object.\n"
"\n"
"  contents\n"
"    the contents of the cell. If not specified, the cell will be empty,\n"
"    and further attempts to access its cell_contents attribute will\n"
"    raise a ValueError.");


static PyObject *
cell_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *obj = NULL;

    if (!_PyArg_NoKeywords("cell", kwargs)) {
        goto exit;
    }
    /* min = 0: we allow the cell to be empty */
    if (!PyArg_UnpackTuple(args, "cell", 0, 1, &obj)) {
        goto exit;
    }
    return_value = PyCell_New(obj);

exit:
    return return_value;
}

PyObject *
PyCell_Get(PyObject *op)
{
    if (!PyCell_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyCell_GetRef((PyCellObject *)op);
}

int
PyCell_Set(PyObject *op, PyObject *value)
{
    if (!PyCell_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    PyCell_SET(op, value);
    return 0;
}

static void
cell_dealloc(PyObject *self)
{
    PyCellObject *op = _PyCell_CAST(self);
    _PyObject_GC_UNTRACK(op);
    PyObject_GC_Del(op);
}

static PyObject *
cell_richcompare(PyObject *a, PyObject *b, int op)
{
    /* neither argument should be NULL, unless something's gone wrong */
    assert(a != NULL && b != NULL);

    /* both arguments should be instances of PyCellObject */
    if (!PyCell_Check(a) || !PyCell_Check(b)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    PyValue a_value = _PyCell_GetValue((PyCellObject *)a);
    PyValue b_value = _PyCell_GetValue((PyCellObject *)b);

    /* compare cells by contents; empty cells come before anything else */
    if (!PyValue_IsNull(a_value) && !PyValue_IsNull(b_value)) {
        return PyValue_Object(PyObject_RichCompareValue(a_value, b_value, op));
    }
    Py_RETURN_RICHCOMPARE(PyValue_IsNull(b_value), PyValue_IsNull(a_value), op);
}

static PyObject *
cell_repr(PyObject *self)
{
    PyValue value = _PyCell_GetValue((PyCellObject *)self);
    if (PyValue_IsNull(value)) {
        return PyUnicode_FromFormat("<cell at %p: empty>", self);
    }
    PyObject *ref = PyValue_Object(value);
    if (ref == NULL) {
        return NULL;
    }
    PyObject *res = PyUnicode_FromFormat("<cell at %p: %.80s object at %p>",
                                         self, Py_TYPE(ref)->tp_name, ref);
    Py_DECREF(ref);
    return res;
}

static int
cell_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyCellObject *op = _PyCell_CAST(self);
    _Py_VISIT_VALUE(op->ob_ref);
    return 0;
}

static int
cell_clear(PyObject *self)
{
    PyCellObject *op = _PyCell_CAST(self);
    op->ob_ref = PyValue_NULL;
    return 0;
}

static PyValue
cell_get_contents(PyObject *self, void *closure)
{
    PyValue value = _PyCell_GetValue(_PyCell_CAST(self));
    if (PyValue_IsNull(value)) {
        PyErr_SetString(PyExc_ValueError, "Cell is empty");
        return PyValue_NULL;
    }
    return value;
}

static int
cell_set_contents(PyObject *self, PyObject *obj, void *Py_UNUSED(ignored))
{
    PyCell_SET(self, obj);
    return 0;
}

static PyGetSetDef cell_getsetlist[] = {
    {"cell_contents", cell_get_contents, cell_set_contents, NULL},
    {NULL} /* sentinel */
};

PyTypeObject PyCell_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "cell",
    sizeof(PyCellObject),
    0,
    cell_dealloc,                               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    cell_repr,                                  /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    cell_new_doc,                               /* tp_doc */
    cell_traverse,                              /* tp_traverse */
    cell_clear,                                 /* tp_clear */
    cell_richcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    cell_getsetlist,                            /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    cell_new,                                   /* tp_new */
    0,                                          /* tp_free */
};
