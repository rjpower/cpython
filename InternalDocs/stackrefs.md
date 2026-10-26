# Stack references (`_PyStackRef`)

Stack references are the interpreter's representation of values: frame slots, evaluation stack
entries and argument arrays. A stackref holds an object pointer, an interpreter-internal index, or
an inline int or float.

## Shape and encoding

- A `_PyStackRef` is a 64-bit word. The header comment in `Include/internal/pycore_stackref.h`
  gives the full bit layout.
- Values below `Py_PTR_LIMIT` are pointer space: `PyStackRef_NULL`, `PyStackRef_ERROR`, and object
  pointers, including `None`, `True` and `False`.
- Above pointer space lie interpreter-internal indices and inline values.

## Ownership

Nothing is reference counted: the tracing collector owns memory. A stackref owns nothing, and
copying one is all that taking or giving up a value means. Dropping a value needs no operation;
`PyStackRef_CLEAR` sets a stackref to `PyStackRef_NULL` so that the collector no longer sees what
it held.

The collector finds stackrefs precisely in frames and in `_PyCStackRef` records linked on the
thread state, and conservatively in other C stack frames. The eval loop's own C frames are not
roots, so the cases generator rejects a handler that keeps a value only in a C local across an
escaping call; see "Roots" in `Tools/cases_generator/interpreter_definition.md`.

## Converting to and from PyObject*

- `PyStackRef_FromPyObject(obj)` makes a stackref for an object. `obj` must not be `NULL`.
- `PyStackRef_AsPyObjectBorrow(ref)` returns the object pointer. It allows `PyStackRef_NULL` and
  must not be given an inline value or an index.
- `PyStackRef_AsPyObjectBoxed(ref)` boxes an inline value first, and returns `NULL` with an
  exception set if boxing fails.

## Tagged indices on the stack

The interpreter stores two kinds of internal index in a stackref: the position of a
virtual iterator in the `FOR_ITER` family and the saved `lasti` during exception unwinding.
`PyStackRef_TagIndex`, `PyStackRef_UntagIndex`, and `PyStackRef_IncrementTaggedIndexNoOverflow`
create and update them, and `PyStackRef_IsTaggedIndex` tests for them. A tagged index is not a
Python object: it never reaches Python code, never converts to a `PyObject*`, and is skipped by the
GC.

## Inline values

A stackref can hold an exact int that fits in 32 bits or an exact float other than NaN.
`PyStackRef_IsInline`, `PyStackRef_IsInlineInt` and `PyStackRef_IsInlineFloat` test for them;
`PyStackRef_TYPE`, `PyStackRef_LongCheck` and `PyStackRef_FloatCheck` report `int` or `float`. The
GC skips them.

Such a value has value identity: `is`, `id()` and `operator.is_` give the same answer whether it is
held inline or boxed, and however many boxes exist. `PyStackRef_Is` and `_PyObject_ValueIdentity`
implement this; see "Value identity" in `Include/internal/pycore_stackref.h`.

An inline value must be boxed with `PyStackRef_Box` before it is converted to a `PyObject*`.
Boxing returns a reference to an int or float, or the inline value unchanged with an exception
set if allocation fails. The cases generator does this for instruction inputs, and guards exit
on them; see "Inline values" in `Tools/cases_generator/interpreter_definition.md`. C code that
converts stackref arrays boxes their elements (`_PyStackRef_BoxArray`, `_PyStackRef_BoxInto`), and
C code that needs an object for a stackref uses `PyStackRef_AsPyObjectBoxed`.

Inline ints are made by `LOAD_SMALL_INT_INLINE` and `LOAD_CONST_INLINE_INT`, to which
`_PyCode_Quicken` rewrites loads of compact int constants, by the `BINARY_OP_*_INT`
specializations for `+`, `-`, `*`, `//`, `%`, `&`, `|`, `^`, `<<` and `>>`, which box a result
that does not fit in 32 bits, by `FOR_ITER_RANGE`, by `CALL_LEN` and by `UNARY_NEGATIVE` of an
inline int. The int guards accept an inline int or a compact int object, and the instructions
after them read either with `PyStackRef_CompactIntValue`.

Inline floats are made by `LOAD_CONST_INLINE_FLOAT`, to which `_PyCode_Quicken` rewrites loads of
float constants other than NaN, by `BINARY_OP_ADD_FLOAT`, `BINARY_OP_SUBTRACT_FLOAT`,
`BINARY_OP_MULTIPLY_FLOAT` and `BINARY_OP_TRUEDIV_FLOAT`, and by `UNARY_NEGATIVE` of an exact
float. The `BINARY_OP_*_FLOAT` instructions accept two floats, inline or boxed, or a float other
than NaN and a compact int, which they convert to a double. `PyStackRef_FromDouble` boxes a NaN
result, so every NaN is an object with its own identity, as before. `COMPARE_OP_FLOAT`, `TO_BOOL`,
`BINARY_SLICE`, `STORE_SLICE` and `CALL_ISINSTANCE` read inline values without boxing them.
Instructions that store into the heap, such as `STORE_ATTR` and `LIST_APPEND`, box them.

`PYTHON_TAGGED_VALUES=0` turns the producers off: the constants stay boxed and the specializer
does not choose the producing `BINARY_OP` and `FOR_ITER` specializations, and `CALL_LEN` and
`UNARY_NEGATIVE` box their results.

## Debugging support

Debug builds check stackrefs with `PyStackRef_IsValid` and `PyStackRef_IsMalformed`, and can wrap
non-object pointers with `PyStackRef_Wrap` so that they are told apart from objects. Helper
routines reside in `Python/stackrefs.c`.
