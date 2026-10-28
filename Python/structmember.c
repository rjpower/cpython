
/* Map C struct members to Python object attributes */

#include "Python.h"
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_descrobject.h"   // _PyMember_GetOffset()
#include "pycore_long.h"          // _PyLong_IsNegative()
#include "pycore_object.h"        // _Py_TryIncrefCompare(), FT_ATOMIC_*()
#include "pycore_critical_section.h"


static inline PyObject *
member_get_object(const char *addr, const char *obj_addr, PyMemberDef *l)
{
    PyObject *v = FT_ATOMIC_LOAD_PTR(*(PyObject **) addr);
    if (v == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "'%T' object has no attribute '%s'",
                     (PyObject *)obj_addr, l->name);
    }
    return v;
}

void *
_PyMember_GetOffset(PyObject *obj, PyMemberDef *mp)
{
    unsigned char *addr = (unsigned char *)obj + mp->offset;
    if (mp->flags & _Py_AFTER_ITEMS) {
        PyTypeObject *type = Py_TYPE(obj);
        addr += _Py_SIZE_ROUND_UP(Py_SIZE(obj) * type->tp_itemsize, SIZEOF_VOID_P);
    }
    return addr;
}

/* The member's value.  Numbers come inline where they fit, and a
   _Py_T_VALUE field is read as it is, so a read makes no box. */
PyValue
PyMember_GetOneValue(const char *obj_addr, PyMemberDef *l)
{
    PyObject *v;
    if (l->flags & Py_RELATIVE_OFFSET) {
        PyErr_SetString(
            PyExc_SystemError,
            "PyMember_GetOne used with Py_RELATIVE_OFFSET");
        return PyValue_NULL;
    }

    const void *addr = _PyMember_GetOffset((PyObject *)obj_addr, l);
    switch (l->type) {
    case Py_T_BOOL:
        v = PyBool_FromLong(FT_ATOMIC_LOAD_CHAR_RELAXED(*(char*)addr));
        break;
    case Py_T_BYTE:
        return PyValue_FromInt32(FT_ATOMIC_LOAD_CHAR_RELAXED(*(char*)addr));
    case Py_T_UBYTE:
        return PyValue_FromInt32(FT_ATOMIC_LOAD_UCHAR_RELAXED(*(unsigned char*)addr));
    case Py_T_SHORT:
        return PyValue_FromInt32(FT_ATOMIC_LOAD_SHORT_RELAXED(*(short*)addr));
    case Py_T_USHORT:
        return PyValue_FromInt32(FT_ATOMIC_LOAD_USHORT_RELAXED(*(unsigned short*)addr));
    case Py_T_INT:
        return PyValue_FromLongLong(FT_ATOMIC_LOAD_INT_RELAXED(*(int*)addr));
    case Py_T_UINT:
        return PyValue_FromUnsignedLongLong(FT_ATOMIC_LOAD_UINT_RELAXED(*(unsigned int*)addr));
    case Py_T_LONG:
        return PyValue_FromLongLong(FT_ATOMIC_LOAD_LONG_RELAXED(*(long*)addr));
    case Py_T_ULONG:
        return PyValue_FromUnsignedLongLong(FT_ATOMIC_LOAD_ULONG_RELAXED(*(unsigned long*)addr));
    case Py_T_PYSSIZET:
        return PyValue_FromLongLong(FT_ATOMIC_LOAD_SSIZE_RELAXED(*(Py_ssize_t*)addr));
    case Py_T_FLOAT:
        return PyValue_FromDouble((double)FT_ATOMIC_LOAD_FLOAT_RELAXED(*(float*)addr));
    case Py_T_DOUBLE:
        return PyValue_FromDouble(FT_ATOMIC_LOAD_DOUBLE_RELAXED(*(double*)addr));
    case Py_T_STRING:
        if (*(char**)addr == NULL) {
            v = Py_NewRef(Py_None);
        }
        else
            v = PyUnicode_FromString(*(char**)addr);
        break;
    case Py_T_STRING_INPLACE:
        v = PyUnicode_FromString((char*)addr);
        break;
    case Py_T_CHAR: {
        char char_val = FT_ATOMIC_LOAD_CHAR_RELAXED(*(char*)addr);
        v = PyUnicode_FromStringAndSize(&char_val, 1);
        break;
    }
    case _Py_T_OBJECT:
        v = FT_ATOMIC_LOAD_PTR(*(PyObject **) addr);
        if (v != NULL) {
            Py_INCREF(v);
        }
        if (v == NULL) {
            v = Py_None;
        }
        break;
    case _Py_T_VALUE: {
        PyValue value = *(PyValue *)addr;
        return PyValue_IsNull(value) ? PyValue_FromObject(Py_None) : value;
    }
    case _Py_T_VALUE_EX: {
        PyValue value = *(PyValue *)addr;
        if (PyValue_IsNull(value)) {
            PyErr_Format(PyExc_AttributeError,
                         "'%T' object has no attribute '%s'",
                         (PyObject *)obj_addr, l->name);
            return PyValue_NULL;
        }
        return value;
    }
    case Py_T_OBJECT_EX:
        v = member_get_object(addr, obj_addr, l);
        Py_XINCREF(v);
        break;
    case Py_T_LONGLONG:
        return PyValue_FromLongLong(FT_ATOMIC_LOAD_LLONG_RELAXED(*(long long *)addr));
    case Py_T_ULONGLONG:
        return PyValue_FromUnsignedLongLong(FT_ATOMIC_LOAD_ULLONG_RELAXED(*(unsigned long long *)addr));
    case _Py_T_NONE:
        // doesn't require free-threading code path
        v = Py_NewRef(Py_None);
        break;
    default:
        PyErr_SetString(PyExc_SystemError, "bad memberdescr type");
        v = NULL;
    }
    return PyValue_FromResult(v);
}

