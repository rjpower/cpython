#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif
#define Py_VALUE_CORE 1
#include "Python.h"
#include "pycore_borrow.h"        // _PyBorrow_TupleItems()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_value.h"      // _PyObject_IsSame()
#include "pycore_value_api.h"  // _PyValue_AsObjectAt()
#include "pycore_value_ops.h"  // _PyValue_GetItem()
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
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

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
_operator_truth_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=ab329cb3331430e2 input=bc74a4cd90235875]*/
{
    return _PyValue_IsTrue(a);
}

/*[clinic input]
_operator.add

    a: object
    b: object
    /

Same as a + b.
[clinic start generated code]*/

static PyValue
_operator_add_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=bb76226a655a37ec input=5efe3bff856ac215]*/
{
    return _PyValue_Add(a, b);
}

/*[clinic input]
_operator.sub = _operator.add

Same as a - b.
[clinic start generated code]*/

static PyValue
_operator_sub_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=41a2c780f0798174 input=6494c6b100b8e795]*/
{
    return _PyValue_Subtract(a, b);
}

/*[clinic input]
_operator.mul = _operator.add

Same as a * b.
[clinic start generated code]*/

static PyValue
_operator_mul_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=7ef7f82c961b3976 input=2368615b4358b70d]*/
{
    return _PyValue_Multiply(a, b);
}

/*[clinic input]
_operator.matmul = _operator.add

Same as a @ b.
[clinic start generated code]*/

static PyValue
_operator_matmul_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=1b6660d70575815b input=9ab304e37fb42dd4]*/
{
    return _PyValue_MatrixMultiply(a, b);
}

/*[clinic input]
_operator.floordiv = _operator.add

Same as a // b.
[clinic start generated code]*/

static PyValue
_operator_floordiv_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=0c1a3082053d3aa6 input=bb2e88ba446c612c]*/
{
    return _PyValue_FloorDivide(a, b);
}

/*[clinic input]
_operator.truediv = _operator.add

Same as a / b.
[clinic start generated code]*/

static PyValue
_operator_truediv_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=7575678642bb0f4b input=ecbb947673f4eb1f]*/
{
    return _PyValue_TrueDivide(a, b);
}

/*[clinic input]
_operator.mod = _operator.add

Same as a % b.
[clinic start generated code]*/

static PyValue
_operator_mod_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=ec502ab1e069f026 input=102e19b422342ac1]*/
{
    return _PyValue_Remainder(a, b);
}

/*[clinic input]
_operator.neg

    a: object
    /

Same as -a.
[clinic start generated code]*/

static PyValue
_operator_neg_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=37237b30ced5b9f4 input=84f09bdcf27c96ec]*/
{
    return _PyValue_Negative(a);
}

/*[clinic input]
_operator.pos = _operator.neg

Same as +a.
[clinic start generated code]*/

static PyValue
_operator_pos_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=572acecb9608c540 input=b6445b63fddb8772]*/
{
    return _PyValue_Positive(a);
}

/*[clinic input]
_operator.abs = _operator.neg

Same as abs(a).
[clinic start generated code]*/

static PyValue
_operator_abs_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=9fc2db8d56bd59ad input=341d07ba86f58039]*/
{
    return _PyValue_Absolute(a);
}

/*[clinic input]
_operator.inv = _operator.neg

Same as ~a.
[clinic start generated code]*/

static PyValue
_operator_inv_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=3b24e31925ef6922 input=b01a4677739f6eb2]*/
{
    return _PyValue_Invert(a);
}

/*[clinic input]
_operator.invert = _operator.neg

Same as ~a.
[clinic start generated code]*/

static PyValue
_operator_invert_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=62f60367fb3fa5b7 input=7f2d607176672e55]*/
{
    return _PyValue_Invert(a);
}

/*[clinic input]
_operator.lshift = _operator.add

Same as a << b.
[clinic start generated code]*/

static PyValue
_operator_lshift_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=d2dac0eff585de3a input=746e8a160cbbc9eb]*/
{
    return _PyValue_Lshift(a, b);
}

/*[clinic input]
_operator.rshift = _operator.add

Same as a >> b.
[clinic start generated code]*/

static PyValue
_operator_rshift_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=7ec2c88c2a2f91f9 input=d2c85bb5a64504c2]*/
{
    return _PyValue_Rshift(a, b);
}

/*[clinic input]
_operator.not_ = _operator.truth

Same as not a.
[clinic start generated code]*/

static int
_operator_not__impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=5a725cb9e2fe12e9 input=854156d50804d9b8]*/
{
    return _PyValue_Not(a);
}

/*[clinic input]
_operator.and_ = _operator.add

Same as a & b.
[clinic start generated code]*/

static PyValue
_operator_and__impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=9cfc098acb9ed7ed input=4f3057c90ec4c99f]*/
{
    return _PyValue_And(a, b);
}

/*[clinic input]
_operator.xor = _operator.add

Same as a ^ b.
[clinic start generated code]*/

static PyValue
_operator_xor_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=0e1341a62ef86dd7 input=3c5cfa7253d808dd]*/
{
    return _PyValue_Xor(a, b);
}

/*[clinic input]
_operator.or_ = _operator.add

Same as a | b.
[clinic start generated code]*/

static PyValue
_operator_or__impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=f0c057db12cdeb07 input=b40c6c44f7c79c09]*/
{
    return _PyValue_Or(a, b);
}

