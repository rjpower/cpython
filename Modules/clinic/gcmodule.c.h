/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#  include "pycore_value.h"       // _PyValue_STATIC_IMMORTAL()
#endif
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywordsValue()
#include "pycore_tuple.h"         // _PyTuple_FromValueArray()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(gc_enable__doc__,
"enable($module, /)\n"
"--\n"
"\n"
"Enable automatic garbage collection.");

#define GC_ENABLE_METHODDEF    \
    {"enable", _PyCFunction_CAST(gc_enable), METH_VALUE|METH_NOARGS, gc_enable__doc__},

static PyValue
gc_enable_impl(PyObject *module);

static PyValue
gc_enable(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = gc_enable_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(gc_disable__doc__,
"disable($module, /)\n"
"--\n"
"\n"
"Disable automatic garbage collection.");

#define GC_DISABLE_METHODDEF    \
    {"disable", _PyCFunction_CAST(gc_disable), METH_VALUE|METH_NOARGS, gc_disable__doc__},

static PyValue
gc_disable_impl(PyObject *module);

static PyValue
gc_disable(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = gc_disable_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(gc_isenabled__doc__,
"isenabled($module, /)\n"
"--\n"
"\n"
"Returns true if automatic garbage collection is enabled.");

#define GC_ISENABLED_METHODDEF    \
    {"isenabled", _PyCFunction_CAST(gc_isenabled), METH_VALUE|METH_NOARGS, gc_isenabled__doc__},

static int
gc_isenabled_impl(PyObject *module);

static PyValue
gc_isenabled(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    _return_value = gc_isenabled_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(gc_collect__doc__,
"collect($module, /, generation=2)\n"
"--\n"
"\n"
"Run the garbage collector.\n"
"\n"
"With no arguments, run a full collection.  The optional argument\n"
"may be an integer specifying which generation to collect.  A ValueError\n"
"is raised if the generation number is invalid.\n"
"\n"
"The number of unreachable objects is returned.");

#define GC_COLLECT_METHODDEF    \
    {"collect", _PyCFunction_CAST(gc_collect), METH_VALUE|METH_KEYWORDS, gc_collect__doc__},

static Py_ssize_t
gc_collect_impl(PyObject *module, int generation);

static PyValue
gc_collect(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(generation)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"generation", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "collect",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int generation = NUM_GENERATIONS - 1;
    Py_ssize_t _return_value;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        generation = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
        if (generation == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = gc_collect_impl(PyValue_AsKnownObjectBorrow(module), generation);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_set_debug__doc__,
"set_debug($module, flags, /)\n"
"--\n"
"\n"
"Set the garbage collection debugging flags.\n"
"\n"
"  flags\n"
"    An integer that can have the following bits turned on:\n"
"      DEBUG_STATS - Print statistics during collection.\n"
"      DEBUG_COLLECTABLE - Print collectable objects found.\n"
"      DEBUG_UNCOLLECTABLE - Print unreachable but uncollectable objects\n"
"        found.\n"
"      DEBUG_SAVEALL - Save objects to gc.garbage rather than freeing them.\n"
"      DEBUG_LEAK - Debug leaking programs (everything but STATS).\n"
"\n"
"Debugging information is written to sys.stderr.");

#define GC_SET_DEBUG_METHODDEF    \
    {"set_debug", _PyCFunction_CAST(gc_set_debug), METH_VALUE|METH_O, gc_set_debug__doc__},

static PyValue
gc_set_debug_impl(PyObject *module, int flags);

static PyValue
gc_set_debug(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flags;

    flags = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = gc_set_debug_impl(PyValue_AsKnownObjectBorrow(module), flags);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_get_debug__doc__,
"get_debug($module, /)\n"
"--\n"
"\n"
"Get the garbage collection debugging flags.");

#define GC_GET_DEBUG_METHODDEF    \
    {"get_debug", _PyCFunction_CAST(gc_get_debug), METH_VALUE|METH_NOARGS, gc_get_debug__doc__},

static int
gc_get_debug_impl(PyObject *module);

static PyValue
gc_get_debug(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    _return_value = gc_get_debug_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_set_threshold__doc__,
"set_threshold(threshold0, [threshold1, [threshold2]])\n"
"Set the collection thresholds (the collection frequency).\n"
"\n"
"Setting \'threshold0\' to zero disables collection.");

#define GC_SET_THRESHOLD_METHODDEF    \
    {"set_threshold", _PyCFunction_CAST(gc_set_threshold), METH_FASTCALL, gc_set_threshold__doc__},

static PyObject *
gc_set_threshold_impl(PyObject *module, int threshold0, int group_right_1,
                      int threshold1, int group_right_2, int threshold2);

static PyObject *
gc_set_threshold(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int threshold0;
    int group_right_1 = 0;
    int threshold1 = 0;
    int group_right_2 = 0;
    int threshold2 = 0;

    if (nargs < 1 || nargs > 3) {
        PyErr_SetString(PyExc_TypeError, "gc.set_threshold requires 1 to 3 arguments");
        goto exit;
    }
    threshold0 = PyLong_AsInt(args[0]);
    if (threshold0 == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs >= 2) {
        threshold1 = PyLong_AsInt(args[1]);
        if (threshold1 == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    if (nargs >= 3) {
        threshold2 = PyLong_AsInt(args[2]);
        if (threshold2 == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_2 = 1;
    }
    return_value = gc_set_threshold_impl(module, threshold0, group_right_1, threshold1, group_right_2, threshold2);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_get_threshold__doc__,
"get_threshold($module, /)\n"
"--\n"
"\n"
"Return the current collection thresholds.");

#define GC_GET_THRESHOLD_METHODDEF    \
    {"get_threshold", _PyCFunction_CAST(gc_get_threshold), METH_VALUE|METH_NOARGS, gc_get_threshold__doc__},

static PyValue
gc_get_threshold_impl(PyObject *module);

static PyValue
gc_get_threshold(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = gc_get_threshold_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(gc_get_count__doc__,
"get_count($module, /)\n"
"--\n"
"\n"
"Return a three-tuple of the current collection counts.");

#define GC_GET_COUNT_METHODDEF    \
    {"get_count", _PyCFunction_CAST(gc_get_count), METH_VALUE|METH_NOARGS, gc_get_count__doc__},

static PyValue
gc_get_count_impl(PyObject *module);

static PyValue
gc_get_count(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = gc_get_count_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(gc_get_referrers__doc__,
"get_referrers($module, /, *objs)\n"
"--\n"
"\n"
"Return the list of objects that directly refer to any of \'objs\'.");

#define GC_GET_REFERRERS_METHODDEF    \
    {"get_referrers", _PyCFunction_CAST(gc_get_referrers), METH_VALUE, gc_get_referrers__doc__},

static PyValue
gc_get_referrers_impl(PyObject *module, PyObject *objs);

static PyValue
gc_get_referrers(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *objs = NULL;

    objs = _PyTuple_FromValueArray(args, nargs);
    if (objs == NULL) {
        goto exit;
    }
    return_value = gc_get_referrers_impl(PyValue_AsKnownObjectBorrow(module), objs);

exit:
    /* Cleanup for objs */
    Py_XDECREF(objs);

    return return_value;
}

PyDoc_STRVAR(gc_get_referents__doc__,
"get_referents($module, /, *objs)\n"
"--\n"
"\n"
"Return the list of objects that are directly referred to by \'objs\'.");

#define GC_GET_REFERENTS_METHODDEF    \
    {"get_referents", _PyCFunction_CAST(gc_get_referents), METH_VALUE, gc_get_referents__doc__},

static PyValue
gc_get_referents_impl(PyObject *module, PyObject *objs);

static PyValue
gc_get_referents(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *objs = NULL;

    objs = _PyTuple_FromValueArray(args, nargs);
    if (objs == NULL) {
        goto exit;
    }
    return_value = gc_get_referents_impl(PyValue_AsKnownObjectBorrow(module), objs);

exit:
    /* Cleanup for objs */
    Py_XDECREF(objs);

    return return_value;
}

PyDoc_STRVAR(gc_get_objects__doc__,
"get_objects($module, /, generation=None)\n"
"--\n"
"\n"
"Return a list of objects tracked by the collector (excluding the list returned).\n"
"\n"
"  generation\n"
"    Generation to extract the objects from.\n"
"\n"
"If generation is not None, return only the objects tracked by the\n"
"collector that are in that generation.");

#define GC_GET_OBJECTS_METHODDEF    \
    {"get_objects", _PyCFunction_CAST(gc_get_objects), METH_VALUE|METH_KEYWORDS, gc_get_objects__doc__},

static PyValue
gc_get_objects_impl(PyObject *module, Py_ssize_t generation);

static PyValue
gc_get_objects(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(generation)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"generation", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_objects",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    Py_ssize_t generation = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[_idx[0]]), &generation)) {
            goto exit;
        }
    }
    return_value = gc_get_objects_impl(PyValue_AsKnownObjectBorrow(module), generation);

exit:
    return return_value;
}

PyDoc_STRVAR(gc_get_stats__doc__,
"get_stats($module, /)\n"
"--\n"
"\n"
"Return a list of dictionaries containing per-generation statistics.");

#define GC_GET_STATS_METHODDEF    \
    {"get_stats", _PyCFunction_CAST(gc_get_stats), METH_VALUE|METH_NOARGS, gc_get_stats__doc__},

static PyValue
gc_get_stats_impl(PyObject *module);

static PyValue
gc_get_stats(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = gc_get_stats_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(gc_is_tracked__doc__,
"is_tracked($module, obj, /)\n"
"--\n"
"\n"
"Returns true if the object is tracked by the garbage collector.\n"
"\n"
"Simple atomic objects will return false.");

#define GC_IS_TRACKED_METHODDEF    \
    {"is_tracked", _PyCFunction_CAST(gc_is_tracked), METH_VALUE|METH_O, gc_is_tracked__doc__},

static int
gc_is_tracked_impl(PyObject *module, PyValueBorrow obj);

static PyValue
gc_is_tracked(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;
    int _return_value;

    obj = PyValue_Borrow(args[0]);
    _return_value = gc_is_tracked_impl(PyValue_AsKnownObjectBorrow(module), obj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(gc_is_finalized__doc__,
"is_finalized($module, obj, /)\n"
"--\n"
"\n"
"Returns true if the object has been already finalized by the GC.");

#define GC_IS_FINALIZED_METHODDEF    \
    {"is_finalized", _PyCFunction_CAST(gc_is_finalized), METH_VALUE|METH_O, gc_is_finalized__doc__},

static int
gc_is_finalized_impl(PyObject *module, PyValueBorrow obj);

static PyValue
gc_is_finalized(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;
    int _return_value;

    obj = PyValue_Borrow(args[0]);
    _return_value = gc_is_finalized_impl(PyValue_AsKnownObjectBorrow(module), obj);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

PyDoc_STRVAR(gc_freeze__doc__,
"freeze($module, /)\n"
"--\n"
"\n"
"Freeze all current tracked objects and ignore them for future collections.\n"
"\n"
"This can be used before a POSIX fork() call to make the gc copy-on-write\n"
"friendly.\n"
"Note: collection before a POSIX fork() call may free pages for future\n"
"allocation which can cause copy-on-write.");

#define GC_FREEZE_METHODDEF    \
    {"freeze", _PyCFunction_CAST(gc_freeze), METH_VALUE|METH_NOARGS, gc_freeze__doc__},

static PyValue
gc_freeze_impl(PyObject *module);

static PyValue
gc_freeze(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = gc_freeze_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(gc_unfreeze__doc__,
"unfreeze($module, /)\n"
"--\n"
"\n"
"Unfreeze all objects in the permanent generation.\n"
"\n"
"Put all objects in the permanent generation back into oldest generation.");

#define GC_UNFREEZE_METHODDEF    \
    {"unfreeze", _PyCFunction_CAST(gc_unfreeze), METH_VALUE|METH_NOARGS, gc_unfreeze__doc__},

static PyValue
gc_unfreeze_impl(PyObject *module);

static PyValue
gc_unfreeze(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = gc_unfreeze_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(gc_get_freeze_count__doc__,
"get_freeze_count($module, /)\n"
"--\n"
"\n"
"Return the number of objects in the permanent generation.");

#define GC_GET_FREEZE_COUNT_METHODDEF    \
    {"get_freeze_count", _PyCFunction_CAST(gc_get_freeze_count), METH_VALUE|METH_NOARGS, gc_get_freeze_count__doc__},

static Py_ssize_t
gc_get_freeze_count_impl(PyObject *module);

static PyValue
gc_get_freeze_count(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t _return_value;

    _return_value = gc_get_freeze_count_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=dda9a8678dbe1423 input=a9049054013a1b77]*/
