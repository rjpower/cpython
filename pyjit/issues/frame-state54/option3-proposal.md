# Option 3: derive frame environments from its owning function

Proposed for independent ownership review; no source edits or execution yet.
Branch `pyjit/frame54-globals`, source `BASE/worktrees/frame54-globals`, golden51
start `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. Root owns generation,
builds, runtime tests, native evidence, timings and commits. This follows lane N
in [Astra's final review](astra-review.md), read before implementation.

Remove borrowed `_PyInterpreterFrame.f_globals` and `f_builtins`, replacing
normal reads with explicit accessors through the frame's owning `f_funcobj`.
Keep independent `f_executable` unchanged: mutable function `__code__` cannot
replace the code executing in a frame. Predict112→96-byte fixed header, two
fewer normal initialization/copy stores, and no new universal guard. This does
not remove the function owner or refcount work and does not claim a measured win.

## Authority and teardown proof required before code

`Objects/funcobject.c` assigns globals/builtins only in constructors
(138–140,230–231) and `func_clear` (1213–1230), among the searched C sources.
Python members are readonly (678–679); dictionary contents remain mutable,
identity remains the retained function's environment, and changing __code__
does not change that environment. A running/suspended/reachable escaped frame
owns the function; tracing through that owner makes the function reachable.
Therefore GC must not clear its environment during ordinary live execution.
This proves the normal hot accessor case without a runtime function/type guard.

It is not a complete teardown proof by itself. `func_clear` NULLs environment
members before callback-capable decrefs. GC finalizers run before tp_clear,
then resurrection is checked and weakrefs to garbage are cleared before
breaking cycles. Existing `test_gc.GCTests.test_function_tp_clear_leaves_consistent_state`
attempts weakref resurrection during tp_clear and asserts the weakref yields
None; it does not demonstrate successful resurrection. Independent review must
check custom collector order and strong-reference callback paths as well as
ordinary CPython weakref rules. Any externally accessible frame must keep the
function reachable; if no such frame can be accessed during garbage clear,
NULL environments are unobservable except deallocation internals, which must
not dereference them. If this argument fails, stop this lane and report the
counterexample; do not silently add owner duplicates or a universal fallback.

`_PyFrame_ClearExceptCode` uses function clear after frame-object takeover or
locals retirement; escaped takeover moves its function owner intact. Explicit
`frame.clear()` preserves specials (`test_frame.ClearTest.test_clear_does_not_clear_specials`)
and `frame_tp_clear` currently clears homes/mapping, not f_funcobj. Retain that
behavior. Frame deallocation clears executable then function before its
remaining homes; audit that callbacks cannot observe a logically dead frame
via another retained pointer. Generator construction moves function ownership
into its final heap frame. All of these remain the owner contract.

Interpreter entry/base sentinels and static cleanup trampoline use None/NULL
functions. Their environment fields are already absent/unused in release or
NULL in the base sentinel. Fast normal accessors require a genuine function;
existing cold safe-dump/debugger/API boundaries must retain the current
interpreter-owner checks or return NULL where those APIs already tolerate it.
Do not sprinkle function tests into normal GLOBALS/BUILTINS handlers. SafeGetCode
currently uses globals/builtins as freed-memory heuristics, so revise those
heuristics around retained executable/function validity rather than blindly
dereferencing an unvalidated function value. GDB must also tolerate None/NULL
sentinels, preserving safe failed inspection.

## Complete affected interfaces

Keep Python frame attributes, eval APIs and globals/builtins identities unchanged.
Port all handwritten C field usages, not public Python attribute callers:

- `Include/internal/pycore_interpframe_structs.h`: remove two pointers.
- `Include/internal/pycore_interpframe.h`: initialization, copy, trampoline
  debug initialization, SafeGetCode heuristic and new explicit accessors.
- `Python/ceval_macros.h`: GLOBALS/BUILTINS; generated users follow macros.
- `Python/ceval.c`: evaluator entry debug fields, PyEval_GetBuiltins,
  GetGlobals/FrameGlobals APIs, globals==locals and name/import lookup helpers.
- `Python/pystate.c`: embedded base initialization.
- `Python/gc_trace.c`: `_PyGC_TraceVisitFrames` must preserve root reachability
  through retained function; whether to retain explicit environment visits
  via accessors or rely on the function edge is decided explicitly in review,
  not accidentally changed. Initial proposal retains explicit accessor visits.
- `Python/import.c`: globals/locals identity and import namespace.
- `Python/_warnings.c`: frame namespace used for warning attribution.
- `Objects/frameobject.c`: frame globals/builtins property getters (nullable
  return behavior), plus any constructor/environment use found by exact search.
- `Tools/gdb/libpython.py`: `_f_globals`/`_f_builtins` derive from f_funcobj
  with proper PyValue decode and None/NULL sentinel handling.

Generated clinic property wrappers merely name public properties and need no
manual rewrites. Public Python attribute callers remain unchanged. Debug
offsets don't export these pointers directly, but every following offset,
co_framesize, deepfreeze FRAME_SPECIALS_SIZE, variable heap object geometry,
arena reservation and entry-frame static assertions must rebuild together.
The independent code owner and normal argument-overlap/NULL-home invariants
remain intact. There is no new code identity or environment eligibility guard.

## Added cost and gates

Every GLOBALS/BUILTINS read now follows frame->function PyValue, decodes the
pointer and loads its environment member. That adds a dependency and may add
instructions versus one direct frame load, including hot LOAD_GLOBAL and
module/eval name paths. Two environment loads may share function decode within
one handler, but do not assume cross-handler caching. Native gates inspect
LOAD_GLOBAL, imports, exact CALL setup and generator/frame copy, prove header96
and removed stores, and report added loads/register spills. Root screens all six
unchanged hot workloads and confirms six pairs only for promising breadth.
Global-heavy work can reject this even if recursive creation improves.
If direct evidence isolates only builtins as worth removing, a later separately
reviewed one-field revision is permitted by Astra; it cannot be reported as the
two-word result and is outside this first implementation.

Minimal release/debug semantic set uses discovered real tests:
`test.test_funcattrs.FunctionPropertiesTest.test___globals__`,
`test.test_funcattrs.FunctionPropertiesTest.test___builtins__`,
`test.test_funcattrs.FunctionPropertiesTest.test_copying___code__`,
`test.test_frame.ClearTest` (especially specials/clear cycles),
`test.test_frame.FrameAttrsTest`, `test.test_capi.test_frame.FrameTest`,
`test.test_gc.GCTests.test_function_tp_clear_leaves_consistent_state`,
`test.test_generators.GeneratorDeallocTest`, and
`test.test_frame.TestIncompleteFrameAreInvisible`.
Add a focused behavior test that retains an escaped frame with a distinct
explicit builtins dictionary, mutates the original function's __code__, drops
outside function/environment references, forces GC, then checks exact frame
f_globals/f_builtins identity before and after frame.clear. Add a finalizer
that inspects the same retained frame during local retirement. These exercise
owner lifetime rather than assert implementation details. Root also exercises
import/warnings/eval APIs through the existing focused project gate or selected
existing module tests. Broaden only for a concrete uncovered path or failure.
