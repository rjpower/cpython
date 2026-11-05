/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE

PyDoc_STRVAR(_curses_panel_panel_bottom__doc__,
"bottom($self, /)\n"
"--\n"
"\n"
"Push the panel to the bottom of the stack.");

#define _CURSES_PANEL_PANEL_BOTTOM_METHODDEF    \
    {"bottom", _PyCFunction_CAST(_curses_panel_panel_bottom), METH_VALUE|METH_NOARGS, _curses_panel_panel_bottom__doc__},

static PyValue
_curses_panel_panel_bottom_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_bottom(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_bottom_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_hide__doc__,
"hide($self, /)\n"
"--\n"
"\n"
"Hide the panel.\n"
"\n"
"This does not delete the object, it just makes the window on screen\n"
"invisible.");

#define _CURSES_PANEL_PANEL_HIDE_METHODDEF    \
    {"hide", _PyCFunction_CAST(_curses_panel_panel_hide), METH_VALUE|METH_NOARGS, _curses_panel_panel_hide__doc__},

static PyValue
_curses_panel_panel_hide_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_hide(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_hide_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_show__doc__,
"show($self, /)\n"
"--\n"
"\n"
"Display the panel (which might have been hidden).\n"
"\n"
"The panel is placed on top of the panel stack.");

#define _CURSES_PANEL_PANEL_SHOW_METHODDEF    \
    {"show", _PyCFunction_CAST(_curses_panel_panel_show), METH_VALUE|METH_NOARGS, _curses_panel_panel_show__doc__},

static PyValue
_curses_panel_panel_show_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_show(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_show_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_top__doc__,
"top($self, /)\n"
"--\n"
"\n"
"Push panel to the top of the stack.");

#define _CURSES_PANEL_PANEL_TOP_METHODDEF    \
    {"top", _PyCFunction_CAST(_curses_panel_panel_top), METH_VALUE|METH_NOARGS, _curses_panel_panel_top__doc__},

static PyValue
_curses_panel_panel_top_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_top(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_top_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_above__doc__,
"above($self, /)\n"
"--\n"
"\n"
"Return the panel above the current panel.");

#define _CURSES_PANEL_PANEL_ABOVE_METHODDEF    \
    {"above", _PyCFunction_CAST(_curses_panel_panel_above), METH_VALUE|METH_NOARGS, _curses_panel_panel_above__doc__},

static PyValue
_curses_panel_panel_above_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_above(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_above_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_below__doc__,
"below($self, /)\n"
"--\n"
"\n"
"Return the panel below the current panel.");

#define _CURSES_PANEL_PANEL_BELOW_METHODDEF    \
    {"below", _PyCFunction_CAST(_curses_panel_panel_below), METH_VALUE|METH_NOARGS, _curses_panel_panel_below__doc__},

static PyValue
_curses_panel_panel_below_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_below(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_below_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_hidden__doc__,
"hidden($self, /)\n"
"--\n"
"\n"
"Return True if the panel is hidden (not visible), False otherwise.");

#define _CURSES_PANEL_PANEL_HIDDEN_METHODDEF    \
    {"hidden", _PyCFunction_CAST(_curses_panel_panel_hidden), METH_VALUE|METH_NOARGS, _curses_panel_panel_hidden__doc__},

static PyValue
_curses_panel_panel_hidden_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_hidden(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_hidden_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_move__doc__,
"move($self, y, x, /)\n"
"--\n"
"\n"
"Move the panel to the screen coordinates (y, x).");

#define _CURSES_PANEL_PANEL_MOVE_METHODDEF    \
    {"move", _PyCFunction_CAST(_curses_panel_panel_move), METH_VALUE, _curses_panel_panel_move__doc__},

static PyValue
_curses_panel_panel_move_impl(PyCursesPanelObject *self, int y, int x);

static PyValue
_curses_panel_panel_move(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int y;
    int x;

    if (!_PyArg_CheckPositional("move", nargs, 2, 2)) {
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
    return_value = _curses_panel_panel_move_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self), y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_window__doc__,
"window($self, /)\n"
"--\n"
"\n"
"Return the window object associated with the panel.");

#define _CURSES_PANEL_PANEL_WINDOW_METHODDEF    \
    {"window", _PyCFunction_CAST(_curses_panel_panel_window), METH_VALUE|METH_NOARGS, _curses_panel_panel_window__doc__},

static PyValue
_curses_panel_panel_window_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_window(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_window_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_replace__doc__,
"replace($self, win, /)\n"
"--\n"
"\n"
"Change the window associated with the panel to the window win.");

#define _CURSES_PANEL_PANEL_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(_curses_panel_panel_replace), METH_VALUE|METH_O, _curses_panel_panel_replace__doc__},

