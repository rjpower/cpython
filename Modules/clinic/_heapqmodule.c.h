/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_heapq_heappush__doc__,
"heappush($module, heap, item, /)\n"
"--\n"
"\n"
"Push item onto heap, maintaining the heap invariant.");

#define _HEAPQ_HEAPPUSH_METHODDEF    \
    {"heappush", _PyCFunction_CAST(_heapq_heappush), METH_VALUE, _heapq_heappush__doc__},

static PyValue
_heapq_heappush_impl(PyObject *module, PyObject *heap, PyValueBorrow item);

static PyValue
_heapq_heappush(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;
    PyValueBorrow item;

    if (!_PyArg_CheckPositional("heappush", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heappush", "argument 1", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    item = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heappush_impl(PyValue_AsKnownObjectBorrow(module), heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappop__doc__,
"heappop($module, heap, /)\n"
"--\n"
"\n"
"Pop the smallest item off the heap, maintaining the heap invariant.");

#define _HEAPQ_HEAPPOP_METHODDEF    \
    {"heappop", _PyCFunction_CAST(_heapq_heappop), METH_VALUE|METH_O, _heapq_heappop__doc__},

static PyValue
_heapq_heappop_impl(PyObject *module, PyObject *heap);

static PyValue
_heapq_heappop(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heappop", "argument", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heappop_impl(PyValue_AsKnownObjectBorrow(module), heap);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heapreplace__doc__,
"heapreplace($module, heap, item, /)\n"
"--\n"
"\n"
"Pop and return the current smallest value, and add the new item.\n"
"\n"
"This is more efficient than heappop() followed by heappush(), and can be\n"
"more appropriate when using a fixed-size heap.  Note that the value\n"
"returned may be larger than item!  That constrains reasonable uses of\n"
"this routine unless written as part of a conditional replacement:\n"
"\n"
"    if item > heap[0]:\n"
"        item = heapreplace(heap, item)");

#define _HEAPQ_HEAPREPLACE_METHODDEF    \
    {"heapreplace", _PyCFunction_CAST(_heapq_heapreplace), METH_VALUE, _heapq_heapreplace__doc__},

static PyValue
_heapq_heapreplace_impl(PyObject *module, PyObject *heap, PyValueBorrow item);

static PyValue
_heapq_heapreplace(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;
    PyValueBorrow item;

    if (!_PyArg_CheckPositional("heapreplace", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heapreplace", "argument 1", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    item = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heapreplace_impl(PyValue_AsKnownObjectBorrow(module), heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappushpop__doc__,
"heappushpop($module, heap, item, /)\n"
"--\n"
"\n"
"Push item on the heap, then pop and return the smallest item from the heap.\n"
"\n"
"The combined action runs more efficiently than heappush() followed by\n"
"a separate call to heappop().");

#define _HEAPQ_HEAPPUSHPOP_METHODDEF    \
    {"heappushpop", _PyCFunction_CAST(_heapq_heappushpop), METH_VALUE, _heapq_heappushpop__doc__},

static PyValue
_heapq_heappushpop_impl(PyObject *module, PyObject *heap, PyValueBorrow item);

static PyValue
_heapq_heappushpop(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;
    PyValueBorrow item;

    if (!_PyArg_CheckPositional("heappushpop", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heappushpop", "argument 1", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    item = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heappushpop_impl(PyValue_AsKnownObjectBorrow(module), heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heapify__doc__,
"heapify($module, heap, /)\n"
"--\n"
"\n"
"Transform list into a heap, in-place, in O(len(heap)) time.");

#define _HEAPQ_HEAPIFY_METHODDEF    \
    {"heapify", _PyCFunction_CAST(_heapq_heapify), METH_VALUE|METH_O, _heapq_heapify__doc__},

static PyValue
_heapq_heapify_impl(PyObject *module, PyObject *heap);

static PyValue
_heapq_heapify(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heapify", "argument", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heapify_impl(PyValue_AsKnownObjectBorrow(module), heap);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappush_max__doc__,
"heappush_max($module, heap, item, /)\n"
"--\n"
"\n"
"Push item onto max heap, maintaining the heap invariant.");

#define _HEAPQ_HEAPPUSH_MAX_METHODDEF    \
    {"heappush_max", _PyCFunction_CAST(_heapq_heappush_max), METH_VALUE, _heapq_heappush_max__doc__},

static PyValue
_heapq_heappush_max_impl(PyObject *module, PyObject *heap,
                         PyValueBorrow item);

static PyValue
_heapq_heappush_max(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;
    PyValueBorrow item;

    if (!_PyArg_CheckPositional("heappush_max", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heappush_max", "argument 1", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    item = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heappush_max_impl(PyValue_AsKnownObjectBorrow(module), heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappop_max__doc__,
"heappop_max($module, heap, /)\n"
"--\n"
"\n"
"Maxheap variant of heappop.");

#define _HEAPQ_HEAPPOP_MAX_METHODDEF    \
    {"heappop_max", _PyCFunction_CAST(_heapq_heappop_max), METH_VALUE|METH_O, _heapq_heappop_max__doc__},

static PyValue
_heapq_heappop_max_impl(PyObject *module, PyObject *heap);

static PyValue
_heapq_heappop_max(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heappop_max", "argument", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heappop_max_impl(PyValue_AsKnownObjectBorrow(module), heap);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heapreplace_max__doc__,
"heapreplace_max($module, heap, item, /)\n"
"--\n"
"\n"
"Maxheap variant of heapreplace.");

#define _HEAPQ_HEAPREPLACE_MAX_METHODDEF    \
    {"heapreplace_max", _PyCFunction_CAST(_heapq_heapreplace_max), METH_VALUE, _heapq_heapreplace_max__doc__},

static PyValue
_heapq_heapreplace_max_impl(PyObject *module, PyObject *heap,
                            PyValueBorrow item);

static PyValue
_heapq_heapreplace_max(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;
    PyValueBorrow item;

    if (!_PyArg_CheckPositional("heapreplace_max", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heapreplace_max", "argument 1", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    item = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heapreplace_max_impl(PyValue_AsKnownObjectBorrow(module), heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heapify_max__doc__,
"heapify_max($module, heap, /)\n"
"--\n"
"\n"
"Maxheap variant of heapify.");

#define _HEAPQ_HEAPIFY_MAX_METHODDEF    \
    {"heapify_max", _PyCFunction_CAST(_heapq_heapify_max), METH_VALUE|METH_O, _heapq_heapify_max__doc__},

static PyValue
_heapq_heapify_max_impl(PyObject *module, PyObject *heap);

static PyValue
_heapq_heapify_max(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heapify_max", "argument", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heapify_max_impl(PyValue_AsKnownObjectBorrow(module), heap);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_heapq_heappushpop_max__doc__,
"heappushpop_max($module, heap, item, /)\n"
"--\n"
"\n"
"Maxheap variant of heappushpop.\n"
"\n"
"The combined action runs more efficiently than heappush_max()\n"
"followed by a separate call to heappop_max().");

#define _HEAPQ_HEAPPUSHPOP_MAX_METHODDEF    \
    {"heappushpop_max", _PyCFunction_CAST(_heapq_heappushpop_max), METH_VALUE, _heapq_heappushpop_max__doc__},

static PyValue
_heapq_heappushpop_max_impl(PyObject *module, PyObject *heap,
                            PyValueBorrow item);

static PyValue
_heapq_heappushpop_max(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *heap;
    PyValueBorrow item;

    if (!_PyArg_CheckPositional("heappushpop_max", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyList_Type)) {
        _PyArg_BadArgumentValue("heappushpop_max", "argument 1", "list", PyValue_Borrow(args[0]));
        goto exit;
    }
    heap = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    item = PyValue_Borrow(args[1]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(heap));
    return_value = _heapq_heappushpop_max_impl(PyValue_AsKnownObjectBorrow(module), heap, item);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
/*[clinic end generated code: output=0e9f37efb53f89f5 input=a9049054013a1b77]*/
