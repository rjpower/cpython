/* Test helpers for value slots in PyTypeObject (V2a-0,
 * pyjit/design/proposals/value-api-v2a0.md, sections 4.2-4.4 and 4.8).
 * The tests are in Lib/test/test_value_slots.py.
 *
 * VInt is a core-style type with both sides of each slot pair. Its static
 * subtypes are written as an extension would write them: object slots only.
 * VIntSpec is a heap type that sets both sides through spec slot ids. */

#include "parts.h"

#include "pycore_value.h"         // PyValue
#include "pycore_value_ops.h"     // _PyValue_GenericHash()
#include "pycore_value_slots.h"   // _Py_VALUE_SLOT_TABLE
#include "pycore_slots.h"         // _Py_value_nb_add

#include <stddef.h>               // offsetof()
#include <stdint.h>               // uintptr_t


/* The call log */

static PyObject *call_log = NULL;

static PyObject *
set_value_slots_log(PyObject *Py_UNUSED(module), PyObject *log)
{
    if (log != Py_None && !PyList_Check(log)) {
        PyErr_SetString(PyExc_TypeError, "expected a list or None");
        return NULL;
    }
    PyObject *old = call_log;
    call_log = log == Py_None ? NULL : Py_NewRef(log);
    Py_XDECREF(old);
    Py_RETURN_NONE;
}

static int
log_call(const char *what)
{
    if (call_log == NULL) {
        return 0;
    }
    PyObject *text = PyUnicode_FromString(what);
    if (text == NULL) {
        return -1;
    }
    int res = PyList_Append(call_log, text);
    Py_DECREF(text);
    return res;
}


/* VInt */

typedef struct {
    PyObject_HEAD
    long long value;
} VIntObject;

static PyTypeObject VInt_Type;

static PyObject *
vint_make(long long value)
{
    VIntObject *self = PyObject_New(VIntObject, &VInt_Type);
    if (self == NULL) {
        return NULL;
    }
    self->value = value;
    return (PyObject *)self;
}

static PyObject *
vint_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    long long value;
    if (kwds != NULL && PyDict_GET_SIZE(kwds) != 0) {
        PyErr_SetString(PyExc_TypeError, "VInt takes no keyword arguments");
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "L:VInt", &value)) {
        return NULL;
    }
    VIntObject *self = (VIntObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->value = value;
    return (PyObject *)self;
}

/* The value of an operand: 1 and *out for a VInt instance (of any subclass,
 * *is_vint set), an inline int or an int object that fits in long long; 0
 * for anything else; -1 with an exception set. */
static int
vint_operand(PyValueBorrow v, long long *out, int *is_vint)
{
    *is_vint = 0;
    if (PyValue_IsInlineInt(v)) {
        *out = PyValue_InlineInt64(v);
        return 1;
    }
    if (!PyValue_IsObject(v)) {
        return 0;
    }
    PyObject *obj = PyValue_AsKnownObjectBorrow(v);
    if (PyObject_TypeCheck(obj, &VInt_Type)) {
        *out = ((VIntObject *)obj)->value;
        *is_vint = 1;
        return 1;
    }
    if (PyLong_Check(obj)) {
        int overflow;
        long long n = PyLong_AsLongLongAndOverflow(obj, &overflow);
        if (n == -1 && PyErr_Occurred()) {
            return -1;
        }
        if (overflow) {
            return 0;
        }
        *out = n;
        return 1;
    }
    return 0;
}

/* A new reference: a VInt of a (op) b + extra if both operands are handled
 * and at least one is a VInt, else NotImplemented. op is '+' or '*'. */