static PyValue
_curses_panel_panel_replace_impl(PyCursesPanelObject *self,
                                 PyCursesWindowObject *win);

static PyValue
_curses_panel_panel_replace(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyCursesWindowObject *win;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyCursesWindow_Type)) {
        _PyArg_BadArgumentValue("replace", "argument", (&PyCursesWindow_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    win = (PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _curses_panel_panel_replace_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self), win);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_set_userptr__doc__,
"set_userptr($self, obj, /)\n"
"--\n"
"\n"
"Set the panel\'s user pointer to obj.");

#define _CURSES_PANEL_PANEL_SET_USERPTR_METHODDEF    \
    {"set_userptr", _PyCFunction_CAST(_curses_panel_panel_set_userptr), METH_VALUE|METH_O, _curses_panel_panel_set_userptr__doc__},

static PyValue
_curses_panel_panel_set_userptr_impl(PyCursesPanelObject *self,
                                     PyValueBorrow obj);

static PyValue
_curses_panel_panel_set_userptr(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow obj;

    obj = PyValue_Borrow(args[0]);
    return_value = _curses_panel_panel_set_userptr_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self), obj);

    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_userptr__doc__,
"userptr($self, /)\n"
"--\n"
"\n"
"Return the user pointer for the panel.");

#define _CURSES_PANEL_PANEL_USERPTR_METHODDEF    \
    {"userptr", _PyCFunction_CAST(_curses_panel_panel_userptr), METH_VALUE|METH_NOARGS, _curses_panel_panel_userptr__doc__},

static PyValue
_curses_panel_panel_userptr_impl(PyCursesPanelObject *self);

static PyValue
_curses_panel_panel_userptr(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_panel_userptr_impl((PyCursesPanelObject *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_bottom_panel__doc__,
"bottom_panel($module, /)\n"
"--\n"
"\n"
"Return the bottom panel in the panel stack.");

#define _CURSES_PANEL_BOTTOM_PANEL_METHODDEF    \
    {"bottom_panel", _PyCFunction_CAST(_curses_panel_bottom_panel), METH_VALUE|METH_NOARGS, _curses_panel_bottom_panel__doc__},

static PyValue
_curses_panel_bottom_panel_impl(PyObject *module);

static PyValue
_curses_panel_bottom_panel(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_bottom_panel_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_new_panel__doc__,
"new_panel($module, win, /)\n"
"--\n"
"\n"
"Return a panel object, associating it with the given window win.\n"
"\n"
"The new panel is placed on top of the panel stack.");

#define _CURSES_PANEL_NEW_PANEL_METHODDEF    \
    {"new_panel", _PyCFunction_CAST(_curses_panel_new_panel), METH_VALUE|METH_O, _curses_panel_new_panel__doc__},

static PyValue
_curses_panel_new_panel_impl(PyObject *module, PyCursesWindowObject *win);

static PyValue
_curses_panel_new_panel(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyCursesWindowObject *win;

    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyCursesWindow_Type)) {
        _PyArg_BadArgumentValue("new_panel", "argument", (&PyCursesWindow_Type)->tp_name, PyValue_Borrow(args[0]));
        goto exit;
    }
    win = (PyCursesWindowObject *)PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = _curses_panel_new_panel_impl(PyValue_AsKnownObjectBorrow(module), win);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_panel_top_panel__doc__,
"top_panel($module, /)\n"
"--\n"
"\n"
"Return the top panel in the panel stack.");

#define _CURSES_PANEL_TOP_PANEL_METHODDEF    \
    {"top_panel", _PyCFunction_CAST(_curses_panel_top_panel), METH_VALUE|METH_NOARGS, _curses_panel_top_panel__doc__},

static PyValue
_curses_panel_top_panel_impl(PyObject *module);

static PyValue
_curses_panel_top_panel(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_top_panel_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(_curses_panel_update_panels__doc__,
"update_panels($module, /)\n"
"--\n"
"\n"
"Updates the virtual screen after changes in the panel stack.\n"
"\n"
"This does not call curses.doupdate(), so you\'ll have to do this\n"
"yourself.");

#define _CURSES_PANEL_UPDATE_PANELS_METHODDEF    \
    {"update_panels", _PyCFunction_CAST(_curses_panel_update_panels), METH_VALUE|METH_NOARGS, _curses_panel_update_panels__doc__},

static PyValue
_curses_panel_update_panels_impl(PyObject *module);

static PyValue
_curses_panel_update_panels(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = _curses_panel_update_panels_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}
/*[clinic end generated code: output=d2a1a23a8a1e666b input=a9049054013a1b77]*/
