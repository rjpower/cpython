#ifndef Py_CPYTHON_MODSUPPORT_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(int) PyArg_ParseArray(
    PyObject *const *args,
    Py_ssize_t nargs,
    const char *format,
    ...);
PyAPI_FUNC(int) PyArg_ParseArrayAndKeywords(
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwnames,
    const char *format,
    const char * const *kwlist,
    ...);

// A data structure that can be used to run initialization code once in a
// thread-safe manner. The C++11 equivalent is std::call_once.
typedef struct {
    uint8_t v;
} _PyOnceFlag;

typedef struct _PyArg_Parser {
    const char *format;
    const char * const *keywords;
    const char *fname;
    const char *custom_msg;
    _PyOnceFlag once;       /* atomic one-time initialization flag */
    int is_kwtuple_owned;   /* does this parser own the kwtuple object? */
    int pos;                /* number of positional-only arguments */
    int min;                /* minimal number of arguments */
    int max;                /* maximal number of positional arguments */
    PyObject *kwtuple;      /* tuple of keyword parameter names */
    struct _PyArg_Parser *next;
} _PyArg_Parser;

PyAPI_FUNC(int) _PyArg_ParseTupleAndKeywordsFast(PyObject *, PyObject *,
                                                 struct _PyArg_Parser *, ...);

/* A borrow scope bounds the borrowed object outputs of the PyArg_* parsers
   (O, O!, S, U, Y, objects passed to O&, PyArg_UnpackTuple).  The runtime
   opens one around every native function body; open one explicitly around
   a loop body that parses, or around parsing outside any callback.  Take a
   strong reference to any output kept past the scope.  Like
   Py_BEGIN_ALLOW_THREADS, break, continue and return must not leave the
   block. */
typedef struct { void *_mark; } PyBorrowScope;
PyAPI_FUNC(void) PyBorrowScope_Enter(PyBorrowScope *scope);
PyAPI_FUNC(void) PyBorrowScope_Exit(PyBorrowScope *scope);

#define Py_BEGIN_BORROW_SCOPE \
    { PyBorrowScope _py_borrow_scope; PyBorrowScope_Enter(&_py_borrow_scope);
#define Py_END_BORROW_SCOPE \
    PyBorrowScope_Exit(&_py_borrow_scope); }

#ifdef Py_BUILD_CORE
// For internal use in stdlib. Needs C99 compound literals.
// Defined here to avoid every stdlib module including pycore_modsupport.h
#define _Py_ABI_SLOT {Py_mod_abi, (void*) &(PyABIInfo) _PyABIInfo_DEFAULT}
#endif
