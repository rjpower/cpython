# Startup exception and name lookup value ports

## OSError

`OSError_new` and `OSError_init` call `oserror_parse_args`, which uses
`PyArg_UnpackTuple`; its borrowed-object adapter boxes inline tuple elements.
The resulting fields are currently object pointers. Port the fields and all
in-tree consumers together, rather than returning temporary object projections.

Move the `PyOSErrorObject` definition and its compatibility aliases from the
public `Include/cpython/pyerrors.h` into `Include/internal/pycore_pyerrors.h`;
include that internal header from `Modules/_ssl.c` and `_io/bufferedio.c`.
`myerrno`, `strerror`, `filename`, `filename2`, and Windows `winerror` become
owned `PyValue` homes, initially `PyValue_NULL`; `written` remains a scalar.
The five members use `_Py_T_VALUE`, retaining the current missing-as-None and
assignment/deletion semantics. Clear closes each owned word; traversal visits
only object-bearing words using the existing value-field traversal idiom.

`oserror_parse_args` reads each optional tuple item via `PyTuple_GetValue`,
returning owners in a small local record initialized to null. The caller closes
all owners on every success or error. Preserve the 2..5 arity logic, `args`
replacement, type-specific keyword/init behavior, and the Windows error map:
construct a value errno and a fresh tuple with value slots, without changing
the original args if construction fails. `oserror_init` duplicates each value
into its home, preserving the existing filename-None tests and BlockingIOError
numeric `characters_written` conversion. `OSError_new` looks up the value errno
in the subclass map directly; returned class remains rooted until used.

`OSError_str` invokes `_PyValue_Str`/`_PyValue_Repr` in the same field order and
formats the returned Unicode objects, retaining missing-field fallbacks and
propagating the first error; `OSError_reduce` copies tuple values into owned
slots and duplicates value homes. `_ssl` checks the held `strerror` value for
an actual Unicode object before returning a new ref. `_PyIO_trap_eintr` uses
value exact-int predicates and `_PyValue_AsLongAndOverflow` on the held
`myerrno` home; preserve its error clearing and raised-exception ownership.
The `written` pointer accessor is unchanged. Verify exact type/subclass
behavior, deleted fields, repeated init, GC cycles, pickle, `str`, Windows
conditional compilation and SSL/error tests. No public struct contains a value
field; external source using these private layouts must be ported.

## SystemExit

Move the exceptions.c-only `PySystemExitObject` layout internal, with `code`
an owned `PyValue` home and `_Py_T_VALUE` member. For one argument,
`SystemExit_init` duplicates the tuple value; for multiple arguments it stores
the owning args tuple; zero-argument and repeated initialization behavior stays
unchanged. Clear and traversal follow value-home rules.

Introduce `_PyErr_SetObjectValue(PyThreadState *, PyObject *exception,
PyValueBorrow value)`, with `PyValue_NULL` retaining the existing no-value
meaning. Refactor the existing `_PyErr_SetObject` to call the shared
value-native normalization path, preserving tuple-as-argument-list rules,
exception-instance reuse, implicit context chaining, traceback, and
normalization failure notes. Construct exceptions using `_PyValue_Call` or
vectorcall from held values; do not box merely for the current object helper.
`sys_exit_impl` uses this entry directly. `pythonrun.c` keeps the owned result
of `_PyValue_GetAttr`, tests int/None using value readers and closes it; for a
non-integer code it retains the original exception-printing behavior. Preserve
long-long overflow mapping to exit -1, subclass overrides and exception text.
Tests cover zero, None, inline ints, huge ints, multiple args, overridden code,
raising inside an active exception, descriptor failure and subprocess exit.

## LOAD_NAME

`_PyEval_LoadName` has one runtime caller: generated `LOAD_NAME` from
`Python/bytecodes.c`, plus its declaration in `pycore_ceval.h`. Replace it
with `PyValue _PyEval_LoadName(...)` returning an owner or `PyValue_NULL` with
exception. Use `_PyValue_GetOptionalItem` for locals and builtins, and the
existing `PyDict_GetValue` globals path. A status of zero falls through; status
negative returns null; a found value keeps its owner. Only object values can be
lazy-import placeholders: keep their owner while `_PyLazyImport_Reify` runs,
then replace and close it exactly as the old object path does. The instruction
stores the owner directly. Regenerate `Python/generated_ir_cases.c.h`. Keep
NameError and absent-locals behavior, mapping callbacks and lazy import
failure order. Focus on compiler/import, name lookup with custom mapping and
lazy imports. This removes a measured nine THUNK boxes without relabeling.