/*[clinic input]
_operator.iadd = _operator.add

Same as a += b.
[clinic start generated code]*/

static PyValue
_operator_iadd_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=210474a368483e24 input=d22a91c07ac69227]*/
{
    return _PyValue_InPlaceAdd(a, b);
}

/*[clinic input]
_operator.isub = _operator.add

Same as a -= b.
[clinic start generated code]*/

static PyValue
_operator_isub_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=fc25c43f2a46e432 input=4591b00d0a0ccafd]*/
{
    return _PyValue_InPlaceSubtract(a, b);
}

/*[clinic input]
_operator.imul = _operator.add

Same as a *= b.
[clinic start generated code]*/

static PyValue
_operator_imul_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=fbceba1ddd24c437 input=0e01fb8631e1b76f]*/
{
    return _PyValue_InPlaceMultiply(a, b);
}

/*[clinic input]
_operator.imatmul = _operator.add

Same as a @= b.
[clinic start generated code]*/

static PyValue
_operator_imatmul_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=dcebb49dacb66c09 input=bb614026372cd542]*/
{
    return _PyValue_InPlaceMatrixMultiply(a, b);
}

/*[clinic input]
_operator.ifloordiv = _operator.add

Same as a //= b.
[clinic start generated code]*/

static PyValue
_operator_ifloordiv_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=dd0bccabf0885a81 input=9df3b5021cff4ca1]*/
{
    return _PyValue_InPlaceFloorDivide(a, b);
}

/*[clinic input]
_operator.itruediv = _operator.add

Same as a /= b.
[clinic start generated code]*/

static PyValue
_operator_itruediv_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=84cf42874d3bdf0d input=9a1ee01608f5f590]*/
{
    return _PyValue_InPlaceTrueDivide(a, b);
}

/*[clinic input]
_operator.imod = _operator.add

Same as a %= b.
[clinic start generated code]*/

static PyValue
_operator_imod_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=2c847ba586bbe7b9 input=d0c384a3ce38e1dd]*/
{
    return _PyValue_InPlaceRemainder(a, b);
}

/*[clinic input]
_operator.ilshift = _operator.add

Same as a <<= b.
[clinic start generated code]*/

static PyValue
_operator_ilshift_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=91b54fcfb83bf0c3 input=e21b6b310f54572e]*/
{
    return _PyValue_InPlaceLshift(a, b);
}

/*[clinic input]
_operator.irshift = _operator.add

Same as a >>= b.
[clinic start generated code]*/

static PyValue
_operator_irshift_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=614e555b403d6172 input=6778dbd0f6e1ec16]*/
{
    return _PyValue_InPlaceRshift(a, b);
}

/*[clinic input]
_operator.iand = _operator.add

Same as a &= b.
[clinic start generated code]*/

static PyValue
_operator_iand_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=d1e3a8d06503df59 input=71dfd8e70c156a7b]*/
{
    return _PyValue_InPlaceAnd(a, b);
}

/*[clinic input]
_operator.ixor = _operator.add

Same as a ^= b.
[clinic start generated code]*/

static PyValue
_operator_ixor_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=be80551cabd9183e input=695c32bec0604d86]*/
{
    return _PyValue_InPlaceXor(a, b);
}

/*[clinic input]
_operator.ior = _operator.add

Same as a |= b.
[clinic start generated code]*/

static PyValue
_operator_ior_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=06c81b27df565064 input=8f01d03eda9920cf]*/
{
    return _PyValue_InPlaceOr(a, b);
}

/*[clinic input]
_operator.concat = _operator.add

Same as a + b, for a and b sequences.
[clinic start generated code]*/

static PyValue
_operator_concat_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=87e644efc04111b4 input=8544ccd5341a3658]*/
{
    return _PyValue_SequenceConcat(a, b);
}

/*[clinic input]
_operator.iconcat = _operator.add

Same as a += b, for a and b sequences.
[clinic start generated code]*/

static PyValue
_operator_iconcat_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=c7cec9ff8e8c7019 input=8f5fe5722fcd837e]*/
{
    return _PyValue_SequenceInPlaceConcat(a, b);
}

/*[clinic input]
_operator.contains -> bool

    a: object
    b: object
    /

Same as b in a (note reversed operands).
[clinic start generated code]*/

static int
_operator_contains_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=0b9ecdfca3ad7a68 input=9122a69b505fde13]*/
{
    return _PyValue_Contains(a, b);
}

/*[clinic input]
_operator.indexOf -> Py_ssize_t

    a: object
    b: object
    /

Return the first index of b in a.
[clinic start generated code]*/

static Py_ssize_t
_operator_indexOf_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=da27b94c4bcb8609 input=8be2e43b6a6fffe3]*/
{
    return _PyValue_SequenceIndex(a, b);
}

/*[clinic input]
_operator.countOf = _operator.indexOf

Return the number of items in a which are, or which equal, b.
[clinic start generated code]*/

static Py_ssize_t
_operator_countOf_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=058a467987c9c763 input=93ea57f170f3f0bb]*/
{
    return _PyValue_SequenceCount(a, b);
}

/*[clinic input]
_operator.getitem

    a: object
    b: object
    /

Same as a[b].
[clinic start generated code]*/

static PyValue
_operator_getitem_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=21747fcecc59cafb input=6682797320e48845]*/
{
    return _PyValue_GetItem(a, b);
}

/*[clinic input]
_operator.setitem

    a: object
    b: object
    c: object
    /

Same as a[b] = c.
[clinic start generated code]*/