static PyObject *
vint_arith(PyValueBorrow a, PyValueBorrow b, char op, long long extra)
{
    long long x, y;
    int a_vint, b_vint;
    int res = vint_operand(a, &x, &a_vint);
    if (res < 0) {
        return NULL;
    }
    if (res == 0) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    res = vint_operand(b, &y, &b_vint);
    if (res < 0) {
        return NULL;
    }
    if (res == 0 || !(a_vint || b_vint)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    return vint_make((op == '+' ? x + y : x * y) + extra);
}

static PyValue
vint_add_value(PyValueBorrow a, PyValueBorrow b)
{
    if (log_call("VInt.add") < 0) {
        return PyValue_NULL;
    }
    return _PyValue_FromObjectResult(vint_arith(a, b, '+', 0));
}

static PyValue
vint_multiply_value(PyValueBorrow a, PyValueBorrow b)
{
    if (log_call("VInt.multiply") < 0) {
        return PyValue_NULL;
    }
    return _PyValue_FromObjectResult(vint_arith(a, b, '*', 0));
}

static PyValue
vint_richcompare_value(PyValueBorrow a, PyValueBorrow b, int op)
{
    if (log_call("VInt.richcompare") < 0) {
        return PyValue_NULL;
    }
    long long x, y;
    int a_vint, b_vint;
    int res = vint_operand(a, &x, &a_vint);
    if (res == 1) {
        res = vint_operand(b, &y, &b_vint);
    }
    if (res < 0) {
        return PyValue_NULL;
    }
    if (res == 0) {
        return PyValue_NotImplemented;
    }
    int truth;
    switch (op) {
    case Py_LT: truth = x < y; break;
    case Py_LE: truth = x <= y; break;
    case Py_EQ: truth = x == y; break;
    case Py_NE: truth = x != y; break;
    case Py_GT: truth = x > y; break;
    case Py_GE: truth = x >= y; break;
    default:
        PyErr_BadInternalCall();
        return PyValue_NULL;
    }
    return truth ? PyValue_True : PyValue_False;
}

static Py_hash_t
vint_hash_value(PyValueBorrow self)
{
    if (log_call("VInt.hash") < 0) {
        return -1;
    }
    Py_hash_t h = (Py_hash_t)((VIntObject *)PyValue_AsKnownObjectBorrow(self))->value;
    return h == -1 ? -2 : h;
}

static PyObject *
vint_repr_value(PyValueBorrow self)
{
    if (log_call("VInt.repr") < 0) {
        return NULL;
    }
    long long value = ((VIntObject *)PyValue_AsKnownObjectBorrow(self))->value;
    return PyUnicode_FromFormat("VInt(%lld)", value);
}

static PyValue
vint_negative_value(PyValueBorrow self)
{
    if (log_call("VInt.negative") < 0) {
        return PyValue_NULL;
    }
    long long value = ((VIntObject *)PyValue_AsKnownObjectBorrow(self))->value;
    return _PyValue_FromObjectResult(vint_make(-value));
}

/* VInt(x ** y) or VInt(x ** y % m) for operands that vint_operand()
 * handles, at least one VInt among x and y, y >= 0 and m > 0; else
 * NotImplemented. */
static PyValue
vint_power_value(PyValueBorrow a, PyValueBorrow b, PyValueBorrow c)
{
    if (log_call("VInt.power") < 0) {
        return PyValue_NULL;
    }
    long long x, y, m = 0;
    int a_vint, b_vint, c_vint;
    int res = vint_operand(a, &x, &a_vint);
    if (res == 1) {
        res = vint_operand(b, &y, &b_vint);
    }
    if (res < 0) {
        return PyValue_NULL;
    }
    if (res == 0 || !(a_vint || b_vint) || y < 0) {
        return PyValue_NotImplemented;
    }
    int has_modulus = !(PyValue_IsObject(c)
                        && PyValue_AsKnownObjectBorrow(c) == Py_None);
    if (has_modulus) {
        res = vint_operand(c, &m, &c_vint);
        if (res < 0) {
            return PyValue_NULL;
        }
        if (res == 0 || m <= 0) {
            return PyValue_NotImplemented;
        }
    }
    long long result = 1;
    for (long long i = 0; i < y; i++) {
        result = has_modulus ? (result * x) % m : result * x;
    }
    return _PyValue_FromObjectResult(
        vint_make(has_modulus ? result % m : result));
}

static int
vint_bool_value(PyValueBorrow self)
{
    if (log_call("VInt.bool") < 0) {
        return -1;
    }
    return ((VIntObject *)PyValue_AsKnownObjectBorrow(self))->value != 0;
}

_Py_VALUE_THUNK_BINARY(vint_add, vint_add_value)
_Py_VALUE_THUNK_BINARY(vint_multiply, vint_multiply_value)
_Py_VALUE_THUNK_RICHCMP(vint_richcompare, vint_richcompare_value)
_Py_VALUE_THUNK_HASH(vint_hash, vint_hash_value)
_Py_VALUE_THUNK_REPR(vint_repr, vint_repr_value)
_Py_VALUE_THUNK_UNARY(vint_negative, vint_negative_value)
_Py_VALUE_THUNK_TERNARY(vint_power, vint_power_value)
_Py_VALUE_THUNK_INQUIRY(vint_bool, vint_bool_value)

static PyNumberMethods vint_as_number = {
    .nb_add = vint_add,
    .nb_multiply = vint_multiply,
    .nb_negative = vint_negative,
    .nb_power = vint_power,
    .nb_bool = vint_bool,
};

static PyMemberDef vint_members[] = {
    {"value", Py_T_LONGLONG, offsetof(VIntObject, value), Py_READONLY},
    {NULL},
};

static PyTypeObject VInt_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.VInt",
    .tp_basicsize = sizeof(VIntObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = vint_new,
    .tp_members = vint_members,
    .tp_as_number = &vint_as_number,
    .tp_richcompare = vint_richcompare,
    .tp_hash = vint_hash,
    .tp_repr = vint_repr,
    .tp_value = {
        .nb_add = _Py_VALUE_SLOT_INIT(nb_add, vint_add_value),
        .nb_multiply = _Py_VALUE_SLOT_INIT(nb_multiply, vint_multiply_value),
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, vint_richcompare_value),
        .hash = _Py_VALUE_SLOT_INIT(hash, vint_hash_value),
        .repr = _Py_VALUE_SLOT_INIT(repr, vint_repr_value),
        .nb_negative = _Py_VALUE_SLOT_INIT(nb_negative, vint_negative_value),
        .nb_power = _Py_VALUE_SLOT_INIT(nb_power, vint_power_value),
        .nb_bool = _Py_VALUE_SLOT_INIT(nb_bool, vint_bool_value),
    },
};


