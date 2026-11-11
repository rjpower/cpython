# C2: select leaf calls once and reuse certified facts

Proposal only. This round closes at useful 48; C and gate-off remain experimental
and unpromoted. No runtime edits, builds or native tests accompany this document.

The primary common-completion design lane is now
[committed completion before owner retirement](C2-completion-review.md). The
scalar nonfinal proof below is a bounded fallback, not a claim that refcount
preflight is semantically necessary. Removing it first requires the durable
PRE-next retirement scene and observer/redirect proof described there.

The matched C profile places 14.38% of flat DeltaBlue samples in the completion
helper, 8.50% in `_PyIR_InstrFeedback`, and 5.65% in `ir_leaf_read_path`. These
are root-supplied measurements, not estimates of independent savings. The
release binary also shows three read-path calls per conditional completion and
a 288-byte completion-helper stack reservation with a canary. Its ordinary raw
probes are inlined. This supports examining work repeated inside the completion
transaction before changing ordinary readers or adding more admitted syntax.

## Attribution gate

Root's diagnostic gate-off change (`af60e5eb43e`) compiles away the checked-CALL
leaf gate and completion execution while retaining shared probes and certificate
construction. Root's gate-off/frozen48 ratios are DeltaBlue 1.01954, logging
1.08961 and nbody 1.00025, compared with full C 1.15213/1.21281/0.99705.
This attributes much of the loss to the optional gate plus transaction. The
remaining logging loss needs separate attribution; a gate that was compiled
away cannot explain that residual. No measured leaf helper samples in logging
also makes the ordinary CALL gate a serious concern in full C. Do not promise
recovery from the profile percentages alone.

## Select the leaf CALL handler at specialization

Add one specialized sibling, provisionally `CALL_PY_LEAF_EXACT_ARGS`, selected
by `specialize_py_call` only after normal exact-call eligibility and successful
whole-body certificate construction. `function_kind` already obtains the
callee execution before CALL specialization. Never select this sibling by
function/name/benchmark identity. Keep ordinary `CALL_PY_EXACT_ARGS` free of an
optional certificate lookup or leaf-helper call.

Reuse the same existing checked uops in both DSL macros and validate their
identical normalized prefix, guard order, cache selectors and CALL window.
Do not add a macro-composition framework to avoid a short repeated prefix.
The leaf sibling adds the terminal
attempt before the existing initializer; the ordinary sibling proceeds directly
to that initializer. They use the same handler ABI and ordinary call fallback.
The leaf handler still validates the selected current certificate before use;
selection does not pin an execution pointer through future lifecycle changes.
MISS leaves homes intact and runs the ordinary initializer, or deopts to generic
CALL before effects if its checked assumptions no longer hold. No wrapper has
been retired and no input has moved before that decision.

Use the same existing CALL feedback payload and function-version guard; do not
cache a borrowed code/region pointer in feedback or add a per-call record. The
new handler needs a generated identity, canonical generic CALL shadow/deopt,
and the same exact roles/extents/window/effects format as its checked sibling.
Audit all handler-indexed generated tables and opcode/magic conventions; root
owns regeneration and any required format revision. Peer recommends explicit
retired slot 112 reuse with magic 64419, following A's slot 111/magic 64418
precedent, subject to the actual restored base. Update the removed marker,
opcode maps, statistics, instrumentation and pinned contract digest together;
do not widen uint8 handler tables. This is one identity and
one shared composite, not another instruction family catalogue. Strengthen the
positive marker/source validation to accept this exact structure and reject
incomplete prefixes, wrong caches, intervening effects or bound-wrapper paths.

## Resolve immutable feedback addresses during certification

`Python/ir.c:leaf_feedback` already resolves and validates the immutable feedback
unit, initializer kind and payload extent. Retain that unit instead of throwing
it away. Each retained attribute proof reference should contain its instruction
word index and feedback unit, for example a scalar `_PyIRLeafProofRef` with
`word` and `feedback_unit` fields. A global root retains its corresponding unit
alongside the existing root word index. All indices remain bounded integers;
the certificate owns no object or execution-storage pointer.

`ir_leaf_read_path` should address the current execution's entry with the cached
unit and `_PyIR_FbEntry(exec->entries, unit)`. It must not call
`_PyIR_InstrFeedback` or decode the original instruction's format on a hit.
Still read the live handler and current typed payload; retain supported-handler,
header-state/kind, provider and owner guards. A certificate proves where a proof
lives, not that a runtime specialization remains valid. Existing whole-final-IR
roles/effects/extents checks, branch bounds, killed-source rules and fresh
`code.replace` construction remain unchanged.

This changes the representation of an existing read path. It introduces no
opcode dispatch loop, second handler-semantics table, generated function recipe,
name recognition or checker exemption. Ordinary handlers continue to use the
same audited raw guard/read primitives exactly once.
Their guard obligations are recorded in
[C-performance-audit49.md](C-performance-audit49.md), including writer,
type-layout and dictionary-key proof sources; those proofs cannot be weakened
to remove the remaining logging regression.

## Consume the checked CALL phase

The hook follows exact function-version/arity, stack and recursion guards. There
is no callback, release, allocation or GIL drop between those guards and the
hook. The generator's positive structural validation should establish that
phase and the exact callable/window relationship. C2 may then consume those
facts rather than checking version and arity again inside the helper.

