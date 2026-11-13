# Option 3 independent post-review

Source review of `frame54-globals` at `a4210e5b9e8` against selected51 is
complete; see [source findings](option3-post-source.md). No source-contract
blocker found. This reviewer has not built, run, tested, or timed the candidate.
Read-only review of the matching release ELF is now complete. Only nm,
objdump and GDB type inspection were used; no inferior was started.

The normal accessor derives from the retained function; mutable function code
cannot replace independent executing-code authority. No environment read was
added to function/frame tp_clear or deallocation. Reachable frames pin their
function/environments; cycle breaking may clear unreachable function members,
but those teardown paths must not consult a derived namespace. Explicit custom
collector roots use a cold FunctionCheck, covering None-function init cleanup.
The complete-frame walk skips that shim because both words precede its
firsttraceable2. Base/interpreter sentinels remain incomplete. SafeGetCode
validates executable bits without a blind function decode; GDB checks function
kind before environment access. Both escaped-frame tests cover meaningful
namespace lifetime/identity, code mutation, GC, clear and finalizer behavior.

## Native findings

GDB type information for `BASE/frame54-globals-release/python` confirms the
fixed header is96 bytes (owned_slots offset96, sizeof104 including its one
declared home). f_locals is40, frame_obj48, instr_ptr56, ir_words64,
ir_feedback72, return_pc80 and return_slot88. These are GIL release offsets.

| Handler | Golden51 text bytes | Globals candidate text bytes | Inspected stack reservation |
| --- | ---: | ---: | --- |
| BINARY_OP_ADD_INT |642 |642 |48 bytes, unchanged |
| LOAD_ATTR_SLOT |1240 |1240 |64 bytes, unchanged |
| LOAD_GLOBAL_MODULE |347 |328 |32 bytes, unchanged |
| LOAD_GLOBAL_BUILTIN |363 |369 |32 bytes, unchanged |
| CALL_PY_EXACT_ARGS |973 |934 |80→64 bytes |
| RETURN_GENERATOR |1002 |986 |16 bytes, unchanged |

LOAD_GLOBAL_MODULE at `0x38be23` loads f_funcobj from frame+32. It checks the
PyValue pointer-space tag with a shift/branch, removes the immortal pointer bit,
then loads func_globals at function+16 (`0x38be42`). Golden51 reads cached
globals directly at `0x38c1be`. The candidate therefore has an extra dependent
function load and decode; the conversion's nonobject fatal branch exists even
though frame ownership proves a function on this route. No new Python-callable
helper or function-code lookup occurs. The smaller total handler text does not
cancel that extra hot work. The candidate also performs its32-byte stack
prologue before the namespace type guard, whereas golden does it after that
guard. Its canonicalization slow path saves/restores three live argument
registers rather than golden's four; this does not imply less work on the
ordinary successful route.

LOAD_GLOBAL_BUILTIN decodes the function once at `0x38bf73..0x38bf84`, then
loads globals at function+16 (`0x38bf92`) and builtins at function+24
(`0x38bfc2`). Thus the two environment reads share the function decode within
this actual handler. They remain dependent on that function load; there is no
evidence of sharing across dispatch boundaries. Its32-byte stack reservation
is unchanged. ADD_INT and LOAD_ATTR_SLOT retain their inspected48/64-byte
reservations and total text sizes; their frame offsets change.

Exact CALL reserves96 rather than112 bytes for its child header
(`0x37a575`, header subtraction0x60; slot-count subtraction12 at0x37a579).
The selected function-environment load/store pair
`0x37a82c/0x37a836` (16-byte vector load from function+16 and child namespace
store) is absent. Candidate initialization begins directly with the remaining
mapping/frame-object zero pair at `0x37a5e7..0x37a5ef`. Result continuation,
argument overlap, code/function owners and uncertain-tail initialization remain.
Native register allocation reduces this handler's stack reservation by16 bytes;
it is an observed code-generation difference, not a promised general saving.

RETURN_GENERATOR omits the selected16-byte globals/builtins source load and
destination store (`0x346889/0x34689e`). The remaining special-field copy starts
with locals/frame_obj at source+40 (`0x345e99/0x345ea7`). Destination homes move
from generator+184 to+168. Its full physical-home move/source-NULL loop and
allocation remain, and stack reservation stays16 bytes.

This closes the requested source/geometry/dependency review without a semantic
blocker. Generated effect metadata and root's semantic/performance gates still
decide admission. Header shrink and smaller handler text do not establish an
elapsed-time win.

## Debug LLTrace qualification

Root's initial debug bootstrap failed before the semantic gate: eager evaluation
of the old `maybe_lltrace_resume_frame(frame, GLOBALS())` call decoded the
interpreter-entry sentinel's deliberately invalid debug f_funcobj. The earlier
source review had checked complete-frame APIs and precise roots but missed this
debug macro argument. This was a real source blocker, not a test-fixture issue.

Independently reviewed root's bounded three-file correction in `ceval.h`,
`ceval_macros.h` and the bytecode DSL. The helper now accepts only frame. Its
first short-circuit operand rejects interpreter/base owners before even reading
f_funcobj; its second rejects the legitimate None-function cleanup shim before
function/environment decoding. Only then does it derive globals, retaining the
NULL-environment return. Both handwritten call sites pass only frame, so no
eager namespace argument remains. The helper/macro/start-frame invocation stay
under Py_DEBUG; no new release namespace eligibility guard is introduced.

The correction is source-approved. Generated output still needs root's refresh,
followed by matching debug bootstrap and release/debug focused validation.
The pre-fix debug failure remains part of the evidence; this review does not
claim that the corrected native checks passed.

### Current corrected identity

Rechecked the clean source tree at
`c2cc6777e4d72e6688854c0423459971fc81b355` (parent `a4210e5b9e8`). The
commit contains the three handwritten LLTrace changes and the matching generated
one-argument start-frame call. Exact source search now finds no eager
`maybe_lltrace_resume_frame(frame, GLOBALS())` call in handwritten or generated
cases. No additional namespace implementation change appears in this commit.

Current ELF identities read with readelf (no inferior):

- `BASE/frame54-globals-release/python`: GNU build ID
  `cfbfa6d57a013cdcc45fbac5c6c24d35e5666fe1`.
- `BASE/frame54-globals-debug/python`: GNU build ID
  `6572eb72037b40e0b1a89dd1e253152975486d17`.

Read-only nm confirms the current release addresses and sizes match the native
table above. Debug objdump directly verifies the corrected order in
`maybe_lltrace_resume_frame` at `0x387856`: compare owner at `0x38785d` and
branch for interpreter owner at `0x387861`, before loading f_funcobj at
`0x38786a`. Function type comparison at `0x38788c` precedes its globals load at
`0x387896`; the following NULL test precedes namespace access. Thus native code
preserves the short-circuit protection for the fake interpreter-entry function
and the legitimate None-function cleanup frame. This is static native
inspection, not a claim that this reviewer ran bootstrap or tests.
