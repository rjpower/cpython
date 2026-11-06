/* Complex object implementation */

/* Borrows heavily from floatobject.c */

/* Submitted by Jim Hugunin */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_complexobject.h" // _PyComplex_FormatAdvancedWriter()
#include "pycore_floatobject.h"   // _Py_convert_int_to_double()
#include "pycore_freelist.h"      // _Py_FREELIST_FREE(), _Py_FREELIST_POP()
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_long_value.h"    // _PyValue_AsDouble()
#include "pycore_object.h"        // _PyObject_Init()
#include "pycore_pymath.h"        // _Py_ADJUST_ERANGE2()
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
#include "pycore_value_ops.h"     // _PyValue_Float()
#include "pycore_value_slots.h"   // _Py_VALUE_THUNK_BINARY()


#define _PyComplexObject_CAST(op)   ((PyComplexObject *)(op))


/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
class complex "PyComplexObject *" "&PyComplex_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=819e057d2d10f5ec]*/

#include "clinic/complexobject.c.h"

/* elementary operations on complex numbers */

Py_complex
_Py_c_sum(Py_complex a, Py_complex b)
{
    Py_complex r;
    r.real = a.real + b.real;
    r.imag = a.imag + b.imag;
    return r;
}

Py_complex
_Py_cr_sum(Py_complex a, double b)
{
    Py_complex r = a;
    r.real += b;
    return r;
}

static inline Py_complex
_Py_rc_sum(double a, Py_complex b)
{
    return _Py_cr_sum(b, a);
}

Py_complex
_Py_c_diff(Py_complex a, Py_complex b)
{
    Py_complex r;
    r.real = a.real - b.real;
    r.imag = a.imag - b.imag;
    return r;
}

Py_complex
_Py_cr_diff(Py_complex a, double b)
{
    Py_complex r = a;
    r.real -= b;
    return r;
}

Py_complex
_Py_rc_diff(double a, Py_complex b)
{
    Py_complex r;
    r.real = a - b.real;
    r.imag = -b.imag;
    return r;
}

Py_complex
_Py_c_neg(Py_complex a)
{
    Py_complex r;
    r.real = -a.real;
    r.imag = -a.imag;
    return r;
}

Py_complex
_Py_c_prod(Py_complex z, Py_complex w)
{
    double a = z.real, b = z.imag, c = w.real, d = w.imag;
    double ac = a*c, bd = b*d, ad = a*d, bc = b*c;
    Py_complex r = {ac - bd, ad + bc};

    /* Recover infinities that computed as nan+nanj.  See e.g. the C11,
       Annex G.5.1, routine _Cmultd(). */
    if (isnan(r.real) && isnan(r.imag)) {
        int recalc = 0;

        if (isinf(a) || isinf(b)) {  /* z is infinite */
            /* "Box" the infinity and change nans in the other factor to 0 */
            a = copysign(isinf(a) ? 1.0 : 0.0, a);
            b = copysign(isinf(b) ? 1.0 : 0.0, b);
            if (isnan(c)) {
                c = copysign(0.0, c);
            }
            if (isnan(d)) {
                d = copysign(0.0, d);
            }
            recalc = 1;
        }
        if (isinf(c) || isinf(d)) {  /* w is infinite */
            /* "Box" the infinity and change nans in the other factor to 0 */
            c = copysign(isinf(c) ? 1.0 : 0.0, c);
            d = copysign(isinf(d) ? 1.0 : 0.0, d);
            if (isnan(a)) {
                a = copysign(0.0, a);
            }
            if (isnan(b)) {
                b = copysign(0.0, b);
            }
            recalc = 1;
        }
        if (!recalc && (isinf(ac) || isinf(bd) || isinf(ad) || isinf(bc))) {
            /* Recover infinities from overflow by changing nans to 0 */
            if (isnan(a)) {
                a = copysign(0.0, a);
            }
            if (isnan(b)) {
                b = copysign(0.0, b);
            }
            if (isnan(c)) {
                c = copysign(0.0, c);
            }
            if (isnan(d)) {
                d = copysign(0.0, d);
            }
            recalc = 1;
        }
        if (recalc) {
            r.real = INFINITY*(a*c - b*d);
            r.imag = INFINITY*(a*d + b*c);
        }
    }

    return r;
}

Py_complex
_Py_cr_prod(Py_complex a, double b)
{
    Py_complex r = a;
    r.real *= b;
    r.imag *= b;
    return r;
}

static inline Py_complex
_Py_rc_prod(double a, Py_complex b)
{
    return _Py_cr_prod(b, a);
}

