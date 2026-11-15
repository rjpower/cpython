# Retained generator delegation68: implementation contract

Status: approved for bounded implementation after [independent source review](retained-delegation68-peer.md).
The inert foundation is in progress; no runtime qualification or performance
result is claimed. This canonical text integrates the approved noncalling
ENTERING addendum into frozen source contract
`836d0a93bc6406392153f7c7233fc1f197dea703c424cf1b25d4344ca594a30a`.
The frozen input and addendum remain in
`E59/retained-delegation68-source-host`.
Runtime source base: `BASE/worktrees/call55-ir`, branch
`pyjit/parity68-retained-delegation`, selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0`. Canonical design input:
`python-jit` `29a016e656d1dc700954ff8e3b642534e56727f7`, design68 and review68.
BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Applicable AGENTS and the official CPython AI policy have been read. This is
one source unit. Root owns integration, generated products and every target job.

## Implementation boundary

Use the explicit representation and lifecycle below, including the cold
nonblocking remote-sample refusal. All symbols introduced below are proposed
private interfaces, not claims about existing selected57 APIs. Existing APIs
named below were checked in selected57 source. No private opcode number is
reserved by this proposal: a new internal entry label needs no public handler ID.

The complete unit must preserve an admitted prefix while the leaf returns,
the parent yields its own item, and that same parent enters its other child.
No whole-prefix rebuild on an ordinary leaf return is permitted. Root's later
natural/native evidence must establish this; source readiness is not a hit.

## Storage and allocation

Add `struct _PyGenDelegation *prefix##_delegation` immediately before the
embedded frame in `_PyGenObject_HEAD`. It is nullable and nonowning. It costs
8 bytes on **every generator, coroutine and async-generator**, including FT;
all constructors initialize it to NULL. It does not consume padding or alter
the 112-byte interpreter-frame header. Embedded-frame offsets and subtype
basicsizes change together. Membership never points into a relocatable array.

The proposed x86-64/GIL layout is exact, checked by static assertions:

| Record field | Offset / bytes | Authority |
| --- | --- | --- |
| root, leaf, links | 0, 8, 16 / 8 each | Borrowed object/array pointers |
| external_frame, external_exc | 24, 32 / 8 each | Present only during active execution |
| instrumentation_version, evaluator_version | 40, 48 / 8 each | Captured observer/evaluator generations |
| sequence | 56 / 8 | Even stable, odd mutating; remote consistency only |
| count, capacity, recursion_debt | 64, 66, 68 / 2 each | Count includes root; capacity 4/8/16/32/64 |
| state, flags | 70, 71 / 1 each | RUNNING/SUSPENDED plus mutation/retirement flags |
| **record total** | **72** | Stable address until dissolution |
| link: generator, loop_row | 0, 8 / 8 each | **16 bytes**, both borrowed; leaf row NULL |

Each intermediate row is immutable and pinned by that member's ordinary code
owner. A 32-byte `_PyIRDelegationLoop` contains four uint32 word indices
(SEND, YIELD_DELEGATED, real resume, backedge), uint32 first-range/count for
the common home view, and uint16 receiver/transfer/resume-input/flags. There
is no pointer owner in a row. Extend `_PyIRCodePlan` by an explicit uint32
loop count and alignment padding if required; price the header as 24 bytes,
up from 16, with row/range arrays included by `_PyIR_PlanSize`. Code without
a loop has no row allocation. Do not invent spare plan fields.

Allocation sizes for record plus links are 136, 200, 328, 584 and 1,096
bytes at capacities 4, 8, 16, 32 and 64. Membership adds 8 bytes per real
object: a full 64-member prefix costs 1,608 extra bytes excluding allocator
rounding and shared code-plan rows. Allocate replacement links before changing
membership. Peak old32+new64+record is 1,608 bytes, or 2,120 including 64
membership pointers. Two allocations at creation; one allocation per growth;
no shrink on pop; free once on dissolution/final root completion. A one-member
root record survives a popped child so parent-yield/right-descent does not
allocate another record.

Use exclusively the existing direct backend pair
`_PyMem_RawMalloc(NULL, size)` / `_PyMem_RawFree(NULL, pointer)` for record and
link storage, declared in `pycore_pymem_init.h`. These call the noncalling
libc backend and address-limit check. Do not mix allocation families. In
debug, apparently default Raw APIs can dispatch installed allocator callbacks;
they do not satisfy this contract. Direct backend storage bypasses installed
hooks and their debug guards; charge that choice explicitly. Optional semantic-
proof scratch remains ordinary PyMem storage.