/* Static subtypes, object slots only */

static PyTypeObject VIntSubNothing_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.VIntSubNothing",
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_base = &VInt_Type,
};

static PyObject *
vintsubrepr_repr(PyObject *self)
{
    if (log_call("VIntSubRepr.repr") < 0) {
        return NULL;
    }
    return PyUnicode_FromFormat("VIntSubRepr(%lld)", ((VIntObject *)self)->value);
}

static PyTypeObject VIntSubRepr_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.VIntSubRepr",
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_base = &VInt_Type,
    .tp_repr = vintsubrepr_repr,
};

static PyObject *
vintsubadd_add(PyObject *a, PyObject *b)
{
    if (log_call("VIntSubAdd.add") < 0) {
        return NULL;
    }
    return vint_arith(_Py_VALUE_ARG(a), _Py_VALUE_ARG(b), '+', 1000);
}

static PyNumberMethods vintsubadd_as_number = {
    .nb_add = vintsubadd_add,
};

static PyTypeObject VIntSubAdd_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.VIntSubAdd",
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_base = &VInt_Type,
    .tp_as_number = &vintsubadd_as_number,
};


/* VIntSpec: both sides through internal spec slot ids */

static PyValue
vintspec_add_value(PyValueBorrow a, PyValueBorrow b)
{
    if (log_call("VIntSpec.add") < 0) {
        return PyValue_NULL;
    }
    return _PyValue_FromObjectResult(vint_arith(a, b, '+', 1));
}

