# Execution boundary73: complete generator transfer and effect lowering

Status: grouped source-contract GO, adopted by root for implementation,
2026-11-16. This is a source-ready replacement slice, not selection of an
executable. Selected57 remains golden. Root owns independent adoption,
integration, generated products and qualification. The companion
[review73](../../issues/register-semantics59/review73.md) records the evidence
and limitations.

## Decision and implementation base

Start from selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0`. Reuse the
proved parts of72 deliberately: target-static payload interpretation,
logical-edge/physical-sink validation, source-generated placement machinery,
and the relevant ownership, monitoring and external-reader tests. Do not
cherry-pick its global continuation map, universal typed recipient conversion,
generic-provider result mode or complete variant registry as a foundation.
Those changes are not prerequisites for this slice.

Implement **SEND entry, yielded delivery, delegated suspension, resume, and
completed return through its actual consumer as one unit**. Include generator
frame retirement and the common case in which releasing the completed receiver
deallocates it. Ordinary CALL and arithmetic keep selected57's placement and
specializations. This is the first complete use of the effect ABI, not another
outward-yield shortcut or a promise to replace completion later.

The architecture is one canonical owned result carried between statically
typed native entries, with explicit transitions into effects. Execution
selects physical storage; observer and error consumers describe the storage
actually reached. An effect entry owns both its root protocol and its return
continuation. It replaces the corresponding transaction rather than wrapping
it. The same contract can later serve calls and arithmetic, after their full
specialization families are supported.

## Source and evidence coordinates

All runtime anchors below refer to real immutable runtime trees, not the stale
C sources in this documentation checkout:

* `I` = `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`, selected57 above.
* `G72` = `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc`, rejected `c6d54fd3b5d53b89d47d1dbda30080e4ebdae529` at review time.
* `E59` = `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59`.

The immediate source seams are I `Python/bytecodes.c:1467` RETURN,
`:1645` SEND_GEN, `:1788` YIELD_DELEGATED; I
`Tools/cases_generator/ir_generator.py:3585` delegated suspension and
`:5548–5689` completion/return/entry lowering; I `Python/ceval.c:3372`
`complete_step_impl` and `:3492` generator completion; and I
`Python/frame.c:451–624` staged frame clearing. G72's added common completion
work is in `Python/ceval.c:3597–3832`, `Python/ceval_macros.h:178`, and
`Tools/cases_generator/ir_generator.py:6060`.

72 passed focused behavior, but its complete delegated-yield control grew
245→253 instructions, generator completion 769→848, and coroutine completion
721→832. Maximum measured RSP decrease grew 24→40 B for yield and 272→368 B
for completion. These are reached paths, not estimated natural frequencies;
they exclude red-zone storage. The new design must replace the work behind
those totals, not book an isolated missing store as a saving.

## One concrete execution interface

Keep the configured five handler arguments and calling convention, including
the existing Py_STATS extension. The fifth scalar is the packed word at an
ordinary entry and canonical **owned value bits** at a declared transfer or
effect entry. Its native target fixes the interpretation. No value tag, global
dispatch mode query, new ABI argument or native activation record is added.
Private entry names below are proposed interfaces, not existing symbols.

Generate three outcome-entry tables from the same source declarations and
placement information that emit the bodies:

| Entry family | Selector and input | Work consumed by the selected native entry |
| --- | --- | --- |
| `GEN_YIELDED` | Parent's executing SEND handler at `frame->instr_ptr`; one yielded owner | The actual yielded-edge consumer, including delegated suspension when selected |
| `GEN_COMPLETED` | Parent's executing SEND handler at `frame->instr_ptr`; one return owner | SEND window retirement and its completed-edge consumer |
| `GEN_RESUMED` | Child's actual RESUME/declared entry; one sent owner | Resume check and the actual sent-value consumer/carrier chain |

These are static native target tables, indexed like the existing handler table,
not per-code or per-word continuation maps. Freeze and charge every table row,
target and byte of text. They are consulted at these generator boundaries only;
ordinary instruction dispatch and ordinary CALL do not consult them. Every
reachable table entry must be defined by a source-declared contract; no NULL
entry followed by generic repair on a supposedly admitted fast path.

The shared cold reverse row is the backend memo's eight-byte
`_PyIRGeneratorBoundary`: uint16 `ordinary, handler`, uint8
`yielded_consumer, completed_consumer, resume_consumer, flags`. Consumer codes
are NONE, DELEGATED, RETURN, STORE_LOCAL, DISCARD and SEND. Generate
`_PyIR_GeneratorBoundaryOf(handler)` and the cold selection query
`_PyIR_GeneratorBoundaryHandler(ordinary, yielded, completed, resume)` from the
same declarations as the bodies. The query succeeds only for complete
family/role/deopt/monitor closure. The row grants no owner or error fact; homes
come from the checked source formats and physical transfer proofs. Private
effect stages are not public handlers. Dense upper-bound data is32*N bytes on
LP64:8*N reverse rows and24*N native target pointers, or11,744 B at I's367
handlers before additions. Record the actual final size rather than assuming
sparsity makes it free.

At a transfer, `frame` is the receiving frame, `slots` its physical base,
`next_instr` the source site belonging to that entry, and the payload the one
owner. The entry loads the source operands once and lowers its selected
consumer. An owner-forwarding landing that merely dispatches a second generic
adapter is not the required implementation. A normal dispatch after an actual
store, discard or completed consumer remains legitimate and is charged.

**Keep `return_pc` unchanged.** It remains the actual successor or committed
debugger landing. The waiting parent's existing `instr_ptr` identifies SEND
before any callback; do not repurpose `return_pc`, put a function pointer in it,
or add a second frame pointer/descriptor. The ordinary source fields still
identify A, B and the completed edge. `return_slot` retains its selected57 raw
recipient meaning for public entry and ordinary home paths; no universal
kind/index encoding is imported. Suspended entry mode comes from the declared
resume target, not from inspecting the bits of a PyValue or from a new map.

After an effect, the original SEND pointer alone is no longer authority.
The effect captures the original semantic site and generation before its first
callback, performs the once-only operation, and consumes the actual current
frame/PC/phase afterward. Pending or applied redirects use their real landing.
A same-PC generation change is still a change. No callback path indexes an
old SEND outcome table from a newly redirected `instr_ptr`.

Native interpreter entry and FOR_ITER require explicit boundary entries with
their existing semantics. They are not fabricated SEND images. There is no
claim of optimizing FOR_ITER in this slice. Native send retains setter-before-
PEP523-hook order and a rooted input on C-entry failure; a materialized native
entry continues through its declared homed consumer without taking an old
spill again after a callback.

## Complete initial IR slice

Use the existing logical values and all-ingress analysis. Keep allocated homes,
NULL initialization, frame size and call-window overlap. Select a finite
complete shape before encoding; compiler and public-image admission enforce
the same relation. Do not infer logical absence from physical NULL.

The first enabled shape includes all of the following:

1. SEND's sent input, yielded result and completed result remain three distinct
   definitions. Its yielded result can feed YIELD_DELEGATED directly. Its
   completed result can feed RETURN directly, be stored in its real local,
   or be consumed as discard. STORE/discard are actual
   consumers, not artificial accumulator spill steps. A further unsupported
   consumer leaves the whole connected shape ordinary before activation.
   An admitted completed edge cannot instead publish A and dispatch an
   unchanged consumer merely because its direct body was omitted.
2. YIELD_DELEGATED takes the outgoing owner and commits the real suspended
   frame, exception links, recursion credit, profile state, PC and external
   detach. It dispatches that owner to the parent's yielded consumer. It
   leaves no live outgoing owner or future resume-input owner in the suspended
   frame. Its reserved future input home stays NULL.
3. The actual RESUME plus transparent NO_INTERRUPT backedge feeds the next
   SEND without an intermediate owning-home round trip. Compile and validate
   that exact logical chain, including alternate ingresses. Retain the
   eval-breaker/instrumentation-version, TLBC and platform checks required by
   the existing RESUME fast skip. A changed observer or pending-work state
   enters the explicit resume effect before skipping anything.
4. Leaf YIELD_VALUE and initial created-frame entry supply the other ends of
   this cycle. An unrelated yield/resume recipient can use its real home,
   local replacement or discard entry; it cannot be mistaken for the
   transparent delegated chain. A public or alternate ingress that lacks the
   whole shape remains a valid ordinary placement chosen before activation.
5. RETURN from the generator performs real child retirement, then enters the
   parent's completed outcome. CLEANUP_THROW and generic SEND completion
   share the checked completed edge where their logical join requires it.
   They do not borrow the yielded edge's B+2 recipient.

Generate the finite placement bodies from the parsed operation/storage model.
Preserve the source words needed for independent entry, exception attribution
and observation. Different lines or columns do not prevent a disabled-observer
span from using a direct consumer. Source positions never establish ownership,
execution eligibility or continuation identity.

Preserve the full SEND family: ordinary generic, GEN, VIRTUAL, ASYNC_GEN,
deopt/role aliases and monitor counterparts that can replace the same site.
An adapter for a provider that really calls Python materializes its inputs at
that effect, then executes that exact provider once and enters the new outcome.
It must not demote the site to generic SEND because the desired specialized
body lacks a placement variant. The same applies to any constructor, builtin,
descriptor, float or Unicode specialization when later expanding the ABI.

The compiler closure rule is cold: derive the complete possible same-site
target set from existing source families, role mappings, deopt targets and
monitor nominations. Require compatible physical entry/output contracts for
all members before selecting the shape. A missing member keeps the entire
site and consumer ordinary before encoding. An explicitly serialized
incomplete shape is rejected by common public admission. This introduces no
hot closure check and no separate proof system.

## Effects own the retirement suffix

Separate native functions are required for ordinary transfer and callback
execution. Conditional root allocation inside one large C function is not a
split: its stack frame and live ranges can still tax every invocation.
Ordinary transfer must tail-enter its effect **before** the first returning
callback-capable call. The effect entry allocates the existing root protocol,
owns the remaining suffix, and dispatches the actual continuation itself.
It does not return through the ordinary transfer's native frame.

Classify effects from actual source behavior. Callback-free allocation, possible
allocation failure and scheduled GC are distinct from running Python, polling,
observing or finalizing a close. Error construction/cleanup has its own reached
entry. No successful path stages every owner merely because failure cleanup
can call Python. Creation itself is unchanged in this first slice.

Use the selected `_PyValue_TryXCloseNoReentry` semantics and selected staged
frame-clear order. It performs a successful non-reentrant close; a false return
leaves the owner untouched. Do not replace it with a prediction followed by an
unchecked close. Do not preflight all input refcounts: earlier closes can make
an aliased later input final.

The completion protocol is explicit:

1. Before PRE observation, the original input window remains authoritative and
   the result has one owner. If observation is required, tail-enter the PRE
   effect before changing it; root the result immediately there. Preserve
   PRE's actual error and redirect behavior.
2. On the callback-free route, process input releases in the existing order.
   Leave not-yet-consumed owners in their authoritative homes. Try each close
   at its actual turn; on success clear its home. At the first potentially
   reentrant release, tail-enter the source-generated remaining-suffix entry.
   Its target/cursor identifies progress; it receives the result payload and
   reconstructs every other owner from the still-owned homes. No hidden C
   temporary may cross this tail boundary.
3. Before the first callback, the effect detaches all remaining input owners,
   installs the real output or discard state, and publishes the reached
   continuation. Handle A/input overlap by detaching the remaining window
   before output publication. This gives POST observers and finalizers the
   same committed state as selected57. Earlier non-reentrant releases have
   completed and are never replayed. A callback-free route may defer this
   publication until its non-reentrant releases finish; there is no observer
   between them. Exception, sent value, cursor, receiver, displaced binding
   and unused result retain their original relative release order.
4. The effect roots every detached remaining owner until its consuming close,
   removes an owner from the root before deallocation, and finishes exactly
   the remaining suffix. It must not call the old whole
   `complete_step_impl` after partial progress. The existing observer primitive
   and ownership-root machinery may be reused; the enclosing transaction is
   replaced for these admitted paths.
5. A reached POST callback sees a real result home. Continue with the actual
   materialized consumer, including its specialized body. Do not retake the
   old A because the PC happens to match. Errors expose the reached physical
   checkpoint and never an invented successful carried output.

Private suffix targets and a reconstructible window/slot cursor encode progress
in the existing handler arguments. They do not allocate a per-activation
continuation record. The parser/emitter owns their owner-transfer receipts,
including partial completion. If a live value cannot be reconstructed at a
cut, change the cut or explicitly root it in the effect; do not silently leave
that cut in the old generic helper and call the slice complete.

Apply the same split to generator frame retirement. RETURN first performs the
existing unlink, state transition, exception linkage and recursion accounting.
Keep the new path under the existing `generator_return` decision; ordinary
thread return/clear gains no boundary lookup or extra kind predicate.
The non-reentrant clear loop stops with the next owner still in its home.
A private retiring entry receives the canonical result and exact remaining
stage, initializes the existing retiring roots, and finishes bindings,
association, function/locals and exception cleanup once. Retain code lifetime
and frame-object takeover rules: cleared generators retain their executable,
so do not invent a generator code-close. Its exact private ABI is
`frame=dying`, `slots=remaining-slot-cursor`, `next_instr=parent semantic site`,
`payload=result`, with `tstate->current_frame` already the parent. Fixed native
targets encode association/slots/locals/function/exception stages; this is not
an ordinary PRE_DISPATCH entry. After callbacks it reloads the real receiving
frame and landing; the retired frame is not read again. Then it enters the
appropriate completed consumer. Keep error retirement distinct from normal
return. Do not add the native-call periodic poll to Python return.
If that consumer itself returns, pop/detach the current roots before tail
transfer to the next retirement; do not retain one C effect frame per completed
Python activation.

Finalizing completion is **common work**. In the unique completed-generator
case, receiver release can immediately enter this effect. Its entry, root
setup, actual generator/free work and final result consumer all belong in the
cost. The design is not justified by a nonfinal receiver fixture alone.

## Ownership, observation and external entry closure

| Boundary | Authoritative ownership/state |
| --- | --- |
| Private transfer/consumer execution | One canonical payload owner; its reserved home NULL; other owners remain in declared homes |
| Parent waiting for child | No future result owner; real SEND site, successor, receiver owner and frame/exception links |
| Suspended generator | No outgoing or unborn incoming owner; real suspended PC/state and receiver homes |
| Effect before callback | Existing root describes result and detached/retiring owners; reached physical phase and semantic PC published |
| Effect after committed output | Frame owns the published output; the root no longer claims it; remaining input owners retire once |
| Redirect/error | Actual reached landing/cleanup receipt; no old-site or old-home authority |

Do not import72's ACC_LIVE phase or ordinary carried-input dispatcher. The
native transfer owner exists only between private entries and the actual
consumer; no ordinary dispatch target receives payload bits. Until commit,
the existing WAIT, suspended or uncommitted physical view describes actual
homes, while the in-flight result remains a native owner as with existing C
temporaries. It must not grant a fictitious result-home receipt. Before a
callback/safepoint, root or materialize and publish the reached view. Existing
asynchronous readers continue to see that physical home view, including
receiver/cursor homes, without a new debugger refusal. A suspended future-input
entry is not a live native owner. Audit frame traversal,
copy/takeover, clear, locals/debugger writers, GDB/debug offsets, remote frame
and asyncio delegated readers against these distinctions. Keep ordinary raw
recipient readers valid; no typed-recipient reinterpretation is introduced.

Monitor installation/removal and deoptimization while a child runs or a
generator is suspended must replace the entire entry contract coherently.
The outcome table must select the proper source-declared monitored/materialized
entry, not strip placement or restart a provider. LINE/INSTRUCTION, PY_YIELD,
PY_RESUME, STOP_ITERATION and error hooks run once at their actual semantic
sites. Recompute after callback generation changes. Preserve profile, DTrace,
PEP523, throw/close, recursion failure, exception chains, last-profiled frame
and generator-return classification after the last possible callback.

Public construction, replace, marshal and exec preparation use the same
logical/physical admission. Validate producer existence, all normal and
recovery ingresses, sole owning use, vacant reserved home, edge kind, generic
deopt body and every materialized/monitored counterpart. Retain the72 fixes for
physical sink versus ordinary local-sink effects, generic SEND's vacated home,
and the CLEANUP_THROW completion join as applicable source invariants, not
blindly copied generated products. FT initially selects no new carried shape;
its ordinary readers and constructors remain valid.

## Deletion ledger and costs retained

| Existing cost | Required replacement/removal in the admitted slice |
| --- | --- |
| SEND input publication through general `_PyFrame_SetReturnValue` | Actual resume consumer selected statically; effect entry handles real local/discard/redirect work |
| Yield B+2 publication and immediate delegated TAKE | One owned payload into the delegated consumer |
| Resume home reload plus transparent backedge transport | Declared resume/consumer entry with the real RESUME checks |
| `_PyEval_CompleteGeneratorReturn` immutable base-opcode decode followed by `complete_step_impl` | Source-handler outcome selection and one completed consumer/retirement lowering |
| Callback-capable completion/clear calls returning through ordinary transfer | Tail effect owns the suffix and final dispatch; no ordinary callback frame |
|72's preemptive result roots, carried/materialized branch stream and status2 return union for SEND | Roots only at the reached effect; direct native outcome entry and actual consumer |
|72's 2 B per word map, +8 B exec pointer and ordinary binding map loads | Not imported; no corresponding map is needed |
|72's global recipient-kind decoding and unsupported-specialization refusals | Ordinary selected57 contracts; complete cold source-family closure for new placements |

Keep real frames, required state/exception/recursion transitions, ownership
acquisition, actual output stores, all finalizers and observer events, native
entry guards and semantic errors. The new tables, target-word loads, phase
writes, native branches/tail transfers, partial-release bookkeeping and effect
frames are costs, not free metadata. Measure their complete paths and code
size. No frame-packing, creation, scalar, CALL, comp/Delta or parity credit is
claimed by this generator slice.

## Implementation order and qualification

1. **Shared inert interface and source closure.** Backend owns payload helpers,
   three generated entry families and effect-cut/consumer declarations;
   compiler owner adds the shared cold family-closure check and public
   admission. Keep nomination disabled. Pin exact target/table census and
   source-visible ownership cuts. `return_pc` and frame geometry stay intact.
2. **Complete transfer plus effects.** Backend emits yielded/delegated/resume
   consumers, completed local/discard/RETURN consumers, and child/input
   retirement suffixes together. Wire generic provider, native entry,
   CLEANUP_THROW and observer/materialized consumers. Port readers and source
   proofs before activation. No partial yield-only executable is the initial
   implementation deliverable.
3. **Activate the closed shape.** Compiler uses existing logical CFG/all-ingress
   analysis; malformed public shapes fail admission. Add focused source
   mutation/closure tests and behavior tests with actual dynamic entry checks.
   Root generates, builds both roles and performs integrated qualification.
4. **Inspect and correct the complete native implementation.** Reuse the six72
   controls, adding full yielded/resumed/completed cycles, unique-receiver
   finalization, local/discard outcomes, and reached callback/error cuts.
   Count from producer through actual consumer, including child teardown,
   helper frames, root pushes/pops, branches, red-zone use and code/table size.
   Require actual entry hits; a selected handler name is insufficient. Ordinary
   CALL/arithmetic controls and natural specialization snapshots must retain
   selected57's mechanisms. A predictive native failure calls for a bounded
   correction to the specified lowering, not dropping completed return or
   routing it back through the old helper to finish sooner.
5. **Measure and select.** Root applies unchanged balanced hot8 protection,
   confirmation and all29/upstream rules at a winning checkpoint. A losing
   executable stays unselected. It may receive a bounded causal diagnosis
   and a reviewable correction under the user's continuing authorization;
   a failed packet is neither a winner nor proof that the whole architecture
   is impossible. No compiler/flag/padding hunt substitutes for the source
   work.

Behavior gates include mortal and aliased values, unique/nonunique receivers,
frame-object takeover, callbacks at every real retirement position, PRE/POST
errors and generation-changing redirects, native recursion failure, PEP523,
profile/monitor installation while active and suspended, send/throw/close,
generic/virtual/async iteration, public malformed edges and remote inspection.
Assert executed branch hits where a test claims a boundary. Source-declared
negative controls must revoke ownership/effect authority when the body changes.

After this complete slice, extend the same effect ABI to common CALL providers
and their real consumers only with the full existing specialization family,
including constructor-init, builtins and descriptors. Then add arithmetic,
comparison and collection input consumers using the same source ownership
model. Replace each provider transaction when porting it; do not add a carried
result wrapper around the old publication. Physical repacking comes only after
a concrete allocation changes frame extent or initialization work and all cold
readers consume that layout. Those are ordered expansions of one architecture,
not alternate73 trials or claimed savings from unimplemented metadata.
