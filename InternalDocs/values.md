# Values (`PyValue`)

`PyValue` is the value type of the core: frame slots, evaluation stack entries and argument
arrays. A value holds an object pointer, an interpreter-internal index, or an inline int or
float.

## Shape and encoding

- A `PyValue` is a 64-bit word. The header comment in `Include/pyvalue.h` gives the full bit
  layout; `Include/internal/pycore_value.h` has the core's internal values and helpers.
- Values below `_PyValue_PTR_LIMIT` are pointer space: `PyValue_NULL`, `_PyValue_ERROR`, and object
  pointers, including `None`, `True` and `False`.
- Above pointer space lie interpreter-internal indices and inline values.

## Ownership

Nothing is reference counted: the tracing collector owns memory. A value owns nothing, and
copying one is all that taking or giving up a value means. Dropping a value needs no operation;
assigning `PyValue_NULL` to a slot means the collector no longer sees what it held.

The collector finds values precisely in frames and in `_PyValueRoot` records linked on the
thread state, and conservatively in other C stack frames. The eval loop's own C frames are not
roots, so the cases generator rejects a handler that keeps a value only in a C local across an
escaping call; see "Roots" in `Tools/cases_generator/interpreter_definition.md`.

## Converting to and from PyObject*

- `PyValue_FromObject(obj)` makes a value for an object. `obj` must not be `NULL`.
- `PyValue_ObjectBorrow(ref)` returns the object pointer. It allows `PyValue_NULL` and
  must not be given an inline value or an index.
- `PyValue_Object(ref)` boxes an inline value first, and returns `NULL` with an
  exception set if boxing fails.

## Tagged indices on the stack

The interpreter stores two kinds of internal index in a value: the position of a
virtual iterator in the `FOR_ITER` family and the saved `lasti` during exception unwinding.
`_PyValue_TagIndex`, `_PyValue_UntagIndex`, and `_PyValue_IncrementTaggedIndexNoOverflow`
create and update them, and `_PyValue_IsTaggedIndex` tests for them. A tagged index is not a
Python object: it never reaches Python code, never converts to a `PyObject*`, and is skipped by the
GC.

## Inline values

A value can hold an exact int that fits in 32 bits or an exact float other than NaN.
`PyValue_IsInline`, `PyValue_IsInlineInt` and `PyValue_IsInlineFloat` test for them;
`PyValue_Type` reports `int` or `float`. The
GC skips them.

Such a value has value identity: `is`, `id()` and `operator.is_` give the same answer whether it is
held inline or boxed, and however many boxes exist. `PyValue_Is` and `_PyObject_ValueIdentity`
implement this; see "Identity" in `Include/pyvalue.h`.

An inline value must be boxed with `PyValue_Box` before it is converted to a `PyObject*`.
Boxing returns a reference to an int or float, or the inline value unchanged with an exception
set if allocation fails. The cases generator does this for instruction inputs, and guards exit
on them; see "Inline values" in `Tools/cases_generator/interpreter_definition.md`. C code that
converts value arrays boxes their elements (`_PyValue_BoxArray`, `_PyValue_BoxInto`), and
C code that needs an object for a value uses `PyValue_Object`.

Inline ints are made by `LOAD_SMALL_INT_INLINE` and `LOAD_CONST_INLINE_INT`, to which
`_PyCode_Quicken` rewrites loads of compact int constants, by the `BINARY_OP_*_INT`
specializations for `+`, `-`, `*`, `//`, `%`, `&`, `|`, `^`, `<<` and `>>`, which box a result
that does not fit in 32 bits, by `FOR_ITER_RANGE`, by `CALL_LEN` and by `UNARY_NEGATIVE` of an
inline int. The int guards accept an inline int or a compact int object, and the instructions
after them read either with `_PyValue_CompactInt`.

Inline floats are made by `LOAD_CONST_INLINE_FLOAT`, to which `_PyCode_Quicken` rewrites loads of
float constants other than NaN, by `BINARY_OP_ADD_FLOAT`, `BINARY_OP_SUBTRACT_FLOAT`,
`BINARY_OP_MULTIPLY_FLOAT` and `BINARY_OP_TRUEDIV_FLOAT`, and by `UNARY_NEGATIVE` of an exact
float. The `BINARY_OP_*_FLOAT` instructions accept two floats, inline or boxed, or a float other
than NaN and a compact int, which they convert to a double. `PyValue_FromDouble` boxes a NaN
result, so every NaN is an object with its own identity, as before. `COMPARE_OP_FLOAT`, `TO_BOOL`,
`BINARY_SLICE`, `STORE_SLICE` and `CALL_ISINSTANCE` read inline values without boxing them.
Instructions that store into the heap, such as `STORE_ATTR` and `LIST_APPEND`, box them.

## Debugging support

Debug builds check values with `_PyValue_IsValid` and `_PyValue_IsMalformed`, and can wrap
non-object pointers with `_PyValue_Wrap` so that they are told apart from objects. Helper
routines reside in `Python/pyvalue.c`.
