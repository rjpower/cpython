# Provider and PEP 523 staged-frame handoff

Status: root and GC full-protocol GO with the clarifications below. Provider
source implementation is frozen in the frames lane with GC final source and
fixture GO; root execution and qualification remain required. The preceding
[binder scope](binder-staging-proposal.md) covers construction but not the
constructor-to-evaluator gap in native providers. Native counted objects retain
the [selected policy](collector-policy.md); this proposal adds arena authority,
not a global precise C-root migration. Root owns merged validation and pricing.

## Ownership and representation

A provider owns an unentered thread-frame reservation until it either publishes
that frame for evaluation or retires it. Its outlined C scope owns one transition
node from before the first binding callback through its evaluator/hook call and
all hook cleanup. A hook may delegate to the default evaluator, return an error,
return a substitute result without evaluating, or explicitly consume the frame.
Those routes must have one disposition, not a guess based on a frame's old PC,
owner byte or a reused arena address.

Use the root-proposed byte layout: `unsigned char result_present:1, kind:2` in
the existing byte 21; explicit enum names `_Py_TRANSITION_KIND_EXTERNAL=0`,
`_Py_TRANSITION_KIND_RETIRING=1`, `_Py_TRANSITION_KIND_STAGED=2`. Bytes 22/23
remain retirement reason/status, and the current 64-bit record stays 40 bytes.
External/prefix producers retain kind 0; `_PyFrame_InitClearRoots` supplies kind 1;
binding/provider producers supply kind 2. Do not overload reason or header flags.
GC owns the shared header and query ports after full protocol approval.

A live STAGED node has nonNULL frame, full initialized NULL/value range and typed
header authority. Its LOCALS and FRAME_OBJECT bits authorize nullable live
fields unconditionally: a sanctioned custom hook may install one before
publication. Executing code/function pin identity and storage geometry remain
immutable during staging. Retirement masks remain close-progress snapshots. Publication or retirement handoff clears remaining/header
and sets **frame=NULL**, without a callback. It retains kind STAGED and remains
linked until the outer C scope ends. The visitor must not read frame storage
for this empty node. A NULL-frame visitor branch asserts kind STAGED plus
zero range/header/result authority and NULL prefix; it cannot silently accept
another malformed record kind. The outer owner checks only its own node's frame pointer;
it never probes the old frame after default evaluation may have consumed it.
Nested nodes remain LIFO; no interior unlink is needed.

Root approved an explicit query contract: `get_transition_shadow_status` returns
`(kind, reason, status, has_frame)` for every node. Update all callers. Retirement
fixtures select RETIRING explicitly; provider fixtures assert STAGED's transition
to has_frame=False. Do not hide empty nodes or equate logical root exhaustion
with an empty physical chain. Root-ID enumeration still reports owner objects,
so an empty staged node contributes no IDs.

## Concrete provider consolidation

Move the owner-array preparation currently in
`Objects/call.c:_PyFunction_ValueVectorcall` into a reached outlined
`_PyEval_VectorValue(tstate, function, args, nargsf, kwnames)` in ceval.c. The
object method becomes a thin delegation. Preserve its borrowed input contract,
explicit argument/function DUPs, locals policy and raw value result. The new
helper owns its transition node, calls the existing private constructor with
that node, frees the C owner buffer after binding, and evaluates with the node
still linked. Select and independently pin the code before constructor
preparation; release that extra C code pin with the stage still live before
evaluation, so the active child's own fixed-last pin remains the execution
owner. No new record enters an exact CALL handler.

`_PyEval_Vector` keeps its object-argument conversion and explicit locals policy,
uses the private constructor with its own outlined node, frees its C argument
buffer and calls the object-result evaluator with the node linked. Preserve the
object result directly; do not round-trip a hook's exact scalar object through
value canonicalization merely to share these wrappers.

Both wrappers finish from the node, never a saved returned-frame pointer:

1. If construction failed, the core already handed any child to retirement;
   no live node remains, as in the bounded binder contract.
