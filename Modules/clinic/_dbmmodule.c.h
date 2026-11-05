/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE

static Py_ssize_t
dbm_length_impl(dbmobject *self);

static Py_ssize_t
dbm_length(PyObject *self)
{
    Py_ssize_t return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = dbm_length_impl((dbmobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
dbm_bool_impl(dbmobject *self);

static int
dbm_bool(PyObject *self)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = dbm_bool_impl((dbmobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
dbm_subscript_impl(dbmobject *self, PyObject *key);

static PyObject *
dbm_subscript(PyObject *self, PyObject *key)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = dbm_subscript_impl((dbmobject *)self, key);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
dbm_ass_sub_impl(dbmobject *self, PyObject *v, PyObject *w);

static int
dbm_ass_sub(PyObject *self, PyObject *v, PyObject *w)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = dbm_ass_sub_impl((dbmobject *)self, v, w);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_dbm_dbm_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the database.");

#define _DBM_DBM_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_dbm_dbm_close), METH_VALUE|METH_NOARGS, _dbm_dbm_close__doc__},

static PyValue
_dbm_dbm_close_impl(dbmobject *self);

static PyValue
_dbm_dbm_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _dbm_dbm_close_impl((dbmobject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_dbm_dbm_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Return a list of all keys in the database.");

#define _DBM_DBM_KEYS_METHODDEF    \
    {"keys", _PyCFunction_CAST(_dbm_dbm_keys), METH_VALUE|METH_METHOD|METH_KEYWORDS, _dbm_dbm_keys__doc__},

static PyValue
_dbm_dbm_keys_impl(dbmobject *self, PyTypeObject *cls);

static PyValue
_dbm_dbm_keys(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "keys() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _dbm_dbm_keys_impl((dbmobject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
dbm_contains_impl(dbmobject *self, PyObject *arg);

static int
dbm_contains(PyObject *self, PyObject *arg)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = dbm_contains_impl((dbmobject *)self, arg);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_dbm_dbm_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Return the value for key if present, otherwise default.");

#define _DBM_DBM_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_dbm_dbm_get), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _dbm_dbm_get__doc__},

static PyObject *
_dbm_dbm_get_impl(dbmobject *self, PyTypeObject *cls, const char *key,
                  Py_ssize_t key_length, PyObject *default_value);

static PyObject *
_dbm_dbm_get(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    const char *key;
    Py_ssize_t key_length;
    PyObject *default_value = Py_None;

    if (!_PyArg_NoKwnames("get", kwnames)) {
        goto exit;
    }
    if (!PyArg_ParseArray(args, nargs, "s#|O:get",
        &key, &key_length, &default_value)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _dbm_dbm_get_impl((dbmobject *)self, cls, key, key_length, default_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_dbm_dbm_setdefault__doc__,
"setdefault($self, key, default=b\'\', /)\n"
"--\n"
"\n"
"Return the value for key if present, otherwise default.\n"
"\n"
"If key is not in the database, it is inserted with default as the\n"
"value.");

#define _DBM_DBM_SETDEFAULT_METHODDEF    \
    {"setdefault", _PyCFunction_CAST(_dbm_dbm_setdefault), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _dbm_dbm_setdefault__doc__},

static PyObject *
_dbm_dbm_setdefault_impl(dbmobject *self, PyTypeObject *cls, const char *key,
                         Py_ssize_t key_length, PyObject *default_value);

static PyObject *
_dbm_dbm_setdefault(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    const char *key;
    Py_ssize_t key_length;
    PyObject *default_value = NULL;

    if (!_PyArg_NoKwnames("setdefault", kwnames)) {
        goto exit;
    }
    if (!PyArg_ParseArray(args, nargs, "s#|O:setdefault",
        &key, &key_length, &default_value)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _dbm_dbm_setdefault_impl((dbmobject *)self, cls, key, key_length, default_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_dbm_dbm_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the database.");

#define _DBM_DBM_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(_dbm_dbm_clear), METH_VALUE|METH_METHOD|METH_KEYWORDS, _dbm_dbm_clear__doc__},

static PyValue
_dbm_dbm_clear_impl(dbmobject *self, PyTypeObject *cls);

static PyValue
_dbm_dbm_clear(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "clear() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _dbm_dbm_clear_impl((dbmobject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_dbm_dbm___exit____doc__,
"__exit__($self, /, *exc_info)\n"
"--\n"
"\n"
"Close the database.");

#define _DBM_DBM___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_dbm_dbm___exit__), METH_VALUE, _dbm_dbm___exit____doc__},

static PyValue
_dbm_dbm___exit___impl(dbmobject *self, const PyValue *exc_info,
                       Py_ssize_t exc_info_length);

static PyValue
_dbm_dbm___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *exc_info;
    Py_ssize_t exc_info_length;

    exc_info = args;
    exc_info_length = nargs;
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _dbm_dbm___exit___impl((dbmobject *)PyValue_AsKnownObjectBorrow(self), exc_info, exc_info_length);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(dbmopen__doc__,
"open($module, filename, flags=\'r\', mode=0o666, /)\n"
"--\n"
"\n"
"Return a database object.\n"
"\n"
"  filename\n"
"    The filename to open.\n"
"  flags\n"
"    How to open the file.  \"r\" for reading, \"w\" for writing, etc.\n"
"  mode\n"
"    If creating a new file, the mode bits for the new file\n"
"    (e.g. os.O_RDWR).");

#define DBMOPEN_METHODDEF    \
    {"open", _PyCFunction_CAST(dbmopen), METH_VALUE, dbmopen__doc__},

static PyValue
dbmopen_impl(PyObject *module, PyValueBorrow filename, const char *flags,
             int mode);

static PyValue
dbmopen(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filename;
    const char *flags = "r";
    int mode = 438;

    if (!_PyArg_CheckPositional("open", nargs, 1, 3)) {
        goto exit;
    }
    filename = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("open", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("open", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    flags = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
    if (flags == NULL) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    mode = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = dbmopen_impl(PyValue_AsKnownObjectBorrow(module), filename, flags, mode);

exit:
    return return_value;
}
/*[clinic end generated code: output=555c0c994e3f2aab input=a9049054013a1b77]*/