A bounded private interface is still four scalar arguments: tstate, caller,
site, and the already checked function pointer, replacing the checked-version
argument. Decode destination/window indices once from the current CALL word.
The pointer is borrowed from the untouched callable home; it is not a pin or
new owner. This requires no public ABI, handler calling-convention, compiler
configuration, addressed argument record or additional ownership protocol.
Actual implementation review must verify the strict API checker accepts it.

Keep dynamic provider, observer, pending-redirect, current-frame, clearing,
RESUME and epoch admission. Passing a checked function does not certify its
current provider or observation state. The final asynchronous epoch check stays
immediately before commit. The plain-exact marker remains unavailable to a
bound-method handler that has already retired a wrapper.

## Make unary retirement scalar

The primary completion shape is formal arity one, provided as either an explicit
argument or split self, with a TEMP result inside the consumed CALL window:

| CALL form | Consumed homes | Count-owning candidates |
| --- | --- | --- |
| Explicit argument | function, NULL, argument | function, argument |
| Split self | function, self | function, self |

The structural test is checked `argc + has_self == 1`, destination at or above
the caller's local boundary, and destination inside the entire consumed window.
Exclude `IR_NO_DST` and old LOCAL displacement from this scalar case. A result
may occupy the NULL home as well as a function or argument home.

After the shared read has selected a non-NULL borrowed result, acquire its
canonical owner while source homes are intact. Use scalar `PyValue` snapshots
for function and argument, with canonical `PyValue_RefcountOnObject` semantics.
If both releases own the same object, require its post-acquisition refcount to
exceed two. Otherwise require each count-owning object's post-acquisition
refcount to exceed one. Inline, immortal and deferred-count representations
retain the existing canonical close semantics; no homemade immortality or
deferred-refcount inference is allowed.

This includes result/function/argument aliases automatically because the count
is observed after result acquisition. It does not count borrowed roots as
owners. A failed proof closes only the duplicate result while original roots
are intact, then declines before effects. Never add pins to pass the proof.

After the final epoch check, detach every consumed home, including NULL; retire
the two scalar owners with the existing canonical closes; set the checked
return slot/PC and publish through the existing ordinary completion primitive.
Every destination home is already NULL, so publication displaces no extra
owner. The function's strict survival proof also protects code and globals.
There is no fallback, periodic check, callback, allocation or error after commit.

## Keep the existing general proof cold first

Do not initially delete the already tested general ownership scope or weaken
its positive hit/miss assertions. Move its arrays and exact aggregate-release
proof into a separate non-inline retirement helper, reached only for a shape
outside the unary/TEMP case. It receives the already acquired scalar result
owner and untouched caller homes. Ownership transfer is explicit: on MISS it
closes that duplicate itself; on DONE it publishes or retires it exactly once.
Its old LOCAL/discard/multiple-alias accounting stays unchanged. A unary proof
failure returns MISS; it does not retry the general proof.
The cold helper owns the acquired result exclusively on both MISS and DONE,
and performs its own final epoch check after its longer preflight. The hot
caller must not close or publish that transferred result again.

Share entry admission, expression selection, result acquisition and ordinary
publication semantics. Separate only retirement proof/storage. Thus the common
case has no owner arrays or scratch-owner record; there is no second expression
evaluator or second call-admission model. Certificate validation belongs only
in the selected leaf sibling; the ordinary exact handler has no leaf gate.

If this split requires another general transaction record, duplicate effect
analysis or extra hot argument setup, stop and propose an explicit unary-only
scope instead. Such narrowing requires root approval and honest test changes;
it must not silently remove existing positive coverage. General behavior still
uses ordinary CALL fallback. Avoid retaining unused machinery merely to claim
broader support.

## Qualification and stop conditions

Retain whole-body/metadata/provider/descriptor/monitoring negatives. Add direct
cached-feedback index/bounds witnesses, including same-code distinct-globals
and current-specialization changes. Generator negatives must reject a hook
before any checked prefix fact, wrong callable/window inputs, intervening
effects and use in the bound-wrapper retirement path.

Exercise both unary layouts, every overlapping destination position, inline and
mortal arguments, function==argument, result aliases, and strict-refcount boundary
misses with observable finalizers. Free-threaded/deferred-refcount qualification
checks conservative decline, not a new hot admission outside the GIL scope.
Existing old-LOCAL, discard and multiple-argument positives must still reach the
cold general path and retain their counters and observations. No malformed IR
body is executed.

Root owns native/API qualification and matched measurements. Required structural
evidence is no hot `_PyIR_InstrFeedback`, no generic owner-array frame on unary
hits, and no duplicated ordinary guard/read work. Actual admission and miss
counters must accompany timing. If those changes do not recover the measured
regression, leave C unpromoted; do not expand syntax or build a specialization
catalogue to rescue it.

Peer input: call_binding_lane reviewed the scalar multiplicities, both window
layouts, destination/old-LOCAL/discard exclusions, checked-phase boundary and
continued dynamic admission and selected sibling handler. Proposal direction is
approved for a future round; implementation and speedups are unproved. This
document incorporates the peer's guard/lifecycle/format constraints;
implementation still needs independent actual-code review.
