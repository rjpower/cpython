/*[clinic input]
output preset block
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3c81ac2402d06a8b]*/

/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
module m
class m.T "TestObj *" "TestType"
class m.I "PyValueBorrow" "&PyLong_Type"
class m.V "TestObj *" "TestType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=5b8e4e9a17b3a586]*/

/*[clinic input]
m.p_object
    a: object
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_object__doc__,
"p_object($module, a, /)\n"
"--\n"
"\n");

#define M_P_OBJECT_METHODDEF    \
    {"p_object", _PyCFunction_CAST(m_p_object), METH_VALUE, m_p_object__doc__},

static PyValue
m_p_object_impl(PyObject *module, PyValueBorrow a);

static PyValue
m_p_object(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("m.p_object()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = m_p_object_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_object_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=fc2533748096e8c2 input=6e2bfb9410f7ba39]*/


/*[clinic input]
m.p_object_converter
    a: object(converter="value_conv")
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_object_converter__doc__,
"p_object_converter($module, a, /)\n"
"--\n"
"\n");

#define M_P_OBJECT_CONVERTER_METHODDEF    \
    {"p_object_converter", _PyCFunction_CAST(m_p_object_converter), METH_VALUE, m_p_object_converter__doc__},

static PyValue
m_p_object_converter_impl(PyObject *module, PyObject *a);

static PyValue
m_p_object_converter(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;

    if (!_PyArg_CheckArityValue("m.p_object_converter()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!value_conv(PyValue_Borrow(args[0]), &a)) {
        goto exit;
    }
    return_value = m_p_object_converter_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_object_converter_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=fcb6a6c45f0c3096 input=42840537ef813516]*/


/*[clinic input]
m.p_subclass_str
    a: object(subclass_of="&PyUnicode_Type")
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_subclass_str__doc__,
"p_subclass_str($module, a, /)\n"
"--\n"
"\n");

#define M_P_SUBCLASS_STR_METHODDEF    \
    {"p_subclass_str", _PyCFunction_CAST(m_p_subclass_str), METH_VALUE, m_p_subclass_str__doc__},

static PyValue
m_p_subclass_str_impl(PyObject *module, PyObject *a);

static PyValue
m_p_subclass_str(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;

    if (!_PyArg_CheckArityValue("m.p_subclass_str()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyUnicode_Type)) {
        _PyArg_BadArgumentValue("p_subclass_str", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    a = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = m_p_subclass_str_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_subclass_str_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=c230348500e69567 input=5f4d91ab070d4160]*/


/*[clinic input]
m.p_subclass_long
    a: object(subclass_of="&PyLong_Type")
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_subclass_long__doc__,
"p_subclass_long($module, a, /)\n"
"--\n"
"\n");

#define M_P_SUBCLASS_LONG_METHODDEF    \
    {"p_subclass_long", _PyCFunction_CAST(m_p_subclass_long), METH_VALUE, m_p_subclass_long__doc__},

static PyValue
m_p_subclass_long_impl(PyObject *module, PyValueBorrow a);

static PyValue
m_p_subclass_long(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("m.p_subclass_long()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyType_IsSubtype(PyValue_TYPE(PyValue_Borrow(args[0])), &PyLong_Type)) {
        _PyArg_BadArgumentValue("p_subclass_long", "argument", "int", PyValue_Borrow(args[0]));
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = m_p_subclass_long_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_subclass_long_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=83e00c5ac234588e input=6c71b0d1a15fc934]*/


/*[clinic input]
m.p_none_cdefault
    a: object(c_default="NULL") = None
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_none_cdefault__doc__,
"p_none_cdefault($module, a=None, /)\n"
"--\n"
"\n");

#define M_P_NONE_CDEFAULT_METHODDEF    \
    {"p_none_cdefault", _PyCFunction_CAST(m_p_none_cdefault), METH_VALUE, m_p_none_cdefault__doc__},

static PyValue
m_p_none_cdefault_impl(PyObject *module, PyValueBorrow a);

static PyValue
m_p_none_cdefault(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a = PyValue_Borrow(PyValue_NULL);

    if (!_PyArg_NoKwnamesValue("m.p_none_cdefault()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("p_none_cdefault", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyValue_Borrow(args[0]);
skip_optional:
    return_value = m_p_none_cdefault_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_none_cdefault_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=342b61f275709e55 input=4d2090a33a64e235]*/


/*[clinic input]
m.p_none
    a: object = None
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_none__doc__,
"p_none($module, a=None, /)\n"
"--\n"
"\n");

#define M_P_NONE_METHODDEF    \
    {"p_none", _PyCFunction_CAST(m_p_none), METH_VALUE, m_p_none__doc__},

static PyValue
m_p_none_impl(PyObject *module, PyValueBorrow a);

static PyValue
m_p_none(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a = PyValue_Borrow(PyValue_None);

    if (!_PyArg_NoKwnamesValue("m.p_none()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("p_none", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    a = PyValue_Borrow(args[0]);
skip_optional:
    return_value = m_p_none_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_none_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=ed845fdb092aa9a5 input=71be72d79bb7ea47]*/


/*[clinic input]
m.p_int
    a: int
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_int__doc__,
"p_int($module, a, /)\n"
"--\n"
"\n");

#define M_P_INT_METHODDEF    \
    {"p_int", _PyCFunction_CAST(m_p_int), METH_VALUE, m_p_int__doc__},

static PyValue
m_p_int_impl(PyObject *module, int a);

static PyValue
m_p_int(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int a;

    if (!_PyArg_CheckArityValue("m.p_int()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = m_p_int_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_int_impl(PyObject *module, int a)
/*[clinic end generated code: output=1734bfd6aceafe3e input=eec185b4787c9f62]*/


/*[clinic input]
m.p_long
    a: long
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_long__doc__,
"p_long($module, a, /)\n"
"--\n"
"\n");

#define M_P_LONG_METHODDEF    \
    {"p_long", _PyCFunction_CAST(m_p_long), METH_VALUE, m_p_long__doc__},

static PyValue
m_p_long_impl(PyObject *module, long a);

static PyValue
m_p_long(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    long a;

    if (!_PyArg_CheckArityValue("m.p_long()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = _PyValue_AsLong(PyValue_Borrow(args[0]));
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = m_p_long_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_long_impl(PyObject *module, long a)
/*[clinic end generated code: output=100f1ecead65eff0 input=a3821cd31266be05]*/


/*[clinic input]
m.p_ssize_t
    a: Py_ssize_t
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_ssize_t__doc__,
"p_ssize_t($module, a, /)\n"
"--\n"
"\n");

#define M_P_SSIZE_T_METHODDEF    \
    {"p_ssize_t", _PyCFunction_CAST(m_p_ssize_t), METH_VALUE, m_p_ssize_t__doc__},

static PyValue
m_p_ssize_t_impl(PyObject *module, Py_ssize_t a);

static PyValue
m_p_ssize_t(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a;

    if (!_PyArg_CheckArityValue("m.p_ssize_t()", nargs, kwnames, 1)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        a = ival;
    }
    return_value = m_p_ssize_t_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_ssize_t_impl(PyObject *module, Py_ssize_t a)
/*[clinic end generated code: output=483e42b761a7e2d2 input=bac9c380f01f94bd]*/


