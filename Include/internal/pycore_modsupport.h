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

/* A callee-local view of an immutable canonical tuple. Begin only pins/counts;
 * the shared unpack preflight precedes storage allocation and object boxing.
 * Args, kwargs and parser keyword metadata stay pinned through implementation
 * and converter cleanup; object outputs borrow the scope's independent owners.
 * The caller must already have published coherent IN_CALL state. */
typedef struct {
    PyObject *tuple;
    PyObject *kwargs;
    PyObject *kwtuple;
    PyValue *values;
    PyObject **objects;
    void *storage;
    Py_ssize_t nargs;
    Py_ssize_t keyword_budget;
    Py_ssize_t capacity;
    Py_ssize_t nvalues;
    Py_ssize_t nobjects;
    int materialized;
    int ended;
    PyValue inline_values[4];
    PyObject *inline_objects[4];
} _PyArg_OwnerScope;

PyAPI_FUNC(int) _PyArg_OwnerScope_BeginTuple(
    _PyArg_OwnerScope *, PyObject *, Py_ssize_t keyword_owner_budget);
/* Valid only after successful scoped unpack; borrows stable scope storage. */
PyAPI_FUNC(PyObject *const *) _PyArg_OwnerScope_Objects(const _PyArg_OwnerScope *);
PyAPI_FUNC(PyObject *const *) _PyArg_UnpackKeywordsOwned(
    _PyArg_OwnerScope *, PyObject *kwargs, struct _PyArg_Parser *,
    int minpos, int maxpos, int minkw, int varpos, PyObject **buf);
/* Fresh successful BeginTuple(args, 0), valid 0 <= min <= max. Check the
 * positional count before materializing; NULL reports an error. The returned
 * views borrow independent scope owners through implementation and cleanup. */
PyAPI_FUNC(PyObject *const *) _PyArg_UnpackPositionalOwned(
    _PyArg_OwnerScope *, const char *name, Py_ssize_t min, Py_ssize_t max);
/* Safe after a partial Begin/unpack. Retains the primary or first cleanup error.
 * Run after converter cleanup; 0 has no pending error, -1 reports an error. */
PyAPI_FUNC(int) _PyArg_OwnerScope_End(_PyArg_OwnerScope *);

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

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_MODSUPPORT_H
