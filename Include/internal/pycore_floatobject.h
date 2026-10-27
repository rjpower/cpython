#ifndef Py_INTERNAL_FLOATOBJECT_H
#define Py_INTERNAL_FLOATOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_long.h"          // _PyLong_ExactDealloc()
#include "pycore_unicodeobject.h" // _PyUnicodeWriter

/* runtime lifecycle */

extern PyStatus _PyFloat_InitTypes(PyInterpreterState *);
extern void _PyFloat_FiniType(PyInterpreterState *);




PyAPI_FUNC(void) _PyFloat_ExactDealloc(PyObject *op);

// Deallocate an exact float or an exact int, the operands of specialized
// float arithmetic. Neither runs Python code.
static inline void
_PyFloat_OrLongExactDealloc(PyObject *op)
{
    if (PyFloat_CheckExact(op)) {
        _PyFloat_ExactDealloc(op);
    }
    else {
        _PyLong_ExactDealloc(op);
    }
}




/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
extern int _PyFloat_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

extern PyObject* _Py_string_to_number_with_underscores(
    const char *str, Py_ssize_t len, const char *what, PyObject *obj, void *arg,
    PyObject *(*innerfunc)(const char *, Py_ssize_t, void *));

extern double _Py_parse_inf_or_nan(const char *p, char **endptr);

extern int _Py_convert_int_to_double(PyObject **v, double *dbl);

/* float's %, divmod() and ** on doubles, shared by the float type's slots
   and the value number protocol (Objects/abstract.c).  The divisor of
   _PyFloat_Rem() and _PyFloat_DivMod() is not zero.  _PyFloat_Pow()
   returns 0 with the result in *res, -1 with an exception set, or 1 if the
   result is complex. */
extern double _PyFloat_Rem(double vx, double wx);
extern void _PyFloat_DivMod(double vx, double wx, double *floordiv, double *mod);
extern int _PyFloat_Pow(double iv, double iw, double *res);

/* Should match endianness of the platform in most (all?) cases. */

#ifdef DOUBLE_IS_BIG_ENDIAN_IEEE754
#  define _PY_FLOAT_BIG_ENDIAN 1
#  define _PY_FLOAT_LITTLE_ENDIAN 0
#else
#  define _PY_FLOAT_BIG_ENDIAN 0
#  define _PY_FLOAT_LITTLE_ENDIAN 1
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FLOATOBJECT_H */
