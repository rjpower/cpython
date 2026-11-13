# Register state and frame layout: results

This round used one broad [Astra review](astra-review.md), independent Sol
implementation and reviews, isolated release/debug builds, native inspection
and balanced timing. All five ideas were implemented and tested. Two revisions
and two compositions tested whether the initial costs could be removed. The
performance candidates are not admitted. A separate, bounded public-constructor
correction is selected as checkpoint54 at
`61015a9452d9069646fb5fede9ff963896833b89`, with explicit qualification limits
and no performance lift. Branch `pyjit/round54-golden` and annotated tag
`pyjit-golden-20261112-frame-state` are published at that commit. Round54 is
closed. The [tables](benchmark-tables.md), [qualification](qualification.md) and
[handoff](../../HANDOFF.md) own exact identities, commands and limitations.

## Five experiments

| Change | Work removed and work added | Measured decision |
| --- | --- | --- |
| R: encode empty versus replace recipient | Avoid old temporary-home read/close at publication; classify and encode the destination at every producer | All29 mean 0.989479 of golden51, 22/29 lower, but six-pair coroutines 1.068741. The R2 default-state revision does not resolve the loss. Reject. |
| F: derive feedback from execution prefix | Header 112→104 bytes; replace cached pointer with a dependent load on feedback access | All29 0.993748; six-pair coroutines 1.040777. Reject. |
| N: derive globals/builtins from function | Header 112→96 bytes; decode the pinned function and load its namespace on use | Hot confirmation mixed; coroutines 1.050058. Reject. |
| L: move locals mapping to optional frame object | Header 112→104 bytes; eager frame object for mapping-bearing entries, indirect access and more retirement work | Six-pair precompiled exec 1.222247; class construction 1.042134. Reject. |
| S: certify an integer same-home overwrite | Remove a real intermediate NULL store and generic destination work; add specialization and handler text | Revised S2 moves four geometry checks from execution to common constructor/specializer validation. All29 0.992128, hot coroutines 1.024050. No convincing net win on the reached target workload. Reject. |

Ratios are elapsed candidate/golden51; lower is better. All29 screens use two
balanced pairs per workload; hot confirmations use six. Small differences are
not automatically established improvements. The two-pair R screen improves
most workloads, but its persistent coroutine loss is material to this round's
call/activation objective. Rejection is a judgment about the measured tradeoff,
not a rule that every workload must improve.

The F+N+L composition really has an 80-byte header, matching upstream's measured
size. It passes focused checks but fails its initial hot screen, including
logging 1.100487. Removing cache words does not remove the work needed to
recover their information. R2+S2 also passes focused checks and native review;
its six-pair confirmation gives generators 0.956948 and coroutines 1.098464.
Composition does not rescue those contracts. No component ratios are multiplied
to predict combined performance.

## Register validity and frame storage

A full bitmap was researched and not implemented in round54. This does not
reject bitmap architecture generally; [round55](../call-model55/plan.md)
reconsiders it with complete call/frame lifecycle costs. Warm thread
storage already has an initialized frontier; we do not zero the full arena on
every call. NULL homes simultaneously provide safely readable canonical bits
and an empty-owner representation. A bitmap would need updates and checked
reads across moves, overlapped arguments, local writes, callback mutation,
traversal, unwind, suspension and escaped frames. Counting only a saved teardown
scan would omit most of its cost. See the [register-state inventory](validity-inventory.md).

Three facts must remain separate: initialized bits, ownership of an RC count,
and future-read liveness. V8 safepoint maps answer where roots are at admitted
stops; they do not supply Python RC transfer and reentrant-local state between
stops. A LuaJIT-like compact header is useful only if reconstructing omitted
state costs less across the whole execution. This round demonstrates that
matching upstream's header width alone does not provide that result.

Keep code and function as independent owners because function.__code__ can
change. Retain the current stack checkpoints and executable continuation
authority. Optional observation storage is already represented by frame_obj;
forcing mapping-bearing frames through it adds actual variable-tail allocation.
Feedback and namespace caches are used during execution, so they are not
merely optional debugging fields. The [field inventory](field-inventory.md)
and [header accounting](header-costs.md) trace each field.

S2 provides a sound design lesson despite its rejection: validate immutable
geometry once, then let a refinement rely on that proof. Runtime type guards,
fallible boxing and owner commit remain distinct. Its handler is 296 bytes
versus the generic handler's 642, and actual coroutine execution reaches the
refined route 121,392 times per run. This establishes real removed work, not
elapsed savings. Comprehensions and generators have zero reached sites in that
census, so their timing changes cannot be attributed to the omitted store.

