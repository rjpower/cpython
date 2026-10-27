// The PyValue protocols (Include/cpython/abstract.h).
//
// Each wrapper takes objects and passes their canonical values, so an int
// that fits in int32 arrives inline, as does a float other than NaN.  A
// value result comes back as (object, kind), where kind is "int" or "float"
// for an inline value and "object" otherwise; a result that is not
// canonical raises AssertionError.

#include "parts.h"
#include "util.h"


static const char *
kind_of(PyValue v)
{
    if (PyValue_IsInlineInt(v)) {
        return "int";
    }
    if (PyValue_IsInlineFloat(v)) {
        return "float";
    }
    return "object";
}

static PyObject *
value_result(PyValue v)
{
    if (PyValue_IsNull(v)) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());
    PyObject *op = PyValue_Object(v);
    if (op == NULL) {
        return NULL;
    }
    if (!PyValue_Is(PyValue_FromObject(op), v)) {
        PyErr_Format(PyExc_AssertionError, "result %R is not canonical", op);
        return NULL;
    }
    return Py_BuildValue("(Os)", op, kind_of(v));
}

// The kind of the canonical value of obj.
static PyObject *
value_kind(PyObject *Py_UNUSED(module), PyObject *obj)
{
    return PyUnicode_FromString(kind_of(PyValue_FromObject(obj)));
}

#define BINARY(NAME) \
    static PyObject * \
    number_##NAME(PyObject *Py_UNUSED(module), PyObject *args) \
    { \
        PyObject *o1, *o2; \
        if (!PyArg_ParseTuple(args, "OO", &o1, &o2)) { \
            return NULL; \
        } \
        return value_result(PyNumber_##NAME##Value(PyValue_FromObject(o1), \
                                                  PyValue_FromObject(o2))); \
    }
BINARY(Add)
BINARY(Subtract)
BINARY(Multiply)
BINARY(MatrixMultiply)
BINARY(FloorDivide)
BINARY(TrueDivide)
BINARY(Remainder)
BINARY(Lshift)
BINARY(Rshift)
BINARY(And)
BINARY(Or)
BINARY(Xor)
BINARY(InPlaceAdd)
BINARY(InPlaceSubtract)
BINARY(InPlaceMultiply)
BINARY(InPlaceMatrixMultiply)
BINARY(InPlaceFloorDivide)
BINARY(InPlaceTrueDivide)
BINARY(InPlaceRemainder)
BINARY(InPlaceLshift)
BINARY(InPlaceRshift)
BINARY(InPlaceAnd)
BINARY(InPlaceOr)
BINARY(InPlaceXor)
#undef BINARY

#define TERNARY(NAME) \
    static PyObject * \
    number_##NAME(PyObject *Py_UNUSED(module), PyObject *args) \
    { \
        PyObject *o1, *o2, *o3 = Py_None; \
        if (!PyArg_ParseTuple(args, "OO|O", &o1, &o2, &o3)) { \
            return NULL; \
        } \
        return value_result(PyNumber_##NAME##Value(PyValue_FromObject(o1), \
                                                  PyValue_FromObject(o2), \
                                                  PyValue_FromObject(o3))); \
    }
TERNARY(Power)
TERNARY(InPlacePower)
#undef TERNARY