static PyValue
_operator_setitem_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b,
                       PyValueBorrow c)
/*[clinic end generated code: output=62ea8dc78d89f578 input=ceaf453c4d3a58df]*/
{
    if (-1 == _PyValue_SetItem(a, b, c))
        return PyValue_NULL;
    return PyValue_None;
}

/*[clinic input]
_operator.delitem = _operator.getitem

Same as del a[b].
[clinic start generated code]*/

static PyValue
_operator_delitem_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=cc5e137fe4a46118 input=991bec56a0d3ec7f]*/
{
    if (-1 == _PyValue_DelItem(a, b))
        return PyValue_NULL;
    return PyValue_None;
}

/*[clinic input]
_operator.eq

    a: object
    b: object
    /

Same as a == b.
[clinic start generated code]*/

static PyValue
_operator_eq_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=ffd4c928101f734d input=586fca687a95a83f]*/
{
    return _PyValue_RichCompare(a, b, Py_EQ);
}

/*[clinic input]
_operator.ne = _operator.eq

Same as a != b.
[clinic start generated code]*/

static PyValue
_operator_ne_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=07754e5a0070faf4 input=5d88f23d35e9abac]*/
{
    return _PyValue_RichCompare(a, b, Py_NE);
}

/*[clinic input]
_operator.lt = _operator.eq

Same as a < b.
[clinic start generated code]*/

static PyValue
_operator_lt_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=df132930212c9ac5 input=34a59ad6d39d3a2b]*/
{
    return _PyValue_RichCompare(a, b, Py_LT);
}

/*[clinic input]
_operator.le = _operator.eq

Same as a <= b.
[clinic start generated code]*/

static PyValue
_operator_le_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=66d6f0ea3ccdddd5 input=b812a7860a0bef44]*/
{
    return _PyValue_RichCompare(a, b, Py_LE);
}

/*[clinic input]
_operator.gt = _operator.eq

Same as a > b.
[clinic start generated code]*/

static PyValue
_operator_gt_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=42a2521c20d3e644 input=9bdb45b995ada35b]*/
{
    return _PyValue_RichCompare(a, b, Py_GT);
}

/*[clinic input]
_operator.ge = _operator.eq

Same as a >= b.
[clinic start generated code]*/

static PyValue
_operator_ge_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=b97ec357eee80854 input=cf1dc4a5ca9c35f5]*/
{
    return _PyValue_RichCompare(a, b, Py_GE);
}

/*[clinic input]
_operator.pow = _operator.add

Same as a ** b.
[clinic start generated code]*/

static PyValue
_operator_pow_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=9f0d875df60c21c2 input=690b40f097ab1637]*/
{
    return _PyValue_Power(a, b, PyValue_None);
}

/*[clinic input]
_operator.ipow = _operator.add

Same as a **= b.
[clinic start generated code]*/

static PyValue
_operator_ipow_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=9c8b03b1f50e6fb0 input=f00623899d07499a]*/
{
    return _PyValue_InPlacePower(a, b, PyValue_None);
}

/*[clinic input]
_operator.index

    a: object
    /

Same as a.__index__()
[clinic start generated code]*/

static PyValue
_operator_index_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=f78df7277497b4c9 input=6f54d50ea64a579c]*/
{
    return _PyValue_Index(a);
}

/*[clinic input]
_operator.is_ = _operator.add

Same as a is b.
[clinic start generated code]*/

static PyValue
_operator_is__impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=492014dc5ab041f5 input=5fa9b97df03c427f]*/
{
    return PyValue_FromBool(PyValue_Is(a, b));
}

/*[clinic input]
_operator.is_not = _operator.add

Same as a is not b.
[clinic start generated code]*/

static PyValue
_operator_is_not_impl(PyObject *module, PyValueBorrow a, PyValueBorrow b)
/*[clinic end generated code: output=2f9cc71a975c3ae8 input=5a93f7e1a93535f1]*/
{
    return PyValue_FromBool(!PyValue_Is(a, b));
}

/*[clinic input]
_operator.is_none = _operator.neg

Same as a is None.
[clinic start generated code]*/

static PyValue
_operator_is_none_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=a475052ce43bbbfd input=0448b38af7b8533d]*/
{
    return PyValue_FromBool(PyValue_IsNone(a));
}

/*[clinic input]
_operator.is_not_none = _operator.neg

Same as a is not None.
[clinic start generated code]*/