/*[clinic input]
m.p_ssize_t_nonneg
    a: Py_ssize_t(allow_negative=False)
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_ssize_t_nonneg__doc__,
"p_ssize_t_nonneg($module, a, /)\n"
"--\n"
"\n");

#define M_P_SSIZE_T_NONNEG_METHODDEF    \
    {"p_ssize_t_nonneg", _PyCFunction_CAST(m_p_ssize_t_nonneg), METH_VALUE, m_p_ssize_t_nonneg__doc__},

static PyValue
m_p_ssize_t_nonneg_impl(PyObject *module, Py_ssize_t a);

static PyValue
m_p_ssize_t_nonneg(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a;

    if (!_PyArg_CheckArityValue("m.p_ssize_t_nonneg()", nargs, kwnames, 1)) {
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyValue iobj = _PyValue_Index(PyValue_Borrow(args[0]));
        if (!PyValue_IsNull(iobj)) {
            ival = _PyValue_IntAsSsize_t(PyValue_Borrow(iobj));
            PyValue_CLOSE(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        a = ival;
        if (a < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "a cannot be negative");
            goto exit;
        }
    }
    return_value = m_p_ssize_t_nonneg_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_ssize_t_nonneg_impl(PyObject *module, Py_ssize_t a)
/*[clinic end generated code: output=acec123210428528 input=a96c2fa19e8b457f]*/


/*[clinic input]
m.p_ssize_t_or_none
    a: Py_ssize_t(accept={int, NoneType}) = -1
    b: Py_ssize_t(accept={int, NoneType}, allow_negative=False) = -1
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_ssize_t_or_none__doc__,
"p_ssize_t_or_none($module, a=-1, b=-1, /)\n"
"--\n"
"\n");

#define M_P_SSIZE_T_OR_NONE_METHODDEF    \
    {"p_ssize_t_or_none", _PyCFunction_CAST(m_p_ssize_t_or_none), METH_VALUE, m_p_ssize_t_or_none__doc__},

static PyValue
m_p_ssize_t_or_none_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b);

static PyValue
m_p_ssize_t_or_none(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a = -1;
    Py_ssize_t b = -1;

    if (!_PyArg_NoKwnamesValue("m.p_ssize_t_or_none()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("p_ssize_t_or_none", nargs, 0, 2)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t_value(PyValue_Borrow(args[0]), &a)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_non_negative_ssize_t_value(PyValue_Borrow(args[1]), &b)) {
        goto exit;
    }
skip_optional:
    return_value = m_p_ssize_t_or_none_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

static PyValue
m_p_ssize_t_or_none_impl(PyObject *module, Py_ssize_t a, Py_ssize_t b)
/*[clinic end generated code: output=bbce0846e55b260c input=fe756f866b641af5]*/


/*[clinic input]
m.p_slice_index
    a: slice_index
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_slice_index__doc__,
"p_slice_index($module, a, /)\n"
"--\n"
"\n");

#define M_P_SLICE_INDEX_METHODDEF    \
    {"p_slice_index", _PyCFunction_CAST(m_p_slice_index), METH_VALUE, m_p_slice_index__doc__},

static PyValue
m_p_slice_index_impl(PyObject *module, Py_ssize_t a);

static PyValue
m_p_slice_index(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a;

    if (!_PyArg_CheckArityValue("m.p_slice_index()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!_PyArg_SliceIndexValue(PyValue_Borrow(args[0]), &a)) {
        goto exit;
    }
    return_value = m_p_slice_index_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_slice_index_impl(PyObject *module, Py_ssize_t a)
/*[clinic end generated code: output=be51ea78571a5510 input=fe61552ba797c26a]*/


/*[clinic input]
m.p_slice_index_int
    a: slice_index(accept={int})
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_slice_index_int__doc__,
"p_slice_index_int($module, a, /)\n"
"--\n"
"\n");

#define M_P_SLICE_INDEX_INT_METHODDEF    \
    {"p_slice_index_int", _PyCFunction_CAST(m_p_slice_index_int), METH_VALUE, m_p_slice_index_int__doc__},

static PyValue
m_p_slice_index_int_impl(PyObject *module, Py_ssize_t a);

static PyValue
m_p_slice_index_int(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t a;

    if (!_PyArg_CheckArityValue("m.p_slice_index_int()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!_PyArg_SliceIndexNotNoneValue(PyValue_Borrow(args[0]), &a)) {
        goto exit;
    }
    return_value = m_p_slice_index_int_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_slice_index_int_impl(PyObject *module, Py_ssize_t a)
/*[clinic end generated code: output=01872d557ac8c909 input=02e8a255649c9deb]*/


/*[clinic input]
m.p_bool
    a: bool
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_bool__doc__,
"p_bool($module, a, /)\n"
"--\n"
"\n");

#define M_P_BOOL_METHODDEF    \
    {"p_bool", _PyCFunction_CAST(m_p_bool), METH_VALUE, m_p_bool__doc__},

static PyValue
m_p_bool_impl(PyObject *module, int a);

static PyValue
m_p_bool(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int a;

    if (!_PyArg_CheckArityValue("m.p_bool()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = _PyValue_IsTrue(PyValue_Borrow(args[0]));
    if (a < 0) {
        goto exit;
    }
    return_value = m_p_bool_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_bool_impl(PyObject *module, int a)
/*[clinic end generated code: output=0485df8e8f8949ce input=44aa563200b9abfd]*/


/*[clinic input]
m.p_bool_int
    a: bool(accept={int})
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_bool_int__doc__,
"p_bool_int($module, a, /)\n"
"--\n"
"\n");

#define M_P_BOOL_INT_METHODDEF    \
    {"p_bool_int", _PyCFunction_CAST(m_p_bool_int), METH_VALUE, m_p_bool_int__doc__},

static PyValue
m_p_bool_int_impl(PyObject *module, int a);

static PyValue
m_p_bool_int(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int a;

    if (!_PyArg_CheckArityValue("m.p_bool_int()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = _PyValue_AsInt(PyValue_Borrow(args[0]));
    if (a == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = m_p_bool_int_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_bool_int_impl(PyObject *module, int a)
/*[clinic end generated code: output=95ee5807e6eaa25f input=1ab5dba2d89caeb5]*/


/*[clinic input]
m.p_double
    a: double
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_double__doc__,
"p_double($module, a, /)\n"
"--\n"
"\n");

#define M_P_DOUBLE_METHODDEF    \
    {"p_double", _PyCFunction_CAST(m_p_double), METH_VALUE, m_p_double__doc__},

static PyValue
m_p_double_impl(PyObject *module, double a);

static PyValue
m_p_double(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double a;

    if (!_PyArg_CheckArityValue("m.p_double()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = _PyValue_AsDouble(PyValue_Borrow(args[0]));
    if (a == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = m_p_double_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_double_impl(PyObject *module, double a)
/*[clinic end generated code: output=938664fba86358db input=834dc585c8d682b1]*/