#define UNARY(PREFIX, NAME) \
    static PyObject * \
    PREFIX##_##NAME(PyObject *Py_UNUSED(module), PyObject *o) \
    { \
        return value_result(Py##PREFIX##_##NAME##Value(PyValue_FromObject(o))); \
    }
UNARY(Number, Negative)
UNARY(Number, Positive)
UNARY(Number, Invert)
UNARY(Number, Absolute)
UNARY(Number, Index)
UNARY(Number, Long)
UNARY(Number, Float)
UNARY(Object, Str)
UNARY(Object, Repr)
UNARY(Object, ASCII)
UNARY(Object, GetIter)
#undef UNARY

static PyObject *
object_getattr(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *name;
    if (!PyArg_ParseTuple(args, "OO", &o, &name)) {
        return NULL;
    }
    return value_result(PyObject_GetAttrValue(PyValue_FromObject(o), name));
}

// v None deletes the attribute.
static PyObject *
object_setattr(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *name, *v;
    if (!PyArg_ParseTuple(args, "OOO", &o, &name, &v)) {
        return NULL;
    }
    NULLABLE(v);
    PyValue value = v == NULL ? PyValue_NULL : PyValue_FromObject(v);
    RETURN_INT(PyObject_SetAttrValue(PyValue_FromObject(o), name, value));
}

static PyObject *
object_richcompare(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;
    int op;
    if (!PyArg_ParseTuple(args, "OOi", &o1, &o2, &op)) {
        return NULL;
    }
    return value_result(PyObject_RichCompareValue(PyValue_FromObject(o1),
                                                  PyValue_FromObject(o2), op));
}

static PyObject *
object_richcomparebool(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o1, *o2;
    int op;
    if (!PyArg_ParseTuple(args, "OOi", &o1, &o2, &op)) {
        return NULL;
    }
    RETURN_INT(PyObject_RichCompareBoolValue(PyValue_FromObject(o1),
                                             PyValue_FromObject(o2), op));
}

static PyObject *
object_istrue(PyObject *Py_UNUSED(module), PyObject *o)
{
    RETURN_INT(PyObject_IsTrueValue(PyValue_FromObject(o)));
}

static PyObject *
object_hash(PyObject *Py_UNUSED(module), PyObject *o)
{
    Py_hash_t hash = PyObject_HashValue(PyValue_FromObject(o));
    if (hash == -1) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return PyLong_FromSsize_t(hash);
}

static PyObject *
object_getitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *key;
    if (!PyArg_ParseTuple(args, "OO", &o, &key)) {
        return NULL;
    }
    return value_result(PyObject_GetItemValue(PyValue_FromObject(o),
                                              PyValue_FromObject(key)));
}

static PyObject *
object_setitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *key, *v;
    if (!PyArg_ParseTuple(args, "OOO", &o, &key, &v)) {
        return NULL;
    }
    RETURN_INT(PyObject_SetItemValue(PyValue_FromObject(o),
                                     PyValue_FromObject(key),
                                     PyValue_FromObject(v)));
}

static PyObject *
object_delitem(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *key;
    if (!PyArg_ParseTuple(args, "OO", &o, &key)) {
        return NULL;
    }
    RETURN_INT(PyObject_DelItemValue(PyValue_FromObject(o),
                                     PyValue_FromObject(key)));
}

static PyObject *
sequence_contains(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *seq, *v;
    if (!PyArg_ParseTuple(args, "OO", &seq, &v)) {
        return NULL;
    }
    RETURN_INT(PySequence_ContainsValue(PyValue_FromObject(seq),
                                        PyValue_FromObject(v)));
}

// (1, (item, kind)), (0, None) when exhausted, or an exception.
static PyObject *
iter_next(PyObject *Py_UNUSED(module), PyObject *iter)
{
    PyValue item = PyValue_FromInt32(-1);
    int res = PyIter_NextValue(PyValue_FromObject(iter), &item);
    if (res < 0) {
        assert(PyErr_Occurred() && PyValue_IsNull(item));
        return NULL;
    }
    if (res == 0) {
        assert(!PyErr_Occurred() && PyValue_IsNull(item));
        return Py_BuildValue("(iO)", 0, Py_None);
    }
    PyObject *pair = value_result(item);
    if (pair == NULL) {
        return NULL;
    }
    return Py_BuildValue("(iN)", 1, pair);
}

static PyObject *
object_format(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *spec = NULL;
    if (!PyArg_ParseTuple(args, "O|O", &o, &spec)) {
        return NULL;
    }
    NULLABLE(spec);
    return value_result(PyObject_FormatValue(PyValue_FromObject(o), spec));
}

#define BINARY_ENTRY(NAME) {#NAME "Value", number_##NAME, METH_VARARGS}

#define AS_C(NAME, TYPE, BUILD) \
    static PyObject * \
    as_c_##NAME(PyObject *Py_UNUSED(module), PyObject *o) \
    { \
        TYPE r = NAME##Value(PyValue_FromObject(o)); \
        if (r == (TYPE)-1 && PyErr_Occurred()) { \
            return NULL; \
        } \
        return BUILD(r); \
    }
AS_C(PyLong_AsLong, long, PyLong_FromLong)
AS_C(PyLong_AsInt, int, PyLong_FromLong)
AS_C(PyLong_AsLongLong, long long, PyLong_FromLongLong)
AS_C(PyLong_AsSsize_t, Py_ssize_t, PyLong_FromSsize_t)
AS_C(PyFloat_AsDouble, double, PyFloat_FromDouble)
#undef AS_C