Record/link allocation neither calls Python nor releases the GIL. Publish a
valid reached state before it, but do not add post-allocation admission checks
solely for hooks this backend bypasses. Any actual preceding callback still
requires the full reached-edge guard: current frame/PC, receiver identity and
owner, NULL transfer/cursor, child state, code/row, epochs/tools and aliases.
Membership/sequence alone cannot establish those facts. Honor any callback-
selected redirect and never replay completed Python.

NULL, growth failure or a 65th member declines at the reached edge with
original owners/PC intact and no new Python exception or provider restart.
Root must fault the actual reached record/link backend for allocation/growth
failure receipts. Generic PyMem failure injection does not cover it. Use
bounded native fault controls without a permanent runtime fault counter.

## Common admission and loop proof

Extend `Objects/codeobject.c:semantic_delegated_step` and the same admission
plan builder used by construction/replacement/marshal/preparation. The current
16-byte `semantic_delegated` role tuple is insufficient: it proves receiver
and outgoing roles, not a transparent cycle or a complete home map.

The new optional proof consumes the real parsed CFG and physical transfer
contracts. It requires all of the following:

* A real SEND child-entry edge, its real yielded edge, an explicit
  YIELD_DELEGATED, its actual RESUME and a noninterrupting backedge to that
  same SEND. No alternative entry into the skipped interval.
* SEND receiver ownership remains in the same scratch home. Its cursor is
  NULL, the outgoing transfer is its B+2 scratch home, and delegated resume
  input is that same scratch home. Local/discard/other-home resume recipients
  remain valid ordinary code but are ineligible. The only skipped transfers
  are the proved yield, RESUME and backedge; no SWAP, cleanup, callback,
  additional read, context operation or ownership retirement is accepted.
* Synthesize the actual child-WAIT physical state by consuming SEND's None
  input through the checked `IR_ENTER_GENERATOR` contract. Compare its
  initialized readable/owning home view to delegated suspension after TAKE
  of the yielded owner. Transfer is physically NULL in both, and receiver
  remains the sole child owner in both. Retained boundaries carry this
  explicit common view; MAY_OWNER or source positions cannot supply it.
* The actual None-resume path restores the transfer input needed by SEND
  without retiring another owner, and its exception/handled-context/saved
  binding state is identical at the two represented boundaries. Errors and
  SEND exhaustion leave through existing semantic completion/EH edges.

The certificate is optional for a valid image and never weakens malformed
ownership rejection. Public routes share it. Pristine words stay ordinary;
no learned handler can be installed publicly. No numeric private ID is needed
unless implementation reveals a real handler requirement, in which case root
and the source peer must first agree an unused selected-registry ID.

The common view explicitly carries `NULL_CONTROL_READABLE` authority, not a
static `MUST_READABLE` claim. Actual generator creation/copy initializes every
home, and the exact heap frame does not overlap caller storage. Full nullable
ranges describe that existing physical control; admission checks the actual
owner/lifecycle. Separately prove the receiver owner, SEND None-input TAKE,
delegated outgoing TAKE, NULL transfer and effect-free resume/backedge preserve
the same remaining homes. Neither semantic MAY facts nor the worklist's
complete bit establish readability. Generic home readers and GC remain unchanged.

Store rows in the immutable semantic plan with exact size/overflow checks and
an ordinary NULL/no-row result if optional allocation fails. Look up a row
only on a reached new SEND. Root resume/yield uses direct record leaf and
tail entries, with no row search or prefix scan. Compatible specialization
preserves immutable operands. Instrumentation mutation invalidates admission
through the explicit epoch below; lazy preparation cannot silently revive
an old record.

## Owners and effective state

| Object/home | Retained owner | Release or mutation boundary |
| --- | --- | --- |
| Root generator | Existing external owner / ordinary active entry | No record self-owner |
| Each child generator | Parent's original receiver home | Existing SEND completion/unwind |
| Function and code | Existing per-frame strong owners | Existing frame retirement |
| Yielded value | One canonical C owner, then root's real external recipient | No intermediate relay owner/store |
| Return value | Existing rooted retirement result | Parent SEND completion once |
| Record and links | Storage lifetime follows membership | No GC object edges or duplicate traversal |
| Exception values | Existing gi_exc_state owners | Existing exception-stack cleanup |

Use one proposed `_PyGen_GetEffectiveState` and a cold
`_PyGen_GetDelegationView(gen, ...)` returning effective PC, phase/recipient
and common home view. With no membership, raw fields are authoritative. With
membership, raw intermediate state/PC/phase **are not** an alternate fallback.
Cold queries may scan at most 64 links to find a member. Hot root/leaf paths
use identity plus direct tail indices. Pure state/yieldfrom getters resolve
without dissolution. Frame publication and writers materialize instead.