/*[clinic input]
m.p_unicode
    a: unicode
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_unicode__doc__,
"p_unicode($module, a, /)\n"
"--\n"
"\n");

#define M_P_UNICODE_METHODDEF    \
    {"p_unicode", _PyCFunction_CAST(m_p_unicode), METH_VALUE, m_p_unicode__doc__},

static PyValue
m_p_unicode_impl(PyObject *module, PyObject *a);

static PyValue
m_p_unicode(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a;

    if (!_PyArg_CheckArityValue("m.p_unicode()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("p_unicode", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("p_unicode", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    a = PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]));
    return_value = m_p_unicode_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_unicode_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=8705f0d019c4f3bf input=dc63b84c18564b5e]*/


/*[clinic input]
m.p_str
    a: str
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_str__doc__,
"p_str($module, a, /)\n"
"--\n"
"\n");

#define M_P_STR_METHODDEF    \
    {"p_str", _PyCFunction_CAST(m_p_str), METH_VALUE, m_p_str__doc__},

static PyValue
m_p_str_impl(PyObject *module, const char *a);

static PyValue
m_p_str(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *a;

    if (!_PyArg_CheckArityValue("m.p_str()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("p_str", "argument", "str", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("p_str", "argument", "str", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    a = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
    if (a == NULL) {
        goto exit;
    }
    return_value = m_p_str_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_str_impl(PyObject *module, const char *a)
/*[clinic end generated code: output=a0e770d1b04967f5 input=300c1b82e89a8602]*/


/*[clinic input]
m.p_str_none
    a: str(accept={str, NoneType})
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_str_none__doc__,
"p_str_none($module, a, /)\n"
"--\n"
"\n");

#define M_P_STR_NONE_METHODDEF    \
    {"p_str_none", _PyCFunction_CAST(m_p_str_none), METH_VALUE, m_p_str_none__doc__},

static PyValue
m_p_str_none_impl(PyObject *module, const char *a);

static PyValue
m_p_str_none(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    const char *a;

    if (!_PyArg_CheckArityValue("m.p_str_none()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("p_str_none", "argument", "str or None", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])) == Py_None) {
        a = NULL;
    }
    else if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        a = _PyUnicode_AsUTF8NoNUL(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        if (a == NULL) {
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("p_str_none", "argument", "str or None", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    return_value = m_p_str_none_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_str_none_impl(PyObject *module, const char *a)
/*[clinic end generated code: output=863ebe3e5c79d6b1 input=e4c918eeb5121afa]*/


/*[clinic input]
m.p_fs_decoded
    a: unicode_fs_decoded
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_fs_decoded__doc__,
"p_fs_decoded($module, a, /)\n"
"--\n"
"\n");

#define M_P_FS_DECODED_METHODDEF    \
    {"p_fs_decoded", _PyCFunction_CAST(m_p_fs_decoded), METH_VALUE, m_p_fs_decoded__doc__},

static PyValue
m_p_fs_decoded_impl(PyObject *module, PyObject *a);

static PyValue
m_p_fs_decoded(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyObject *a = NULL;

    if (!_PyArg_CheckArityValue("m.p_fs_decoded()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "expected str, bytes or os.PathLike object, not %.200s", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (!PyUnicode_FSDecoder(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &a)) {
        goto exit;
    }
    return_value = m_p_fs_decoded_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    /* Cleanup for a */
    Py_XDECREF(a);

    return return_value;
}

static PyValue
m_p_fs_decoded_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=446449e7d9c83379 input=4c11647e9a9556e8]*/


/*[clinic input]
m.p_buffer
    a: Py_buffer
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_buffer__doc__,
"p_buffer($module, a, /)\n"
"--\n"
"\n");

#define M_P_BUFFER_METHODDEF    \
    {"p_buffer", _PyCFunction_CAST(m_p_buffer), METH_VALUE, m_p_buffer__doc__},

static PyValue
m_p_buffer_impl(PyObject *module, Py_buffer *a);

static PyValue
m_p_buffer(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer a = {NULL, NULL};

    if (!_PyArg_CheckArityValue("m.p_buffer()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &a, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = m_p_buffer_impl(PyValue_AsKnownObjectBorrow(module), &a);

exit:
    /* Cleanup for a */
    if (a.obj) {
       PyBuffer_Release(&a);
    }

    return return_value;
}

static PyValue
m_p_buffer_impl(PyObject *module, Py_buffer *a)
/*[clinic end generated code: output=ba437e620e7e36aa input=db645b20e4afb265]*/


/*[clinic input]
m.p_buffer_str
    a: Py_buffer(accept={str, buffer})
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_buffer_str__doc__,
"p_buffer_str($module, a, /)\n"
"--\n"
"\n");

#define M_P_BUFFER_STR_METHODDEF    \
    {"p_buffer_str", _PyCFunction_CAST(m_p_buffer_str), METH_VALUE, m_p_buffer_str__doc__},

static PyValue
m_p_buffer_str_impl(PyObject *module, Py_buffer *a);

static PyValue
m_p_buffer_str(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer a = {NULL, NULL};

    if (!_PyArg_CheckArityValue("m.p_buffer_str()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        PyErr_Format(PyExc_TypeError, "a bytes-like object is required, not '%.100s'", PyValue_TYPE(PyValue_Borrow(args[0]))->tp_name);
        goto exit;
    }
    if (PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &len);
        if (ptr == NULL) {
            goto exit;
        }
        if (PyBuffer_FillInfo(&a, PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), (void *)ptr, len, 1, PyBUF_SIMPLE) < 0) {
            goto exit;
        }
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &a, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
    }
    return_value = m_p_buffer_str_impl(PyValue_AsKnownObjectBorrow(module), &a);

exit:
    /* Cleanup for a */
    if (a.obj) {
       PyBuffer_Release(&a);
    }

    return return_value;
}

static PyValue
m_p_buffer_str_impl(PyObject *module, Py_buffer *a)
/*[clinic end generated code: output=d70312c020ebf7f3 input=b8a9c925ed941387]*/


/*[clinic input]
m.p_buffer_rw
    a: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_buffer_rw__doc__,
"p_buffer_rw($module, a, /)\n"
"--\n"
"\n");

#define M_P_BUFFER_RW_METHODDEF    \
    {"p_buffer_rw", _PyCFunction_CAST(m_p_buffer_rw), METH_VALUE, m_p_buffer_rw__doc__},

static PyValue
m_p_buffer_rw_impl(PyObject *module, Py_buffer *a);

static PyValue
m_p_buffer_rw(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_buffer a = {NULL, NULL};

    if (!_PyArg_CheckArityValue("m.p_buffer_rw()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("p_buffer_rw", "argument", "read-write bytes-like object", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), &a, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("p_buffer_rw", "argument", "read-write bytes-like object", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    return_value = m_p_buffer_rw_impl(PyValue_AsKnownObjectBorrow(module), &a);

exit:
    /* Cleanup for a */
    if (a.obj) {
       PyBuffer_Release(&a);
    }

    return return_value;
}

static PyValue
m_p_buffer_rw_impl(PyObject *module, Py_buffer *a)
/*[clinic end generated code: output=f392e3f511bb9012 input=4d663d5036cca93e]*/