// AsSsize_tValue(o, exc): exc is None for NULL, clipping on overflow.
static PyObject *
number_as_ssize_t(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *exc;
    if (!PyArg_ParseTuple(args, "OO", &o, &exc)) {
        return NULL;
    }
    Py_ssize_t r = PyNumber_AsSsize_tValue(PyValue_FromObject(o),
                                           Py_IsNone(exc) ? NULL : exc);
    if (r == -1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromSsize_t(r);
}

static PyMethodDef test_methods[] = {
    {"AsLongValue", as_c_PyLong_AsLong, METH_O},
    {"AsIntValue", as_c_PyLong_AsInt, METH_O},
    {"AsLongLongValue", as_c_PyLong_AsLongLong, METH_O},
    {"AsSsize_tValue", as_c_PyLong_AsSsize_t, METH_O},
    {"NumberAsSsize_tValue", number_as_ssize_t, METH_VARARGS},
    {"AsDoubleValue", as_c_PyFloat_AsDouble, METH_O},
    {"value_kind", value_kind, METH_O},
    BINARY_ENTRY(Add),
    BINARY_ENTRY(Subtract),
    BINARY_ENTRY(Multiply),
    BINARY_ENTRY(MatrixMultiply),
    BINARY_ENTRY(FloorDivide),
    BINARY_ENTRY(TrueDivide),
    BINARY_ENTRY(Remainder),
    BINARY_ENTRY(Power),
    BINARY_ENTRY(Lshift),
    BINARY_ENTRY(Rshift),
    BINARY_ENTRY(And),
    BINARY_ENTRY(Or),
    BINARY_ENTRY(Xor),
    BINARY_ENTRY(InPlaceAdd),
    BINARY_ENTRY(InPlaceSubtract),
    BINARY_ENTRY(InPlaceMultiply),
    BINARY_ENTRY(InPlaceMatrixMultiply),
    BINARY_ENTRY(InPlaceFloorDivide),
    BINARY_ENTRY(InPlaceTrueDivide),
    BINARY_ENTRY(InPlaceRemainder),
    BINARY_ENTRY(InPlacePower),
    BINARY_ENTRY(InPlaceLshift),
    BINARY_ENTRY(InPlaceRshift),
    BINARY_ENTRY(InPlaceAnd),
    BINARY_ENTRY(InPlaceOr),
    BINARY_ENTRY(InPlaceXor),
    {"NegativeValue", Number_Negative, METH_O},
    {"PositiveValue", Number_Positive, METH_O},
    {"InvertValue", Number_Invert, METH_O},
    {"AbsoluteValue", Number_Absolute, METH_O},
    {"IndexValue", Number_Index, METH_O},
    {"LongValue", Number_Long, METH_O},
    {"FloatValue", Number_Float, METH_O},
    {"GetAttrValue", object_getattr, METH_VARARGS},
    {"SetAttrValue", object_setattr, METH_VARARGS},
    {"RichCompareValue", object_richcompare, METH_VARARGS},
    {"RichCompareBoolValue", object_richcomparebool, METH_VARARGS},
    {"IsTrueValue", object_istrue, METH_O},
    {"HashValue", object_hash, METH_O},
    {"GetItemValue", object_getitem, METH_VARARGS},
    {"SetItemValue", object_setitem, METH_VARARGS},
    {"DelItemValue", object_delitem, METH_VARARGS},
    {"ContainsValue", sequence_contains, METH_VARARGS},
    {"GetIterValue", Object_GetIter, METH_O},
    {"NextValue", iter_next, METH_O},
    {"StrValue", Object_Str, METH_O},
    {"ReprValue", Object_Repr, METH_O},
    {"ASCIIValue", Object_ASCII, METH_O},
    {"FormatValue", object_format, METH_VARARGS},
    {NULL},
};

#undef BINARY_ENTRY

int
_PyTestCapi_Init_PyValue(PyObject *m)
{
    PyObject *mod = PyModule_New("_testcapi.pyvalue");
    if (mod == NULL) {
        return -1;
    }
    if (PyModule_AddFunctions(mod, test_methods) < 0
        || PyModule_Add(m, "pyvalue", mod) < 0)
    {
        return -1;
    }
    return 0;
}