| Member / record state | Effective state, PC and links |
| --- | --- |
| Intermediate / RUNNING | EXECUTING at row SEND; return_pc SEND+1; return_slot B+2; internal real previous/exception chain retained |
| Intermediate / SUSPENDED | SUSPENDED_YIELD_FROM at row real resume; delegated phase/recipient from row; same physical NULL transfer and owning receiver |
| Leaf / RUNNING | Actual leaf PC, recipient and phase; EXECUTING; ordinary nested Python calls keep the real chain |
| Leaf / SUSPENDED | Actual leaf suspension state, PC and recipient |
| Root outside evaluator / SUSPENDED | previous and exc.previous_item NULL; record external pointers NULL; no departed sentinel address retained |

GC traversal uses original owning homes exactly once and the explicit member
home view. It never visits record/link pointers. Materialize before any
member's clear, finalize, deallocation or code-owner release, including an
interior selected first by cyclic GC. Weakref callbacks must not see a live
membership pointing to already released storage. Finalizer no-op tests use
effective state. FT never creates records and keeps its atomic claim protocol.

## Entry, yield and incremental suffix table

The actual existing `_PyEval_EvalFrameLoop` installs a sentinel by overwriting
the supplied frame's `previous`. Passing the retained leaf directly to it
would destroy the chain. Keep **root** as the evaluator entry frame.

Add a private `_PyEval_EvalRetainedGeneratorValue` wrapper that directly calls
`eval_frame_default_value` with a named retained-None entry value. It is the
only producer of that value. `_PyEval_EvalFrameDefault` normalizes its public
integer flag to `throwflag != 0` before calling the private common wrapper;
thus PyEval_EvalFrameEx/default still treat **every** public nonzero as throw.
Make existing internal `_PyEval_EvalFrameValue`'s throw argument `bool` and
update all in-tree declarations/callers (selected calls are generator exc
0/1 and ordinary eval-code literal0). No discriminator goes to `_PyEval_EvalFrame`
or a PEP523 hook: the new private wrapper requires eval_frame NULL and a claimed
retained root. An evaluator change before entry dissolves and calls the existing
ordinary API with Boolean false exactly once. The wrapper uses the same borrow
floor and evaluator body; it is not another evaluator. Price its additional
symbol/text and the public cold flag normalization explicitly.

`gen_send_value_ex` chooses this wrapper only for a
checked retained root, no evaluator hook, None input and sufficient Python
recursion budget. Skip the root's intermediate `_PyFrame_SetReturnValue`.
In `_PyEval_EvalFrameLoop`, dispatch the retained start label inside the
existing nonzero arm; the ordinary zero arm and its `start_frame` remain
unchanged. This uses the same activation, sentinel, borrow floor, tail-call
handlers and unwind engine, not another evaluator or fabricated instruction.
The source peer must approve this internal discriminator extension.

Shortcut interception precedes the ordinary claim/materialize route. Native
`gen_send_value` examines a member before `_Py_GEN_TRY_SET_FRAME_STATE`;
SEND/SEND_GEN/FOR do so before `gen_try_set_executing`. Only record root +
SUSPENDED + None + sufficient R + admissible observers can claim ENTERING.
All other member ingress dissolves first, then uses the original ordinary
state claim once. A successful shortcut bypasses that ordinary claim and its
one-unit recursion entry. Thus a mandatory ordinary mutation materializer
cannot accidentally dissolve every eligible root. RUNNING makes every member
executing; ENTERING makes **only the root** executing. A RUNNING member or an
ENTERING root rejects a second execution (materializing before the original
rejection when needed). An ENTERING nonroot remains SUSPENDED: an
early-entry-error callback that resumes its alias first materializes, then
uses the ordinary suspended ingress. It must not raise already-executing merely
because its record root is ENTERING.

Use a stable ENTERING state for the claim-to-first-handler interval, not an
unmodeled suspended/running blend. Effective root state is EXECUTING at its
real resume PC; nonroot members remain suspended; recursion_debt=0. Root's
new external exc link and (after evaluator setup) sentinel/parent link are
real and must be preserved. The native claim sets ENTERING, then gen_send's
existing root exception setup runs, then the evaluator links root's sentinel.
The retained label commits RUNNING/full debt and switches current/exc to leaf
without callbacks. A VM claim follows the same ENTERING phase with its real
parent rather than a new sentinel.

If the existing `_Py_EnterRecursiveCallTstate` fails **before** sentinel
construction, materialize ENTERING before the existing root close: root stays
EXECUTING, preserves its actual external exception link, and gets its real
resume PC/recipient without changing its recipient owner; descendants become ordinary
suspended frames with detached internal links. No Python recursion debit was
made. The original root clear consumes its existing receiver owners and
restores the original external exception stack; it cannot touch a live record.
This is a real evaluator failure, not optional allocation decline. No retained
pointer survives root clearing. Any callback during error construction sees
the explicit ENTERING effective view or materializes it under this same rule.