/*[clinic input]
m.p_int_str
    a: int(accept={str})
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_p_int_str__doc__,
"p_int_str($module, a, /)\n"
"--\n"
"\n");

#define M_P_INT_STR_METHODDEF    \
    {"p_int_str", _PyCFunction_CAST(m_p_int_str), METH_VALUE, m_p_int_str__doc__},

static PyValue
m_p_int_str_impl(PyObject *module, int a);

static PyValue
m_p_int_str(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int a;

    if (!_PyArg_CheckArityValue("m.p_int_str()", nargs, kwnames, 1)) {
        goto exit;
    }
    if (!PyValue_IsObject(PyValue_Borrow(args[0]))) {
        _PyArg_BadArgumentValue("p_int_str", "argument", "a unicode character", PyValue_Borrow(args[0]));
        goto exit;
    }
    if (!PyUnicode_Check(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))) {
        _PyArg_BadArgument("p_int_str", "argument", "a unicode character", PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])));
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]))) != 1) {
        PyErr_Format(PyExc_TypeError,
            "p_int_str(): argument must be a unicode character, "
            "not a string of length %zd",
            PyUnicode_GET_LENGTH(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0]))));
        goto exit;
    }
    a = PyUnicode_READ_CHAR(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])), 0);
    return_value = m_p_int_str_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_p_int_str_impl(PyObject *module, int a)
/*[clinic end generated code: output=5a37c1cc8c567f80 input=51e83a237518dfbc]*/


/*[clinic input]
m.p_varpos_tuple
    a: object
    /
    *args: tuple
[clinic start generated code]*/

PyDoc_STRVAR(m_p_varpos_tuple__doc__,
"p_varpos_tuple($module, a, /, *args)\n"
"--\n"
"\n");

#define M_P_VARPOS_TUPLE_METHODDEF    \
    {"p_varpos_tuple", _PyCFunction_CAST(m_p_varpos_tuple), METH_VALUE, m_p_varpos_tuple__doc__},

static PyValue
m_p_varpos_tuple_impl(PyObject *module, PyValueBorrow a, PyObject *args);

