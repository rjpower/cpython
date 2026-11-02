#include "Python.h"
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_value.h"      // _PyObject_ValueIdentity()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_typeobject.h"    // _PyType_GetModuleState()
#include "pycore_unicodeobject.h" // _PyUnicode_InternMortal()


#include "clinic/_operator.c.h"

typedef struct {
    PyObject *itemgetter_type;
    PyObject *attrgetter_type;
    PyObject *methodcaller_type;
} _operator_state;

static inline _operator_state*
get_operator_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (_operator_state *)state;
}

/*[clinic input]
module _operator
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=672ecf48487521e7]*/

PyDoc_STRVAR(operator_doc,
"Operator interface.\n\
\n\
This module exports a set of functions implemented in C corresponding\n\
to the intrinsic operators of Python.  For example, operator.add(x, y)\n\
is equivalent to the expression x+y.  The function names are those\n\
used for special methods; variants without leading and trailing\n\
'__' are also provided for convenience.");


/*[clinic input]
_operator.truth -> bool

    a: object
    /

Return True if a is true, False otherwise.
[clinic start generated code]*/

static int
_operator_truth_impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=eaf87767234fa5d7 input=bc74a4cd90235875]*/
{
    return PyObject_IsTrue(a);
}

/*[clinic input]
_operator.add

    a: object
    b: object
    /

Same as a + b.
[clinic start generated code]*/

static PyObject *
_operator_add_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=8292984204f45164 input=5efe3bff856ac215]*/
{
    return PyNumber_Add(a, b);
}

/*[clinic input]
_operator.sub = _operator.add

Same as a - b.
[clinic start generated code]*/

static PyObject *
_operator_sub_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=4adfc3b888c1ee2e input=6494c6b100b8e795]*/
{
    return PyNumber_Subtract(a, b);
}

/*[clinic input]
_operator.mul = _operator.add

Same as a * b.
[clinic start generated code]*/

static PyObject *
_operator_mul_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=d24d66f55a01944c input=2368615b4358b70d]*/
{
    return PyNumber_Multiply(a, b);
}

/*[clinic input]
_operator.matmul = _operator.add

Same as a @ b.
[clinic start generated code]*/

static PyObject *
_operator_matmul_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=a20d917eb35d0101 input=9ab304e37fb42dd4]*/
{
    return PyNumber_MatrixMultiply(a, b);
}

/*[clinic input]
_operator.floordiv = _operator.add

Same as a // b.
[clinic start generated code]*/

static PyObject *
_operator_floordiv_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=df26b71a60589f99 input=bb2e88ba446c612c]*/
{
    return PyNumber_FloorDivide(a, b);
}

/*[clinic input]
_operator.truediv = _operator.add

Same as a / b.
[clinic start generated code]*/

static PyObject *
_operator_truediv_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=0e6a959944d77719 input=ecbb947673f4eb1f]*/
{
    return PyNumber_TrueDivide(a, b);
}

/*[clinic input]
_operator.mod = _operator.add

Same as a % b.
[clinic start generated code]*/

static PyObject *
_operator_mod_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=9519822f0bbec166 input=102e19b422342ac1]*/
{
    return PyNumber_Remainder(a, b);
}

/*[clinic input]
_operator.neg

    a: object
    /

Same as -a.
[clinic start generated code]*/

static PyObject *
_operator_neg(PyObject *module, PyObject *a)
/*[clinic end generated code: output=36e08ecfc6a1c08c input=84f09bdcf27c96ec]*/
{
    return PyNumber_Negative(a);
}

/*[clinic input]
_operator.pos = _operator.neg

Same as +a.
[clinic start generated code]*/

static PyObject *
_operator_pos(PyObject *module, PyObject *a)
/*[clinic end generated code: output=dad7a126221dd091 input=b6445b63fddb8772]*/
{
    return PyNumber_Positive(a);
}

/*[clinic input]
_operator.abs = _operator.neg

Same as abs(a).
[clinic start generated code]*/

static PyObject *
_operator_abs(PyObject *module, PyObject *a)
/*[clinic end generated code: output=1389a93ba053ea3e input=341d07ba86f58039]*/
{
    return PyNumber_Absolute(a);
}

/*[clinic input]
_operator.inv = _operator.neg

Same as ~a.
[clinic start generated code]*/

static PyObject *
_operator_inv(PyObject *module, PyObject *a)
/*[clinic end generated code: output=a56875ba075ee06d input=b01a4677739f6eb2]*/
{
    return PyNumber_Invert(a);
}

/*[clinic input]
_operator.invert = _operator.neg

Same as ~a.
[clinic start generated code]*/

static PyObject *
_operator_invert(PyObject *module, PyObject *a)
/*[clinic end generated code: output=406b5aa030545fcc input=7f2d607176672e55]*/
{
    return PyNumber_Invert(a);
}

/*[clinic input]
_operator.lshift = _operator.add

Same as a << b.
[clinic start generated code]*/

static PyObject *
_operator_lshift_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=37f7e52c41435bd8 input=746e8a160cbbc9eb]*/
{
    return PyNumber_Lshift(a, b);
}

/*[clinic input]
_operator.rshift = _operator.add

Same as a >> b.
[clinic start generated code]*/

static PyObject *
_operator_rshift_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=4593c7ef30ec2ee3 input=d2c85bb5a64504c2]*/
{
    return PyNumber_Rshift(a, b);
}

/*[clinic input]
_operator.not_ = _operator.truth

Same as not a.
[clinic start generated code]*/

static int
_operator_not__impl(PyObject *module, PyObject *a)
/*[clinic end generated code: output=743f9c24a09759ef input=854156d50804d9b8]*/
{
    return PyObject_Not(a);
}

/*[clinic input]
_operator.and_ = _operator.add

Same as a & b.
[clinic start generated code]*/

static PyObject *
_operator_and__impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=93c4fe88f7b76d9e input=4f3057c90ec4c99f]*/
{
    return PyNumber_And(a, b);
}

/*[clinic input]
_operator.xor = _operator.add

Same as a ^ b.
[clinic start generated code]*/

static PyObject *
_operator_xor_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=b24cd8b79fde0004 input=3c5cfa7253d808dd]*/
{
    return PyNumber_Xor(a, b);
}

/*[clinic input]
_operator.or_ = _operator.add

Same as a | b.
[clinic start generated code]*/

static PyObject *
_operator_or__impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=58024867b8d90461 input=b40c6c44f7c79c09]*/
{
    return PyNumber_Or(a, b);
}

/*[clinic input]
_operator.iadd = _operator.add

Same as a += b.
[clinic start generated code]*/

static PyObject *
_operator_iadd_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=07dc627832526eb5 input=d22a91c07ac69227]*/
{
    return PyNumber_InPlaceAdd(a, b);
}

/*[clinic input]
_operator.isub = _operator.add

Same as a -= b.
[clinic start generated code]*/

static PyObject *
_operator_isub_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=4513467d23b5e0b1 input=4591b00d0a0ccafd]*/
{
    return PyNumber_InPlaceSubtract(a, b);
}

/*[clinic input]
_operator.imul = _operator.add

Same as a *= b.
[clinic start generated code]*/

static PyObject *
_operator_imul_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=5e87dacd19a71eab input=0e01fb8631e1b76f]*/
{
    return PyNumber_InPlaceMultiply(a, b);
}

/*[clinic input]
_operator.imatmul = _operator.add

Same as a @= b.
[clinic start generated code]*/

static PyObject *
_operator_imatmul_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=d603cbdf716ce519 input=bb614026372cd542]*/
{
    return PyNumber_InPlaceMatrixMultiply(a, b);
}

/*[clinic input]
_operator.ifloordiv = _operator.add

Same as a //= b.
[clinic start generated code]*/

static PyObject *
_operator_ifloordiv_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=535336048c681794 input=9df3b5021cff4ca1]*/
{
    return PyNumber_InPlaceFloorDivide(a, b);
}

/*[clinic input]
_operator.itruediv = _operator.add

Same as a /= b.
[clinic start generated code]*/

static PyObject *
_operator_itruediv_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=28017fbd3563952f input=9a1ee01608f5f590]*/
{
    return PyNumber_InPlaceTrueDivide(a, b);
}

/*[clinic input]
_operator.imod = _operator.add

Same as a %= b.
[clinic start generated code]*/

static PyObject *
_operator_imod_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=f7c540ae0fc70904 input=d0c384a3ce38e1dd]*/
{
    return PyNumber_InPlaceRemainder(a, b);
}

/*[clinic input]
_operator.ilshift = _operator.add

Same as a <<= b.
[clinic start generated code]*/

static PyObject *
_operator_ilshift_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=e73a8fee1ac18749 input=e21b6b310f54572e]*/
{
    return PyNumber_InPlaceLshift(a, b);
}

/*[clinic input]
_operator.irshift = _operator.add

Same as a >>= b.
[clinic start generated code]*/

static PyObject *
_operator_irshift_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=97f2af6b5ff2ed81 input=6778dbd0f6e1ec16]*/
{
    return PyNumber_InPlaceRshift(a, b);
}

