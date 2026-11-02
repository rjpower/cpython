#include "parts.h"
#include "util.h"

typedef struct {
    PyMemAllocatorEx saved;
    int fail_next;
} ListBridgeAllocator;

static int
list_bridge_should_fail(ListBridgeAllocator *allocator)
{
    int fail = allocator->fail_next;
    allocator->fail_next = 0;
    return fail;
}

static void *
list_bridge_malloc(void *context, size_t size)
{
    ListBridgeAllocator *allocator = context;
    if (list_bridge_should_fail(allocator)) return NULL;
    return allocator->saved.malloc(allocator->saved.ctx, size);
}

static void *
list_bridge_calloc(void *context, size_t count, size_t size)
{
    ListBridgeAllocator *allocator = context;
    if (list_bridge_should_fail(allocator)) return NULL;
    return allocator->saved.calloc(allocator->saved.ctx, count, size);
}

static void *
list_bridge_realloc(void *context, void *pointer, size_t size)
{
    ListBridgeAllocator *allocator = context;
    if (list_bridge_should_fail(allocator)) return NULL;
    return allocator->saved.realloc(allocator->saved.ctx, pointer, size);
}

static void
list_bridge_free(void *context, void *pointer)
{
    ListBridgeAllocator *allocator = context;
    allocator->saved.free(allocator->saved.ctx, pointer);
}

static void
list_bridge_install(ListBridgeAllocator *context)
{
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &context->saved);
    context->fail_next = 1;
    PyMemAllocatorEx allocator = {
        context, list_bridge_malloc, list_bridge_calloc,
        list_bridge_realloc, list_bridge_free
    };
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator);
}

static PyObject *
list_get_item_oom(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) return NULL;
    ListBridgeAllocator allocator;
    list_bridge_install(&allocator);
    PyObject *result = PyList_GetItemRef(obj, index);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator.saved);
    if (allocator.fail_next) {
        Py_XDECREF(result);
        PyErr_SetString(PyExc_AssertionError, "getter did not reach the boxing allocator");
        return NULL;
    }
    return result;
}

static PyObject *
list_next_oom(PyObject *Py_UNUSED(module), PyObject *iterator)
{
    ListBridgeAllocator allocator;
    list_bridge_install(&allocator);
    PyObject *result = PyIter_Next(iterator);
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &allocator.saved);
    if (allocator.fail_next) {
        Py_XDECREF(result);
        PyErr_SetString(PyExc_AssertionError, "iterator did not reach the boxing allocator");
        return NULL;
    }
    return result;
}


static PyObject *
list_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyList_GET_SIZE(obj));
}


static PyObject *
list_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return PyValue_AsObject(PyList_GET_VALUE(obj, i));
}

static PyObject *
list_get_value(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) return NULL;
    PyValue value = PyList_GetValue(obj, index);
    if (!PyValue_IsLanguageValue(value)) return NULL;
    PyObject *result = PyValue_AsObject(value);
    PyValue_Close(value);
    return result;
}

static PyObject *
list_get_value_and_clear(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) return NULL;
    PyValue value = PyList_GetValue(obj, index);
    if (!PyValue_IsLanguageValue(value)) return NULL;
    if (PyList_Clear(obj) < 0) {
        PyValue_Close(value);
        return NULL;
    }
    PyObject *result = PyValue_AsObject(value);
    PyValue_Close(value);
    return result;
}

static PyObject *
list_value_bits(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t index;
    if (!PyArg_ParseTuple(args, "On", &obj, &index)) return NULL;
    PyValue value = PyList_GetValue(obj, index);
    if (!PyValue_IsLanguageValue(value)) return NULL;
    uint64_t bits = value.bits;
    PyValue_Close(value);
    return PyLong_FromUnsignedLongLong(bits);
}

static PyObject *
list_unpublished_nulls(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *obj = PyList_New(3);
    if (obj == NULL) return NULL;
    int valid = 1;
    for (Py_ssize_t i = 0; i < 3; i++) {
        valid &= ((PyListObject *)obj)->ob_item[i].bits == 1;
        PyValue value = PyList_GetValue(obj, i);
        valid &= value.bits == 1 && !PyErr_Occurred();
        PyObject *object = PyList_GetItemRef(obj, i);
        valid &= object == NULL && !PyErr_Occurred();
        Py_XDECREF(object);
    }
    Py_DECREF(obj);
    return PyBool_FromLong(valid);
}

static PyObject *
list_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    PyList_SET_ITEM(obj, i, Py_XNewRef(value));
    Py_RETURN_NONE;

}


static PyObject *
list_clear(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Clear(obj));
}


static PyObject *
list_extend(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *arg;
    if (!PyArg_ParseTuple(args, "OO", &obj, &arg)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(arg);
    RETURN_INT(PyList_Extend(obj, arg));
}


static PyObject*
test_list_api(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* list;
    int i;

    /* SF bug 132008:  PyList_Reverse segfaults */
#define NLIST 30
    list = PyList_New(NLIST);
    if (list == (PyObject*)NULL)
        return (PyObject*)NULL;
    /* list = range(NLIST) */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyLong_FromLong(i);
        if (anint == (PyObject*)NULL) {
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
        PyList_SET_ITEM(list, i, anint);
    }
    /* list.reverse(), via PyList_Reverse() */
    i = PyList_Reverse(list);   /* should not blow up! */
    if (i != 0) {
        Py_DECREF(list);
        return (PyObject*)NULL;
    }
    /* Check that list == range(29, -1, -1) now */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyList_GetItemRef(list, i);
        if (anint == NULL) {
            Py_DECREF(list);
            return NULL;
        }
        if (PyLong_AS_LONG(anint) != NLIST-1-i) {
            Py_DECREF(anint);
            PyErr_SetString(PyExc_AssertionError,
                            "test_list_api: reverse screwed up");
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
        Py_DECREF(anint);
    }
    Py_DECREF(list);
#undef NLIST

    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"list_get_size", list_get_size, METH_O},
    {"list_get_item", list_get_item, METH_VARARGS},
    {"list_get_item_oom", list_get_item_oom, METH_VARARGS},
    {"list_next_oom", list_next_oom, METH_O},
    {"list_get_value", list_get_value, METH_VARARGS},
    {"list_get_value_and_clear", list_get_value_and_clear, METH_VARARGS},
    {"list_value_bits", list_value_bits, METH_VARARGS},
    {"list_unpublished_nulls", list_unpublished_nulls, METH_NOARGS},
    {"list_set_item", list_set_item, METH_VARARGS},
    {"list_clear", list_clear, METH_O},
    {"list_extend", list_extend, METH_VARARGS},
    {"test_list_api", test_list_api, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
