/* Value-emission Argument Clinic output, compiled
   (pyjit/design/proposals/value-api-m3.md, section 2.7). */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#undef NDEBUG
#include "Python.h"
#include "pycore_tuple.h"        // _PyTuple_FromValueArray()
#include "pycore_value.h"
#include "pycore_value_call.h"   // METH_VALUE
#include "pycore_value_ops.h"    // _PyValue_IsTrue()
#include "pycore_value_slots.h"  // _PyValue_FromObjectResult()

/*[clinic input]
value_emission
module _testclinic_value
class _testclinic_value.Pair "PairObject *" "&Pair_Type"
class _testclinic_value.Probe "PyObject *" "&PyBaseObject_Type"
class _testclinic_value.Cell "PairObject *" "&Cell_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7aea4a52f2ec4d91]*/

typedef struct {
    PyObject_HEAD
    PyValue a;
    PyValue b;
} PairObject;

static PyTypeObject Pair_Type;
static PyTypeObject Cell_Type;
#include "clinic/_testclinic_value.c.h"

/* Return a tuple of the values (each an owner, closed here); an error
 * is PyValue_NULL. */
static PyValue
pack_values(PyValue *items, Py_ssize_t n)
{
    PyObject *tuple = _PyTuple_FromValueArray(items, n);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyValue_CLOSE(items[i]);
    }
    if (tuple == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(tuple);
}

/* ---- converters ---- */

/*[clinic input]
_testclinic_value.object_converter as object_converter
    a: object
    /
[clinic start generated code]*/

static PyValue
object_converter_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=07cadd3a9d67cd0c input=481e8c970dbbfe2d]*/
{
    return PyValue_DUP(a);
}

/*[clinic input]
_testclinic_value.objects_converter as objects_converter
    a: object
    b: object = None
    /
[clinic start generated code]*/

static PyValue
objects_converter_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=24dcf5bdcd3af81a input=4e1ca3defb1997d3]*/
{
    PyValue items[2] = {PyValue_DUP(a), PyValue_DUP(b)};
    return pack_values(items, 2);
}

/*[clinic input]
_testclinic_value.int_converter as int_converter
    a: int = 12
    b: int(accept={int}) = 34
    /
[clinic start generated code]*/

static PyValue
int_converter_impl(PyObject *module, int a, int b)
/*[clinic end generated code: output=3816f1949b8c23a8 input=3e960a9eccc7a874]*/
{
    PyValue items[2] = {PyValue_FromInt64(a), PyValue_FromInt64(b)};
    return pack_values(items, 2);
}

/*[clinic input]
_testclinic_value.long_converter as long_converter
    a: long = 12
    /
[clinic start generated code]*/

static PyValue
long_converter_impl(PyObject *module, long a)
/*[clinic end generated code: output=ef9890945af45424 input=1d97e02771cce11f]*/
{
    return PyValue_FromInt64(a);
}

/*[clinic input]
_testclinic_value.py_ssize_t_converter as py_ssize_t_converter
    a: Py_ssize_t = 12
    b: Py_ssize_t(accept={int, NoneType}) = 56
    c: Py_ssize_t(accept={int}, allow_negative=False) = 78
    d: Py_ssize_t(accept={int, NoneType}, allow_negative=False) = 90
    /
[clinic start generated code]*/

static PyValue
py_ssize_t_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                          Py_ssize_t c, Py_ssize_t d)
/*[clinic end generated code: output=26a520ce9d31e5ae input=6db068a90fb864fe]*/
{
    PyValue items[4] = {PyValue_FromInt64(a), PyValue_FromInt64(b),
                        PyValue_FromInt64(c), PyValue_FromInt64(d)};
    return pack_values(items, 4);
}

/*[clinic input]
_testclinic_value.slice_index_converter as slice_index_converter
    a: slice_index = 12
    b: slice_index(accept={int}) = 34
    c: slice_index(accept={int, NoneType}) = 56
    /
[clinic start generated code]*/

static PyValue
slice_index_converter_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b,
                           Py_ssize_t c)
/*[clinic end generated code: output=092b072188ad2765 input=14e9bafb3643cc10]*/
{
    PyValue items[3] = {PyValue_FromInt64(a), PyValue_FromInt64(b),
                        PyValue_FromInt64(c)};
    return pack_values(items, 3);
}

/*[clinic input]
_testclinic_value.bool_converter as bool_converter
    a: bool = True
    b: bool(accept={int}) = True
    /
[clinic start generated code]*/