2. If evaluation consumed/published the child, node.frame is NULL. Pop the empty
   node after the evaluator/hook returns, preserving its C result ownership.
3. If the hook returned with node.frame still nonNULL, the provider still owns
   an unentered child. Preserve the hook's pending error, retire that child with
   the staged-failure reason, restore the error, then pop the now-empty node.
   A nonNULL substitute result remains a counted native owner through retirement.
   A NULL hook result remains an error; this cleanup does not invent success.

The hook must restore current_frame and any nested reservations/scopes to their
entry state before returning, as an ordinary synchronous native call does.
Assert those invariants before outer cleanup; do not manufacture recovery from
a hook that leaves a different current frame or raw-popped storage. An explicit linked-success flag tracks constructor disposition. A constructor
failure before node linkage must not inspect uninitialized record storage.

This defines abandoned-frame cleanup on both hook success and failure. Current
`record_eval`/`parse_eval` early errors return before default evaluation; the new
outer disposition closes their actual leak rather than assuming every hook
always delegates. Cleanup cannot inspect the child's owner/PC to decide whether
it was consumed; only the explicit node transition decides.

## Publication, retirement and custom consumers

Add an outlined exact-kind/frame-identity lookup over the existing transition
chain. Default external eval entry calls it only behind a nonNULL-head check:

- After the eval loop has set `frame->previous` and `current_frame=frame`, disarm
  a matching STAGED node before the first handler/callback/poll. Current-frame
  authority is established first, so no callback sees an ownership gap.
- On recursion-entry failure before publication, disarm immediately before the
  existing frame-clear boundary. The profiler-anchor update is only pointer/
  sequence stores; `_PyFrame_TryClearExceptCode` performs callback-free progress
  and its reached slow continuation links retirement roots before the first
  may-reenter close. No callback/poll occurs between stage disarm and that
  retirement publication. The existing M2 reservation lifetime remains intact.

This lookup is not added to ordinary exact CALL/SEND inline child publication or
to every hot return. Already-active frames and resumed heap generators may enter
without a matching staged node. Their current authority is unchanged.

Freeze two narrow internal contracts for custom engines before implementing:

- `int _PyEval_ActivateStagedFrame(tstate, frame)`: caller has just published
  the frame as current, without intervening callback/poll. Before any frame
  dereference, the strict helper requires a matching live STAGED token or
  returns an error. It disarms that token and does not pop the outer
  scope. Default external entry uses the optional shared primitive; the
  selective custom-activation fixture is this strict API's actual consumer. A custom
  evaluator that publishes its own active frame uses the same contract before
  executing Python; it must supply that evaluator's active-home authority.
- `int _PyEval_RetireStagedFrame(tstate, frame)`: frame is still an unentered
  thread reservation and is not current. Before any frame dereference, the
  strict helper requires a matching live STAGED token or returns an error.
  It disarms its matching node
  and immediately invokes existing retirement with STAGED_FAILURE reason.
  Provider-abandon cleanup is its first production consumer. A custom hook may
  use it to consume without default evaluation, then perform callbacks or return
  a replacement/error. Test that actual independent-consumption route.

The default entry uses a separate optional handoff: it accepts a genuinely
unstaged frame and never reports a strict-token error. Strict and optional
entry share one kind/identity find-and-disarm primitive, with one lookup per
handoff. The nonNULL-head condition is kept at actual external entry, not
ordinary inline child dispatch. Default recursion failure
uses that optional disarm operation then its existing owner-specific clear path,
including heap-generator entry failure. The strict custom APIs return 0 on
success and -1 with an error for missing token, without reading stale storage.
A helper that explicitly expects a live staged frame must reject a missing
matching token before any frame dereference. Consuming a token twice is invalid;
a hook cannot call default again on a consumed frame or release its storage with
raw PopFrame/Copy. No compatibility fallback infers this from stale frame bits.
Default entry also accepts genuinely unstaged public/resumed frames, so this is
a consuming-API contract, not a claim that arbitrary dangling C pointers can be
validated. Rebuilt custom engines must obey it.