_Py_VALUE_THUNK_BINARY(vintspec_add, vintspec_add_value)

static PyType_Slot vintspec_slots[] = {
    {Py_nb_add, vintspec_add},
    {_Py_value_nb_add, (void *)vintspec_add_value},
    {0, NULL},
};

static PyType_Spec vintspec_spec = {
    .name = "_testinternalcapi.VIntSpec",
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = vintspec_slots,
};


/* NotImpl: object slots that return NotImplemented unless every operand
 * is a NotImpl (script class N), with value sides made by the
 * _Py_VALUE_NOTIMPL_* wrappers. */

typedef struct {
    PyObject_HEAD
    long long value;
} NotImplObject;

static PyTypeObject NotImpl_Type;

static PyObject *
notimpl_make(long long value)
{
    NotImplObject *self = PyObject_New(NotImplObject, &NotImpl_Type);
    if (self != NULL) {
        self->value = value;
    }
    return (PyObject *)self;
}

static PyObject *
notimpl_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    long long value;
    if (kwds != NULL && PyDict_GET_SIZE(kwds) != 0) {
        PyErr_SetString(PyExc_TypeError, "NotImpl takes no keyword arguments");
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "L:NotImpl", &value)) {
        return NULL;
    }
    NotImplObject *self = (NotImplObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->value = value;
    }
    return (PyObject *)self;
}

#define NotImpl_Check(op) PyObject_TypeCheck(op, &NotImpl_Type)
#define NOTIMPL_VALUE(op) (((NotImplObject *)(op))->value)