static PyValue
bool_converter_impl(PyObject *module, int a, int b)
/*[clinic end generated code: output=74750749d16751fc input=bf7319e9d41b4005]*/
{
    PyValue items[2] = {a ? PyValue_True : PyValue_False,
                        b ? PyValue_True : PyValue_False};
    return pack_values(items, 2);
}

/*[clinic input]
_testclinic_value.double_converter as double_converter
    a: double = 12.5
    /
[clinic start generated code]*/

static PyValue
double_converter_impl(PyObject *module, double a)
/*[clinic end generated code: output=e0d522c50849eeab input=1785bad47127efa1]*/
{
    return PyValue_FromDouble(a);
}

/*[clinic input]
_testclinic_value.unicode_converter as unicode_converter
    a: unicode
    /
[clinic start generated code]*/

static PyValue
unicode_converter_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=0c2957956d63e0af input=d1bd79aeca31c4ec]*/
{
    return PyValue_FromObjectSteal(Py_NewRef(a));
}

/*[clinic input]
_testclinic_value.str_converter as str_converter
    a: str = "a"
    /
[clinic start generated code]*/

static PyValue
str_converter_impl(PyObject *module, const char *a)
/*[clinic end generated code: output=53a3754bfc6675f2 input=9525b9b7557c84d7]*/
{
    return _PyValue_FromObjectResult(PyUnicode_FromString(a));
}

/*[clinic input]
_testclinic_value.str_none_converter as str_none_converter
    a: str(accept={str, NoneType})
    /
[clinic start generated code]*/

static PyValue
str_none_converter_impl(PyObject *module, const char *a)
/*[clinic end generated code: output=fb619b0de02757b0 input=fa9e4910644b91ae]*/
{
    if (a == NULL) {
        return PyValue_None;
    }
    return _PyValue_FromObjectResult(PyUnicode_FromString(a));
}

/*[clinic input]
_testclinic_value.py_buffer_converter as py_buffer_converter
    a: Py_buffer
    /
[clinic start generated code]*/

static PyValue
py_buffer_converter_impl(PyObject *module, Py_buffer *a)
/*[clinic end generated code: output=d7474210a251c6bc input=0e4e199551578e6d]*/
{
    return PyValue_FromInt64(a->len);
}

/*[clinic input]
_testclinic_value.subclass_tuple as subclass_tuple
    a: object(subclass_of="&PyTuple_Type")
    /
[clinic start generated code]*/

static PyValue
subclass_tuple_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=20ba3a6d48604515 input=16073a68e7ee03b5]*/
{
    return PyValue_FromObjectSteal(Py_NewRef(a));
}

/*[clinic input]
_testclinic_value.subclass_long as subclass_long
    a: object(subclass_of="&PyLong_Type")
    /
[clinic start generated code]*/

static PyValue
subclass_long_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=602b273a88d14a4a input=8b62a7dd8d422a86]*/
{
    return PyValue_DUP(a);
}

/* ---- return converters ---- */

/*[clinic input]
_testclinic_value.ret_bool as ret_bool -> bool
    a: bool
    /
[clinic start generated code]*/

static int
ret_bool_impl(PyObject *module, int a)
/*[clinic end generated code: output=afe097824bdfcb1b input=201b33ef9188dcd4]*/
{
    return a;
}

/*[clinic input]
_testclinic_value.ret_int as ret_int -> int
    a: int
    /
[clinic start generated code]*/

static int
ret_int_impl(PyObject *module, int a)
/*[clinic end generated code: output=a3ce98e7c2a2f8ea input=2280a75057111996]*/
{
    return a;
}

/*[clinic input]
_testclinic_value.ret_ssize_t as ret_ssize_t -> Py_ssize_t
    a: Py_ssize_t
    /
[clinic start generated code]*/

static Py_ssize_t
ret_ssize_t_impl(PyObject *module, Py_ssize_t a)
/*[clinic end generated code: output=dd30b4e47e857709 input=b35b2816a0a12978]*/
{
    return a;
}

/*[clinic input]
_testclinic_value.ret_double as ret_double -> double
    a: double
    /
[clinic start generated code]*/

static double
ret_double_impl(PyObject *module, double a)
/*[clinic end generated code: output=66979b0bfb16bcb6 input=d8f770b589e7fdde]*/
{
    return a;
}

/* ---- shapes ---- */

/*[clinic input]
_testclinic_value.test_empty_function as test_empty_function
[clinic start generated code]*/

static PyValue
test_empty_function_impl(PyObject *module)
/*[clinic end generated code: output=6425683386d6a250 input=1516bf34c1122f8e]*/
{
    return PyValue_None;
}

