# Certified CALL input roles

2026-11-13. Root froze the encoding/interfaces; source packets are in progress
against integrated `6ff13875cfe` as recorded below. NULL control remains.
IR owns generated families and common admission/proof work. This increment
removes specific call-hole reads; the remaining all-phase M4 gates stay open.

## Actual missing authority

`regcfg_passes.c:_PyRegPass_ElideNull` retains PUSH_NULL's virtual register and
reserved window cell but emits no value store. `assemble.c:handler_oparg` also
clears LOAD_GLOBAL's low bit: the frontend `_NULL_SLOT_CONDITIONAL` output
remains a virtual NULL, while the executing LOAD_GLOBAL writes only the callable.
These are positive logical producer facts. A zero physical word, a source line
or an in-bounds reserved slot is not that fact.

The compiler must preserve the self-hole role while virtual operands still
exist, before compaction/elision/handler selection erase its producer identity.
Track definite virtual NULL through checked copies/renames and all-predecessor
joins. LOAD_ATTR's method output is nullable at runtime, not definite virtual
NULL; it stays dynamic. Passes changing inputs must recompute or invalidate the
role, and `_PyRegCfg_Verify` must check it. Keep the reserved two-cell callable/
self prefix and argument placement, including zero-copy overlap.

There is a second EX hole: `codegen_call_helper` emits PUSH_NULL for absent
kwargs at window3. EX helpers currently read it as an object-or-NULL. Cover that
declared role too; fixing only self does not make EX safe for stale scratch.

## Canonical encoding

Add dedicated canonical IR-only families, with pinned numbers and unchanged
A/B/C/D fields:

| Canonical family | Self input | EX kwargs input |
| --- | --- | --- |
| `CALL_NO_SELF` | Virtual NULL; window1 is never read | Not applicable |
| `CALL_KW_NO_SELF` | Virtual NULL; window1 is never read | Names remain the real trailing tuple |
| `CALL_EX_NO_SELF_NO_KW` | Virtual NULL; window1 is never read | Virtual NULL; window3 is never read |
| `CALL_EX_NO_SELF_KW` | Virtual NULL; window1 is never read | Real readable kwargs home3 |
| Existing CALL/CALL_KW | Readable nullable self; resolve once | Existing names contract |
| Existing CALL_FUNCTION_EX | Readable NULL self protocol | Readable nullable kwargs protocol |

Generate specialization and instrumentation twins from the same family/IO
contract. Do not hand-maintain another set of call bodies. These are canonical
image roles, not execution-only refinements: public normalization/marshal must
preserve them. Existing execution variants that normalize to an ordinary
handler cannot carry this promise. Semantic opcode projection can remain CALL/
CALL_KW/CALL_FUNCTION_EX for tracing/disassembly stack behavior, while canonical
family/deopt identity retains the role. Generated 16-bit handler metadata and
matching deopt/instrumentation targets must not index the 8-bit opcode table
with an IR-only ID or erase the role on a miss.

The compiler selects these forms from virtual producer provenance, using new
CFG flags rather than operand bits. A public image explicitly declares the
same role. The common normalized-image proof checks its window bounds, roles,
transport/owner obligations and holes; a known owner in a declared hole is an
error, not silently retired. Complete proof across unknown/error/observer paths
remains required before stale scratch. The initial NULL-control increment must
not call an uncovered role/owner proof exact or bypass common admission.

## Runtime helper contract

Use explicit `_PyCallSelfRole` ABSENT/PRESENT at runtime. A no-self handler
starts with literal ABSENT and supplies virtual `PyValue_NULL` to any shared
value expression without loading window1. Dynamic forms first prove readable
nullable input and resolve it once. Helpers take the role, not a borrowed
pointer they use to rediscover NULL. Store it in the existing outlined call
transaction and, where needed, a small handler register local; no frame field,
owner array or new head is needed.

Bound-method expansion takes ABSENT, publishes the real function and owned self,
then changes the role to PRESENT before closing the wrapper or calling out.
Preparation can return the resulting role/status in a small scalar encoding;
do not force an addressable owner out-parameter into every handler. Deopt after
effects forwards the actual prepared role; it must not return to an entry form
that assumes the original hole. Refresh validates site/phase/role along with
existing frame/continuation checks. Monitoring uses the actual role when taking
the first argument snapshot.

Constructor entry starts ABSENT. `_ALLOCATE_OBJECT` explicitly publishes the
new instance in window1, making the prepared constructor role PRESENT before
the class owner closes. `_PyEval_CreateInitFrame` already requires that actual
instance; its exact argument mover must receive PRESENT explicitly rather than
testing window1. Do not add a generic call-role branch inside its shim protocol.