static PyObject *
notimpl_add(PyObject *a, PyObject *b)
{
    if (log_call("NotImpl.add") < 0) {
        return NULL;
    }
    if (!NotImpl_Check(a) || !NotImpl_Check(b)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    return notimpl_make(NOTIMPL_VALUE(a) + NOTIMPL_VALUE(b));
}

static PyObject *
notimpl_power(PyObject *a, PyObject *b, PyObject *c)
{
    if (log_call("NotImpl.power") < 0) {
        return NULL;
    }
    if (!NotImpl_Check(a) || !NotImpl_Check(b)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    // The modulus, if any, is an int: the result is a**b % c, b small.
    long long result = 1;
    for (long long i = 0; i < NOTIMPL_VALUE(b); i++) {
        result *= NOTIMPL_VALUE(a);
    }
    if (c != Py_None) {
        long long modulus = PyLong_AsLongLong(c);
        if (modulus == -1 && PyErr_Occurred()) {
            return NULL;
        }
        result %= modulus;
    }
    return notimpl_make(result);
}

static PyObject *
notimpl_richcompare(PyObject *a, PyObject *b, int op)
{
    if (log_call("NotImpl.richcompare") < 0) {
        return NULL;
    }
    if (!NotImpl_Check(a) || !NotImpl_Check(b)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    Py_RETURN_RICHCOMPARE(NOTIMPL_VALUE(a), NOTIMPL_VALUE(b), op);
}

_Py_VALUE_NOTIMPL_BINARY(notimpl_add_value, notimpl_add)
_Py_VALUE_NOTIMPL_TERNARY(notimpl_power_value, notimpl_power)
_Py_VALUE_NOTIMPL_RICHCMP(notimpl_richcompare_value, notimpl_richcompare)

static PyNumberMethods notimpl_as_number = {
    .nb_add = notimpl_add,
    .nb_power = notimpl_power,
};

static PyMemberDef notimpl_members[] = {
    {"value", Py_T_LONGLONG, offsetof(NotImplObject, value), Py_READONLY},
    {NULL},
};

static PyTypeObject NotImpl_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.NotImpl",
    .tp_basicsize = sizeof(NotImplObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = notimpl_new,
    .tp_members = notimpl_members,
    .tp_as_number = &notimpl_as_number,
    .tp_richcompare = notimpl_richcompare,
    .tp_value = {
        .nb_add = _Py_VALUE_SLOT_INIT(nb_add, notimpl_add_value),
        .nb_power = _Py_VALUE_SLOT_INIT(nb_power, notimpl_power_value),
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare,
                                           notimpl_richcompare_value),
    },
};


/* VSeq: a sequence whose sq_concat is a thunk of its value side. */

typedef struct {
    PyObject_HEAD
    long long value;
} VSeqObject;

static PyTypeObject VSeq_Type;

static PyObject *
vseq_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    long long value;
    if (kwds != NULL && PyDict_GET_SIZE(kwds) != 0) {
        PyErr_SetString(PyExc_TypeError, "VSeq takes no keyword arguments");
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "L:VSeq", &value)) {
        return NULL;
    }
    VSeqObject *self = (VSeqObject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    self->value = value;
    return (PyObject *)self;
}

/* VSeq(self.value * 1000 + n) for an int or VInt n; a sequence raises
 * TypeError for anything else. */
static PyValue
vseq_concat_value(PyObject *self, PyValueBorrow other)
{
    if (log_call("VSeq.concat") < 0) {
        return PyValue_NULL;
    }
    long long n;
    int is_vint;
    int res = vint_operand(other, &n, &is_vint);
    if (res < 0) {
        return PyValue_NULL;
    }
    if (res == 0) {
        PyErr_SetString(PyExc_TypeError, "can only concatenate an int to VSeq");
        return PyValue_NULL;
    }
    VSeqObject *result = PyObject_New(VSeqObject, &VSeq_Type);
    if (result == NULL) {
        return PyValue_NULL;
    }
    result->value = ((VSeqObject *)self)->value * 1000 + n;
    return _PyValue_FromObjectResult((PyObject *)result);
}

_Py_VALUE_THUNK_CONCAT(vseq_concat, vseq_concat_value)

static PySequenceMethods vseq_as_sequence = {
    .sq_concat = vseq_concat,
};

static PyMemberDef vseq_members[] = {
    {"value", Py_T_LONGLONG, offsetof(VSeqObject, value), Py_READONLY},
    {NULL},
};

static PyTypeObject VSeq_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testinternalcapi.VSeq",
    .tp_basicsize = sizeof(VSeqObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = vseq_new,
    .tp_members = vseq_members,
    .tp_as_sequence = &vseq_as_sequence,
    .tp_value = {
        .sq_concat = _Py_VALUE_SLOT_INIT(sq_concat, vseq_concat_value),
    },
};


/* Types that break the invariant (section 4.4), for the tests that show
 * that the debug check aborts. A release build creates them unchecked. */

static PyType_Slot value_beside_null_slots[] = {
    {_Py_value_nb_negative, (void *)vint_negative_value},
    {0, NULL},
};

static PyType_Spec value_beside_null_spec = {
    .name = "_testinternalcapi.ValueBesideNull",
    .basicsize = sizeof(VIntObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = value_beside_null_slots,
};

// An own value nb_add beside the nb_add that VInt passes down.
static PyType_Slot value_beside_inherited_slots[] = {
    {_Py_value_nb_add, (void *)vintspec_add_value},
    {0, NULL},
};

static PyType_Spec value_beside_inherited_spec = {
    .name = "_testinternalcapi.ValueBesideInherited",
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = value_beside_inherited_slots,
};

static PyObject *
make_invalid_value_type(PyObject *Py_UNUSED(module), PyObject *kind)
{
    if (PyUnicode_EqualToUTF8(kind, "null")) {
        return PyType_FromSpec(&value_beside_null_spec);
    }
    if (PyUnicode_EqualToUTF8(kind, "inherited")) {
        PyObject *bases = PyTuple_Pack(1, (PyObject *)&VInt_Type);
        if (bases == NULL) {
            return NULL;
        }
        PyObject *type = PyType_FromSpecWithBases(&value_beside_inherited_spec,
                                                  bases);
        Py_DECREF(bases);
        return type;
    }
    PyErr_SetString(PyExc_ValueError, "expected 'null' or 'inherited'");
    return NULL;
}


/* Introspection */

static PyObject *
value_slot_names(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *names = PyTuple_New(_Py_VALUE_SLOT_COUNT);
    if (names == NULL) {
        return NULL;
    }
    Py_ssize_t i = 0;
#define ADD_NAME(FIELD, PATH, KIND, WRAPPER) \
    do { \
        PyObject *name = PyUnicode_FromString(#FIELD); \
        if (name == NULL) { \
            Py_DECREF(names); \
            return NULL; \
        } \
        PyTuple_SET_ITEM(names, i++, name); \
    } while (0);
    _Py_VALUE_SLOT_TABLE(ADD_NAME)
#undef ADD_NAME
    return names;
}

enum {
    KIND_UNARY, KIND_BINARY, KIND_TERNARY, KIND_INQUIRY, KIND_RICHCMP,
    KIND_HASH, KIND_REPR, KIND_GETATTRO, KIND_SETATTRO, KIND_DESCRGET,
    KIND_DESCRSET, KIND_SUBSCRIPT, KIND_ASSSUBSCRIPT, KIND_ITEM,
    KIND_ASSITEM, KIND_CONTAINS, KIND_ITERNEXT, KIND_SEND, KIND_VECTORCALL,
    KIND_CONCAT,
};

typedef struct {
    const char *name;
    size_t value_offset;   // in PyTypeObject
    size_t object_offset;  // in PyHeapTypeObject
    int kind;
} slot_row;

static const slot_row slot_rows[] = {
#define ROW(FIELD, PATH, KIND, WRAPPER) \
    {#FIELD, offsetof(PyTypeObject, tp_value.FIELD), \
     offsetof(PyHeapTypeObject, PATH), KIND_##KIND},
    _Py_VALUE_SLOT_TABLE(ROW)
#undef ROW
};

static const slot_row *
find_row(PyObject *name)
{
    const char *s = PyUnicode_AsUTF8(name);
    if (s == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < Py_ARRAY_LENGTH(slot_rows); i++) {
        if (strcmp(slot_rows[i].name, s) == 0) {
            return &slot_rows[i];
        }
    }
    PyErr_Format(PyExc_ValueError, "unknown value slot %R", name);
    return NULL;
}

/* As slotptr() in Objects/typeobject.c. */
static void **
object_slotptr(PyTypeObject *type, size_t offset)
{
    char *ptr;
    if (offset >= offsetof(PyHeapTypeObject, as_buffer)) {
        ptr = (char *)type->tp_as_buffer;
        offset -= offsetof(PyHeapTypeObject, as_buffer);
    }
    else if (offset >= offsetof(PyHeapTypeObject, as_sequence)) {
        ptr = (char *)type->tp_as_sequence;
        offset -= offsetof(PyHeapTypeObject, as_sequence);
    }
    else if (offset >= offsetof(PyHeapTypeObject, as_mapping)) {
        ptr = (char *)type->tp_as_mapping;
        offset -= offsetof(PyHeapTypeObject, as_mapping);
    }
    else if (offset >= offsetof(PyHeapTypeObject, as_number)) {
        ptr = (char *)type->tp_as_number;
        offset -= offsetof(PyHeapTypeObject, as_number);
    }
    else if (offset >= offsetof(PyHeapTypeObject, as_async)) {
        ptr = (char *)type->tp_as_async;
        offset -= offsetof(PyHeapTypeObject, as_async);
    }
    else {
        ptr = (char *)type;
    }
    if (ptr != NULL) {
        ptr += offset;
    }
    return (void **)ptr;
}

static PyObject *
address_to_int(const void *address)
{
    return PyLong_FromUnsignedLongLong((uintptr_t)address);
}

static PyObject *
value_slot(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyTypeObject *type;
    PyObject *name;
    if (!PyArg_ParseTuple(args, "O!U:value_slot", &PyType_Type, &type, &name)) {
        return NULL;
    }
    const slot_row *row = find_row(name);
    if (row == NULL) {
        return NULL;
    }
    void *fn = *(void **)((char *)type + row->value_offset);
    return address_to_int(fn);
}

static PyObject *
object_slot(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyTypeObject *type;
    PyObject *name;
    if (!PyArg_ParseTuple(args, "O!U:object_slot", &PyType_Type, &type, &name)) {
        return NULL;
    }
    const slot_row *row = find_row(name);
    if (row == NULL) {
        return NULL;
    }
    void **ptr = object_slotptr(type, row->object_offset);
    return address_to_int(ptr == NULL ? NULL : *ptr);
}

/* The object of a value-slot result. It also checks that PyValue_Is()
 * tells NotImplemented as identity does, which the dispatchers rely on. */
static PyObject *
value_result(PyValue result)
{
    int is_notimpl = (!PyValue_IsNull(result)
                      && PyValue_Is(result, PyValue_NotImplemented));
    PyObject *obj = _PyValue_THUNK_EXIT(result);
    if (obj != NULL && is_notimpl != (obj == Py_NotImplemented)) {
        Py_DECREF(obj);
        PyErr_SetString(PyExc_AssertionError,
                        "PyValue_Is() disagrees with NotImplemented's identity");
        return NULL;
    }
    return obj;
}

/* call_value_slot(type, name, *args): call type's value slot name with the
 * arguments as borrowed values (an exact int or float that fits is inline),
 * and return its result as an object. A comparison takes the operator as
 * its third argument; a concatenation passes its first argument, the
 * sequence, as an object. */
static PyObject *
call_value_slot(PyObject *Py_UNUSED(module), PyObject *const *args,
                Py_ssize_t nargs)
{
    if (nargs < 2) {
        PyErr_SetString(PyExc_TypeError, "call_value_slot(type, name, *args)");
        return NULL;
    }
    PyObject *type = args[0];
    if (!PyType_Check(type)) {
        PyErr_SetString(PyExc_TypeError, "expected a type");
        return NULL;
    }
    PyTypeObject *tp = (PyTypeObject *)type;
    const slot_row *row = find_row(args[1]);
    if (row == NULL) {
        return NULL;
    }
    _Py_valuefunc fn = *(_Py_valuefunc *)((char *)tp + row->value_offset);
    if (fn == NULL) {
        PyErr_Format(PyExc_LookupError, "%s has no value slot %s",
                     tp->tp_name, row->name);
        return NULL;
    }
    PyValueBorrow v[3];
    for (Py_ssize_t i = 2; i < nargs && i < 5; i++) {
        v[i - 2] = PyValue_Borrow(
            PyValue_FromObjectBorrow(args[i]));
    }
    Py_ssize_t operands = nargs - 2;
#define EXPECT(N) \
    if (operands != (N)) { \
        PyErr_Format(PyExc_TypeError, "value slot %s takes %d arguments", \
                     row->name, (N)); \
        return NULL; \
    }
    switch (row->kind) {
    case KIND_UNARY:
        EXPECT(1);
        return value_result(((_Py_unaryvaluefunc)fn)(v[0]));
    case KIND_BINARY:
        EXPECT(2);
        return value_result(((_Py_binaryvaluefunc)fn)(v[0], v[1]));
    case KIND_TERNARY:
        EXPECT(3);
        return value_result(((_Py_ternaryvaluefunc)fn)(v[0], v[1], v[2]));
    case KIND_CONCAT:
        EXPECT(2);
        return value_result(((_Py_concatvaluefunc)fn)(args[2], v[1]));
    case KIND_RICHCMP: {
        EXPECT(3);
        int op = PyLong_AsInt(args[4]);
        if (op == -1 && PyErr_Occurred()) {
            return NULL;
        }
        return value_result(((_Py_richcmpvaluefunc)fn)(v[0], v[1], op));
    }
    case KIND_INQUIRY: {
        EXPECT(1);
        int res = ((_Py_inquiryvaluefunc)fn)(v[0]);
        return res < 0 ? NULL : PyBool_FromLong(res);
    }
    case KIND_HASH: {
        EXPECT(1);
        Py_hash_t res = ((_Py_hashvaluefunc)fn)(v[0]);
        return res == -1 ? NULL : PyLong_FromSsize_t(res);
    }
    case KIND_REPR:
        EXPECT(1);
        return ((_Py_reprvaluefunc)fn)(v[0]);
    default:
        PyErr_Format(PyExc_NotImplementedError,
                     "call_value_slot does not call %s", row->name);
        return NULL;
    }
#undef EXPECT
}

static PyObject *
value_functions(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyObject *dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }
    static const struct {
        const char *name;
        const void *address;
    } entries[] = {
        {"VInt.add", (const void *)vint_add_value},
        {"VInt.multiply", (const void *)vint_multiply_value},
        {"VInt.richcompare", (const void *)vint_richcompare_value},
        {"VInt.hash", (const void *)vint_hash_value},
        {"VInt.repr", (const void *)vint_repr_value},
        {"VInt.negative", (const void *)vint_negative_value},
        {"VInt.power", (const void *)vint_power_value},
        {"VInt.bool", (const void *)vint_bool_value},
        {"VSeq.concat", (const void *)vseq_concat_value},
        {"VIntSpec.add", (const void *)vintspec_add_value},
        {"_PyValue_GenericHash", (const void *)_PyValue_GenericHash},
        {"_PyValue_HashNotImplemented", (const void *)_PyValue_HashNotImplemented},
        {"VInt.add.object", (const void *)vint_add},
        {"NotImpl.add", (const void *)notimpl_add_value},
        {"NotImpl.power", (const void *)notimpl_power_value},
        {"NotImpl.richcompare", (const void *)notimpl_richcompare_value},
    };
    for (size_t i = 0; i < Py_ARRAY_LENGTH(entries); i++) {
        PyObject *address = address_to_int(entries[i].address);
        if (address == NULL
            || PyDict_SetItemString(dict, entries[i].name, address) < 0)
        {
            Py_XDECREF(address);
            Py_DECREF(dict);
            return NULL;
        }
        Py_DECREF(address);
    }
    return dict;
}

static PyMethodDef methods[] = {
    {"set_value_slots_log", set_value_slots_log, METH_O},
    {"value_slot_names", value_slot_names, METH_NOARGS},
    {"value_slot", value_slot, METH_VARARGS},
    {"object_slot", object_slot, METH_VARARGS},
    {"value_functions", value_functions, METH_NOARGS},
    {"call_value_slot", _PyCFunction_CAST(call_value_slot), METH_FASTCALL},
    {"make_invalid_value_type", make_invalid_value_type, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_ValueSlots(PyObject *module)
{
    if (PyModule_AddFunctions(module, methods) < 0) {
        return -1;
    }
    PyTypeObject *static_types[] = {
        &VInt_Type,
        &VIntSubNothing_Type,
        &VIntSubRepr_Type,
        &VIntSubAdd_Type,
        &NotImpl_Type,
        &VSeq_Type,
    };
    for (size_t i = 0; i < Py_ARRAY_LENGTH(static_types); i++) {
        if (PyType_Ready(static_types[i]) < 0) {
            return -1;
        }
        if (PyModule_AddType(module, static_types[i]) < 0) {
            return -1;
        }
    }
    PyObject *bases = PyTuple_Pack(1, (PyObject *)&VInt_Type);
    if (bases == NULL) {
        return -1;
    }
    PyObject *spec_type = PyType_FromSpecWithBases(&vintspec_spec, bases);
    Py_DECREF(bases);
    if (spec_type == NULL) {
        return -1;
    }
    return PyModule_Add(module, "VIntSpec", spec_type);
}