/* Avoid bad optimization on Windows ARM64 until the compiler is fixed */
#ifdef _M_ARM64
#pragma optimize("", off)
#endif
Py_complex
_Py_c_quot(Py_complex a, Py_complex b)
{
    /******************************************************************
    This was the original algorithm.  It's grossly prone to spurious
    overflow and underflow errors.  It also merrily divides by 0 despite
    checking for that(!).  The code still serves a doc purpose here, as
    the algorithm following is a simple by-cases transformation of this
    one:

    Py_complex r;
    double d = b.real*b.real + b.imag*b.imag;
    if (d == 0.)
        errno = EDOM;
    r.real = (a.real*b.real + a.imag*b.imag)/d;
    r.imag = (a.imag*b.real - a.real*b.imag)/d;
    return r;
    ******************************************************************/

    /* This algorithm is better, and is pretty obvious:  first divide the
     * numerators and denominator by whichever of {b.real, b.imag} has
     * larger magnitude.  The earliest reference I found was to CACM
     * Algorithm 116 (Complex Division, Robert L. Smith, Stanford
     * University).
     */
     Py_complex r;      /* the result */
     const double abs_breal = b.real < 0 ? -b.real : b.real;
     const double abs_bimag = b.imag < 0 ? -b.imag : b.imag;

    if (abs_breal >= abs_bimag) {
        /* divide tops and bottom by b.real */
        if (abs_breal == 0.0) {
            errno = EDOM;
            r.real = r.imag = 0.0;
        }
        else {
            const double ratio = b.imag / b.real;
            const double denom = b.real + b.imag * ratio;
            r.real = (a.real + a.imag * ratio) / denom;
            r.imag = (a.imag - a.real * ratio) / denom;
        }
    }
    else if (abs_bimag >= abs_breal) {
        /* divide tops and bottom by b.imag */
        const double ratio = b.real / b.imag;
        const double denom = b.real * ratio + b.imag;
        assert(b.imag != 0.0);
        r.real = (a.real * ratio + a.imag) / denom;
        r.imag = (a.imag * ratio - a.real) / denom;
    }
    else {
        /* At least one of b.real or b.imag is a NaN */
        r.real = r.imag = Py_NAN;
    }

    /* Recover infinities and zeros that computed as nan+nanj.  See e.g.
       the C11, Annex G.5.2, routine _Cdivd(). */
    if (isnan(r.real) && isnan(r.imag)) {
        if ((isinf(a.real) || isinf(a.imag))
            && isfinite(b.real) && isfinite(b.imag))
        {
            const double x = copysign(isinf(a.real) ? 1.0 : 0.0, a.real);
            const double y = copysign(isinf(a.imag) ? 1.0 : 0.0, a.imag);
            r.real = INFINITY * (x*b.real + y*b.imag);
            r.imag = INFINITY * (y*b.real - x*b.imag);
        }
        else if ((isinf(abs_breal) || isinf(abs_bimag))
                 && isfinite(a.real) && isfinite(a.imag))
        {
            const double x = copysign(isinf(b.real) ? 1.0 : 0.0, b.real);
            const double y = copysign(isinf(b.imag) ? 1.0 : 0.0, b.imag);
            r.real = 0.0 * (a.real*x + a.imag*y);
            r.imag = 0.0 * (a.imag*x - a.real*y);
        }
    }

    return r;
}

Py_complex
_Py_cr_quot(Py_complex a, double b)
{
    Py_complex r = a;
    if (b) {
        r.real /= b;
        r.imag /= b;
    }
    else {
        errno = EDOM;
        r.real = r.imag = 0.0;
    }
    return r;
}

/* an equivalent of _Py_c_quot() function, when 1st argument is real */
Py_complex
_Py_rc_quot(double a, Py_complex b)
{
    Py_complex r;
    const double abs_breal = b.real < 0 ? -b.real : b.real;
    const double abs_bimag = b.imag < 0 ? -b.imag : b.imag;

    if (abs_breal >= abs_bimag) {
        if (abs_breal == 0.0) {
            errno = EDOM;
            r.real = r.imag = 0.0;
        }
        else {
            const double ratio = b.imag / b.real;
            const double denom = b.real + b.imag * ratio;
            r.real = a / denom;
            r.imag = (-a * ratio) / denom;
        }
    }
    else if (abs_bimag >= abs_breal) {
        const double ratio = b.real / b.imag;
        const double denom = b.real * ratio + b.imag;
        assert(b.imag != 0.0);
        r.real = (a * ratio) / denom;
        r.imag = (-a) / denom;
    }
    else {
        r.real = r.imag = Py_NAN;
    }

    if (isnan(r.real) && isnan(r.imag) && isfinite(a)
        && (isinf(abs_breal) || isinf(abs_bimag)))
    {
        const double x = copysign(isinf(b.real) ? 1.0 : 0.0, b.real);
        const double y = copysign(isinf(b.imag) ? 1.0 : 0.0, b.imag);
        r.real = 0.0 * (a*x);
        r.imag = 0.0 * (-a*y);
    }

    return r;
}
#ifdef _M_ARM64
#pragma optimize("", on)
#endif

Py_complex
_Py_c_pow(Py_complex a, Py_complex b)
{
    Py_complex r;
    double vabs,len,at,phase;
    if (b.real == 0. && b.imag == 0.) {
        r.real = 1.;
        r.imag = 0.;
    }
    else if (a.real == 0. && a.imag == 0.) {
        if (b.imag != 0. || b.real < 0.)
            errno = EDOM;
        r.real = 0.;
        r.imag = 0.;
    }
    else {
        vabs = hypot(a.real,a.imag);
        len = pow(vabs,b.real);
        at = atan2(a.imag, a.real);
        phase = at*b.real;
        if (b.imag != 0.0) {
            len *= exp(-at*b.imag);
            phase += b.imag*log(vabs);
        }
        r.real = len*cos(phase);
        r.imag = len*sin(phase);

        /* Don't rely on errno set by the math functions above, for
           example cos() and sin() set EDOM for an infinite phase. */
        errno = 0;
        if (isfinite(a.real) && isfinite(a.imag)
            && isfinite(b.real) && isfinite(b.imag))
        {
            _Py_ADJUST_ERANGE2(r.real, r.imag);
        }
    }
    return r;
}

#define INT_EXP_CUTOFF 100

static Py_complex
c_powu(Py_complex x, long n)
{
    assert(0 < n && n <= INT_EXP_CUTOFF);
    while ((n & 1) == 0) {
        x = _Py_c_prod(x, x);
        n >>= 1;
    }
    Py_complex r = x;
    n >>= 1;
    while (n) {
        x = _Py_c_prod(x, x);
        if (n & 1) {
            r = _Py_c_prod(r, x);
        }
        n >>= 1;
    }
    return r;
}

static Py_complex
c_powi(Py_complex x, long n)
{
    if (n > 0)
        return c_powu(x, n);
    else if (n < 0)
        return _Py_rc_quot(1.0, c_powu(x, -n));
    else
        return (Py_complex){1., 0.};
}

double
_Py_c_abs(Py_complex z)
{
    /* sets errno = ERANGE on overflow */
    double result;
    int saved_errno = errno;

    if (!isfinite(z.real) || !isfinite(z.imag)) {
        /* C99 rules: if either the real or the imaginary part is an
           infinity, return infinity, even if the other part is a
           NaN. */
        if (isinf(z.real)) {
            result = fabs(z.real);
            errno = saved_errno;
            return result;
        }
        if (isinf(z.imag)) {
            result = fabs(z.imag);
            errno = saved_errno;
            return result;
        }
        /* either the real or imaginary part is a NaN,
           and neither is infinite. Result should be NaN. */
        errno = saved_errno;
        return Py_NAN;
    }
    result = hypot(z.real, z.imag);
    if (!isfinite(result))
        errno = ERANGE;
    else
        errno = saved_errno;
    return result;
}