static PyValue
_operator_is_not_none_impl(PyObject *module, PyValueBorrow a)
/*[clinic end generated code: output=d1eb0ba398fd64d6 input=7587f38ebac51688]*/
{
    return PyValue_FromBool(!PyValue_IsNone(a));
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

/* An inline int or float has no buffer interface. */
static int
value_has_buffer(PyValueBorrow v)
{
    return PyValue_IsObject(v)
           && PyObject_CheckBuffer(PyValue_AsKnownObjectBorrow(v));
}

/* PyObject_GetBuffer() on a value; an inline value is boxed so that the
   call raises the same TypeError as before. */
static int
value_get_buffer(PyValueBorrow v, Py_buffer *view, int flags)
{
    if (PyValue_IsObject(v)) {
        return PyObject_GetBuffer(PyValue_AsKnownObjectBorrow(v), view, flags);
    }
    PyObject *o = _PyValue_AsObjectAt(v, BOX_SITE_PARSER);
    if (o == NULL) {
        return -1;
    }
    int rc = PyObject_GetBuffer(o, view, flags);
    Py_DECREF(o);
    return rc;
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
_operator_length_hint_impl(PyObject *module, PyValueBorrow obj,
                           Py_ssize_t default_value)
/*[clinic end generated code: output=8d580d0b5a25903e input=65ed29f04401e96a]*/
{
    return _PyValue_LengthHint(obj, default_value);
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

static PyValue
_operator__compare_digest_impl(PyObject *module, PyValueBorrow a,
                               PyValueBorrow b)
/*[clinic end generated code: output=7e9dcac42f20bfe1 input=9ac7e2c4e30bc356]*/
{
    int rc;

    /* ASCII unicode string */
    if((PyValue_IsObject(a) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(a))) && (PyValue_IsObject(b) && PyUnicode_Check(PyValue_AsKnownObjectBorrow(b)))) {
        PyObject *oa = PyValue_AsKnownObjectBorrow(a);
        PyObject *ob = PyValue_AsKnownObjectBorrow(b);
        if (!PyUnicode_IS_ASCII(oa) || !PyUnicode_IS_ASCII(ob)) {
            PyErr_SetString(PyExc_TypeError,
                            "comparing strings with non-ASCII characters is "
                            "not supported");
            return PyValue_NULL;
        }

        rc = _tscmp(PyUnicode_DATA(oa),
                    PyUnicode_DATA(ob),
                    PyUnicode_GET_LENGTH(oa),
                    PyUnicode_GET_LENGTH(ob));
    }
    /* fallback to buffer interface for bytes, bytearray and other */
    else {
        Py_buffer view_a;
        Py_buffer view_b;

        if (!value_has_buffer(a) && !value_has_buffer(b)) {
            PyErr_Format(PyExc_TypeError,
                         "unsupported operand types(s) or combination of types: "
                         "'%.100s' and '%.100s'",
                         PyValue_TYPE(a)->tp_name, PyValue_TYPE(b)->tp_name);
            return PyValue_NULL;
        }

        if (value_get_buffer(a, &view_a, PyBUF_SIMPLE) == -1) {
            return PyValue_NULL;
        }
        if (view_a.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            return PyValue_NULL;
        }

        if (value_get_buffer(b, &view_b, PyBUF_SIMPLE) == -1) {
            PyBuffer_Release(&view_a);
            return PyValue_NULL;
        }
        if (view_b.ndim > 1) {
            PyErr_SetString(PyExc_BufferError,
                            "Buffer must be single dimension");
            PyBuffer_Release(&view_a);
            PyBuffer_Release(&view_b);
            return PyValue_NULL;
        }

        rc = _tscmp((const unsigned char*)view_a.buf,
                    (const unsigned char*)view_b.buf,
                    view_a.len,
                    view_b.len);

        PyBuffer_Release(&view_a);
        PyBuffer_Release(&view_b);
    }

    return PyValue_FromBool(rc);
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
    PyValue item;     // required home: the key, or a tuple of keys
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
    itemgetterobject *ig;
    PyObject *item;
    Py_ssize_t nitems;
    Py_ssize_t index;

    if (!_PyArg_NoKeywords("itemgetter", kwds))
        return NULL;

    nitems = PyTuple_GET_SIZE(args);
    if (nitems <= 1) {
        if (!_PyArg_CheckPositional("itemgetter", nitems, 1, 1))
            return NULL;
        PyValue item_v = PyTuple_GetValue(args, 0);
        item = PyValue_IsNull(item_v) ? NULL : _PyValue_AsObjectStealAt(item_v, BOX_SITE_TUPLE);
        if (item == NULL)
            return NULL;
    } else {
        item = Py_NewRef(args);
    }
    _operator_state *state = _PyType_GetModuleState(type);
    /* create itemgetterobject structure */
    ig = PyObject_GC_New(itemgetterobject, (PyTypeObject *) state->itemgetter_type);
    if (ig == NULL) {
        Py_DECREF(item);
        return NULL;
    }

    ig->nitems = nitems;
    ig->index = -1;
    if (PyLong_CheckExact(item)) {
        index = PyLong_AsSsize_t(item);
        if (index < 0) {
            /* If we get here, then either the index conversion failed
             * due to being out of range, or the index was a negative
             * integer.  Either way, we clear any possible exception
             * and fall back to the slow path, where ig->index is -1.
             */
            PyErr_Clear();
        }
        else {
            ig->index = index;
        }
    }

    ig->item = PyValue_PrepareOwned(item);
    _PyValue_CheckHome(ig->item);
    ig->vectorcall = itemgetter_vectorcall;
    PyObject_GC_Track(ig);
    return (PyObject *)ig;
}

static int
itemgetter_clear(PyObject *op)
{
    itemgetterobject *ig = itemgetterobject_CAST(op);
    PyValue_CLEAR(ig->item);
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
    _Py_VISIT_VALUE(ig->item);
    return 0;
}

static PyObject *
itemgetter_call(PyObject *op, PyObject *args, PyObject *kw)
{
    assert(PyTuple_CheckExact(args));
    if (!_PyArg_NoKeywords("itemgetter", kw))
        return NULL;
    if (!_PyArg_CheckPositional("itemgetter", PyTuple_GET_SIZE(args), 1, 1))
        return NULL;
    PyValue obj_v = PyTuple_GetValue(args, 0);
    PyObject *obj = PyValue_IsNull(obj_v) ? NULL : _PyValue_AsObjectStealAt(obj_v, BOX_SITE_TUPLE);
    if (obj == NULL)
        return NULL;
    itemgetterobject *ig = itemgetterobject_CAST(op);
    PyObject *result = itemgetter_call_impl(ig, obj);
    Py_DECREF(obj);
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
    PyObject *result;
    Py_ssize_t i, nitems=ig->nitems;
    if (nitems == 1) {
        if (ig->index >= 0
            && PyTuple_CheckExact(obj)
            && ig->index < PyTuple_GET_SIZE(obj))
        {
            PyValue item_v2 = PyTuple_GetValue(obj, ig->index);
            return PyValue_IsNull(item_v2) ? NULL : _PyValue_AsObjectStealAt(item_v2, BOX_SITE_TUPLE);
        }
        PyValue found = _PyValue_GetItem(PyValue_FromObjectBorrow(obj),
                                         ig->item);
        if (PyValue_IsNull(found)) {
            return NULL;
        }
        return _PyValue_AsObjectStealAt(found, BOX_SITE_SEQUENCE);
    }

    PyObject *items = PyValue_AsKnownObjectBorrow(ig->item);
    assert(PyTuple_Check(items));
    assert(PyTuple_GET_SIZE(items) == nitems);

    result = PyTuple_New(nitems);
    if (result == NULL)
        return NULL;

    for (i=0 ; i < nitems ; i++) {
        /* The key tuple is immutable and owned by ig, so the view is safe. */
        PyValue val = _PyValue_GetItem(PyValue_FromObjectBorrow(obj),
                                       PyTuple_GET_VALUE(items, i));
        if (PyValue_IsNull(val)) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_VALUE(result, i, val);
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
    PyObject *item = _PyValue_AsObjectAt(ig->item, BOX_SITE_REPR);
    if (item == NULL) {
        Py_ReprLeave(op);
        return NULL;
    }
    repr = PyUnicode_FromFormat(reprfmt, Py_TYPE(ig)->tp_name, item);
    Py_DECREF(item);
    Py_ReprLeave(op);
    return repr;
}

static PyObject *
itemgetter_reduce(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    itemgetterobject *ig = itemgetterobject_CAST(op);
    PyObject *item = _PyValue_AsObjectAt(ig->item, BOX_SITE_FIELD);
    if (item == NULL) {
        return NULL;
    }
    PyObject *res;
    if (ig->nitems == 1) {
        res = Py_BuildValue("O(O)", Py_TYPE(ig), item);
        Py_DECREF(item);
        return res;
    }
    return _PyTuple_FromPairTakeValue(PyValue_DUP(_PyValue_FromObjectKnown(_PyObject_CAST(Py_TYPE(ig)))), PyValue_PrepareOwned(item));
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
    attrgetterobject *ag;
    PyObject *attr;
    Py_ssize_t nattrs, idx, char_idx;

    if (!_PyArg_NoKeywords("attrgetter", kwds))
        return NULL;

    nattrs = PyTuple_GET_SIZE(args);
    if (nattrs <= 1) {
        if (!_PyArg_CheckPositional("attrgetter", nattrs, 1, 1))
            return NULL;
    }

    attr = PyTuple_New(nattrs);
    if (attr == NULL)
        return NULL;

    /* prepare attr while checking args */
    PyInterpreterState *interp = _PyInterpreterState_GET();
    for (idx = 0; idx < nattrs; ++idx) {
        PyValueBorrow candidate = PyTuple_GET_VALUE(args, idx);
        int dot_count;

        if (!PyValue_IsObject(candidate) ||
            !PyUnicode_Check(PyValue_AsKnownObjectBorrow(candidate))) {
            PyErr_SetString(PyExc_TypeError,
                            "attribute name must be a string");
            Py_DECREF(attr);
            return NULL;
        }
        PyObject *item = PyValue_AsKnownObjectBorrow(candidate);
        Py_ssize_t item_len = PyUnicode_GET_LENGTH(item);
        int kind = PyUnicode_KIND(item);
        const void *data = PyUnicode_DATA(item);

        /* check whether the string is dotted */
        dot_count = 0;
        for (char_idx = 0; char_idx < item_len; ++char_idx) {
            if (PyUnicode_READ(kind, data, char_idx) == '.')
                ++dot_count;
        }

        if (dot_count == 0) {
            Py_INCREF(item);
            _PyUnicode_InternMortal(interp, &item);
            PyTuple_SET_VALUE(attr, idx, PyValue_PrepareOwned(item));
        } else { /* make it a tuple of non-dotted attrnames */
            PyObject *attr_chain = PyTuple_New(dot_count + 1);
            PyObject *attr_chain_item;
            Py_ssize_t unibuff_from = 0;
            Py_ssize_t unibuff_till = 0;
            Py_ssize_t attr_chain_idx = 0;

            if (attr_chain == NULL) {
                Py_DECREF(attr);
                return NULL;
            }

            for (; dot_count > 0; --dot_count) {
                while (PyUnicode_READ(kind, data, unibuff_till) != '.') {
                    ++unibuff_till;
                }
                attr_chain_item = PyUnicode_Substring(item,
                                      unibuff_from,
                                      unibuff_till);
                if (attr_chain_item == NULL) {
                    Py_DECREF(attr_chain);
                    Py_DECREF(attr);
                    return NULL;
                }
                _PyUnicode_InternMortal(interp, &attr_chain_item);
                PyTuple_SET_VALUE(attr_chain, attr_chain_idx, PyValue_PrepareOwned(attr_chain_item));
                ++attr_chain_idx;
                unibuff_till = unibuff_from = unibuff_till + 1;
            }

            /* now add the last dotless name */
            attr_chain_item = PyUnicode_Substring(item,
                                                  unibuff_from, item_len);
            if (attr_chain_item == NULL) {
                Py_DECREF(attr_chain);
                Py_DECREF(attr);
                return NULL;
            }
            _PyUnicode_InternMortal(interp, &attr_chain_item);
            PyTuple_SET_VALUE(attr_chain, attr_chain_idx, PyValue_PrepareOwned(attr_chain_item));

            PyTuple_SET_VALUE(attr, idx, PyValue_PrepareOwned(attr_chain));
        }
    }

    _operator_state *state = _PyType_GetModuleState(type);
    /* create attrgetterobject structure */
    ag = PyObject_GC_New(attrgetterobject, (PyTypeObject *)state->attrgetter_type);
    if (ag == NULL) {
        Py_DECREF(attr);
        return NULL;
    }

    ag->attr = attr;
    ag->nattrs = nattrs;
    ag->vectorcall = attrgetter_vectorcall;

    PyObject_GC_Track(ag);
    return (PyObject *)ag;
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
    PyObject *newobj;

    /* attr is either a tuple or instance of str.
       Ensured by the setup code of attrgetter_new */
    if (PyTuple_CheckExact(attr)) { /* chained getattr */
        Py_ssize_t name_idx = 0, name_count;
        PyObject *attr_name;

        name_count = PyTuple_GET_SIZE(attr);
        Py_INCREF(obj);
        for (name_idx = 0; name_idx < name_count; ++name_idx) {
            attr_name = _PyTuple_GET_OBJECT(attr, name_idx);
            PyValue newobj_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(obj), attr_name);
            newobj = PyValue_IsNull(newobj_v) ? NULL : _PyValue_AsObjectStealAt(newobj_v, BOX_SITE_ATTRIBUTE);
            Py_DECREF(obj);
            if (newobj == NULL) {
                return NULL;
            }
            /* here */
            obj = newobj;
        }
    } else { /* single getattr */
        PyValue newobj_v2 = _PyValue_GetAttr(PyValue_FromObjectBorrow(obj), attr);
        newobj = PyValue_IsNull(newobj_v2) ? NULL : _PyValue_AsObjectStealAt(newobj_v2, BOX_SITE_ATTRIBUTE);
        if (newobj == NULL)
            return NULL;
        obj = newobj;
    }

    return obj;
}

static PyObject *
attrgetter_call(PyObject *op, PyObject *args, PyObject *kw)
{
    if (!_PyArg_NoKeywords("attrgetter", kw))
        return NULL;
    if (!_PyArg_CheckPositional("attrgetter", PyTuple_GET_SIZE(args), 1, 1))
        return NULL;
    PyValue obj_v2 = PyTuple_GetValue(args, 0);
    PyObject *obj = PyValue_IsNull(obj_v2) ? NULL : _PyValue_AsObjectStealAt(obj_v2, BOX_SITE_TUPLE);
    if (obj == NULL)
        return NULL;
    attrgetterobject *ag = attrgetterobject_CAST(op);
    PyObject *result = attrgetter_call_impl(ag, obj);
    Py_DECREF(obj);
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
    PyObject *result;
    Py_ssize_t i, nattrs=ag->nattrs;

    if (ag->nattrs == 1) {
        /* ag->attr is always a tuple */
        return dotted_getattr(obj, _PyTuple_GET_OBJECT(ag->attr, 0));
    }

    assert(PyTuple_Check(ag->attr));
    assert(PyTuple_GET_SIZE(ag->attr) == nattrs);

    result = PyTuple_New(nattrs);
    if (result == NULL)
        return NULL;

    for (i=0 ; i < nattrs ; i++) {
        PyObject *attr, *val;
        attr = _PyTuple_GET_OBJECT(ag->attr, i);
        val = dotted_getattr(obj, attr);
        if (val == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_VALUE(result, i, PyValue_PrepareOwned(val));
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
    Py_ssize_t i;
    PyObject *attrsep = NULL;
    PyObject *attrstrings = PyTuple_New(ag->nattrs);
    if (attrstrings == NULL)
        return NULL;

    for (i = 0; i < ag->nattrs; ++i) {
        PyObject *attr = _PyTuple_GET_OBJECT(ag->attr, i);
        PyObject *attrstr = dotjoinattr(attr, &attrsep);
        if (attrstr == NULL) {
            Py_XDECREF(attrsep);
            Py_DECREF(attrstrings);
            return NULL;
        }
        PyTuple_SET_VALUE(attrstrings, i, PyValue_PrepareOwned(attrstr));
    }
    Py_XDECREF(attrsep);
    return attrstrings;
}

static PyObject *
attrgetter_repr(PyObject *op)
{
    PyObject *repr = NULL;
    attrgetterobject *ag = attrgetterobject_CAST(op);
    int status = Py_ReprEnter(op);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(ag)->tp_name);
    }

    if (ag->nattrs == 1) {
        PyObject *attrsep = NULL;
        PyObject *attr = dotjoinattr(_PyTuple_GET_OBJECT(ag->attr, 0), &attrsep);
        if (attr != NULL) {
            repr = PyUnicode_FromFormat("%s(%R)", Py_TYPE(ag)->tp_name, attr);
            Py_DECREF(attr);
        }
        Py_XDECREF(attrsep);
    }
    else {
        PyObject *attrstrings = attrgetter_args(ag);
        if (attrstrings != NULL) {
            repr = PyUnicode_FromFormat("%s%R",
                                        Py_TYPE(ag)->tp_name, attrstrings);
            Py_DECREF(attrstrings);
        }
    }
    Py_ReprLeave(op);
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
    methodcallerobject *mc = methodcallerobject_CAST(op);
    if (!_PyArg_CheckPositional("methodcaller", PyVectorcall_NARGS(nargsf), 1, 1)
        || !_PyArg_NoKwnames("methodcaller", kwnames)) {
        return NULL;
    }
    assert(mc->vectorcall_args != NULL);

    Py_ssize_t nall = PyTuple_GET_SIZE(mc->vectorcall_args);
    assert(1 + nall <= _METHODCALLER_MAX_ARGS);
    /* The stored arguments may hold inline values: lend them as a borrow
       stack run for the duration of the call. */
    PyThreadState *tstate = _PyThreadState_GET();
    _PyBorrowMark mark = _PyBorrowScope_Enter(tstate);
    PyObject *result = NULL;
    PyObject *const *items = _PyBorrow_TupleItems(mc->vectorcall_args, nall);
    if (items != NULL) {
        PyObject *tmp_args[_METHODCALLER_MAX_ARGS];
        tmp_args[0] = args[0];
        memcpy(tmp_args + 1, items, sizeof(PyObject *) * nall);
        result = PyObject_VectorcallMethod(mc->name, tmp_args,
                (1 + PyTuple_GET_SIZE(mc->args)) | PY_VECTORCALL_ARGUMENTS_OFFSET,
                mc->vectorcall_kwnames);
    }
    _PyBorrowScope_Exit(tstate, mark);
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
        PyObject *values_tuple = _PyValue_SequenceTuple(PyValue_FromObjectBorrow(values));
        Py_DECREF(values);
        if (!values_tuple) {
            return -1;
        }
        if (PyTuple_GET_SIZE(args)) {
            PyValue joined = _PyValue_SequenceConcat(
                PyValue_FromObjectBorrow(args),
                PyValue_FromObjectBorrow(values_tuple));
            Py_DECREF(values_tuple);
            /* A tuple concatenated with a tuple is a tuple object. */
            mc->vectorcall_args = PyValue_IsNull(joined)
                ? NULL : _PyValue_AsObjectStealAt(joined, BOX_SITE_TUPLE);
            if (mc->vectorcall_args == NULL) {
                return -1;
            }
        }
        else {
            mc->vectorcall_args = values_tuple;
        }
        mc->vectorcall_kwnames = _PyValue_SequenceTuple(PyValue_FromObjectBorrow(kwds));
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
    methodcallerobject *mc;
    PyObject *name;

    if (PyTuple_GET_SIZE(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "methodcaller needs at least "
                        "one argument, the method name");
        return NULL;
    }

    PyValueBorrow candidate = PyTuple_GET_VALUE(args, 0);
    if (!PyValue_IsObject(candidate) ||
        !PyUnicode_Check(PyValue_AsKnownObjectBorrow(candidate))) {
        PyErr_SetString(PyExc_TypeError,
                        "method name must be a string");
        return NULL;
    }
    name = PyValue_AsKnownObjectBorrow(candidate);

    _operator_state *state = _PyType_GetModuleState(type);
    /* create methodcallerobject structure */
    mc = PyObject_GC_New(methodcallerobject, (PyTypeObject *)state->methodcaller_type);
    if (mc == NULL) {
        return NULL;
    }
    mc->vectorcall = NULL;
    mc->vectorcall_args = NULL;
    mc->vectorcall_kwnames = NULL;
    mc->kwds = Py_XNewRef(kwds);

    Py_INCREF(name);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyUnicode_InternMortal(interp, &name);
    mc->name = name;

    mc->args = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    if (mc->args == NULL) {
        Py_DECREF(mc);
        return NULL;
    }

    Py_ssize_t vectorcall_size = PyTuple_GET_SIZE(args)
                                   + (kwds ? PyDict_Size(kwds) : 0);
    if (vectorcall_size < (_METHODCALLER_MAX_ARGS)) {
        if (_methodcaller_initialize_vectorcall(mc) < 0) {
            Py_DECREF(mc);
            return NULL;
        }
    }

    PyObject_GC_Track(mc);
    return (PyObject *)mc;
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
    PyObject *method, *obj, *result;
    methodcallerobject *mc = methodcallerobject_CAST(op);

    if (!_PyArg_NoKeywords("methodcaller", kw))
        return NULL;
    if (!_PyArg_CheckPositional("methodcaller", PyTuple_GET_SIZE(args), 1, 1))
        return NULL;
    PyValue obj_v3 = PyTuple_GetValue(args, 0);
    obj = PyValue_IsNull(obj_v3) ? NULL : _PyValue_AsObjectStealAt(obj_v3, BOX_SITE_TUPLE);
    if (obj == NULL)
        return NULL;
    PyValue method_v = _PyValue_GetAttr(PyValue_FromObjectBorrow(obj), mc->name);
    method = PyValue_IsNull(method_v) ? NULL : _PyValue_AsObjectStealAt(method_v, BOX_SITE_ATTRIBUTE);
    Py_DECREF(obj);
    if (method == NULL)
        return NULL;

    result = PyObject_Call(method, mc->args, mc->kwds);
    Py_DECREF(method);
    return result;
}

static PyObject *
methodcaller_repr(PyObject *op)
{
    methodcallerobject *mc = methodcallerobject_CAST(op);
    PyObject *argreprs, *repr = NULL, *sep, *joinedargreprs;
    Py_ssize_t numtotalargs, numposargs, numkwdargs, i;
    int status = Py_ReprEnter(op);
    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(mc)->tp_name);
    }

    numkwdargs = mc->kwds != NULL ? PyDict_GET_SIZE(mc->kwds) : 0;
    numposargs = PyTuple_GET_SIZE(mc->args);
    numtotalargs = numposargs + numkwdargs;

    if (numtotalargs == 0) {
        repr = PyUnicode_FromFormat("%s(%R)", Py_TYPE(mc)->tp_name, mc->name);
        Py_ReprLeave(op);
        return repr;
    }

    argreprs = PyTuple_New(numtotalargs);
    if (argreprs == NULL) {
        Py_ReprLeave(op);
        return NULL;
    }

    for (i = 0; i < numposargs; ++i) {
        PyValue arg_v = PyTuple_GetValue(mc->args, i);
        PyObject *arg = PyValue_IsNull(arg_v) ? NULL : _PyValue_AsObjectStealAt(arg_v, BOX_SITE_TUPLE);
        if (arg == NULL)
            goto done;
        PyObject *onerepr = _PyValue_Repr(PyValue_FromObjectBorrow(arg));
        Py_DECREF(arg);
        if (onerepr == NULL)
            goto done;
        PyTuple_SET_VALUE(argreprs, i, PyValue_PrepareOwned(onerepr));
    }

    if (numkwdargs != 0) {
        PyValue keyv, valuev;
        Py_ssize_t pos = 0;
        int next;
        /* The entries are owned: formatting a value runs __repr__, which can
           mutate mc->kwds. */
        while ((next = PyDict_NextValue(mc->kwds, &pos, &keyv, &valuev)) > 0) {
            PyObject *onerepr = NULL;
            PyObject *key = _PyValue_AsObjectAt(keyv, BOX_SITE_DICT);
            PyObject *value = _PyValue_AsObjectAt(valuev, BOX_SITE_DICT);
            PyValue_CLOSE(keyv);
            PyValue_CLOSE(valuev);
            if (key == NULL || value == NULL) {
                Py_XDECREF(key);
                Py_XDECREF(value);
                goto done;
            }
            if (!PyUnicode_Check(key)) {
                PyErr_SetString(PyExc_TypeError, "keywords must be strings");
            }
            else {
                onerepr = PyUnicode_FromFormat("%U=%R", key, value);
            }
            Py_DECREF(key);
            Py_DECREF(value);
            if (onerepr == NULL)
                goto done;
            if (i >= numtotalargs) {
                i = -1;
                Py_DECREF(onerepr);
                break;
            }
            PyTuple_SET_VALUE(argreprs, i, PyValue_PrepareOwned(onerepr));
            ++i;
        }
        if (next < 0)
            goto done;
        if (i != numtotalargs) {
            PyErr_SetString(PyExc_RuntimeError,
                            "keywords dict changed size during iteration");
            goto done;
        }
    }

    sep = PyUnicode_FromString(", ");
    if (sep == NULL)
        goto done;

    joinedargreprs = PyUnicode_Join(sep, argreprs);
    Py_DECREF(sep);
    if (joinedargreprs == NULL)
        goto done;

    repr = PyUnicode_FromFormat("%s(%R, %U)", Py_TYPE(mc)->tp_name,
                                mc->name, joinedargreprs);
    Py_DECREF(joinedargreprs);

done:
    Py_DECREF(argreprs);
    Py_ReprLeave(op);
    return repr;
}