/*[clinic input]
_testclinic_value.keywords as keywords
    a: object
    b: object
[clinic start generated code]*/

static PyValue
keywords_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=0d16945e0b7c1d58 input=b1ba07318986de94]*/
{
    PyValue items[2] = {PyValue_DUP(a), PyValue_DUP(b)};
    return pack_values(items, 2);
}

/*[clinic input]
_testclinic_value.keywords_kwonly as keywords_kwonly
    a: object
    *
    b: object
[clinic start generated code]*/

static PyValue
keywords_kwonly_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=58361bd4f83d8450 input=05d9d3324a2c29a5]*/
{
    PyValue items[2] = {PyValue_DUP(a), PyValue_DUP(b)};
    return pack_values(items, 2);
}

/*[clinic input]
_testclinic_value.keywords_opt as keywords_opt
    a: object
    b: object = None
    c: object = None
[clinic start generated code]*/

static PyValue
keywords_opt_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b,
                  PyValueBorrow c)
/*[clinic end generated code: output=3010b9c7dfaa55ef input=faa73b0bd839764d]*/
{
    PyValue items[3] = {PyValue_DUP(a), PyValue_DUP(b), PyValue_DUP(c)};
    return pack_values(items, 3);
}

/*[clinic input]
_testclinic_value.posonly_keywords as posonly_keywords
    a: object
    /
    b: object
[clinic start generated code]*/

static PyValue
posonly_keywords_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=ec0f5320a7b31157 input=38a3a71c58f90a1a]*/
{
    PyValue items[2] = {PyValue_DUP(a), PyValue_DUP(b)};
    return pack_values(items, 2);
}

/*[clinic input]
_testclinic_value.varpos as varpos
    *args: tuple
[clinic start generated code]*/

static PyValue
varpos_impl(PyObject *module, PyObject *args)
/*[clinic end generated code: output=d5e7eb3ce47fb344 input=5f82d9806b35b248]*/
{
    return PyValue_FromObjectSteal(Py_NewRef(args));
}

/*[clinic input]
_testclinic_value.varpos_array as varpos_array
    *args: array
[clinic start generated code]*/

static PyValue
varpos_array_impl(PyObject *module, const PyValue *args,
                  Py_ssize_t args_length)
/*[clinic end generated code: output=1c8d264a24201bd6 input=689b0ab8735a85d4]*/
{
    PyObject *tuple = _PyTuple_FromValueArray(args, args_length);
    if (tuple == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(tuple);
}

/*[clinic input]
_testclinic_value.kwds_with_pos_only as kwds_with_pos_only
    a: object
    b: object
    /
    **kwds: dict
[clinic start generated code]*/

static PyValue
kwds_with_pos_only_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b,
                        PyObject *kwds)
/*[clinic end generated code: output=d238cecbf883a2e2 input=a7e5ea0ba48f2de5]*/
{
    PyValue items[3] = {PyValue_DUP(a), PyValue_DUP(b),
                        PyValue_FromObjectSteal(Py_NewRef(kwds))};
    return pack_values(items, 3);
}

/* ---- classes ---- */

/*[clinic input]
@vectorcall
@classmethod
_testclinic_value.Pair.__new__
    a: object
    *
    b: object = None
[clinic start generated code]*/

static PyValue
_testclinic_value_Pair_impl(PyTypeObject *type, PyValueBorrow a,
                            PyValueBorrow b)
/*[clinic end generated code: output=101e1558c6bc0cd0 input=dd248f338cb86f52]*/
{
    PairObject *self = (PairObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return PyValue_NULL;
    }
    self->a = PyValue_DUP(a);
    self->b = PyValue_DUP(b);
    return PyValue_FromObjectSteal((PyObject *)self);
}

/*[clinic input]
_testclinic_value.Pair.get
[clinic start generated code]*/

static PyValue
_testclinic_value_Pair_get_impl(PairObject *self)
/*[clinic end generated code: output=a70e009235570d32 input=082c0036e4a2210c]*/
{
    PyValue items[2] = {PyValue_DUP(self->a), PyValue_DUP(self->b)};
    return pack_values(items, 2);
}

/* Cell: Pair's layout, filled by an __init__ with a value vectorcall. */

static PyObject *
Cell_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PairObject *self = (PairObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->a = PyValue_None;
    self->b = PyValue_None;
    return (PyObject *)self;
}

/*[clinic input]
@vectorcall
_testclinic_value.Cell.__init__
    a: object
    *
    b: object = None
[clinic start generated code]*/

static int
_testclinic_value_Cell___init___impl(PairObject *self, PyValueBorrow a,
                                     PyValueBorrow b)
