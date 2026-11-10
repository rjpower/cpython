# Guard positional plans against the current defaults shape

Source triage confirms a semantic defect in the preexisting positional direct
binder; no native reproduction has run. MAKE_FUNCTION initializes a function's
version from its code version, then SET_FUNCTION_ATTRIBUTE installs defaults
without making that version unique. Two functions can therefore share code and
version while carrying different default counts. Replacing a factory's defaults
tuple constant while retaining the nested code object constructs this case.

The positional specializer caches `default_start = supplied - first_default`.
The hit checks supplied count but reads the current tuple using that old offset.
For a three-parameter target warmed with two defaults and one positional input,
replacing it by a same-code function with defaults `(30, 40, 50)` incorrectly
selects `(30, 40)` instead of `(40, 50)`. A shorter tuple can produce an
out-of-bounds read instead of the missing-argument error. Function-version
mutation invalidation does not address replacement by a distinct same-code
function whose version was never cleared.

## Minimal proposed repair

Before any allocation, callable detach or overlap in the positional branch of
`call_bind_known_python`, when positional defaults are needed:

1. Read the current defaults tuple and count, using zero for NULL.
2. Compute `first_default = co_argcount - current_count`.
3. Return the existing pre-effect miss if current count exceeds the supported
   scalar bound, supplied count is below `first_default`, or cached
   `default_start` differs from `supplied - first_default`.

After this guard the current tuple has exactly the shape required by the cached
offset, so the existing duplication loop is safe. With no missing positionals,
no default offset is used and no new check is needed. Equal-sized replacement
tuples still work through the hit and read current values. Changed-sized tuples
use the existing generic transaction; it already checks coverage and selects
current defaults. This matches the keyword branch's current-default coverage
rule, preserves the scalar plan and one ABI, and introduces no callback or owner
movement. It needs only the assigned positional region in `Python/ceval.c`.

## Focused regression

Add a test to `test_ir_frame_calls.py` using a factory-created target with three
parameters and two positional defaults. Warm a one-positional-argument caller
and assert CALL_PY_GENERAL, then change its target to same-code functions made
by replacing the factory's tuple constant. Check equal-size replacement values,
larger-count right alignment, shorter/empty defaults raising the ordinary
missing-argument error, and overlong legal defaults using fallback. Assert each
replacement retains the exact nested code object. Root owns real release/debug
compilation and native execution; no native pass is claimed here.

Await coordinator approval before runtime or test edits.