/*[clinic input]
_operator.iand = _operator.add

Same as a &= b.
[clinic start generated code]*/

static PyObject *
_operator_iand_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=4599e9d40cbf7d00 input=71dfd8e70c156a7b]*/
{
    return PyNumber_InPlaceAnd(a, b);
}

/*[clinic input]
_operator.ixor = _operator.add

Same as a ^= b.
[clinic start generated code]*/

static PyObject *
_operator_ixor_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=5ff881766872be03 input=695c32bec0604d86]*/
{
    return PyNumber_InPlaceXor(a, b);
}

/*[clinic input]
_operator.ior = _operator.add

Same as a |= b.
[clinic start generated code]*/

static PyObject *
_operator_ior_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=48aac319445bf759 input=8f01d03eda9920cf]*/
{
    return PyNumber_InPlaceOr(a, b);
}

/*[clinic input]
_operator.concat = _operator.add

Same as a + b, for a and b sequences.
[clinic start generated code]*/

static PyObject *
_operator_concat_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=80028390942c5f11 input=8544ccd5341a3658]*/
{
    return PySequence_Concat(a, b);
}

/*[clinic input]
_operator.iconcat = _operator.add

Same as a += b, for a and b sequences.
[clinic start generated code]*/

static PyObject *
_operator_iconcat_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=3ea0a162ebb2e26d input=8f5fe5722fcd837e]*/
{
    return PySequence_InPlaceConcat(a, b);
}

/*[clinic input]
_operator.contains -> bool

    a: object
    b: object
    /

Same as b in a (note reversed operands).
[clinic start generated code]*/

static int
_operator_contains_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=413b4dbe82b6ffc1 input=9122a69b505fde13]*/
{
    return PySequence_Contains(a, b);
}

/*[clinic input]
_operator.indexOf -> Py_ssize_t

    a: object
    b: object
    /

Return the first index of b in a.
[clinic start generated code]*/

static Py_ssize_t
_operator_indexOf_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=c6226d8e0fb60fa6 input=8be2e43b6a6fffe3]*/
{
    return PySequence_Index(a, b);
}

/*[clinic input]
_operator.countOf = _operator.indexOf

Return the number of items in a which are, or which equal, b.
[clinic start generated code]*/

static Py_ssize_t
_operator_countOf_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=9e1623197daf3382 input=93ea57f170f3f0bb]*/
{
    return PySequence_Count(a, b);
}

/*[clinic input]
_operator.getitem

    a: object
    b: object
    /

Same as a[b].
[clinic start generated code]*/

static PyObject *
_operator_getitem_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=6c8d8101a676e594 input=6682797320e48845]*/
{
    return PyObject_GetItem(a, b);
}

/*[clinic input]
_operator.setitem

    a: object
    b: object
    c: object
    /

Same as a[b] = c.
[clinic start generated code]*/

static PyObject *
_operator_setitem_impl(PyObject *module, PyObject *a, PyObject *b,
                       PyObject *c)
/*[clinic end generated code: output=1324f9061ae99e25 input=ceaf453c4d3a58df]*/
{
    if (-1 == PyObject_SetItem(a, b, c))
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
_operator.delitem = _operator.getitem

Same as del a[b].
[clinic start generated code]*/

static PyObject *
_operator_delitem_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=db18f61506295799 input=991bec56a0d3ec7f]*/
{
    if (-1 == PyObject_DelItem(a, b))
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
_operator.eq

    a: object
    b: object
    /

Same as a == b.
[clinic start generated code]*/

static PyObject *
_operator_eq_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=8d7d46ed4135677c input=586fca687a95a83f]*/
{
    return PyObject_RichCompare(a, b, Py_EQ);
}

/*[clinic input]
_operator.ne = _operator.eq

Same as a != b.
[clinic start generated code]*/

static PyObject *
_operator_ne_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=c99bd0c3a4c01297 input=5d88f23d35e9abac]*/
{
    return PyObject_RichCompare(a, b, Py_NE);
}

/*[clinic input]
_operator.lt = _operator.eq

Same as a < b.
[clinic start generated code]*/

static PyObject *
_operator_lt_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=082d7c45c440e535 input=34a59ad6d39d3a2b]*/
{
    return PyObject_RichCompare(a, b, Py_LT);
}

/*[clinic input]
_operator.le = _operator.eq

Same as a <= b.
[clinic start generated code]*/

static PyObject *
_operator_le_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=00970a2923d0ae17 input=b812a7860a0bef44]*/
{
    return PyObject_RichCompare(a, b, Py_LE);
}

/*[clinic input]
_operator.gt = _operator.eq

Same as a > b.
[clinic start generated code]*/

static PyObject *
_operator_gt_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=8d373349ecf25641 input=9bdb45b995ada35b]*/
{
    return PyObject_RichCompare(a, b, Py_GT);
}

/*[clinic input]
_operator.ge = _operator.eq

Same as a >= b.
[clinic start generated code]*/

static PyObject *
_operator_ge_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=7ce3882256d4b137 input=cf1dc4a5ca9c35f5]*/
{
    return PyObject_RichCompare(a, b, Py_GE);
}

/*[clinic input]
_operator.pow = _operator.add

Same as a ** b.
[clinic start generated code]*/

static PyObject *
_operator_pow_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=09e668ad50036120 input=690b40f097ab1637]*/
{
    return PyNumber_Power(a, b, Py_None);
}

/*[clinic input]
_operator.ipow = _operator.add

Same as a **= b.
[clinic start generated code]*/

static PyObject *
_operator_ipow_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=7189ff4d4367c808 input=f00623899d07499a]*/
{
    return PyNumber_InPlacePower(a, b, Py_None);
}

/*[clinic input]
_operator.index

    a: object
    /

Same as a.__index__()
[clinic start generated code]*/

static PyObject *
_operator_index(PyObject *module, PyObject *a)
/*[clinic end generated code: output=d972b0764ac305fc input=6f54d50ea64a579c]*/
{
    return PyNumber_Index(a);
}

/*[clinic input]
_operator.is_ = _operator.add

Same as a is b.
[clinic start generated code]*/

static PyObject *
_operator_is__impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=bcd47a402e482e1d input=5fa9b97df03c427f]*/
{
    int is = _PyObject_ValueIdentity(a) == _PyObject_ValueIdentity(b);
    return Py_NewRef(is ? Py_True : Py_False);
}

/*[clinic input]
_operator.is_not = _operator.add

Same as a is not b.
[clinic start generated code]*/

static PyObject *
_operator_is_not_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=491a1f2f81f6c7f9 input=5a93f7e1a93535f1]*/
{
    int is = _PyObject_ValueIdentity(a) == _PyObject_ValueIdentity(b);
    return Py_NewRef(is ? Py_False : Py_True);
}

/*[clinic input]
_operator.is_none = _operator.neg

Same as a is None.
[clinic start generated code]*/

static PyObject *
_operator_is_none(PyObject *module, PyObject *a)
/*[clinic end generated code: output=07159cc102261dec input=0448b38af7b8533d]*/
{
    PyObject *result = Py_IsNone(a) ? Py_True : Py_False;
    return Py_NewRef(result);
}

/*[clinic input]
_operator.is_not_none = _operator.neg

Same as a is not None.
[clinic start generated code]*/

static PyObject *
_operator_is_not_none(PyObject *module, PyObject *a)
/*[clinic end generated code: output=b0168a51451d9140 input=7587f38ebac51688]*/
{
    PyObject *result = Py_IsNone(a) ? Py_False : Py_True;
    return Py_NewRef(result);
}

/* compare_digest **********************************************************/

/*
 * timing safe compare
 *
 * Returns 1 if the strings are equal.
 * In case of len(a) != len(b) the function tries to keep the timing
 * dependent on the length of b. CPU cache locality may still alter timing
 * a bit.
 */
static int
_tscmp(const unsigned char *a, const unsigned char *b,
        Py_ssize_t len_a, Py_ssize_t len_b)
{
    /* The volatile type declarations make sure that the compiler has no
     * chance to optimize and fold the code in any way that may change
     * the timing.
     */
    volatile Py_ssize_t length;
    volatile const unsigned char *left;
    volatile const unsigned char *right;
    Py_ssize_t i;
    volatile unsigned char result;

    /* loop count depends on length of b */
    length = len_b;
    left = NULL;
    right = b;

    /* don't use else here to keep the amount of CPU instructions constant,
     * volatile forces re-evaluation
     *  */
    if (len_a == length) {
        left = *((volatile const unsigned char**)&a);
        result = 0;
    }
    if (len_a != length) {
        left = b;
        result = 1;
    }

    for (i=0; i < length; i++) {
        result |= *left++ ^ *right++;
    }

    return (result == 0);
}

/*[clinic input]
_operator.length_hint -> Py_ssize_t

    obj: object
    default: Py_ssize_t = 0
    /

Return an estimate of the number of items in obj.

This is useful for presizing containers when building from an iterable.

If the object supports len(), the result will be exact.
Otherwise, it may over- or under-estimate by an arbitrary amount.
The result will be an integer >= 0.
[clinic start generated code]*/

