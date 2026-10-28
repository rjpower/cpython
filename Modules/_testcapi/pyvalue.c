// The PyValue protocols (Include/cpython/abstract.h).
//
// Each wrapper passes the canonical values of its arguments, so an int that
// fits in int32 arrives inline, as does a float other than NaN.  A
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

// The kind of the canonical value v.
static PyValue
value_kind(PyObject *Py_UNUSED(module), PyValue v)
{
    return PyValue_FromResult(PyUnicode_FromString(kind_of(v)));
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
    static PyValue \
    PREFIX##_##NAME(PyObject *Py_UNUSED(module), PyValue o) \
    { \
        return PyValue_FromResult(value_result(Py##PREFIX##_##NAME##Value(o))); \
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

/* The call forms (rung 3 of 6d).  The callable is taken from the argument
   tuple as its canonical value, so that an inline int or float arrives
   inline; kwargs None is NULL. */
static PyObject *
object_callvalue(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *callargs, *kwargs = Py_None;
    if (!PyArg_ParseTuple(args, "OO!|O", &o, &PyTuple_Type, &callargs,
                          &kwargs)) {
        return NULL;
    }
    NULLABLE(kwargs);
    return value_result(PyObject_CallValue(PyTuple_GET_VALUE(args, 0),
                                           callargs, kwargs));
}

static PyObject *
object_vectorcallvalue(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *callargs;
    if (!PyArg_ParseTuple(args, "OO!", &o, &PyTuple_Type, &callargs)) {
        return NULL;
    }
    PyValue items[8];
    Py_ssize_t n = PyTuple_GET_SIZE(callargs);
    if (n > (Py_ssize_t)Py_ARRAY_LENGTH(items)) {
        PyErr_SetString(PyExc_ValueError, "too many arguments");
        return NULL;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        items[i] = PyTuple_GET_VALUE(callargs, i);
    }
    return value_result(PyObject_VectorcallValue(PyTuple_GET_VALUE(args, 0),
                                                 items, n, NULL));
}

static PyObject *
vectorcall_call(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *callargs, *kwargs = Py_None;
    if (!PyArg_ParseTuple(args, "OO!|O", &o, &PyTuple_Type, &callargs,
                          &kwargs)) {
        return NULL;
    }
    NULLABLE(kwargs);
    return value_result(PyVectorcall_Call(o, callargs, kwargs));
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

// (found, value or None, kind or None); an error propagates.
static PyObject *
object_getoptionalattr(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *name;
    if (!PyArg_ParseTuple(args, "OO", &o, &name)) {
        return NULL;
    }
    PyValue result;
    int rc = PyObject_GetOptionalAttrValue(PyValue_FromObject(o), name,
                                           &result);
    if (rc < 0) {
        assert(PyValue_IsNull(result));
        return NULL;
    }
    if (rc == 0) {
        assert(PyValue_IsNull(result) && !PyErr_Occurred());
        return Py_BuildValue("(iOO)", 0, Py_None, Py_None);
    }
    PyObject *pair = value_result(result);
    if (pair == NULL) {
        return NULL;
    }
    return Py_BuildValue("(iOO)", 1, PyTuple_GET_ITEM(pair, 0),
                         PyTuple_GET_ITEM(pair, 1));
}

// The PyMemberDef of type(o) named name, or NULL with an exception.
static PyMemberDef *
find_member(PyObject *o, const char *name)
{
    for (PyMemberDef *m = Py_TYPE(o)->tp_members; m && m->name; m++) {
        if (strcmp(m->name, name) == 0) {
            return m;
        }
    }
    PyErr_Format(PyExc_LookupError, "no member %s", name);
    return NULL;
}

static PyObject *
member_getonevalue(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o;
    const char *name;
    if (!PyArg_ParseTuple(args, "Os", &o, &name)) {
        return NULL;
    }
    PyMemberDef *m = find_member(o, name);
    if (m == NULL) {
        return NULL;
    }
    return value_result(PyMember_GetOneValue((const char *)o, m));
}

// v None deletes the member.
static PyObject *
member_setonevalue(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *o, *v;
    const char *name;
    if (!PyArg_ParseTuple(args, "OsO", &o, &name, &v)) {
        return NULL;
    }
    PyMemberDef *m = find_member(o, name);
    if (m == NULL) {
        return NULL;
    }
    NULLABLE(v);
    PyValue value = v == NULL ? PyValue_NULL : PyValue_FromObject(v);
    RETURN_INT(PyMember_SetOneValue((char *)o, m, value));
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

static PyValue
object_istrue(PyObject *Py_UNUSED(module), PyValue o)
{
    RETURN_INT_VALUE(PyObject_IsTrueValue(o));
}

static PyValue
object_hash(PyObject *Py_UNUSED(module), PyValue o)
{
    Py_hash_t hash = PyObject_HashValue(o);
    if (hash == -1) {
        assert(PyErr_Occurred());
        return PyValue_NULL;
    }
    return PyValue_FromResult(PyLong_FromSsize_t(hash));
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
static PyValue
iter_next(PyObject *Py_UNUSED(module), PyValue iter)
{
    PyValue item = PyValue_FromInt32(-1);
    int res = PyIter_NextValue(iter, &item);
    if (res < 0) {
        assert(PyErr_Occurred() && PyValue_IsNull(item));
        return PyValue_NULL;
    }
    if (res == 0) {
        assert(!PyErr_Occurred() && PyValue_IsNull(item));
        return PyValue_FromResult(Py_BuildValue("(iO)", 0, Py_None));
    }
    PyObject *pair = value_result(item);
    if (pair == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromResult(Py_BuildValue("(iN)", 1, pair));
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
    static PyValue \
    as_c_##NAME(PyObject *Py_UNUSED(module), PyValue o) \
    { \
        TYPE r = NAME##Value(o); \
        if (r == (TYPE)-1 && PyErr_Occurred()) { \
            return PyValue_NULL; \
        } \
        return PyValue_FromResult(BUILD(r)); \
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


/* Types whose attribute slots return and take values (rung 2 of 6d).

   ValueSlots: tp_getattro returns 12345 for "i" and 2.5 for "f", inline,
   then the item of its dict, then the generic lookup; tp_setattro stores
   the value in its dict as it is.  As a descriptor, __get__ returns 12345
   for an instance and __set__ stores the value under "set" in its own
   dict.  It is defined static and, from the same functions, as a heap
   type.

   BadGetattro and BadDescrGet return a non-canonical box of 5 (the raw
   bits of a box), which the debug check stops; RawKeep's setattro stores
   a value's bits in a PyObject * field, which the tracer check stops. */

typedef struct {
    PyObject_HEAD
    PyObject *dict;
} ValueSlotsObject;

static PyObject *
valueslots_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ValueSlotsObject *self = (ValueSlotsObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->dict = PyDict_New();
    if (self->dict == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    return (PyObject *)self;
}

static int
valueslots_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(op));
    Py_VISIT(((ValueSlotsObject *)op)->dict);
    return 0;
}

static int
valueslots_clear(PyObject *op)
{
    Py_CLEAR(((ValueSlotsObject *)op)->dict);
    return 0;
}

static void
valueslots_dealloc(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    (void)valueslots_clear(op);
    tp->tp_free(op);
    if (tp->tp_flags & Py_TPFLAGS_HEAPTYPE) {
        Py_DECREF(tp);
    }
}

static PyValue
valueslots_getattro(PyObject *op, PyObject *name)
{
    if (PyUnicode_EqualToUTF8(name, "i")) {
        return PyValue_FromInt32(12345);
    }
    if (PyUnicode_EqualToUTF8(name, "f")) {
        return PyValue_FromDouble(2.5);
    }
    PyValue v;
    if (PyDict_GetItemRefValue(((ValueSlotsObject *)op)->dict, name, &v)) {
        return v;
    }
    return PyObject_GenericGetAttr(op, name);
}

static int
valueslots_setattro(PyObject *op, PyObject *name, PyValue value)
{
    PyValue dict = PyValue_FromObject(((ValueSlotsObject *)op)->dict);
    if (PyValue_IsNull(value)) {
        return PyObject_DelItemValue(dict, PyValue_FromObject(name));
    }
    return PyObject_SetItemValue(dict, PyValue_FromObject(name), value);
}

static PyValue
valueslots_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    if (obj == NULL) {
        return PyValue_FromObject(self);
    }
    return PyValue_FromInt32(12345);
}

static int
valueslots_descr_set(PyObject *self, PyObject *obj, PyValue value)
{
    PyObject *key = PyUnicode_FromString("set");
    if (key == NULL) {
        return -1;
    }
    int rc = valueslots_setattro(self, key, value);
    Py_DECREF(key);
    return rc;
}

static PyTypeObject ValueSlots_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.pyvalue.ValueSlots",
    .tp_basicsize = sizeof(ValueSlotsObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = valueslots_new,
    .tp_dealloc = valueslots_dealloc,
    .tp_traverse = valueslots_traverse,
    .tp_clear = valueslots_clear,
    .tp_getattro = valueslots_getattro,
    .tp_setattro = valueslots_setattro,
    .tp_descr_get = valueslots_descr_get,
    .tp_descr_set = valueslots_descr_set,
};

static PyType_Slot valueslots_heap_slots[] = {
    {Py_tp_new, valueslots_new},
    {Py_tp_dealloc, valueslots_dealloc},
    {Py_tp_traverse, valueslots_traverse},
    {Py_tp_clear, valueslots_clear},
    {Py_tp_getattro, valueslots_getattro},
    {Py_tp_setattro, valueslots_setattro},
    {Py_tp_descr_get, valueslots_descr_get},
    {Py_tp_descr_set, valueslots_descr_set},
    {0, NULL},
};

static PyType_Spec valueslots_heap_spec = {
    .name = "_testcapi.pyvalue.HeapValueSlots",
    .basicsize = sizeof(ValueSlotsObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = valueslots_heap_slots,
};

/* A spec type naming the generic functions. */
static PyType_Slot generic_heap_slots[] = {
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_setattro, PyObject_GenericSetAttr},
    {0, NULL},
};

static PyType_Spec generic_heap_spec = {
    .name = "_testcapi.pyvalue.HeapGeneric",
    .basicsize = sizeof(PyObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = generic_heap_slots,
};

// The raw bits of a box of 5: a heap encoding that is not canonical.
static PyValue
bad_box(void)
{
    PyObject *box = PyValue_Object(PyValue_FromInt32(5));
    if (box == NULL) {
        return PyValue_NULL;
    }
    return _PyValue_FromBits((uintptr_t)box);
}

static PyValue
badgetattro_getattro(PyObject *op, PyObject *name)
{
    if (PyUnicode_EqualToUTF8(name, "x")) {
        return bad_box();
    }
    return PyObject_GenericGetAttr(op, name);
}

static PyTypeObject BadGetattro_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.pyvalue.BadGetattro",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_getattro = badgetattro_getattro,
};

static PyValue
baddescrget_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    return bad_box();
}

/* A tp_call in the old form, returning a PyObject *, through an unchecked
   PyType_Slot entry: its box of 5 arrives as a value whose encoding is not
   canonical. */
static PyObject *
oldform_call(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return PyLong_FromLong(5);
}

static PyType_Slot oldform_call_slots[] = {
    {Py_tp_call, oldform_call},
    {0, 0},
};

static PyType_Spec oldform_call_spec = {
    .name = "_testcapi.pyvalue.OldFormCall",
    .basicsize = sizeof(PyObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = oldform_call_slots,
};

static PyTypeObject BadDescrGet_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.pyvalue.BadDescrGet",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_descr_get = baddescrget_descr_get,
};

typedef struct {
    PyObject_HEAD
    PyObject *raw;
} RawKeepObject;

static int
rawkeep_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_VISIT(((RawKeepObject *)op)->raw);
    return 0;
}

static int
rawkeep_setattro(PyObject *op, PyObject *name, PyValue value)
{
    // Wrong on purpose: an inline value's bits are not a PyObject *.
    ((RawKeepObject *)op)->raw = (PyObject *)(uintptr_t)value.bits;
    return 0;
}

static PyTypeObject RawKeep_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.pyvalue.RawKeep",
    .tp_basicsize = sizeof(RawKeepObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = PyType_GenericNew,
    .tp_traverse = rawkeep_traverse,
    .tp_setattro = rawkeep_setattro,
};

static PyMethodDef test_methods[] = {
    {"AsLongValue", _PyCFunction_CAST(as_c_PyLong_AsLong), METH_O},
    {"AsIntValue", _PyCFunction_CAST(as_c_PyLong_AsInt), METH_O},
    {"AsLongLongValue", _PyCFunction_CAST(as_c_PyLong_AsLongLong), METH_O},
    {"AsSsize_tValue", _PyCFunction_CAST(as_c_PyLong_AsSsize_t), METH_O},
    {"NumberAsSsize_tValue", number_as_ssize_t, METH_VARARGS},
    {"AsDoubleValue", _PyCFunction_CAST(as_c_PyFloat_AsDouble), METH_O},
    {"value_kind", _PyCFunction_CAST(value_kind), METH_O},
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
    {"NegativeValue", _PyCFunction_CAST(Number_Negative), METH_O},
    {"PositiveValue", _PyCFunction_CAST(Number_Positive), METH_O},
    {"InvertValue", _PyCFunction_CAST(Number_Invert), METH_O},
    {"AbsoluteValue", _PyCFunction_CAST(Number_Absolute), METH_O},
    {"IndexValue", _PyCFunction_CAST(Number_Index), METH_O},
    {"LongValue", _PyCFunction_CAST(Number_Long), METH_O},
    {"FloatValue", _PyCFunction_CAST(Number_Float), METH_O},
    {"GetAttrValue", object_getattr, METH_VARARGS},
    {"CallValue", object_callvalue, METH_VARARGS},
    {"VectorcallValue", object_vectorcallvalue, METH_VARARGS},
    {"VectorcallCall", vectorcall_call, METH_VARARGS},
    {"SetAttrValue", object_setattr, METH_VARARGS},
    {"GetOptionalAttrValue", object_getoptionalattr, METH_VARARGS},
    {"MemberGetOneValue", member_getonevalue, METH_VARARGS},
    {"MemberSetOneValue", member_setonevalue, METH_VARARGS},
    {"RichCompareValue", object_richcompare, METH_VARARGS},
    {"RichCompareBoolValue", object_richcomparebool, METH_VARARGS},
    {"IsTrueValue", _PyCFunction_CAST(object_istrue), METH_O},
    {"HashValue", _PyCFunction_CAST(object_hash), METH_O},
    {"GetItemValue", object_getitem, METH_VARARGS},
    {"SetItemValue", object_setitem, METH_VARARGS},
    {"DelItemValue", object_delitem, METH_VARARGS},
    {"ContainsValue", sequence_contains, METH_VARARGS},
    {"GetIterValue", _PyCFunction_CAST(Object_GetIter), METH_O},
    {"NextValue", _PyCFunction_CAST(iter_next), METH_O},
    {"StrValue", _PyCFunction_CAST(Object_Str), METH_O},
    {"ReprValue", _PyCFunction_CAST(Object_Repr), METH_O},
    {"ASCIIValue", _PyCFunction_CAST(Object_ASCII), METH_O},
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
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        Py_DECREF(mod);
        return -1;
    }
    PyTypeObject *types[] = {&ValueSlots_Type, &BadGetattro_Type,
                             &BadDescrGet_Type, &RawKeep_Type};
    for (size_t i = 0; i < Py_ARRAY_LENGTH(types); i++) {
        if (PyModule_AddType(mod, types[i]) < 0) {
            Py_DECREF(mod);
            return -1;
        }
    }
    PyType_Spec *specs[] = {&valueslots_heap_spec, &generic_heap_spec,
                            &oldform_call_spec};
    for (size_t i = 0; i < Py_ARRAY_LENGTH(specs); i++) {
        PyObject *tp = PyType_FromSpec(specs[i]);
        if (tp == NULL || PyModule_AddType(mod, (PyTypeObject *)tp) < 0) {
            Py_XDECREF(tp);
            Py_DECREF(mod);
            return -1;
        }
        Py_DECREF(tp);
    }
    return PyModule_Add(m, "pyvalue", mod);
}