/*[clinic end generated code: output=f6c4e9849264b500 input=ac1a5b9d8f83ee82]*/
{
    PyValue old_a = self->a;
    PyValue old_b = self->b;
    self->a = PyValue_DUP(a);
    self->b = PyValue_DUP(b);
    PyValue_CLOSE(old_a);
    PyValue_CLOSE(old_b);
    return 0;
}

/*[clinic input]
_testclinic_value.Probe.where
    cls: defining_class
[clinic start generated code]*/

static PyValue
_testclinic_value_Probe_where_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=bb208f53acd5ae33 input=bf12838c89aeb76f]*/
{
    return PyValue_FromObjectSteal(Py_NewRef((PyObject *)cls));
}

static void
Pair_dealloc(PyObject *op)
{
    PairObject *self = (PairObject *)op;
    PyValue_CLOSE(self->a);
    PyValue_CLOSE(self->b);
    Py_TYPE(op)->tp_free(op);
}

static PyMethodDef Pair_methods[] = {
    _TESTCLINIC_VALUE_PAIR_GET_METHODDEF
    {NULL, NULL}
};

static PyTypeObject Pair_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic_value.Pair",
    .tp_basicsize = sizeof(PairObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = _testclinic_value_Pair,
    .tp_value_vectorcall = (_Py_valuefunc)_testclinic_value_Pair_vectorcall,
    .tp_dealloc = Pair_dealloc,
    .tp_methods = Pair_methods,
};

static PyTypeObject Cell_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testclinic_value.Cell",
    .tp_basicsize = sizeof(PairObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Cell_new,
    .tp_init = _testclinic_value_Cell___init__,
    .tp_value_vectorcall = (_Py_valuefunc)_testclinic_value_Cell_vectorcall,
    .tp_dealloc = Pair_dealloc,
    .tp_methods = Pair_methods,
};

static PyMethodDef Probe_methods[] = {
    _TESTCLINIC_VALUE_PROBE_WHERE_METHODDEF
    {NULL, NULL}
};

static PyType_Slot Probe_slots[] = {
    {Py_tp_methods, Probe_methods},
    {Py_tp_new, PyType_GenericNew},
    {0, NULL}
};

static PyType_Spec Probe_spec = {
    .name = "_testclinic_value.Probe",
    .basicsize = sizeof(PyObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Probe_slots,
};

static PyMethodDef tester_methods[] = {
    OBJECT_CONVERTER_METHODDEF
    OBJECTS_CONVERTER_METHODDEF
    INT_CONVERTER_METHODDEF
    LONG_CONVERTER_METHODDEF
    PY_SSIZE_T_CONVERTER_METHODDEF
    SLICE_INDEX_CONVERTER_METHODDEF
    BOOL_CONVERTER_METHODDEF
    DOUBLE_CONVERTER_METHODDEF
    UNICODE_CONVERTER_METHODDEF
    STR_CONVERTER_METHODDEF
    STR_NONE_CONVERTER_METHODDEF
    PY_BUFFER_CONVERTER_METHODDEF
    SUBCLASS_TUPLE_METHODDEF
    SUBCLASS_LONG_METHODDEF
    RET_BOOL_METHODDEF
    RET_INT_METHODDEF
    RET_SSIZE_T_METHODDEF
    RET_DOUBLE_METHODDEF
    TEST_EMPTY_FUNCTION_METHODDEF
    KEYWORDS_METHODDEF
    KEYWORDS_KWONLY_METHODDEF
    KEYWORDS_OPT_METHODDEF
    POSONLY_KEYWORDS_METHODDEF
    VARPOS_METHODDEF
    VARPOS_ARRAY_METHODDEF
    KWDS_WITH_POS_ONLY_METHODDEF
    {NULL, NULL}
};

static int
_testclinic_value_exec(PyObject *module)
{
    if (PyModule_AddType(module, &Pair_Type) < 0) {
        return -1;
    }
    if (PyModule_AddType(module, &Cell_Type) < 0) {
        return -1;
    }
    PyObject *probe = PyType_FromModuleAndSpec(module, &Probe_spec, NULL);
    return PyModule_Add(module, "Probe", probe);
}

static PyModuleDef_Slot _testclinic_value_slots[] = {
    {Py_mod_exec, _testclinic_value_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _testclinic_value_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testclinic_value",
    .m_size = 0,
    .m_methods = tester_methods,
    .m_slots = _testclinic_value_slots,
};

PyMODINIT_FUNC
PyInit__testclinic_value(void)
{
    return PyModuleDef_Init(&_testclinic_value_module);
}
