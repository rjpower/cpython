/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long.h"          // _PyLong_UInt64_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_random_Random_random__doc__,
"random($self, /)\n"
"--\n"
"\n"
"random() -> x in the interval [0, 1).");

#define _RANDOM_RANDOM_RANDOM_METHODDEF    \
    {"random", (PyCFunction)_random_Random_random, METH_NOARGS, _random_Random_random__doc__},

static PyObject *
_random_Random_random_impl(RandomObject *self);

static PyObject *
_random_Random_random(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _random_Random_random_impl((RandomObject *)self);
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
    {"seed", _PyCFunction_CAST(_random_Random_seed), METH_FASTCALL, _random_Random_seed__doc__},

static PyObject *
_random_Random_seed_impl(RandomObject *self, PyObject *n);

static PyValue
_random_Random_seed(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *n = Py_None;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("seed", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    n = PyValue_Object(args[0]);
    if (n == NULL) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _random_Random_seed_impl((RandomObject *)self, n);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_random_Random_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n"
"getstate() -> tuple containing the current state.");

#define _RANDOM_RANDOM_GETSTATE_METHODDEF    \
    {"getstate", (PyCFunction)_random_Random_getstate, METH_NOARGS, _random_Random_getstate__doc__},

static PyObject *
_random_Random_getstate_impl(RandomObject *self);

static PyObject *
_random_Random_getstate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _random_Random_getstate_impl((RandomObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_random_Random_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n"
"setstate(state) -> None.  Restores generator state.");

#define _RANDOM_RANDOM_SETSTATE_METHODDEF    \
    {"setstate", _PyCFunction_CAST(_random_Random_setstate), METH_O, _random_Random_setstate__doc__},

static PyObject *
_random_Random_setstate_impl(RandomObject *self, PyObject *state);

static PyValue
_random_Random_setstate(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    PyObject *state;
    PyObject *_return_value;

    state = PyValue_Object(arg);
    if (state == NULL) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _random_Random_setstate_impl((RandomObject *)self, state);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_random_Random_getrandbits__doc__,
"getrandbits($self, k, /)\n"
"--\n"
"\n"
"getrandbits(k) -> x.  Generates an int with k random bits.");

#define _RANDOM_RANDOM_GETRANDBITS_METHODDEF    \
    {"getrandbits", _PyCFunction_CAST(_random_Random_getrandbits), METH_O, _random_Random_getrandbits__doc__},

static PyObject *
_random_Random_getrandbits_impl(RandomObject *self, uint64_t k);

static PyValue
_random_Random_getrandbits(PyObject *self, PyValue arg)
{
    PyValue return_value = PyValue_NULL;
    uint64_t k;
    PyObject *_return_value;

    {
        PyObject *_arg = PyValue_Object(arg);
        if (_arg == NULL) {
            goto exit;
        }
        if (!_PyLong_UInt64_Converter(_arg, &k)) {
            goto exit;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _random_Random_getrandbits_impl((RandomObject *)self, k);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

static int
random_init_impl(RandomObject *self, PyObject *seed);

static int
random_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    PyTypeObject *base_tp = (PyTypeObject *)_randomstate_type(Py_TYPE(self))->Random_Type;
    PyObject *seed = NULL;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKeywords("Random", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("Random", PyTuple_GET_SIZE(args), 0, 1)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        goto skip_optional;
    }
    seed = PyValue_Object(PyTuple_GET_VALUE(args, 0));
    if (seed == NULL) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = random_init_impl((RandomObject *)self, seed);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
/*[clinic end generated code: output=da1b61e0ba03f106 input=a9049054013a1b77]*/
