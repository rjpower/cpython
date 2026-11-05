#ifndef Py_INTERNAL_MODSUPPORT_H
#define Py_INTERNAL_MODSUPPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


// Export for 'array' shared extension
PyAPI_FUNC(int) _PyArg_NoKwnames(const char *funcname, PyObject *kwnames);
#define _PyArg_NoKwnames(funcname, kwnames) \
    ((kwnames) == NULL || _PyArg_NoKwnames((funcname), (kwnames)))

// Export for '_bz2' shared extension
PyAPI_FUNC(int) _PyArg_NoPositional(const char *funcname, PyObject *args);
#define _PyArg_NoPositional(funcname, args) \
    ((args) == NULL || _PyArg_NoPositional((funcname), (args)))

// Export for Argument Clinic generated code in shared extensions
PyAPI_FUNC(int) _PyArg_NoPositionalStack(const char *funcname,
                                         Py_ssize_t nargs);
#define _PyArg_NoPositionalStack(funcname, nargs) \
    ((nargs) == 0 || _PyArg_NoPositionalStack((funcname), (nargs)))

// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyArg_NoKeywords(const char *funcname, PyObject *kwargs);
#define _PyArg_NoKeywords(funcname, kwargs) \
    ((kwargs) == NULL || _PyArg_NoKeywords((funcname), (kwargs)))

// Export for 'zlib' shared extension
PyAPI_FUNC(int) _PyArg_CheckPositional(const char *, Py_ssize_t,
                                       Py_ssize_t, Py_ssize_t);
#define _PyArg_CheckPositional(funcname, nargs, min, max) \
    (((min) <= (nargs) && (nargs) <= (max)) \
     || _PyArg_CheckPositional((funcname), (nargs), (min), (max)))

extern PyObject ** _Py_VaBuildStack(
    PyObject **small_stack,
    Py_ssize_t small_stack_len,
    const char *format,
    va_list va,
    Py_ssize_t *p_nargs);

extern PyObject* _PyModule_CreateInitialized(PyModuleDef*, int apiver);

extern int _PyArg_UnpackStack(
    PyObject *const *args,
    Py_ssize_t nargs,
    const char *name,
    Py_ssize_t min,
    Py_ssize_t max,
    ...);

// Export for '_heapq' shared extension
PyAPI_FUNC(void) _PyArg_BadArgument(
    const char *fname,
    const char *displayname,
    const char *expected,
    PyObject *arg);

// --- _PyArg_Parser API ---------------------------------------------------

// Similar to public PyArg_ParseArrayAndKeywords(), but faster thanks
// to "struct _PyArg_Parser *parser" cache.
// Export for '_dbm' shared extension.
PyAPI_FUNC(int) _PyArg_ParseStackAndKeywords(
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwnames,
    struct _PyArg_Parser *,
    ...);

// Export for 'math' shared extension
PyAPI_FUNC(PyObject * const *) _PyArg_UnpackKeywords(
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs,
    PyObject *kwnames,
    struct _PyArg_Parser *parser,
    int minpos,
    int maxpos,
    int minkw,
    int varpos,
    PyObject **buf);
#define _PyArg_UnpackKeywords(args, nargs, kwargs, kwnames, parser, minpos, maxpos, minkw, varpos, buf) \
    (((minkw) == 0 && (kwargs) == NULL && (kwnames) == NULL && \
      (minpos) <= (nargs) && ((varpos) || (nargs) <= (maxpos)) && (args) != NULL) ? \
      (args) : \
     _PyArg_UnpackKeywords((args), (nargs), (kwargs), (kwnames), (parser), \
                           (minpos), (maxpos), (minkw), (varpos), (buf)))

// --- Value parsing --------------------------------------------------------
//
// The helpers of value-emission Argument Clinic output (pyjit/design/
// proposals/value-api-m3.md, section 2): a METH_VALUE function receives a
// window of words that its caller owns for the call, args[0..nargs) for the
// positional arguments followed by the values of kwnames.  Every message is
// upstream's, built from the value's type: no error path boxes.

// "f() argument 1 must be str, not int": _PyArg_BadArgument() of a value.
PyAPI_FUNC(void) _PyArg_BadArgumentValue(
    const char *fname,
    const char *displayname,
    const char *expected,
    PyValueBorrow arg);

// Reject keyword arguments with the message of the convention a METH_VALUE
// function replaces: funcstr is its _PyObject_FunctionStr() ("list.clear()").
PyAPI_FUNC(int) _PyArg_NoKwnamesValue(const char *funcstr, PyObject *kwnames);
#define _PyArg_NoKwnamesValue(funcstr, kwnames) \
    ((kwnames) == NULL || _PyArg_NoKwnamesValue((funcstr), (kwnames)))

// The same check for a method of the static type tp, named as its
// descriptor's __qualname__ shows it ("TypeVar.__reduce__()").
PyAPI_FUNC(int) _PyArg_NoKwnamesValueMethod(PyTypeObject *tp, const char *name,
                                            PyObject *kwnames);
#define _PyArg_NoKwnamesValueMethod(tp, name, kwnames) \
    ((kwnames) == NULL || _PyArg_NoKwnamesValueMethod((tp), (name), (kwnames)))

// _PyEval_SliceIndex() and _PyEval_SliceIndexNotNone() of a value: the
// slice_index converter.
// Py_ssize_t(accept={int, NoneType}): the value twins of
// _Py_convert_optional_to_ssize_t() and its non-negative form.
PyAPI_FUNC(int) _Py_convert_optional_to_ssize_t_value(PyValueBorrow v,
                                                      void *result);
PyAPI_FUNC(int) _Py_convert_optional_to_non_negative_ssize_t_value(
    PyValueBorrow v, void *result);

PyAPI_FUNC(int) _PyArg_SliceIndexValue(PyValueBorrow v, Py_ssize_t *pi);
PyAPI_FUNC(int) _PyArg_SliceIndexNotNoneValue(PyValueBorrow v,
                                              Py_ssize_t *pi);

// The value form of _PyArg_UnpackKeywords() (D15): it fills index[i] with
// the position in args of the value of parameter i, or -1 if it is absent,
// for the nindex parameters, instead of copying words into a buffer.
// Returns 0 with an exception set on error.
PyAPI_FUNC(int) _PyArg_UnpackKeywordsValue(
    const PyValue *args,
    Py_ssize_t nargs,
    PyObject *kwnames,
    struct _PyArg_Parser *parser,
    int minpos,
    int maxpos,
    int minkw,
    int varpos,
    Py_ssize_t *index,
    int nindex);

// The indices of a call with positional arguments only, in range.
static inline int
_PyArg_IndexPositionalValue(Py_ssize_t nargs, int maxpos, Py_ssize_t *index,
                            int nindex)
{
    for (int i = 0; i < nindex; i++) {
        index[i] = (i < nargs && i < maxpos) ? i : -1;
    }
    return 1;
}

#define _PyArg_UnpackKeywordsValue(args, nargs, kwnames, parser, minpos, maxpos, minkw, varpos, index, nindex) \
    (((minkw) == 0 && (kwnames) == NULL && \
      (minpos) <= (nargs) && ((varpos) || (nargs) <= (maxpos))) ? \
     _PyArg_IndexPositionalValue((nargs), (maxpos), (index), (nindex)) : \
     _PyArg_UnpackKeywordsValue((args), (nargs), (kwnames), (parser), \
                                (minpos), (maxpos), (minkw), (varpos), \
                                (index), (nindex)))

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_MODSUPPORT_H