## Actual reader and writer ports

| Source consumer | Required change |
| --- | --- |
| `ir_generator.py:call_reload`, `prepare_call`, `call_completion`, `IRStack` window loads; `ceval.c:call_transaction_begin` | Substitute virtual NULL and skip hole loads in generated reload/recheck/consume/clear. Rechecking a whole window is itself a read. Preserve window geometry and successful/error DEAD ordering. |
| `ceval.c:call_transaction_expand_method`, `call_record_expand_method` | Consult explicit role, publish real self before changing it, return/retain prepared role across callbacks; never inspect absent home1. |
| `call_transaction_monitor`, `_MONITOR_CALL[_KW]`, `_SPECIALIZE_CALL[_KW]` | Select first argument from the role; pass virtual NULL or real self to existing instrumentation/specialization APIs without reloading the hole. Specialization installs the same-role target. |
| `call_transaction_invoke`, `call_transaction_push_python_fallback`, `call_transaction_overlap_python`, `call_bind_known_python` | Derive offset/count and cached binding SELF checks from role. Preserve revalidation and selected-code/default guards. EX passes local NULL for absent kwargs, not a load from home3. |
| `call_transaction_detach`, copy/retirement buffers, emitter IR_CLEAR_SLOTS | Materialize NULL in native retired-buffer hole entries without reading arena holes; do not count/close another owner. Clear only actual taken homes; leave inactive physical cells untouched. |
| `call_transfer_exact_args`, `_CHECK_FUNCTION_EXACT_ARGS`, `_INIT_CALL_PY_EXACT_ARGS` | Derive argument base/count from the explicit role, keep overlap geometry unchanged, and move a real self only when PRESENT. Constructor calls the same mover with PRESENT. |
| `_CHECK_CALL_BOUND_METHOD_EXACT_ARGS`, `_INIT_CALL_BOUND_METHOD_EXACT_ARGS`, `_EXPAND_METHOD[_KW]` | No-self entry has no physical NULL check. Expansion explicitly creates the real self owner; later guards/initialization consume PRESENT. |
| `_CHECK_OBJECT`, `_ALLOCATE_OBJECT`, `_CREATE_INIT_FRAME` | No-self type guard supplies virtual NULL; allocation publishes fresh self before prepared helper entry. Keep current function pin, capacity/code checks, staged failure and recursion accounting. |
| CALL_TYPE_1/STR_1/TUPLE_1/LEN/ISINSTANCE; BUILTIN_CLASS/O/FAST/FAST_WITH_KEYWORDS | Derive nullable-self expressions/argument adjustment from role, including fixed NativeScope adapter. Even a body that ignores `null` must not inherit a whole-window recheck of it. |
| METHOD_DESCRIPTOR_O/NOARGS/FAST/FAST_WITH_KEYWORDS; CALL_LIST_APPEND | Select actual receiver from PRESENT self or first explicit arg; list-append's real self contract stays checked. Do not broaden a method-only specialization to an absent receiver. |
| CALL_KW_PY/BOUND_METHOD/NON_PY and instrumented KW | Same role-preserving preparation, feedback, reload, binder and instrumentation path; real trailing kwnames remains readable. |
| CALL_EX_PY/NON_PY_GENERAL, `_MAKE_CALLARGS_A_TUPLE`, tuple/kwargs preparation and monitor | Self is virtual or explicitly readable NULL; absent kwargs is a separate virtual role. Tuple preparation reads callable/tuple only, preserves arguments across callbacks and never treats either hole as an input owner. |

Cold validation, debugger reconstruction and phase generation use these same
declared input roles. Retained native C providers/arrays are not new arena roots.
Other elided PUSH_NULL protocols, including SEND's NULL/index state, still need
their own role port before M4; this proposal does not silently certify them.

## Writable offset capacity is separate

`call_transaction_invoke` currently grants PY_VECTORCALL_ARGUMENTS_OFFSET for
absent self. `_PyValue_VectorcallPrepend` saves args[-1], installs a DUP(self)
there across a delegated call, then closes/restores it. A virtual hole supplies
neither readable-value nor writable-owner authority for that operation.

Initially omit OFFSET on calls using an arena window, including resolved-absent
dynamic calls, until an explicit writable-window capability exists. This adds
no unconditional NULL store or hidden arena owner. Object adapters already
create their own native offset slot; Python providers do not require it; exact
PyMethod expansion publishes the real self before invocation. Remaining reached
prepend users use the existing native scratch-copy branch. Real native arrays
with actual offset capacity keep their API. Price that branch's frequency and
copy/allocation work; do not claim that withholding OFFSET is cost-free.