static Py_ssize_t
_operator_length_hint_impl(PyObject *module, PyObject *obj,
                           Py_ssize_t default_value)
/*[clinic end generated code: output=01d469edc1d612ad input=65ed29f04401e96a]*/
{
    return PyObject_LengthHint(obj, default_value);
}

/* NOTE: Keep in sync with _hashopenssl.c implementation. */

/*[clinic input]
_operator._compare_digest = _operator.eq

Return 'a == b'.

This function uses an approach designed to prevent
timing analysis, making it appropriate for cryptography.

a and b must both be of the same type: either str (ASCII only),
or any bytes-like object.

Note: If a and b are of different lengths, or if an error occurs,
a timing attack could theoretically reveal information about the
types and lengths of a and b--but not their values.
[clinic start generated code]*/

static PyObject *
_operator__compare_digest_impl(PyObject *module, PyObject *a, PyObject *b)
/*[clinic end generated code: output=11d452bdd3a23cbc input=9ac7e2c4e30bc356]*/
{
    int rc;

    /* ASCII unicode string */
    if(PyUnicode_Check(a) && PyUnicode_Check(b)) {
        if (!PyUnicode_IS_ASCII(a) || !PyUnicode_IS_ASCII(b)) {
            PyErr_SetString(PyExc_TypeError,
                            "comparing strings with non-ASCII characters is "
                            "not supported");
            return NULL;
        }

        rc = _tscmp(PyUnicode_DATA(a),
                    PyUnicode_DATA(b),
                    PyUnicode_GET_LENGTH(a),
                    PyUnicode_GET_LENGTH(b));
    }
    /* fallback to buffer interface for bytes, bytearray and other */
    else {
        Py_buffer view_a;
        Py_buffer view_b;

        if (PyObject_CheckBuffer(a) == 0 && PyObject_CheckBuffer(b) == 0) {
            PyErr_Format(PyExc_TypeError,
                         "unsupported operand types(s) or combination of types: "
                         "'%.100s' and '%.100s'",
                         Py_TYPE(a)->tp_name, Py_TYPE(b)->tp_name);
            return NULL;
        }

        if (PyObject_GetBuffer(a, &view_a, PyBUF_SIMPLE) == -1) {
            return NULL;
        }
        if (view_a.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            return NULL;
        }

        if (PyObject_GetBuffer(b, &view_b, PyBUF_SIMPLE) == -1) {
            PyBuffer_Release(&view_a);
            return NULL;
        }
        if (view_b.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            PyBuffer_Release(&view_b);
            return NULL;
        }

        rc = _tscmp((const unsigned char*)view_a.buf,
                    (const unsigned char*)view_b.buf,
                    view_a.len,
                    view_b.len);

        PyBuffer_Release(&view_a);
        PyBuffer_Release(&view_b);
    }

    return PyBool_FromLong(rc);
}

PyDoc_STRVAR(_operator_call__doc__,
"call($module, obj, /, *args, **kwargs)\n"
"--\n"
"\n"
"Same as obj(*args, **kwargs).");

#define _OPERATOR_CALL_METHODDEF    \
    {"call", _PyCFunction_CAST(_operator_call), METH_FASTCALL | METH_KEYWORDS, _operator_call__doc__},

static PyObject *
_operator_call(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (!_PyArg_CheckPositional("call", nargs, 1, PY_SSIZE_T_MAX)) {
        return NULL;
    }
    return PyObject_Vectorcall(
            args[0],
            &args[1], (PyVectorcall_NARGS(nargs) - 1) | PY_VECTORCALL_ARGUMENTS_OFFSET,
            kwnames);
}

/* operator methods **********************************************************/

static struct PyMethodDef operator_methods[] = {

    _OPERATOR_TRUTH_METHODDEF
    _OPERATOR_CONTAINS_METHODDEF
    _OPERATOR_INDEXOF_METHODDEF
    _OPERATOR_COUNTOF_METHODDEF
    _OPERATOR_IS__METHODDEF
    _OPERATOR_IS_NOT_METHODDEF
    _OPERATOR_IS_NONE_METHODDEF
    _OPERATOR_IS_NOT_NONE_METHODDEF
    _OPERATOR_INDEX_METHODDEF
    _OPERATOR_ADD_METHODDEF
    _OPERATOR_SUB_METHODDEF
    _OPERATOR_MUL_METHODDEF
    _OPERATOR_MATMUL_METHODDEF
    _OPERATOR_FLOORDIV_METHODDEF
    _OPERATOR_TRUEDIV_METHODDEF
    _OPERATOR_MOD_METHODDEF
    _OPERATOR_NEG_METHODDEF
    _OPERATOR_POS_METHODDEF
    _OPERATOR_ABS_METHODDEF
    _OPERATOR_INV_METHODDEF
    _OPERATOR_INVERT_METHODDEF
    _OPERATOR_LSHIFT_METHODDEF
    _OPERATOR_RSHIFT_METHODDEF
    _OPERATOR_NOT__METHODDEF
    _OPERATOR_AND__METHODDEF
    _OPERATOR_XOR_METHODDEF
    _OPERATOR_OR__METHODDEF
    _OPERATOR_IADD_METHODDEF
    _OPERATOR_ISUB_METHODDEF
    _OPERATOR_IMUL_METHODDEF
    _OPERATOR_IMATMUL_METHODDEF
    _OPERATOR_IFLOORDIV_METHODDEF
    _OPERATOR_ITRUEDIV_METHODDEF
    _OPERATOR_IMOD_METHODDEF
    _OPERATOR_ILSHIFT_METHODDEF
    _OPERATOR_IRSHIFT_METHODDEF
    _OPERATOR_IAND_METHODDEF
    _OPERATOR_IXOR_METHODDEF
    _OPERATOR_IOR_METHODDEF
    _OPERATOR_CONCAT_METHODDEF
    _OPERATOR_ICONCAT_METHODDEF
    _OPERATOR_GETITEM_METHODDEF
    _OPERATOR_SETITEM_METHODDEF
    _OPERATOR_DELITEM_METHODDEF
    _OPERATOR_POW_METHODDEF
    _OPERATOR_IPOW_METHODDEF
    _OPERATOR_EQ_METHODDEF
    _OPERATOR_NE_METHODDEF
    _OPERATOR_LT_METHODDEF
    _OPERATOR_LE_METHODDEF
    _OPERATOR_GT_METHODDEF
    _OPERATOR_GE_METHODDEF
    _OPERATOR__COMPARE_DIGEST_METHODDEF
    _OPERATOR_LENGTH_HINT_METHODDEF
    _OPERATOR_CALL_METHODDEF
    {NULL,              NULL}           /* sentinel */

};


static PyObject *
text_signature(PyObject *Py_UNUSED(self), void *Py_UNUSED(closure))
{
    return PyUnicode_FromString("(obj, /)");
}

static PyGetSetDef common_getset[] = {
    {"__text_signature__", text_signature, NULL},
    {NULL}
};

