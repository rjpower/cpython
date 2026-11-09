# GDB value reader and wrapper-input tests

Proposal before source edits. Scope is N Tools/gdb/libpython.py and
Lib/test/test_gdb/test_pretty_print.py; util.py only if an actual shared harness
need appears. No runtime, generated provider, build or inferior execution by A.

The verbose38 receipt confirms fifteen failures at the obsolete
`builtin_id(self=..., v=...)` extraction and the frame test printing an optimized
out local v. Actual wrapper arguments are self,args,nargs,kwnames. Existing
corruption cases also attempt invalid struct casts/address-taking and then hit
old generic backtrace skips. They must become valid corruption tests, not be
counted as qualified skips. Numeric/cpu resource cases were not enabled in this
receipt; the new bounded smoke test must exercise inline values without that
resource gate.

## Reader binding

Add one exact value-type predicate shared by printer registration,
PyObjectPtr.__init__ and from_pyobject_ptr. Normalize qualifiers while retaining
the typedef name; accept only PyValue or PyValueBorrow whose underlying GDB
type is STRUCT with the expected bits field and pointer-sized storage. These
are the exact two anonymous typedefs in Include/pytypedefs.h21/26. Do not match
arbitrary structs by a coincidental bits field, and do not keep the obsolete
UNION/tag-only path as a second ABI.

Both forms then share the existing int51/double bit decoder and tagged-object
conversion. Borrow is a read-only view of the owner's exact bits, so reading
it performs no DUP, decref, inferior boxing or function call. The registered
printer must actually be selected for both forms; tests must not bypass lookup
by directly calling a logical-value printer from GDB Python.

## Harness binding

Keep builtin_id as the real breakpoint. Its actual Clinic signature is
`builtin_id(PyValueBorrow self, const PyValue *args, Py_ssize_t nargs,
PyObject *kwnames)`, and its first input is args[0]. Capture that input in a GDB
convenience variable `$value = args[0]` before any test commands. Borrow
coverage uses `*(PyValueBorrow *)args`, a view of the same live memory. Neither
expression calls or writes the inferior.

Print the selected value through ordinary GDB `print` between unique begin/end
markers, then extract exactly one `$N = ...` result from that bounded block.
Keep backtrace1 as a separate actual-breakpoint check rather than requiring a
now-nonexistent v parameter. Do not accept arbitrary output on regex failure.
Existing exact logical repr assertions, recursive/truncated output and address
patterns stay intact. The frame case prints the same actual input value and
retains its frame filename/line/local assertions instead of casting local v.

Add one small non-resource-gated test matrix for owned and borrowed inputs:
positive/negative ints, a large boxed int, finite float, signed zero, and a heap
container. The values come from ordinary source expressions under id. Type
lookup and actual logical repr must agree on both views; this exercises the
new registration paths, not merely the bit decoder in isolation.

## Fault injection

Use an explicit heap instance for ob_type/tp_flags/tp_name corruption, derived
as `$object = (PyObject *)($value.bits & ~(uintptr_t)3)`. Print that pointer view
after corruption; it describes the same heap object. Never interpret an inline
numeric word as a heap address. Keep safe fallback assertions and the optional
successful-render case where the existing test permits it.

The NULL pointer case prints an explicit `(PyObject *)0` convenience value;
a separate value-NULL test sets only the copied `$value.bits = 0`. This does
not modify an inferior owner. Corrupting `$object->ob_type` does modify the
throwaway inferior as the original tests intended. The inferior is not resumed
after corrupting its type. Preserve existing failure messages and do not add
skip/refusal branches for these migrated cases.

## Gates

H/B direction peer precedes edits. Host syntax/diff checks and a small fake-GDB
type/bit test may validate helper dispatch without executing a candidate. Root
first runs the new value matrix, one container, NULL/fault injection and frame
case, then the full pretty-print module with numeric resource coverage as
appropriate. Preserve the original16-failure receipt. No backend uninstall,
blanket skip, inferior boxing or weakened representation regex is admissible.