static PyValue
m_p_varpos_tuple(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyObject *__clinic_args = NULL;

    if (!_PyArg_NoKwnamesValue("m.p_varpos_tuple()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("p_varpos_tuple", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    __clinic_args = _PyTuple_FromValueArray(args + 1,
                                          Py_MAX(nargs - 1, 0));
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = m_p_varpos_tuple_impl(PyValue_AsKnownObjectBorrow(module), a, __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

static PyValue
m_p_varpos_tuple_impl(PyObject *module, PyValueBorrow a, PyObject *args)
/*[clinic end generated code: output=a5a093a36d0dd05c input=72ab422ef81e1d41]*/


/*[clinic input]
m.p_varpos_array
    a: object
    /
    *args: array
[clinic start generated code]*/

PyDoc_STRVAR(m_p_varpos_array__doc__,
"p_varpos_array($module, a, /, *args)\n"
"--\n"
"\n");

#define M_P_VARPOS_ARRAY_METHODDEF    \
    {"p_varpos_array", _PyCFunction_CAST(m_p_varpos_array), METH_VALUE, m_p_varpos_array__doc__},

static PyValue
m_p_varpos_array_impl(PyObject *module, PyValueBorrow a, const PyValue *args,
                      Py_ssize_t args_length);

static PyValue
m_p_varpos_array(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    const PyValue *__clinic_args;
    Py_ssize_t args_length;

    if (!_PyArg_NoKwnamesValue("m.p_varpos_array()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("p_varpos_array", nargs, 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    __clinic_args = args + 1;
    args_length = nargs - 1;
    return_value = m_p_varpos_array_impl(PyValue_AsKnownObjectBorrow(module), a, __clinic_args, args_length);

exit:
    return return_value;
}

static PyValue
m_p_varpos_array_impl(PyObject *module, PyValueBorrow a, const PyValue *args,
                      Py_ssize_t args_length)
/*[clinic end generated code: output=ebdd97b497064491 input=d1461250cc862dad]*/


/*[clinic input]
m.p_varkw
    a: object
    b: object
    /
    **kwargs: dict
[clinic start generated code]*/

PyDoc_STRVAR(m_p_varkw__doc__,
"p_varkw($module, a, b, /, **kwargs)\n"
"--\n"
"\n");

#define M_P_VARKW_METHODDEF    \
    {"p_varkw", _PyCFunction_CAST(m_p_varkw), METH_VALUE, m_p_varkw__doc__},

static PyValue
m_p_varkw_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b,
               PyObject *kwargs);

static PyValue
m_p_varkw(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    PyValueBorrow b;
    PyObject *__clinic_kwargs = NULL;

    if (!_PyArg_CheckPositional("p_varkw", nargs, 2, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    b = PyValue_Borrow(args[1]);
    __clinic_kwargs = kwnames == NULL ? PyDict_New()
        : _PyValue_StackAsDict(args + nargs, kwnames);
    if (__clinic_kwargs == NULL) {
        goto exit;
    }
    return_value = m_p_varkw_impl(PyValue_AsKnownObjectBorrow(module), a, b, __clinic_kwargs);

exit:
    /* Cleanup for kwargs */
    Py_XDECREF(__clinic_kwargs);

    return return_value;
}

static PyValue
m_p_varkw_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b,
               PyObject *kwargs)
/*[clinic end generated code: output=a89c9779e348c5b8 input=480a6d4d37c82c03]*/


/*[clinic input]
m.s_noargs
[clinic start generated code]*/

PyDoc_STRVAR(m_s_noargs__doc__,
"s_noargs($module, /)\n"
"--\n"
"\n");

#define M_S_NOARGS_METHODDEF    \
    {"s_noargs", _PyCFunction_CAST(m_s_noargs), METH_VALUE, m_s_noargs__doc__},

static PyValue
m_s_noargs_impl(PyObject *module);

static PyValue
m_s_noargs(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("m.s_noargs()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = m_s_noargs_impl(PyValue_AsKnownObjectBorrow(module));

exit:
    return return_value;
}

static PyValue
m_s_noargs_impl(PyObject *module)
/*[clinic end generated code: output=ad76be7a132cc0a0 input=fc7deed602049763]*/


/*[clinic input]
m.T.s_noargs
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_noargs__doc__,
"s_noargs($self, /)\n"
"--\n"
"\n");

#define M_T_S_NOARGS_METHODDEF    \
    {"s_noargs", _PyCFunction_CAST(m_T_s_noargs), METH_VALUE, m_T_s_noargs__doc__},

static PyValue
m_T_s_noargs_impl(TestObj *self);

static PyValue
m_T_s_noargs(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValue("T.s_noargs()", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = m_T_s_noargs_impl((TestObj *)PyValue_AsKnownObjectBorrow(self));

exit:
    return return_value;
}

static PyValue
m_T_s_noargs_impl(TestObj *self)
/*[clinic end generated code: output=45efb73cf2b166a0 input=74cc8df829870cdd]*/


/*[clinic input]
m.s_one
    a: object
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_s_one__doc__,
"s_one($module, a, /)\n"
"--\n"
"\n");

#define M_S_ONE_METHODDEF    \
    {"s_one", _PyCFunction_CAST(m_s_one), METH_VALUE, m_s_one__doc__},

static PyValue
m_s_one_impl(PyObject *module, PyValueBorrow a);

static PyValue
m_s_one(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("m.s_one()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = m_s_one_impl(PyValue_AsKnownObjectBorrow(module), a);

exit:
    return return_value;
}

static PyValue
m_s_one_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=ae7938d62b5d7e79 input=5747c9b4ff7ea891]*/


/*[clinic input]
m.T.s_one
    a: object
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_one__doc__,
"s_one($self, a, /)\n"
"--\n"
"\n");

#define M_T_S_ONE_METHODDEF    \
    {"s_one", _PyCFunction_CAST(m_T_s_one), METH_VALUE, m_T_s_one__doc__},

static PyValue
m_T_s_one_impl(TestObj *self, PyValueBorrow a);

static PyValue
m_T_s_one(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("T.s_one()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = m_T_s_one_impl((TestObj *)PyValue_AsKnownObjectBorrow(self), a);

exit:
    return return_value;
}

static PyValue
m_T_s_one_impl(TestObj *self, PyValueBorrow a)
/*[clinic end generated code: output=622518f73fd46ce2 input=0f24fa955ffed3c2]*/


/*[clinic input]
m.s_posonly_opt
    a: object
    b: int = 1
    c: int = 2
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_s_posonly_opt__doc__,
"s_posonly_opt($module, a, b=1, c=2, /)\n"
"--\n"
"\n");

#define M_S_POSONLY_OPT_METHODDEF    \
    {"s_posonly_opt", _PyCFunction_CAST(m_s_posonly_opt), METH_VALUE, m_s_posonly_opt__doc__},

static PyValue
m_s_posonly_opt_impl(PyObject *module, PyValueBorrow a, int b, int c);

static PyValue
m_s_posonly_opt(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    int b = 1;
    int c = 2;

    if (!_PyArg_NoKwnamesValue("m.s_posonly_opt()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("s_posonly_opt", nargs, 1, 3)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    b = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    c = _PyValue_AsInt(PyValue_Borrow(args[2]));
    if (c == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = m_s_posonly_opt_impl(PyValue_AsKnownObjectBorrow(module), a, b, c);

exit:
    return return_value;
}

static PyValue
m_s_posonly_opt_impl(PyObject *module, PyValueBorrow a, int b, int c)
/*[clinic end generated code: output=f09c6e96f9efe03b input=fd78c67aac8d15f2]*/


/*[clinic input]
m.s_poskw
    a: object
    b: int = 1
[clinic start generated code]*/

PyDoc_STRVAR(m_s_poskw__doc__,
"s_poskw($module, /, a, b=1)\n"
"--\n"
"\n");

#define M_S_POSKW_METHODDEF    \
    {"s_poskw", _PyCFunction_CAST(m_s_poskw), METH_VALUE, m_s_poskw__doc__},

static PyValue
m_s_poskw_impl(PyObject *module, PyValueBorrow a, int b);

static PyValue
m_s_poskw(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "s_poskw",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    int b = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        b = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (b == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = m_s_poskw_impl(PyValue_AsKnownObjectBorrow(module), a, b);

exit:
    return return_value;
}

static PyValue
m_s_poskw_impl(PyObject *module, PyValueBorrow a, int b)
/*[clinic end generated code: output=2e04b822e139fcf3 input=49d8b606dc56d590]*/


/*[clinic input]
m.s_kwonly
    a: object
    *
    b: int
    c: double = 1.0
[clinic start generated code]*/

PyDoc_STRVAR(m_s_kwonly__doc__,
"s_kwonly($module, /, a, *, b, c=1.0)\n"
"--\n"
"\n");

#define M_S_KWONLY_METHODDEF    \
    {"s_kwonly", _PyCFunction_CAST(m_s_kwonly), METH_VALUE, m_s_kwonly__doc__},

static PyValue
m_s_kwonly_impl(PyObject *module, PyValueBorrow a, int b, double c);

static PyValue
m_s_kwonly(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyValue ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('c')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", "c", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "s_kwonly",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow a;
    int b;
    double c = 1.0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 1, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    b = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (_idx[2] >= 0) {
        c = _PyValue_AsDouble(PyValue_Borrow(args[_idx[2]]));
        if (c == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = m_s_kwonly_impl(PyValue_AsKnownObjectBorrow(module), a, b, c);

exit:
    return return_value;
}

static PyValue
m_s_kwonly_impl(PyObject *module, PyValueBorrow a, int b, double c)
/*[clinic end generated code: output=bc5f7a94f7586182 input=147c74ba84cb07e9]*/


/*[clinic input]
m.T.s_defcls
    cls: defining_class
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_defcls__doc__,
"s_defcls($self, /)\n"
"--\n"
"\n");

#define M_T_S_DEFCLS_METHODDEF    \
    {"s_defcls", _PyCFunction_CAST(m_T_s_defcls), METH_VALUE|METH_METHOD, m_T_s_defcls__doc__},

static PyValue
m_T_s_defcls_impl(TestObj *self, PyTypeObject *cls);

static PyValue
m_T_s_defcls(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "s_defcls() takes no arguments");
        goto exit;
    }
    return_value = m_T_s_defcls_impl((TestObj *)PyValue_AsKnownObjectBorrow(self), cls);

exit:
    return return_value;
}

static PyValue
m_T_s_defcls_impl(TestObj *self, PyTypeObject *cls)
/*[clinic end generated code: output=06953a96fb119032 input=77ef50d9149f350a]*/


/*[clinic input]
m.T.s_defcls_params
    cls: defining_class
    a: object
    b: int = 1
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_defcls_params__doc__,
"s_defcls_params($self, a, b=1, /)\n"
"--\n"
"\n");

#define M_T_S_DEFCLS_PARAMS_METHODDEF    \
    {"s_defcls_params", _PyCFunction_CAST(m_T_s_defcls_params), METH_VALUE|METH_METHOD, m_T_s_defcls_params__doc__},

static PyValue
m_T_s_defcls_params_impl(TestObj *self, PyTypeObject *cls, PyValueBorrow a,
                         int b);

static PyValue
m_T_s_defcls_params(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    int b = 1;

    if (!_PyArg_NoKwnames("s_defcls_params", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("s_defcls_params", nargs, 1, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    b = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = m_T_s_defcls_params_impl((TestObj *)PyValue_AsKnownObjectBorrow(self), cls, a, b);

exit:
    return return_value;
}

static PyValue
m_T_s_defcls_params_impl(TestObj *self, PyTypeObject *cls, PyValueBorrow a,
                         int b)
/*[clinic end generated code: output=8ad931f76dc29e49 input=7389fc7e85e31303]*/


/*[clinic input]
m.T.s_defcls_kw
    cls: defining_class
    a: object
    *
    b: int = 1
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_defcls_kw__doc__,
"s_defcls_kw($self, /, a, *, b=1)\n"
"--\n"
"\n");

#define M_T_S_DEFCLS_KW_METHODDEF    \
    {"s_defcls_kw", _PyCFunction_CAST(m_T_s_defcls_kw), METH_VALUE|METH_METHOD, m_T_s_defcls_kw__doc__},

static PyValue
m_T_s_defcls_kw_impl(TestObj *self, PyTypeObject *cls, PyValueBorrow a,
                     int b);

static PyValue
m_T_s_defcls_kw(PyValueBorrow self, PyTypeObject *cls, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('a')), _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"a", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "s_defcls_kw",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    int b = 1;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        b = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (b == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = m_T_s_defcls_kw_impl((TestObj *)PyValue_AsKnownObjectBorrow(self), cls, a, b);

exit:
    return return_value;
}

static PyValue
m_T_s_defcls_kw_impl(TestObj *self, PyTypeObject *cls, PyValueBorrow a,
                     int b)
/*[clinic end generated code: output=00cbb1ecdfb1b936 input=8b3627fe49809aba]*/


/*[clinic input]
@classmethod
m.T.s_classmethod
    a: object
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_classmethod__doc__,
"s_classmethod($type, a, /)\n"
"--\n"
"\n");

#define M_T_S_CLASSMETHOD_METHODDEF    \
    {"s_classmethod", _PyCFunction_CAST(m_T_s_classmethod), METH_VALUE|METH_CLASS, m_T_s_classmethod__doc__},

static PyValue
m_T_s_classmethod_impl(PyTypeObject *type, PyValueBorrow a);

static PyValue
m_T_s_classmethod(PyValueBorrow type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("T.s_classmethod()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = m_T_s_classmethod_impl((PyTypeObject *)PyValue_AsKnownObjectBorrow(type), a);

exit:
    return return_value;
}

static PyValue
m_T_s_classmethod_impl(PyTypeObject *type, PyValueBorrow a)
/*[clinic end generated code: output=08d28a2929d3984c input=678963802e343cb6]*/


/*[clinic input]
@staticmethod
m.T.s_staticmethod
    a: object
    b: int = 1
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_staticmethod__doc__,
"s_staticmethod(a, b=1, /)\n"
"--\n"
"\n");

#define M_T_S_STATICMETHOD_METHODDEF    \
    {"s_staticmethod", _PyCFunction_CAST(m_T_s_staticmethod), METH_VALUE|METH_STATIC, m_T_s_staticmethod__doc__},

static PyValue
m_T_s_staticmethod_impl(PyValueBorrow a, int b);

static PyValue
m_T_s_staticmethod(PyValueBorrow null, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;
    int b = 1;

    if (!_PyArg_NoKwnamesValue("T.s_staticmethod()", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("s_staticmethod", nargs, 1, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    b = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = m_T_s_staticmethod_impl(a, b);

exit:
    return return_value;
}

static PyValue
m_T_s_staticmethod_impl(PyValueBorrow a, int b)
/*[clinic end generated code: output=072497a39398317e input=1aada2343f8e92c9]*/


/*[clinic input]
m.I.s_method
    a: object
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_I_s_method__doc__,
"s_method($self, a, /)\n"
"--\n"
"\n");

#define M_I_S_METHOD_METHODDEF    \
    {"s_method", _PyCFunction_CAST(m_I_s_method), METH_VALUE, m_I_s_method__doc__},

static PyValue
m_I_s_method_impl(PyValueBorrow self, PyValueBorrow a);

static PyValue
m_I_s_method(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValueMethod(&PyLong_Type, "s_method", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = m_I_s_method_impl(self, a);

exit:
    return return_value;
}

static PyValue
m_I_s_method_impl(PyValueBorrow self, PyValueBorrow a)
/*[clinic end generated code: output=75b489e8c39e7e4c input=ae2d1fc6c27d2088]*/


/*[clinic input]
m.I.s_noargs
[clinic start generated code]*/

PyDoc_STRVAR(m_I_s_noargs__doc__,
"s_noargs($self, /)\n"
"--\n"
"\n");

#define M_I_S_NOARGS_METHODDEF    \
    {"s_noargs", _PyCFunction_CAST(m_I_s_noargs), METH_VALUE, m_I_s_noargs__doc__},

static PyValue
m_I_s_noargs_impl(PyValueBorrow self);

static PyValue
m_I_s_noargs(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;

    if (!_PyArg_CheckArityValueMethod(&PyLong_Type, "s_noargs", nargs, kwnames, 0)) {
        goto exit;
    }
    return_value = m_I_s_noargs_impl(self);

exit:
    return return_value;
}

static PyValue
m_I_s_noargs_impl(PyValueBorrow self)
/*[clinic end generated code: output=7965a4ad87e44b82 input=9be02c32b2b0e415]*/


/*[clinic input]
@critical_section
m.T.s_critical
    a: object
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_critical__doc__,
"s_critical($self, a, /)\n"
"--\n"
"\n");

#define M_T_S_CRITICAL_METHODDEF    \
    {"s_critical", _PyCFunction_CAST(m_T_s_critical), METH_VALUE, m_T_s_critical__doc__},

static PyValue
m_T_s_critical_impl(TestObj *self, PyValueBorrow a);

static PyValue
m_T_s_critical(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("T.s_critical()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(self));
    return_value = m_T_s_critical_impl((TestObj *)PyValue_AsKnownObjectBorrow(self), a);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyValue
m_T_s_critical_impl(TestObj *self, PyValueBorrow a)
/*[clinic end generated code: output=95a53f4ae6f999c4 input=a528b101e7e82724]*/

/*[clinic input]
@critical_section a
m.T.s_critical_target
    a: object
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_critical_target__doc__,
"s_critical_target($self, a, /)\n"
"--\n"
"\n");

#define M_T_S_CRITICAL_TARGET_METHODDEF    \
    {"s_critical_target", _PyCFunction_CAST(m_T_s_critical_target), METH_VALUE, m_T_s_critical_target__doc__},

static PyValue
m_T_s_critical_target_impl(TestObj *self, PyValueBorrow a);

static PyValue
m_T_s_critical_target(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("T.s_critical_target()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION(PyValue_AsKnownObjectBorrow(a));
    return_value = m_T_s_critical_target_impl((TestObj *)PyValue_AsKnownObjectBorrow(self), a);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

static PyValue
m_T_s_critical_target_impl(TestObj *self, PyValueBorrow a)
/*[clinic end generated code: output=4701cefff9868bd9 input=9a6ff0a463de3d69]*/

/*[clinic input]
@critical_section self a
m.T.s_critical_two
    a: object
    /
[clinic start generated code]*/

PyDoc_STRVAR(m_T_s_critical_two__doc__,
"s_critical_two($self, a, /)\n"
"--\n"
"\n");

#define M_T_S_CRITICAL_TWO_METHODDEF    \
    {"s_critical_two", _PyCFunction_CAST(m_T_s_critical_two), METH_VALUE, m_T_s_critical_two__doc__},

static PyValue
m_T_s_critical_two_impl(TestObj *self, PyValueBorrow a);

static PyValue
m_T_s_critical_two(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyValueBorrow a;

    if (!_PyArg_CheckArityValue("T.s_critical_two()", nargs, kwnames, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    Py_BEGIN_CRITICAL_SECTION2(PyValue_AsKnownObjectBorrow(self), PyValue_AsKnownObjectBorrow(a));
    return_value = m_T_s_critical_two_impl((TestObj *)PyValue_AsKnownObjectBorrow(self), a);
    Py_END_CRITICAL_SECTION2();

exit:
    return return_value;
}

static PyValue
m_T_s_critical_two_impl(TestObj *self, PyValueBorrow a)
/*[clinic end generated code: output=477d1106f27e2b25 input=5279915e942e1458]*/


/*[clinic input]
m.s_example
    x: object
    /
    n: int = 1
    *
    scale: double = 1.0
[clinic start generated code]*/

PyDoc_STRVAR(m_s_example__doc__,
"s_example($module, x, /, n=1, *, scale=1.0)\n"
"--\n"
"\n");

#define M_S_EXAMPLE_METHODDEF    \
    {"s_example", _PyCFunction_CAST(m_s_example), METH_VALUE, m_s_example__doc__},

static PyValue
m_s_example_impl(PyObject *module, PyValueBorrow x, int n, double scale);

static PyValue
m_s_example(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('n')), _PyValue_STATIC_IMMORTAL(&_Py_ID(scale)), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "n", "scale", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "s_example",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[3];
    PyValueBorrow x;
    int n = 1;
    double scale = 1.0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 3)) {
        goto exit;
    }
    x = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        n = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (n == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (_idx[2] >= 0) {
        scale = _PyValue_AsDouble(PyValue_Borrow(args[_idx[2]]));
        if (scale == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = m_s_example_impl(PyValue_AsKnownObjectBorrow(module), x, n, scale);

exit:
    return return_value;
}

static PyValue
m_s_example_impl(PyObject *module, PyValueBorrow x, int n, double scale)
/*[clinic end generated code: output=77a8dd4a48eb7a12 input=cf765b68a8f39d2f]*/


/*[clinic input]
m.r_bool -> bool
[clinic start generated code]*/

PyDoc_STRVAR(m_r_bool__doc__,
"r_bool($module, /)\n"
"--\n"
"\n");

#define M_R_BOOL_METHODDEF    \
    {"r_bool", _PyCFunction_CAST(m_r_bool), METH_VALUE, m_r_bool__doc__},

static int
m_r_bool_impl(PyObject *module);

static PyValue
m_r_bool(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("m.r_bool()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = m_r_bool_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _return_value ? PyValue_True : PyValue_False;

exit:
    return return_value;
}

static int
m_r_bool_impl(PyObject *module)
/*[clinic end generated code: output=7b797c474c0c9f79 input=3ffe2ef2ea564c9f]*/


/*[clinic input]
m.r_int -> int
[clinic start generated code]*/

PyDoc_STRVAR(m_r_int__doc__,
"r_int($module, /)\n"
"--\n"
"\n");

#define M_R_INT_METHODDEF    \
    {"r_int", _PyCFunction_CAST(m_r_int), METH_VALUE, m_r_int__doc__},

static int
m_r_int_impl(PyObject *module);

static PyValue
m_r_int(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    int _return_value;

    if (!_PyArg_CheckArityValue("m.r_int()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = m_r_int_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

static int
m_r_int_impl(PyObject *module)
/*[clinic end generated code: output=77a8bc40ac831ea5 input=f6e04136f0c29ef6]*/


/*[clinic input]
m.r_py_ssize_t -> Py_ssize_t
[clinic start generated code]*/

PyDoc_STRVAR(m_r_py_ssize_t__doc__,
"r_py_ssize_t($module, /)\n"
"--\n"
"\n");

#define M_R_PY_SSIZE_T_METHODDEF    \
    {"r_py_ssize_t", _PyCFunction_CAST(m_r_py_ssize_t), METH_VALUE, m_r_py_ssize_t__doc__},

static Py_ssize_t
m_r_py_ssize_t_impl(PyObject *module);

static PyValue
m_r_py_ssize_t(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    Py_ssize_t _return_value;

    if (!_PyArg_CheckArityValue("m.r_py_ssize_t()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = m_r_py_ssize_t_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

static Py_ssize_t
m_r_py_ssize_t_impl(PyObject *module)
/*[clinic end generated code: output=150d6db82581be86 input=853f6b73cb788e76]*/


/*[clinic input]
m.r_double -> double
[clinic start generated code]*/

PyDoc_STRVAR(m_r_double__doc__,
"r_double($module, /)\n"
"--\n"
"\n");

#define M_R_DOUBLE_METHODDEF    \
    {"r_double", _PyCFunction_CAST(m_r_double), METH_VALUE, m_r_double__doc__},

static double
m_r_double_impl(PyObject *module);

static PyValue
m_r_double(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    double _return_value;

    if (!_PyArg_CheckArityValue("m.r_double()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = m_r_double_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble(_return_value);

exit:
    return return_value;
}

static double
m_r_double_impl(PyObject *module)
/*[clinic end generated code: output=40c3302b88557abc input=e64f22ac6e6c71de]*/


/*[clinic input]
m.r_long -> long
[clinic start generated code]*/

PyDoc_STRVAR(m_r_long__doc__,
"r_long($module, /)\n"
"--\n"
"\n");

#define M_R_LONG_METHODDEF    \
    {"r_long", _PyCFunction_CAST(m_r_long), METH_VALUE, m_r_long__doc__},

static long
m_r_long_impl(PyObject *module);

static PyValue
m_r_long(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    long _return_value;

    if (!_PyArg_CheckArityValue("m.r_long()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = m_r_long_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromInt64((int64_t)_return_value);

exit:
    return return_value;
}

static long
m_r_long_impl(PyObject *module)
/*[clinic end generated code: output=856ec36b2b896c32 input=5cd21e679694bc0d]*/


/*[clinic input]
m.r_unsigned_long -> unsigned_long
[clinic start generated code]*/

PyDoc_STRVAR(m_r_unsigned_long__doc__,
"r_unsigned_long($module, /)\n"
"--\n"
"\n");

#define M_R_UNSIGNED_LONG_METHODDEF    \
    {"r_unsigned_long", _PyCFunction_CAST(m_r_unsigned_long), METH_VALUE, m_r_unsigned_long__doc__},

static unsigned long
m_r_unsigned_long_impl(PyObject *module);

static PyValue
m_r_unsigned_long(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    unsigned long _return_value;

    if (!_PyArg_CheckArityValue("m.r_unsigned_long()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = m_r_unsigned_long_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == (unsigned long)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = (uint64_t)_return_value <= INT64_MAX
        ? PyValue_FromInt64((int64_t)_return_value)
        : _PyValue_FromObjectResult(PyLong_FromUnsignedLongLong((unsigned long long)_return_value));

exit:
    return return_value;
}

static unsigned long
m_r_unsigned_long_impl(PyObject *module)
/*[clinic end generated code: output=93711a9786695546 input=dc087f072849e06b]*/


/*[clinic input]
m.r_size_t -> size_t
[clinic start generated code]*/

PyDoc_STRVAR(m_r_size_t__doc__,
"r_size_t($module, /)\n"
"--\n"
"\n");

#define M_R_SIZE_T_METHODDEF    \
    {"r_size_t", _PyCFunction_CAST(m_r_size_t), METH_VALUE, m_r_size_t__doc__},

static size_t
m_r_size_t_impl(PyObject *module);

static PyValue
m_r_size_t(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    size_t _return_value;

    if (!_PyArg_CheckArityValue("m.r_size_t()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = m_r_size_t_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == (size_t)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = (uint64_t)_return_value <= INT64_MAX
        ? PyValue_FromInt64((int64_t)_return_value)
        : _PyValue_FromObjectResult(PyLong_FromUnsignedLongLong((unsigned long long)_return_value));

exit:
    return return_value;
}

static size_t
m_r_size_t_impl(PyObject *module)
/*[clinic end generated code: output=83a02249ffac1150 input=2932238de30af1f7]*/


/*[clinic input]
m.r_float -> float
[clinic start generated code]*/

PyDoc_STRVAR(m_r_float__doc__,
"r_float($module, /)\n"
"--\n"
"\n");

#define M_R_FLOAT_METHODDEF    \
    {"r_float", _PyCFunction_CAST(m_r_float), METH_VALUE, m_r_float__doc__},

static float
m_r_float_impl(PyObject *module);

static PyValue
m_r_float(PyValueBorrow module, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    float _return_value;

    if (!_PyArg_CheckArityValue("m.r_float()", nargs, kwnames, 0)) {
        goto exit;
    }
    _return_value = m_r_float_impl(PyValue_AsKnownObjectBorrow(module));
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyValue_FromDouble((double)_return_value);

exit:
    return return_value;
}

static float
m_r_float_impl(PyObject *module)
/*[clinic end generated code: output=6dff11d928acb18e input=a57dc6b7adb09707]*/


/*[clinic input]
@classmethod
m.T.__new__
    a: object
    /
    *
    b: int = 0
[clinic start generated code]*/

static PyValue
m_T_impl(PyTypeObject *type, PyValueBorrow a, int b);

static PyValue
m_T_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "T",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    int b = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        b = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (b == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = m_T_impl(type, a, b);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
m_T(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(m_T_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = m_T_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyValue
m_T_impl(PyTypeObject *type, PyValueBorrow a, int b)
/*[clinic end generated code: output=8a8c3a9920734d96 input=91d1134cda3ca13d]*/


/*[clinic input]
m.T.__init__
    a: object
    b: int = 0
    /
[clinic start generated code]*/

static int
m_T___init___impl(TestObj *self, PyValueBorrow a, int b);

static int
m_T___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    PyTypeObject *base_tp = TestType;
    PyValueBorrow a;
    int b = 0;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKwnames("T", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("T", nargs, 1, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    b = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = m_T___init___impl((TestObj *)self, a, b);

exit:
    return return_value;
}

static int
m_T___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return m_T___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = m_T___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

static int
m_T___init___impl(TestObj *self, PyValueBorrow a, int b)
/*[clinic end generated code: output=60715d5a38e9cf6f input=e70a76639231afb7]*/


/*[clinic input]
@classmethod
m.I.__new__
    a: object
    /
    b: int = 0
[clinic start generated code]*/

static PyValue
m_I_impl(PyTypeObject *type, PyValueBorrow a, int b);

static PyValue
m_I_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { _PyValue_STATIC_IMMORTAL(_Py_LATIN1_CHR('b')), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "b", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "I",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    Py_ssize_t _idx[2];
    PyValueBorrow a;
    int b = 0;

    if (!_PyArg_UnpackKeywordsValue(args, nargs, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, _idx, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[_idx[0]]);
    if (_idx[1] >= 0) {
        b = _PyValue_AsInt(PyValue_Borrow(args[_idx[1]]));
        if (b == -1 && PyErr_Occurred()) {
            goto exit;
        }
    }
    return_value = m_I_impl(type, a, b);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
m_I(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(m_I_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = m_I_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyValue
m_I_impl(PyTypeObject *type, PyValueBorrow a, int b)
/*[clinic end generated code: output=93745e7e887c863a input=a4f6a99526d5c926]*/


/*[clinic input]
m.I.__init__
    a: object
    b: int = 0
    /
[clinic start generated code]*/

static int
m_I___init___impl(PyValueBorrow self, PyValueBorrow a, int b);

static int
m_I___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    PyTypeObject *base_tp = &PyLong_Type;
    PyValueBorrow a;
    int b = 0;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKwnames("I", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("I", nargs, 1, 2)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    if (nargs < 2) {
        goto skip_optional;
    }
    b = _PyValue_AsInt(PyValue_Borrow(args[1]));
    if (b == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = m_I___init___impl(PyValue_FromObjectBorrow(self), a, b);

exit:
    return return_value;
}

static int
m_I___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return m_I___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = m_I___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

static int
m_I___init___impl(PyValueBorrow self, PyValueBorrow a, int b)
/*[clinic end generated code: output=2155066f9b0f93eb input=15b052fbee30f250]*/


/*[clinic input]
@vectorcall
@classmethod
m.V.__new__
[clinic start generated code]*/

static PyValue
m_V_impl(PyTypeObject *type);

static PyValue
m_V_value(PyTypeObject *type, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyValue return_value = PyValue_NULL;
    PyTypeObject *base_tp = TestType;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositionalStack("V", nargs)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKwnames("V", kwnames)) {
        goto exit;
    }
    return_value = m_V_impl(type);

exit:
    return return_value;
}

Py_VALUE_ENTRY static PyObject *
m_V(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return _PyValue_THUNK_EXIT(m_V_value(type, _PyTuple_ITEMS(args), nargs, NULL), BOX_SITE_CALL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return NULL;
    }
    PyValue result = m_V_value(type, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return _PyValue_THUNK_EXIT(result, BOX_SITE_CALL);
}

static PyValue
m_V_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_new == m_V);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    return m_V_value(_PyType_CAST(type), args, nargs, kwnames);
}

static PyValue
m_V_impl(PyTypeObject *type)
/*[clinic end generated code: output=0b020979cd1a690a input=57eb51daa4458d1b]*/


/*[clinic input]
@vectorcall
m.V.__init__
    a: object
    /
[clinic start generated code]*/

static int
m_V___init___impl(TestObj *self, PyValueBorrow a);

static int
m_V___init___value(PyObject *self, const PyValue *args, Py_ssize_t nargs, PyObject *kwnames)
{
    int return_value = -1;
    PyTypeObject *base_tp = TestType;
    PyValueBorrow a;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKwnames("V", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("V", nargs, 1, 1)) {
        goto exit;
    }
    a = PyValue_Borrow(args[0]);
    return_value = m_V___init___impl((TestObj *)self, a);

exit:
    return return_value;
}

static int
m_V___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    if (kwargs == NULL || PyDict_GET_SIZE(kwargs) == 0) {
        return m_V___init___value(self, _PyTuple_ITEMS(args), nargs, NULL);
    }
    PyObject *kwnames;
    PyValue *window = _PyValue_StackUnpackDict(_PyThreadState_GET(),
        _PyTuple_ITEMS(args), nargs, kwargs, &kwnames);
    if (window == NULL) {
        return -1;
    }
    int result = m_V___init___value(self, window, nargs, kwnames);
    _PyValue_StackUnpackDict_Free(window, nargs, kwnames);
    return result;
}

static PyValue
m_V_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
    size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_init == m_V___init__);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *self = _PyType_CAST(type)->tp_new(_PyType_CAST(type),
        (PyObject *)&_Py_SINGLETON(tuple_empty), NULL);
    if (self == NULL) {
        return PyValue_NULL;
    }
    if (m_V___init___value(self, args, nargs, kwnames) != 0) {
        Py_DECREF(self);
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(self);
}

static int
m_V___init___impl(TestObj *self, PyValueBorrow a)
/*[clinic end generated code: output=6dbf19d86abcf41a input=cf040bb1b8dbe7fa]*/
