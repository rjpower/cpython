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
gdbm_length_impl(gdbmobject *self);

static Py_ssize_t
gdbm_length(PyObject *self)
{
    Py_ssize_t return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gdbm_length_impl((gdbmobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static int
gdbm_bool_impl(gdbmobject *self);

static int
gdbm_bool(PyObject *self)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gdbm_bool_impl((gdbmobject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

static PyObject *
gdbm_subscript_impl(gdbmobject *self, PyObject *key);

static PyObject *
gdbm_subscript(PyObject *self, PyObject *key)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gdbm_subscript_impl((gdbmobject *)self, key);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_get__doc__,
"get($self, key, default=None, /)\n"
"--\n"
"\n"
"Get the value for key, or default if not present.");

#define _GDBM_GDBM_GET_METHODDEF    \
    {"get", _PyCFunction_CAST(_gdbm_gdbm_get), METH_VALUE, _gdbm_gdbm_get__doc__},

static PyValue
_gdbm_gdbm_get_impl(gdbmobject *self, PyValueBorrow key,
                    PyValueBorrow default_value);

static PyValue
_gdbm_gdbm_get(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("get", nargs, 1, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Borrow(args[1]);
skip_optional:
    return_value = _gdbm_gdbm_get_impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self), key, default_value);

exit:
    return return_value;
}

static int
gdbm_ass_sub_impl(gdbmobject *self, PyObject *v, PyObject *w);

static int
gdbm_ass_sub(PyObject *self, PyObject *v, PyObject *w)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gdbm_ass_sub_impl((gdbmobject *)self, v, w);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_setdefault__doc__,
"setdefault($self, key, default=None, /)\n"
"--\n"
"\n"
"Get value for key, or set it to default and return default if not present.");

#define _GDBM_GDBM_SETDEFAULT_METHODDEF    \
    {"setdefault", _PyCFunction_CAST(_gdbm_gdbm_setdefault), METH_VALUE, _gdbm_gdbm_setdefault__doc__},

static PyValue
_gdbm_gdbm_setdefault_impl(gdbmobject *self, PyValueBorrow key,
                           PyValueBorrow default_value);

static PyValue
_gdbm_gdbm_setdefault(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow key;
    PyValueBorrow default_value = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("setdefault", nargs, 1, 2)) {
        goto exit;
    }
    key = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    default_value = PyValue_Borrow(args[1]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _gdbm_gdbm_setdefault_impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self), key, default_value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the database.");

#define _GDBM_GDBM_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_gdbm_gdbm_close), METH_VALUE|METH_NOARGS, _gdbm_gdbm_close__doc__},

static PyValue
_gdbm_gdbm_close_impl(gdbmobject *self);

static PyValue
_gdbm_gdbm_close(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _gdbm_gdbm_close_impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_keys__doc__,
"keys($self, /)\n"
"--\n"
"\n"
"Get a list of all keys in the database.");

#define _GDBM_GDBM_KEYS_METHODDEF    \
    {"keys", _PyCFunction_CAST(_gdbm_gdbm_keys), METH_VALUE|METH_METHOD|METH_KEYWORDS, _gdbm_gdbm_keys__doc__},

static PyValue
_gdbm_gdbm_keys_impl(gdbmobject *self, PyTypeObject *cls);

static PyValue
_gdbm_gdbm_keys(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "keys() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _gdbm_gdbm_keys_impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static int
gdbm_contains_impl(gdbmobject *self, PyObject *arg);

static int
gdbm_contains(PyObject *self, PyObject *arg)
{
    int return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gdbm_contains_impl((gdbmobject *)self, arg);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_firstkey__doc__,
"firstkey($self, /)\n"
"--\n"
"\n"
"Return the starting key for the traversal.\n"
"\n"
"It\'s possible to loop over every key in the database using this\n"
"method and the nextkey() method.  The traversal is ordered by GDBM\'s\n"
"internal hash values, and won\'t be sorted by the key values.");

#define _GDBM_GDBM_FIRSTKEY_METHODDEF    \
    {"firstkey", _PyCFunction_CAST(_gdbm_gdbm_firstkey), METH_VALUE|METH_METHOD|METH_KEYWORDS, _gdbm_gdbm_firstkey__doc__},

static PyValue
_gdbm_gdbm_firstkey_impl(gdbmobject *self, PyTypeObject *cls);

static PyValue
_gdbm_gdbm_firstkey(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "firstkey() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _gdbm_gdbm_firstkey_impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_nextkey__doc__,
"nextkey($self, key, /)\n"
"--\n"
"\n"
"Returns the key that follows key in the traversal.\n"
"\n"
"The following code prints every key in the database db, without\n"
"having to create a list in memory that contains them all:\n"
"\n"
"      k = db.firstkey()\n"
"      while k is not None:\n"
"          print(k)\n"
"          k = db.nextkey(k)");

#define _GDBM_GDBM_NEXTKEY_METHODDEF    \
    {"nextkey", _PyCFunction_CAST(_gdbm_gdbm_nextkey), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _gdbm_gdbm_nextkey__doc__},

static PyObject *
_gdbm_gdbm_nextkey_impl(gdbmobject *self, PyTypeObject *cls, const char *key,
                        Py_ssize_t key_length);

static PyObject *
_gdbm_gdbm_nextkey(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    const char *key;
    Py_ssize_t key_length;

    if (!_PyArg_NoKwnames("nextkey", kwnames)) {
        goto exit;
    }
    if (!PyArg_ParseArray(args, nargs, "s#:nextkey",
        &key, &key_length)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _gdbm_gdbm_nextkey_impl((gdbmobject *)self, cls, key, key_length);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_reorganize__doc__,
"reorganize($self, /)\n"
"--\n"
"\n"
"Reorganize the database.\n"
"\n"
"If you have carried out a lot of deletions and would like to shrink\n"
"the space used by the GDBM file, this routine will reorganize the\n"
"database.  GDBM will not shorten the length of a database file\n"
"except by using this reorganization; otherwise, deleted file space\n"
"will be kept and reused as new (key,value) pairs are added.");

#define _GDBM_GDBM_REORGANIZE_METHODDEF    \
    {"reorganize", _PyCFunction_CAST(_gdbm_gdbm_reorganize), METH_VALUE|METH_METHOD|METH_KEYWORDS, _gdbm_gdbm_reorganize__doc__},

static PyValue
_gdbm_gdbm_reorganize_impl(gdbmobject *self, PyTypeObject *cls);

static PyValue
_gdbm_gdbm_reorganize(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "reorganize() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _gdbm_gdbm_reorganize_impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_sync__doc__,
"sync($self, /)\n"
"--\n"
"\n"
"Flush the database to the disk file.\n"
"\n"
"When the database has been opened in fast mode, this method forces\n"
"any unwritten data to be written to the disk.");

#define _GDBM_GDBM_SYNC_METHODDEF    \
    {"sync", _PyCFunction_CAST(_gdbm_gdbm_sync), METH_VALUE|METH_METHOD|METH_KEYWORDS, _gdbm_gdbm_sync__doc__},

static PyValue
_gdbm_gdbm_sync_impl(gdbmobject *self, PyTypeObject *cls);

static PyValue
_gdbm_gdbm_sync(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "sync() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _gdbm_gdbm_sync_impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm_clear__doc__,
"clear($self, /)\n"
"--\n"
"\n"
"Remove all items from the database.");

#define _GDBM_GDBM_CLEAR_METHODDEF    \
    {"clear", _PyCFunction_CAST(_gdbm_gdbm_clear), METH_VALUE|METH_METHOD|METH_KEYWORDS, _gdbm_gdbm_clear__doc__},

static PyValue
_gdbm_gdbm_clear_impl(gdbmobject *self, PyTypeObject *cls);

static PyValue
_gdbm_gdbm_clear(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "clear() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _gdbm_gdbm_clear_impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self), cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_gdbm_gdbm___exit____doc__,
"__exit__($self, /, *exc_info)\n"
"--\n"
"\n"
"Close the database.");

#define _GDBM_GDBM___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_gdbm_gdbm___exit__), METH_VALUE, _gdbm_gdbm___exit____doc__},

static PyValue
_gdbm_gdbm___exit___impl(gdbmobject *self, const PyValue *exc_info,
                         Py_ssize_t exc_info_length);

static PyValue
_gdbm_gdbm___exit__(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const PyValue *exc_info;
    Py_ssize_t exc_info_length;

    exc_info = args;
    exc_info_length = nargs;
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = _gdbm_gdbm___exit___impl((gdbmobject *)PyValue_AsKnownObjectBorrow(self), exc_info, exc_info_length);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(dbmopen__doc__,
"open($module, filename, flags=\'r\', mode=0o666, /)\n"
"--\n"
"\n"
"Open a dbm database and return a dbm object.\n"
"\n"
"The filename argument is the name of the database file.\n"
"\n"
"The optional flags argument can be \'r\' (to open an existing database\n"
"for reading only -- default), \'w\' (to open an existing database for\n"
"reading and writing), \'c\' (which creates the database if it doesn\'t\n"
"exist), or \'n\' (which always creates a new empty database).\n"
"\n"
"Some versions of gdbm support additional flags which must be\n"
"appended to one of the flags described above.  The module constant\n"
"\'open_flags\' is a string of valid additional flags.  The \'f\' flag\n"
"opens the database in fast mode; altered data will not automatically\n"
"be written to the disk after every change.  This results in faster\n"
"writes to the database, but may result in an inconsistent database\n"
"if the program crashes while the database is still open.  Use the\n"
"sync() method to force any unwritten data to be written to the disk.\n"
"The \'s\' flag causes all database operations to be synchronized to\n"
"disk.  The \'u\' flag disables locking of the database file.\n"
"\n"
"The optional mode argument is the Unix mode of the file, used only\n"
"when the database has to be created.  It defaults to octal 0o666.");

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
/*[clinic end generated code: output=479c5ac4cbbc8ad2 input=a9049054013a1b77]*/