static PyObject *
complex_subtype_from_c_complex(PyTypeObject *type, Py_complex cval)
{
    PyObject *op;

    op = type->tp_alloc(type, 0);
    if (op != NULL)
        ((PyComplexObject *)op)->cval = cval;
    return op;
}

PyObject *
PyComplex_FromCComplex(Py_complex cval)
{
    PyComplexObject *op = _Py_FREELIST_POP(PyComplexObject, complexes);

    if (op == NULL) {
        /* Inline PyObject_New */
        op = PyObject_Malloc(sizeof(PyComplexObject));
        if (op == NULL) {
            return PyErr_NoMemory();
        }
        _PyObject_Init((PyObject*)op, &PyComplex_Type);
    }
    op->cval = cval;
    return (PyObject *) op;
}

static void
complex_dealloc(PyObject *op)
{
    assert(PyComplex_Check(op));
    if (PyComplex_CheckExact(op)) {
        _Py_FREELIST_FREE(complexes, op, PyObject_Free);
    }
    else {
        Py_TYPE(op)->tp_free(op);
    }
}

static PyObject *
complex_subtype_from_doubles(PyTypeObject *type, double real, double imag)
{
    Py_complex c;
    c.real = real;
    c.imag = imag;
    return complex_subtype_from_c_complex(type, c);
}

PyObject *
PyComplex_FromDoubles(double real, double imag)
{
    Py_complex c;
    c.real = real;
    c.imag = imag;
    return PyComplex_FromCComplex(c);
}

static PyObject * try_complex_special_method(PyObject *);

double
PyComplex_RealAsDouble(PyObject *op)
{
    double real = -1.0;

    if (PyComplex_Check(op)) {
        real = ((PyComplexObject *)op)->cval.real;
    }
    else {
        PyObject* newop = try_complex_special_method(op);
        if (newop) {
            real = ((PyComplexObject *)newop)->cval.real;
            Py_DECREF(newop);
        } else if (!PyErr_Occurred()) {
            real = _PyValue_AsDouble(PyValue_FromObjectBorrow(op));
        }
    }

    return real;
}

double
PyComplex_ImagAsDouble(PyObject *op)
{
    double imag = -1.0;

    if (PyComplex_Check(op)) {
        imag = ((PyComplexObject *)op)->cval.imag;
    }
    else {
        PyObject* newop = try_complex_special_method(op);
        if (newop) {
            imag = ((PyComplexObject *)newop)->cval.imag;
            Py_DECREF(newop);
        } else if (!PyErr_Occurred()) {
            _PyValue_AsDouble(PyValue_FromObjectBorrow(op));
            if (!PyErr_Occurred()) {
                imag = 0.0;
            }
        }
    }

    return imag;
}

static PyObject *
try_complex_special_method(PyObject *op)
{
    PyObject *f;

    f = _PyObject_LookupSpecial(op, &_Py_ID(__complex__));
    if (f) {
        PyObject *res = _PyObject_CallNoArgs(f);
        Py_DECREF(f);
        if (!res || PyComplex_CheckExact(res)) {
            return res;
        }
        if (!PyComplex_Check(res)) {
            PyErr_Format(PyExc_TypeError,
                "%T.__complex__() must return a complex, not %T",
                op, res);
            Py_DECREF(res);
            return NULL;
        }
        /* Issue #29894: warn if 'res' not of exact type complex. */
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                "%T.__complex__() must return a complex, not %T.  "
                "The ability to return an instance of a strict subclass of complex "
                "is deprecated, and may be removed in a future version of Python.",
                op, res)) {
            Py_DECREF(res);
            return NULL;
        }
        return res;
    }
    return NULL;
}

Py_complex
PyComplex_AsCComplex(PyObject *op)
{
    Py_complex cv;
    PyObject *newop = NULL;

    assert(op);
    /* If op is already of type PyComplex_Type, return its value */
    if (PyComplex_Check(op)) {
        return ((PyComplexObject *)op)->cval;
    }
    /* If not, use op's __complex__  method, if it exists */

    /* return -1 on failure */
    cv.real = -1.;
    cv.imag = 0.;

    newop = try_complex_special_method(op);

    if (newop) {
        cv = ((PyComplexObject *)newop)->cval;
        Py_DECREF(newop);
        return cv;
    }
    else if (PyErr_Occurred()) {
        return cv;
    }
    /* If neither of the above works, interpret op as a float giving the
       real part of the result, and fill in the imaginary part as 0. */
    else {
        /* PyFloat_AsDouble will return -1 on failure */
        cv.real = _PyValue_AsDouble(PyValue_FromObjectBorrow(op));
        return cv;
    }
}

static PyObject *
complex_repr(PyObject *op)
{
    int precision = 0;
    char format_code = 'r';
    PyObject *result = NULL;
    PyComplexObject *v = _PyComplexObject_CAST(op);

    /* If these are non-NULL, they'll need to be freed. */
    char *pre = NULL;
    char *im = NULL;

    /* These do not need to be freed. re is either an alias
       for pre or a pointer to a constant.  lead and tail
       are pointers to constants. */
    const char *re = NULL;
    const char *lead = "";
    const char *tail = "";

    if (v->cval.real == 0. && copysign(1.0, v->cval.real)==1.0) {
        /* Real part is +0: just output the imaginary part and do not
           include parens. */
        re = "";
        im = PyOS_double_to_string(v->cval.imag, format_code,
                                   precision, 0, NULL);
        if (!im) {
            PyErr_NoMemory();
            goto done;
        }
    } else {
        /* Format imaginary part with sign, real part without. Include
           parens in the result. */
        pre = PyOS_double_to_string(v->cval.real, format_code,
                                    precision, 0, NULL);
        if (!pre) {
            PyErr_NoMemory();
            goto done;
        }
        re = pre;

        im = PyOS_double_to_string(v->cval.imag, format_code,
                                   precision, Py_DTSF_SIGN, NULL);
        if (!im) {
            PyErr_NoMemory();
            goto done;
        }
        lead = "(";
        tail = ")";
    }
    result = PyUnicode_FromFormat("%s%s%sj%s", lead, re, im, tail);
  done:
    PyMem_Free(im);
    PyMem_Free(pre);

    return result;
}

