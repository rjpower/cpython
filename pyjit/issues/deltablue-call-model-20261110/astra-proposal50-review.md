# Astra grouped review: DeltaBlue proposal 50

Reviewed selected48 `5559f60093e`, proposal50, cache-path-audit50 and
static-path-audit50, plus selected source for generated method/CALL handlers,
canonical-output generation, feedback, frame initialization/retirement and
assembly fusion. Read canonical guidance and the CPython AI policy. This is the
requested broad review before implementation; it is not a code approval or a
performance result.

## Decision

Approve the direction, with **A and C as the main architectural work**. Make
canonical representation/ownership facts explicit at producer boundaries, then
let an adjacent method producer enter the ordinary exact-call implementation
without discarding those facts. Keep a real activation. B is a small independent
experiment; D is an alternative only if C cannot meet the bounded contract below.
Do not let A/B become the whole response to the broader execution question.

C is feasible as a bounded experiment now. Prefer **specialized execution of two
ordinary IR words**, selected when the producer specializes, over compiler fusion
that deletes the CALL word. Each word retains its ordinary meaning and source
position. The method entry may execute the adjacent call directly; independent
entry to that CALL still works. This is a general producer/consumer specialization
pattern, not a selector-body evaluator. It can remove one dispatch, function
publication/reload/clear traffic, and redundant representation/kind checks. It
cannot eliminate the function or receiver owner, independent code owner, frame
initialization, recursion/stack admission, or retirement.

The new profile establishes excess work: DeltaBlue executes 42.1% more native
instructions and 17.7% more cycles than upstream in this diagnostic campaign.
It does not show that any proposed lane removes that whole gap. The exact-call
and method samples justify this experiment; sample percentages are not additive
speedup predictions. The static appendix finds 43 eligible adjacent zero-argument method/CALL pairs
among 180 CALL sites, including 11 output and 5 input sites. That coverage and
the older semantic census justify investigation, not a claim of current
optimized-hit coverage.

## A: use a positive generator contract, not another instruction-name exception

Approve canonical guarded method payloads for all existing method-layout
variants. The cache payload is borrowed and valid only under the live lookup
proof. Acquiring its reference must happen before any operation that could
retire the receiver or otherwise invoke Python. A type version protects lookup
and absence of a shadowing name; inline validity or lazy-dict absence remains a
separate per-instance guard. A canonical descriptor does not by itself prove a
Python function: ordinary method descriptors include other callable types.

The generator currently has one-result recognition and special source-pattern
proofs. This increment needs a narrow reusable declaration for **multiple
canonical outputs**, with checked producers and preserved transfers. A direct
canonical guarded-home duplicate and a TAKE of a canonical input are the two
needed forms. Keep canonical provenance independent of the current
IR_CANONICAL_HOME_DUP result-retirement contract: method capture retains self
instead of retiring it. Validate the whole output flow; reject later reassignment or
unrecognized transformations rather than silently trusting an opcode name.
Do not write a general dataflow optimizer or add a Boolean exemption to every
store. Negative generator tests should demonstrate rejected false declarations.

Maintain the separate raw descriptor and guarded-value fields. Update nomination,
reset, feedback copies, historical/poly rows and diagnostic consumers together.
Acquire one function/descriptor owner; transfer a temporary receiver or duplicate
a local receiver. Canonical publication removes conversion, not ownership.
This contract should serve C directly, so C does not invent a second method
representation.

## C: bounded adjacent method-to-call contract

Start with method lookup immediately followed by positional CALL with zero
explicit arguments, an ordinary exact Python target, and a thread-owned caller.
This is a useful general class even though DeltaBlue strongly exercises it.
Decline generator callers initially through the existing ordinary path. Do not
add more arities, classmethod/super cases or callee-body grammars to rescue a
failed first measurement.

Select a distinct execution refinement at specialization time. Do not add a
per-hit candidate search or optional transaction gate to every ordinary method
or exact CALL. Keep the next CALL word live and use its own feedback/operands;
do not copy mutable function versions or destinations into an unsynchronized
second call cache. The refined handler must validate the currently selected
neighbor and the relation between the method destination and CALL window.
A neighboring opcode replacement must defeat the refinement, or invalidate it
through an explicit existing protocol. A stale scalar feedback index is not
proof that the current handler still has the same semantics.

Share the **ordinary exact activation implementation** between standalone CALL
and this entry. Prefer a generated common body or statically specialized helper
whose unchecked inputs have a stated contract. Avoid a runtime flags/Boolean
API that makes ordinary calls pay for the new path. Inspect native output: a
refactoring that introduces a new out-of-line boundary or repeats all guards
may erase the saving. Retain one implementation of stack allocation, overlap,
header initialization, recipient publication and entry behavior.

The required phases are:

1. **Before capture:** verify adjacency/window geometry, observer eligibility,
   method type/absence/layout guards and exact Python target kind. Failure
   leaves the original receiver home and executes ordinary lookup. Cached
   Python-function kind is usable only while its descriptor identity proof
   remains valid; method-descriptor status alone is insufficient.
2. **After capture:** hold an owned function and an owned self. Preserve the
   existing CALL function-version, argument-shape, PEP523, recursion and stack
   checks. If admission fails, publish the captured canonical pair in the
   ordinary call window and execute CALL at its original PC. Never repeat the
   attribute lookup after capture. This rule also covers stack growth and
   other fallback paths that can execute callbacks.
