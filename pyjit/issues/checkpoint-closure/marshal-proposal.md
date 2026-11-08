# Marshal value decoder proposal

Scope: `Python/marshal.c` reader, with the public marshal object API unchanged.
The current `TYPE_REF` read calls `_PyValue_AsObjectAt` on an owned list value;
the startup sample attributes 81 core boxes to this path. Changing that one
read alone is unsafe: every recursive caller of `r_object` expects an owned
`PyObject *` and will box an inline result or mishandle its ownership. Make the
private decoder value-native end to end.

## Contract and boundaries

Introduce `static PyValue r_value(RFILE *)`. It returns one owned language
value on success. `PyValue_NULL` means either the marshal `TYPE_NULL` sentinel
(no exception) or failure (exception set); callers retain the existing
context-specific `TYPE_NULL` errors. The private `read_value` performs the
existing audit first and invokes `r_value` once. `marshal.load[s]` already
return `PyValue`, so they consume `read_value` directly. The public
`PyMarshal_Read*` C APIs alone use a thin `read_object` wrapper that projects
the successful word to a new `PyObject *` and closes the word. That
unavoidable C-API boundary box must be attributed as a boundary rather than
hidden as core work. Preserve the existing EOF, recursion-depth and
`allow_code` checks.

Primitive tags construct canonical values: int32/int64 via
`PyValue_FromInt64` where representable, float via `PyValue_FromDouble`, and
large long/NaN/string/bytes/complex via owned-object-to-value transfer.
`None`, booleans, ellipsis and `StopIteration` retain their present identity
semantics. Do not alter the byte format, flags, intern policy or `.pyc` magic.

Reference helpers take and return owned values. `r_ref_reserve` keeps a
`PyValue_None` placeholder. `r_ref_value` appends a *borrow* of a successful
value and returns the original owner; `r_ref_insert_value` writes a duplicate
into a reserved index and returns the original owner, closing it on insertion
failure as the current helper does. `TYPE_REF` takes the owner returned by
`PyList_GetValue` and returns it directly. An invalid index or placeholder
still raises `ValueError("bad marshal data (invalid reference)")`. An inline
ref must never be projected merely to be placed in the ref list.

## Recursive consumers

* Tuple/list slots take each successful `r_value` owner with
  `PyTuple_SET_VALUE`/`PyList_SET_VALUE`. On a null sentinel or error, close
  the partially built container and retain its present error text.
* Dict/frozendict read owned key and value words, call `PyDict_SetValue`
  with borrows, then close both on every path. The null key terminates a dict;
  an error remains an error. Preserve the early reference registration for a
  mutable dict and delayed registration after freezing a frozendict.
* Set/frozenset call `PySet_AddValue` with a borrow and close each item.
  Preserve mutable-set early registration, frozenset delayed registration,
  and empty-frozenset singleton behavior.
* Slice reads three owned words and calls `_PySlice_FromValues` with borrows,
  then closes all three. Reserve its ref index before children and insert the
  finished slice afterward, including the existing failure cleanup.
* Code reads the same field sequence into owned values. Its
  `_PyCodeConstructor` still takes objects: project the typed object fields
  while their owners are held, validate, construct, and close every temporary
  on both paths. Its `consts` tuple may hold inline values, so do not box its
  elements. Preserve the shared `_PyCode_Validate` check that rejects
  execution-only handler IDs in serialized code.

No recursive branch calls the public object wrapper. Value-bearing container
slots and reference entries remain values; object-only constructors receive
objects at their actual boundary. Retain current callback behavior and
container snapshots through dict/set insertion, including mutation and
reference cycles. Keep `p->depth` decremented exactly once for every entered
decode, including all error exits.

## Review and validation

Before implementation, trace every `r_object` call and classify it as recursive
or public; leave no recursive object-form caller. Peer review the reference
ownership table and each error exit, especially `TYPE_NULL` versus exception,
failed `r_ref_insert_value`, a self-referential mutable container, delayed
frozen refs, and code-field cleanup. Build `marshal.c` and run focused
`test_marshal`, `test_py_compile`, `test_compileall`, and import/pyc tests.
Add a focused regression with repeated inline integer and float references,
malformed/placeholder refs, recursive mutable containers, and truncated code
and slice payloads. At integration, use the R27 startup dump to confirm the
81 `TYPE_REF` core boxes disappear and no new core owner replaces them.