static Py_hash_t
complex_hash(PyObject *op)
{
    Py_uhash_t hashreal, hashimag, combined;
    PyComplexObject *v = _PyComplexObject_CAST(op);
    hashreal = (Py_uhash_t)_Py_HashDouble(op, v->cval.real);
    if (hashreal == (Py_uhash_t)-1)
        return -1;
    hashimag = (Py_uhash_t)_Py_HashDouble(op, v->cval.imag);
    if (hashimag == (Py_uhash_t)-1)
        return -1;
    /* Note:  if the imaginary part is 0, hashimag is 0 now,
     * so the following returns hashreal unchanged.  This is
     * important because numbers of different types that
     * compare equal must have the same hash value, so that
     * hash(x + 0*j) must equal hash(x).
     */
    combined = hashreal + PyHASH_IMAG * hashimag;
    if (combined == (Py_uhash_t)-1)
        combined = (Py_uhash_t)-2;
    return (Py_hash_t)combined;
}

/* Complex arithmetic rules implement special mixed-mode case where combining
   a pure-real (float or int) value and a complex value is performed directly
   without first coercing the real value to a complex value.

   Let us consider the addition as an example, assuming that ints are implicitly
   converted to floats.  We have the following rules (up to variants with changed
   order of operands):

       complex(a, b) + complex(c, d) = complex(a + c, b + d)
       float(a) + complex(b, c) = complex(a + b, c)

   Similar rules are implemented for subtraction, multiplication and division.
   See C11's Annex G, sections G.5.1 and G.5.2.
 */

/* The real operand of a mixed-mode operation, converted as
   _Py_convert_int_to_double() does for objects. Returns 0 with *dbl set, 1 for an
   operand that is not a real number (the caller returns NotImplemented), or
   -1 with an exception set. */
static int
complex_real_operand(PyValueBorrow v, double *dbl)
{
    if (PyValue_IsInlineFloat(v)) {
        *dbl = PyValue_InlineFloat(v);
        return 0;
    }
    if (PyValue_IsInlineInt(v)) {
        *dbl = (double)PyValue_InlineInt64(v);
        return 0;
    }
    PyObject *obj = PyValue_AsKnownObjectBorrow(v);
    if (PyFloat_Check(obj)) {
        *dbl = PyFloat_AS_DOUBLE(obj);
        return 0;
    }
    if (PyLong_Check(obj)) {
        *dbl = _PyValue_IntAsDouble(PyValue_FromObjectBorrow(obj));
        return (*dbl == -1.0 && PyErr_Occurred()) ? -1 : 0;
    }
    return 1;
}

static bool
complex_is_complex(PyValueBorrow v)
{
    return PyValue_IsObject(v) && PyComplex_Check(PyValue_AsKnownObjectBorrow(v));
}

static Py_complex
complex_cval(PyValueBorrow v)
{
    return ((PyComplexObject *)PyValue_AsKnownObjectBorrow(v))->cval;
}

#define COMPLEX_BINOP(NAME, FUNC)                                   \
    static PyValue                                                  \
    complex_##NAME##_value(PyValueBorrow v, PyValueBorrow w)        \
    {                                                               \
        Py_complex a;                                               \
        errno = 0;                                                  \
        if (complex_is_complex(w)) {                                \
            Py_complex b = complex_cval(w);                         \
            if (complex_is_complex(v)) {                            \
                a = _Py_c_##FUNC(complex_cval(v), b);               \
            }                                                       \
            else {                                                  \
                double real;                                        \
                int rc = complex_real_operand(v, &real);            \
                if (rc != 0) {                                      \
                    return rc < 0 ? PyValue_NULL : PyValue_NotImplemented; \
                }                                                   \
                a = _Py_rc_##FUNC(real, b);                         \
            }                                                       \
        }                                                           \
        else if (!complex_is_complex(v)) {                          \
            return PyValue_NotImplemented;                          \
        }                                                           \
        else {                                                      \
            double b;                                               \
            int rc = complex_real_operand(w, &b);                   \
            if (rc != 0) {                                          \
                return rc < 0 ? PyValue_NULL : PyValue_NotImplemented; \
            }                                                       \
            a = _Py_cr_##FUNC(complex_cval(v), b);                  \
        }                                                           \
        if (errno == EDOM) {                                        \
            PyErr_SetString(PyExc_ZeroDivisionError,                \
                            "division by zero");                    \
            return PyValue_NULL;                                    \
        }                                                           \
        return PyValue_FromObjectSteal(PyComplex_FromCComplex(a));  \
   }                                                                \
   _Py_VALUE_THUNK_BINARY(complex_##NAME, complex_##NAME##_value)

COMPLEX_BINOP(add, sum)
COMPLEX_BINOP(mul, prod)
COMPLEX_BINOP(sub, diff)
COMPLEX_BINOP(div, quot)

static int
complex_any_operand(PyValueBorrow v, Py_complex *c)
{
    if (complex_is_complex(v)) {
        *c = complex_cval(v);
        return 0;
    }
    c->imag = 0.0;
    return complex_real_operand(v, &c->real);
}

static PyValue
complex_pow_value(PyValueBorrow v, PyValueBorrow w, PyValueBorrow z)
{
    Py_complex p;
    Py_complex a, b;
    int rc = complex_any_operand(v, &a);
    if (rc == 0) {
        rc = complex_any_operand(w, &b);
    }
    if (rc != 0) {
        return rc < 0 ? PyValue_NULL : PyValue_NotImplemented;
    }

    if (!PyValue_Is(z, PyValue_None)) {
        PyErr_SetString(PyExc_ValueError, "complex modulo");
        return PyValue_NULL;
    }
    errno = 0;
    // Check whether the exponent has a small integer value, and if so use
    // a faster and more accurate algorithm.
    if (b.imag == 0.0 && b.real == floor(b.real)
        && fabs(b.real) <= INT_EXP_CUTOFF)
    {
        p = c_powi(a, (long)b.real);
        if (isfinite(a.real) && isfinite(a.imag)) {
            _Py_ADJUST_ERANGE2(p.real, p.imag);
        }
    }
    else {
        p = _Py_c_pow(a, b);
    }

    if (errno == EDOM) {
        PyErr_SetString(PyExc_ZeroDivisionError,
                        "zero to a negative or complex power");
        return PyValue_NULL;
    }
    else if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError,
                        "complex exponentiation");
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(PyComplex_FromCComplex(p));
}
_Py_VALUE_THUNK_TERNARY(complex_pow, complex_pow_value)

