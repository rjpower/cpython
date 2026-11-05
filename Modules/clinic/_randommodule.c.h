/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsNativeBytes()
#include "pycore_modsupport.h"    // _PyArg_NoKwnamesValue()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_random_Random_random__doc__,
"random($self, /)\n"
"--\n"
"\n"
"random() -> x in the interval [0, 1).");

#define _RANDOM_RANDOM_RANDOM_METHODDEF    \
    {"random", _PyCFunction_CAST(_random_Random_random), METH_VALUE|METH_NOARGS, _random_Random_random__doc__},

static PyValue
_random_Random_random_impl(RandomObject *self);

static PyValue
_random_Random_random(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _random_Random_random_impl((RandomObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_random_Random_seed__doc__,
"seed($self, n=None, /)\n"
"--\n"
"\n"
"seed([n]) -> None.\n"
"\n"
"Defaults to use urandom and falls back to a combination\n"
"of the current time and the process identifier.");

#define _RANDOM_RANDOM_SEED_METHODDEF    \
    {"seed", _PyCFunction_CAST(_random_Random_seed), METH_VALUE, _random_Random_seed__doc__},

static PyValue
_random_Random_seed_impl(RandomObject *self, PyValueBorrow n);

static PyValue
_random_Random_seed(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow n = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("Random.seed()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("seed", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    n = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _random_Random_seed_impl((RandomObject *)PyValue_AsKnownObjectBorrow(self), n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_random_Random_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n"
"getstate() -> tuple containing the current state.");

#define _RANDOM_RANDOM_GETSTATE_METHODDEF    \
    {"getstate", _PyCFunction_CAST(_random_Random_getstate), METH_VALUE|METH_NOARGS, _random_Random_getstate__doc__},

static PyValue
_random_Random_getstate_impl(RandomObject *self);

static PyValue
_random_Random_getstate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _random_Random_getstate_impl((RandomObject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_random_Random_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n"
"setstate(state) -> None.  Restores generator state.");

#define _RANDOM_RANDOM_SETSTATE_METHODDEF    \
    {"setstate", _PyCFunction_CAST(_random_Random_setstate), METH_VALUE|METH_O, _random_Random_setstate__doc__},

static PyValue
_random_Random_setstate_impl(RandomObject *self, PyValueBorrow state);

static PyValue
_random_Random_setstate(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow state;

    state = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _random_Random_setstate_impl((RandomObject *)PyValue_AsKnownObjectBorrow(self), state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_random_Random_getrandbits__doc__,
"getrandbits($self, k, /)\n"
"--\n"
"\n"
"getrandbits(k) -> x.  Generates an int with k random bits.");

#define _RANDOM_RANDOM_GETRANDBITS_METHODDEF    \
    {"getrandbits", _PyCFunction_CAST(_random_Random_getrandbits), METH_VALUE|METH_O, _random_Random_getrandbits__doc__},

static PyValue
_random_Random_getrandbits_impl(RandomObject *self, uint64_t k);

static PyValue
_random_Random_getrandbits(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    uint64_t k;

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &k, sizeof(uint64_t),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(uint64_t)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C uint64_t");
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _random_Random_getrandbits_impl((RandomObject *)PyValue_AsKnownObjectBorrow(self), k);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
random_init_impl(RandomObject *self, PyValueBorrow seed);

static int
random_init_value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    PyTypeObject *base_tp = (PyTypeObject *)_randomstate_type(Py_TYPE(self))->Random_Type;
    PyValueBorrow seed = PyValue_Borrow(PyValue_NULL);

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKwnames("Random", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Random", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    seed = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = random_init_impl((RandomObject *)self, seed);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
random_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return random_init_value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = random_init_value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}
/*[clinic end generated code: output=802a691f680a3386 input=a9049054013a1b77]*/
