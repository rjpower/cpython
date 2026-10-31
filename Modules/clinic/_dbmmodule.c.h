/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()

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
    {"close", (PyCFunction)_dbm_dbm_close, METH_NOARGS, _dbm_dbm_close__doc__},

static PyObject *
_dbm_dbm_close_impl(dbmobject *self);

static PyObject *
_dbm_dbm_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _dbm_dbm_close_impl((dbmobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_dbm_dbm_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Return a list of all keys in the database.");

#define _DBM_DBM_KEYS_METHODDEF    \
    {"keys", _PyCFunction_CAST(_dbm_dbm_keys), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _dbm_dbm_keys__doc__},

static PyObject *
_dbm_dbm_keys_impl(dbmobject *self, PyTypeObject *cls);

static PyValue
_dbm_dbm_keys(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "keys() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _dbm_dbm_keys_impl((dbmobject *)self, cls);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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

static PyValue
_dbm_dbm_get(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *key;
    Py_ssize_t key_length;
    PyObject *default_value = Py_None;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("get", kwnames)) {
        goto exit;
    }
    if (!PyArg_ParseArray(args, nargs, "s#|O:get",
        &key, &key_length, &default_value)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _dbm_dbm_get_impl((dbmobject *)self, cls, key, key_length, default_value);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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

static PyValue
_dbm_dbm_setdefault(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *key;
    Py_ssize_t key_length;
    PyObject *default_value = NULL;
    PyObject *_return_value;

    if (!_PyArg_NoKwnames("setdefault", kwnames)) {
        goto exit;
    }
    if (!PyArg_ParseArray(args, nargs, "s#|O:setdefault",
        &key, &key_length, &default_value)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _dbm_dbm_setdefault_impl((dbmobject *)self, cls, key, key_length, default_value);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_dbm_dbm_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the database.");

#define _DBM_DBM_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(_dbm_dbm_clear), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _dbm_dbm_clear__doc__},

static PyObject *
_dbm_dbm_clear_impl(dbmobject *self, PyTypeObject *cls);

static PyValue
_dbm_dbm_clear(PyObject *self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *_return_value;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "clear() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _dbm_dbm_clear_impl((dbmobject *)self, cls);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_dbm_dbm___exit____doc__,
"__exit__($self, /, *exc_info)\n"
"--\n"
"\n"
"Close the database.");

#define _DBM_DBM___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_dbm_dbm___exit__), METH_FASTCALL, _dbm_dbm___exit____doc__},

static PyObject *
_dbm_dbm___exit___impl(dbmobject *self, const PyValue *exc_info,
                       Py_ssize_t exc_info_length);

static PyValue
_dbm_dbm___exit__(PyObject *self, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *exc_info;
    Py_ssize_t exc_info_length;
    PyObject *_return_value;

    exc_info = args;
    exc_info_length = nargs;
    Py_BEGIN_CRITICAL_SECTION(self);
    _return_value = _dbm_dbm___exit___impl((dbmobject *)self, exc_info, exc_info_length);
    Py_END_CRITICAL_SECTION();
    return_value = PyValue_FromResult(_return_value);

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
    {"open", _PyCFunction_CAST(dbmopen), METH_FASTCALL, dbmopen__doc__},

static PyObject *
dbmopen_impl(PyObject *module, PyObject *filename, const char *flags,
             int mode);

static PyValue
dbmopen(PyObject *module, const PyValue *args, Py_ssize_t nargs)
{
    PyValue return_value = PyValue_NULL;
    PyObject *filename;
    const char *flags = "r";
    int mode = 438;
    PyObject *_return_value;

    if (!_PyArg_CheckPositional("open", nargs, 1, 3)) {
        goto exit;
    }
    filename = PyValue_Object(args[0]);
    if (filename == NULL) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        PyObject *_arg = PyValue_Object(args[1]);
        if (_arg == NULL) {
            goto exit;
        }
        if (!PyUnicode_Check(_arg)) {
            _PyArg_BadArgument("open", "argument 2", "str", _arg);
            goto exit;
        }
        flags = _PyUnicode_AsUTF8NoNUL(_arg);
        if (flags == NULL) {
            goto exit;
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    mode = PyLong_AsIntValue(args[2]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    _return_value = dbmopen_impl(module, filename, flags, mode);
    return_value = PyValue_FromResult(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=bbc280128169e5c9 input=a9049054013a1b77]*/