static PyValue
complex_neg_value(PyValueBorrow op)
{
    Py_complex neg = complex_cval(op);
    neg.real = -neg.real;
    neg.imag = -neg.imag;
    return PyValue_FromObjectSteal(PyComplex_FromCComplex(neg));
}
_Py_VALUE_THUNK_UNARY(complex_neg, complex_neg_value)

static PyValue
complex_pos_value(PyValueBorrow op)
{
    PyObject *v = PyValue_AsKnownObjectBorrow(op);
    if (PyComplex_CheckExact(v)) {
        return PyValue_FromObjectSteal(Py_NewRef(v));
    }
    return PyValue_FromObjectSteal(PyComplex_FromCComplex(complex_cval(op)));
}
_Py_VALUE_THUNK_UNARY(complex_pos, complex_pos_value)

static PyValue
complex_abs_value(PyValueBorrow op)
{
    double result;

    errno = 0;
    result = _Py_c_abs(complex_cval(op));
    if (errno == ERANGE) {
        PyErr_SetString(PyExc_OverflowError,
                        "absolute value too large");
        return PyValue_NULL;
    }
    return PyValue_FromDouble(result);
}
_Py_VALUE_THUNK_UNARY(complex_abs, complex_abs_value)

static int
complex_bool_value(PyValueBorrow op)
{
    Py_complex c = complex_cval(op);
    return c.real != 0.0 || c.imag != 0.0;
}
_Py_VALUE_THUNK_INQUIRY(complex_bool, complex_bool_value)

static PyValue
complex_richcompare_value(PyValueBorrow v, PyValueBorrow w, int op)
{
    Py_complex i;
    int equal;

    if (op != Py_EQ && op != Py_NE) {
        return PyValue_NotImplemented;
    }

    assert(complex_is_complex(v));
    i = complex_cval(v);

    if (PyValue_IsInlineInt(w) ||
        (PyValue_IsObject(w) && PyLong_Check(PyValue_AsKnownObjectBorrow(w)))) {
        /* Check for 0.0 imaginary part first to avoid the rich
         * comparison when possible.
         */
        if (i.imag == 0.0) {
            return _PyValue_RichCompare(PyValue_FromDouble(i.real), w, op);
        }
        equal = 0;
    }
    else if (PyValue_IsInlineFloat(w)) {
        equal = (i.real == PyValue_InlineFloat(w) && i.imag == 0.0);
    }
    else if (PyFloat_Check(PyValue_AsKnownObjectBorrow(w))) {
        equal = (i.real == PyFloat_AS_DOUBLE(PyValue_AsKnownObjectBorrow(w))
                 && i.imag == 0.0);
    }
    else if (complex_is_complex(w)) {
        Py_complex j = complex_cval(w);
        equal = (i.real == j.real && i.imag == j.imag);
    }
    else {
        return PyValue_NotImplemented;
    }

    return PyValue_FromBool(equal == (op == Py_EQ));
}
_Py_VALUE_THUNK_RICHCMP(complex_richcompare, complex_richcompare_value)

/*[clinic input]
@permit_long_summary
complex.conjugate

Return the complex conjugate of its argument. (3-4j).conjugate() == 3+4j.
[clinic start generated code]*/

static PyValue
complex_conjugate_impl(PyComplexObject *self)
/*[clinic end generated code: output=00a9731e81fcfdec input=71b8ab003e1cec95]*/
{
    Py_complex c = self->cval;
    c.imag = -c.imag;
    return _PyValue_FromObjectResult(PyComplex_FromCComplex(c));
}

/*[clinic input]
complex.__getnewargs__

[clinic start generated code]*/

static PyValue
complex___getnewargs___impl(PyComplexObject *self)
/*[clinic end generated code: output=c6fadfb85ff4abfb input=539543e0a50533d7]*/
{
    Py_complex c = self->cval;
    return _PyValue_FromObjectResult(Py_BuildValue("(dd)", c.real, c.imag));
}


/*[clinic input]
complex.__format__

    format_spec: unicode
    /

Convert to a string according to format_spec.
[clinic start generated code]*/

static PyValue
complex___format___impl(PyComplexObject *self, PyObject *format_spec)
/*[clinic end generated code: output=891f508a8a3f5f80 input=014ef5488acbe1d5]*/
{
    _PyUnicodeWriter writer;
    int ret;
    _PyUnicodeWriter_Init(&writer);
    ret = _PyComplex_FormatAdvancedWriter(
        &writer,
        (PyObject *)self,
        format_spec, 0, PyUnicode_GET_LENGTH(format_spec));
    if (ret == -1) {
        _PyUnicodeWriter_Dealloc(&writer);
        return PyValue_NULL;
    }
    return _PyValue_FromObjectResult(_PyUnicodeWriter_Finish(&writer));
}

/*[clinic input]
complex.__complex__

Convert this value to exact type complex.
[clinic start generated code]*/