| Reached transition | Noncalling commit | Callback / recovery rule |
| --- | --- | --- |
| First eligible nested SEND | Ordinary child claim/entry/None consumption/recursion checks first; attach root+child to fully initialized record; parent row and child membership published together | Optional allocation happens before commit. On refusal, ordinary entry once. No unexecuted-child prewalk |
| Current leaf reaches eligible SEND | Check exact PyGen_Type, no alias/hooks/tools, matching epoch, child ordinary state and NULL transfer; append tail row+child; perform its ordinary entry once | Already-member child or unsupported ingress materializes at this edge. Never cache a record across a callback |
| Native root next/send(None) | Evaluator links root sentinel; retained start commits root external links, shared RUNNING and one count-sized Python-recursion debit; switch tstate current/exc to leaf | Publish None with existing `_PyFrame_SetReturnValue` only after full RUNNING commit; it can retire a local/call Python or honor pending_redirect. Reload state after setter and boundary poll before dispatch |
| Evaluator SEND/FOR resumes retained root | Generator-only branch uses existing parent's recipient/continuation, links root once and commits the same count-sized debt before leaf entry | If tools/budget/provider refuse, materialize and use ordinary already reached root entry; no provider replay |
| Leaf yields | Take canonical value; publish leaf's real next PC/recipient/state; shared SUSPENDED; detach only root external frame/exception links; clear saved external pointers; credit count debt | Existing external `_PyFrame_SetReturnValue` publishes yielded result once **after** full SUSPENDED commit; it can retire a local/call Python or honor pending_redirect. No record use after setter. Recheck tools before skipping ancestors; mismatch materializes ACTIVE then ordinary yield propagation |
| Leaf returns, count>1 | Before callback-capable retirement, remove child membership, decrement count, set new leaf, clear its old intermediate row; restore new leaf SEND/return continuation/phase from popped parent row | Existing `_PyFrame_ClearGenerator` roots return owner. Child cleanup may materialize/free surviving record. Reload parent membership/PC after cleanup; existing `_PyEval_CompleteGeneratorReturn` performs exactly one real parent SEND completion |
| Root returns, count=1 | Clear its membership and free storage before any code/header owner release | Existing root return/retirement and result/error semantics |
| Child error, throw/close/non-None/interior ingress | Materialize at actual reached state before raw ordinary ingress/unwind | Existing provider and completion once. Running interior still raises already-executing; suspended interior is independently resumable after dissolution |

Pop happens within the existing generator-only retirement path, after the
ordinary RETURN has restored `tstate->current_frame` to its real parent and
credited the returning child's recursion unit, and before the first release.
The popped member remains ordinary and nonmember for every callback. The
parent owns its old child until the existing SEND completion detaches the
window. Any cleanup redirect/error uses the existing completion rules.

Leaf resume recipients are not restricted to the intermediate certificate's
vacant scratch recipient. Native and VM retained entry first commit the full
linked RUNNING chain/debt/current_frame/exc_info, end the noncalling commit,
then invoke the actual leaf's `_PyFrame_SetReturnValue(..., PyValue_None)`.
It may close a displaced local, complete pending_redirect and materialize/free
the record. Retain only the stable leaf-frame pointer for this publication;
afterward reload `tstate->current_frame`, actual instr_ptr/return_pc selected
by the setter and fresh membership before deciding whether the optimized
entry still exists. Honor the setter's redirected result; never dispatch an
old saved leaf PC or reapply the None input. Poll runs at a fully published
boundary and similarly invalidates cached decisions. No error is introduced
by optional retention or yield.

On shortcut yield, save the real external frame/recipient continuation locally
while committing SUSPENDED, detached root links, zero debt and profiling-anchor
invalidation. Only then invoke the existing external recipient setter with the
single outgoing owner. A displaced local's finalizer can inspect/materialize,
resume or even finish the suspended root; the outgoing transfer is already
complete. After the setter, reload the actual external continuation/redirect
and use no cached record, root-state assertion or membership pointer. Preserve
existing generator_return_kind publication at the final no-callback evaluator
exit boundary. Do not turn leaf or external setters into an asserted noncalling
interval or add a universal return barrier.

One-member records are valid after pop. A parent can run arbitrary ordinary
body code, yield its own value, and later append its right child without
touching the retained ancestor prefix. Ordinary Python CALL/return inside
the leaf has no membership field, guard, allocation, effect barrier or mode.

## Recursion and materialization ledger

