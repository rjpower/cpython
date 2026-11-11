# C50 performance and source-metadata review

Review of `3e96bd9311e` follows completed component source/native/focused runtime
review. This file distinguishes measured results from hypotheses; no runtime
edits, builds or benchmarks were run by the reviewer.

## Performance evidence so far

Exclude `measure50c-call-pair48`: it overlapped debug generator testing. The
clean `measure50c-clean-call-pair48` campaign has stable before/after identities
and no failure or postflight error. Six balanced elapsed ratios against frozen48
are:

| Workload | C with A / frozen48 mean | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.964298 | 0.933622–0.976496 |
| Logging silent | 0.949135 | 0.914355–0.973452 |
| nbody | 1.011550 | 1.001531–1.024105 |

Every DeltaBlue/logging pair improves and every nbody pair regresses in this
campaign. This establishes only the combined candidate against frozen48. A50's
earlier two-pair campaign is separate and cannot isolate C's contribution.
Direct matched C/A measurement remains necessary before selecting the broader
refinement or claiming it improves A.

The native mechanism is real: C skips a dispatch and function-home publication,
reload and Python-function kind comparison, while preserving real activation.
It also adds live observer/epoch/neighbor admission, retains a generic
generator/null-self branch that the compiler did not eliminate, and retains
the value API's checked object conversions. These are plausible costs, not
sampled attribution or predicted speedups.

If a further bounded C experiment is justified, the existing
`_PyFrame_PushOverlapping` already implements ordinary thread-owned activation.
C can consume its proved thread/nonnull-self/NULL-function-window facts and
call that same implementation directly with one argument home; ordinary
PushExact continues to delegate there. This avoids a new helper, flag API or
duplicate frame initialization/retirement. Measure C/A before choosing this
experiment; its benefit is unproved.

## Source/debug boundary

Cold source inspection confirms no line/column/source-map lookup or source
equality gate in ordinary dispatch, method nomination or C admission. C retains
both original executable PCs and source mappings. Execution PC, return PC,
control-flow targets and exception regions are semantic state and must remain
valid without source coordinates.

Two existing couplings remain outside this C increment. Compiler passes still
use source-line predicates when selecting some rewrites, including compare/jump
fusion. Separately, co_linetable's IRSM records pack display coordinates beside
observer recovery fields (handler, exc_slot, lasti).
`get_frame_exception_handler` reads those fields when a source-only observer
callback raises; ordinary unobserved unwind reads co_exceptiontable.

The clean direction is an optional attribution/observation overlay mapped to
stable semantic instruction/event identities. Core execution and rewrite safety
use value, effect, control-flow and ownership proofs. Line-to-PC translation
for f_lineno belongs to the observer layer; the resulting redirect must still
pass semantic live-state checks. Stripping coordinates must not delete needed
observer recovery or silently change ordinary control/ownership behavior.

Removing compiler line gates requires a concrete late-observer/reconstruction
contract. Blanket removal or renaming mixed source/recovery records does not
establish that contract. Worker audit/proposal and independent review are pending.

## Direct comparison and bounded follow-up approval

Reviewed `measure50c-canonical50a`: six balanced C/A pairs yield DeltaBlue
1.043648 and nbody 1.038921 (every pair slower), with logging 0.968599 (every
pair faster). Current C is not admitted for the DeltaBlue objective. This direct
comparison supersedes any attempted inference from separate C/48 and A/48 runs.

Reviewed `debug-metadata-audit50.md` before the follow-up implementation.
**Approve one bounded direct-overlap experiment.** After captured admission,
store the owned self at call_window[1], assert thread ownership, nonnull self
and NULL call_window[0], and call the existing PushOverlapping implementation
with argument base call_window+1 and nargs one. It consumes the C-owned function
into f_funcobj and the physical self home into the child argument; its existing
Initialize acquires independent code ownership and performs the same stack/header
commit. A temporary input was already NULLed by TAKE; a local retained its own
owner and supplied a duplicate. The parent callable home remains NULL exactly
as after ordinary PushExact. Fallback still publishes both owners and resumes
CALL; continuation and shared entry/return are unchanged. No new flags, cache,
header implementation or body grammar is approved. Retain current mutable and
observer checks in this experiment, including PEP523.

Approve the proposed caller/callee `code.replace(co_linetable=b'')` test using
the existing constructor normalization path. Require unchanged co_code,
co_fbinit and co_exceptiontable, all source positions unavailable, successful
refinement installation and correct execution. This tests C admission/execution
independence from coordinates. It does not prove source-independent compilation:
the executable IR was originally lowered with source metadata. Use the simple
fixture without eliminated observer-only spans or exception regions.

The audit's semantic/source classification and bounded future recovery proposal
pass review. Copying a resolved exception recipe into activation-local pending
observer-error state is a credible separate increment; it requires its own
reentry/redirect/nested-entry tests before implementation. Broad lowering changes
remain deferred. If the single direct-overlap experiment fails direct C/A target
admission, select A and retain rejected C evidence; no further C iterations.

## Direct-overlap source post-review

The actual follow-up delta is confined to the generated terminal success path
and two test files. It stores self, asserts thread ownership/nonnull self/NULL
callable home, then calls existing PushOverlapping with one argument. Current
admission and captured fallback are unchanged, and ordinary exact CALL still
uses PushExact. The code matches the approved owner/code/frame contract and
passes source review. Native and executed checks remain coordinator gates.

The no-positions fixture uses the supported empty-linetable replacement path,
checks executable code, feedback initialization and exception bytes are
unchanged, checks all positions unavailable, and requires actual refined
installation plus the correct value. Its scope matches the approved claim.

## Cold recovery boundary proposal review