Empty outer STAGED nodes can outlive several nested callback activations. A new
live node for a reused arena address is independent: lookup matches its explicit
STAGED kind and current live frame pointer, skipping empty outer nodes. LIFO pop
assertions catch leaked scope nesting. No descriptor retains a borrowed frame
pointer after its actual handoff.

## Complete route and low-level consumer matrix

| Route | Authority and disposition |
| --- | --- |
| Value function provider | New outlined helper owns binding→buffer free→eval/hook→outer cleanup node; direct value result preserved |
| Object `_PyEval_Vector` | Same lifetime, explicit object arguments/locals policy and object result preserved |
| Native hook before default | Live staged child remains precise through recursive callbacks, collection, locals/code changes and borrow-scope cleanup |
| Hook delegates once to default | Actual eval publication disarms; empty outer node survives body, frame retirement and hook post-default callbacks |
| Perf trampoline evaluator | Its compile/write-state/extra-code work runs while provider stage is live; generated trampoline or fallback delegates to saved hook/default once. No frame-state read after delegate return; any saved hook obeys the same consuming contract |
| Hook returns without default | Outer owner retires still-live unentered child once, preserving result/error |
| Hook explicitly retires | Narrow consuming helper disarms→M2; outer sees NULL and never touches old storage |
| Hook custom active execution | Must use activation handoff after actual current publication; custom map authority is its explicit engine obligation, not a borrowed-frame fallback |
| Recursion-entry failure | Keep stage through entry check; callback-free disarm→owner-specific M2 clear; outer later pops empty node |
| Generic bytecode fallback | Existing outer binder record publishes current child before pop; no provider/default lookup on this direct inline route |
| Exact CALL/SEND/property/getitem child | No new head lookup, record, argument store or hot signature change |
| `PyEval_EvalFrame*` existing frame / resumed generator | No new constructor reservation; default entry may find no staged token. Preserve existing active/heap owner semantics |
| Exported constructor wrappers | Existing local binding scope remains for immediate init handoff; no claim unknown future C users are covered unless they use an outer provider/token scope |
| Unused exported EX constructor | No source users; current local scope retained, no invented outer consumer |
| `_CREATE_INIT_FRAME` shim | Separate open arena gate: shim+self exists before child creation, including pre-child failure cleanup. Success bridge is callback-free under exact numeric Temps + valid refresh proof; that alone does not cover failed-construction shim ownership |

Raw production `_PyThreadState_PopFrame` routes are ceval thread retirement and
RETURN_GENERATOR. The former runs after staged→retirement handoff or after an
already-published active frame; the latter executes only after evaluator
publication and stage disarm. Generator conversion `_PyFrame_Copy` therefore
moves an active frame, not a live stage. Other `_PyFrame_Copy` uses are retiring
frame takeover and conversion of a frame-owned heap frame to a generator; their
existing authority does not consume a live provider stage. Arena testhelper
PopFrame calls operate isolated synthetic states. No existing independent custom
hook raw-pops a staged frame; test interpreters delegate once to default.

No low-level raw pop becomes a silent token consumer or gains a release-build
head lookup in this increment. A future debug assertion must account for the
arena test's synthetic `PyThreadState` rather than blindly cast it to the larger
`_PyThreadStateImpl`. The source census plus explicit consuming helper supplies
the bounded contract; arbitrary internal raw destruction is not inferred safe.

## Focused proof and cost obligations

Use an actual selective test hook with before/after Python callbacks, recursion
guard and configurable default/return/error/explicit-retire route. Record all
kind-aware node states and direct precise root IDs before default, inside the
body and after default. Verify code/function/payload roots before publication;
STAGED has_frame=False inside/postdefault; nested staging and arena growth;
returning a substitute; raising before default; consuming explicitly then
callback/reentry; selected-code mutation; no token/storage read after consume.
Cover object and value provider paths and recursion-entry failure without
changing observer/jump admission. Root owns execution and failure diagnosis.