3. **Commit:** set the caller's instruction/source state to CALL, save CALL's
   destination and the PC following CALL, then use ordinary frame activation.
   Transfer function ownership to f_funcobj; retain independent code ownership.
   Place self in the overlapping argument home. A local receiver keeps its
   owner and supplies a duplicate; a temporary receiver supplies its existing
   owner and its original slot becomes NULL. Do not introduce an intermediate
   increment/decrement pair merely to reproduce a publication that is skipped.
4. **After commit:** ordinary body, return, exception and retirement machinery
   owns completion. No special fused return, ledger, delayed ownership state or
   selector-specific cleanup is needed.

The current method handler's two destinations are NULL temporary slots;
IR_STORE_TEMP asserts that contract. Reuse/prove it rather than introducing
callback-capable replacement into capture. Check aliasing when a temporary
receiver shares storage with the output/window. All ownership moves must be
ordered so a temporary input is consumed once and a surviving local is never
cleared. Captured-pair fallback must reconstruct precisely the state ordinary
CALL expects; it must not leave two owners for one reference count.

Observation is an admission condition, not an afterthought. Retain both source
positions and exception attribution: failed lookup belongs to LOAD_ATTR; call
admission/body failure belongs to CALL. Decline this execution refinement when
opcode events, tracing, monitoring, PEP523 or supported debug tracing require
intermediate observation. Use the runtime's actual instrumentation epoch and
handler invalidation protocol; do not invent an observer flag cached forever.
If an observer can become enabled between the checks and commit, either the
existing synchronization makes that interval safe or the implementation must
revalidate before skipping CALL dispatch. Free-threaded builds need the same
explicit argument or must decline this refinement initially.

Because both words survive, a control-flow edge to CALL remains meaningful.
For the first bounded implementation, decline interior control-flow or
exception/redirect entries unless ordinary incoming window state is proved.
Nomination must also prove the producer's fallthrough reaches precisely that
word and no required source-edge event is skipped. Do not reuse comparison
fusion mechanically: its removed-word/source-edge contract is different.

## B and D

B is reasonable if a four-byte displacement really removes the dependent
basicsize load without feedback growth. Require a proof covering supported
__class__ reassignment/layout mutation, not merely ordinary class attribute
mutation. Guard equality must precede any cached-offset dereference. Reject
unrepresentable offsets at nomination; preserve atomic validity/shape reads and
all feedback-copy paths. Compare generated native code and timings separately.
Do not force every attribute variant to use cached addressing just for uniformity.

Defer D. Both runtimes retain retirement helpers, and the existing frame path
already avoids a full temporary sweep. A shared cold/ordinary split may help,
but it has a wider callback/escape contract and less demonstrated differential
benefit than C. If pursued, preserve parent-current-before-close, NULL-before-close,
profile-cache updates, escaped-frame ownership, code lifetime, stack geometry
and callback-selected continuation. Inlining alone is not an architectural win.

## Larger opportunities deliberately left outside this increment

The clean long-term pattern is proof-carrying specialization: representation,
ownership and mutable guard facts survive until an operation invalidates them.
A/C establish that at one useful boundary. Repeated same-receiver guards within
callback-free callee regions are another credible opportunity, but require an
effect/invalidation model; global class versions alone cannot protect instance
shape through arbitrary operations. Do not build that optimizer in this packet.

Activation omission could remove more work than C, but rejected49 already shows
that replacing ordinary activation with generic decoding and a dynamic ledger
loses. It needs a cheaper shared execution contract first. Changing instance
layout to maintain a single global shape token also has no present justification:
current caches already index instructions directly, and integer comparisons are
cheap relative to dependent metadata loads and repeated surrounding work.

## Implementation and admission

Use two principal lanes: A's guarded canonical producers/generator contract;
C's adjacency selection plus shared exact activation. Agree that producer contract
before dependent C edits. B can be a disjoint optional lane with isolated evidence.
No concurrent worker should independently redesign the activation boundary.

Required semantic checks include method/type/MRO replacement, instance shadowing,
dict materialization, alternating receiver layouts, custom descriptors, changed
function __code__/defaults, local and temporary receivers, aliasing, mortal and
immortal values, recursion/stack fallback, exceptions and source attribution.
Exercise captured-target fallback under mutation/reentry, observer enable/disable,
frame inspection, callee code replacement and callbacks during ordinary return.
Test independent CALL entry and live neighbor re-specialization. Add structural
IR/generator tests for admission and proof rejection so passing semantics do not
merely mean the optimization never ran. Focused execution counters may establish
real hit/fallback coverage, but collect them separately from quiet timings.

First inspect generated and release-native changes. A must remove numeric
recanonicalization. C must remove actual boundary work while leaving ordinary
CALL's hot route unchanged in substance. Then use balanced component and combined
DeltaBlue/logging/nbody comparisons against frozen48, followed by six-pair target
confirmation and the full29 screen for a useful candidate. Report both speed and
regressions, with real source/build identities. Stop or simplify a lane that adds
more admission/decoding work than it removes; do not expand coverage to hide it.

Approval is for this bounded architecture with these contracts incorporated into
the proposal. Component source review and merged semantic/performance validation
remain necessary. No runtime change was made for this review.