static PyObject *
methodcaller_reduce(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    methodcallerobject *mc = methodcallerobject_CAST(op);
    if (!mc->kwds || PyDict_GET_SIZE(mc->kwds) == 0) {
        Py_ssize_t i;
        Py_ssize_t callargcount = PyTuple_GET_SIZE(mc->args);
        PyObject *newargs = PyTuple_New(1 + callargcount);
        if (newargs == NULL)
            return NULL;
        PyTuple_SET_VALUE(newargs, 0, PyValue_DUP(PyValue_FromObjectBorrow(mc->name)));
        for (i = 0; i < callargcount; ++i) {
            PyValue arg_v2 = PyTuple_GetValue(mc->args, i);
            PyObject *arg = PyValue_IsNull(arg_v2) ? NULL : _PyValue_AsObjectStealAt(arg_v2, BOX_SITE_TUPLE);
            if (arg == NULL) {
                Py_DECREF(newargs);
                return NULL;
            }
            PyTuple_SET_VALUE(newargs, i + 1, PyValue_PrepareOwned(arg));
        }
        return Py_BuildValue("ON", Py_TYPE(mc), newargs);
    }
    else {
        PyObject *partial;
        PyObject *constructor;
        PyObject *newargs[2];

        partial = PyImport_ImportModuleAttrString("functools", "partial");
        if (!partial)
            return NULL;

        newargs[0] = (PyObject *)Py_TYPE(mc);
        newargs[1] = mc->name;
        constructor = PyObject_VectorcallDict(partial, newargs, 2, mc->kwds);

        Py_DECREF(partial);
        return Py_BuildValue("NO", constructor, mc->args);
    }
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
