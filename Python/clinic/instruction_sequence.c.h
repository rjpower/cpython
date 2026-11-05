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
#include "pycore_modsupport.h"    // _PyArg_NoPositionalStack()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_value_call.h"    // METH_VALUE
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

PyDoc_STRVAR(inst_seq_new__doc__,
"InstructionSequenceType()\n"
"--\n"
"\n"
"Create a new InstructionSequence object.");

static PyValue
inst_seq_new_impl(PyTypeObject *type);

static PyValue
inst_seq_new_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = &_PyInstructionSequence_Type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositionalStack("InstructionSequenceType", nargs)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("InstructionSequenceType", kwnames)) {
        goto exit;
    }
    return_value = inst_seq_new_impl(type);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
inst_seq_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(inst_seq_new_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = inst_seq_new_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

PyDoc_STRVAR(InstructionSequenceType_use_label__doc__,
"use_label($self, /, label)\n"
"--\n"
"\n"
"Place label at current location.");

#define INSTRUCTIONSEQUENCETYPE_USE_LABEL_METHODDEF    \
    {"use_label", _PyCFunction_CAST(InstructionSequenceType_use_label), METH_VALUE, InstructionSequenceType_use_label__doc__},

static PyValue
InstructionSequenceType_use_label_impl(_PyInstructionSequence *self,
                                       int label);

static PyValue
InstructionSequenceType_use_label(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(label)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"label", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "use_label",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    int label;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    label = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (label == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = InstructionSequenceType_use_label_impl((_PyInstructionSequence *)PyValue_AsKnownObjectBorrow(self), label);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_addop__doc__,
"addop($self, /, opcode, oparg, lineno, col_offset, end_lineno,\n"
"      end_col_offset)\n"
"--\n"
"\n"
"Append an instruction.");

#define INSTRUCTIONSEQUENCETYPE_ADDOP_METHODDEF    \
    {"addop", _PyCFunction_CAST(InstructionSequenceType_addop), METH_VALUE, InstructionSequenceType_addop__doc__},

static PyValue
InstructionSequenceType_addop_impl(_PyInstructionSequence *self, int opcode,
                                   int oparg, int lineno, int col_offset,
                                   int end_lineno, int end_col_offset);

static PyValue
InstructionSequenceType_addop(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(opcode)), _PyValue_STATIC_IMMORTAL(&_Py_ID(oparg)), _PyValue_STATIC_IMMORTAL(&_Py_ID(lineno)), _PyValue_STATIC_IMMORTAL(&_Py_ID(col_offset)), _PyValue_STATIC_IMMORTAL(&_Py_ID(end_lineno)), _PyValue_STATIC_IMMORTAL(&_Py_ID(end_col_offset)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"opcode", "oparg", "lineno", "col_offset", "end_lineno", "end_col_offset", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "addop",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[6];
    int opcode;
    int oparg;
    int lineno;
    int col_offset;
    int end_lineno;
    int end_col_offset;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 6, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, _idx, 6)) {
        goto exit;
    }
    opcode = _PyValue_AsInt(PyValue_Borrow(args[_idx[0]]));
    if (opcode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    oparg = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (oparg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    lineno = _PyValue_AsInt(PyValue_Borrow(args[_idx[2]]));
    if (lineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    col_offset = _PyValue_AsInt(PyValue_Borrow(args[_idx[3]]));
    if (col_offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    end_lineno = _PyValue_AsInt(PyValue_Borrow(args[_idx[4]]));
    if (end_lineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    end_col_offset = _PyValue_AsInt(PyValue_Borrow(args[_idx[5]]));
    if (end_col_offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = InstructionSequenceType_addop_impl((_PyInstructionSequence *)PyValue_AsKnownObjectBorrow(self), opcode, oparg, lineno, col_offset, end_lineno, end_col_offset);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_new_label__doc__,
"new_label($self, /)\n"
"--\n"
"\n"
"Return a new label.");

#define INSTRUCTIONSEQUENCETYPE_NEW_LABEL_METHODDEF    \
    {"new_label", _PyCFunction_CAST(InstructionSequenceType_new_label), METH_VALUE|METH_NOARGS, InstructionSequenceType_new_label__doc__},

static int
InstructionSequenceType_new_label_impl(_PyInstructionSequence *self);

static PyValue
InstructionSequenceType_new_label(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    _return_value = InstructionSequenceType_new_label_impl((_PyInstructionSequence *)PyValue_AsKnownObjectBorrow(self));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_add_nested__doc__,
"add_nested($self, /, nested)\n"
"--\n"
"\n"
"Add a nested sequence.");

#define INSTRUCTIONSEQUENCETYPE_ADD_NESTED_METHODDEF    \
    {"add_nested", _PyCFunction_CAST(InstructionSequenceType_add_nested), METH_VALUE, InstructionSequenceType_add_nested__doc__},

static PyValue
InstructionSequenceType_add_nested_impl(_PyInstructionSequence *self,
                                        PyValueBorrow nested);

static PyValue
InstructionSequenceType_add_nested(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(&_Py_ID(nested)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"nested", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "add_nested",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[1];
    PyValueBorrow nested;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 1)) {
        goto exit;
    }
    nested = PyValue_Borrow(args[_idx[0]]);
    return_value = InstructionSequenceType_add_nested_impl((_PyInstructionSequence *)PyValue_AsKnownObjectBorrow(self), nested);

exit:
    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_get_nested__doc__,
"get_nested($self, /)\n"
"--\n"
"\n"
"Add a nested sequence.");

#define INSTRUCTIONSEQUENCETYPE_GET_NESTED_METHODDEF    \
    {"get_nested", _PyCFunction_CAST(InstructionSequenceType_get_nested), METH_VALUE|METH_NOARGS, InstructionSequenceType_get_nested__doc__},

static PyValue
InstructionSequenceType_get_nested_impl(_PyInstructionSequence *self);

static PyValue
InstructionSequenceType_get_nested(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = InstructionSequenceType_get_nested_impl((_PyInstructionSequence *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}

PyDoc_STRVAR(InstructionSequenceType_get_instructions__doc__,
"get_instructions($self, /)\n"
"--\n"
"\n"
"Return the instructions as a list of tuples or labels.");

#define INSTRUCTIONSEQUENCETYPE_GET_INSTRUCTIONS_METHODDEF    \
    {"get_instructions", _PyCFunction_CAST(InstructionSequenceType_get_instructions), METH_VALUE|METH_NOARGS, InstructionSequenceType_get_instructions__doc__},

static PyValue
InstructionSequenceType_get_instructions_impl(_PyInstructionSequence *self);

static PyValue
InstructionSequenceType_get_instructions(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    return_value = InstructionSequenceType_get_instructions_impl((_PyInstructionSequence *)PyValue_AsKnownObjectBorrow(self));

    return return_value;
}
/*[clinic end generated code: output=aed7a05146bf87db input=a9049054013a1b77]*/
