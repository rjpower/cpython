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
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_unicodeobject.h" // _PyUnicode_AsUTF8NoNUL()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(complexchar_new__doc__,
"complexchar(text, /, attr=0, pair=0)\n"
"--\n"
"\n"
"A styled wide-character cell.\n"
"\n"
"  text\n"
"    A spacing character optionally followed by combining characters.\n"
"  attr\n"
"    The attributes of the character cell.\n"
"  pair\n"
"    The color pair number of the character cell.\n"
"\n"
"text is a spacing character optionally followed by combining\n"
"characters.  attr is a set of attributes (the WA_* constants) and pair\n"
"is a color pair number.  The object is immutable; str(cc) returns its\n"
"text, and the attr and pair attributes return its rendition.");

static PyValue
complexchar_new_impl(PyTypeObject *type, PyObject *text, attr_t attr,
                     int pair);

static PyValue
complexchar_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(attr)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pair)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "attr", "pair", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "complexchar",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyObject *text;
    attr_t attr = 0;
    int pair = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
        _PyArg_BadArgumentValue("complexchar", "argument 1", "str", PyValue_Borrow(args[_idx[0]]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
        _PyArg_BadArgument("complexchar", "argument 1", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
        goto exit;
    }
    text = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]]));
    if (_idx[1] >= 0) {
        if (!attr_converter(PyValue_Borrow(args[_idx[1]]), &attr)) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        pair = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
        if (pair == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = complexchar_new_impl(type, text, attr, pair);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
complexchar_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(complexchar_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = complexchar_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(complexstr_new__doc__,
"complexstr(cells, /, attr=<unrepresentable>, pair=<unrepresentable>)\n"
"--\n"
"\n"
"An immutable string of styled wide-character cells.\n"
"\n"
"  cells\n"
"    An iterable of cells, each a complexchar or a str.\n"
"  attr\n"
"    Attributes applied to every cell (only with a string).\n"
"  pair\n"
"    Color pair applied to every cell (only with a string).\n"
"\n"
"It is the counterpart of complexchar for a run of cells, and the type\n"
"returned by window.in_wchstr().  Each cell is a complexchar or a str (a\n"
"spacing character optionally followed by combining characters).\n"
"\n"
"When cells is a string it is split into character cells, and attr and\n"
"pair (if given) style every cell.  Otherwise each item carries its own\n"
"rendition, and attr and pair must be omitted.");

static PyValue
complexstr_new_impl(PyTypeObject *type, PyValueBorrow cells,
                    PyValueBorrow attr, PyValueBorrow pair);

static PyValue
complexstr_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(attr)), _PyValue_STATIC_IMMORTAL(&_Py_ID(pair)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "attr", "pair", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "complexstr",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow cells;
    PyValueBorrow attr = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow pair = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    cells = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        attr = PyValue_Borrow(args[_idx[1]]);
    }
    if (_idx[2] >= 0) {
        pair = PyValue_Borrow(args[_idx[2]]);
    }
    return_value = complexstr_new_impl(type, cells, attr, pair);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
complexstr_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(complexstr_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = complexstr_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(_curses_window_addch__doc__,
"addch([y, x,] ch, [attr])\n"
"Paint the character.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  ch\n"
"    Character to add.\n"
"  attr\n"
"    Attributes for the character.\n"
"\n"
"Paint character ch at (y, x) with attributes attr,\n"
"overwriting any character previously painted at that location.\n"
"By default, the character position and attributes are the\n"
"current settings for the window object.");

#define _CURSES_WINDOW_ADDCH_METHODDEF    \
    {"addch", _PyCFunction_CAST(_curses_window_addch), METH_FASTCALL, _curses_window_addch__doc__},

static PyObject *
_curses_window_addch_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int group_right_1,
                          attr_t attr);

static PyObject *
_curses_window_addch(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int group_right_1 = 0;
    attr_t attr = 0;

    Py_ssize_t offset = 0;
    if (nargs < 1 || nargs > 4) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.addch requires 1 to 4 arguments");
        goto exit;
    }
    if (nargs >= 3) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    ch = args[offset];
    if (nargs == 2 || nargs == 4) {
        if (!attr_converter_object(args[nargs - 1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_addch_impl((PyCursesWindowObject *)self, group_left_1, y, x, ch, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_addstr__doc__,
"addstr([y, x,] str, [attr])\n"
"Paint the string.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  str\n"
"    String to add.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Paint the string str at (y, x) with attributes attr,\n"
"overwriting anything previously on the display.\n"
"By default, the character position and attributes are the\n"
"current settings for the window object.");

#define _CURSES_WINDOW_ADDSTR_METHODDEF    \
    {"addstr", _PyCFunction_CAST(_curses_window_addstr), METH_FASTCALL, _curses_window_addstr__doc__},

static PyObject *
_curses_window_addstr_impl(PyCursesWindowObject *self, int group_left_1,
                           int y, int x, PyObject *str, int group_right_1,
                           attr_t attr);

static PyObject *
_curses_window_addstr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *str;
    int group_right_1 = 0;
    attr_t attr = 0;

    Py_ssize_t offset = 0;
    if (nargs < 1 || nargs > 4) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.addstr requires 1 to 4 arguments");
        goto exit;
    }
    if (nargs >= 3) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    str = args[offset];
    if (nargs == 2 || nargs == 4) {
        if (!attr_converter_object(args[nargs - 1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_addstr_impl((PyCursesWindowObject *)self, group_left_1, y, x, str, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_addnstr__doc__,
"addnstr([y, x,] str, n, [attr])\n"
"Paint at most n characters of the string.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  str\n"
"    String to add.\n"
"  n\n"
"    Maximal number of characters.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Paint at most n characters of the string str at (y, x) with\n"
"attributes attr, overwriting anything previously on the display.\n"
"By default, the character position and attributes are the\n"
"current settings for the window object.");

#define _CURSES_WINDOW_ADDNSTR_METHODDEF    \
    {"addnstr", _PyCFunction_CAST(_curses_window_addnstr), METH_FASTCALL, _curses_window_addnstr__doc__},

static PyObject *
_curses_window_addnstr_impl(PyCursesWindowObject *self, int group_left_1,
                            int y, int x, PyObject *str, int n,
                            int group_right_1, attr_t attr);

static PyObject *
_curses_window_addnstr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *str;
    int n;
    int group_right_1 = 0;
    attr_t attr = 0;

    Py_ssize_t offset = 0;
    if (nargs < 2 || nargs > 5) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.addnstr requires 2 to 5 arguments");
        goto exit;
    }
    if (nargs >= 4) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    str = args[offset];
    n = _PyValue_AsInt(PyValue_FromObjectBorrow(args[offset + 1]));
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs == 3 || nargs == 5) {
        if (!attr_converter_object(args[nargs - 1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_addnstr_impl((PyCursesWindowObject *)self, group_left_1, y, x, str, n, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_bkgd__doc__,
"bkgd(ch, [attr])\n"
"Set the background property of the window.\n"
"\n"
"  ch\n"
"    Background character.\n"
"  attr\n"
"    Background attributes.");

#define _CURSES_WINDOW_BKGD_METHODDEF    \
    {"bkgd", _PyCFunction_CAST(_curses_window_bkgd), METH_FASTCALL, _curses_window_bkgd__doc__},

static PyObject *
_curses_window_bkgd_impl(PyCursesWindowObject *self, PyObject *ch,
                         int group_right_1, attr_t attr);

static PyObject *
_curses_window_bkgd(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ch;
    int group_right_1 = 0;
    attr_t attr = 0;

    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.bkgd requires 1 to 2 arguments");
        goto exit;
    }
    ch = args[0];
    if (nargs >= 2) {
        if (!attr_converter_object(args[1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_bkgd_impl((PyCursesWindowObject *)self, ch, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_attroff__doc__,
"attroff($self, attr, /)\n"
"--\n"
"\n"
"Remove attribute attr from the \"background\" set.");

#define _CURSES_WINDOW_ATTROFF_METHODDEF    \
    {"attroff", _PyCFunction_CAST(_curses_window_attroff), METH_VALUE|METH_O, _curses_window_attroff__doc__},

static PyValue
_curses_window_attroff_impl(PyCursesWindowObject *self, attr_t attr);

static PyValue
_curses_window_attroff(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_window_attroff_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_attron__doc__,
"attron($self, attr, /)\n"
"--\n"
"\n"
"Add attribute attr to the \"background\" set.");

#define _CURSES_WINDOW_ATTRON_METHODDEF    \
    {"attron", _PyCFunction_CAST(_curses_window_attron), METH_VALUE|METH_O, _curses_window_attron__doc__},

static PyValue
_curses_window_attron_impl(PyCursesWindowObject *self, attr_t attr);

static PyValue
_curses_window_attron(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_window_attron_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_attrset__doc__,
"attrset($self, attr, /)\n"
"--\n"
"\n"
"Set the \"background\" set of attributes.");

#define _CURSES_WINDOW_ATTRSET_METHODDEF    \
    {"attrset", _PyCFunction_CAST(_curses_window_attrset), METH_VALUE|METH_O, _curses_window_attrset__doc__},

static PyValue
_curses_window_attrset_impl(PyCursesWindowObject *self, attr_t attr);

static PyValue
_curses_window_attrset(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_window_attrset_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), attr);

exit:
    return return_value;
}

#if defined(HAVE_CURSES_WATTR_GET)

PyDoc_STRVAR(_curses_window_attr_get__doc__,
"attr_get($self, /)\n"
"--\n"
"\n"
"Return the window\'s attributes and color pair as (attrs, pair).");

#define _CURSES_WINDOW_ATTR_GET_METHODDEF    \
    {"attr_get", _PyCFunction_CAST(_curses_window_attr_get), METH_VALUE|METH_NOARGS, _curses_window_attr_get__doc__},

static PyValue
_curses_window_attr_get_impl(PyCursesWindowObject *self);

static PyValue
_curses_window_attr_get(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_window_attr_get_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* defined(HAVE_CURSES_WATTR_GET) */

#if defined(HAVE_CURSES_WATTR_SET)

PyDoc_STRVAR(_curses_window_attr_set__doc__,
"attr_set($self, attr, pair=0, /)\n"
"--\n"
"\n"
"Set the window\'s attributes and color pair.");

#define _CURSES_WINDOW_ATTR_SET_METHODDEF    \
    {"attr_set", _PyCFunction_CAST(_curses_window_attr_set), METH_VALUE, _curses_window_attr_set__doc__},

static PyValue
_curses_window_attr_set_impl(PyCursesWindowObject *self, attr_t attr,
                             int pair);

static PyValue
_curses_window_attr_set(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;
    int pair = 0;

    if (!_PyArg_CheckPositional("attr_set", nargs, 1, 2)) {
        goto exit;
    }
    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!pair_converter(PyValue_Borrow(args[1]), &pair)) {
        goto exit;
    }
skip_optional:
    return_value = _curses_window_attr_set_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), attr, pair);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_WATTR_SET) */

#if defined(HAVE_CURSES_WATTR_ON)

PyDoc_STRVAR(_curses_window_attr_on__doc__,
"attr_on($self, attr, /)\n"
"--\n"
"\n"
"Turn on the given attributes without affecting any others.");

#define _CURSES_WINDOW_ATTR_ON_METHODDEF    \
    {"attr_on", _PyCFunction_CAST(_curses_window_attr_on), METH_VALUE|METH_O, _curses_window_attr_on__doc__},

static PyValue
_curses_window_attr_on_impl(PyCursesWindowObject *self, attr_t attr);

static PyValue
_curses_window_attr_on(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_window_attr_on_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), attr);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_WATTR_ON) */

#if defined(HAVE_CURSES_WATTR_OFF)

PyDoc_STRVAR(_curses_window_attr_off__doc__,
"attr_off($self, attr, /)\n"
"--\n"
"\n"
"Turn off the given attributes without affecting any others.");

#define _CURSES_WINDOW_ATTR_OFF_METHODDEF    \
    {"attr_off", _PyCFunction_CAST(_curses_window_attr_off), METH_VALUE|METH_O, _curses_window_attr_off__doc__},

static PyValue
_curses_window_attr_off_impl(PyCursesWindowObject *self, attr_t attr);

static PyValue
_curses_window_attr_off(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_window_attr_off_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), attr);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_WATTR_OFF) */

#if defined(HAVE_CURSES_WCOLOR_SET)

PyDoc_STRVAR(_curses_window_color_set__doc__,
"color_set($self, pair, /)\n"
"--\n"
"\n"
"Set the window\'s color pair attribute.");

#define _CURSES_WINDOW_COLOR_SET_METHODDEF    \
    {"color_set", _PyCFunction_CAST(_curses_window_color_set), METH_VALUE|METH_O, _curses_window_color_set__doc__},

static PyValue
_curses_window_color_set_impl(PyCursesWindowObject *self, int pair);

static PyValue
_curses_window_color_set(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int pair;

    if (!pair_converter(PyValue_Borrow(args[0]), &pair)) {
        goto exit;
    }
    return_value = _curses_window_color_set_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), pair);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_WCOLOR_SET) */

PyDoc_STRVAR(_curses_window_getattrs__doc__,
"getattrs($self, /)\n"
"--\n"
"\n"
"Return the window\'s current attributes.");

#define _CURSES_WINDOW_GETATTRS_METHODDEF    \
    {"getattrs", _PyCFunction_CAST(_curses_window_getattrs), METH_VALUE|METH_NOARGS, _curses_window_getattrs__doc__},

static PyValue
_curses_window_getattrs_impl(PyCursesWindowObject *self);

static PyValue
_curses_window_getattrs(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_window_getattrs_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_window_bkgdset__doc__,
"bkgdset(ch, [attr])\n"
"Set the window\'s background.\n"
"\n"
"  ch\n"
"    Background character.\n"
"  attr\n"
"    Background attributes.");

#define _CURSES_WINDOW_BKGDSET_METHODDEF    \
    {"bkgdset", _PyCFunction_CAST(_curses_window_bkgdset), METH_FASTCALL, _curses_window_bkgdset__doc__},

static PyObject *
_curses_window_bkgdset_impl(PyCursesWindowObject *self, PyObject *ch,
                            int group_right_1, attr_t attr);

static PyObject *
_curses_window_bkgdset(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ch;
    int group_right_1 = 0;
    attr_t attr = 0;

    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.bkgdset requires 1 to 2 arguments");
        goto exit;
    }
    ch = args[0];
    if (nargs >= 2) {
        if (!attr_converter_object(args[1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_bkgdset_impl((PyCursesWindowObject *)self, ch, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_border__doc__,
"border($self, ls=0, rs=0, ts=0, bs=0, tl=0, tr=0, bl=0, br=0, /)\n"
"--\n"
"\n"
"Draw a border around the edges of the window.\n"
"\n"
"  ls\n"
"    Left side.\n"
"  rs\n"
"    Right side.\n"
"  ts\n"
"    Top side.\n"
"  bs\n"
"    Bottom side.\n"
"  tl\n"
"    Upper-left corner.\n"
"  tr\n"
"    Upper-right corner.\n"
"  bl\n"
"    Bottom-left corner.\n"
"  br\n"
"    Bottom-right corner.\n"
"\n"
"Each parameter specifies the character to use for a specific part of\n"
"the border.  The characters can be specified as integers or as\n"
"one-character strings.  A 0 value for any parameter will cause the\n"
"default character to be used for that parameter.");

#define _CURSES_WINDOW_BORDER_METHODDEF    \
    {"border", _PyCFunction_CAST(_curses_window_border), METH_VALUE, _curses_window_border__doc__},

static PyValue
_curses_window_border_impl(PyCursesWindowObject *self, PyValueBorrow ls,
                           PyValueBorrow rs, PyValueBorrow ts,
                           PyValueBorrow bs, PyValueBorrow tl,
                           PyValueBorrow tr, PyValueBorrow bl,
                           PyValueBorrow br);

static PyValue
_curses_window_border(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow ls = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow rs = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow ts = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow bs = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow tl = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow tr = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow bl = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow br = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_CheckPositional("border", nargs, 0, 8)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ls = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    rs = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    ts = PyValue_Borrow(args[2]);
    if (nargs < 4) {
        goto skip_optional;
    }
    bs = PyValue_Borrow(args[3]);
    if (nargs < 5) {
        goto skip_optional;
    }
    tl = PyValue_Borrow(args[4]);
    if (nargs < 6) {
        goto skip_optional;
    }
    tr = PyValue_Borrow(args[5]);
    if (nargs < 7) {
        goto skip_optional;
    }
    bl = PyValue_Borrow(args[6]);
    if (nargs < 8) {
        goto skip_optional;
    }
    br = PyValue_Borrow(args[7]);
skip_optional:
    return_value = _curses_window_border_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), ls, rs, ts, bs, tl, tr, bl, br);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_box__doc__,
"box([verch=0, horch=0])\n"
"Draw a border around the edges of the window.\n"
"\n"
"  verch\n"
"    Left and right side.\n"
"  horch\n"
"    Top and bottom side.\n"
"\n"
"Similar to border(), but both ls and rs are verch and both ts and bs\n"
"are horch.  The default corner characters are always used by this\n"
"function.");

#define _CURSES_WINDOW_BOX_METHODDEF    \
    {"box", _PyCFunction_CAST(_curses_window_box), METH_FASTCALL, _curses_window_box__doc__},

static PyObject *
_curses_window_box_impl(PyCursesWindowObject *self, int group_right_1,
                        PyObject *verch, PyObject *horch);

static PyObject *
_curses_window_box(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    PyObject *verch = _PyLong_GetZero();
    PyObject *horch = _PyLong_GetZero();

    switch (nargs) {
        case 0:
        case 2:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.box requires 0 to 2 arguments");
            goto exit;
    }
    if (nargs >= 2) {
        verch = args[0];
        horch = args[1];
        group_right_1 = 1;
    }
    return_value = _curses_window_box_impl((PyCursesWindowObject *)self, group_right_1, verch, horch);

exit:
    return return_value;
}

#if defined(HAVE_CURSES_WCHGAT)

PyDoc_STRVAR(_curses_window_chgat__doc__,
"chgat([y, x,] [n=-1,] attr)\n"
"Set the attributes of characters.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Number of characters.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Set the attributes of n characters at the current cursor position,\n"
"or at position (y, x) if supplied.  If no value of n is given or\n"
"n = -1, the attribute will be set on all the characters to the end\n"
"of the line.  This function does not move the cursor.  The changed\n"
"line will be touched using the touchline() method so that the\n"
"contents will be redisplayed by the next window refresh.");

#define _CURSES_WINDOW_CHGAT_METHODDEF    \
    {"chgat", _PyCFunction_CAST(_curses_window_chgat), METH_FASTCALL, _curses_window_chgat__doc__},

static PyObject *
_curses_window_chgat_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, int group_left_2, int n, attr_t attr);

static PyObject *
_curses_window_chgat(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    int group_left_2 = 0;
    int n = -1;
    attr_t attr;

    if (nargs < 1 || nargs > 4) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.chgat requires 1 to 4 arguments");
        goto exit;
    }
    if (nargs >= 3) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_left_1 = 1;
    }
    if (nargs == 2 || nargs == 4) {
        n = _PyValue_AsInt(PyValue_FromObjectBorrow(args[nargs - 2]));
        if (n == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_left_2 = 1;
    }
    if (!attr_converter_object(args[nargs - 1], &attr)) {
        goto exit;
    }
    return_value = _curses_window_chgat_impl((PyCursesWindowObject *)self, group_left_1, y, x, group_left_2, n, attr);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_WCHGAT) */

PyDoc_STRVAR(_curses_window_delch__doc__,
"delch([y, x])\n"
"Delete the character under the cursor, or at (y, x) if specified.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"All characters to the right on the same line are shifted one\n"
"position left.");

#define _CURSES_WINDOW_DELCH_METHODDEF    \
    {"delch", _PyCFunction_CAST(_curses_window_delch), METH_FASTCALL, _curses_window_delch__doc__},

static PyObject *
_curses_window_delch_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x);

static PyObject *
_curses_window_delch(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (nargs) {
        case 0:
        case 2:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.delch requires 0 to 2 arguments");
            goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_delch_impl((PyCursesWindowObject *)self, group_right_1, y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_derwin__doc__,
"derwin([nlines=0, ncols=0,] begin_y, begin_x)\n"
"Create a sub-window (window-relative coordinates).\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"  begin_y\n"
"    Top side y-coordinate.\n"
"  begin_x\n"
"    Left side x-coordinate.\n"
"\n"
"derwin() is the same as calling subwin(), except that begin_y and\n"
"begin_x are relative to the origin of the window, rather than\n"
"relative to the entire screen.");

#define _CURSES_WINDOW_DERWIN_METHODDEF    \
    {"derwin", _PyCFunction_CAST(_curses_window_derwin), METH_FASTCALL, _curses_window_derwin__doc__},

static PyObject *
_curses_window_derwin_impl(PyCursesWindowObject *self, int group_left_1,
                           int nlines, int ncols, int begin_y, int begin_x);

static PyObject *
_curses_window_derwin(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int nlines = 0;
    int ncols = 0;
    int begin_y;
    int begin_x;

    switch (nargs) {
        case 2:
        case 4:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.derwin requires 2 to 4 arguments");
            goto exit;
    }
    if (nargs >= 4) {
        nlines = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (nlines == -1 && PyErr_Occurred()) {
            goto exit;
        }
        ncols = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (ncols == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_left_1 = 1;
    }
    begin_y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[nargs - 2]));
    if (begin_y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    begin_x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[nargs - 1]));
    if (begin_x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_derwin_impl((PyCursesWindowObject *)self, group_left_1, nlines, ncols, begin_y, begin_x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_dupwin__doc__,
"dupwin($self, /)\n"
"--\n"
"\n"
"Create an exact duplicate of the window.\n"
"\n"
"The new window is independent of the original: it has the same size,\n"
"position, contents and attributes, but its own cell buffer, so later\n"
"changes to one do not affect the other.");

#define _CURSES_WINDOW_DUPWIN_METHODDEF    \
    {"dupwin", _PyCFunction_CAST(_curses_window_dupwin), METH_VALUE|METH_NOARGS, _curses_window_dupwin__doc__},

static PyValue
_curses_window_dupwin_impl(PyCursesWindowObject *self);

static PyValue
_curses_window_dupwin(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_window_dupwin_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_window_echochar__doc__,
"echochar(ch, [attr])\n"
"Add character ch with attribute attr, and refresh.\n"
"\n"
"  ch\n"
"    Character to add.\n"
"  attr\n"
"    Attributes for the character.");

#define _CURSES_WINDOW_ECHOCHAR_METHODDEF    \
    {"echochar", _PyCFunction_CAST(_curses_window_echochar), METH_FASTCALL, _curses_window_echochar__doc__},

static PyObject *
_curses_window_echochar_impl(PyCursesWindowObject *self, PyObject *ch,
                             int group_right_1, attr_t attr);

static PyObject *
_curses_window_echochar(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ch;
    int group_right_1 = 0;
    attr_t attr = 0;

    if (nargs < 1 || nargs > 2) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.echochar requires 1 to 2 arguments");
        goto exit;
    }
    ch = args[0];
    if (nargs >= 2) {
        if (!attr_converter_object(args[1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_echochar_impl((PyCursesWindowObject *)self, ch, group_right_1, attr);

exit:
    return return_value;
}

#if (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES))

PyDoc_STRVAR(_curses_window_enclose__doc__,
"enclose($self, y, x, /)\n"
"--\n"
"\n"
"Return True if the screen-relative coordinates are enclosed by the window.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.");

#define _CURSES_WINDOW_ENCLOSE_METHODDEF    \
    {"enclose", _PyCFunction_CAST(_curses_window_enclose), METH_VALUE, _curses_window_enclose__doc__},

static PyValue
_curses_window_enclose_impl(PyCursesWindowObject *self, int y, int x);

static PyValue
_curses_window_enclose(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int y;
    int x;

    if (!_PyArg_CheckPositional("enclose", nargs, 2, 2)) {
        goto exit;
    }
    y = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    x = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_enclose_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), y, x);

exit:
    return return_value;
}

#endif /* (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES)) */

#if (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES))

PyDoc_STRVAR(_curses_window_mouse_trafo__doc__,
"mouse_trafo($self, y, x, to_screen, /)\n"
"--\n"
"\n"
"Convert coordinates between window-relative and screen-relative.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  to_screen\n"
"    If True, convert window-relative coordinates to\n"
"    stdscr-relative ones; otherwise convert the other way.\n"
"\n"
"Return the converted (y, x) coordinates, or None if they are\n"
"outside the window.");

#define _CURSES_WINDOW_MOUSE_TRAFO_METHODDEF    \
    {"mouse_trafo", _PyCFunction_CAST(_curses_window_mouse_trafo), METH_VALUE, _curses_window_mouse_trafo__doc__},

static PyValue
_curses_window_mouse_trafo_impl(PyCursesWindowObject *self, int y, int x,
                                int to_screen);

static PyValue
_curses_window_mouse_trafo(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int y;
    int x;
    int to_screen;

    if (!_PyArg_CheckPositional("mouse_trafo", nargs, 3, 3)) {
        goto exit;
    }
    y = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    x = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    to_screen = _PyValue_IsTrue(PyValue_Borrow(args[2]));
    if (to_screen < 0) {
        goto exit;
    }
    return_value = _curses_window_mouse_trafo_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), y, x, to_screen);

exit:
    return return_value;
}

#endif /* (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES)) */

PyDoc_STRVAR(_curses_window_getbkgd__doc__,
"getbkgd($self, /)\n"
"--\n"
"\n"
"Return the window\'s current background character/attribute pair.");

#define _CURSES_WINDOW_GETBKGD_METHODDEF    \
    {"getbkgd", _PyCFunction_CAST(_curses_window_getbkgd), METH_VALUE|METH_NOARGS, _curses_window_getbkgd__doc__},

static PyValue
_curses_window_getbkgd_impl(PyCursesWindowObject *self);

static PyValue
_curses_window_getbkgd(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_window_getbkgd_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_window_in_wch__doc__,
"in_wch([y, x])\n"
"Return the complex character at the given position in the window.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"The returned object is a complexchar carrying the cell\'s text,\n"
"attributes and color pair.");

#define _CURSES_WINDOW_IN_WCH_METHODDEF    \
    {"in_wch", _PyCFunction_CAST(_curses_window_in_wch), METH_FASTCALL, _curses_window_in_wch__doc__},

static PyObject *
_curses_window_in_wch_impl(PyCursesWindowObject *self, int group_right_1,
                           int y, int x);

static PyObject *
_curses_window_in_wch(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (nargs) {
        case 0:
        case 2:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.in_wch requires 0 to 2 arguments");
            goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_in_wch_impl((PyCursesWindowObject *)self, group_right_1, y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_getbkgrnd__doc__,
"getbkgrnd($self, /)\n"
"--\n"
"\n"
"Return the window\'s current background complex character.");

#define _CURSES_WINDOW_GETBKGRND_METHODDEF    \
    {"getbkgrnd", _PyCFunction_CAST(_curses_window_getbkgrnd), METH_VALUE|METH_NOARGS, _curses_window_getbkgrnd__doc__},

static PyValue
_curses_window_getbkgrnd_impl(PyCursesWindowObject *self);

static PyValue
_curses_window_getbkgrnd(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_window_getbkgrnd_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_window_getch__doc__,
"getch([y, x])\n"
"Read a key press and return it as an integer.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"Wait until a key is pressed, or return -1 if the read is\n"
"non-blocking or times out.\n"
"\n"
"An ordinary key is returned as the code of a single byte of its\n"
"encoding in the current locale, so a character encoded with several\n"
"bytes takes several calls.  Use get_wch() to read it as a single\n"
"character.\n"
"\n"
"In keypad mode function keys and other special keys are returned as\n"
"one of the KEY_* constants, which cannot be mistaken for an ordinary\n"
"key.  Otherwise their bytes are returned one at a time.");

#define _CURSES_WINDOW_GETCH_METHODDEF    \
    {"getch", _PyCFunction_CAST(_curses_window_getch), METH_FASTCALL, _curses_window_getch__doc__},

static PyObject *
_curses_window_getch_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x);

static PyObject *
_curses_window_getch(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (nargs) {
        case 0:
        case 2:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.getch requires 0 to 2 arguments");
            goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_getch_impl((PyCursesWindowObject *)self, group_right_1, y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_getkey__doc__,
"getkey([y, x])\n"
"Read a key press and return it as a str.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"Read as getch() does, but return an ordinary key as a one-character\n"
"string, the byte decoded as Latin-1, and a special key as its name,\n"
"such as \'KEY_UP\'.  Raise curses.error instead of returning -1 if\n"
"there is no input.");

#define _CURSES_WINDOW_GETKEY_METHODDEF    \
    {"getkey", _PyCFunction_CAST(_curses_window_getkey), METH_FASTCALL, _curses_window_getkey__doc__},

static PyObject *
_curses_window_getkey_impl(PyCursesWindowObject *self, int group_right_1,
                           int y, int x);

static PyObject *
_curses_window_getkey(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (nargs) {
        case 0:
        case 2:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.getkey requires 0 to 2 arguments");
            goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_getkey_impl((PyCursesWindowObject *)self, group_right_1, y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_get_wch__doc__,
"get_wch([y, x])\n"
"Read a key press and return it as a one-character str.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"Wait until a key is pressed, or raise curses.error if the read is\n"
"non-blocking or times out.\n"
"\n"
"In keypad mode function keys and other special keys are returned as\n"
"one of the KEY_* constants, an integer.  Otherwise their characters\n"
"are returned one at a time.");

#define _CURSES_WINDOW_GET_WCH_METHODDEF    \
    {"get_wch", _PyCFunction_CAST(_curses_window_get_wch), METH_FASTCALL, _curses_window_get_wch__doc__},

static PyObject *
_curses_window_get_wch_impl(PyCursesWindowObject *self, int group_right_1,
                            int y, int x);

static PyObject *
_curses_window_get_wch(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (nargs) {
        case 0:
        case 2:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.get_wch requires 0 to 2 arguments");
            goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_get_wch_impl((PyCursesWindowObject *)self, group_right_1, y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_getstr__doc__,
"getstr([y, x,] n=2047)\n"
"Read a line of input and return it as a bytes object.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of bytes.\n"
"\n"
"The input is read with primitive line editing capacity, encoded in\n"
"the current locale, and does not include the terminating newline.\n"
"At most n bytes are read.");

#define _CURSES_WINDOW_GETSTR_METHODDEF    \
    {"getstr", _PyCFunction_CAST(_curses_window_getstr), METH_FASTCALL, _curses_window_getstr__doc__},

static PyObject *
_curses_window_getstr_impl(PyCursesWindowObject *self, int group_left_1,
                           int y, int x, unsigned int n);

static PyObject *
_curses_window_getstr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    unsigned int n = 2047;

    Py_ssize_t offset = 0;
    if (nargs > 3) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.getstr requires 0 to 3 arguments");
        goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    if (nargs <= offset) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_FromObjectBorrow(args[offset]), &n, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned int");
            goto exit;
        }
    }
skip_optional:
    return_value = _curses_window_getstr_impl((PyCursesWindowObject *)self, group_left_1, y, x, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_hline__doc__,
"hline([y, x,] ch, n, [attr])\n"
"Display a horizontal line.\n"
"\n"
"  y\n"
"    Starting Y-coordinate.\n"
"  x\n"
"    Starting X-coordinate.\n"
"  ch\n"
"    Character to draw.\n"
"  n\n"
"    Line length.\n"
"  attr\n"
"    Attributes for the characters.");

#define _CURSES_WINDOW_HLINE_METHODDEF    \
    {"hline", _PyCFunction_CAST(_curses_window_hline), METH_FASTCALL, _curses_window_hline__doc__},

static PyObject *
_curses_window_hline_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int n,
                          int group_right_1, attr_t attr);

static PyObject *
_curses_window_hline(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int n;
    int group_right_1 = 0;
    attr_t attr = 0;

    Py_ssize_t offset = 0;
    if (nargs < 2 || nargs > 5) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.hline requires 2 to 5 arguments");
        goto exit;
    }
    if (nargs >= 4) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    ch = args[offset];
    n = _PyValue_AsInt(PyValue_FromObjectBorrow(args[offset + 1]));
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs == 3 || nargs == 5) {
        if (!attr_converter_object(args[nargs - 1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_hline_impl((PyCursesWindowObject *)self, group_left_1, y, x, ch, n, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_insch__doc__,
"insch([y, x,] ch, [attr])\n"
"Insert a character before the current or specified position.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  ch\n"
"    Character to insert.\n"
"  attr\n"
"    Attributes for the character.\n"
"\n"
"All characters to the right of the cursor are shifted one position\n"
"right, with the rightmost characters on the line being lost.");

#define _CURSES_WINDOW_INSCH_METHODDEF    \
    {"insch", _PyCFunction_CAST(_curses_window_insch), METH_FASTCALL, _curses_window_insch__doc__},

static PyObject *
_curses_window_insch_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int group_right_1,
                          attr_t attr);

static PyObject *
_curses_window_insch(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int group_right_1 = 0;
    attr_t attr = 0;

    Py_ssize_t offset = 0;
    if (nargs < 1 || nargs > 4) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.insch requires 1 to 4 arguments");
        goto exit;
    }
    if (nargs >= 3) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    ch = args[offset];
    if (nargs == 2 || nargs == 4) {
        if (!attr_converter_object(args[nargs - 1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_insch_impl((PyCursesWindowObject *)self, group_left_1, y, x, ch, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_inch__doc__,
"inch([y, x])\n"
"Return the character at the given position in the window.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"The bottom 8 bits are the character proper, and upper bits are the\n"
"attributes.");

#define _CURSES_WINDOW_INCH_METHODDEF    \
    {"inch", _PyCFunction_CAST(_curses_window_inch), METH_FASTCALL, _curses_window_inch__doc__},

static PyObject *
_curses_window_inch_impl(PyCursesWindowObject *self, int group_right_1,
                         int y, int x);

static PyObject *
_curses_window_inch(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (nargs) {
        case 0:
        case 2:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.inch requires 0 to 2 arguments");
            goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_inch_impl((PyCursesWindowObject *)self, group_right_1, y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_instr__doc__,
"instr([y, x,] [n])\n"
"Return the text of the window as a bytes object.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of bytes.  The rest of the line by default.\n"
"\n"
"Read from the current cursor position, or from y, x if specified, to\n"
"the end of the line or at most n bytes if n is specified, and return\n"
"the text in the encoding of the current locale, with attributes and\n"
"color pairs stripped.");

#define _CURSES_WINDOW_INSTR_METHODDEF    \
    {"instr", _PyCFunction_CAST(_curses_window_instr), METH_FASTCALL, _curses_window_instr__doc__},

static PyObject *
_curses_window_instr_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x, int group_right_2, unsigned int n);

static PyObject *
_curses_window_instr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;
    int group_right_2 = 0;
    unsigned int n = 0;

    if (nargs > 3) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.instr requires 0 to 3 arguments");
        goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    if (nargs == 1 || nargs == 3) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_FromObjectBorrow(args[nargs - 1]), &n, sizeof(unsigned int),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned int)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C unsigned int");
                goto exit;
            }
        }
        group_right_2 = 1;
    }
    return_value = _curses_window_instr_impl((PyCursesWindowObject *)self, group_right_1, y, x, group_right_2, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_get_wstr__doc__,
"get_wstr([y, x,] n=2047)\n"
"Read a line of input and return it as a str.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of characters.\n"
"\n"
"This is the wide-character variant of getstr().  The input is read\n"
"with primitive line editing capacity and does not include the\n"
"terminating newline.  At most n characters are read.");

#define _CURSES_WINDOW_GET_WSTR_METHODDEF    \
    {"get_wstr", _PyCFunction_CAST(_curses_window_get_wstr), METH_FASTCALL, _curses_window_get_wstr__doc__},

static PyObject *
_curses_window_get_wstr_impl(PyCursesWindowObject *self, int group_left_1,
                             int y, int x, unsigned int n);

static PyObject *
_curses_window_get_wstr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    unsigned int n = 2047;

    Py_ssize_t offset = 0;
    if (nargs > 3) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.get_wstr requires 0 to 3 arguments");
        goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    if (nargs <= offset) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_FromObjectBorrow(args[offset]), &n, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned int");
            goto exit;
        }
    }
skip_optional:
    return_value = _curses_window_get_wstr_impl((PyCursesWindowObject *)self, group_left_1, y, x, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_in_wstr__doc__,
"in_wstr([y, x,] [n])\n"
"Return the text of the window as a str.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of characters.  The rest of the line by default.\n"
"\n"
"This is the wide-character variant of instr().  Read from the\n"
"current cursor position, or from y, x if specified, to the end of\n"
"the line or at most n characters if n is specified, with attributes\n"
"and color pairs stripped.");

#define _CURSES_WINDOW_IN_WSTR_METHODDEF    \
    {"in_wstr", _PyCFunction_CAST(_curses_window_in_wstr), METH_FASTCALL, _curses_window_in_wstr__doc__},

static PyObject *
_curses_window_in_wstr_impl(PyCursesWindowObject *self, int group_right_1,
                            int y, int x, int group_right_2, unsigned int n);

static PyObject *
_curses_window_in_wstr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;
    int group_right_2 = 0;
    unsigned int n = 0;

    if (nargs > 3) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.in_wstr requires 0 to 3 arguments");
        goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    if (nargs == 1 || nargs == 3) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_FromObjectBorrow(args[nargs - 1]), &n, sizeof(unsigned int),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned int)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C unsigned int");
                goto exit;
            }
        }
        group_right_2 = 1;
    }
    return_value = _curses_window_in_wstr_impl((PyCursesWindowObject *)self, group_right_1, y, x, group_right_2, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_in_wchstr__doc__,
"in_wchstr([y, x,] [n])\n"
"Return the styled cells of the window as a complexstr.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  n\n"
"    Maximal number of cells.  The rest of the line by default.\n"
"\n"
"Read from the current cursor position, or from y, x if specified,\n"
"to the end of the line or at most n cells if n is specified.\n"
"Unlike instr() and in_wstr(), each cell keeps its attributes and\n"
"color pair, so the result can be written back unchanged with\n"
"addstr().");

#define _CURSES_WINDOW_IN_WCHSTR_METHODDEF    \
    {"in_wchstr", _PyCFunction_CAST(_curses_window_in_wchstr), METH_FASTCALL, _curses_window_in_wchstr__doc__},

static PyObject *
_curses_window_in_wchstr_impl(PyCursesWindowObject *self, int group_right_1,
                              int y, int x, int group_right_2,
                              unsigned int n);

static PyObject *
_curses_window_in_wchstr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;
    int group_right_2 = 0;
    unsigned int n = 0;

    if (nargs > 3) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.in_wchstr requires 0 to 3 arguments");
        goto exit;
    }
    if (nargs >= 2) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    if (nargs == 1 || nargs == 3) {
        {
            Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_FromObjectBorrow(args[nargs - 1]), &n, sizeof(unsigned int),
                    Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                    Py_ASNATIVEBYTES_ALLOW_INDEX |
                    Py_ASNATIVEBYTES_REJECT_NEGATIVE |
                    Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
            if (_bytes < 0) {
                goto exit;
            }
            if ((size_t)_bytes > sizeof(unsigned int)) {
                PyErr_SetString(PyExc_OverflowError,
                            "Python int too large for C unsigned int");
                goto exit;
            }
        }
        group_right_2 = 1;
    }
    return_value = _curses_window_in_wchstr_impl((PyCursesWindowObject *)self, group_right_1, y, x, group_right_2, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_insstr__doc__,
"insstr([y, x,] str, [attr])\n"
"Insert the string before the current or specified position.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  str\n"
"    String to insert.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Insert a character string (as many characters as will fit on the\n"
"line) before the character under the cursor.  All characters to the\n"
"right of the cursor are shifted right, with the rightmost characters\n"
"on the line being lost.  The cursor position does not change (after\n"
"moving to y, x, if specified).");

#define _CURSES_WINDOW_INSSTR_METHODDEF    \
    {"insstr", _PyCFunction_CAST(_curses_window_insstr), METH_FASTCALL, _curses_window_insstr__doc__},

static PyObject *
_curses_window_insstr_impl(PyCursesWindowObject *self, int group_left_1,
                           int y, int x, PyObject *str, int group_right_1,
                           attr_t attr);

static PyObject *
_curses_window_insstr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *str;
    int group_right_1 = 0;
    attr_t attr = 0;

    Py_ssize_t offset = 0;
    if (nargs < 1 || nargs > 4) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.insstr requires 1 to 4 arguments");
        goto exit;
    }
    if (nargs >= 3) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    str = args[offset];
    if (nargs == 2 || nargs == 4) {
        if (!attr_converter_object(args[nargs - 1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_insstr_impl((PyCursesWindowObject *)self, group_left_1, y, x, str, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_insnstr__doc__,
"insnstr([y, x,] str, n, [attr])\n"
"Insert at most n characters of the string.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  str\n"
"    String to insert.\n"
"  n\n"
"    Maximal number of characters.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Insert a character string (as many characters as will fit on the\n"
"line) before the character under the cursor, up to n characters.  If\n"
"n is zero or negative, the entire string is inserted.  All\n"
"characters to the right of the cursor are shifted right, with the\n"
"rightmost characters on the line being lost.  The cursor position\n"
"does not change (after moving to y, x, if specified).");

#define _CURSES_WINDOW_INSNSTR_METHODDEF    \
    {"insnstr", _PyCFunction_CAST(_curses_window_insnstr), METH_FASTCALL, _curses_window_insnstr__doc__},

static PyObject *
_curses_window_insnstr_impl(PyCursesWindowObject *self, int group_left_1,
                            int y, int x, PyObject *str, int n,
                            int group_right_1, attr_t attr);

static PyObject *
_curses_window_insnstr(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *str;
    int n;
    int group_right_1 = 0;
    attr_t attr = 0;

    Py_ssize_t offset = 0;
    if (nargs < 2 || nargs > 5) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.insnstr requires 2 to 5 arguments");
        goto exit;
    }
    if (nargs >= 4) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    str = args[offset];
    n = _PyValue_AsInt(PyValue_FromObjectBorrow(args[offset + 1]));
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs == 3 || nargs == 5) {
        if (!attr_converter_object(args[nargs - 1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_insnstr_impl((PyCursesWindowObject *)self, group_left_1, y, x, str, n, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_is_linetouched__doc__,
"is_linetouched($self, line, /)\n"
"--\n"
"\n"
"Return True if the specified line was modified, otherwise return False.\n"
"\n"
"  line\n"
"    Line number.\n"
"\n"
"Raise a curses.error exception if line is not valid for the given\n"
"window.");

#define _CURSES_WINDOW_IS_LINETOUCHED_METHODDEF    \
    {"is_linetouched", _PyCFunction_CAST(_curses_window_is_linetouched), METH_VALUE|METH_O, _curses_window_is_linetouched__doc__},

static PyValue
_curses_window_is_linetouched_impl(PyCursesWindowObject *self, int line);

static PyValue
_curses_window_is_linetouched(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int line;

    line = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (line == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_is_linetouched_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), line);

exit:
    return return_value;
}

#if defined(py_is_pad)

PyDoc_STRVAR(_curses_window_noutrefresh__doc__,
"noutrefresh([pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol])\n"
"Mark for refresh but wait.\n"
"\n"
"This function updates the data structure representing the desired\n"
"state of the window, but does not force an update of the physical\n"
"screen.  To accomplish that, call doupdate().");

#define _CURSES_WINDOW_NOUTREFRESH_METHODDEF    \
    {"noutrefresh", _PyCFunction_CAST(_curses_window_noutrefresh), METH_FASTCALL, _curses_window_noutrefresh__doc__},

static PyObject *
_curses_window_noutrefresh_impl(PyCursesWindowObject *self,
                                int group_right_1, int pminrow, int pmincol,
                                int sminrow, int smincol, int smaxrow,
                                int smaxcol);

static PyObject *
_curses_window_noutrefresh(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int pminrow = 0;
    int pmincol = 0;
    int sminrow = 0;
    int smincol = 0;
    int smaxrow = 0;
    int smaxcol = 0;

    switch (nargs) {
        case 0:
        case 6:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.noutrefresh requires 0 to 6 arguments");
            goto exit;
    }
    if (nargs >= 6) {
        pminrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (pminrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        pmincol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (pmincol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        sminrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[2]));
        if (sminrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        smincol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[3]));
        if (smincol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        smaxrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[4]));
        if (smaxrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        smaxcol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[5]));
        if (smaxcol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_noutrefresh_impl((PyCursesWindowObject *)self, group_right_1, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);

exit:
    return return_value;
}

#endif /* defined(py_is_pad) */

#if !defined(py_is_pad)

PyDoc_STRVAR(_curses_window_noutrefresh__doc__,
"noutrefresh($self, /)\n"
"--\n"
"\n"
"Mark for refresh but wait.\n"
"\n"
"This function updates the data structure representing the desired\n"
"state of the window, but does not force an update of the physical\n"
"screen.  To accomplish that, call doupdate().");

#define _CURSES_WINDOW_NOUTREFRESH_METHODDEF    \
    {"noutrefresh", _PyCFunction_CAST(_curses_window_noutrefresh), METH_VALUE|METH_NOARGS, _curses_window_noutrefresh__doc__},

static PyValue
_curses_window_noutrefresh_impl(PyCursesWindowObject *self);

static PyValue
_curses_window_noutrefresh(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_window_noutrefresh_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

#endif /* !defined(py_is_pad) */

PyDoc_STRVAR(_curses_window_overlay__doc__,
"overlay(destwin, [sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol])\n"
"Overlay the window on top of destwin.\n"
"\n"
"The windows need not be the same size, only the overlapping region\n"
"is copied.  This copy is non-destructive, which means that the\n"
"current background character does not overwrite the old contents of\n"
"destwin.\n"
"\n"
"To get fine-grained control over the copied region, the second form\n"
"of overlay() can be used.  sminrow and smincol are the upper-left\n"
"coordinates of the source window, and the other variables mark\n"
"a rectangle in the destination window.");

#define _CURSES_WINDOW_OVERLAY_METHODDEF    \
    {"overlay", _PyCFunction_CAST(_curses_window_overlay), METH_FASTCALL, _curses_window_overlay__doc__},

static PyObject *
_curses_window_overlay_impl(PyCursesWindowObject *self,
                            PyCursesWindowObject *destwin, int group_right_1,
                            int sminrow, int smincol, int dminrow,
                            int dmincol, int dmaxrow, int dmaxcol);

static PyObject *
_curses_window_overlay(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyCursesWindowObject *destwin;
    int group_right_1 = 0;
    int sminrow = 0;
    int smincol = 0;
    int dminrow = 0;
    int dmincol = 0;
    int dmaxrow = 0;
    int dmaxcol = 0;

    switch (nargs) {
        case 1:
        case 7:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.overlay requires 1 to 7 arguments");
            goto exit;
    }
    if (!PyObject_TypeCheck(args[0], clinic_state()->window_type)) {
        _PyArg_BadArgument("overlay", "argument 1", (clinic_state()->window_type)->tp_name, args[0]);
        goto exit;
    }
    destwin = (PyCursesWindowObject *)args[0];
    if (nargs >= 7) {
        sminrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (sminrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        smincol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[2]));
        if (smincol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dminrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[3]));
        if (dminrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dmincol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[4]));
        if (dmincol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dmaxrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[5]));
        if (dmaxrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dmaxcol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[6]));
        if (dmaxcol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_overlay_impl((PyCursesWindowObject *)self, destwin, group_right_1, sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_overwrite__doc__,
"overwrite(destwin, [sminrow, smincol, dminrow, dmincol, dmaxrow,\n"
"          dmaxcol])\n"
"Overwrite the window on top of destwin.\n"
"\n"
"The windows need not be the same size, in which case only the\n"
"overlapping region is copied.  This copy is destructive, which means\n"
"that the current background character overwrites the old contents of\n"
"destwin.\n"
"\n"
"To get fine-grained control over the copied region, the second form\n"
"of overwrite() can be used. sminrow and smincol are the upper-left\n"
"coordinates of the source window, the other variables mark\n"
"a rectangle in the destination window.");

#define _CURSES_WINDOW_OVERWRITE_METHODDEF    \
    {"overwrite", _PyCFunction_CAST(_curses_window_overwrite), METH_FASTCALL, _curses_window_overwrite__doc__},

static PyObject *
_curses_window_overwrite_impl(PyCursesWindowObject *self,
                              PyCursesWindowObject *destwin,
                              int group_right_1, int sminrow, int smincol,
                              int dminrow, int dmincol, int dmaxrow,
                              int dmaxcol);

static PyObject *
_curses_window_overwrite(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyCursesWindowObject *destwin;
    int group_right_1 = 0;
    int sminrow = 0;
    int smincol = 0;
    int dminrow = 0;
    int dmincol = 0;
    int dmaxrow = 0;
    int dmaxcol = 0;

    switch (nargs) {
        case 1:
        case 7:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.overwrite requires 1 to 7 arguments");
            goto exit;
    }
    if (!PyObject_TypeCheck(args[0], clinic_state()->window_type)) {
        _PyArg_BadArgument("overwrite", "argument 1", (clinic_state()->window_type)->tp_name, args[0]);
        goto exit;
    }
    destwin = (PyCursesWindowObject *)args[0];
    if (nargs >= 7) {
        sminrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (sminrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        smincol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[2]));
        if (smincol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dminrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[3]));
        if (dminrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dmincol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[4]));
        if (dmincol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dmaxrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[5]));
        if (dmaxrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dmaxcol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[6]));
        if (dmaxcol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_overwrite_impl((PyCursesWindowObject *)self, destwin, group_right_1, sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_putwin__doc__,
"putwin($self, file, /)\n"
"--\n"
"\n"
"Write all data associated with the window into the provided file object.\n"
"\n"
"This information can be later retrieved using the getwin() function.");

#define _CURSES_WINDOW_PUTWIN_METHODDEF    \
    {"putwin", _PyCFunction_CAST(_curses_window_putwin), METH_VALUE|METH_O, _curses_window_putwin__doc__},

static PyValue
_curses_window_putwin_impl(PyCursesWindowObject *self, PyValueBorrow file);

static PyValue
_curses_window_putwin(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow file;

    file = PyValue_Borrow(args[0]);
    return_value = _curses_window_putwin_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), file);

    return return_value;
}

PyDoc_STRVAR(_curses_window_redrawln__doc__,
"redrawln($self, beg, num, /)\n"
"--\n"
"\n"
"Mark the specified lines corrupted.\n"
"\n"
"  beg\n"
"    Starting line number.\n"
"  num\n"
"    The number of lines.\n"
"\n"
"They should be completely redrawn on the next refresh() call.");

#define _CURSES_WINDOW_REDRAWLN_METHODDEF    \
    {"redrawln", _PyCFunction_CAST(_curses_window_redrawln), METH_VALUE, _curses_window_redrawln__doc__},

static PyValue
_curses_window_redrawln_impl(PyCursesWindowObject *self, int beg, int num);

static PyValue
_curses_window_redrawln(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int beg;
    int num;

    if (!_PyArg_CheckPositional("redrawln", nargs, 2, 2)) {
        goto exit;
    }
    beg = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (beg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    num = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (num == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_redrawln_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), beg, num);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_refresh__doc__,
"refresh([pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol])\n"
"Update the display immediately.\n"
"\n"
"Synchronize actual screen with previous drawing/deleting methods.\n"
"The 6 optional arguments can only be specified when the window is\n"
"a pad created with newpad().  The additional parameters are needed\n"
"to indicate what part of the pad and screen are involved.  pminrow\n"
"and pmincol specify the upper left-hand corner of the rectangle to\n"
"be displayed in the pad.  sminrow, smincol, smaxrow, and smaxcol\n"
"specify the edges of the rectangle to be displayed on the screen.\n"
"The lower right-hand corner of the rectangle to be displayed in the\n"
"pad is calculated from the screen coordinates, since the rectangles\n"
"must be the same size.  Both rectangles must be entirely contained\n"
"within their respective structures.  Negative values of pminrow,\n"
"pmincol, sminrow, or smincol are treated as if they were zero.");

#define _CURSES_WINDOW_REFRESH_METHODDEF    \
    {"refresh", _PyCFunction_CAST(_curses_window_refresh), METH_FASTCALL, _curses_window_refresh__doc__},

static PyObject *
_curses_window_refresh_impl(PyCursesWindowObject *self, int group_right_1,
                            int pminrow, int pmincol, int sminrow,
                            int smincol, int smaxrow, int smaxcol);

static PyObject *
_curses_window_refresh(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int pminrow = 0;
    int pmincol = 0;
    int sminrow = 0;
    int smincol = 0;
    int smaxrow = 0;
    int smaxcol = 0;

    switch (nargs) {
        case 0:
        case 6:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.refresh requires 0 to 6 arguments");
            goto exit;
    }
    if (nargs >= 6) {
        pminrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (pminrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        pmincol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (pmincol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        sminrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[2]));
        if (sminrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        smincol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[3]));
        if (smincol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        smaxrow = _PyValue_AsInt(PyValue_FromObjectBorrow(args[4]));
        if (smaxrow == -1 && PyErr_Occurred()) {
            goto exit;
        }
        smaxcol = _PyValue_AsInt(PyValue_FromObjectBorrow(args[5]));
        if (smaxcol == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_refresh_impl((PyCursesWindowObject *)self, group_right_1, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_setscrreg__doc__,
"setscrreg($self, top, bottom, /)\n"
"--\n"
"\n"
"Define a software scrolling region.\n"
"\n"
"  top\n"
"    First line number.\n"
"  bottom\n"
"    Last line number.\n"
"\n"
"All scrolling actions will take place in this region.");

#define _CURSES_WINDOW_SETSCRREG_METHODDEF    \
    {"setscrreg", _PyCFunction_CAST(_curses_window_setscrreg), METH_VALUE, _curses_window_setscrreg__doc__},

static PyValue
_curses_window_setscrreg_impl(PyCursesWindowObject *self, int top,
                              int bottom);

static PyValue
_curses_window_setscrreg(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int top;
    int bottom;

    if (!_PyArg_CheckPositional("setscrreg", nargs, 2, 2)) {
        goto exit;
    }
    top = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (top == -1 && PyErr_Occurred()) {
        goto exit;
    }
    bottom = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (bottom == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_setscrreg_impl((PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(self), top, bottom);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_subwin__doc__,
"subwin([nlines=0, ncols=0,] begin_y, begin_x)\n"
"Create a sub-window (screen-relative coordinates).\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"  begin_y\n"
"    Top side y-coordinate.\n"
"  begin_x\n"
"    Left side x-coordinate.\n"
"\n"
"By default, the sub-window will extend from the specified position\n"
"to the lower right corner of the window.");

#define _CURSES_WINDOW_SUBWIN_METHODDEF    \
    {"subwin", _PyCFunction_CAST(_curses_window_subwin), METH_FASTCALL, _curses_window_subwin__doc__},

static PyObject *
_curses_window_subwin_impl(PyCursesWindowObject *self, int group_left_1,
                           int nlines, int ncols, int begin_y, int begin_x);

static PyObject *
_curses_window_subwin(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int nlines = 0;
    int ncols = 0;
    int begin_y;
    int begin_x;

    switch (nargs) {
        case 2:
        case 4:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.subwin requires 2 to 4 arguments");
            goto exit;
    }
    if (nargs >= 4) {
        nlines = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (nlines == -1 && PyErr_Occurred()) {
            goto exit;
        }
        ncols = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (ncols == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_left_1 = 1;
    }
    begin_y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[nargs - 2]));
    if (begin_y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    begin_x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[nargs - 1]));
    if (begin_x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_subwin_impl((PyCursesWindowObject *)self, group_left_1, nlines, ncols, begin_y, begin_x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_scroll__doc__,
"scroll([lines=1])\n"
"Scroll the screen or scrolling region.\n"
"\n"
"  lines\n"
"    Number of lines to scroll.\n"
"\n"
"Scroll upward if the argument is positive and downward if it is\n"
"negative.");

#define _CURSES_WINDOW_SCROLL_METHODDEF    \
    {"scroll", _PyCFunction_CAST(_curses_window_scroll), METH_FASTCALL, _curses_window_scroll__doc__},

static PyObject *
_curses_window_scroll_impl(PyCursesWindowObject *self, int group_right_1,
                           int lines);

static PyObject *
_curses_window_scroll(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int lines = 1;

    if (nargs > 1) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.scroll requires 0 to 1 arguments");
        goto exit;
    }
    if (nargs >= 1) {
        lines = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (lines == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_scroll_impl((PyCursesWindowObject *)self, group_right_1, lines);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_touchline__doc__,
"touchline(start, count, [changed=True])\n"
"Pretend count lines have been changed, starting with line start.\n"
"\n"
"If changed is supplied, it specifies whether the affected lines are\n"
"marked as having been changed (changed=True) or unchanged\n"
"(changed=False).");

#define _CURSES_WINDOW_TOUCHLINE_METHODDEF    \
    {"touchline", _PyCFunction_CAST(_curses_window_touchline), METH_FASTCALL, _curses_window_touchline__doc__},

static PyObject *
_curses_window_touchline_impl(PyCursesWindowObject *self, int start,
                              int count, int group_right_1, int changed);

static PyObject *
_curses_window_touchline(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int start;
    int count;
    int group_right_1 = 0;
    int changed = 1;

    if (nargs < 2 || nargs > 3) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.touchline requires 2 to 3 arguments");
        goto exit;
    }
    start = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
    if (start == -1 && PyErr_Occurred()) {
        goto exit;
    }
    count = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
    if (count == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs >= 3) {
        changed = _PyValue_IsTrue(PyValue_FromObjectBorrow(args[2]));
        if (changed < 0) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_touchline_impl((PyCursesWindowObject *)self, start, count, group_right_1, changed);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_vline__doc__,
"vline([y, x,] ch, n, [attr])\n"
"Display a vertical line.\n"
"\n"
"  y\n"
"    Starting Y-coordinate.\n"
"  x\n"
"    Starting X-coordinate.\n"
"  ch\n"
"    Character to draw.\n"
"  n\n"
"    Line length.\n"
"  attr\n"
"    Attributes for the character.");

#define _CURSES_WINDOW_VLINE_METHODDEF    \
    {"vline", _PyCFunction_CAST(_curses_window_vline), METH_FASTCALL, _curses_window_vline__doc__},

static PyObject *
_curses_window_vline_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int n,
                          int group_right_1, attr_t attr);

static PyObject *
_curses_window_vline(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int n;
    int group_right_1 = 0;
    attr_t attr = 0;

    Py_ssize_t offset = 0;
    if (nargs < 2 || nargs > 5) {
        PyErr_SetString(PyExc_TypeError, "_curses.window.vline requires 2 to 5 arguments");
        goto exit;
    }
    if (nargs >= 4) {
        y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
        if (y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
        if (x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset += 2;
        group_left_1 = 1;
    }
    ch = args[offset];
    n = _PyValue_AsInt(PyValue_FromObjectBorrow(args[offset + 1]));
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs == 3 || nargs == 5) {
        if (!attr_converter_object(args[nargs - 1], &attr)) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_window_vline_impl((PyCursesWindowObject *)self, group_left_1, y, x, ch, n, group_right_1, attr);

exit:
    return return_value;
}

#if defined(HAVE_CURSES_FILTER)

PyDoc_STRVAR(_curses_filter__doc__,
"filter($module, /)\n"
"--\n"
"\n"
"Restrict screen updates to the current line.\n"
"\n"
"Must be called before initscr().  Afterwards curses confines the cursor\n"
"and screen updates to a single line, which is useful for enabling\n"
"character-at-a-time line editing without touching the rest of the\n"
"screen.");

#define _CURSES_FILTER_METHODDEF    \
    {"filter", _PyCFunction_CAST(_curses_filter), METH_VALUE|METH_NOARGS, _curses_filter__doc__},

static PyValue
_curses_filter_impl(PyObject *module);

static PyValue
_curses_filter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_filter_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_CURSES_FILTER) */

#if defined(HAVE_CURSES_NOFILTER)

PyDoc_STRVAR(_curses_nofilter__doc__,
"nofilter($module, /)\n"
"--\n"
"\n"
"Undo the effect of a preceding filter() call.\n"
"\n"
"Must be called before initscr().  It restores the normal behaviour that\n"
"filter() disables, so that the next initscr() or newterm() uses the full\n"
"screen rather than a single line.");

#define _CURSES_NOFILTER_METHODDEF    \
    {"nofilter", _PyCFunction_CAST(_curses_nofilter), METH_VALUE|METH_NOARGS, _curses_nofilter__doc__},

static PyValue
_curses_nofilter_impl(PyObject *module);

static PyValue
_curses_nofilter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_nofilter_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_CURSES_NOFILTER) */

PyDoc_STRVAR(_curses_baudrate__doc__,
"baudrate($module, /)\n"
"--\n"
"\n"
"Return the output speed of the terminal in bits per second.");

#define _CURSES_BAUDRATE_METHODDEF    \
    {"baudrate", _PyCFunction_CAST(_curses_baudrate), METH_VALUE|METH_NOARGS, _curses_baudrate__doc__},

static PyValue
_curses_baudrate_impl(PyObject *module);

static PyValue
_curses_baudrate(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_baudrate_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_beep__doc__,
"beep($module, /)\n"
"--\n"
"\n"
"Emit a short attention sound.");

#define _CURSES_BEEP_METHODDEF    \
    {"beep", _PyCFunction_CAST(_curses_beep), METH_VALUE|METH_NOARGS, _curses_beep__doc__},

static PyValue
_curses_beep_impl(PyObject *module);

static PyValue
_curses_beep(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_beep_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_can_change_color__doc__,
"can_change_color($module, /)\n"
"--\n"
"\n"
"Return True if the programmer can change the colors displayed by the terminal.");

#define _CURSES_CAN_CHANGE_COLOR_METHODDEF    \
    {"can_change_color", _PyCFunction_CAST(_curses_can_change_color), METH_VALUE|METH_NOARGS, _curses_can_change_color__doc__},

static PyValue
_curses_can_change_color_impl(PyObject *module);

static PyValue
_curses_can_change_color(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_can_change_color_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_cbreak__doc__,
"cbreak($module, flag=True, /)\n"
"--\n"
"\n"
"Enter cbreak mode.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling nocbreak().\n"
"\n"
"In cbreak mode (sometimes called \"rare\" mode) normal tty line buffering\n"
"is turned off and characters are available to be read one by one.\n"
"However, unlike raw mode, special characters (interrupt, quit, suspend,\n"
"and flow control) retain their effects on the tty driver and calling\n"
"program.  Calling first raw() then cbreak() leaves the terminal in\n"
"cbreak mode.");

#define _CURSES_CBREAK_METHODDEF    \
    {"cbreak", _PyCFunction_CAST(_curses_cbreak), METH_VALUE, _curses_cbreak__doc__},

static PyValue
_curses_cbreak_impl(PyObject *module, int flag);

static PyValue
_curses_cbreak(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("cbreak", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
skip_optional:
    return_value = _curses_cbreak_impl(PyValue_AsKnownObjectBorrow(module), flag);

exit:
    return return_value;
}

#if ((defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427) || defined(PDCURSES))

PyDoc_STRVAR(_curses_is_cbreak__doc__,
"is_cbreak($module, /)\n"
"--\n"
"\n"
"Return True if cbreak mode is enabled, False otherwise.");

#define _CURSES_IS_CBREAK_METHODDEF    \
    {"is_cbreak", _PyCFunction_CAST(_curses_is_cbreak), METH_VALUE|METH_NOARGS, _curses_is_cbreak__doc__},

static PyValue
_curses_is_cbreak_impl(PyObject *module);

static PyValue
_curses_is_cbreak(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_is_cbreak_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* ((defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427) || defined(PDCURSES)) */

#if ((defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427) || defined(PDCURSES))

PyDoc_STRVAR(_curses_is_echo__doc__,
"is_echo($module, /)\n"
"--\n"
"\n"
"Return True if echo mode is enabled, False otherwise.");

#define _CURSES_IS_ECHO_METHODDEF    \
    {"is_echo", _PyCFunction_CAST(_curses_is_echo), METH_VALUE|METH_NOARGS, _curses_is_echo__doc__},

static PyValue
_curses_is_echo_impl(PyObject *module);

static PyValue
_curses_is_echo(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_is_echo_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* ((defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427) || defined(PDCURSES)) */

#if ((defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427) || defined(PDCURSES))

PyDoc_STRVAR(_curses_is_nl__doc__,
"is_nl($module, /)\n"
"--\n"
"\n"
"Return True if nl mode is enabled, False otherwise.");

#define _CURSES_IS_NL_METHODDEF    \
    {"is_nl", _PyCFunction_CAST(_curses_is_nl), METH_VALUE|METH_NOARGS, _curses_is_nl__doc__},

static PyValue
_curses_is_nl_impl(PyObject *module);

static PyValue
_curses_is_nl(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_is_nl_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* ((defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427) || defined(PDCURSES)) */

#if ((defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427) || defined(PDCURSES))

PyDoc_STRVAR(_curses_is_raw__doc__,
"is_raw($module, /)\n"
"--\n"
"\n"
"Return True if raw mode is enabled, False otherwise.");

#define _CURSES_IS_RAW_METHODDEF    \
    {"is_raw", _PyCFunction_CAST(_curses_is_raw), METH_VALUE|METH_NOARGS, _curses_is_raw__doc__},

static PyValue
_curses_is_raw_impl(PyObject *module);

static PyValue
_curses_is_raw(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_is_raw_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* ((defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20240427) || defined(PDCURSES)) */

PyDoc_STRVAR(_curses_color_content__doc__,
"color_content($module, color_number, /)\n"
"--\n"
"\n"
"Return the red, green, and blue (RGB) components of the specified color.\n"
"\n"
"  color_number\n"
"    The number of the color (0 - (COLORS-1)).\n"
"\n"
"A 3-tuple is returned, containing the R, G, B values for the given\n"
"color, which will be between 0 (no component) and 1000 (maximum amount\n"
"of component).");

#define _CURSES_COLOR_CONTENT_METHODDEF    \
    {"color_content", _PyCFunction_CAST(_curses_color_content), METH_VALUE|METH_O, _curses_color_content__doc__},

static PyValue
_curses_color_content_impl(PyObject *module, int color_number);

static PyValue
_curses_color_content(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int color_number;

    if (!color_converter(PyValue_Borrow(args[0]), &color_number)) {
        goto exit;
    }
    return_value = _curses_color_content_impl(PyValue_AsKnownObjectBorrow(module), color_number);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_color_pair__doc__,
"color_pair($module, pair_number, /)\n"
"--\n"
"\n"
"Return the attribute value for displaying text in the specified color.\n"
"\n"
"  pair_number\n"
"    The number of the color pair.\n"
"\n"
"This attribute value can be combined with A_STANDOUT, A_REVERSE, and the\n"
"other A_* attributes.  pair_number() is the counterpart to this\n"
"function.");

#define _CURSES_COLOR_PAIR_METHODDEF    \
    {"color_pair", _PyCFunction_CAST(_curses_color_pair), METH_VALUE|METH_O, _curses_color_pair__doc__},

static PyValue
_curses_color_pair_impl(PyObject *module, int pair_number);

static PyValue
_curses_color_pair(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int pair_number;

    pair_number = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (pair_number == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_color_pair_impl(PyValue_AsKnownObjectBorrow(module), pair_number);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_curs_set__doc__,
"curs_set($module, visibility, /)\n"
"--\n"
"\n"
"Set the cursor state.\n"
"\n"
"  visibility\n"
"    0 for invisible, 1 for normal visible, or 2 for very visible.\n"
"\n"
"If the terminal supports the visibility requested, the previous cursor\n"
"state is returned; otherwise, an exception is raised.  On many\n"
"terminals, the \"visible\" mode is an underline cursor and the \"very\n"
"visible\" mode is a block cursor.");

#define _CURSES_CURS_SET_METHODDEF    \
    {"curs_set", _PyCFunction_CAST(_curses_curs_set), METH_VALUE|METH_O, _curses_curs_set__doc__},

static PyValue
_curses_curs_set_impl(PyObject *module, int visibility);

static PyValue
_curses_curs_set(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int visibility;

    visibility = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (visibility == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_curs_set_impl(PyValue_AsKnownObjectBorrow(module), visibility);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_def_prog_mode__doc__,
"def_prog_mode($module, /)\n"
"--\n"
"\n"
"Save the current terminal mode as the \"program\" mode.\n"
"\n"
"The \"program\" mode is the mode when the running program is using curses.\n"
"\n"
"Subsequent calls to reset_prog_mode() will restore this mode.");

#define _CURSES_DEF_PROG_MODE_METHODDEF    \
    {"def_prog_mode", _PyCFunction_CAST(_curses_def_prog_mode), METH_VALUE|METH_NOARGS, _curses_def_prog_mode__doc__},

static PyValue
_curses_def_prog_mode_impl(PyObject *module);

static PyValue
_curses_def_prog_mode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_def_prog_mode_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_def_shell_mode__doc__,
"def_shell_mode($module, /)\n"
"--\n"
"\n"
"Save the current terminal mode as the \"shell\" mode.\n"
"\n"
"The \"shell\" mode is the mode when the running program is not using\n"
"curses.\n"
"\n"
"Subsequent calls to reset_shell_mode() will restore this mode.");

#define _CURSES_DEF_SHELL_MODE_METHODDEF    \
    {"def_shell_mode", _PyCFunction_CAST(_curses_def_shell_mode), METH_VALUE|METH_NOARGS, _curses_def_shell_mode__doc__},

static PyValue
_curses_def_shell_mode_impl(PyObject *module);

static PyValue
_curses_def_shell_mode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_def_shell_mode_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_delay_output__doc__,
"delay_output($module, ms, /)\n"
"--\n"
"\n"
"Insert a pause in output.\n"
"\n"
"  ms\n"
"    Duration in milliseconds.");

#define _CURSES_DELAY_OUTPUT_METHODDEF    \
    {"delay_output", _PyCFunction_CAST(_curses_delay_output), METH_VALUE|METH_O, _curses_delay_output__doc__},

static PyValue
_curses_delay_output_impl(PyObject *module, int ms);

static PyValue
_curses_delay_output(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int ms;

    ms = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (ms == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_delay_output_impl(PyValue_AsKnownObjectBorrow(module), ms);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_doupdate__doc__,
"doupdate($module, /)\n"
"--\n"
"\n"
"Update the physical screen to match the virtual screen.");

#define _CURSES_DOUPDATE_METHODDEF    \
    {"doupdate", _PyCFunction_CAST(_curses_doupdate), METH_VALUE|METH_NOARGS, _curses_doupdate__doc__},

static PyValue
_curses_doupdate_impl(PyObject *module);

static PyValue
_curses_doupdate(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_doupdate_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_echo__doc__,
"echo($module, flag=True, /)\n"
"--\n"
"\n"
"Enter echo mode.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling noecho().\n"
"\n"
"In echo mode, each character input is echoed to the screen as it is\n"
"entered.");

#define _CURSES_ECHO_METHODDEF    \
    {"echo", _PyCFunction_CAST(_curses_echo), METH_VALUE, _curses_echo__doc__},

static PyValue
_curses_echo_impl(PyObject *module, int flag);

static PyValue
_curses_echo(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("echo", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
skip_optional:
    return_value = _curses_echo_impl(PyValue_AsKnownObjectBorrow(module), flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_endwin__doc__,
"endwin($module, /)\n"
"--\n"
"\n"
"De-initialize the library, and return terminal to normal status.");

#define _CURSES_ENDWIN_METHODDEF    \
    {"endwin", _PyCFunction_CAST(_curses_endwin), METH_VALUE|METH_NOARGS, _curses_endwin__doc__},

static PyValue
_curses_endwin_impl(PyObject *module);

static PyValue
_curses_endwin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_endwin_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_erasechar__doc__,
"erasechar($module, /)\n"
"--\n"
"\n"
"Return the user\'s current erase character.");

#define _CURSES_ERASECHAR_METHODDEF    \
    {"erasechar", _PyCFunction_CAST(_curses_erasechar), METH_VALUE|METH_NOARGS, _curses_erasechar__doc__},

static PyValue
_curses_erasechar_impl(PyObject *module);

static PyValue
_curses_erasechar(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_erasechar_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_erasewchar__doc__,
"erasewchar($module, /)\n"
"--\n"
"\n"
"Return the user\'s current wide-character erase character.");

#define _CURSES_ERASEWCHAR_METHODDEF    \
    {"erasewchar", _PyCFunction_CAST(_curses_erasewchar), METH_VALUE|METH_NOARGS, _curses_erasewchar__doc__},

static PyValue
_curses_erasewchar_impl(PyObject *module);

static PyValue
_curses_erasewchar(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_erasewchar_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_flash__doc__,
"flash($module, /)\n"
"--\n"
"\n"
"Flash the screen.\n"
"\n"
"That is, change it to reverse-video and then change it back in a short\n"
"interval.");

#define _CURSES_FLASH_METHODDEF    \
    {"flash", _PyCFunction_CAST(_curses_flash), METH_VALUE|METH_NOARGS, _curses_flash__doc__},

static PyValue
_curses_flash_impl(PyObject *module);

static PyValue
_curses_flash(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_flash_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_flushinp__doc__,
"flushinp($module, /)\n"
"--\n"
"\n"
"Flush all input buffers.\n"
"\n"
"This throws away any typeahead that has been typed by the user and has\n"
"not yet been processed by the program.");

#define _CURSES_FLUSHINP_METHODDEF    \
    {"flushinp", _PyCFunction_CAST(_curses_flushinp), METH_VALUE|METH_NOARGS, _curses_flushinp__doc__},

static PyValue
_curses_flushinp_impl(PyObject *module);

static PyValue
_curses_flushinp(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_flushinp_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(getsyx)

PyDoc_STRVAR(_curses_getsyx__doc__,
"getsyx($module, /)\n"
"--\n"
"\n"
"Return the current coordinates of the virtual screen cursor.\n"
"\n"
"Return a (y, x) tuple.  If leaveok is currently true, return (-1, -1).");

#define _CURSES_GETSYX_METHODDEF    \
    {"getsyx", _PyCFunction_CAST(_curses_getsyx), METH_VALUE|METH_NOARGS, _curses_getsyx__doc__},

static PyValue
_curses_getsyx_impl(PyObject *module);

static PyValue
_curses_getsyx(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_getsyx_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(getsyx) */

#if (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES))

PyDoc_STRVAR(_curses_getmouse__doc__,
"getmouse($module, /)\n"
"--\n"
"\n"
"Retrieve the queued mouse event.\n"
"\n"
"After getch() returns KEY_MOUSE to signal a mouse event, this function\n"
"returns a 5-tuple (id, x, y, z, bstate).");

#define _CURSES_GETMOUSE_METHODDEF    \
    {"getmouse", _PyCFunction_CAST(_curses_getmouse), METH_VALUE|METH_NOARGS, _curses_getmouse__doc__},

static PyValue
_curses_getmouse_impl(PyObject *module);

static PyValue
_curses_getmouse(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_getmouse_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES)) */

#if (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES))

PyDoc_STRVAR(_curses_ungetmouse__doc__,
"ungetmouse($module, id, x, y, z, bstate, /)\n"
"--\n"
"\n"
"Push a KEY_MOUSE event onto the input queue.\n"
"\n"
"The following getmouse() will return the given state data.");

#define _CURSES_UNGETMOUSE_METHODDEF    \
    {"ungetmouse", _PyCFunction_CAST(_curses_ungetmouse), METH_VALUE, _curses_ungetmouse__doc__},

static PyValue
_curses_ungetmouse_impl(PyObject *module, short id, int x, int y, int z,
                        unsigned long bstate);

static PyValue
_curses_ungetmouse(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    short id;
    int x;
    int y;
    int z;
    unsigned long bstate;

    if (!_PyArg_CheckPositional("ungetmouse", nargs, 5, 5)) {
        goto exit;
    }
    {
        long ival = _PyValue_AsLong(PyValue_Borrow(args[0]));
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < SHRT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is less than minimum");
            goto exit;
        }
        else if (ival > SHRT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is greater than maximum");
            goto exit;
        }
        else {
            id = (short) ival;
        }
    }
    x = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    y = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    z = _PyValue_AsInt(PyValue_Borrow(args[3]));
    if (z == -1 && PyErr_Occurred()) {
        goto exit;
    }
        if (PyValue_TYPE(PyValue_Borrow(args[4]))->tp_as_number == NULL ||
            PyValue_TYPE(PyValue_Borrow(args[4]))->tp_as_number->nb_index == NULL)
        {
            _PyArg_BadArgumentValue("ungetmouse", "argument 5", "int", PyValue_Borrow(args[4]));
            goto exit;
        }

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[4]), &bstate, sizeof(unsigned long),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    return_value = _curses_ungetmouse_impl(PyValue_AsKnownObjectBorrow(module), id, x, y, z, bstate);

exit:
    return return_value;
}

#endif /* (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES)) */

PyDoc_STRVAR(_curses_getwin__doc__,
"getwin($module, file, /)\n"
"--\n"
"\n"
"Read window related data stored in the file by an earlier putwin() call.\n"
"\n"
"The routine then creates and initializes a new window using that data,\n"
"returning the new window object.");

#define _CURSES_GETWIN_METHODDEF    \
    {"getwin", _PyCFunction_CAST(_curses_getwin), METH_VALUE|METH_O, _curses_getwin__doc__},

static PyValue
_curses_getwin_impl(PyObject *module, PyValueBorrow file);

static PyValue
_curses_getwin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow file;

    file = PyValue_Borrow(args[0]);
    return_value = _curses_getwin_impl(PyValue_AsKnownObjectBorrow(module), file);

    return return_value;
}

#if defined(HAVE_CURSES_SCR_DUMP)

PyDoc_STRVAR(_curses_scr_dump__doc__,
"scr_dump($module, filename, /)\n"
"--\n"
"\n"
"Write the current contents of the virtual screen to a file.\n"
"\n"
"  filename\n"
"    The file to write to.\n"
"\n"
"The file can later be used to restore the screen with scr_restore(),\n"
"scr_init() or scr_set().");

#define _CURSES_SCR_DUMP_METHODDEF    \
    {"scr_dump", _PyCFunction_CAST(_curses_scr_dump), METH_VALUE|METH_O, _curses_scr_dump__doc__},

static PyValue
_curses_scr_dump_impl(PyObject *module, PyValueBorrow filename);

static PyValue
_curses_scr_dump(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filename;

    filename = PyValue_Borrow(args[0]);
    return_value = _curses_scr_dump_impl(PyValue_AsKnownObjectBorrow(module), filename);

    return return_value;
}

#endif /* defined(HAVE_CURSES_SCR_DUMP) */

#if defined(HAVE_CURSES_SCR_DUMP)

PyDoc_STRVAR(_curses_scr_restore__doc__,
"scr_restore($module, filename, /)\n"
"--\n"
"\n"
"Set the virtual screen to the contents of a file made by scr_dump().\n"
"\n"
"  filename\n"
"    The file to read from.\n"
"\n"
"The next call to doupdate() or refresh() restores the screen to those\n"
"contents.");

#define _CURSES_SCR_RESTORE_METHODDEF    \
    {"scr_restore", _PyCFunction_CAST(_curses_scr_restore), METH_VALUE|METH_O, _curses_scr_restore__doc__},

static PyValue
_curses_scr_restore_impl(PyObject *module, PyValueBorrow filename);

static PyValue
_curses_scr_restore(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filename;

    filename = PyValue_Borrow(args[0]);
    return_value = _curses_scr_restore_impl(PyValue_AsKnownObjectBorrow(module), filename);

    return return_value;
}

#endif /* defined(HAVE_CURSES_SCR_DUMP) */

#if defined(HAVE_CURSES_SCR_DUMP)

PyDoc_STRVAR(_curses_scr_init__doc__,
"scr_init($module, filename, /)\n"
"--\n"
"\n"
"Initialize the assumed terminal contents from a scr_dump() file.\n"
"\n"
"  filename\n"
"    The file to read from.\n"
"\n"
"Use it as what the terminal currently displays, for example after\n"
"another program has drawn the screen.");

#define _CURSES_SCR_INIT_METHODDEF    \
    {"scr_init", _PyCFunction_CAST(_curses_scr_init), METH_VALUE|METH_O, _curses_scr_init__doc__},

static PyValue
_curses_scr_init_impl(PyObject *module, PyValueBorrow filename);

static PyValue
_curses_scr_init(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filename;

    filename = PyValue_Borrow(args[0]);
    return_value = _curses_scr_init_impl(PyValue_AsKnownObjectBorrow(module), filename);

    return return_value;
}

#endif /* defined(HAVE_CURSES_SCR_DUMP) */

#if defined(HAVE_CURSES_SCR_DUMP) && defined(HAVE_CURSES_SCR_SET)

PyDoc_STRVAR(_curses_scr_set__doc__,
"scr_set($module, filename, /)\n"
"--\n"
"\n"
"Use a scr_dump() file as both the virtual screen and the terminal.\n"
"\n"
"  filename\n"
"    The file to read from.\n"
"\n"
"This combines the effects of scr_restore() and scr_init().");

#define _CURSES_SCR_SET_METHODDEF    \
    {"scr_set", _PyCFunction_CAST(_curses_scr_set), METH_VALUE|METH_O, _curses_scr_set__doc__},

static PyValue
_curses_scr_set_impl(PyObject *module, PyValueBorrow filename);

static PyValue
_curses_scr_set(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filename;

    filename = PyValue_Borrow(args[0]);
    return_value = _curses_scr_set_impl(PyValue_AsKnownObjectBorrow(module), filename);

    return return_value;
}

#endif /* defined(HAVE_CURSES_SCR_DUMP) && defined(HAVE_CURSES_SCR_SET) */

PyDoc_STRVAR(_curses_halfdelay__doc__,
"halfdelay($module, tenths, /)\n"
"--\n"
"\n"
"Enter half-delay mode.\n"
"\n"
"  tenths\n"
"    Maximal blocking delay in tenths of seconds (1 - 255).\n"
"\n"
"Use nocbreak() to leave half-delay mode.");

#define _CURSES_HALFDELAY_METHODDEF    \
    {"halfdelay", _PyCFunction_CAST(_curses_halfdelay), METH_VALUE|METH_O, _curses_halfdelay__doc__},

static PyValue
_curses_halfdelay_impl(PyObject *module, unsigned char tenths);

static PyValue
_curses_halfdelay(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned char tenths;

    {
        long ival = _PyValue_AsLong(PyValue_Borrow(args[0]));
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < 0) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is less than minimum");
            goto exit;
        }
        else if (ival > UCHAR_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is greater than maximum");
            goto exit;
        }
        else {
            tenths = (unsigned char) ival;
        }
    }
    return_value = _curses_halfdelay_impl(PyValue_AsKnownObjectBorrow(module), tenths);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_has_colors__doc__,
"has_colors($module, /)\n"
"--\n"
"\n"
"Return True if the terminal can display colors; otherwise, return False.");

#define _CURSES_HAS_COLORS_METHODDEF    \
    {"has_colors", _PyCFunction_CAST(_curses_has_colors), METH_VALUE|METH_NOARGS, _curses_has_colors__doc__},

static PyValue
_curses_has_colors_impl(PyObject *module);

static PyValue
_curses_has_colors(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_has_colors_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_has_ic__doc__,
"has_ic($module, /)\n"
"--\n"
"\n"
"Return True if the terminal has insert- and delete-character capabilities.");

#define _CURSES_HAS_IC_METHODDEF    \
    {"has_ic", _PyCFunction_CAST(_curses_has_ic), METH_VALUE|METH_NOARGS, _curses_has_ic__doc__},

static PyValue
_curses_has_ic_impl(PyObject *module);

static PyValue
_curses_has_ic(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_has_ic_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_has_il__doc__,
"has_il($module, /)\n"
"--\n"
"\n"
"Return True if the terminal has insert- and delete-line capabilities.");

#define _CURSES_HAS_IL_METHODDEF    \
    {"has_il", _PyCFunction_CAST(_curses_has_il), METH_VALUE|METH_NOARGS, _curses_has_il__doc__},

static PyValue
_curses_has_il_impl(PyObject *module);

static PyValue
_curses_has_il(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_has_il_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(HAVE_CURSES_HAS_KEY)

PyDoc_STRVAR(_curses_has_key__doc__,
"has_key($module, key, /)\n"
"--\n"
"\n"
"Return True if the current terminal type recognizes a key with that value.\n"
"\n"
"  key\n"
"    Key number.");

#define _CURSES_HAS_KEY_METHODDEF    \
    {"has_key", _PyCFunction_CAST(_curses_has_key), METH_VALUE|METH_O, _curses_has_key__doc__},

static PyValue
_curses_has_key_impl(PyObject *module, int key);

static PyValue
_curses_has_key(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int key;

    key = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (key == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_has_key_impl(PyValue_AsKnownObjectBorrow(module), key);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_HAS_KEY) */

#if defined(HAVE_CURSES_DEFINE_KEY)

PyDoc_STRVAR(_curses_define_key__doc__,
"define_key($module, definition, keycode, /)\n"
"--\n"
"\n"
"Define an escape sequence for a key code.\n"
"\n"
"  definition\n"
"    Escape sequence to bind, or None to remove a binding.\n"
"  keycode\n"
"    Key code to generate.\n"
"\n"
"If definition is None, any existing binding for keycode is removed.\n"
"If keycode is zero or negative, the binding for definition is removed.");

#define _CURSES_DEFINE_KEY_METHODDEF    \
    {"define_key", _PyCFunction_CAST(_curses_define_key), METH_VALUE, _curses_define_key__doc__},

static PyValue
_curses_define_key_impl(PyObject *module, const char *definition,
                        int keycode);

static PyValue
_curses_define_key(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *definition;
    int keycode;

    if (!_PyArg_CheckPositional("define_key", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("define_key", "argument 1", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        definition = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        definition = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (definition == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("define_key", "argument 1", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    keycode = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (keycode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_define_key_impl(PyValue_AsKnownObjectBorrow(module), definition, keycode);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_DEFINE_KEY) */

#if defined(HAVE_CURSES_KEY_DEFINED)

PyDoc_STRVAR(_curses_key_defined__doc__,
"key_defined($module, definition, /)\n"
"--\n"
"\n"
"Return the key code bound to an escape sequence.\n"
"\n"
"  definition\n"
"    Escape sequence.\n"
"\n"
"Return 0 if no key code is bound to the escape sequence, or -1 if the\n"
"escape sequence is a prefix of another bound sequence (so ambiguous).");

#define _CURSES_KEY_DEFINED_METHODDEF    \
    {"key_defined", _PyCFunction_CAST(_curses_key_defined), METH_VALUE|METH_O, _curses_key_defined__doc__},

static PyValue
_curses_key_defined_impl(PyObject *module, const char *definition);

static PyValue
_curses_key_defined(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *definition;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("key_defined", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("key_defined", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    definition = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (definition == NULL) {
        goto exit;
    }
    return_value = _curses_key_defined_impl(PyValue_AsKnownObjectBorrow(module), definition);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_KEY_DEFINED) */

#if defined(HAVE_CURSES_KEYOK)

PyDoc_STRVAR(_curses_keyok__doc__,
"keyok($module, keycode, enable, /)\n"
"--\n"
"\n"
"Enable or disable interpretation of an individual key code.\n"
"\n"
"  keycode\n"
"    Key code.\n"
"  enable\n"
"    Whether the key code is interpreted.");

#define _CURSES_KEYOK_METHODDEF    \
    {"keyok", _PyCFunction_CAST(_curses_keyok), METH_VALUE, _curses_keyok__doc__},

static PyValue
_curses_keyok_impl(PyObject *module, int keycode, int enable);

static PyValue
_curses_keyok(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int keycode;
    int enable;

    if (!_PyArg_CheckPositional("keyok", nargs, 2, 2)) {
        goto exit;
    }
    keycode = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (keycode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    enable = _PyValue_IsTrue(PyValue_Borrow(args[1]));
    if (enable < 0) {
        goto exit;
    }
    return_value = _curses_keyok_impl(PyValue_AsKnownObjectBorrow(module), keycode, enable);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_KEYOK) */

PyDoc_STRVAR(_curses_init_color__doc__,
"init_color($module, color_number, r, g, b, /)\n"
"--\n"
"\n"
"Change the definition of a color.\n"
"\n"
"  color_number\n"
"    The number of the color to be changed (0 - (COLORS-1)).\n"
"  r\n"
"    Red component (0 - 1000).\n"
"  g\n"
"    Green component (0 - 1000).\n"
"  b\n"
"    Blue component (0 - 1000).\n"
"\n"
"When init_color() is used, all occurrences of that color on the screen\n"
"immediately change to the new definition.  This function is a no-op on\n"
"most terminals; it is active only if can_change_color() returns true.");

#define _CURSES_INIT_COLOR_METHODDEF    \
    {"init_color", _PyCFunction_CAST(_curses_init_color), METH_VALUE, _curses_init_color__doc__},

static PyValue
_curses_init_color_impl(PyObject *module, int color_number, short r, short g,
                        short b);

static PyValue
_curses_init_color(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int color_number;
    short r;
    short g;
    short b;

    if (!_PyArg_CheckPositional("init_color", nargs, 4, 4)) {
        goto exit;
    }
    if (!color_converter(PyValue_Borrow(args[0]), &color_number)) {
        goto exit;
    }
    if (!component_converter(PyValue_Borrow(args[1]), &r)) {
        goto exit;
    }
    if (!component_converter(PyValue_Borrow(args[2]), &g)) {
        goto exit;
    }
    if (!component_converter(PyValue_Borrow(args[3]), &b)) {
        goto exit;
    }
    return_value = _curses_init_color_impl(PyValue_AsKnownObjectBorrow(module), color_number, r, g, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_init_pair__doc__,
"init_pair($module, pair_number, fg, bg, /)\n"
"--\n"
"\n"
"Change the definition of a color-pair.\n"
"\n"
"  pair_number\n"
"    The number of the color-pair to be changed (1 - (COLOR_PAIRS-1)).\n"
"  fg\n"
"    Foreground color number (-1 - (COLORS-1)).\n"
"  bg\n"
"    Background color number (-1 - (COLORS-1)).\n"
"\n"
"If the color-pair was previously initialized, the screen is refreshed\n"
"and all occurrences of that color-pair are changed to the new\n"
"definition.");

#define _CURSES_INIT_PAIR_METHODDEF    \
    {"init_pair", _PyCFunction_CAST(_curses_init_pair), METH_VALUE, _curses_init_pair__doc__},

static PyValue
_curses_init_pair_impl(PyObject *module, int pair_number, int fg, int bg);

static PyValue
_curses_init_pair(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int pair_number;
    int fg;
    int bg;

    if (!_PyArg_CheckPositional("init_pair", nargs, 3, 3)) {
        goto exit;
    }
    if (!pair_converter(PyValue_Borrow(args[0]), &pair_number)) {
        goto exit;
    }
    if (!color_allow_default_converter(PyValue_Borrow(args[1]), &fg)) {
        goto exit;
    }
    if (!color_allow_default_converter(PyValue_Borrow(args[2]), &bg)) {
        goto exit;
    }
    return_value = _curses_init_pair_impl(PyValue_AsKnownObjectBorrow(module), pair_number, fg, bg);

exit:
    return return_value;
}

#if (_NCURSES_EXTENDED_COLOR_FUNCS)

PyDoc_STRVAR(_curses_alloc_pair__doc__,
"alloc_pair($module, fg, bg, /)\n"
"--\n"
"\n"
"Allocate a color pair for the given foreground and background colors.\n"
"\n"
"  fg\n"
"    Foreground color number.\n"
"  bg\n"
"    Background color number.\n"
"\n"
"If a color pair for the same colors already exists, return its number.\n"
"Otherwise allocate a new color pair and return its number.");

#define _CURSES_ALLOC_PAIR_METHODDEF    \
    {"alloc_pair", _PyCFunction_CAST(_curses_alloc_pair), METH_VALUE, _curses_alloc_pair__doc__},

static PyValue
_curses_alloc_pair_impl(PyObject *module, int fg, int bg);

static PyValue
_curses_alloc_pair(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fg;
    int bg;

    if (!_PyArg_CheckPositional("alloc_pair", nargs, 2, 2)) {
        goto exit;
    }
    if (!color_allow_default_converter(PyValue_Borrow(args[0]), &fg)) {
        goto exit;
    }
    if (!color_allow_default_converter(PyValue_Borrow(args[1]), &bg)) {
        goto exit;
    }
    return_value = _curses_alloc_pair_impl(PyValue_AsKnownObjectBorrow(module), fg, bg);

exit:
    return return_value;
}

#endif /* (_NCURSES_EXTENDED_COLOR_FUNCS) */

#if (_NCURSES_EXTENDED_COLOR_FUNCS)

PyDoc_STRVAR(_curses_find_pair__doc__,
"find_pair($module, fg, bg, /)\n"
"--\n"
"\n"
"Return the number of a color pair for the given colors, or -1.\n"
"\n"
"  fg\n"
"    Foreground color number.\n"
"  bg\n"
"    Background color number.\n"
"\n"
"Return -1 if no color pair for this combination of foreground and\n"
"background colors has been allocated.");

#define _CURSES_FIND_PAIR_METHODDEF    \
    {"find_pair", _PyCFunction_CAST(_curses_find_pair), METH_VALUE, _curses_find_pair__doc__},

static PyValue
_curses_find_pair_impl(PyObject *module, int fg, int bg);

static PyValue
_curses_find_pair(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fg;
    int bg;

    if (!_PyArg_CheckPositional("find_pair", nargs, 2, 2)) {
        goto exit;
    }
    if (!color_allow_default_converter(PyValue_Borrow(args[0]), &fg)) {
        goto exit;
    }
    if (!color_allow_default_converter(PyValue_Borrow(args[1]), &bg)) {
        goto exit;
    }
    return_value = _curses_find_pair_impl(PyValue_AsKnownObjectBorrow(module), fg, bg);

exit:
    return return_value;
}

#endif /* (_NCURSES_EXTENDED_COLOR_FUNCS) */

#if (_NCURSES_EXTENDED_COLOR_FUNCS)

PyDoc_STRVAR(_curses_free_pair__doc__,
"free_pair($module, pair, /)\n"
"--\n"
"\n"
"Free a color pair allocated by alloc_pair().\n"
"\n"
"  pair\n"
"    The number of the color pair to free.");

#define _CURSES_FREE_PAIR_METHODDEF    \
    {"free_pair", _PyCFunction_CAST(_curses_free_pair), METH_VALUE|METH_O, _curses_free_pair__doc__},

static PyValue
_curses_free_pair_impl(PyObject *module, int pair);

static PyValue
_curses_free_pair(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int pair;

    if (!pair_converter(PyValue_Borrow(args[0]), &pair)) {
        goto exit;
    }
    return_value = _curses_free_pair_impl(PyValue_AsKnownObjectBorrow(module), pair);

exit:
    return return_value;
}

#endif /* (_NCURSES_EXTENDED_COLOR_FUNCS) */

#if (_NCURSES_EXTENDED_COLOR_FUNCS)

PyDoc_STRVAR(_curses_reset_color_pairs__doc__,
"reset_color_pairs($module, /)\n"
"--\n"
"\n"
"Discard all color-pair definitions.");

#define _CURSES_RESET_COLOR_PAIRS_METHODDEF    \
    {"reset_color_pairs", _PyCFunction_CAST(_curses_reset_color_pairs), METH_VALUE|METH_NOARGS, _curses_reset_color_pairs__doc__},

static PyValue
_curses_reset_color_pairs_impl(PyObject *module);

static PyValue
_curses_reset_color_pairs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_reset_color_pairs_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* (_NCURSES_EXTENDED_COLOR_FUNCS) */

PyDoc_STRVAR(_curses_initscr__doc__,
"initscr($module, /)\n"
"--\n"
"\n"
"Initialize the library.\n"
"\n"
"Return a window object which represents the whole screen.");

#define _CURSES_INITSCR_METHODDEF    \
    {"initscr", _PyCFunction_CAST(_curses_initscr), METH_VALUE|METH_NOARGS, _curses_initscr__doc__},

static PyValue
_curses_initscr_impl(PyObject *module);

static PyValue
_curses_initscr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_initscr_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_setupterm__doc__,
"setupterm($module, /, term=None, fd=-1)\n"
"--\n"
"\n"
"Initialize the terminal.\n"
"\n"
"  term\n"
"    Terminal name.\n"
"    If omitted, the value of the TERM environment variable will be used.\n"
"  fd\n"
"    File descriptor to which any initialization sequences will be sent.\n"
"    If not supplied, the file descriptor for sys.stdout will be used.");

#define _CURSES_SETUPTERM_METHODDEF    \
    {"setupterm", _PyCFunction_CAST(_curses_setupterm), METH_VALUE|METH_KEYWORDS, _curses_setupterm__doc__},

static PyValue
_curses_setupterm_impl(PyObject *module, const char *term, int fd);

static PyValue
_curses_setupterm(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(term)), _PyValue_STATIC_IMMORTAL(&_Py_ID(fd)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"term", "fd", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "setupterm",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    const char *term = NULL;
    int fd = -1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    if (_idx[0] >= 0) {
        if (!PyValue_IsObject(PyValue_Borrow(args[_idx[0]]))) {
            _PyArg_BadArgumentValue("setupterm", "argument 'term'", "str or None", PyValue_Borrow(args[_idx[0]]));
            goto exit;
        }
        if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])) == Py_None) {
            term = NULL;
        }
        else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])))) {
            term = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            if (term == NULL) {
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("setupterm", "argument 'term'", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[_idx[0]])));
            goto exit;
        }
    }
    if (_idx[1] >= 0) {
        fd = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (fd == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = _curses_setupterm_impl(PyValue_AsKnownObjectBorrow(module), term, fd);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_newterm__doc__,
"newterm($module, type=None, fd=None, infd=None, /)\n"
"--\n"
"\n"
"Return a new screen for the terminal, in addition to the initial screen.\n"
"\n"
"  type\n"
"    Terminal name; if None, the TERM environment variable is used.\n"
"  fd\n"
"    Output file object or descriptor (default: sys.stdout).\n"
"  infd\n"
"    Input file object or descriptor (default: sys.stdin).\n"
"\n"
"This is an alternative to initscr() for programs running on more than\n"
"one terminal.  Use set_term() to switch between the screens.");

#define _CURSES_NEWTERM_METHODDEF    \
    {"newterm", _PyCFunction_CAST(_curses_newterm), METH_VALUE, _curses_newterm__doc__},

static PyValue
_curses_newterm_impl(PyObject *module, const char *type, PyValueBorrow fd,
                     PyValueBorrow infd);

static PyValue
_curses_newterm(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *type = NULL;
    PyValueBorrow fd = PyValue_Borrow(PyValue_None);
    PyValueBorrow infd = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("newterm", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("newterm", "argument 1", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        type = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        type = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (type == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("newterm", "argument 1", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    fd = PyValue_Borrow(args[1]);
    if (nargs < 3) {
        goto skip_optional;
    }
    infd = PyValue_Borrow(args[2]);
skip_optional:
    return_value = _curses_newterm_impl(PyValue_AsKnownObjectBorrow(module), type, fd, infd);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_set_term__doc__,
"set_term($module, screen, /)\n"
"--\n"
"\n"
"Switch to the given screen and return the previously current screen.\n"
"\n"
"Returns None if the previous screen was the one created by initscr().");

#define _CURSES_SET_TERM_METHODDEF    \
    {"set_term", _PyCFunction_CAST(_curses_set_term), METH_VALUE|METH_O, _curses_set_term__doc__},

static PyValue
_curses_set_term_impl(PyObject *module, PyValueBorrow screen);

static PyValue
_curses_set_term(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow screen;

    screen = PyValue_Borrow(args[0]);
    return_value = _curses_set_term_impl(PyValue_AsKnownObjectBorrow(module), screen);

    return return_value;
}

#if defined(HAVE_CURSES_NEW_PRESCR)

PyDoc_STRVAR(_curses_new_prescr__doc__,
"new_prescr($module, /)\n"
"--\n"
"\n"
"Create a screen and return it, without initializing a terminal.\n"
"\n"
"The screen can be used to call functions that affect the screen before\n"
"calling newterm() or initscr().");

#define _CURSES_NEW_PRESCR_METHODDEF    \
    {"new_prescr", _PyCFunction_CAST(_curses_new_prescr), METH_VALUE|METH_NOARGS, _curses_new_prescr__doc__},

static PyValue
_curses_new_prescr_impl(PyObject *module);

static PyValue
_curses_new_prescr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_new_prescr_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_CURSES_NEW_PRESCR) */

#if defined(HAVE_CURSES_ESCDELAY)

PyDoc_STRVAR(_curses_get_escdelay__doc__,
"get_escdelay($module, /)\n"
"--\n"
"\n"
"Gets the curses ESCDELAY setting.\n"
"\n"
"Gets the number of milliseconds to wait after reading an escape\n"
"character, to distinguish between an individual escape character entered\n"
"on the keyboard from escape sequences sent by cursor and function keys.");

#define _CURSES_GET_ESCDELAY_METHODDEF    \
    {"get_escdelay", _PyCFunction_CAST(_curses_get_escdelay), METH_VALUE|METH_NOARGS, _curses_get_escdelay__doc__},

static PyValue
_curses_get_escdelay_impl(PyObject *module);

static PyValue
_curses_get_escdelay(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_get_escdelay_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_CURSES_ESCDELAY) */

#if defined(HAVE_CURSES_SET_ESCDELAY)

PyDoc_STRVAR(_curses_set_escdelay__doc__,
"set_escdelay($module, ms, /)\n"
"--\n"
"\n"
"Sets the curses ESCDELAY setting.\n"
"\n"
"  ms\n"
"    length of the delay in milliseconds.\n"
"\n"
"Sets the number of milliseconds to wait after reading an escape\n"
"character, to distinguish between an individual escape character entered\n"
"on the keyboard from escape sequences sent by cursor and function keys.");

#define _CURSES_SET_ESCDELAY_METHODDEF    \
    {"set_escdelay", _PyCFunction_CAST(_curses_set_escdelay), METH_VALUE|METH_O, _curses_set_escdelay__doc__},

static PyValue
_curses_set_escdelay_impl(PyObject *module, int ms);

static PyValue
_curses_set_escdelay(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int ms;

    ms = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (ms == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_set_escdelay_impl(PyValue_AsKnownObjectBorrow(module), ms);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_SET_ESCDELAY) */

#if defined(HAVE_CURSES_TABSIZE)

PyDoc_STRVAR(_curses_get_tabsize__doc__,
"get_tabsize($module, /)\n"
"--\n"
"\n"
"Gets the curses TABSIZE setting.\n"
"\n"
"Gets the number of columns used by the curses library when converting\n"
"a tab character to spaces as it adds the tab to a window.");

#define _CURSES_GET_TABSIZE_METHODDEF    \
    {"get_tabsize", _PyCFunction_CAST(_curses_get_tabsize), METH_VALUE|METH_NOARGS, _curses_get_tabsize__doc__},

static PyValue
_curses_get_tabsize_impl(PyObject *module);

static PyValue
_curses_get_tabsize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_get_tabsize_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_CURSES_TABSIZE) */

#if defined(HAVE_CURSES_SET_TABSIZE)

PyDoc_STRVAR(_curses_set_tabsize__doc__,
"set_tabsize($module, size, /)\n"
"--\n"
"\n"
"Sets the curses TABSIZE setting.\n"
"\n"
"  size\n"
"    rendered cell width of a tab character.\n"
"\n"
"Sets the number of columns used by the curses library when converting\n"
"a tab character to spaces as it adds the tab to a window.");

#define _CURSES_SET_TABSIZE_METHODDEF    \
    {"set_tabsize", _PyCFunction_CAST(_curses_set_tabsize), METH_VALUE|METH_O, _curses_set_tabsize__doc__},

static PyValue
_curses_set_tabsize_impl(PyObject *module, int size);

static PyValue
_curses_set_tabsize(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int size;

    size = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_set_tabsize_impl(PyValue_AsKnownObjectBorrow(module), size);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_SET_TABSIZE) */

PyDoc_STRVAR(_curses_intrflush__doc__,
"intrflush($module, flag, /)\n"
"--\n"
"\n"
"Control flushing of the output buffer when an interrupt key is pressed.\n"
"\n"
"If flag is true, pressing an interrupt key (interrupt, break, or quit)\n"
"flushes all output in the terminal driver queue.  If flag is false, no\n"
"flushing is done.");

#define _CURSES_INTRFLUSH_METHODDEF    \
    {"intrflush", _PyCFunction_CAST(_curses_intrflush), METH_VALUE|METH_O, _curses_intrflush__doc__},

static PyValue
_curses_intrflush_impl(PyObject *module, int flag);

static PyValue
_curses_intrflush(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag;

    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
    return_value = _curses_intrflush_impl(PyValue_AsKnownObjectBorrow(module), flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_isendwin__doc__,
"isendwin($module, /)\n"
"--\n"
"\n"
"Return True if endwin() has been called.");

#define _CURSES_ISENDWIN_METHODDEF    \
    {"isendwin", _PyCFunction_CAST(_curses_isendwin), METH_VALUE|METH_NOARGS, _curses_isendwin__doc__},

static PyValue
_curses_isendwin_impl(PyObject *module);

static PyValue
_curses_isendwin(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_isendwin_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(HAVE_CURSES_IS_TERM_RESIZED)

PyDoc_STRVAR(_curses_is_term_resized__doc__,
"is_term_resized($module, nlines, ncols, /)\n"
"--\n"
"\n"
"Return True if resize_term() would modify the window structure, False otherwise.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.");

#define _CURSES_IS_TERM_RESIZED_METHODDEF    \
    {"is_term_resized", _PyCFunction_CAST(_curses_is_term_resized), METH_VALUE, _curses_is_term_resized__doc__},

static PyValue
_curses_is_term_resized_impl(PyObject *module, int nlines, int ncols);

static PyValue
_curses_is_term_resized(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int nlines;
    int ncols;

    if (!_PyArg_CheckPositional("is_term_resized", nargs, 2, 2)) {
        goto exit;
    }
    nlines = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (nlines == -1 && PyErr_Occurred()) {
        goto exit;
    }
    ncols = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (ncols == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_is_term_resized_impl(PyValue_AsKnownObjectBorrow(module), nlines, ncols);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_IS_TERM_RESIZED) */

PyDoc_STRVAR(_curses_keyname__doc__,
"keyname($module, key, /)\n"
"--\n"
"\n"
"Return the name of specified key.\n"
"\n"
"  key\n"
"    Key number.");

#define _CURSES_KEYNAME_METHODDEF    \
    {"keyname", _PyCFunction_CAST(_curses_keyname), METH_VALUE|METH_O, _curses_keyname__doc__},

static PyValue
_curses_keyname_impl(PyObject *module, int key);

static PyValue
_curses_keyname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int key;

    key = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (key == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_keyname_impl(PyValue_AsKnownObjectBorrow(module), key);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_killchar__doc__,
"killchar($module, /)\n"
"--\n"
"\n"
"Return the user\'s current line kill character.");

#define _CURSES_KILLCHAR_METHODDEF    \
    {"killchar", _PyCFunction_CAST(_curses_killchar), METH_VALUE|METH_NOARGS, _curses_killchar__doc__},

static PyValue
_curses_killchar_impl(PyObject *module);

static PyValue
_curses_killchar(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_killchar_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_killwchar__doc__,
"killwchar($module, /)\n"
"--\n"
"\n"
"Return the user\'s current wide-character line kill character.");

#define _CURSES_KILLWCHAR_METHODDEF    \
    {"killwchar", _PyCFunction_CAST(_curses_killwchar), METH_VALUE|METH_NOARGS, _curses_killwchar__doc__},

static PyValue
_curses_killwchar_impl(PyObject *module);

static PyValue
_curses_killwchar(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_killwchar_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_longname__doc__,
"longname($module, /)\n"
"--\n"
"\n"
"Return the terminfo long name field describing the current terminal.\n"
"\n"
"The maximum length of a verbose description is 128 characters.  It is\n"
"defined only after the call to initscr().");

#define _CURSES_LONGNAME_METHODDEF    \
    {"longname", _PyCFunction_CAST(_curses_longname), METH_VALUE|METH_NOARGS, _curses_longname__doc__},

static PyValue
_curses_longname_impl(PyObject *module);

static PyValue
_curses_longname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_longname_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_meta__doc__,
"meta($module, yes, /)\n"
"--\n"
"\n"
"Enable/disable meta keys.\n"
"\n"
"If yes is True, allow 8-bit characters to be input.  If yes is False,\n"
"allow only 7-bit characters.");

#define _CURSES_META_METHODDEF    \
    {"meta", _PyCFunction_CAST(_curses_meta), METH_VALUE|METH_O, _curses_meta__doc__},

static PyValue
_curses_meta_impl(PyObject *module, int yes);

static PyValue
_curses_meta(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int yes;

    yes = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (yes < 0) {
        goto exit;
    }
    return_value = _curses_meta_impl(PyValue_AsKnownObjectBorrow(module), yes);

exit:
    return return_value;
}

#if (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES)) && (defined(HAVE_CURSES_HAS_MOUSE) || defined(PDCURSES))

PyDoc_STRVAR(_curses_has_mouse__doc__,
"has_mouse($module, /)\n"
"--\n"
"\n"
"Return True if the mouse driver has been successfully initialized.");

#define _CURSES_HAS_MOUSE_METHODDEF    \
    {"has_mouse", _PyCFunction_CAST(_curses_has_mouse), METH_VALUE|METH_NOARGS, _curses_has_mouse__doc__},

static PyValue
_curses_has_mouse_impl(PyObject *module);

static PyValue
_curses_has_mouse(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_has_mouse_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES)) && (defined(HAVE_CURSES_HAS_MOUSE) || defined(PDCURSES)) */

#if (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES))

PyDoc_STRVAR(_curses_mouseinterval__doc__,
"mouseinterval($module, interval, /)\n"
"--\n"
"\n"
"Set and retrieve the maximum time between press and release in a click.\n"
"\n"
"  interval\n"
"    Time in milliseconds.\n"
"\n"
"Set the maximum time that can elapse between press and release events in\n"
"order for them to be recognized as a click, and return the previous\n"
"interval value.");

#define _CURSES_MOUSEINTERVAL_METHODDEF    \
    {"mouseinterval", _PyCFunction_CAST(_curses_mouseinterval), METH_VALUE|METH_O, _curses_mouseinterval__doc__},

static PyValue
_curses_mouseinterval_impl(PyObject *module, int interval);

static PyValue
_curses_mouseinterval(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int interval;

    interval = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (interval == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_mouseinterval_impl(PyValue_AsKnownObjectBorrow(module), interval);

exit:
    return return_value;
}

#endif /* (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES)) */

#if (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES))

PyDoc_STRVAR(_curses_mousemask__doc__,
"mousemask($module, newmask, /)\n"
"--\n"
"\n"
"Set the mouse events to be reported, and return (availmask, oldmask).\n"
"\n"
"Return a tuple (availmask, oldmask).  availmask indicates which of the\n"
"specified mouse events can be reported; on complete failure it returns\n"
"0.  oldmask is the previous value of the mouse event mask.  If this\n"
"function is never called, no mouse events are ever reported.");

#define _CURSES_MOUSEMASK_METHODDEF    \
    {"mousemask", _PyCFunction_CAST(_curses_mousemask), METH_VALUE|METH_O, _curses_mousemask__doc__},

static PyValue
_curses_mousemask_impl(PyObject *module, unsigned long newmask);

static PyValue
_curses_mousemask(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned long newmask;

        if (PyValue_TYPE(PyValue_Borrow(args[0]))->tp_as_number == NULL ||
            PyValue_TYPE(PyValue_Borrow(args[0]))->tp_as_number->nb_index == NULL)
        {
            _PyArg_BadArgumentValue("mousemask", "argument", "int", PyValue_Borrow(args[0]));
            goto exit;
        }

    {
        Py_ssize_t _bytes = _PyValue_AsNativeBytes(PyValue_Borrow(args[0]), &newmask, sizeof(unsigned long),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    return_value = _curses_mousemask_impl(PyValue_AsKnownObjectBorrow(module), newmask);

exit:
    return return_value;
}

#endif /* (defined(HAVE_CURSES_GETMOUSE) || defined(PDCURSES)) */

PyDoc_STRVAR(_curses_napms__doc__,
"napms($module, ms, /)\n"
"--\n"
"\n"
"Sleep for specified time.\n"
"\n"
"  ms\n"
"    Duration in milliseconds.");

#define _CURSES_NAPMS_METHODDEF    \
    {"napms", _PyCFunction_CAST(_curses_napms), METH_VALUE|METH_O, _curses_napms__doc__},

static int
_curses_napms_impl(PyObject *module, int ms);

static PyValue
_curses_napms(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int ms;
    int _return_value;

    ms = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (ms == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _curses_napms_impl(PyValue_AsKnownObjectBorrow(module), ms);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_newpad__doc__,
"newpad($module, nlines, ncols, /)\n"
"--\n"
"\n"
"Create and return a pointer to a new pad data structure.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.");

#define _CURSES_NEWPAD_METHODDEF    \
    {"newpad", _PyCFunction_CAST(_curses_newpad), METH_VALUE, _curses_newpad__doc__},

static PyValue
_curses_newpad_impl(PyObject *module, int nlines, int ncols);

static PyValue
_curses_newpad(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int nlines;
    int ncols;

    if (!_PyArg_CheckPositional("newpad", nargs, 2, 2)) {
        goto exit;
    }
    nlines = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (nlines == -1 && PyErr_Occurred()) {
        goto exit;
    }
    ncols = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (ncols == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_newpad_impl(PyValue_AsKnownObjectBorrow(module), nlines, ncols);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_newwin__doc__,
"newwin(nlines, ncols, [begin_y=0, begin_x=0])\n"
"Return a new window.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"  begin_y\n"
"    Top side y-coordinate.\n"
"  begin_x\n"
"    Left side x-coordinate.\n"
"\n"
"By default, the window will extend from the specified position to the\n"
"lower right corner of the screen.");

#define _CURSES_NEWWIN_METHODDEF    \
    {"newwin", _PyCFunction_CAST(_curses_newwin), METH_FASTCALL, _curses_newwin__doc__},

static PyObject *
_curses_newwin_impl(PyObject *module, int nlines, int ncols,
                    int group_right_1, int begin_y, int begin_x);

static PyObject *
_curses_newwin(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int nlines;
    int ncols;
    int group_right_1 = 0;
    int begin_y = 0;
    int begin_x = 0;

    switch (nargs) {
        case 2:
        case 4:
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.newwin requires 2 to 4 arguments");
            goto exit;
    }
    nlines = _PyValue_AsInt(PyValue_FromObjectBorrow(args[0]));
    if (nlines == -1 && PyErr_Occurred()) {
        goto exit;
    }
    ncols = _PyValue_AsInt(PyValue_FromObjectBorrow(args[1]));
    if (ncols == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs >= 4) {
        begin_y = _PyValue_AsInt(PyValue_FromObjectBorrow(args[2]));
        if (begin_y == -1 && PyErr_Occurred()) {
            goto exit;
        }
        begin_x = _PyValue_AsInt(PyValue_FromObjectBorrow(args[3]));
        if (begin_x == -1 && PyErr_Occurred()) {
            goto exit;
        }
        group_right_1 = 1;
    }
    return_value = _curses_newwin_impl(module, nlines, ncols, group_right_1, begin_y, begin_x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_nl__doc__,
"nl($module, flag=True, /)\n"
"--\n"
"\n"
"Enter newline mode.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling nonl().\n"
"\n"
"This mode translates the return key into newline on input, and\n"
"translates newline into return and line-feed on output.  Newline mode\n"
"is initially on.");

#define _CURSES_NL_METHODDEF    \
    {"nl", _PyCFunction_CAST(_curses_nl), METH_VALUE, _curses_nl__doc__},

static PyValue
_curses_nl_impl(PyObject *module, int flag);

static PyValue
_curses_nl(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("nl", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
skip_optional:
    return_value = _curses_nl_impl(PyValue_AsKnownObjectBorrow(module), flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_nocbreak__doc__,
"nocbreak($module, /)\n"
"--\n"
"\n"
"Leave cbreak mode.\n"
"\n"
"Return to normal \"cooked\" mode with line buffering.");

#define _CURSES_NOCBREAK_METHODDEF    \
    {"nocbreak", _PyCFunction_CAST(_curses_nocbreak), METH_VALUE|METH_NOARGS, _curses_nocbreak__doc__},

static PyValue
_curses_nocbreak_impl(PyObject *module);

static PyValue
_curses_nocbreak(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_nocbreak_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_noecho__doc__,
"noecho($module, /)\n"
"--\n"
"\n"
"Leave echo mode.\n"
"\n"
"Echoing of input characters is turned off.");

#define _CURSES_NOECHO_METHODDEF    \
    {"noecho", _PyCFunction_CAST(_curses_noecho), METH_VALUE|METH_NOARGS, _curses_noecho__doc__},

static PyValue
_curses_noecho_impl(PyObject *module);

static PyValue
_curses_noecho(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_noecho_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_nonl__doc__,
"nonl($module, /)\n"
"--\n"
"\n"
"Leave newline mode.\n"
"\n"
"Disable translation of return into newline on input, and disable\n"
"low-level translation of newline into newline/return on output.");

#define _CURSES_NONL_METHODDEF    \
    {"nonl", _PyCFunction_CAST(_curses_nonl), METH_VALUE|METH_NOARGS, _curses_nonl__doc__},

static PyValue
_curses_nonl_impl(PyObject *module);

static PyValue
_curses_nonl(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_nonl_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_noqiflush__doc__,
"noqiflush($module, /)\n"
"--\n"
"\n"
"Disable queue flushing.\n"
"\n"
"When queue flushing is disabled, normal flush of input and output queues\n"
"associated with the INTR, QUIT and SUSP characters will not be done.");

#define _CURSES_NOQIFLUSH_METHODDEF    \
    {"noqiflush", _PyCFunction_CAST(_curses_noqiflush), METH_VALUE|METH_NOARGS, _curses_noqiflush__doc__},

static PyValue
_curses_noqiflush_impl(PyObject *module);

static PyValue
_curses_noqiflush(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_noqiflush_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_noraw__doc__,
"noraw($module, /)\n"
"--\n"
"\n"
"Leave raw mode.\n"
"\n"
"Return to normal \"cooked\" mode with line buffering.");

#define _CURSES_NORAW_METHODDEF    \
    {"noraw", _PyCFunction_CAST(_curses_noraw), METH_VALUE|METH_NOARGS, _curses_noraw__doc__},

static PyValue
_curses_noraw_impl(PyObject *module);

static PyValue
_curses_noraw(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_noraw_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_pair_content__doc__,
"pair_content($module, pair_number, /)\n"
"--\n"
"\n"
"Return a tuple (fg, bg) containing the colors for the requested color pair.\n"
"\n"
"  pair_number\n"
"    The number of the color pair (0 - (COLOR_PAIRS-1)).");

#define _CURSES_PAIR_CONTENT_METHODDEF    \
    {"pair_content", _PyCFunction_CAST(_curses_pair_content), METH_VALUE|METH_O, _curses_pair_content__doc__},

static PyValue
_curses_pair_content_impl(PyObject *module, int pair_number);

static PyValue
_curses_pair_content(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int pair_number;

    if (!pair_converter(PyValue_Borrow(args[0]), &pair_number)) {
        goto exit;
    }
    return_value = _curses_pair_content_impl(PyValue_AsKnownObjectBorrow(module), pair_number);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_pair_number__doc__,
"pair_number($module, attr, /)\n"
"--\n"
"\n"
"Return the number of the color-pair set by the specified attribute value.\n"
"\n"
"color_pair() is the counterpart to this function.");

#define _CURSES_PAIR_NUMBER_METHODDEF    \
    {"pair_number", _PyCFunction_CAST(_curses_pair_number), METH_VALUE|METH_O, _curses_pair_number__doc__},

static PyValue
_curses_pair_number_impl(PyObject *module, attr_t attr);

static PyValue
_curses_pair_number(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_pair_number_impl(PyValue_AsKnownObjectBorrow(module), attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_putp__doc__,
"putp($module, string, /)\n"
"--\n"
"\n"
"Emit the value of a specified terminfo capability for the current terminal.\n"
"\n"
"Note that the output of putp() always goes to standard output.");

#define _CURSES_PUTP_METHODDEF    \
    {"putp", (PyCFunction)_curses_putp, METH_O, _curses_putp__doc__},

static PyObject *
_curses_putp_impl(PyObject *module, const char *string);

static PyObject *
_curses_putp(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *string;

    if (!PyArg_Parse(arg, "y:putp", &string)) {
        goto exit;
    }
    return_value = _curses_putp_impl(module, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_qiflush__doc__,
"qiflush($module, flag=True, /)\n"
"--\n"
"\n"
"Enable queue flushing.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling noqiflush().\n"
"\n"
"If queue flushing is enabled, all output in the display driver queue\n"
"will be flushed when the INTR, QUIT and SUSP characters are read.");

#define _CURSES_QIFLUSH_METHODDEF    \
    {"qiflush", _PyCFunction_CAST(_curses_qiflush), METH_VALUE, _curses_qiflush__doc__},

static PyValue
_curses_qiflush_impl(PyObject *module, int flag);

static PyValue
_curses_qiflush(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("qiflush", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
skip_optional:
    return_value = _curses_qiflush_impl(PyValue_AsKnownObjectBorrow(module), flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_update_lines_cols__doc__,
"update_lines_cols($module, /)\n"
"--\n"
"\n"
"Update the LINES and COLS module variables.\n"
"\n"
"This is useful for detecting manual screen resize.");

#define _CURSES_UPDATE_LINES_COLS_METHODDEF    \
    {"update_lines_cols", _PyCFunction_CAST(_curses_update_lines_cols), METH_VALUE|METH_NOARGS, _curses_update_lines_cols__doc__},

static PyValue
_curses_update_lines_cols_impl(PyObject *module);

static PyValue
_curses_update_lines_cols(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_update_lines_cols_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_raw__doc__,
"raw($module, flag=True, /)\n"
"--\n"
"\n"
"Enter raw mode.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling noraw().\n"
"\n"
"In raw mode, normal line buffering and processing of interrupt, quit,\n"
"suspend, and flow control keys are turned off; characters are presented\n"
"to curses input functions one by one.");

#define _CURSES_RAW_METHODDEF    \
    {"raw", _PyCFunction_CAST(_curses_raw), METH_VALUE, _curses_raw__doc__},

static PyValue
_curses_raw_impl(PyObject *module, int flag);

static PyValue
_curses_raw(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("raw", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
skip_optional:
    return_value = _curses_raw_impl(PyValue_AsKnownObjectBorrow(module), flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_reset_prog_mode__doc__,
"reset_prog_mode($module, /)\n"
"--\n"
"\n"
"Restore the terminal to \"program\" mode, as previously saved by def_prog_mode().");

#define _CURSES_RESET_PROG_MODE_METHODDEF    \
    {"reset_prog_mode", _PyCFunction_CAST(_curses_reset_prog_mode), METH_VALUE|METH_NOARGS, _curses_reset_prog_mode__doc__},

static PyValue
_curses_reset_prog_mode_impl(PyObject *module);

static PyValue
_curses_reset_prog_mode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_reset_prog_mode_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_reset_shell_mode__doc__,
"reset_shell_mode($module, /)\n"
"--\n"
"\n"
"Restore the terminal to \"shell\" mode, as previously saved by def_shell_mode().");

#define _CURSES_RESET_SHELL_MODE_METHODDEF    \
    {"reset_shell_mode", _PyCFunction_CAST(_curses_reset_shell_mode), METH_VALUE|METH_NOARGS, _curses_reset_shell_mode__doc__},

static PyValue
_curses_reset_shell_mode_impl(PyObject *module);

static PyValue
_curses_reset_shell_mode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_reset_shell_mode_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_resetty__doc__,
"resetty($module, /)\n"
"--\n"
"\n"
"Restore terminal mode.");

#define _CURSES_RESETTY_METHODDEF    \
    {"resetty", _PyCFunction_CAST(_curses_resetty), METH_VALUE|METH_NOARGS, _curses_resetty__doc__},

static PyValue
_curses_resetty_impl(PyObject *module);

static PyValue
_curses_resetty(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_resetty_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(HAVE_CURSES_RESIZETERM)

PyDoc_STRVAR(_curses_resizeterm__doc__,
"resizeterm($module, nlines, ncols, /)\n"
"--\n"
"\n"
"Resize the standard and current windows to the specified dimensions.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"\n"
"Adjusts other bookkeeping data used by the curses library that record\n"
"the window dimensions (in particular the SIGWINCH handler).");

#define _CURSES_RESIZETERM_METHODDEF    \
    {"resizeterm", _PyCFunction_CAST(_curses_resizeterm), METH_VALUE, _curses_resizeterm__doc__},

static PyValue
_curses_resizeterm_impl(PyObject *module, short nlines, short ncols);

static PyValue
_curses_resizeterm(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    short nlines;
    short ncols;

    if (!_PyArg_CheckPositional("resizeterm", nargs, 2, 2)) {
        goto exit;
    }
    {
        long ival = _PyValue_AsLong(PyValue_Borrow(args[0]));
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < SHRT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is less than minimum");
            goto exit;
        }
        else if (ival > SHRT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is greater than maximum");
            goto exit;
        }
        else {
            nlines = (short) ival;
        }
    }
    {
        long ival = _PyValue_AsLong(PyValue_Borrow(args[1]));
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < SHRT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is less than minimum");
            goto exit;
        }
        else if (ival > SHRT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is greater than maximum");
            goto exit;
        }
        else {
            ncols = (short) ival;
        }
    }
    return_value = _curses_resizeterm_impl(PyValue_AsKnownObjectBorrow(module), nlines, ncols);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_RESIZETERM) */

#if defined(HAVE_CURSES_RESIZE_TERM)

PyDoc_STRVAR(_curses_resize_term__doc__,
"resize_term($module, nlines, ncols, /)\n"
"--\n"
"\n"
"Backend function used by resizeterm(), performing most of the work.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"\n"
"When resizing the windows, resize_term() blank-fills the areas that are\n"
"extended.  The calling application should fill in these areas with\n"
"appropriate data.  The resize_term() function attempts to resize all\n"
"windows.  However, due to the calling convention of pads, it is not\n"
"possible to resize these without additional interaction with the\n"
"application.");

#define _CURSES_RESIZE_TERM_METHODDEF    \
    {"resize_term", _PyCFunction_CAST(_curses_resize_term), METH_VALUE, _curses_resize_term__doc__},

static PyValue
_curses_resize_term_impl(PyObject *module, short nlines, short ncols);

static PyValue
_curses_resize_term(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    short nlines;
    short ncols;

    if (!_PyArg_CheckPositional("resize_term", nargs, 2, 2)) {
        goto exit;
    }
    {
        long ival = _PyValue_AsLong(PyValue_Borrow(args[0]));
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < SHRT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is less than minimum");
            goto exit;
        }
        else if (ival > SHRT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is greater than maximum");
            goto exit;
        }
        else {
            nlines = (short) ival;
        }
    }
    {
        long ival = _PyValue_AsLong(PyValue_Borrow(args[1]));
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < SHRT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is less than minimum");
            goto exit;
        }
        else if (ival > SHRT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is greater than maximum");
            goto exit;
        }
        else {
            ncols = (short) ival;
        }
    }
    return_value = _curses_resize_term_impl(PyValue_AsKnownObjectBorrow(module), nlines, ncols);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_RESIZE_TERM) */

PyDoc_STRVAR(_curses_savetty__doc__,
"savetty($module, /)\n"
"--\n"
"\n"
"Save terminal mode.");

#define _CURSES_SAVETTY_METHODDEF    \
    {"savetty", _PyCFunction_CAST(_curses_savetty), METH_VALUE|METH_NOARGS, _curses_savetty__doc__},

static PyValue
_curses_savetty_impl(PyObject *module);

static PyValue
_curses_savetty(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_savetty_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(getsyx)

PyDoc_STRVAR(_curses_setsyx__doc__,
"setsyx($module, y, x, /)\n"
"--\n"
"\n"
"Set the virtual screen cursor.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"If y and x are both -1, then leaveok is set.");

#define _CURSES_SETSYX_METHODDEF    \
    {"setsyx", _PyCFunction_CAST(_curses_setsyx), METH_VALUE, _curses_setsyx__doc__},

static PyValue
_curses_setsyx_impl(PyObject *module, int y, int x);

static PyValue
_curses_setsyx(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int y;
    int x;

    if (!_PyArg_CheckPositional("setsyx", nargs, 2, 2)) {
        goto exit;
    }
    y = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    x = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_setsyx_impl(PyValue_AsKnownObjectBorrow(module), y, x);

exit:
    return return_value;
}

#endif /* defined(getsyx) */

PyDoc_STRVAR(_curses_start_color__doc__,
"start_color($module, /)\n"
"--\n"
"\n"
"Initializes eight basic colors and global variables COLORS and COLOR_PAIRS.\n"
"\n"
"Must be called if the programmer wants to use colors, and before any\n"
"other color manipulation routine is called.  It is good practice to call\n"
"this routine right after initscr().\n"
"\n"
"It also restores the colors on the terminal to the values they had when\n"
"the terminal was just turned on.");

#define _CURSES_START_COLOR_METHODDEF    \
    {"start_color", _PyCFunction_CAST(_curses_start_color), METH_VALUE|METH_NOARGS, _curses_start_color__doc__},

static PyValue
_curses_start_color_impl(PyObject *module);

static PyValue
_curses_start_color(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_start_color_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_termattrs__doc__,
"termattrs($module, /)\n"
"--\n"
"\n"
"Return a logical OR of all video attributes supported by the terminal.");

#define _CURSES_TERMATTRS_METHODDEF    \
    {"termattrs", _PyCFunction_CAST(_curses_termattrs), METH_VALUE|METH_NOARGS, _curses_termattrs__doc__},

static PyValue
_curses_termattrs_impl(PyObject *module);

static PyValue
_curses_termattrs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_termattrs_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#if defined(HAVE_CURSES_TERM_ATTRS)

PyDoc_STRVAR(_curses_term_attrs__doc__,
"term_attrs($module, /)\n"
"--\n"
"\n"
"Return a logical OR of all video attributes supported by the terminal.\n"
"\n"
"The attributes are WA_* values, the extended-attribute counterparts of\n"
"the A_* values returned by termattrs().");

#define _CURSES_TERM_ATTRS_METHODDEF    \
    {"term_attrs", _PyCFunction_CAST(_curses_term_attrs), METH_VALUE|METH_NOARGS, _curses_term_attrs__doc__},

static PyValue
_curses_term_attrs_impl(PyObject *module);

static PyValue
_curses_term_attrs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_term_attrs_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* defined(HAVE_CURSES_TERM_ATTRS) */

PyDoc_STRVAR(_curses_termname__doc__,
"termname($module, /)\n"
"--\n"
"\n"
"Return the value of the environment variable TERM, truncated to 14 characters.");

#define _CURSES_TERMNAME_METHODDEF    \
    {"termname", _PyCFunction_CAST(_curses_termname), METH_VALUE|METH_NOARGS, _curses_termname__doc__},

static PyValue
_curses_termname_impl(PyObject *module);

static PyValue
_curses_termname(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_termname_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_tigetflag__doc__,
"tigetflag($module, capname, /)\n"
"--\n"
"\n"
"Return the value of the Boolean capability.\n"
"\n"
"  capname\n"
"    The terminfo capability name.\n"
"\n"
"The value -1 is returned if capname is not a Boolean capability, or 0 if\n"
"it is canceled or absent from the terminal description.");

#define _CURSES_TIGETFLAG_METHODDEF    \
    {"tigetflag", _PyCFunction_CAST(_curses_tigetflag), METH_VALUE|METH_O, _curses_tigetflag__doc__},

static PyValue
_curses_tigetflag_impl(PyObject *module, const char *capname);

static PyValue
_curses_tigetflag(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *capname;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("tigetflag", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("tigetflag", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    capname = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (capname == NULL) {
        goto exit;
    }
    return_value = _curses_tigetflag_impl(PyValue_AsKnownObjectBorrow(module), capname);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_tigetnum__doc__,
"tigetnum($module, capname, /)\n"
"--\n"
"\n"
"Return the value of the numeric capability.\n"
"\n"
"  capname\n"
"    The terminfo capability name.\n"
"\n"
"The value -2 is returned if capname is not a numeric capability, or -1\n"
"if it is canceled or absent from the terminal description.");

#define _CURSES_TIGETNUM_METHODDEF    \
    {"tigetnum", _PyCFunction_CAST(_curses_tigetnum), METH_VALUE|METH_O, _curses_tigetnum__doc__},

static PyValue
_curses_tigetnum_impl(PyObject *module, const char *capname);

static PyValue
_curses_tigetnum(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *capname;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("tigetnum", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("tigetnum", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    capname = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (capname == NULL) {
        goto exit;
    }
    return_value = _curses_tigetnum_impl(PyValue_AsKnownObjectBorrow(module), capname);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_tigetstr__doc__,
"tigetstr($module, capname, /)\n"
"--\n"
"\n"
"Return the value of the string capability.\n"
"\n"
"  capname\n"
"    The terminfo capability name.\n"
"\n"
"None is returned if capname is not a string capability, or is canceled\n"
"or absent from the terminal description.");

#define _CURSES_TIGETSTR_METHODDEF    \
    {"tigetstr", _PyCFunction_CAST(_curses_tigetstr), METH_VALUE|METH_O, _curses_tigetstr__doc__},

static PyValue
_curses_tigetstr_impl(PyObject *module, const char *capname);

static PyValue
_curses_tigetstr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *capname;

    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("tigetstr", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("tigetstr", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    capname = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (capname == NULL) {
        goto exit;
    }
    return_value = _curses_tigetstr_impl(PyValue_AsKnownObjectBorrow(module), capname);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_tparm__doc__,
"tparm($module, str, i1=0, i2=0, i3=0, i4=0, i5=0, i6=0, i7=0, i8=0,\n"
"      i9=0, /)\n"
"--\n"
"\n"
"Instantiate the specified byte string with the supplied parameters.\n"
"\n"
"  str\n"
"    Parameterized byte string obtained from the terminfo database.");

#define _CURSES_TPARM_METHODDEF    \
    {"tparm", _PyCFunction_CAST(_curses_tparm), METH_FASTCALL, _curses_tparm__doc__},

static PyObject *
_curses_tparm_impl(PyObject *module, const char *str, int i1, int i2, int i3,
                   int i4, int i5, int i6, int i7, int i8, int i9);

static PyObject *
_curses_tparm(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *str;
    int i1 = 0;
    int i2 = 0;
    int i3 = 0;
    int i4 = 0;
    int i5 = 0;
    int i6 = 0;
    int i7 = 0;
    int i8 = 0;
    int i9 = 0;

    if (!PyArg_ParseArray(args, nargs, "y|iiiiiiiii:tparm",
        &str, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8, &i9)) {
        goto exit;
    }
    return_value = _curses_tparm_impl(module, str, i1, i2, i3, i4, i5, i6, i7, i8, i9);

exit:
    return return_value;
}

#if defined(HAVE_CURSES_TYPEAHEAD)

PyDoc_STRVAR(_curses_typeahead__doc__,
"typeahead($module, fd, /)\n"
"--\n"
"\n"
"Specify that the file descriptor fd be used for typeahead checking.\n"
"\n"
"  fd\n"
"    File descriptor.\n"
"\n"
"If fd is -1, then no typeahead checking is done.");

#define _CURSES_TYPEAHEAD_METHODDEF    \
    {"typeahead", _PyCFunction_CAST(_curses_typeahead), METH_VALUE|METH_O, _curses_typeahead__doc__},

static PyValue
_curses_typeahead_impl(PyObject *module, int fd);

static PyValue
_curses_typeahead(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fd;

    fd = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_typeahead_impl(PyValue_AsKnownObjectBorrow(module), fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_TYPEAHEAD) */

PyDoc_STRVAR(_curses_unctrl__doc__,
"unctrl($module, ch, /)\n"
"--\n"
"\n"
"Return a bytes object which is a printable representation of ch.\n"
"\n"
"Control characters are displayed as a caret followed by the\n"
"character, for example as ^C.  Printing characters are left as they\n"
"are.  Any attributes and color pair are ignored.  ch must fit in a\n"
"single byte; use wunctrl() for other characters.");

#define _CURSES_UNCTRL_METHODDEF    \
    {"unctrl", _PyCFunction_CAST(_curses_unctrl), METH_VALUE|METH_O, _curses_unctrl__doc__},

static PyValue
_curses_unctrl_impl(PyObject *module, PyValueBorrow ch);

static PyValue
_curses_unctrl(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow ch;

    ch = PyValue_Borrow(args[0]);
    return_value = _curses_unctrl_impl(PyValue_AsKnownObjectBorrow(module), ch);

    return return_value;
}

PyDoc_STRVAR(_curses_wunctrl__doc__,
"wunctrl($module, ch, /)\n"
"--\n"
"\n"
"Return a printable representation of the wide character ch.\n"
"\n"
"Control characters are displayed as a caret followed by the character,\n"
"for example as ^C.  Printing characters are left as they are.");

#define _CURSES_WUNCTRL_METHODDEF    \
    {"wunctrl", _PyCFunction_CAST(_curses_wunctrl), METH_VALUE|METH_O, _curses_wunctrl__doc__},

static PyValue
_curses_wunctrl_impl(PyObject *module, PyValueBorrow ch);

static PyValue
_curses_wunctrl(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow ch;

    ch = PyValue_Borrow(args[0]);
    return_value = _curses_wunctrl_impl(PyValue_AsKnownObjectBorrow(module), ch);

    return return_value;
}

PyDoc_STRVAR(_curses_ungetch__doc__,
"ungetch($module, ch, /)\n"
"--\n"
"\n"
"Push ch so the next getch() will return it.");

#define _CURSES_UNGETCH_METHODDEF    \
    {"ungetch", _PyCFunction_CAST(_curses_ungetch), METH_VALUE|METH_O, _curses_ungetch__doc__},

static PyValue
_curses_ungetch_impl(PyObject *module, PyValueBorrow ch);

static PyValue
_curses_ungetch(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow ch;

    ch = PyValue_Borrow(args[0]);
    return_value = _curses_ungetch_impl(PyValue_AsKnownObjectBorrow(module), ch);

    return return_value;
}

PyDoc_STRVAR(_curses_unget_wch__doc__,
"unget_wch($module, ch, /)\n"
"--\n"
"\n"
"Push ch so the next get_wch() will return it.");

#define _CURSES_UNGET_WCH_METHODDEF    \
    {"unget_wch", _PyCFunction_CAST(_curses_unget_wch), METH_VALUE|METH_O, _curses_unget_wch__doc__},

static PyValue
_curses_unget_wch_impl(PyObject *module, PyValueBorrow ch);

static PyValue
_curses_unget_wch(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow ch;

    ch = PyValue_Borrow(args[0]);
    return_value = _curses_unget_wch_impl(PyValue_AsKnownObjectBorrow(module), ch);

    return return_value;
}

PyDoc_STRVAR(_curses_slk_init__doc__,
"slk_init($module, fmt=0, /)\n"
"--\n"
"\n"
"Reserve a line for soft labels and choose their layout.\n"
"\n"
"  fmt\n"
"    Label layout: 0 = 3-2-3, 1 = 4-4 (8 labels each); 2 = 4-4-4,\n"
"    3 = 4-4-4 with an index line (12 labels each, ncurses extensions).\n"
"\n"
"Must be called before initscr() or newterm().");

#define _CURSES_SLK_INIT_METHODDEF    \
    {"slk_init", _PyCFunction_CAST(_curses_slk_init), METH_VALUE, _curses_slk_init__doc__},

static PyValue
_curses_slk_init_impl(PyObject *module, int fmt);

static PyValue
_curses_slk_init(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fmt = 0;

    if (!_PyArg_CheckPositional("slk_init", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    fmt = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (fmt == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_slk_init_impl(PyValue_AsKnownObjectBorrow(module), fmt);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_slk_set__doc__,
"slk_set($module, labnum, label, justify=0, /)\n"
"--\n"
"\n"
"Set the text of a soft label.\n"
"\n"
"  labnum\n"
"    The label number (1 to 8, or 1 to 12 in a 12-label layout).\n"
"  label\n"
"    The text to display.\n"
"  justify\n"
"    0 = left, 1 = center, 2 = right.");

#define _CURSES_SLK_SET_METHODDEF    \
    {"slk_set", _PyCFunction_CAST(_curses_slk_set), METH_VALUE, _curses_slk_set__doc__},

static PyValue
_curses_slk_set_impl(PyObject *module, int labnum, PyObject *label,
                     int justify);

static PyValue
_curses_slk_set(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int labnum;
    PyObject *label;
    int justify = 0;

    if (!_PyArg_CheckPositional("slk_set", nargs, 2, 3)) {
        goto exit;
    }
    labnum = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (labnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("slk_set", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("slk_set", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    label = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    if (nargs < 3) {
        goto skip_optional;
    }
    justify = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (justify == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_slk_set_impl(PyValue_AsKnownObjectBorrow(module), labnum, label, justify);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_slk_label__doc__,
"slk_label($module, labnum, /)\n"
"--\n"
"\n"
"Return the current text of a soft label.\n"
"\n"
"  labnum\n"
"    The label number.");

#define _CURSES_SLK_LABEL_METHODDEF    \
    {"slk_label", _PyCFunction_CAST(_curses_slk_label), METH_VALUE|METH_O, _curses_slk_label__doc__},

static PyValue
_curses_slk_label_impl(PyObject *module, int labnum);

static PyValue
_curses_slk_label(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int labnum;

    labnum = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (labnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_slk_label_impl(PyValue_AsKnownObjectBorrow(module), labnum);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_slk_refresh__doc__,
"slk_refresh($module, /)\n"
"--\n"
"\n"
"Update the soft labels on the screen.");

#define _CURSES_SLK_REFRESH_METHODDEF    \
    {"slk_refresh", _PyCFunction_CAST(_curses_slk_refresh), METH_VALUE|METH_NOARGS, _curses_slk_refresh__doc__},

static PyValue
_curses_slk_refresh_impl(PyObject *module);

static PyValue
_curses_slk_refresh(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_slk_refresh_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_slk_noutrefresh__doc__,
"slk_noutrefresh($module, /)\n"
"--\n"
"\n"
"Update the soft labels on the virtual screen only.");

#define _CURSES_SLK_NOUTREFRESH_METHODDEF    \
    {"slk_noutrefresh", _PyCFunction_CAST(_curses_slk_noutrefresh), METH_VALUE|METH_NOARGS, _curses_slk_noutrefresh__doc__},

static PyValue
_curses_slk_noutrefresh_impl(PyObject *module);

static PyValue
_curses_slk_noutrefresh(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_slk_noutrefresh_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_slk_clear__doc__,
"slk_clear($module, /)\n"
"--\n"
"\n"
"Erase the soft labels from the screen.");

#define _CURSES_SLK_CLEAR_METHODDEF    \
    {"slk_clear", _PyCFunction_CAST(_curses_slk_clear), METH_VALUE|METH_NOARGS, _curses_slk_clear__doc__},

static PyValue
_curses_slk_clear_impl(PyObject *module);

static PyValue
_curses_slk_clear(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_slk_clear_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_slk_restore__doc__,
"slk_restore($module, /)\n"
"--\n"
"\n"
"Restore the soft labels after a preceding slk_clear().");

#define _CURSES_SLK_RESTORE_METHODDEF    \
    {"slk_restore", _PyCFunction_CAST(_curses_slk_restore), METH_VALUE|METH_NOARGS, _curses_slk_restore__doc__},

static PyValue
_curses_slk_restore_impl(PyObject *module);

static PyValue
_curses_slk_restore(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_slk_restore_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_slk_touch__doc__,
"slk_touch($module, /)\n"
"--\n"
"\n"
"Force the soft labels to be redrawn by the next slk_refresh().");

#define _CURSES_SLK_TOUCH_METHODDEF    \
    {"slk_touch", _PyCFunction_CAST(_curses_slk_touch), METH_VALUE|METH_NOARGS, _curses_slk_touch__doc__},

static PyValue
_curses_slk_touch_impl(PyObject *module);

static PyValue
_curses_slk_touch(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_slk_touch_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_slk_attron__doc__,
"slk_attron($module, attr, /)\n"
"--\n"
"\n"
"Add the given chtype attributes to the soft labels.");

#define _CURSES_SLK_ATTRON_METHODDEF    \
    {"slk_attron", _PyCFunction_CAST(_curses_slk_attron), METH_VALUE|METH_O, _curses_slk_attron__doc__},

static PyValue
_curses_slk_attron_impl(PyObject *module, attr_t attr);

static PyValue
_curses_slk_attron(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_slk_attron_impl(PyValue_AsKnownObjectBorrow(module), attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_slk_attroff__doc__,
"slk_attroff($module, attr, /)\n"
"--\n"
"\n"
"Remove the given chtype attributes from the soft labels.");

#define _CURSES_SLK_ATTROFF_METHODDEF    \
    {"slk_attroff", _PyCFunction_CAST(_curses_slk_attroff), METH_VALUE|METH_O, _curses_slk_attroff__doc__},

static PyValue
_curses_slk_attroff_impl(PyObject *module, attr_t attr);

static PyValue
_curses_slk_attroff(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_slk_attroff_impl(PyValue_AsKnownObjectBorrow(module), attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_slk_attrset__doc__,
"slk_attrset($module, attr, /)\n"
"--\n"
"\n"
"Set the chtype attributes of the soft labels.");

#define _CURSES_SLK_ATTRSET_METHODDEF    \
    {"slk_attrset", _PyCFunction_CAST(_curses_slk_attrset), METH_VALUE|METH_O, _curses_slk_attrset__doc__},

static PyValue
_curses_slk_attrset_impl(PyObject *module, attr_t attr);

static PyValue
_curses_slk_attrset(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_slk_attrset_impl(PyValue_AsKnownObjectBorrow(module), attr);

exit:
    return return_value;
}

#if (defined(NCURSES_EXT_FUNCS) || defined(PDCURSES))

PyDoc_STRVAR(_curses_slk_attr__doc__,
"slk_attr($module, /)\n"
"--\n"
"\n"
"Return the current chtype attributes of the soft labels.");

#define _CURSES_SLK_ATTR_METHODDEF    \
    {"slk_attr", _PyCFunction_CAST(_curses_slk_attr), METH_VALUE|METH_NOARGS, _curses_slk_attr__doc__},

static PyValue
_curses_slk_attr_impl(PyObject *module);

static PyValue
_curses_slk_attr(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_slk_attr_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* (defined(NCURSES_EXT_FUNCS) || defined(PDCURSES)) */

#if defined(HAVE_CURSES_SLK_ATTR_ON)

PyDoc_STRVAR(_curses_slk_attr_on__doc__,
"slk_attr_on($module, attr, /)\n"
"--\n"
"\n"
"Turn on attributes of the soft labels without affecting others.");

#define _CURSES_SLK_ATTR_ON_METHODDEF    \
    {"slk_attr_on", _PyCFunction_CAST(_curses_slk_attr_on), METH_VALUE|METH_O, _curses_slk_attr_on__doc__},

static PyValue
_curses_slk_attr_on_impl(PyObject *module, attr_t attr);

static PyValue
_curses_slk_attr_on(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_slk_attr_on_impl(PyValue_AsKnownObjectBorrow(module), attr);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_SLK_ATTR_ON) */

#if defined(HAVE_CURSES_SLK_ATTR_OFF)

PyDoc_STRVAR(_curses_slk_attr_off__doc__,
"slk_attr_off($module, attr, /)\n"
"--\n"
"\n"
"Turn off attributes of the soft labels without affecting others.");

#define _CURSES_SLK_ATTR_OFF_METHODDEF    \
    {"slk_attr_off", _PyCFunction_CAST(_curses_slk_attr_off), METH_VALUE|METH_O, _curses_slk_attr_off__doc__},

static PyValue
_curses_slk_attr_off_impl(PyObject *module, attr_t attr);

static PyValue
_curses_slk_attr_off(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;

    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    return_value = _curses_slk_attr_off_impl(PyValue_AsKnownObjectBorrow(module), attr);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_SLK_ATTR_OFF) */

#if defined(HAVE_CURSES_SLK_ATTR_SET)

PyDoc_STRVAR(_curses_slk_attr_set__doc__,
"slk_attr_set($module, attr, pair=0, /)\n"
"--\n"
"\n"
"Set the attributes and color pair of the soft labels.");

#define _CURSES_SLK_ATTR_SET_METHODDEF    \
    {"slk_attr_set", _PyCFunction_CAST(_curses_slk_attr_set), METH_VALUE, _curses_slk_attr_set__doc__},

static PyValue
_curses_slk_attr_set_impl(PyObject *module, attr_t attr, int pair);

static PyValue
_curses_slk_attr_set(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    attr_t attr;
    int pair = 0;

    if (!_PyArg_CheckPositional("slk_attr_set", nargs, 1, 2)) {
        goto exit;
    }
    if (!attr_converter(PyValue_Borrow(args[0]), &attr)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!pair_converter(PyValue_Borrow(args[1]), &pair)) {
        goto exit;
    }
skip_optional:
    return_value = _curses_slk_attr_set_impl(PyValue_AsKnownObjectBorrow(module), attr, pair);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_SLK_ATTR_SET) */

#if defined(HAVE_CURSES_SLK_COLOR)

PyDoc_STRVAR(_curses_slk_color__doc__,
"slk_color($module, pair, /)\n"
"--\n"
"\n"
"Set the color pair of the soft labels.");

#define _CURSES_SLK_COLOR_METHODDEF    \
    {"slk_color", _PyCFunction_CAST(_curses_slk_color), METH_VALUE|METH_O, _curses_slk_color__doc__},

static PyValue
_curses_slk_color_impl(PyObject *module, int pair);

static PyValue
_curses_slk_color(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int pair;

    if (!pair_converter(PyValue_Borrow(args[0]), &pair)) {
        goto exit;
    }
    return_value = _curses_slk_color_impl(PyValue_AsKnownObjectBorrow(module), pair);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_SLK_COLOR) */

#if defined(HAVE_CURSES_USE_ENV)

PyDoc_STRVAR(_curses_use_env__doc__,
"use_env($module, flag, /)\n"
"--\n"
"\n"
"Use environment variables LINES and COLUMNS.\n"
"\n"
"If used, this function should be called before initscr() or newterm()\n"
"are called.\n"
"\n"
"When flag is False, the values of lines and columns specified in the\n"
"terminfo database will be used, even if environment variables LINES and\n"
"COLUMNS (used by default) are set, or if curses is running in a window\n"
"(in which case default behavior would be to use the window size if LINES\n"
"and COLUMNS are not set).");

#define _CURSES_USE_ENV_METHODDEF    \
    {"use_env", _PyCFunction_CAST(_curses_use_env), METH_VALUE|METH_O, _curses_use_env__doc__},

static PyValue
_curses_use_env_impl(PyObject *module, int flag);

static PyValue
_curses_use_env(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int flag;

    flag = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (flag < 0) {
        goto exit;
    }
    return_value = _curses_use_env_impl(PyValue_AsKnownObjectBorrow(module), flag);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_USE_ENV) */

#if !defined(STRICT_SYSV_CURSES)

PyDoc_STRVAR(_curses_use_default_colors__doc__,
"use_default_colors($module, /)\n"
"--\n"
"\n"
"Equivalent to assume_default_colors(-1, -1).");

#define _CURSES_USE_DEFAULT_COLORS_METHODDEF    \
    {"use_default_colors", _PyCFunction_CAST(_curses_use_default_colors), METH_VALUE|METH_NOARGS, _curses_use_default_colors__doc__},

static PyValue
_curses_use_default_colors_impl(PyObject *module);

static PyValue
_curses_use_default_colors(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_use_default_colors_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#endif /* !defined(STRICT_SYSV_CURSES) */

#if !defined(STRICT_SYSV_CURSES)

PyDoc_STRVAR(_curses_assume_default_colors__doc__,
"assume_default_colors($module, fg, bg, /)\n"
"--\n"
"\n"
"Allow use of default values for colors on terminals supporting this feature.\n"
"\n"
"Assign terminal default foreground/background colors to color number -1.\n"
"Change the definition of the color-pair 0 to (fg, bg).\n"
"\n"
"Use this to support transparency in your application.");

#define _CURSES_ASSUME_DEFAULT_COLORS_METHODDEF    \
    {"assume_default_colors", _PyCFunction_CAST(_curses_assume_default_colors), METH_VALUE, _curses_assume_default_colors__doc__},

static PyValue
_curses_assume_default_colors_impl(PyObject *module, int fg, int bg);

static PyValue
_curses_assume_default_colors(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int fg;
    int bg;

    if (!_PyArg_CheckPositional("assume_default_colors", nargs, 2, 2)) {
        goto exit;
    }
    fg = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (fg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    bg = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (bg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_assume_default_colors_impl(PyValue_AsKnownObjectBorrow(module), fg, bg);

exit:
    return return_value;
}

#endif /* !defined(STRICT_SYSV_CURSES) */

PyDoc_STRVAR(_curses_has_extended_color_support__doc__,
"has_extended_color_support($module, /)\n"
"--\n"
"\n"
"Return True if the module supports extended colors; otherwise, return False.\n"
"\n"
"Extended color support allows more than 256 color-pairs for terminals\n"
"that support more than 16 colors (e.g. xterm-256color).");

#define _CURSES_HAS_EXTENDED_COLOR_SUPPORT_METHODDEF    \
    {"has_extended_color_support", _PyCFunction_CAST(_curses_has_extended_color_support), METH_VALUE|METH_NOARGS, _curses_has_extended_color_support__doc__},

static PyValue
_curses_has_extended_color_support_impl(PyObject *module);

static PyValue
_curses_has_extended_color_support(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_has_extended_color_support_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

#ifndef _CURSES_WINDOW_ATTR_GET_METHODDEF
    #define _CURSES_WINDOW_ATTR_GET_METHODDEF
#endif /* !defined(_CURSES_WINDOW_ATTR_GET_METHODDEF) */

#ifndef _CURSES_WINDOW_ATTR_SET_METHODDEF
    #define _CURSES_WINDOW_ATTR_SET_METHODDEF
#endif /* !defined(_CURSES_WINDOW_ATTR_SET_METHODDEF) */

#ifndef _CURSES_WINDOW_ATTR_ON_METHODDEF
    #define _CURSES_WINDOW_ATTR_ON_METHODDEF
#endif /* !defined(_CURSES_WINDOW_ATTR_ON_METHODDEF) */

#ifndef _CURSES_WINDOW_ATTR_OFF_METHODDEF
    #define _CURSES_WINDOW_ATTR_OFF_METHODDEF
#endif /* !defined(_CURSES_WINDOW_ATTR_OFF_METHODDEF) */

#ifndef _CURSES_WINDOW_COLOR_SET_METHODDEF
    #define _CURSES_WINDOW_COLOR_SET_METHODDEF
#endif /* !defined(_CURSES_WINDOW_COLOR_SET_METHODDEF) */

#ifndef _CURSES_WINDOW_CHGAT_METHODDEF
    #define _CURSES_WINDOW_CHGAT_METHODDEF
#endif /* !defined(_CURSES_WINDOW_CHGAT_METHODDEF) */

#ifndef _CURSES_WINDOW_ENCLOSE_METHODDEF
    #define _CURSES_WINDOW_ENCLOSE_METHODDEF
#endif /* !defined(_CURSES_WINDOW_ENCLOSE_METHODDEF) */

#ifndef _CURSES_WINDOW_MOUSE_TRAFO_METHODDEF
    #define _CURSES_WINDOW_MOUSE_TRAFO_METHODDEF
#endif /* !defined(_CURSES_WINDOW_MOUSE_TRAFO_METHODDEF) */

#ifndef _CURSES_WINDOW_NOUTREFRESH_METHODDEF
    #define _CURSES_WINDOW_NOUTREFRESH_METHODDEF
#endif /* !defined(_CURSES_WINDOW_NOUTREFRESH_METHODDEF) */

#ifndef _CURSES_FILTER_METHODDEF
    #define _CURSES_FILTER_METHODDEF
#endif /* !defined(_CURSES_FILTER_METHODDEF) */

#ifndef _CURSES_NOFILTER_METHODDEF
    #define _CURSES_NOFILTER_METHODDEF
#endif /* !defined(_CURSES_NOFILTER_METHODDEF) */

#ifndef _CURSES_IS_CBREAK_METHODDEF
    #define _CURSES_IS_CBREAK_METHODDEF
#endif /* !defined(_CURSES_IS_CBREAK_METHODDEF) */

#ifndef _CURSES_IS_ECHO_METHODDEF
    #define _CURSES_IS_ECHO_METHODDEF
#endif /* !defined(_CURSES_IS_ECHO_METHODDEF) */

#ifndef _CURSES_IS_NL_METHODDEF
    #define _CURSES_IS_NL_METHODDEF
#endif /* !defined(_CURSES_IS_NL_METHODDEF) */

#ifndef _CURSES_IS_RAW_METHODDEF
    #define _CURSES_IS_RAW_METHODDEF
#endif /* !defined(_CURSES_IS_RAW_METHODDEF) */

#ifndef _CURSES_GETSYX_METHODDEF
    #define _CURSES_GETSYX_METHODDEF
#endif /* !defined(_CURSES_GETSYX_METHODDEF) */

#ifndef _CURSES_GETMOUSE_METHODDEF
    #define _CURSES_GETMOUSE_METHODDEF
#endif /* !defined(_CURSES_GETMOUSE_METHODDEF) */

#ifndef _CURSES_UNGETMOUSE_METHODDEF
    #define _CURSES_UNGETMOUSE_METHODDEF
#endif /* !defined(_CURSES_UNGETMOUSE_METHODDEF) */

#ifndef _CURSES_SCR_DUMP_METHODDEF
    #define _CURSES_SCR_DUMP_METHODDEF
#endif /* !defined(_CURSES_SCR_DUMP_METHODDEF) */

#ifndef _CURSES_SCR_RESTORE_METHODDEF
    #define _CURSES_SCR_RESTORE_METHODDEF
#endif /* !defined(_CURSES_SCR_RESTORE_METHODDEF) */

#ifndef _CURSES_SCR_INIT_METHODDEF
    #define _CURSES_SCR_INIT_METHODDEF
#endif /* !defined(_CURSES_SCR_INIT_METHODDEF) */

#ifndef _CURSES_SCR_SET_METHODDEF
    #define _CURSES_SCR_SET_METHODDEF
#endif /* !defined(_CURSES_SCR_SET_METHODDEF) */

#ifndef _CURSES_HAS_KEY_METHODDEF
    #define _CURSES_HAS_KEY_METHODDEF
#endif /* !defined(_CURSES_HAS_KEY_METHODDEF) */

#ifndef _CURSES_DEFINE_KEY_METHODDEF
    #define _CURSES_DEFINE_KEY_METHODDEF
#endif /* !defined(_CURSES_DEFINE_KEY_METHODDEF) */

#ifndef _CURSES_KEY_DEFINED_METHODDEF
    #define _CURSES_KEY_DEFINED_METHODDEF
#endif /* !defined(_CURSES_KEY_DEFINED_METHODDEF) */

#ifndef _CURSES_KEYOK_METHODDEF
    #define _CURSES_KEYOK_METHODDEF
#endif /* !defined(_CURSES_KEYOK_METHODDEF) */

#ifndef _CURSES_ALLOC_PAIR_METHODDEF
    #define _CURSES_ALLOC_PAIR_METHODDEF
#endif /* !defined(_CURSES_ALLOC_PAIR_METHODDEF) */

#ifndef _CURSES_FIND_PAIR_METHODDEF
    #define _CURSES_FIND_PAIR_METHODDEF
#endif /* !defined(_CURSES_FIND_PAIR_METHODDEF) */

#ifndef _CURSES_FREE_PAIR_METHODDEF
    #define _CURSES_FREE_PAIR_METHODDEF
#endif /* !defined(_CURSES_FREE_PAIR_METHODDEF) */

#ifndef _CURSES_RESET_COLOR_PAIRS_METHODDEF
    #define _CURSES_RESET_COLOR_PAIRS_METHODDEF
#endif /* !defined(_CURSES_RESET_COLOR_PAIRS_METHODDEF) */

#ifndef _CURSES_NEW_PRESCR_METHODDEF
    #define _CURSES_NEW_PRESCR_METHODDEF
#endif /* !defined(_CURSES_NEW_PRESCR_METHODDEF) */

#ifndef _CURSES_GET_ESCDELAY_METHODDEF
    #define _CURSES_GET_ESCDELAY_METHODDEF
#endif /* !defined(_CURSES_GET_ESCDELAY_METHODDEF) */

#ifndef _CURSES_SET_ESCDELAY_METHODDEF
    #define _CURSES_SET_ESCDELAY_METHODDEF
#endif /* !defined(_CURSES_SET_ESCDELAY_METHODDEF) */

#ifndef _CURSES_GET_TABSIZE_METHODDEF
    #define _CURSES_GET_TABSIZE_METHODDEF
#endif /* !defined(_CURSES_GET_TABSIZE_METHODDEF) */

#ifndef _CURSES_SET_TABSIZE_METHODDEF
    #define _CURSES_SET_TABSIZE_METHODDEF
#endif /* !defined(_CURSES_SET_TABSIZE_METHODDEF) */

#ifndef _CURSES_IS_TERM_RESIZED_METHODDEF
    #define _CURSES_IS_TERM_RESIZED_METHODDEF
#endif /* !defined(_CURSES_IS_TERM_RESIZED_METHODDEF) */

#ifndef _CURSES_HAS_MOUSE_METHODDEF
    #define _CURSES_HAS_MOUSE_METHODDEF
#endif /* !defined(_CURSES_HAS_MOUSE_METHODDEF) */

#ifndef _CURSES_MOUSEINTERVAL_METHODDEF
    #define _CURSES_MOUSEINTERVAL_METHODDEF
#endif /* !defined(_CURSES_MOUSEINTERVAL_METHODDEF) */

#ifndef _CURSES_MOUSEMASK_METHODDEF
    #define _CURSES_MOUSEMASK_METHODDEF
#endif /* !defined(_CURSES_MOUSEMASK_METHODDEF) */

#ifndef _CURSES_RESIZETERM_METHODDEF
    #define _CURSES_RESIZETERM_METHODDEF
#endif /* !defined(_CURSES_RESIZETERM_METHODDEF) */

#ifndef _CURSES_RESIZE_TERM_METHODDEF
    #define _CURSES_RESIZE_TERM_METHODDEF
#endif /* !defined(_CURSES_RESIZE_TERM_METHODDEF) */

#ifndef _CURSES_SETSYX_METHODDEF
    #define _CURSES_SETSYX_METHODDEF
#endif /* !defined(_CURSES_SETSYX_METHODDEF) */

#ifndef _CURSES_TERM_ATTRS_METHODDEF
    #define _CURSES_TERM_ATTRS_METHODDEF
#endif /* !defined(_CURSES_TERM_ATTRS_METHODDEF) */

#ifndef _CURSES_TYPEAHEAD_METHODDEF
    #define _CURSES_TYPEAHEAD_METHODDEF
#endif /* !defined(_CURSES_TYPEAHEAD_METHODDEF) */

#ifndef _CURSES_SLK_ATTR_METHODDEF
    #define _CURSES_SLK_ATTR_METHODDEF
#endif /* !defined(_CURSES_SLK_ATTR_METHODDEF) */

#ifndef _CURSES_SLK_ATTR_ON_METHODDEF
    #define _CURSES_SLK_ATTR_ON_METHODDEF
#endif /* !defined(_CURSES_SLK_ATTR_ON_METHODDEF) */

#ifndef _CURSES_SLK_ATTR_OFF_METHODDEF
    #define _CURSES_SLK_ATTR_OFF_METHODDEF
#endif /* !defined(_CURSES_SLK_ATTR_OFF_METHODDEF) */

#ifndef _CURSES_SLK_ATTR_SET_METHODDEF
    #define _CURSES_SLK_ATTR_SET_METHODDEF
#endif /* !defined(_CURSES_SLK_ATTR_SET_METHODDEF) */

#ifndef _CURSES_SLK_COLOR_METHODDEF
    #define _CURSES_SLK_COLOR_METHODDEF
#endif /* !defined(_CURSES_SLK_COLOR_METHODDEF) */

#ifndef _CURSES_USE_ENV_METHODDEF
    #define _CURSES_USE_ENV_METHODDEF
#endif /* !defined(_CURSES_USE_ENV_METHODDEF) */

#ifndef _CURSES_USE_DEFAULT_COLORS_METHODDEF
    #define _CURSES_USE_DEFAULT_COLORS_METHODDEF
#endif /* !defined(_CURSES_USE_DEFAULT_COLORS_METHODDEF) */

#ifndef _CURSES_ASSUME_DEFAULT_COLORS_METHODDEF
    #define _CURSES_ASSUME_DEFAULT_COLORS_METHODDEF
#endif /* !defined(_CURSES_ASSUME_DEFAULT_COLORS_METHODDEF) */
/*[clinic end generated code: output=569929b1c691a5fb input=a9049054013a1b77]*/