static PyValue
complex___complex___impl(PyComplexObject *self)
/*[clinic end generated code: output=0fbff554ae55d086 input=3589ada9d27db854]*/
{
    if (PyComplex_CheckExact(self)) {
        return PyValue_DUP(PyValue_FromObjectBorrow(self));
    }
    else {
        return _PyValue_FromObjectResult(PyComplex_FromCComplex(self->cval));
    }
}


static PyObject *
complex_from_string_inner(const char *s, Py_ssize_t len, void *type)
{
    double x=0.0, y=0.0, z;
    int got_bracket=0;
    const char *start;
    char *end;

    /* position on first nonblank */
    start = s;
    while (Py_ISSPACE(*s))
        s++;
    if (*s == '(') {
        /* Skip over possible bracket from repr(). */
        got_bracket = 1;
        s++;
        while (Py_ISSPACE(*s))
            s++;
    }

    /* a valid complex string usually takes one of the three forms:

         <float>                  - real part only
         <float>j                 - imaginary part only
         <float><signed-float>j   - real and imaginary parts

       where <float> represents any numeric string that's accepted by the
       float constructor (including 'nan', 'inf', 'infinity', etc.), and
       <signed-float> is any string of the form <float> whose first
       character is '+' or '-'.

       For backwards compatibility, the extra forms

         <float><sign>j
         <sign>j
         j

       are also accepted, though support for these forms may be removed from
       a future version of Python.
    */

    /* first look for forms starting with <float> */
    z = PyOS_string_to_double(s, &end, NULL);
    if (z == -1.0 && PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_ValueError))
            PyErr_Clear();
        else
            return NULL;
    }
    if (end != s) {
        /* all 4 forms starting with <float> land here */
        s = end;
        if (*s == '+' || *s == '-') {
            /* <float><signed-float>j | <float><sign>j */
            x = z;
            y = PyOS_string_to_double(s, &end, NULL);
            if (y == -1.0 && PyErr_Occurred()) {
                if (PyErr_ExceptionMatches(PyExc_ValueError))
                    PyErr_Clear();
                else
                    return NULL;
            }
            if (end != s)
                /* <float><signed-float>j */
                s = end;
            else {
                /* <float><sign>j */
                y = *s == '+' ? 1.0 : -1.0;
                s++;
            }
            if (!(*s == 'j' || *s == 'J'))
                goto parse_error;
            s++;
        }
        else if (*s == 'j' || *s == 'J') {
            /* <float>j */
            s++;
            y = z;
        }
        else
            /* <float> */
            x = z;
    }
    else {
        /* not starting with <float>; must be <sign>j or j */
        if (*s == '+' || *s == '-') {
            /* <sign>j */
            y = *s == '+' ? 1.0 : -1.0;
            s++;
        }
        else
            /* j */
            y = 1.0;
        if (!(*s == 'j' || *s == 'J'))
            goto parse_error;
        s++;
    }

    /* trailing whitespace and closing bracket */
    while (Py_ISSPACE(*s))
        s++;
    if (got_bracket) {
        /* if there was an opening parenthesis, then the corresponding
           closing parenthesis should be right here */
        if (*s != ')')
            goto parse_error;
        s++;
        while (Py_ISSPACE(*s))
            s++;
    }

    /* we should now be at the end of the string */
    if (s-start != len)
        goto parse_error;

    return complex_subtype_from_doubles(_PyType_CAST(type), x, y);

  parse_error:
    PyErr_SetString(PyExc_ValueError,
                    "complex() arg is a malformed string");
    return NULL;
}

static PyObject *
complex_subtype_from_string(PyTypeObject *type, PyObject *v)
{
    const char *s;
    PyObject *s_buffer = NULL, *result = NULL;
    Py_ssize_t len;

    if (PyUnicode_Check(v)) {
        s_buffer = _PyUnicode_TransformDecimalAndSpaceToASCII(v);
        if (s_buffer == NULL) {
            return NULL;
        }
        assert(PyUnicode_IS_ASCII(s_buffer));
        /* Simply get a pointer to existing ASCII characters. */
        s = PyUnicode_AsUTF8AndSize(s_buffer, &len);
        assert(s != NULL);
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "complex() argument must be a string or a number, not %T",
            v);
        return NULL;
    }

    result = _Py_string_to_number_with_underscores(s, len, "complex", v, type,
                                                   complex_from_string_inner);
    Py_DECREF(s_buffer);
    return result;
}

/* The constructor should only accept a string as a positional argument,
 * not as by the 'real' keyword.  But Argument Clinic does not allow
 * to distinguish between argument passed positionally and by keyword.
 * So the constructor must be split into two parts: actual_complex_new()
 * handles the case of no arguments and one positional argument, and calls
 * complex_new(), implemented with Argument Clinic, to handle the remaining
 * cases: 'real' and 'imag' arguments.  This separation is well suited
 * for different constructor roles: converting a string or number to a complex
 * number and constructing a complex number from real and imaginary parts.
 */