Reviewed `source-error-recipe50-proposal.md` against RecordSourceError,
InvalidateSourceErrors, source_line_event and get_frame_exception_handler.
**Approve the bounded proposal before implementation, subject to the
coordinator's source unfreeze.** Move the existing four-int handler type to the
shared internal header, copy the already decoded event's semantic recipe into
the current activation on its existing nonredirected failure path, and have
the matching-frame unwinder consume that recipe without a source-map read.
The event ordinal may remain solely as attribution identity.

Copy out the recipe and clear validity before any subsequent unwind/cleanup;
clear the ordinal and stored recipe too. Activation initialization and chain
invalidation must clear the complete pending state consistently. The borrowed
frame validity pointer acquires no new owner. Four scalar recipe fields require
no allocation, callback or GC root. Preserve the existing pre-callback redirect
generation check, including same-PC redirects and frame-materialization failure.
Nested C evaluation keeps a separate activation and must not consume its
parent's pending recipe. Ordinary exceptiontable lookup remains unchanged.

The proposed caught/uncaught monitoring failures, same-frame later ordinary
raise, repeated observer enable/disable, attribution and existing nested cleanup
coverage address the critical state transitions. Include handled=false recovery
explicitly. No serialized-map, optimizer, frame-layout or hot dispatch change is
approved. This removes a cold unwinder dependency on an attribution container;
it predicts no DeltaBlue speedup and does not by itself remove compile-time
source-line restrictions.

The staged five-file recovery patch passes actual-source review. It moves the
existing type unchanged, copies the decoded event on the existing error branch,
consumes the recipe only for the matching current activation/frame, clears all
pending state, preserves chain invalidation and removes the source-map include
from ceval.h. The two tests cover consumed recovery followed by an ordinary
error, cleanup/attribution and handled=false behavior across monitoring cycles.
Actual application, release/debug compilation and focused execution remain
coordinator gates. Publishing the validity frame last is a clean optional store
ordering; the present recording function has no intervening callback or allocation.

## Direct-overlap native review and fixture correction

Reviewed the coordinator's `native50c4` disassembly at `d952dc8ea5d`.
The refined handler is 1052 bytes, down from 1396. Its success path no longer
rechecks the parent frame owner, branches into generator setup, or computes a
dynamic self/argument base. It directly initializes the existing overlapping
thread frame with one argument. The admission preflight still checks thread
ownership. Function and independent code ownership, parent return slot and
CALL+1 continuation, recursion accounting and shared callee entry remain intact.
Ordinary exact CALL remains 1410 bytes; the base method remains 249 bytes.
An independent relocation-normalized disassembly comparison of ordinary exact
CALL against original C is identical: 339 instructions in both versions.
This establishes the intended native simplification, not a performance win.
Direct C4/A timing remains the admission gate.

The initial empty-position fixture failed because assigning an existing
function's `__code__` permanently clears its specialization version. Approve
the test-only correction: recursively strip the factory's nested code objects
before executing MAKE_FUNCTION, preserving executable code, feedback and
exception bytes and requiring unavailable positions throughout. Actual source
inspection confirms fresh caller/callee functions and the unchanged assertion
that the refined handler installs. The worker reports the corrected release
unit passes; broader execution remains coordinator-owned.

The earlier permission to retain an activation event ordinal is superseded.
After recovery uses the copied recipe, that ordinal has no reader: attribution
already uses the frame object's existing event fields. Remove the activation
ordinal and API argument, retaining only the semantic recipe and frame validity
pointer. This reduces pending state without changing attribution. The revised
staged patch must be inspected before final application approval.

The revised staged patch has now been inspected and passes source review.
The activation ordinal and API argument are removed consistently, recipe storage
precedes frame validity publication, and matching unwind clears both before
cleanup. Existing frame attribution fields are untouched. No source blocker
remains; coordinator application, release/debug builds and focused execution
are still required.

## Final C admission decision

Reviewed the stable-identity six balanced pairs in
`measure50c5-canonical50a`. Final C/A time ratios are DeltaBlue 1.028054
(five of six slower), logging 1.140367 (all six slower), and nbody 1.008522
(five of six slower). **Reject C for this batch.** The bounded native
simplification succeeded structurally but did not produce the required runtime
gain. No further C iteration is admitted. Retain its evidence separately and
select A plus the independently reviewed cold recovery change, subject to final
merged correctness and performance qualification.

Carry the general no-coordinate regression onto A using fresh MAKE_FUNCTION
targets from stripped code. Assert ordinary canonical method specialization,
exact CALL specialization and the result, retaining unchanged executable,
feedback and exception bytes and unavailable coordinates. The C-only refinement
assertion retires with C. This preserves useful source-independence coverage
without retaining the rejected optimization.

Reviewed the actual selected runtime composition on
`pyjit/deltablue-checkpoint50-20261111` at A `e014965013b`: exactly the reviewed
five-file cold recipe delta is applied, with 108 insertions and 23 deletions.
Recipe/frame publication, consumption, invalidation and the observation caller
match the final staged contract. No C refinement, activation-helper or generator
change remains in that selected delta. Source review approves commit and merged
validation; native artifacts must be rebuilt before they can describe this
candidate. The general no-coordinate test carryover remains separately pending.

The carryover is now inspected and source-approved in
`TestCanonicalMethodCapture.test_method_call_without_source_positions`.
It preserves code/feedback/exception bytes recursively, creates fresh functions
through MAKE_FUNCTION, requires unavailable positions, warms ordinary canonical
method and exact CALL specialization, and checks the result. It contains no
C-specific expectation. The selected six-file source composition has no open
review blocker; coordinator builds and execution remain required.