/* itemgetter object **********************************************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t nitems;
    PyObject *item;
    Py_ssize_t index; // -1 unless *item* is a single non-negative integer index
    vectorcallfunc vectorcall;
} itemgetterobject;

#define itemgetterobject_CAST(op)   ((itemgetterobject *)(op))

// Forward declarations
static PyObject *
itemgetter_vectorcall(PyObject *, PyObject *const *, size_t, PyObject *);
static PyObject *
itemgetter_call_impl(itemgetterobject *, PyObject *);

/* AC 3.5: treats first argument as an iterable, otherwise uses *args */
static PyObject *
itemgetter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *type_owner = Py_NewRef((PyObject *)type);
    PyObject *args_owner = Py_NewRef(args);
    PyObject *kwds_owner = Py_XNewRef(kwds);
    PyObject *item = NULL;
    PyObject *result = NULL;
    PyObject *saved;
    itemgetterobject *ig = NULL;
    Py_ssize_t nitems = PyTuple_GET_SIZE(args_owner);
    Py_ssize_t index = -1;
    if (PyErr_Occurred()) goto done;
    if (!_PyArg_NoKeywords("itemgetter", kwds_owner)) goto done;
    if (nitems <= 1) {
        if (!_PyArg_CheckPositional("itemgetter", nitems, 1, 1)) goto done;
        item = PyTuple_GetItemRef(args_owner, 0);
    } else {
        item = Py_NewRef(args_owner);
    }
    if (item == NULL || PyErr_Occurred()) goto done;
    _operator_state *state = _PyType_GetModuleState((PyTypeObject *)type_owner);
    ig = PyObject_GC_New(itemgetterobject, (PyTypeObject *)state->itemgetter_type);
    if (ig == NULL) goto done;
    ig->item = NULL;
    ig->nitems = 0;
    ig->index = -1;
    ig->vectorcall = NULL;
    if (PyErr_Occurred()) goto done;
    ig->item = item; item = NULL;
    ig->nitems = nitems;
    ig->vectorcall = itemgetter_vectorcall;
    if (PyLong_CheckExact(ig->item)) {
        index = PyLong_AsSsize_t(ig->item);
        if (index < 0) PyErr_Clear();
        else ig->index = index;
    }
    PyObject_GC_Track(ig);
    result = (PyObject *)ig; ig = NULL;
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = item;
        item = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = args_owner;
        args_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = kwds_owner;
        kwds_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (ig != NULL) {
        saved=PyErr_GetRaisedException();
        itemgetterobject *retired_ig=ig; ig=NULL;
        Py_DECREF(retired_ig);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = type_owner;
        type_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static int
itemgetter_clear(PyObject *op)
{
    itemgetterobject *ig = itemgetterobject_CAST(op);
    Py_CLEAR(ig->item);
    return 0;
}

static void
itemgetter_dealloc(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    (void)itemgetter_clear(op);
    tp->tp_free(op);
    Py_DECREF(tp);
}

static int
itemgetter_traverse(PyObject *op, visitproc visit, void *arg)
{
    itemgetterobject *ig = itemgetterobject_CAST(op);
    Py_VISIT(Py_TYPE(ig));
    Py_VISIT(ig->item);
    return 0;
}

static PyObject *
itemgetter_call(PyObject *op, PyObject *args, PyObject *kw)
{
    PyObject *op_owner = Py_NewRef(op), *args_owner = Py_NewRef(args);
    PyObject *selected = NULL, *result = NULL, *saved;
    if (PyErr_Occurred()) goto done;
    assert(PyTuple_CheckExact(args_owner));
    if (!_PyArg_NoKeywords("itemgetter", kw) ||
        !_PyArg_CheckPositional("itemgetter", PyTuple_GET_SIZE(args_owner), 1, 1)) goto done;
    selected = PyTuple_GetItemRef(args_owner, 0);
    if (selected == NULL || PyErr_Occurred()) goto done;
    result = itemgetter_call_impl(itemgetterobject_CAST(op_owner), selected);
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
    }
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = selected;
        selected = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = args_owner;
        args_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = op_owner;
        op_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
itemgetter_vectorcall(PyObject *op, PyObject *const *args,
                      size_t nargsf, PyObject *kwnames)
{
    if (!_PyArg_NoKwnames("itemgetter", kwnames)) {
        return NULL;
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("itemgetter", nargs, 1, 1)) {
        return NULL;
    }
    itemgetterobject *ig = itemgetterobject_CAST(op);
    return itemgetter_call_impl(ig, args[0]);
}

static PyObject *
itemgetter_call_impl(itemgetterobject *ig, PyObject *obj)
{
    PyObject *ig_owner=Py_NewRef((PyObject *)ig), *obj_owner=Py_NewRef(obj);
    PyObject *result=NULL, *saved;
    Py_ssize_t i, nitems=ig->nitems;
    if (PyErr_Occurred()) goto done;
    if (nitems == 1) {
        if (ig->index >= 0 && PyTuple_CheckExact(obj_owner) && ig->index < PyTuple_GET_SIZE(obj_owner))
            result=PyTuple_GetItemRef(obj_owner, ig->index);
        else
            result=PyObject_GetItem(obj_owner, ig->item);
        goto done;
    }
    assert(PyTuple_Check(ig->item));
    assert(PyTuple_GET_SIZE(ig->item) == nitems);
    result=PyTuple_New(nitems);
    if (result==NULL || PyErr_Occurred()) goto done;
    for (i=0; i<nitems; i++) {
        PyObject *item=PyTuple_GetItemRef(ig->item, i), *value=NULL;
        if (item != NULL && !PyErr_Occurred()) value=PyObject_GetItem(obj_owner,item);
        {
            saved = PyErr_GetRaisedException();
            PyObject *retired = item;
            item = NULL;
            Py_XDECREF(retired);
            _PyValue_RestoreException(saved);
        }
        if (value==NULL || PyErr_Occurred()) {
            {
                saved = PyErr_GetRaisedException();
                PyObject *retired = value;
                value = NULL;
                Py_XDECREF(retired);
                _PyValue_RestoreException(saved);
            } goto done;
        }
        if (PyTuple_SetItem(result,i,value)<0 || PyErr_Occurred()) goto done;
    }
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = obj_owner;
        obj_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = ig_owner;
        ig_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
itemgetter_repr(PyObject *op)
{
    PyObject *repr;
    const char *reprfmt;
    itemgetterobject *ig = itemgetterobject_CAST(op);

    int status = Py_ReprEnter(op);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(ig)->tp_name);
    }

    reprfmt = ig->nitems == 1 ? "%s(%R)" : "%s%R";
    repr = PyUnicode_FromFormat(reprfmt, Py_TYPE(ig)->tp_name, ig->item);
    Py_ReprLeave(op);
    return repr;
}

static PyObject *
itemgetter_reduce(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    itemgetterobject *ig = itemgetterobject_CAST(op);
    if (ig->nitems == 1)
        return Py_BuildValue("O(O)", Py_TYPE(ig), ig->item);
    return _PyTuple_FromPair((PyObject *)Py_TYPE(ig), ig->item);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling");

static PyMethodDef itemgetter_methods[] = {
    {"__reduce__", itemgetter_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};

static PyMemberDef itemgetter_members[] = {
    {"__vectorcalloffset__", Py_T_PYSSIZET, offsetof(itemgetterobject, vectorcall), Py_READONLY},
    {NULL} /* Sentinel */
};

PyDoc_STRVAR(itemgetter_doc,
"itemgetter(item, /, *items)\n--\n\n\
Return a callable object that fetches the given item(s) from its operand.\n\
After f = itemgetter(2), the call f(r) returns r[2].\n\
After g = itemgetter(2, 5, 3), the call g(r) returns (r[2], r[5], r[3])");

static PyType_Slot itemgetter_type_slots[] = {
    {Py_tp_doc, (void *)itemgetter_doc},
    {Py_tp_dealloc, itemgetter_dealloc},
    {Py_tp_call, itemgetter_call},
    {Py_tp_traverse, itemgetter_traverse},
    {Py_tp_clear, itemgetter_clear},
    {Py_tp_methods, itemgetter_methods},
    {Py_tp_members, itemgetter_members},
    {Py_tp_getset, common_getset},
    {Py_tp_new, itemgetter_new},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_repr, itemgetter_repr},
    {0, 0}
};

static PyType_Spec itemgetter_type_spec = {
    .name = "operator.itemgetter",
    .basicsize = sizeof(itemgetterobject),
    .itemsize = 0,
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_VECTORCALL),
    .slots = itemgetter_type_slots,
};

/* attrgetter object **********************************************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t nattrs;
    PyObject *attr;
    vectorcallfunc vectorcall;
} attrgetterobject;

#define attrgetterobject_CAST(op)   ((attrgetterobject *)(op))

// Forward declarations
static PyObject *
attrgetter_vectorcall(PyObject *, PyObject *const *, size_t, PyObject *);
static PyObject *
attrgetter_call_impl(attrgetterobject *, PyObject *);

/* AC 3.5: treats first argument as an iterable, otherwise uses *args */
static PyObject *
attrgetter_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *type_owner=Py_NewRef((PyObject *)type), *args_owner=Py_NewRef(args);
    PyObject *kwds_owner=Py_XNewRef(kwds), *attr=NULL, *item=NULL;
    PyObject *chain=NULL, *chain_item=NULL, *result=NULL, *saved;
    attrgetterobject *ag=NULL;
    Py_ssize_t nattrs, idx, char_idx;
    PyInterpreterState *interp=_PyInterpreterState_GET();
    if (PyErr_Occurred()) goto error;
    if (!_PyArg_NoKeywords("attrgetter",kwds_owner) || PyErr_Occurred()) goto error;
    nattrs=PyTuple_GET_SIZE(args_owner);
    if (nattrs<=1 && !_PyArg_CheckPositional("attrgetter",nattrs,1,1)) goto error;
    attr=PyTuple_New(nattrs);
    if (attr==NULL || PyErr_Occurred()) goto error;
    for (idx=0; idx<nattrs; ++idx) {
        PyValue candidate=PyTuple_GET_VALUE(args_owner,idx);
        if (!PyValue_IsObject(candidate) ||
            !PyUnicode_Check(PyValue_AsPyObjectBorrow(candidate))) {
            PyErr_SetString(PyExc_TypeError,"attribute name must be a string");
            goto error;
        }
        item=PyTuple_GetItemRef(args_owner,idx);
        if (item==NULL || PyErr_Occurred()) goto error;
        int dot_count=0;
        Py_ssize_t item_len=PyUnicode_GET_LENGTH(item);
        int kind=PyUnicode_KIND(item);
        const void *data=PyUnicode_DATA(item);
        for (char_idx=0;char_idx<item_len;++char_idx)
            if (PyUnicode_READ(kind,data,char_idx)=='.') ++dot_count;
        if (dot_count==0) {
            chain_item=Py_NewRef(item);
            if (PyErr_Occurred()) goto error;
            _PyUnicode_InternMortal(interp,&chain_item);
            if (PyErr_Occurred()) goto error;
            if (PyTuple_SetItem(attr,idx,chain_item)<0 || PyErr_Occurred()) { chain_item=NULL; goto error; }
            chain_item=NULL;
        } else {
            chain=PyTuple_New(dot_count+1);
            if (chain==NULL || PyErr_Occurred()) goto error;
            Py_ssize_t from=0, till=0, chain_idx=0;
            while (dot_count>0) {
                while (PyUnicode_READ(kind,data,till)!='.') ++till;
                chain_item=PyUnicode_Substring(item,from,till);
                if (chain_item==NULL || PyErr_Occurred()) goto error;
                _PyUnicode_InternMortal(interp,&chain_item);
                if (PyErr_Occurred()) goto error;
                if (PyTuple_SetItem(chain,chain_idx++,chain_item)<0 || PyErr_Occurred()) { chain_item=NULL; goto error; }
                chain_item=NULL;
                --dot_count;
                till=from=till+1;
            }
            chain_item=PyUnicode_Substring(item,from,item_len);
            if (chain_item==NULL || PyErr_Occurred()) goto error;
            _PyUnicode_InternMortal(interp,&chain_item);
            if (PyErr_Occurred()) goto error;
            if (PyTuple_SetItem(chain,chain_idx,chain_item)<0 || PyErr_Occurred()) { chain_item=NULL; goto error; }
            chain_item=NULL;
            if (PyTuple_SetItem(attr,idx,chain)<0 || PyErr_Occurred()) { chain=NULL; goto error; }
            chain=NULL;
        }
        {
            saved = PyErr_GetRaisedException();
            PyObject *retired = item;
            item = NULL;
            Py_XDECREF(retired);
            _PyValue_RestoreException(saved);
        }
        if (PyErr_Occurred()) goto error;
    }
    _operator_state *state=_PyType_GetModuleState((PyTypeObject *)type_owner);
    ag=PyObject_GC_New(attrgetterobject,(PyTypeObject *)state->attrgetter_type);
    if (ag==NULL) goto error;
    ag->attr=NULL; ag->nattrs=0; ag->vectorcall=NULL;
    if (PyErr_Occurred()) goto error;
    ag->attr=attr; attr=NULL;
    ag->nattrs=nattrs;
    ag->vectorcall=attrgetter_vectorcall;
    PyObject_GC_Track(ag);
    result=(PyObject *)ag; ag=NULL;
    goto close_inputs;
error:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = chain_item;
        chain_item = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = chain;
        chain = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = item;
        item = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = attr;
        attr = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (ag!=NULL) {
        saved=PyErr_GetRaisedException();
        attrgetterobject *retired_ag=ag; ag=NULL;
        Py_DECREF(retired_ag);
        _PyValue_RestoreException(saved);
    }
close_inputs:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = args_owner;
        args_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = kwds_owner;
        kwds_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = type_owner;
        type_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static int
attrgetter_clear(PyObject *op)
{
    attrgetterobject *ag = attrgetterobject_CAST(op);
    Py_CLEAR(ag->attr);
    return 0;
}

static void
attrgetter_dealloc(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    (void)attrgetter_clear(op);
    tp->tp_free(op);
    Py_DECREF(tp);
}

static int
attrgetter_traverse(PyObject *op, visitproc visit, void *arg)
{
    attrgetterobject *ag = attrgetterobject_CAST(op);
    Py_VISIT(ag->attr);
    Py_VISIT(Py_TYPE(ag));
    return 0;
}

static PyObject *
dotted_getattr(PyObject *obj, PyObject *attr)
{
    PyObject *current=Py_NewRef(obj), *attr_owner=Py_NewRef(attr);
    PyObject *result=NULL, *saved;
    if (PyErr_Occurred()) goto done;
    if (PyTuple_CheckExact(attr_owner)) {
        Py_ssize_t count=PyTuple_GET_SIZE(attr_owner);
        for (Py_ssize_t i=0; i<count; i++) {
            PyObject *name=PyTuple_GetItemRef(attr_owner,i);
            PyObject *next=NULL;
            if (name != NULL && !PyErr_Occurred()) next=PyObject_GetAttr(current,name);
            {
                saved = PyErr_GetRaisedException();
                PyObject *retired = name;
                name = NULL;
                Py_XDECREF(retired);
                _PyValue_RestoreException(saved);
            }
            if (next==NULL || PyErr_Occurred()) {
                {
                    saved = PyErr_GetRaisedException();
                    PyObject *retired = next;
                    next = NULL;
                    Py_XDECREF(retired);
                    _PyValue_RestoreException(saved);
                }
                goto done;
            }
            {
                saved=PyErr_GetRaisedException();
                PyObject *retired_current=current; current=NULL;
                Py_DECREF(retired_current);
                _PyValue_RestoreException(saved);
            }
            if (PyErr_Occurred()) {
                saved=PyErr_GetRaisedException();
                PyObject *retired_next=next; next=NULL;
                Py_XDECREF(retired_next);
                _PyValue_RestoreException(saved);
                goto done;
            }
            current=next;
        }
        result=current; current=NULL;
    } else {
        result=PyObject_GetAttr(current,attr_owner);
    }
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = current;
        current = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = attr_owner;
        attr_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
attrgetter_call(PyObject *op, PyObject *args, PyObject *kw)
{
    PyObject *op_owner=Py_NewRef(op), *args_owner=Py_NewRef(args);
    PyObject *selected=NULL, *result=NULL, *saved;
    if (PyErr_Occurred()) goto done;
    if (!_PyArg_NoKeywords("attrgetter",kw) ||
        !_PyArg_CheckPositional("attrgetter",PyTuple_GET_SIZE(args_owner),1,1)) goto done;
    selected=PyTuple_GetItemRef(args_owner,0);
    if (selected==NULL || PyErr_Occurred()) goto done;
    result=attrgetter_call_impl(attrgetterobject_CAST(op_owner),selected);
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
    }
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = selected;
        selected = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = args_owner;
        args_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = op_owner;
        op_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
attrgetter_vectorcall(PyObject *op, PyObject *const *args,
                      size_t nargsf, PyObject *kwnames)
{
    if (!_PyArg_NoKwnames("attrgetter", kwnames)) {
        return NULL;
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("attrgetter", nargs, 1, 1)) {
        return NULL;
    }
    attrgetterobject *ag = attrgetterobject_CAST(op);
    return attrgetter_call_impl(ag, args[0]);
}

static PyObject *
attrgetter_call_impl(attrgetterobject *ag, PyObject *obj)
{
    PyObject *result=NULL, *ag_owner=Py_NewRef((PyObject *)ag), *obj_owner=Py_NewRef(obj), *saved;
    PyObject *attrs=Py_NewRef(ag->attr);
    Py_ssize_t nattrs=ag->nattrs;
    if (PyErr_Occurred()) goto done;
    if (nattrs==1) {
        PyObject *attr=PyTuple_GetItemRef(attrs,0);
        if (attr != NULL && !PyErr_Occurred()) result=dotted_getattr(obj_owner,attr);
        {
            saved = PyErr_GetRaisedException();
            PyObject *retired = attr;
            attr = NULL;
            Py_XDECREF(retired);
            _PyValue_RestoreException(saved);
        }
        goto done;
    }
    assert(PyTuple_Check(attrs));
    assert(PyTuple_GET_SIZE(attrs)==nattrs);
    result=PyTuple_New(nattrs);
    if (result==NULL || PyErr_Occurred()) goto done;
    for (Py_ssize_t i=0;i<nattrs;i++) {
        PyObject *attr=PyTuple_GetItemRef(attrs,i), *val=NULL;
        if (attr != NULL && !PyErr_Occurred()) val=dotted_getattr(obj_owner,attr);
        {
            saved = PyErr_GetRaisedException();
            PyObject *retired = attr;
            attr = NULL;
            Py_XDECREF(retired);
            _PyValue_RestoreException(saved);
        }
        if (val==NULL || PyErr_Occurred()) {
            {
                saved = PyErr_GetRaisedException();
                PyObject *retired = val;
                val = NULL;
                Py_XDECREF(retired);
                _PyValue_RestoreException(saved);
            } goto done;
        }
        if (PyTuple_SetItem(result,i,val)<0 || PyErr_Occurred()) goto done;
    }
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = attrs;
        attrs = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = obj_owner;
        obj_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = ag_owner;
        ag_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
dotjoinattr(PyObject *attr, PyObject **attrsep)
{
    if (PyTuple_CheckExact(attr)) {
        if (*attrsep == NULL) {
            *attrsep = PyUnicode_FromString(".");
            if (*attrsep == NULL)
                return NULL;
        }
        return PyUnicode_Join(*attrsep, attr);
    } else {
        return Py_NewRef(attr);
    }
}

static PyObject *
attrgetter_args(attrgetterobject *ag)
{
    PyObject *ag_owner=Py_NewRef((PyObject *)ag), *attrs=Py_NewRef(ag->attr);
    PyObject *attrsep=NULL, *attrstrings=NULL, *saved;
    if (PyErr_Occurred()) goto done;
    attrstrings=PyTuple_New(ag->nattrs);
    if (attrstrings==NULL || PyErr_Occurred()) goto done;
    for (Py_ssize_t i=0;i<ag->nattrs;i++) {
        PyObject *attr=PyTuple_GetItemRef(attrs,i), *attrstr=NULL;
        if (attr!=NULL && !PyErr_Occurred()) attrstr=dotjoinattr(attr,&attrsep);
        {
            saved = PyErr_GetRaisedException();
            PyObject *retired = attr;
            attr = NULL;
            Py_XDECREF(retired);
            _PyValue_RestoreException(saved);
        }
        if (attrstr==NULL || PyErr_Occurred()) {
            {
                saved = PyErr_GetRaisedException();
                PyObject *retired = attrstr;
                attrstr = NULL;
                Py_XDECREF(retired);
                _PyValue_RestoreException(saved);
            } goto done;
        }
        if (PyTuple_SetItem(attrstrings,i,attrstr)<0 || PyErr_Occurred()) goto done;
    }
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = attrsep;
        attrsep = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = attrs;
        attrs = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = ag_owner;
        ag_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = attrstrings;
        attrstrings = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return attrstrings;
}

static PyObject *
attrgetter_repr(PyObject *op)
{
    PyObject *op_owner=Py_NewRef(op), *attrs=NULL, *attrsep=NULL, *attr=NULL;
    PyObject *attrstrings=NULL, *repr=NULL, *saved;
    attrgetterobject *ag=attrgetterobject_CAST(op_owner);
    if (PyErr_Occurred()) goto done;
    int status=Py_ReprEnter(op_owner);
    if (status<0) {
        saved=PyErr_GetRaisedException();
        PyObject *retired_op=op_owner; op_owner=NULL;
        Py_DECREF(retired_op);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    if (status>0) {
        repr=PyUnicode_FromFormat("%s(...)",Py_TYPE(ag)->tp_name);
        goto done;
    }
    attrs=Py_NewRef(ag->attr);
    if (PyErr_Occurred()) goto leave;
    if (ag->nattrs==1) {
        attr=PyTuple_GetItemRef(attrs,0);
        if (attr!=NULL && !PyErr_Occurred()) attrstrings=dotjoinattr(attr,&attrsep);
        {
            saved = PyErr_GetRaisedException();
            PyObject *retired = attr;
            attr = NULL;
            Py_XDECREF(retired);
            _PyValue_RestoreException(saved);
        }
        if (attrstrings!=NULL && !PyErr_Occurred()) {
            repr=PyUnicode_FromFormat("%s(%R)",Py_TYPE(ag)->tp_name,attrstrings);
        }
    } else {
        attrstrings=attrgetter_args(ag);
        if (attrstrings!=NULL && !PyErr_Occurred())
            repr=PyUnicode_FromFormat("%s%R",Py_TYPE(ag)->tp_name,attrstrings);
    }
leave:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = attrstrings;
        attrstrings = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = attrsep;
        attrsep = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = attrs;
        attrs = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    Py_ReprLeave(op_owner);
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = op_owner;
        op_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = repr;
        repr = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return repr;
}

static PyObject *
attrgetter_reduce(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    attrgetterobject *ag = attrgetterobject_CAST(op);
    PyObject *attrstrings = attrgetter_args(ag);
    if (attrstrings == NULL)
        return NULL;
    return Py_BuildValue("ON", Py_TYPE(ag), attrstrings);
}

static PyMethodDef attrgetter_methods[] = {
    {"__reduce__", attrgetter_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};

static PyMemberDef attrgetter_members[] = {
    {"__vectorcalloffset__", Py_T_PYSSIZET, offsetof(attrgetterobject, vectorcall), Py_READONLY},
    {NULL} /* Sentinel*/
};

PyDoc_STRVAR(attrgetter_doc,
"attrgetter(attr, /, *attrs)\n--\n\n\
Return a callable object that fetches the given attribute(s) from its operand.\n\
After f = attrgetter('name'), the call f(r) returns r.name.\n\
After g = attrgetter('name', 'date'), the call g(r) returns (r.name, r.date).\n\
After h = attrgetter('name.first', 'name.last'), the call h(r) returns\n\
(r.name.first, r.name.last).");

static PyType_Slot attrgetter_type_slots[] = {
    {Py_tp_doc, (void *)attrgetter_doc},
    {Py_tp_dealloc, attrgetter_dealloc},
    {Py_tp_call, attrgetter_call},
    {Py_tp_traverse, attrgetter_traverse},
    {Py_tp_clear, attrgetter_clear},
    {Py_tp_methods, attrgetter_methods},
    {Py_tp_members, attrgetter_members},
    {Py_tp_getset, common_getset},
    {Py_tp_new, attrgetter_new},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_repr, attrgetter_repr},
    {0, 0}
};

static PyType_Spec attrgetter_type_spec = {
    .name = "operator.attrgetter",
    .basicsize = sizeof(attrgetterobject),
    .itemsize = 0,
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_VECTORCALL),
    .slots = attrgetter_type_slots,
};


/* methodcaller object **********************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *args;
    PyObject *kwds;
    PyObject *vectorcall_args;
    PyObject *vectorcall_kwnames;
    vectorcallfunc vectorcall;
} methodcallerobject;

#define methodcallerobject_CAST(op) ((methodcallerobject *)(op))

#define _METHODCALLER_MAX_ARGS 8

static PyObject *
methodcaller_vectorcall(PyObject *op, PyObject *const *args,
                        size_t nargsf, PyObject* kwnames)
{
    methodcallerobject *mc;
    PyObject *op_owner=Py_NewRef(op), *kwnames_owner=Py_XNewRef(kwnames);
    PyObject *target=NULL, *name=NULL, *posargs=NULL, *combined=NULL, *names=NULL;
    PyObject *result=NULL, *saved;
    int scope_status=0;
    PyObject *tmp_args[_METHODCALLER_MAX_ARGS];
    _PyArg_OwnerScope scope={0};
    bool scope_started=false;
    PyObject *const *owned=NULL;
    Py_ssize_t nargs=PyVectorcall_NARGS(nargsf), nall, npos, nkw;
    if (PyErr_Occurred()) goto done;
    if (!_PyArg_CheckPositional("methodcaller",nargs,1,1) ||
        !_PyArg_NoKwnames("methodcaller",kwnames_owner)) goto done;
    target=Py_NewRef(args[0]);
    if (PyErr_Occurred()) goto done;
    mc=methodcallerobject_CAST(op_owner);
    name=Py_XNewRef(mc->name); posargs=Py_XNewRef(mc->args);
    combined=Py_XNewRef(mc->vectorcall_args); names=Py_XNewRef(mc->vectorcall_kwnames);
    if (PyErr_Occurred()) goto done;
    if (posargs==NULL || combined==NULL ||
        !PyTuple_Check(posargs) || !PyTuple_Check(combined) ||
        (names!=NULL && !PyTuple_Check(names))) {
        PyErr_SetString(PyExc_RuntimeError,"methodcaller argument layout changed");
        goto done;
    }
    nall=PyTuple_GET_SIZE(combined); npos=PyTuple_GET_SIZE(posargs);
    nkw=names ? PyTuple_GET_SIZE(names) : 0;
    if (nall > _METHODCALLER_MAX_ARGS-1 || npos > nall || nkw != nall-npos) {
        PyErr_SetString(PyExc_RuntimeError,"methodcaller argument layout changed");
        goto done;
    }
    assert(1 + nall <= _METHODCALLER_MAX_ARGS);
    for (Py_ssize_t i=0;i<nkw;i++) {
        PyValue key=PyTuple_GET_VALUE(names,i);
        if (!PyValue_IsObject(key) ||
            !PyUnicode_Check(PyValue_AsPyObjectBorrow(key))) {
            PyErr_SetString(PyExc_TypeError,"keywords must be strings");
            goto done;
        }
    }
    if (_PyArg_OwnerScope_BeginTuple(&scope,combined,0)<0 || PyErr_Occurred()) {
        scope_started=true;
        goto done;
    }
    scope_started=true;
    owned=_PyArg_UnpackPositionalOwned(&scope,NULL,nall,nall);
    if (owned==NULL || PyErr_Occurred()) goto done;
    tmp_args[0]=target;
    for (Py_ssize_t i=0;i<nall;i++) tmp_args[i+1]=owned[i];
    result=PyObject_VectorcallMethod(name,tmp_args,
            (1+npos)|PY_VECTORCALL_ARGUMENTS_OFFSET,names);
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = names;
        names = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = combined;
        combined = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = posargs;
        posargs = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = name;
        name = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = target;
        target = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = kwnames_owner;
        kwnames_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = op_owner;
        op_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (scope_started) {
        scope_status = _PyArg_OwnerScope_End(&scope);
    }
    if (scope_status < 0 || PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static int
_methodcaller_initialize_vectorcall(methodcallerobject* mc)
{
    PyObject* args = mc->args;
    PyObject* kwds = mc->kwds;

    if (kwds && PyDict_Size(kwds)) {
        PyObject *values = PyDict_Values(kwds);
        if (!values) {
            return -1;
        }
        PyObject *values_tuple = PySequence_Tuple(values);
        Py_DECREF(values);
        if (!values_tuple) {
            return -1;
        }
        if (PyTuple_GET_SIZE(args)) {
            mc->vectorcall_args = PySequence_Concat(args, values_tuple);
            Py_DECREF(values_tuple);
            if (mc->vectorcall_args == NULL) {
                return -1;
            }
        }
        else {
            mc->vectorcall_args = values_tuple;
        }
        mc->vectorcall_kwnames = PySequence_Tuple(kwds);
        if (!mc->vectorcall_kwnames) {
            return -1;
        }
    }
    else {
        mc->vectorcall_args = Py_NewRef(args);
        mc->vectorcall_kwnames = NULL;
    }

    mc->vectorcall = methodcaller_vectorcall;
    return 0;
}

/* AC 3.5: variable number of arguments, not currently support by AC */
static PyObject *
methodcaller_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *type_owner=Py_NewRef((PyObject *)type), *args_owner=Py_NewRef(args);
    PyObject *kwds_owner=Py_XNewRef(kwds), *name=NULL, *interned=NULL, *result=NULL, *saved;
    methodcallerobject *mc=NULL;
    Py_ssize_t nitems=PyTuple_GET_SIZE(args_owner);
    if (PyErr_Occurred()) goto error;
    if (nitems<1) {
        PyErr_SetString(PyExc_TypeError,"methodcaller needs at least one argument, the method name");
        goto error;
    }
    PyValue candidate=PyTuple_GET_VALUE(args_owner,0);
    if (!PyValue_IsObject(candidate) ||
        !PyUnicode_Check(PyValue_AsPyObjectBorrow(candidate))) {
        PyErr_SetString(PyExc_TypeError,"method name must be a string");
        goto error;
    }
    name=PyTuple_GetItemRef(args_owner,0);
    if (name==NULL || PyErr_Occurred()) goto error;
    _operator_state *state=_PyType_GetModuleState((PyTypeObject *)type_owner);
    mc=PyObject_GC_New(methodcallerobject,(PyTypeObject *)state->methodcaller_type);
    if (mc==NULL) goto error;
    mc->name=NULL;
    mc->args=NULL;
    mc->kwds=NULL;
    mc->vectorcall_args=NULL;
    mc->vectorcall_kwnames=NULL;
    mc->vectorcall=NULL;
    if (PyErr_Occurred()) goto error;
    mc->kwds=Py_XNewRef(kwds_owner);
    if (PyErr_Occurred()) goto error;
    PyInterpreterState *interp=_PyInterpreterState_GET();
    interned=Py_NewRef(name);
    if (PyErr_Occurred()) goto error;
    _PyUnicode_InternMortal(interp,&interned);
    if (interned==NULL || PyErr_Occurred()) goto error;
    mc->name=interned;
    interned=NULL;
    mc->args=PyTuple_GetSlice(args_owner,1,nitems);
    if (mc->args==NULL || PyErr_Occurred()) goto error;
    Py_ssize_t vectorcall_size=nitems+(kwds_owner ? PyDict_Size(kwds_owner) : 0);
    if (PyErr_Occurred()) goto error;
    if (vectorcall_size<_METHODCALLER_MAX_ARGS) {
        if (_methodcaller_initialize_vectorcall(mc)<0 || PyErr_Occurred()) goto error;
    }
    PyObject_GC_Track(mc);
    result=(PyObject *)mc; mc=NULL;
    goto close_inputs;
error:
    if (mc!=NULL) {
        saved=PyErr_GetRaisedException(); Py_CLEAR(mc->vectorcall_kwnames); _PyValue_RestoreException(saved);
        saved=PyErr_GetRaisedException(); Py_CLEAR(mc->vectorcall_args); _PyValue_RestoreException(saved);
        saved=PyErr_GetRaisedException(); Py_CLEAR(mc->kwds); _PyValue_RestoreException(saved);
        saved=PyErr_GetRaisedException(); Py_CLEAR(mc->args); _PyValue_RestoreException(saved);
        saved=PyErr_GetRaisedException(); Py_CLEAR(mc->name); _PyValue_RestoreException(saved);
        saved=PyErr_GetRaisedException();
        methodcallerobject *retired_mc=mc; mc=NULL;
        Py_DECREF(retired_mc);
        _PyValue_RestoreException(saved);
    }
close_inputs:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = interned;
        interned = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = name;
        name = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = args_owner;
        args_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = kwds_owner;
        kwds_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = type_owner;
        type_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static int
methodcaller_clear(PyObject *op)
{
    methodcallerobject *mc = methodcallerobject_CAST(op);
    Py_CLEAR(mc->name);
    Py_CLEAR(mc->args);
    Py_CLEAR(mc->kwds);
    Py_CLEAR(mc->vectorcall_args);
    Py_CLEAR(mc->vectorcall_kwnames);
    return 0;
}

static void
methodcaller_dealloc(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(op);
    (void)methodcaller_clear(op);
    tp->tp_free(op);
    Py_DECREF(tp);
}

static int
methodcaller_traverse(PyObject *op, visitproc visit, void *arg)
{
    methodcallerobject *mc = methodcallerobject_CAST(op);
    Py_VISIT(mc->name);
    Py_VISIT(mc->args);
    Py_VISIT(mc->kwds);
    Py_VISIT(mc->vectorcall_args);
    Py_VISIT(mc->vectorcall_kwnames);
    Py_VISIT(Py_TYPE(mc));
    return 0;
}

static PyObject *
methodcaller_call(PyObject *op, PyObject *args, PyObject *kw)
{
    PyObject *op_owner=Py_NewRef(op), *args_owner=Py_NewRef(args);
    PyObject *obj=NULL, *name=NULL, *callargs=NULL, *callkw=NULL;
    PyObject *method=NULL, *result=NULL, *saved;
    if (PyErr_Occurred()) goto done;
    if (!_PyArg_NoKeywords("methodcaller",kw) ||
        !_PyArg_CheckPositional("methodcaller",PyTuple_GET_SIZE(args_owner),1,1)) goto done;
    obj=PyTuple_GetItemRef(args_owner,0);
    methodcallerobject *mc=methodcallerobject_CAST(op_owner);
    name=Py_XNewRef(mc->name); callargs=Py_XNewRef(mc->args); callkw=Py_XNewRef(mc->kwds);
    if (obj==NULL || PyErr_Occurred()) goto done;
    method=PyObject_GetAttr(obj,name);
    if (method!=NULL && !PyErr_Occurred()) result=PyObject_Call(method,callargs,callkw);
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = method;
        method = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = callkw;
        callkw = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = callargs;
        callargs = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = name;
        name = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = obj;
        obj = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = args_owner;
        args_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = op_owner;
        op_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyObject *
methodcaller_repr(PyObject *op)
{
    PyObject *op_owner=Py_NewRef(op), *name=NULL, *args=NULL, *kwds=NULL;
    PyObject *argreprs=NULL, *one=NULL, *sep=NULL, *joined=NULL, *repr=NULL;
    PyObject *key=NULL, *value=NULL, *saved;
    Py_ssize_t npos=0, nkw=0, total=0, i;
    if (PyErr_Occurred()) goto finish;
    int status=Py_ReprEnter(op_owner);
    if (status<0) {
        {
            saved = PyErr_GetRaisedException();
            PyObject *retired = op_owner;
            op_owner = NULL;
            Py_XDECREF(retired);
            _PyValue_RestoreException(saved);
        }
        return NULL;
    }
    if (status>0) {
        repr=PyUnicode_FromFormat("%s(...)",Py_TYPE(op_owner)->tp_name);
        goto finish;
    }
    methodcallerobject *mc=methodcallerobject_CAST(op_owner);
    name=Py_XNewRef(mc->name); args=Py_XNewRef(mc->args); kwds=Py_XNewRef(mc->kwds);
    if (PyErr_Occurred()) goto cleanup;
    npos=PyTuple_GET_SIZE(args); nkw=kwds ? PyDict_GET_SIZE(kwds) : 0; total=npos+nkw;
    if (total==0) {
        repr=PyUnicode_FromFormat("%s(%R)",Py_TYPE(op_owner)->tp_name,name);
        goto cleanup;
    }
    argreprs=PyTuple_New(total);
    if (argreprs==NULL || PyErr_Occurred()) goto cleanup;
    for (i=0;i<npos;i++) {
        PyObject *item=PyTuple_GetItemRef(args,i);
        if (item!=NULL && !PyErr_Occurred()) one=PyObject_Repr(item);
        {
            saved = PyErr_GetRaisedException();
            PyObject *retired = item;
            item = NULL;
            Py_XDECREF(retired);
            _PyValue_RestoreException(saved);
        }
        if (one==NULL || PyErr_Occurred()) goto cleanup;
        if (PyTuple_SetItem(argreprs,i,one)<0 || PyErr_Occurred()) { one=NULL; goto cleanup; }
        one=NULL;
    }
    if (nkw) {
        Py_ssize_t pos=0;
        int iter_status;
        while ((iter_status=PyDict_NextEntryRef(kwds,&pos,&key,&value))>0) {
            if (PyErr_Occurred()) goto cleanup;
            if (!PyUnicode_Check(key)) {
                PyErr_SetString(PyExc_TypeError,"keywords must be strings");
                goto cleanup;
            }
            one=PyUnicode_FromFormat("%U=%R",key,value);
            if (one==NULL || PyErr_Occurred()) goto cleanup;
            if (i>=total) {
                {
                    saved = PyErr_GetRaisedException();
                    PyObject *retired = one;
                    one = NULL;
                    Py_XDECREF(retired);
                    _PyValue_RestoreException(saved);
                }
                {
                    saved = PyErr_GetRaisedException();
                    PyObject *retired = key;
                    key = NULL;
                    Py_XDECREF(retired);
                    _PyValue_RestoreException(saved);
                }
                {
                    saved = PyErr_GetRaisedException();
                    PyObject *retired = value;
                    value = NULL;
                    Py_XDECREF(retired);
                    _PyValue_RestoreException(saved);
                }
                if (PyErr_Occurred()) goto cleanup;
                i=-1;
                break;
            }
            if (PyTuple_SetItem(argreprs,i,one)<0 || PyErr_Occurred()) { one=NULL; goto cleanup; }
            one=NULL;
            ++i;
            {
                saved = PyErr_GetRaisedException();
                PyObject *retired = key;
                key = NULL;
                Py_XDECREF(retired);
                _PyValue_RestoreException(saved);
            }
            {
                saved = PyErr_GetRaisedException();
                PyObject *retired = value;
                value = NULL;
                Py_XDECREF(retired);
                _PyValue_RestoreException(saved);
            }
            if (PyErr_Occurred()) goto cleanup;
        }
        if (key!=NULL || value!=NULL) goto cleanup;
        if (iter_status<0 || PyErr_Occurred()) goto cleanup;
        if (i!=total) {
            PyErr_SetString(PyExc_RuntimeError,"keywords dict changed size during iteration");
            goto cleanup;
        }
    }
    sep=PyUnicode_FromString(", ");
    if (sep==NULL || PyErr_Occurred()) goto cleanup;
    joined=PyUnicode_Join(sep,argreprs);
    if (joined==NULL || PyErr_Occurred()) goto cleanup;
    repr=PyUnicode_FromFormat("%s(%R, %U)",Py_TYPE(op_owner)->tp_name,name,joined);
cleanup:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = one;
        one = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = key;
        key = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = value;
        value = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = joined;
        joined = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = sep;
        sep = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = argreprs;
        argreprs = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = kwds;
        kwds = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = args;
        args = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = name;
        name = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    Py_ReprLeave(op_owner);
finish:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = op_owner;
        op_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = repr;
        repr = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return repr;
}

static PyObject *
methodcaller_reduce(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    PyObject *op_owner=Py_NewRef(op), *name=NULL, *args=NULL, *kwds=NULL;
    PyObject *partial=NULL, *constructor=NULL, *newargs=NULL, *result=NULL, *selected=NULL, *saved;
    Py_ssize_t npos;
    methodcallerobject *mc=methodcallerobject_CAST(op_owner);
    name=Py_XNewRef(mc->name); args=Py_XNewRef(mc->args); kwds=Py_XNewRef(mc->kwds);
    if (PyErr_Occurred()) goto done;
    npos=PyTuple_GET_SIZE(args);
    if (kwds==NULL || PyDict_GET_SIZE(kwds)==0) {
        newargs=PyTuple_New(1+npos);
        if (newargs==NULL || PyErr_Occurred()) goto done;
        if (PyTuple_SetItem(newargs,0,Py_NewRef(name))<0 || PyErr_Occurred()) goto done;
        for (Py_ssize_t i=0;i<npos;i++) {
            selected=PyTuple_GetItemRef(args,i);
            if (selected==NULL || PyErr_Occurred()) goto done;
            PyObject *transfer=selected;
            selected=NULL;
            if (PyTuple_SetItem(newargs,i+1,transfer)<0 || PyErr_Occurred()) goto done;
        }
        result=Py_BuildValue("ON",Py_TYPE(op_owner),newargs);
        newargs=NULL;
        goto done;
    }
    partial=PyImport_ImportModuleAttrString("functools","partial");
    if (partial==NULL || PyErr_Occurred()) goto done;
    PyObject *vector_args[2]={(PyObject *)Py_TYPE(op_owner),name};
    constructor=PyObject_VectorcallDict(partial,vector_args,2,kwds);
    if (PyErr_Occurred()) { saved=PyErr_GetRaisedException(); Py_CLEAR(constructor); _PyValue_RestoreException(saved); }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = partial;
        partial = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (constructor==NULL || PyErr_Occurred()) goto done;
    result=Py_BuildValue("NO",constructor,args);
    constructor=NULL;
done:
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = selected;
        selected = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = constructor;
        constructor = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = partial;
        partial = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = newargs;
        newargs = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = kwds;
        kwds = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = args;
        args = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = name;
        name = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    {
        saved = PyErr_GetRaisedException();
        PyObject *retired = op_owner;
        op_owner = NULL;
        Py_XDECREF(retired);
        _PyValue_RestoreException(saved);
    }
    if (PyErr_Occurred()) {
        PyObject *saved = PyErr_GetRaisedException();
        PyObject *retired_result = result;
        result = NULL;
        Py_XDECREF(retired_result);
        _PyValue_RestoreException(saved);
        return NULL;
    }
    return result;
}

static PyMethodDef methodcaller_methods[] = {
    {"__reduce__", methodcaller_reduce, METH_NOARGS,
     reduce_doc},
    {NULL}
};

static PyMemberDef methodcaller_members[] = {
    {"__vectorcalloffset__", Py_T_PYSSIZET, offsetof(methodcallerobject, vectorcall), Py_READONLY},
    {NULL}
};

PyDoc_STRVAR(methodcaller_doc,
"methodcaller(name, /, *args, **kwargs)\n--\n\n\
Return a callable object that calls the given method on its operand.\n\
After f = methodcaller('name'), the call f(r) returns r.name().\n\
After g = methodcaller('name', 'date', foo=1), the call g(r) returns\n\
r.name('date', foo=1).");

static PyType_Slot methodcaller_type_slots[] = {
    {Py_tp_doc, (void *)methodcaller_doc},
    {Py_tp_dealloc, methodcaller_dealloc},
    {Py_tp_call, methodcaller_call},
    {Py_tp_traverse, methodcaller_traverse},
    {Py_tp_clear, methodcaller_clear},
    {Py_tp_methods, methodcaller_methods},
    {Py_tp_members, methodcaller_members},
    {Py_tp_getset, common_getset},
    {Py_tp_new, methodcaller_new},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_repr, methodcaller_repr},
    {0, 0}
};

static PyType_Spec methodcaller_type_spec = {
    .name = "operator.methodcaller",
    .basicsize = sizeof(methodcallerobject),
    .itemsize = 0,
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_HAVE_VECTORCALL | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = methodcaller_type_slots,
};

static int
operator_exec(PyObject *module)
{
    _operator_state *state = get_operator_state(module);
    state->attrgetter_type = PyType_FromModuleAndSpec(module, &attrgetter_type_spec, NULL);
    if (state->attrgetter_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, (PyTypeObject *)state->attrgetter_type) < 0) {
        return -1;
    }

    state->itemgetter_type = PyType_FromModuleAndSpec(module, &itemgetter_type_spec, NULL);
    if (state->itemgetter_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, (PyTypeObject *)state->itemgetter_type) < 0) {
        return -1;
    }

    state->methodcaller_type = PyType_FromModuleAndSpec(module, &methodcaller_type_spec, NULL);
    if (state->methodcaller_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, (PyTypeObject *)state->methodcaller_type) < 0) {
        return -1;
    }

    return 0;
}


static struct PyModuleDef_Slot operator_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, operator_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static int
operator_traverse(PyObject *module, visitproc visit, void *arg)
{
    _operator_state *state = get_operator_state(module);
    Py_VISIT(state->attrgetter_type);
    Py_VISIT(state->itemgetter_type);
    Py_VISIT(state->methodcaller_type);
    return 0;
}

static int
operator_clear(PyObject *module)
{
    _operator_state *state = get_operator_state(module);
    Py_CLEAR(state->attrgetter_type);
    Py_CLEAR(state->itemgetter_type);
    Py_CLEAR(state->methodcaller_type);
    return 0;
}

static void
operator_free(void *module)
{
    (void)operator_clear((PyObject *)module);
}

static struct PyModuleDef operatormodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_operator",
    .m_doc = operator_doc,
    .m_size = sizeof(_operator_state),
    .m_methods = operator_methods,
    .m_slots = operator_slots,
    .m_traverse = operator_traverse,
    .m_clear = operator_clear,
    .m_free = operator_free,
};

PyMODINIT_FUNC
PyInit__operator(void)
{
    return PyModuleDef_Init(&operatormodule);
}