static PyObject *
actual_complex_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *res = NULL;
    PyNumberMethods *nbr;

    if (PyTuple_GET_SIZE(args) > 1 || (kwargs != NULL && PyDict_GET_SIZE(kwargs))) {
        return complex_new(type, args, kwargs);
    }
    if (!PyTuple_GET_SIZE(args)) {
        return complex_subtype_from_doubles(type, 0, 0);
    }

    PyValue v = PyTuple_GetValue(args, 0);
    PyObject *arg = PyValue_IsNull(v) ? NULL : _PyValue_AsObjectAt(v, BOX_SITE_TUPLE);
    PyValue_XCLOSE(v);
    if (arg == NULL) {
        return NULL;
    }
    /* Special-case for a single argument when type(arg) is complex. */
    if (PyComplex_CheckExact(arg) && type == &PyComplex_Type) {
        /* Note that we can't know whether it's safe to return
           a complex *subclass* instance as-is, hence the restriction
           to exact complexes here.  If either the input or the
           output is a complex subclass, it will be handled below
           as a non-orthogonal vector.  */
        return arg;
    }
    if (PyUnicode_Check(arg)) {
        res = complex_subtype_from_string(type, arg);
        Py_DECREF(arg);
        return res;
    }
    PyObject *tmp = try_complex_special_method(arg);
    if (tmp) {
        Py_complex c = ((PyComplexObject*)tmp)->cval;
        res = complex_subtype_from_doubles(type, c.real, c.imag);
        Py_DECREF(tmp);
    }
    else if (PyErr_Occurred()) {
        Py_DECREF(arg);
        return NULL;
    }
    else if (PyComplex_Check(arg)) {
        /* Note that if arg is of a complex subtype, we're only
           retaining its real & imag parts here, and the return
           value is (properly) of the builtin complex type. */
        Py_complex c = ((PyComplexObject*)arg)->cval;
        res = complex_subtype_from_doubles(type, c.real, c.imag);
    }
    else if ((nbr = Py_TYPE(arg)->tp_as_number) != NULL &&
             (nbr->nb_float != NULL || nbr->nb_index != NULL))
    {
        /* The argument really is entirely real, and contributes
           nothing in the imaginary direction.
           Just treat it as a double. */
        double r = _PyValue_AsDouble(PyValue_FromObjectBorrow(arg));
        if (r != -1.0 || !PyErr_Occurred()) {
            res = complex_subtype_from_doubles(type, r, 0);
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "complex() argument must be a string or a number, not %T",
                     arg);
    }
    Py_DECREF(arg);
    return res;
}

/*[clinic input]
@classmethod
complex.__new__ as complex_new
    real as r: object(c_default="NULL") = 0
    imag as i: object(c_default="NULL") = 0

Create a complex number from a string or numbers.

If a string is given, parse it as a complex number.
If a single number is given, convert it to a complex number.
If the 'real' or 'imag' arguments are given, create a complex number
with the specified real and imaginary components.
[clinic start generated code]*/

static PyValue
complex_new_impl(PyTypeObject *type, PyValueBorrow r, PyValueBorrow i)
/*[clinic end generated code: output=e78ddd99d3c6a63e input=ff4268dc540958a4]*/
{
    PyObject *tmp;
    PyNumberMethods *nbr, *nbi = NULL;
    Py_complex cr, ci;
    PyValue r_owned = PyValue_NULL;  // the result of __complex__, if any
    int own_r = 0;
    int cr_is_complex = 0;
    int ci_is_complex = 0;

    if (PyValue_IsNull(r)) {
        r = PyValue_FromObjectBorrow(_PyLong_GetZero());
    }
    PyValueBorrow orig_r = r;

    /* DEPRECATED: The call of try_complex_special_method() for the "real"
     * part will be dropped after the end of the deprecation period.
     * An inline int or float has no __complex__. */
    if (PyValue_IsObject(r)) {
        tmp = try_complex_special_method(PyValue_AsKnownObjectBorrow(r));
        if (tmp) {
            r_owned = PyValue_FromObjectSteal(tmp);
            r = PyValue_Borrow(r_owned);
            own_r = 1;
        }
        else if (PyErr_Occurred()) {
            return PyValue_NULL;
        }
    }

    nbr = PyValue_TYPE(r)->tp_as_number;
    if (nbr == NULL ||
        (nbr->nb_float == NULL && nbr->nb_index == NULL && !(PyValue_IsObject(r) && PyComplex_Check(PyValue_AsKnownObjectBorrow(r)))))
    {
        PyErr_Format(PyExc_TypeError,
                     "complex() argument 'real' must be a real number, not %N",
                     PyValue_TYPE(r));
        if (own_r) {
            PyValue_CLOSE(r_owned);
        }
        return PyValue_NULL;
    }
    if (!PyValue_IsNull(i)) {
        nbi = PyValue_TYPE(i)->tp_as_number;
        if (nbi == NULL ||
            (nbi->nb_float == NULL && nbi->nb_index == NULL && !(PyValue_IsObject(i) && PyComplex_Check(PyValue_AsKnownObjectBorrow(i)))))
        {
            PyErr_Format(PyExc_TypeError,
                         "complex() argument 'imag' must be a real number, not %N",
                         PyValue_TYPE(i));
            if (own_r) {
                PyValue_CLOSE(r_owned);
            }
            return PyValue_NULL;
        }
    }

    /* If we get this far, then the "real" and "imag" parts should
       both be treated as numbers, and the constructor should return a
       complex number equal to (real + imag*1j).

       The following is DEPRECATED:
       Note that we do NOT assume the input to already be in canonical
       form; the "real" and "imag" parts might themselves be complex
       numbers, which slightly complicates the code below. */
    if ((PyValue_IsObject(r) && PyComplex_Check(PyValue_AsKnownObjectBorrow(r)))) {
        /* Note that if r is of a complex subtype, we're only
           retaining its real & imag parts here, and the return
           value is (properly) of the builtin complex type. */
        cr = ((PyComplexObject*)PyValue_AsKnownObjectBorrow(r))->cval;
        cr_is_complex = 1;
        if (own_r) {
            /* r was a newly created complex number, rather
               than the original "real" argument. */
            PyValue_CLOSE(r_owned);
        }
        nbr = PyValue_TYPE(orig_r)->tp_as_number;
        if (nbr == NULL ||
            (nbr->nb_float == NULL && nbr->nb_index == NULL))
        {
            if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                    "complex() argument 'real' must be a real number, not %N",
                    PyValue_TYPE(orig_r))) {
                return PyValue_NULL;
            }
        }
    }
    else {
        /* The "real" part really is entirely real, and contributes
           nothing in the imaginary direction.
           Just treat it as a double. */
        PyValue fr = _PyValue_Float(r);
        assert(!own_r);
        if (PyValue_IsNull(fr))
            return PyValue_NULL;
        assert(PyValue_FloatCheck(fr));
        cr.real = _PyValue_AsDouble(fr);
        cr.imag = 0.0;
        PyValue_CLOSE(fr);
    }
    if (PyValue_IsNull(i)) {
        ci.real = cr.imag;
    }
    else if ((PyValue_IsObject(i) && PyComplex_Check(PyValue_AsKnownObjectBorrow(i)))) {
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                "complex() argument 'imag' must be a real number, not %N",
                PyValue_TYPE(i))) {
            return PyValue_NULL;
        }
        ci = ((PyComplexObject*)PyValue_AsKnownObjectBorrow(i))->cval;
        ci_is_complex = 1;
    } else {
        /* The "imag" part really is entirely imaginary, and
           contributes nothing in the real direction.
           Just treat it as a double. */
        PyValue fi = _PyValue_Float(i);
        if (PyValue_IsNull(fi))
            return PyValue_NULL;
        ci.real = _PyValue_AsDouble(fi);
        PyValue_CLOSE(fi);
    }
    /*  If the input was in canonical form, then the "real" and "imag"
        parts are real numbers, so that ci.imag and cr.imag are zero.
        We need this correction in case they were not real numbers. */

    if (ci_is_complex) {
        cr.real -= ci.imag;
    }
    if (cr_is_complex && !PyValue_IsNull(i)) {
        ci.real += cr.imag;
    }
    return _PyValue_FromObjectResult(complex_subtype_from_doubles(type, cr.real, ci.real));
}