## Why upstream remains faster

Universal PyValues and the register interpreter are integrated. Exact calls
already overlap argument homes in one frame stack, reserve/init once, and
publish ordinary returns inline. Generator resume already dispatches inside
the same evaluator. Repeating those proposed fixes would describe work already
done.

The final selected54/upstream all29 comparison has geometric mean 1.156543,
with 6/29 faster. Separate six-pair confirmation comes from the completed
`final54-hot6-confirm2-upstream` campaign, not the initial hot campaign that
failed its load guard:

| Workload | Selected54 / upstream elapsed | Pair range |
| --- | ---: | ---: |
| Generators | 1.450207 | 1.424808–1.472757 |
| Coroutines | 1.254281 | 1.240417–1.265182 |
| DeltaBlue | 1.154663 | 1.134484–1.184544 |
| Richards | 1.280930 | 1.229266–1.407654 |
| Comprehensions | 1.390876 | 1.347206–1.511459 |
| Logging silent | 0.712216 | 0.671358–0.779797 |

Ranges are wide for Richards, comprehensions and logging. Keep all29 and hot6
separate. Selected54 versus golden51 is 1.006381 over all29 (11/29 lower),
while hot6 is 0.996669. Logging silent is 1.138347 in all29 versus 0.978879
in hot6, with a wide 0.876076–1.005017 hot range. Neither general performance
improvement nor elapsed equivalence follows from these samples.

Final PMU instruction ratios versus upstream are 1.353135 for DeltaBlue,
1.413895 for generators and 1.386260 for coroutines; cycles are 1.210496,
1.434569 and 1.249114. Each stat group completes 18/18 valid invocations;
record completes 6/6 with stable identities and no lost records. Candidate
flat samples put generator SEND/YIELD at 26.96%/11.78%; coroutine call,
RETURN_GENERATOR, ClearLocals and completion at 10.43%/7.70%/5.84%/6.00%;
DeltaBlue call/clear at 12.78%/5.42%. These overlapping execution mechanisms
and flat symbols are not an exclusive removable cycle budget.

Known extra mechanisms include canonical-value classification, separate
function/code authority, recipient/continuation handling, per-operation owner
transactions and broader physical-home movement at generator construction.
Upstream uses its own value, stack and specialization contracts. The examined
coroutine workload mostly creates/completes recursive coroutine objects rather
than repeatedly suspending, while the generator workload performs about
1.57 million resume/yield transitions per run. A single generic resume fix
cannot explain both workloads.

This round rules out several attractive small changes as sufficient solutions.
It does not establish an exact causal partition of the remaining gap or a cache,
branch or placement explanation for every elapsed change. The next design
researches calls, frames, occupancy/bitmaps, handoff and observation together.
Neither another field change nor native JIT is a predetermined answer.

## Qualification and next decision

Every initial lane and composition has matching release/debug focused evidence;
the combined candidate passes 1,152 focused tests per role and the strict value
API audit of 306 units. Failed build/debug attempts remain in the evidence
directory and are distinguished from corrected passes. No rejected performance
candidate receives an unnecessary full-suite run or golden label.

The selected cold-only correction passes 808 focused tests per role and the
306-unit value API AST gate. Its own allocation probe covers 48 points × three
subtypes × two roles with code/mapping owner checks. Both full diagnostic runs
complete 54,692 tests but fail: release retains nine old debugger errors and
GC-verifier instability; debug retains nine old debugger errors, old LLTrace
failure and one unexplained profiling failure. GC-verifier repeats reproduce
4/6 failures on both control and candidate. Ten profiling comparisons pass,
but the original full-run failure is neither proven preexisting nor fixed.
No quarantine was added. See [qualification](qualification.md) and the
[admission review](checkpoint-admission-review.md).

The coordinator selects the bounded correctness checkpoint with those limits,
not a performance variant or clean full suite. All nine rejected branches are
published; 27 directories were retired after verifying 18 archives/3,042 files.
Selected builds and immutable controls remain. The
[round55 plan](../call-model55/plan.md) owns the completed architecture research
and reviewed migration sequence. The
[proposed design](../../design/proposals/register-call-model55.md) has passed
independent post-review; implementation has not started. As-built contracts
remain authoritative.