All current retirement query fixtures must be deliberately ported to select
kind RETIRING, while asserting the complete visible node list in provider tests.
Root-ID emptiness means logical exhaustion only. Existing native numeric leaf
boxes retain the explicit native conservative policy; no global C-root chain or
registration INCREF is added.

Two outlined providers gain one 40-byte record. Default external evaluator entry
gains the nullable-head check and a reached cold chain lookup; direct inline
CALL/SEND children do not. Explicit custom retirement and outer-abandon cleanup
reach the same helper. Price external-call frequency, lookup nesting, wrapper
stack/ABI cost, result/error preservation and actual native code before any
performance claim. This changes neither scratch NULL authority nor arena-scan
policy, and does not close independent shim/phase/alias gates for M4.

Frames owns provider/ceval/call/testhelper/tests; GC owns shared kind/header,
visitor/query and retiring-producer kind ports. Root owns private retirement
reason hooks and generation, IR owns map descriptors. Full protocol peer and
root approval precede source edits.


## Implemented source increment

Five runtime/test files implement this protocol over integrated `63c35c2b6d7`:
ceval declarations, thin `Objects/call.c` delegation, ceval provider/handoff
bodies, scoped `_testinternalcapi` hook and seven provider fixture methods. No
generated handler or ordinary inline CALL/SEND/return source changed. The shared
GC kind/query component is already present in this base.

The shared lookup requires a nonNULL live frame, so a consumed STAGED node
cannot match a NULL pointer. Strict retirement tests both the original consumed
address and NULL before any storage read. Default recursion failure uses the
matched-stage boolean for its explicit STAGED_FAILURE reason. Provider linked
flags prevent reading record storage after a pre-link or already-retired
constructor failure. Entry current-frame and debug arena-frontier snapshots
check restoration; the initial root chunk is allowed to remain allocated after
its first frame exits.

The scoped hook exercises both object and value providers with default
before/body/after snapshots, recursive before/post callbacks, arena growth and
collection, substitute/error abandonment, explicit retire and custom activation,
selected-code mutation, and recoverable recursion-entry failure. A dedicated
object-provider fixture preserves the hook's exact float object identity.
Constructor failure verifies no hook runs and no live stage remains. The entry
failure probe changes only the recoverable soft stack limit and restores it
before post-observation; it adds no production fault control.

The existing retirement query fixtures already explicitly select RETIRING under
the shared four-tuple contract. The new provider fixtures retain and inspect all
nodes, assert visible empty STAGED nodes and distinguish logical root exhaustion
from physical node linkage. Host AST parse, source-usage census and whitespace
checks pass; no candidate interpreter, compiler or native test ran in this lane.
Root source post-review, both-role fixtures, native costs and source/native
callback proofs remain qualification obligations.


GC production source post-review is GO. A scoped-test-helper cleanup finding
was corrected before freeze: an explicit running flag remains set until all
selected-code, observer and result fields finish closing. A sixth fixture makes
the selected code's last constant close reenter that helper, checks rejection
during cleanup and checks that a subsequent fresh probe succeeds. This prevents
partial-state overwrite during a code-constant finalizer; production ownership
code did not change for the finding. GC final fixture source peer is GO.


The agreed mutable-header clarification is covered by a seventh fixture: an
actual hook installs a strong locals mapping before publication, and direct
transition-root enumeration reports that mapping only while STAGED remains live.
A payload heap edge keeps the identity inspectable later; the helper bypasses
transitive traversal, so that edge cannot mask the missing direct header visit.
No incomplete-frame association API is invoked to fabricate a test state.


Final freeze: exactly five runtime/test files over `63c35c2b6d7`, 619 added and
43 removed lines. GC final source and all-seven-fixture review is GO after the
running-guard and nullable-header refinements. Root has the exact manifest and
focused test list. Host AST, source usage and diff whitespace checks pass; no
native or generation ran in this lane. The coordinator owns component commit,
integration, both-role execution and native cost evidence.