Initial actual descent pays one `_Py_EnterRecursivePy` for each real member;
append adds one and ordinary pop restores one. During RUNNING, `recursion_debt`
equals member count. Shortcut yield restores that count once and sets debt=0;
shortcut resume debits it once. C recursion still belongs to the one existing
evaluator activation. Ordinary leaf calls pay/restore their own debt. Check
the entire count before entering the retained discriminator; insufficient
budget dissolves before ordinary root entry so the existing descent produces
its real failure site. No speculative partial debit is allowed.

One proposed `_PyGen_MaterializeDelegation` is nonallocating and cannot call
Python. It marks the record mutating, resolves all rows while code owners
still pin them, writes ordinary state/PC/return slots/phase, clears every
membership, then frees arrays/record. It does not release Python owners.

* ACTIVE: preserve every actual frame/exception link and exact current_frame
  (possibly an ordinary leaf callee); write ancestors EXECUTING at SEND with
  an explicit child-WAIT home phase. Preserve total recursion debt: subsequent
  ordinary exits own one unit each. Root remains linked to its current sentinel
  or actual parent. After dissolution no path uses record debt again.
* SUSPENDED: write ancestors' real delegated suspension, NULL each frame and
  exception previous link, leave leaf's actual suspension intact; debt remains
  zero. Each member is independently resumable under existing rules.
* ENTERING: preserve root's EXECUTING claim, actual root external links and
  zero debt; make every descendant ordinarily suspended/detached and publish
  root's real resume PC without changing the recipient owner. The materializer
  never invokes the callback-capable input setter. Early C-entry failure clears
  without publishing unused input. A successful retained-entry continuation
  commits ordinary root RUNNING/current/exception links and exactly one root
  recursion debit, then publishes None once with the existing setter. Reload
  actual PC/redirect afterward and dispatch without another start_frame debit.
  Never restart the caller provider or add a universal pending-input field.

Numeric ledger (R denotes incoming py_recursion_remaining; n includes root):

| Ingress/exit | Before / after | Exact owner of debt |
| --- | --- | --- |
| Native retained root, R=100,n=8 | ENTERING100 -> RUNNING92 | Retained label debits8; ordinary start_frame is not also called |
| VM SEND/FOR retained root, R=100,n=8 | ENTERING100 -> RUNNING92 | VM retained entry debits8; skip its ordinary one-unit entry |
| Initial ordinary root plus seven real descents | 100 ->99 ->...->92 | Existing actual per-frame entries; record records debt8 only |
| Append ninth child | 92 ->91, debt8->9 | Existing child-entry debit1 |
| Return ninth child | 91 ->92, debt9->8 | Existing RETURN credit1; pop must not credit again |
| Ordinary nested leaf function | 92 ->91 ->92 | Existing thread CALL/return; record debt stays8 |
| Shortcut yield/resume at n8 | 92 ->100 /100->92 | Shared credit8/debit8 once |
| ACTIVE dissolution n8 | 92 ->92 | Eight ordinary eventual exits now own the debt |
| SUSPENDED or ENTERING dissolution | 100 ->100 | No outstanding Python debt; ENTERING root will pay ordinary start once if it continues |
| R=7,n8 before retained claim | 7 unchanged on optional decline | Dissolve SUSPENDED first, then existing real root/descent recursion checks and error site |
| Native or VM retained count1,R100 | ENTERING100 -> RUNNING99 | Exactly one debit, later yield credits1; same interception before ordinary debit |
| Native or VM count1,R0 | 0 unchanged on decline | Dissolve before claim, then original one-unit ordinary entry/failure |
| Early C-recursion entry failure | 100 ->100 Python debt | Materialize ENTERING then ordinary root failure/clear; C-recursion error semantics unchanged |

Admission requires R>=n so every skipped `_Py_EnterRecursivePy` would have
seen a positive old value (actual source uses remaining-- <=0). Low-budget
and recursion_headroom cases take ordinary descent rather than approximating
the source's error checks. After poll-induced ACTIVE dissolution, dispatch the
actual current leaf with debt already charged; never reenter start_frame.
For VM ingress specifically, the new generated branch is placed **before**
the existing `_Py_EnterRecursivePy` line in `enter_generator`; the retained
branch debits n and dispatches itself, while the untouched ordinary branch
debits1. There is no extra n-1 adjustment after an already charged root.

The common-plan WAIT phase/ranges are explicit authority, not a stale raw-PC
guess or a MAY_OWNER fallback. Suspended materialization uses existing
`_PyFrame_PublishDelegatedSuspension` semantics with real row operands.
Publishing a frame object, copying/taking over its storage, changing locals,
or selecting a debugger destination first dissolves the whole record. Existing
frame aliases prevent initial admission and append. Frame addresses never move
because of retention.

## Observer, parked-thread and storage closure

