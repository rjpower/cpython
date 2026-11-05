/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long_value.h"    // _PyValue_AsInt()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_ops.h"     // _PyValue_IsTrue()

PyDoc_STRVAR(readline_parse_and_bind__doc__,
"parse_and_bind($module, string, /)\n"
"--\n"
"\n"
"Execute the init line provided in the string argument.");

#define READLINE_PARSE_AND_BIND_METHODDEF    \
    {"parse_and_bind", _PyCFunction_CAST(readline_parse_and_bind), METH_VALUE|METH_O, readline_parse_and_bind__doc__},

static PyValue
readline_parse_and_bind_impl(PyObject *module, PyValueBorrow string);

static PyValue
readline_parse_and_bind(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow string;

    string = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_parse_and_bind_impl(PyValue_AsKnownObjectBorrow(module), string);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(readline_read_init_file__doc__,
"read_init_file($module, filename=None, /)\n"
"--\n"
"\n"
"Execute a readline initialization file.\n"
"\n"
"The default filename is the last filename used.");

#define READLINE_READ_INIT_FILE_METHODDEF    \
    {"read_init_file", _PyCFunction_CAST(readline_read_init_file), METH_VALUE, readline_read_init_file__doc__},

static PyValue
readline_read_init_file_impl(PyObject *module, PyValueBorrow filename_obj);

static PyValue
readline_read_init_file(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filename_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("read_init_file", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    filename_obj = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_read_init_file_impl(PyValue_AsKnownObjectBorrow(module), filename_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(readline_read_history_file__doc__,
"read_history_file($module, filename=None, /)\n"
"--\n"
"\n"
"Load a readline history file.\n"
"\n"
"The default filename is ~/.history.");

#define READLINE_READ_HISTORY_FILE_METHODDEF    \
    {"read_history_file", _PyCFunction_CAST(readline_read_history_file), METH_VALUE, readline_read_history_file__doc__},

static PyValue
readline_read_history_file_impl(PyObject *module, PyValueBorrow filename_obj);

static PyValue
readline_read_history_file(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filename_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("read_history_file", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    filename_obj = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_read_history_file_impl(PyValue_AsKnownObjectBorrow(module), filename_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(readline_write_history_file__doc__,
"write_history_file($module, filename=None, /)\n"
"--\n"
"\n"
"Save a readline history file.\n"
"\n"
"The default filename is ~/.history.");

#define READLINE_WRITE_HISTORY_FILE_METHODDEF    \
    {"write_history_file", _PyCFunction_CAST(readline_write_history_file), METH_VALUE, readline_write_history_file__doc__},

static PyValue
readline_write_history_file_impl(PyObject *module,
                                 PyValueBorrow filename_obj);

static PyValue
readline_write_history_file(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow filename_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("write_history_file", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    filename_obj = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_write_history_file_impl(PyValue_AsKnownObjectBorrow(module), filename_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#if defined(HAVE_RL_APPEND_HISTORY)

PyDoc_STRVAR(readline_append_history_file__doc__,
"append_history_file($module, nelements, filename=None, /)\n"
"--\n"
"\n"
"Append the last nelements items of the history list to file.\n"
"\n"
"The default filename is ~/.history.");

#define READLINE_APPEND_HISTORY_FILE_METHODDEF    \
    {"append_history_file", _PyCFunction_CAST(readline_append_history_file), METH_VALUE, readline_append_history_file__doc__},

static PyValue
readline_append_history_file_impl(PyObject *module, int nelements,
                                  PyValueBorrow filename_obj);

static PyValue
readline_append_history_file(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int nelements;
    PyValueBorrow filename_obj = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("append_history_file", nargs, 1, 2)) {
        goto exit;
    }
    nelements = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (nelements == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    filename_obj = PyValue_Borrow(args[1]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_append_history_file_impl(PyValue_AsKnownObjectBorrow(module), nelements, filename_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(HAVE_RL_APPEND_HISTORY) */

PyDoc_STRVAR(readline_set_history_length__doc__,
"set_history_length($module, length, /)\n"
"--\n"
"\n"
"Set the maximal number of lines which will be written to the history file.\n"
"\n"
"A negative length is used to inhibit history truncation.");

#define READLINE_SET_HISTORY_LENGTH_METHODDEF    \
    {"set_history_length", _PyCFunction_CAST(readline_set_history_length), METH_VALUE|METH_O, readline_set_history_length__doc__},

static PyValue
readline_set_history_length_impl(PyObject *module, int length);

static PyValue
readline_set_history_length(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int length;

    length = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (length == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = readline_set_history_length_impl(PyValue_AsKnownObjectBorrow(module), length);

exit:
    return return_value;
}

PyDoc_STRVAR(readline_get_history_length__doc__,
"get_history_length($module, /)\n"
"--\n"
"\n"
"Return the maximum number of lines that will be written to the history file.");

#define READLINE_GET_HISTORY_LENGTH_METHODDEF    \
    {"get_history_length", _PyCFunction_CAST(readline_get_history_length), METH_VALUE|METH_NOARGS, readline_get_history_length__doc__},

static PyValue
readline_get_history_length_impl(PyObject *module);

static PyValue
readline_get_history_length(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = readline_get_history_length_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(readline_set_completion_display_matches_hook__doc__,
"set_completion_display_matches_hook($module, function=None, /)\n"
"--\n"
"\n"
"Set or remove the completion display function.\n"
"\n"
"The function is called as\n"
"  function(substitution, [matches], longest_match_length)\n"
"once each time matches need to be displayed.");

#define READLINE_SET_COMPLETION_DISPLAY_MATCHES_HOOK_METHODDEF    \
    {"set_completion_display_matches_hook", _PyCFunction_CAST(readline_set_completion_display_matches_hook), METH_VALUE, readline_set_completion_display_matches_hook__doc__},

static PyValue
readline_set_completion_display_matches_hook_impl(PyObject *module,
                                                  PyValueBorrow function);

static PyValue
readline_set_completion_display_matches_hook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow function = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("set_completion_display_matches_hook", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    function = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_set_completion_display_matches_hook_impl(PyValue_AsKnownObjectBorrow(module), function);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(readline_set_startup_hook__doc__,
"set_startup_hook($module, function=None, /)\n"
"--\n"
"\n"
"Set or remove the function invoked by the rl_startup_hook callback.\n"
"\n"
"The function is called with no arguments just\n"
"before readline prints the first prompt.");

#define READLINE_SET_STARTUP_HOOK_METHODDEF    \
    {"set_startup_hook", _PyCFunction_CAST(readline_set_startup_hook), METH_VALUE, readline_set_startup_hook__doc__},

static PyValue
readline_set_startup_hook_impl(PyObject *module, PyValueBorrow function);

static PyValue
readline_set_startup_hook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow function = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("set_startup_hook", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    function = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_set_startup_hook_impl(PyValue_AsKnownObjectBorrow(module), function);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#if defined(HAVE_RL_PRE_INPUT_HOOK)

PyDoc_STRVAR(readline_set_pre_input_hook__doc__,
"set_pre_input_hook($module, function=None, /)\n"
"--\n"
"\n"
"Set or remove the function invoked by the rl_pre_input_hook callback.\n"
"\n"
"The function is called with no arguments after the first prompt\n"
"has been printed and just before readline starts reading input\n"
"characters.");

#define READLINE_SET_PRE_INPUT_HOOK_METHODDEF    \
    {"set_pre_input_hook", _PyCFunction_CAST(readline_set_pre_input_hook), METH_VALUE, readline_set_pre_input_hook__doc__},

static PyValue
readline_set_pre_input_hook_impl(PyObject *module, PyValueBorrow function);

static PyValue
readline_set_pre_input_hook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow function = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("set_pre_input_hook", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    function = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_set_pre_input_hook_impl(PyValue_AsKnownObjectBorrow(module), function);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(HAVE_RL_PRE_INPUT_HOOK) */

#if defined(HAVE_RL_PRE_INPUT_HOOK)

PyDoc_STRVAR(readline_get_pre_input_hook__doc__,
"get_pre_input_hook($module, /)\n"
"--\n"
"\n"
"Get the current pre-input hook function.");

#define READLINE_GET_PRE_INPUT_HOOK_METHODDEF    \
    {"get_pre_input_hook", _PyCFunction_CAST(readline_get_pre_input_hook), METH_VALUE|METH_NOARGS, readline_get_pre_input_hook__doc__},

static PyValue
readline_get_pre_input_hook_impl(PyObject *module);

static PyValue
readline_get_pre_input_hook(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_get_pre_input_hook_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(HAVE_RL_PRE_INPUT_HOOK) */

PyDoc_STRVAR(readline_get_completion_type__doc__,
"get_completion_type($module, /)\n"
"--\n"
"\n"
"Get the type of completion being attempted.");

#define READLINE_GET_COMPLETION_TYPE_METHODDEF    \
    {"get_completion_type", _PyCFunction_CAST(readline_get_completion_type), METH_VALUE|METH_NOARGS, readline_get_completion_type__doc__},

static PyValue
readline_get_completion_type_impl(PyObject *module);

static PyValue
readline_get_completion_type(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = readline_get_completion_type_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(readline_get_begidx__doc__,
"get_begidx($module, /)\n"
"--\n"
"\n"
"Get the beginning index of the completion scope.");

#define READLINE_GET_BEGIDX_METHODDEF    \
    {"get_begidx", _PyCFunction_CAST(readline_get_begidx), METH_VALUE|METH_NOARGS, readline_get_begidx__doc__},

static PyValue
readline_get_begidx_impl(PyObject *module);

static PyValue
readline_get_begidx(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = readline_get_begidx_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(readline_get_endidx__doc__,
"get_endidx($module, /)\n"
"--\n"
"\n"
"Get the ending index of the completion scope.");

#define READLINE_GET_ENDIDX_METHODDEF    \
    {"get_endidx", _PyCFunction_CAST(readline_get_endidx), METH_VALUE|METH_NOARGS, readline_get_endidx__doc__},

static PyValue
readline_get_endidx_impl(PyObject *module);

static PyValue
readline_get_endidx(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = readline_get_endidx_impl(PyValue_AsKnownObjectBorrow(module));

    return return_value;
}

PyDoc_STRVAR(readline_set_completer_delims__doc__,
"set_completer_delims($module, string, /)\n"
"--\n"
"\n"
"Set the word delimiters for completion.");

#define READLINE_SET_COMPLETER_DELIMS_METHODDEF    \
    {"set_completer_delims", _PyCFunction_CAST(readline_set_completer_delims), METH_VALUE|METH_O, readline_set_completer_delims__doc__},

static PyValue
readline_set_completer_delims_impl(PyObject *module, PyValueBorrow string);

static PyValue
readline_set_completer_delims(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow string;

    string = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_set_completer_delims_impl(PyValue_AsKnownObjectBorrow(module), string);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(readline_remove_history_item__doc__,
"remove_history_item($module, pos, /)\n"
"--\n"
"\n"
"Remove history item given by its zero-based position.");

#define READLINE_REMOVE_HISTORY_ITEM_METHODDEF    \
    {"remove_history_item", _PyCFunction_CAST(readline_remove_history_item), METH_VALUE|METH_O, readline_remove_history_item__doc__},

static PyValue
readline_remove_history_item_impl(PyObject *module, int entry_number);

static PyValue
readline_remove_history_item(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int entry_number;

    entry_number = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (entry_number == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_remove_history_item_impl(PyValue_AsKnownObjectBorrow(module), entry_number);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(readline_replace_history_item__doc__,
"replace_history_item($module, pos, line, /)\n"
"--\n"
"\n"
"Replaces history item given by its position with contents of line.\n"
"\n"
"pos is zero-based.");

#define READLINE_REPLACE_HISTORY_ITEM_METHODDEF    \
    {"replace_history_item", _PyCFunction_CAST(readline_replace_history_item), METH_VALUE, readline_replace_history_item__doc__},

static PyValue
readline_replace_history_item_impl(PyObject *module, int entry_number,
                                   PyObject *line);

static PyValue
readline_replace_history_item(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int entry_number;
    PyObject *line;

    if (!_PyArg_CheckPositional("replace_history_item", nargs, 2, 2)) {
        goto exit;
    }
    entry_number = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (entry_number == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[1]))) {
        _PyArg_BadArgumentValue("replace_history_item", "argument 2", "str", PyValue_Borrow(args[1]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])))) {
        _PyArg_BadArgument("replace_history_item", "argument 2", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1])));
        goto exit;
    }
    line = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[1]));
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_replace_history_item_impl(PyValue_AsKnownObjectBorrow(module), entry_number, line);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(readline_add_history__doc__,
"add_history($module, string, /)\n"
"--\n"
"\n"
"Add an item to the history buffer.");

#define READLINE_ADD_HISTORY_METHODDEF    \
    {"add_history", _PyCFunction_CAST(readline_add_history), METH_VALUE|METH_O, readline_add_history__doc__},

static PyValue
readline_add_history_impl(PyObject *module, PyValueBorrow string);

static PyValue
readline_add_history(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow string;

    string = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_add_history_impl(PyValue_AsKnownObjectBorrow(module), string);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(readline_set_auto_history__doc__,
"set_auto_history($module, enabled, /)\n"
"--\n"
"\n"
"Enables or disables automatic history.");

#define READLINE_SET_AUTO_HISTORY_METHODDEF    \
    {"set_auto_history", _PyCFunction_CAST(readline_set_auto_history), METH_VALUE|METH_O, readline_set_auto_history__doc__},

static PyValue
readline_set_auto_history_impl(PyObject *module,
                               int _should_auto_add_history);

static PyValue
readline_set_auto_history(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _should_auto_add_history;

    _should_auto_add_history = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (_should_auto_add_history < 0) {
        goto exit;
    }
    return_value = readline_set_auto_history_impl(PyValue_AsKnownObjectBorrow(module), _should_auto_add_history);

exit:
    return return_value;
}

PyDoc_STRVAR(readline_get_completer_delims__doc__,
"get_completer_delims($module, /)\n"
"--\n"
"\n"
"Get the word delimiters for completion.");

#define READLINE_GET_COMPLETER_DELIMS_METHODDEF    \
    {"get_completer_delims", _PyCFunction_CAST(readline_get_completer_delims), METH_VALUE|METH_NOARGS, readline_get_completer_delims__doc__},

static PyValue
readline_get_completer_delims_impl(PyObject *module);

static PyValue
readline_get_completer_delims(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_get_completer_delims_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(readline_set_completer__doc__,
"set_completer($module, function=None, /)\n"
"--\n"
"\n"
"Set or remove the completer function.\n"
"\n"
"The function is called as function(text, state),\n"
"for state in 0, 1, 2, ..., until it returns a non-string.\n"
"It should return the next possible completion starting with \'text\'.");

#define READLINE_SET_COMPLETER_METHODDEF    \
    {"set_completer", _PyCFunction_CAST(readline_set_completer), METH_VALUE, readline_set_completer__doc__},

static PyValue
readline_set_completer_impl(PyObject *module, PyValueBorrow function);

static PyValue
readline_set_completer(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow function = PyValue_Borrow(PyValue_None);

    if (!_PyArg_CheckPositional("set_completer", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    function = PyValue_Borrow(args[0]);
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_set_completer_impl(PyValue_AsKnownObjectBorrow(module), function);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(readline_get_completer__doc__,
"get_completer($module, /)\n"
"--\n"
"\n"
"Get the current completer function.");

#define READLINE_GET_COMPLETER_METHODDEF    \
    {"get_completer", _PyCFunction_CAST(readline_get_completer), METH_VALUE|METH_NOARGS, readline_get_completer__doc__},

static PyValue
readline_get_completer_impl(PyObject *module);

static PyValue
readline_get_completer(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_get_completer_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(readline_get_history_item__doc__,
"get_history_item($module, index, /)\n"
"--\n"
"\n"
"Return the current contents of history item at one-based index.");

#define READLINE_GET_HISTORY_ITEM_METHODDEF    \
    {"get_history_item", _PyCFunction_CAST(readline_get_history_item), METH_VALUE|METH_O, readline_get_history_item__doc__},

static PyValue
readline_get_history_item_impl(PyObject *module, int idx);

static PyValue
readline_get_history_item(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int idx;

    idx = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (idx == -1 && PyErr_Occurred()) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_get_history_item_impl(PyValue_AsKnownObjectBorrow(module), idx);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(readline_get_current_history_length__doc__,
"get_current_history_length($module, /)\n"
"--\n"
"\n"
"Return the current (not the maximum) length of history.");

#define READLINE_GET_CURRENT_HISTORY_LENGTH_METHODDEF    \
    {"get_current_history_length", _PyCFunction_CAST(readline_get_current_history_length), METH_VALUE|METH_NOARGS, readline_get_current_history_length__doc__},

static PyValue
readline_get_current_history_length_impl(PyObject *module);

static PyValue
readline_get_current_history_length(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_get_current_history_length_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(readline_get_line_buffer__doc__,
"get_line_buffer($module, /)\n"
"--\n"
"\n"
"Return the current contents of the line buffer.");

#define READLINE_GET_LINE_BUFFER_METHODDEF    \
    {"get_line_buffer", _PyCFunction_CAST(readline_get_line_buffer), METH_VALUE|METH_NOARGS, readline_get_line_buffer__doc__},

static PyValue
readline_get_line_buffer_impl(PyObject *module);

static PyValue
readline_get_line_buffer(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_get_line_buffer_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if defined(HAVE_RL_COMPLETION_APPEND_CHARACTER)

PyDoc_STRVAR(readline_clear_history__doc__,
"clear_history($module, /)\n"
"--\n"
"\n"
"Clear the current readline history.");

#define READLINE_CLEAR_HISTORY_METHODDEF    \
    {"clear_history", _PyCFunction_CAST(readline_clear_history), METH_VALUE|METH_NOARGS, readline_clear_history__doc__},

static PyValue
readline_clear_history_impl(PyObject *module);

static PyValue
readline_clear_history(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_clear_history_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(HAVE_RL_COMPLETION_APPEND_CHARACTER) */

PyDoc_STRVAR(readline_insert_text__doc__,
"insert_text($module, string, /)\n"
"--\n"
"\n"
"Insert text into the line buffer at the cursor position.");

#define READLINE_INSERT_TEXT_METHODDEF    \
    {"insert_text", _PyCFunction_CAST(readline_insert_text), METH_VALUE|METH_O, readline_insert_text__doc__},

static PyValue
readline_insert_text_impl(PyObject *module, PyValueBorrow string);

static PyValue
readline_insert_text(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow string;

    string = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_insert_text_impl(PyValue_AsKnownObjectBorrow(module), string);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(readline_redisplay__doc__,
"redisplay($module, /)\n"
"--\n"
"\n"
"Change what\'s displayed on the screen to reflect contents of the line buffer.");

#define READLINE_REDISPLAY_METHODDEF    \
    {"redisplay", _PyCFunction_CAST(readline_redisplay), METH_VALUE|METH_NOARGS, readline_redisplay__doc__},

static PyValue
readline_redisplay_impl(PyObject *module);

static PyValue
readline_redisplay(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(module));
    return_value = readline_redisplay_impl(PyValue_AsKnownObjectBorrow(module));
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#ifndef READLINE_APPEND_HISTORY_FILE_METHODDEF
    #define READLINE_APPEND_HISTORY_FILE_METHODDEF
#endif /* !defined(READLINE_APPEND_HISTORY_FILE_METHODDEF) */

#ifndef READLINE_SET_PRE_INPUT_HOOK_METHODDEF
    #define READLINE_SET_PRE_INPUT_HOOK_METHODDEF
#endif /* !defined(READLINE_SET_PRE_INPUT_HOOK_METHODDEF) */

#ifndef READLINE_GET_PRE_INPUT_HOOK_METHODDEF
    #define READLINE_GET_PRE_INPUT_HOOK_METHODDEF
#endif /* !defined(READLINE_GET_PRE_INPUT_HOOK_METHODDEF) */

#ifndef READLINE_CLEAR_HISTORY_METHODDEF
    #define READLINE_CLEAR_HISTORY_METHODDEF
#endif /* !defined(READLINE_CLEAR_HISTORY_METHODDEF) */
/*[clinic end generated code: output=bdf4ee1b0a4f5ddc input=a9049054013a1b77]*/