/*[clinic input]
@classmethod
complex.from_number

    number: object
    /

Convert number to a complex floating-point number.
[clinic start generated code]*/

static PyValue
complex_from_number_impl(PyTypeObject *type, PyValueBorrow number)
/*[clinic end generated code: output=412f3832b6958704 input=3f8bdd3a2bc3facd]*/
{
    if ((PyValue_IsObject(number) && PyComplex_CheckExact(PyValue_AsKnownObjectBorrow(number))) && type == &PyComplex_Type) {
        return PyValue_DUP(number);
    }
    Py_complex cv;
    if (PyValue_IsObject(number)) {
        cv = PyComplex_AsCComplex(PyValue_AsKnownObjectBorrow(number));
    }
    else {
        // An inline int or float has no __complex__: it is the real part.
        cv.real = _PyValue_AsDouble(number);
        cv.imag = 0.0;
    }
    if (cv.real == -1.0 && PyErr_Occurred()) {
        return PyValue_NULL;
    }
    PyObject *result = PyComplex_FromCComplex(cv);
    if (type != &PyComplex_Type && result != NULL) {
        Py_SETREF(result, PyObject_CallOneArg((PyObject *)type, result));
    }
    return _PyValue_FromObjectResult(result);
}

static PyMethodDef complex_methods[] = {
    COMPLEX_FROM_NUMBER_METHODDEF
    COMPLEX_CONJUGATE_METHODDEF
    COMPLEX___COMPLEX___METHODDEF
    COMPLEX___GETNEWARGS___METHODDEF
    COMPLEX___FORMAT___METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static PyMemberDef complex_members[] = {
    {"real", Py_T_DOUBLE, offsetof(PyComplexObject, cval.real), Py_READONLY,
     "the real part of a complex number"},
    {"imag", Py_T_DOUBLE, offsetof(PyComplexObject, cval.imag), Py_READONLY,
     "the imaginary part of a complex number"},
    {0},
};

static PyNumberMethods complex_as_number = {
    complex_add,                                /* nb_add */
    complex_sub,                                /* nb_subtract */
    complex_mul,                                /* nb_multiply */
    0,                                          /* nb_remainder */
    0,                                          /* nb_divmod */
    complex_pow,                                /* nb_power */
    complex_neg,                                /* nb_negative */
    complex_pos,                                /* nb_positive */
    complex_abs,                                /* nb_absolute */
    complex_bool,                               /* nb_bool */
    0,                                          /* nb_invert */
    0,                                          /* nb_lshift */
    0,                                          /* nb_rshift */
    0,                                          /* nb_and */
    0,                                          /* nb_xor */
    0,                                          /* nb_or */
    0,                                          /* nb_int */
    0,                                          /* nb_reserved */
    0,                                          /* nb_float */
    0,                                          /* nb_inplace_add */
    0,                                          /* nb_inplace_subtract */
    0,                                          /* nb_inplace_multiply*/
    0,                                          /* nb_inplace_remainder */
    0,                                          /* nb_inplace_power */
    0,                                          /* nb_inplace_lshift */
    0,                                          /* nb_inplace_rshift */
    0,                                          /* nb_inplace_and */
    0,                                          /* nb_inplace_xor */
    0,                                          /* nb_inplace_or */
    0,                                          /* nb_floor_divide */
    complex_div,                                /* nb_true_divide */
    0,                                          /* nb_inplace_floor_divide */
    0,                                          /* nb_inplace_true_divide */
};

PyTypeObject PyComplex_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "complex",
    sizeof(PyComplexObject),
    0,
    complex_dealloc,                            /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    complex_repr,                               /* tp_repr */
    &complex_as_number,                         /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    complex_hash,                               /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    complex_new__doc__,                         /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    complex_richcompare,                        /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    complex_methods,                            /* tp_methods */
    complex_members,                            /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    actual_complex_new,                         /* tp_new */
    PyObject_Free,                              /* tp_free */
    .tp_version_tag = _Py_TYPE_VERSION_COMPLEX,
    .tp_value = {
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, complex_richcompare_value),
        .nb_add = _Py_VALUE_SLOT_INIT(nb_add, complex_add_value),
        .nb_subtract = _Py_VALUE_SLOT_INIT(nb_subtract, complex_sub_value),
        .nb_multiply = _Py_VALUE_SLOT_INIT(nb_multiply, complex_mul_value),
        .nb_true_divide = _Py_VALUE_SLOT_INIT(nb_true_divide, complex_div_value),
        .nb_power = _Py_VALUE_SLOT_INIT(nb_power, complex_pow_value),
        .nb_negative = _Py_VALUE_SLOT_INIT(nb_negative, complex_neg_value),
        .nb_positive = _Py_VALUE_SLOT_INIT(nb_positive, complex_pos_value),
        .nb_absolute = _Py_VALUE_SLOT_INIT(nb_absolute, complex_abs_value),
        .nb_bool = _Py_VALUE_SLOT_INIT(nb_bool, complex_bool_value),
    },
};