Add a uint64 interpreter delegation epoch (8 bytes/interpreter), initialized
explicitly. Increment on successful monitoring global/local/restart/tool
changes and evaluator/specialization-hook changes. This is a cold mutation
epoch, not a global effect counter or an ordinary CALL test. Record root
boundaries compare it and existing instrumentation version. Check tracing,
profile, actual DTrace enablement and debug lltrace at new edge/resume/yield;
there is no assumption that external DTrace enablement has a setter.

Before a cold observer/evaluator mutation makes skipped events observable,
use the existing stopped-world tstate walk to materialize records on actual
active frame chains, including parked/native frames. The real internal chain
contains every member even while the leaf is inside a normal call or a native
call that released the GIL. Suspended records need no registry: their next
root resume sees epoch mismatch and dissolves before code executes. Code-local
monitoring changes need this explicit epoch because selected57's
`_PyMonitoring_SetLocalEvents` does **not** advance the global instrumentation
version. Setter audit/allocation callbacks follow ordinary publication rules;
they may materialize any member through its getter, and callers reload state.

No generic allocation, native callout, attribute, thread-return or owner-close
hook is added. Direct record/growth allocation is noncalling; actual callback
boundaries still require revalidation. Existing frame/native-alias APIs
materialize before returning a pointer. External rebuilt code that reads raw internal state must
port to the explicit effective accessor; no ABI compatibility promise is made.

Sampling cache authority is updated in O(1), without searching for a cached
ancestor. At every shortcut root resume/yield and every retained append/pop,
set `tstate->last_profiled_frame = NULL` and increment the existing
`last_profiled_frame_seq` once, inside that transition's odd/even record
commit. Charge both stores even if the anchor was already NULL. The existing
`_PyThreadState_UpdateLastProfiledFrame` equality macro is insufficient: an
anchor may identify any skipped interior member, not the leaf or root. Leave
that macro and ordinary thread returns unchanged; add a generator-only reset
helper. Active materialization resets its owning tstate's anchor/sequence
before detaching membership too. Cold foreign/parked materialization receives
the tstate from the stopped-world walk; a cold arbitrary-member entry without
an owner hint finds its active owner using the existing tstate/frame-chain
walk, never a hot prefix search or global registry. Suspended records have no
live tstate anchor after the mandatory shortcut-yield reset.

Remote `threads.c` captures the existing frame+sequence pair, and
`frames.c:collect_frames_with_cache` must discard cached/chunk/prefetch ancestry
after a changed sequence or NULL anchor before any cache-shortened walk.
Generator membership/effective-PC validation still applies to any frame read
used for a cache hit; a cache identity cannot override a retained refusal.
The existing remote anchor write cannot resurrect an old cache generation:
sequence remains target-owned, and a later sample with a different sequence
must miss. Nonblocking races remain incomplete samples under the existing
protocol, not new claims of atomic sampling. No prefix walk is added to a
shortcut yield/resume to salvage a cached anchor.

## Selected57 reader/writer closure inventory

This table comes from selected source searches, not stale canonical C. Every
remaining raw access in changed source must be classified ordinary/no-member
or an FT-only claim; none may silently use stale intermediate fields.

