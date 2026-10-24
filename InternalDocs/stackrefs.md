# Stack references (`_PyStackRef`)

Stack references are the interpreter's tagged representation of values on the evaluation stack.
They carry metadata to track ownership and can hold interpreter-internal indices, and on most
64-bit builds small ints and floats, without a heap object.

## Shape and tagging

- A `_PyStackRef` is a tagged pointer-sized value. The header comment in
  `Include/internal/pycore_stackref.h` gives the full bit layout.
- Values below `Py_PTR_LIMIT` are pointer space, where bit 0 distinguishes two cases:
  - `Py_TAG_REFCNT` unset - reference count lives on the pointed-to object.
  - `Py_TAG_REFCNT` set - ownership is "borrowed" (no refcount to drop on close) or the object is immortal.
- Above pointer space lie interpreter-internal indices and inline values, which own nothing.
  Free-threaded and 32-bit builds have no inline values and mark indices with `Py_INDEX_TAG`.
- Special constants: `PyStackRef_NULL`, `PyStackRef_ERROR`, and embedded `None`/`True`/`False`.

In GIL builds, most objects carry their refcount; tagged borrowed refs skip decref on close. In free
threading builds, the tag is also used to mark deferred refcounted objects so the GC can see them and
to avoid refcount contention on commonly shared objects.

## Converting to and from PyObject*

Three conversions control ownership:

- `PyStackRef_FromPyObjectNew(obj)` - create a new reference (INCREF if mortal).
- `PyStackRef_FromPyObjectSteal(obj)` - take over ownership without changing the count unless the
  object is immortal.
- `PyStackRef_FromPyObjectBorrow(obj)` - create a borrowed stackref (never decref on close).

The `obj` argument must not be `NULL`.

Going back to `PyObject*` mirrors this:

- `PyStackRef_AsPyObjectBorrow(ref)` - borrow the underlying pointer
- `PyStackRef_AsPyObjectSteal(ref)` - transfer ownership from the stackref; if ref is borrowed or
   deferred, this creates a new owning `PyObject*` reference.
- `PyStackRef_AsPyObjectNew(ref)` - create a new owning reference

Only `PyStackRef_AsPyObjectBorrow` allows ref to be `PyStackRef_NULL`.

## Operations on stackrefs

The interpreter treats `_PyStackRef` as the unit of stack storage. Ownership must be managed with
the stackref primitives:

- `PyStackRef_DUP` - like `Py_NewRef` for stackrefs; preserves the original.
- `PyStackRef_Borrow` - create a borrowed stackref from another stackref.
- `PyStackRef_CLOSE` / `PyStackRef_XCLOSE` - like `Py_DECREF`; invalidates the stackref.
- `PyStackRef_CLEAR` - like `Py_CLEAR`; closes and sets the stackref to `PyStackRef_NULL`
- `PyStackRef_MakeHeapSafe` - converts borrowed reference to owning reference

Borrow tracking (for debug builds with `Py_STACKREF_DEBUG`) records who you borrowed from and reports
double-close, leaked borrows, or use-after-close via fatal errors.

## Borrow-friendly opcodes

The interpreter can push borrowed references directly. For example, `LOAD_FAST_BORROW` loads a local
variable as a borrowed `_PyStackRef`, avoiding both INCREF and DECREF for the temporary lifetime on
the evaluation stack.

## Tagged indices on the stack

The interpreter stores two kinds of internal index in a stackref: the position of a
virtual iterator in the `FOR_ITER` family and the saved `lasti` during exception unwinding.
`PyStackRef_TagIndex`, `PyStackRef_UntagIndex`, and `PyStackRef_IncrementTaggedIndexNoOverflow`
create and update them, and `PyStackRef_IsTaggedIndex` tests for them. A tagged index is not a
Python object: it never reaches Python code, never converts to a `PyObject*`, and is skipped by the
GC.

## Inline values

On 64-bit builds without free threading (`Py_STACKREF_INLINE_VALUES`), a stackref can hold an
exact int that fits in 32 bits or an exact float other than NaN. `PyStackRef_IsInline`,
`PyStackRef_IsInlineInt` and `PyStackRef_IsInlineFloat` test for them; `PyStackRef_TYPE`,
`PyStackRef_LongCheck` and `PyStackRef_FloatCheck` report `int` or `float`. CLOSE, DUP and Borrow
leave them unchanged and the GC skips them.

Such a value has value identity: `is`, `id()` and `operator.is_` give the same answer whether it is
held inline or boxed, and however many boxes exist. `PyStackRef_Is` and `_PyObject_ValueIdentity`
implement this; see "Value identity" in `Include/internal/pycore_stackref.h`.

An inline value must be boxed with `PyStackRef_Box` before it is converted to a `PyObject*`.
Boxing returns a new reference to an int or float, or the inline value unchanged with an exception
set if allocation fails. The cases generator does this for instruction inputs, and guards exit
on them; see "Inline values" in `Tools/cases_generator/interpreter_definition.md`. C code that
converts stackref arrays boxes their elements (`_PyStackRef_BoxArray`, `_PyStackRef_BoxInto`), and
C code that needs an object for a stackref it does not own uses `PyStackRef_AsPyObjectNewBoxed`.

Inline ints are made by `LOAD_SMALL_INT_INLINE` and `LOAD_CONST_INLINE_INT`, to which
`_PyCode_Quicken` rewrites loads of compact int constants, by `BINARY_OP_ADD_INT`,
`BINARY_OP_SUBTRACT_INT` and `BINARY_OP_MULTIPLY_INT`, which box a result that overflows int32, by
`FOR_ITER_RANGE` and by `UNARY_NEGATIVE` of an inline int. The int guards accept an inline int or a
compact int object, and the instructions after them read either with
`PyStackRef_CompactIntValue`. `PYTHON_TAGGED_VALUES=0` turns the producers off.

## Free threading considerations

Objects that support deferred reference counting can be pushed to the evaluation
stack and stored in local variables without directly incrementing the reference
count because they are only freed during cyclic garbage collection. This avoids
reference count contention on commonly shared objects such as methods and types. The GC
scans each thread's locals and evaluation stack to keep objects that use
deferred reference counting alive.

## Debugging support

`Py_STACKREF_DEBUG` builds replace the inline tags with table-backed IDs so the runtime can track
creation sites, borrows, closes, and leaks. Enabling `Py_STACKREF_CLOSE_DEBUG` additionally records
double closes. The tables live on `PyInterpreterState` and are initialized in `pystate.c`; helper
routines reside in `Python/stackrefs.c`.