PyObject *
PyMember_GetOne(const char *obj_addr, PyMemberDef *l)
{
    // NULL means an error here, so PyValue_Object()'s NULL is one too.
    return PyValue_Object(PyMember_GetOneValue(obj_addr, l));
}

#define WARN(msg)                                               \
    do {                                                        \
    if (PyErr_WarnEx(PyExc_RuntimeWarning, msg, 1) < 0)         \
        return -1;                                              \
    } while (0)

/* An unsigned field's store of an inline int, VAL, with today's
   acceptance of a negative int. */
#define STORE_UNSIGNED(STORE, CTYPE, VAL)                               \
    do {                                                                \
        STORE(*(CTYPE *)addr, (CTYPE)(VAL));                            \
        if ((VAL) < 0) {                                                \
            WARN("Writing negative value into unsigned field");         \
        }                                                               \
    } while (0)

/* PyMember_SetOne() with the value as a PyValue; PyValue_NULL deletes.  An
   inline int or float is stored without a box.  A box is made for a
   _Py_T_OBJECT or Py_T_OBJECT_EX field, which holds an object, and stored
   with no call in between; and, as the value converters do, for the
   TypeError of an inline float given to an integer field. */
int
PyMember_SetOneValue(char *addr, PyMemberDef *l, PyValue v)
{
    PyObject *oldv;
    if (l->flags & Py_RELATIVE_OFFSET) {
        PyErr_SetString(
            PyExc_SystemError,
            "PyMember_SetOne used with Py_RELATIVE_OFFSET");
        return -1;
    }

    PyObject *obj = (PyObject *)addr;
    addr = _PyMember_GetOffset(obj, l);

    if ((l->flags & Py_READONLY))
    {
        PyErr_SetString(PyExc_AttributeError, "readonly attribute");
        return -1;
    }
    if (PyValue_IsNull(v) && l->type != Py_T_OBJECT_EX && l->type != _Py_T_OBJECT
        && l->type != _Py_T_VALUE_EX)
    {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete numeric/char attribute");
        return -1;
    }
    switch (l->type) {
    case Py_T_BOOL:{
        if (!PyValue_IsObject(v) || !PyBool_Check(PyValue_ObjectBorrow(v))) {
            PyErr_SetString(PyExc_TypeError,
                            "attribute value type must be bool");
            return -1;
        }
        if (PyValue_ObjectBorrow(v) == Py_True)
            FT_ATOMIC_STORE_CHAR_RELAXED(*(char*)addr, 1);
        else
            FT_ATOMIC_STORE_CHAR_RELAXED(*(char*)addr, 0);
        break;
        }
    case Py_T_BYTE:{
        long long_val = PyLong_AsLongValue(v);
        if ((long_val == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_CHAR_RELAXED(*(char*)addr, (char)long_val);
        /* XXX: For compatibility, only warn about truncations
           for now. */
        if ((long_val > CHAR_MAX) || (long_val < CHAR_MIN))
            WARN("Truncation of value to char");
        break;
        }
    case Py_T_UBYTE:{
        long long_val = PyLong_AsLongValue(v);
        if ((long_val == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_UCHAR_RELAXED(*(unsigned char*)addr, (unsigned char)long_val);
        if ((long_val > UCHAR_MAX) || (long_val < 0))
            WARN("Truncation of value to unsigned char");
        break;
        }
    case Py_T_SHORT:{
        long long_val = PyLong_AsLongValue(v);
        if ((long_val == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_SHORT_RELAXED(*(short*)addr, (short)long_val);
        if ((long_val > SHRT_MAX) || (long_val < SHRT_MIN))
            WARN("Truncation of value to short");
        break;
        }
    case Py_T_USHORT:{
        long long_val = PyLong_AsLongValue(v);
        if ((long_val == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_USHORT_RELAXED(*(unsigned short*)addr, (unsigned short)long_val);
        if ((long_val > USHRT_MAX) || (long_val < 0))
            WARN("Truncation of value to unsigned short");
        break;
        }
    case Py_T_INT:{
        long long_val = PyLong_AsLongValue(v);
        if ((long_val == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_INT_RELAXED(*(int *)addr, (int)long_val);
        if ((long_val > INT_MAX) || (long_val < INT_MIN))
            WARN("Truncation of value to int");
        break;
        }
    case Py_T_UINT: {
        /* XXX: For compatibility, accept negative int values
           as well. */
        if (PyValue_IsInlineInt(v)) {
            long long_val = PyValue_Int32(v);
            STORE_UNSIGNED(FT_ATOMIC_STORE_UINT_RELAXED, unsigned int, long_val);
            break;
        }
        PyObject *o = PyValue_Object(v);
        o = o == NULL ? NULL : _PyNumber_Index(o);
        if (o == NULL) {
            return -1;
        }
        if (_PyLong_IsNegative((PyLongObject *)o)) {
            long long_val = PyLong_AsLong(o);
            Py_DECREF(o);
            if (long_val == -1 && PyErr_Occurred()) {
                return -1;
            }
            FT_ATOMIC_STORE_UINT_RELAXED(*(unsigned int *)addr, (unsigned int)(unsigned long)long_val);
            WARN("Writing negative value into unsigned field");
        }
        else {
            unsigned long ulong_val = PyLong_AsUnsignedLong(o);
            Py_DECREF(o);
            if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
                return -1;
            }
            FT_ATOMIC_STORE_UINT_RELAXED(*(unsigned int *)addr, (unsigned int)ulong_val);
            if (ulong_val > UINT_MAX) {
                WARN("Truncation of value to unsigned int");
            }
        }
        break;
    }
    case Py_T_LONG: {
        const long long_val = PyLong_AsLongValue(v);
        if ((long_val == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_LONG_RELAXED(*(long*)addr, long_val);
        break;
    }
    case Py_T_ULONG: {
        /* XXX: For compatibility, accept negative int values
           as well. */
        if (PyValue_IsInlineInt(v)) {
            long long_val = PyValue_Int32(v);
            STORE_UNSIGNED(FT_ATOMIC_STORE_ULONG_RELAXED, unsigned long, long_val);
            break;
        }
        PyObject *o = PyValue_Object(v);
        o = o == NULL ? NULL : _PyNumber_Index(o);
        if (o == NULL) {
            return -1;
        }
        if (_PyLong_IsNegative((PyLongObject *)o)) {
            long long_val = PyLong_AsLong(o);
            Py_DECREF(o);
            if (long_val == -1 && PyErr_Occurred()) {
                return -1;
            }
            FT_ATOMIC_STORE_ULONG_RELAXED(*(unsigned long *)addr, (unsigned long)long_val);
            WARN("Writing negative value into unsigned field");
        }
        else {
            unsigned long ulong_val = PyLong_AsUnsignedLong(o);
            Py_DECREF(o);
            if (ulong_val == (unsigned long)-1 && PyErr_Occurred()) {
                return -1;
            }
            FT_ATOMIC_STORE_ULONG_RELAXED(*(unsigned long *)addr, ulong_val);
        }
        break;
    }
    case Py_T_PYSSIZET: {
        const Py_ssize_t ssize_val = PyLong_AsSsize_tValue(v);
        if ((ssize_val == (Py_ssize_t)-1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_SSIZE_RELAXED(*(Py_ssize_t*)addr, ssize_val);
        break;
    }
    case Py_T_FLOAT:{
        double double_val = PyFloat_AsDoubleValue(v);
        if ((double_val == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_FLOAT_RELAXED(*(float*)addr, (float)double_val);
        break;
        }
    case Py_T_DOUBLE: {
        const double double_val = PyFloat_AsDoubleValue(v);
        if ((double_val == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_DOUBLE_RELAXED(*(double *) addr, double_val);
        break;
    }
    case _Py_T_VALUE:
        *(PyValue *)addr = v;
        break;
    case _Py_T_VALUE_EX: {
        PyValue old;
        Py_BEGIN_CRITICAL_SECTION(obj);
        old = *(PyValue *)addr;
        *(PyValue *)addr = v;
        Py_END_CRITICAL_SECTION();
        if (PyValue_IsNull(v) && PyValue_IsNull(old)) {
            // Deleting an attribute that is already deleted
            PyErr_SetString(PyExc_AttributeError, l->name);
            return -1;
        }
        break;
    }
    case _Py_T_OBJECT:
    case Py_T_OBJECT_EX: {
        // The field holds an object: an inline value's box is stored with
        // no call in between, so nothing collects while only this frame
        // holds it.
        PyObject *o = PyValue_Object(v);
        if (o == NULL && !PyValue_IsNull(v)) {
            return -1;
        }
        Py_BEGIN_CRITICAL_SECTION(obj);
        oldv = *(PyObject **)addr;
        FT_ATOMIC_STORE_PTR_RELEASE(*(PyObject **)addr, Py_XNewRef(o));
        Py_END_CRITICAL_SECTION();
        if (o == NULL && oldv == NULL && l->type == Py_T_OBJECT_EX) {
            // Raise an exception when attempting to delete an already deleted
            // attribute.
            // Differently from Py_T_OBJECT_EX, _Py_T_OBJECT does not raise an
            // exception here (PyMember_GetOne will return Py_None instead of
            // NULL).
            PyErr_SetString(PyExc_AttributeError, l->name);
            return -1;
        }
        Py_XDECREF(oldv);
        break;
    }
    case Py_T_CHAR: {
        const char *string = NULL;
        Py_ssize_t len = 0;

        if (PyValue_IsObject(v)) {
            string = PyUnicode_AsUTF8AndSize(PyValue_ObjectBorrow(v), &len);
        }
        if (string == NULL || len != 1) {
            PyErr_BadArgument();
            return -1;
        }
        FT_ATOMIC_STORE_CHAR_RELAXED(*(char*)addr, string[0]);
        break;
        }
    case Py_T_STRING:
    case Py_T_STRING_INPLACE:
        PyErr_SetString(PyExc_TypeError, "readonly attribute");
        return -1;
    case Py_T_LONGLONG:{
        long long value = PyLong_AsLongLongValue(v);
        if ((value == -1) && PyErr_Occurred())
            return -1;
        FT_ATOMIC_STORE_LLONG_RELAXED(*(long long*)addr, value);
        break;
        }
    case Py_T_ULONGLONG: {
        if (PyValue_IsInlineInt(v)) {
            long long_val = PyValue_Int32(v);
            STORE_UNSIGNED(FT_ATOMIC_STORE_ULLONG_RELAXED, unsigned long long, (long long)long_val);
            break;
        }
        PyObject *o = PyValue_Object(v);
        o = o == NULL ? NULL : _PyNumber_Index(o);
        if (o == NULL) {
            return -1;
        }
        if (_PyLong_IsNegative((PyLongObject *)o)) {
            long long_val = PyLong_AsLong(o);
            Py_DECREF(o);
            if (long_val == -1 && PyErr_Occurred()) {
                return -1;
            }
            FT_ATOMIC_STORE_ULLONG_RELAXED(*(unsigned long long *)addr, (unsigned long long)(long long)long_val);
            WARN("Writing negative value into unsigned field");
        }
        else {
            unsigned long long ulonglong_val = PyLong_AsUnsignedLongLong(o);
            Py_DECREF(o);
            if (ulonglong_val == (unsigned long long)-1 && PyErr_Occurred()) {
                return -1;
            }
            FT_ATOMIC_STORE_ULLONG_RELAXED(*(unsigned long long *)addr, ulonglong_val);
        }
        break;
    }
    default:
        PyErr_Format(PyExc_SystemError,
                     "bad memberdescr type for %s", l->name);
        return -1;
    }
    return 0;
}

int
PyMember_SetOne(char *addr, PyMemberDef *l, PyObject *v)
{
    return PyMember_SetOneValue(addr, l, v == NULL ? PyValue_NULL
                                                  : PyValue_FromObject(v));
}