| Source | Actual consumers/writers and port |
| --- | --- |
| `Include/internal/pycore_interpframe_structs.h` | Shared declaration and all subtype offsets; explicit membership declaration |
| `Objects/genobject.c` | `gen_new_with_code` and frame constructor initialize pointer before tracking/callback; gen_traverse; finalize/dealloc/clear; gen_send_value[_ex], am_send/iternext; close/throw claims and state restoration; gen_yieldfrom_ref / `_PyGen_yf`; running/suspended/state/frame getters; coroutine/async state getters and async throw finished checks. Pure reads use effective state; mutation ingress/teardown materializes; FT atomics remain ordinary. `gen_dealloc` materializes immediately after casting self, before GC untrack and **before FT_CLEAR_WEAKREFS** or any finalizer callback |
| `Include/internal/pycore_genobject.h` | Generator-from-frame uses new offsetof; `_PyGen_FinalizerIsNoop` uses effective state; declarations and exact layout assertions |
| `Python/bytecodes.c` | Generic SEND and SEND_GEN raw CREATED/claim checks; `_YIELD_VALUE`, delegated yield; RETURN_GENERATOR initialization. Record logic only on generator paths; preserve ordinary thread RETURN body |
| `Python/ceval_macros.h` | `gen_try_set_executing` must resolve/materialize a member before writing raw state; coroutine transfer predicate uses effective interface (membership always NULL for actual coro); generator-only helpers, no generic dispatch/effect change |
| `Tools/cases_generator/ir_generator.py` | Generic/SEND source substitutions around 1400/1433; `suspend_delegated`; `enter_generator`; return_to_caller; all duplicated literal canonical/abrupt-yield/created-copy contracts around 5121/6956/7872/7990/8669. Update checked source contracts and receipts together; root alone regenerates |
| `Python/ceval.c` | Retained discriminator arm in actual evaluator; `clear_gen_frame`, `clear_error_frame`; `_PyEval_CompleteGeneratorReturn` actual SEND completion; generator-only unwind materialization before frame walks; coroutine awaiting reader around 6326 uses interface. Ordinary thread clear/return/call functions unchanged |
| `Python/frame.c` | Existing generator-owned branches and cold alias/copy/native Lasti/Line ingress resolve/materialize before generic home or PC readers; created/cleared assertions; `_PyFrame_MakeAndSetFrameObject`; take_ownership/copy; `_PyFrame_ClearGenerator[Bindings]` materializes before releases; error/transition roots preserve reached current state |
| `Include/internal/pycore_interpframe.h` | Keep `_PyInterpreterFrame_LASTI`, `_PyFrame_InstrOffset` and generic home readers/writers unchanged. Cold copy/header-copy and `_PyFrame_GetFrameObject` publication materialize generator members first; generator delegated phase getter resolves. No generic pointer math, home-write or return-slot check is added |
| `Objects/frameobject.c` | `is_suspended` raw state around 3337; all f_lasti/line/locals/back/generator getters and f_lineno/f_locals proxy writers use already materialized public frames; debugger redirect materializes before any snapshot/PC write |
| `Python/instrumentation.c` | global/local events, restart/clear-tool and code instrumentation ingress; stopped active-chain invalidation before observer changes; existing `instrument_all_executing_code_objects` walk sees real full chain |
| `Python/legacy_tracing.c`, `sysmodule.c`, `pystate.c` | actual trace/profile setters and all-thread setters; frame/current-frame publication; evaluator setter and allow-specialization setter; epoch initialization. Audit callbacks occur under valid published state |
| `Modules/_testinternalcapi.c` | generator-offset layout report; copy/created fixture allocation and raw state writer near 5279 initialize membership too; any native alias getter uses materialization |
| `Python/gc_free_threading.c` | Embedded gi_iframe offset follows declaration, membership always NULL; no retained behavior |
| `Include/internal/pycore_debug_offsets.h` and remote validation | schema bump, gen membership and record/link/row sizes and fields, valid offset/alignment/overflow checks; update descriptor sizes and fixed reader buffers |
| `Modules/_remote_debugging/{frames,code_objects,asyncio}.c` and header | `frames.c:parse_frame_buffer`'s raw instr_ptr read requires effective PC, including chunk/prefetch/cache paths; asyncio delegated receiver selection uses row state, not raw gi_frame_state/phase; invalid or unstable snapshots refuse |
| `Tools/gdb/libpython.py` and in-tree stack/debug readers | cold stopped effective-state/PC resolution or explicit unsupported record diagnostic; preserve PrettyPrint quarantine, never use stale raw PC as authority |

Review every `instr_ptr`, return_pc/return_slot, phase_state, previous and
frame_obj write reached by a retained frame. The expected writer set is
ordinary leaf execution, append/pop boundary, materializer and already
materialized cold writer/alias paths. Ordinary selected thread paths need
neither a new member field nor a new check. A final source search and actual
diff peer readback must classify every occurrence, including generated products.

Concrete resolver placement: gen_traverse/state/yieldfrom paths know their
generator already; use the member view there. Retained intermediates publish
phase0 once on append so unchanged generic GC traversal uses selected57's valid
initialized NULL-control home scan, visiting only original homes. Do not put
membership resolution in `_PyFrame_GetHomeView`, LASTI or InstrOffset. Cold
`_PyFrame_MakeAndSetFrameObject`, `_PyFrame_GetCopyViewUnderNullControl`,
PyUnstable_InterpreterFrame_GetLasti/GetLine, and native alias/test ingress
first test their cold generator-owned partition and resolve/materialize there.
All public frame-object readers/writers then operate on already materialized
ordinary frames. ACTIVE materialization uses an explicit unpublished CHILD_WAIT
phase, value 8 with PUBLISHED clear, disjoint from existing lifecycle bits. Its actual
SEND PC/continuation and links are current. Generic GetHomeView returns NONE;
GC keeps the initialized NULL scan, and the cold copy adapter returns explicit
NULL_CONTROL_READABLE. Do not publish a MUST map or relax the generic shadow
check for valid tagged iteration indices. No generic membership branch is added.
Thread execution's generic PC/home primitives remain exact selected source.
Native remote parsing has its own explicit generator partition.

## Remote authority and lifetime

Selected `_Py_Debug_OffsetsSchema` is 1. Bump it for this added layout and
export explicit record/link/row offsets plus state constants. Existing remote
code already reports `RuntimeError` on incomplete/broken frame chains and
read failure; do not invent an exception-suppression policy or return fabricated
frames. `process_frame_chain` also has chunk, prefetch and cache shortcuts that
must not bypass the new authority check.