## Coherent file set and evidence

Change compiler conversion/role pass and checks (`regcfg.c`, `regcfg_passes.c`,
`pycore_regcfg.h`), assembler selection and operand layout, pinned handler
numbers/cache magic, generator family/IO/effect/phase/deopt metadata, generated
cases/targets, and call helpers together. Common CodeType/replace/marshal/frozen
admission must preserve canonical role IDs, reject invalid role/owner geometry
and project them correctly for disassembly/instrumentation. This is a format
change; hard extension compatibility is not an admission gate, but updating all
actual helpers/providers is required. No source-position sideband or per-call
generic role decoder is selected.

Fixtures must exercise compiler PUSH_NULL and LOAD_GLOBAL virtual producers,
method-producing LOAD_ATTR, joins/copies, zero args and kwargs, EX with/without
kwargs, malformed declared holes through every public image route, and each
specialization/instrumentation/deopt role. Add real bound-method expansion and
unexpanded native prepend with callbacks/GC, observing native-copy behavior,
plus constructor and caller-local mutation/redirect compatibility. Use admitted
programs and bounded cold mismatch probes, not stale-image execution before the
switch. Native inspection must show no no-self slot load/recheck, no extra
record on exact CALL/SEND, preserved zero-copy arguments, and account for added
handler/code/data size, role/argument traffic and actual prepend copying.

GC owns this proposal/census; freeze source ownership with IR and frames before
implementation. IR's generic-binary region is disjoint, but shared emitter
metadata/header and bytecodes hunks integrate through root. Source status is
recorded below; root owns native validation.

## Bounded peer disposition

IR and frames have design GO. IR requires all-predecessor virtual provenance,
revalidation after input rewrites, and consistent ReadsHome/nullable/physical/
outcome/observer APIs so a generic full-window reader cannot reintroduce a hole
load. Canonical identities must survive normalization, marshal, deopt and
instrumentation using the 16-bit family domain. Frames requires explicit role
forwarding through rechecks, copies and offset helpers, including EX kwargs
and post-effect method/constructor promotion. Both agree that withholding arena
OFFSET closes the unproved writable-hole capability; its native-copy cost must
be measured. Root froze the encoding and coordinated source ownership before
implementation.

## Source checkpoint

GC's source packet uses integrated `6ff13875cfe` as its base. Its files are
`pycore_regcfg.h`, `regcfg.c`, `regcfg_passes.c`, `assemble.c`, `ceval.c`, the
call-only bodies in `bytecodes.c`, and `_testinternalcapi/value_call.c`.
IR owns synthetic families/pinned IDs, emitter/metadata, common admission and
observer/outcome proofs, cold specialization mapping, and admitted role tests.
The components require combined generation/building; neither is standalone.

Runtime SELF_PRESENT=32 and KWARGS_PRESENT=64 use the existing call-state
scalar and old transaction preparation field, preserving runner argument count
and cold-record extent. Method preparation always returns the updated state;
FAILED=128 marks an error after promotion, and both callers assign state before
testing it. Thus error/deopt cleanup cannot recover the old absent-self role.
Exact transfer takes an explicit enum role. Shared clear/native publication
and detachment skip absent cells rather than reading or filling them.

The compiler proof intersects canonical entry facts over complete continued
regions, retaining a single fresh-register work vector. Persistent state is
bounded by blocks times the actual canonical stack domain (at most 4095), not
blocks times all fresh registers. Fresh definitions replace their facts before
use. COPY/SWAP snapshot inputs, declared through geometry preserves values, and
exception joins retain only the canonical handler prefix. Flags are selected
after convergence and checked after existing structural verification. No-call
functions and verifiers without role flags skip proof allocation.

VCallPrepend is a test-only native delegate in the existing value-call module.
It holds a GC-traversed bound method, rejects incoming OFFSET before any
args[-1] access and forwards the original arguments/flags through its real
value provider to the prepend-copy branch. An argument fingerprint adds a
source-mutation check without another owner bank; Python fixtures must also
check exact receiver/argument/keyword identity through callbacks and failures.
Frames' runtime peer and IR's fixture and compiler resource-proof peers are GO.
The common public-image absence proof, coherent generated residual inspection
and root native tests remain before the packet freezes. The absence fact must
exclude logical home obligations, including inline/control values; lack of a
possible counted owner after an unknown operation is insufficient. Successful
CALL cleanup certifies only its actual window, not arbitrary parent scratch.