Nonblocking mode stays nonblocking. When the freshly read generator prefix
shows non-NULL membership, do **not dereference that pointer**. Return an
explicit unstable/unsupported retained-delegation sample RuntimeError through
the existing incomplete-sample path. A caller may retry; no unbounded internal
retry or silent stop-all escalation. Ordinary nonmember samples retain their
existing race protocol. A nonblocking alias can become a member between reads,
so the generic reader must re-read the containing generator membership after
collecting raw frame fields and refuse if either observation is non-NULL or
changed. It must also reject a stale cached/prefetched member image rather than
using a missing fresh check as absence authority.
These observations bound refusal only; they do not prove an ABA-free snapshot.
A nonmember->member->nonmember transition between reads remains governed by
the sampler's existing racy incomplete-sample protocol. Claim no stronger
nonblocking lifetime or coherence guarantee and never dereference record storage
in that mode, even when two observations agree. Blocking is the supported
complete retained-record inspection path.

Blocking mode already pauses target threads. After pause, invalidate cached
pages, reread the generator header and membership, then validate even sequence,
state/count/capacity, root/leaf identities, bounded link membership, pinned code
and row bounds before resolving effective state/PC. No target can free the
record until resume. If the target stopped during a mutating publication,
report an incomplete sample; do not spin while it cannot finish. The writer
marks mutation **before** changing any member and clears all memberships
**before** freeing record storage. Stopped readers that see a remaining member
therefore cannot be holding an already freed record. Corrupt/odd/count>64 or
inconsistent rows fail, without a raw fallback. Remote read does not materialize.
ENTERING/retiring snapshots are rejected even when their sequence is even.
Every append/pop/resume/yield/materialize commit is bracketed by odd/even
sequence publication; full thread suspension can land between any stores and
does not itself establish a committed snapshot. A stopped read must validate
the record before interpreting a frame's raw fields, not after manufacturing
a source location from stale instr_ptr.

## Added work and source acceptance

Charge the universal +8-byte subtype allocation and constructor store; nullable
membership checks on ordinary generator native and VM entry, yield, return,
state/frame/delegation getters and teardown; root epoch/tool/poll/budget work;
the 72-byte record, link allocation/growth, immutable plan rows/ranges; cold
materialization and remote refusal. The FT pointer store/layout cost is real
even though FT never retains. Ordinary generator depth1 may lose. Ordinary
THREAD CALL/return, attributes and native callouts acquire zero semantic checks,
fields or allocations. Source equality is insufficient for native-cost claims;
root's native review must verify actual emitted paths and simultaneous stack/text.

## Implementation sequence

1. Implement shared declaration, initialized constructors, immutable optional
   certificate/plan geometry and effective view/materializer; source/host tests
   establish malformed/valid-ineligible public images. No nomination/runtime claim.
2. Implement one complete generator-only entry/descent/yield/pop/error lifecycle
   with count1 retention and explicit debt. Add discoverable
   `Lib/test/test_ir_retained_delegation.py` covering long-leaf and natural tree
   suffix churn, return/PEP479/exception-context/recursion. Root generates/builds
   and runs the smallest relevant two-role increments.
3. Close every reader/alias/writer, trace/profile/evaluator/parked-thread setter,
   GC/interior teardown and remote path in the same unit. Add failure/observer
   and owner tests before nomination. Fault the actual reached direct backend
   for record/link allocation failures, without permanent runtime counters.
4. Independent actual-diff postreview, then root merged generation/build/focused
   execution. Only a coherent complete checkpoint can seek natural/native
   mechanism receipts and the protected screen.

Runtime tests: depths 1/2/8/32/64/65; many-yield leaf; repeated left-return →
parent-own-yield → right-descent with values/return owners; real finalizer and
weakref lifetimes; non-None/interior resume, running rejection, throw/close/finally;
handled exceptions and recursion failure with ordinary nested calls; duplicate
and cyclic receivers/root and interior GC teardown; first allocation and each
growth OOM with no replay; pending callback and monitoring/profile/trace/evaluator
changes inside leaf; existing and newly acquired frame/native aliases, copy,
locals and supported redirect; blocking remote state/PC and explicit nonblocking
refusal; compiler/CodeType/replace/marshal/prepare proof agreement. Behavioral
tests do not inspect record counters as substitutes for Python outcomes. Root's
bounded native receipts independently require zero skipped ancestor bodies and
zero per-member stores at depths 2/8/32, retained prefix across the actual tree
transition, and changed-suffix-only append/pop. This proposal claims no runtime
result, logging fix, speedup or complete four-workload parity budget.
