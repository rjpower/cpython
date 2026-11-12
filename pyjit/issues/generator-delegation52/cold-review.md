# Independent cold review of delegation52

2026-11-12. Reviewed candidate `e85171923c08b0d68ee169119e761d7583bbcb26`
in `BASE/worktrees/delegation52-entry`, relative to selected golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. BASE is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Canonical documentation is the python-jit checkout. This reviewer made no
runtime edits and ran no builds, tests, workloads or native programs. Review
covered the actual runtime/producer/generator/constructor/observer diff,
generated suspension body, focused test source and existing evidence. Applicable
AGENTS guidance, the [CPython AI policy](https://devguide.python.org/getting-started/ai-tools/),
canonical project entry documents, and IR/interpreter/frame/observer/delegation
contracts were read.

## Decision

Reject both candidates for selection; retain golden51. The final results below
supersede the conditional admission language in the earlier review updates.
No new concrete runtime correctness blocker was found in this source review.
The actual implementation has a coherent bounded ownership model and implements
yield-side propagation. It does not implement cheap whole-chain resume or
leaf-only execution. Architectural clarity and demonstrated forwarding do not
justify accepting the confirmed elapsed losses.

The initial `58f9ea6cfee` fails performance admission: its six paired ratios
against golden51 are 1.043694 for generators, 1.046551 for coroutines and
1.026699 for DeltaBlue; all six pairs lose in each case. Logging's 0.926142
geometric mean includes a losing pair. These are the retained
`confirm52-hot-golden/summary.json` results, not measurements of `e851719`.
The initial candidate's all29 upstream mean is 1.167121. The coordinator will
provide final followup timing and qualification; no result is inferred here.

## Correctness, ownership and observation

| Boundary | Assessment of actual candidate |
| --- | --- |
| Frame authority | `_CHECK_GENERATOR_OWNER` precedes DELEGATE activation and SUSPEND input consumption. Code flags do not substitute for actual generator ownership. This is necessary for malformed public construction and exceptional entry. |
| Terminal owner | `IREmitter.suspend` detaches the outgoing home once, retains one canonical C owner through the iterative loop, and publishes through `_PyFrame_SetReturnValue` once on the terminal miss. A forwarding hit does not reread or populate the intermediate B+2. Actual frame, exception, recursion, previous-link and suspended-state transitions remain present. |
| Shortcut authority | `can_propagate_delegate_yield` is pure and rejects pending redirects, wrong continuation/recipient, non-NULL B+2, wrong executing operation/image, stale epoch, monitoring data, frame objects, tracing/profile/PEP523 and enabled native hooks. Free-threaded builds reject fusion. Rechecking each restored parent avoids inheriting leaf authority across a callback. The conservative frame-object and monitoring-data exclusions affect opportunity, not semantics. |
| Miss and callbacks | LLTrace runs with the restored parent still in its pending-child phase; the held owner has not become a fictitious SUSPEND input. On a miss the ordinary recipient transaction handles redirects and displaced/discarded owners. Only then does cold landing synchronize instrumentation with a real input home and an actual SUSPEND failure recipe. |
| Resume entry followup | The no-hook release arm recognizes an actual outer LINE/INSTRUCTION wrapper before inspecting generation metadata. No escaping operation intervenes on the ordinary arm. The wrapper arm retains the existing preparation/error/redirect protocol. Debug and all WITH_DTRACE configurations retain the prior complete sequence. This factoring is sound by source inspection; compiled placement and timing remain separate evidence. |
| Resume continuation | Dispatched RESUME_DELEGATE keeps the real site and site+1 error cursor through fallible work. Successful callback redirects, including generation-only intent, win over CD. Skipping retains the breaker/version, TLBC and Emscripten guards. Native/exact entry share preparation; fresh calls retain their own path. |
| Provider versus injected errors | Constructor/producer checks preserve DELEGATE outside the synthetic throw region and SUSPEND inside its CLEANUP_THROW recipe. Saving the real following RESUME_DELEGATE preserves previous-word attribution on injected entry. Completion uses the existing owning helper with a literal DELEGATE kind. Close recognizes the new resumes; delegate discovery retains return_slot-2 geometry. |
| Far exits and consumers | The exact triple/adapter normalizer separates semantic branch/source destination from physical adapter dispatch. The triple is not grouped into one observation unit. Handler storage, tables, disassembly, IR targets, statistics and remote await recognition were ported. Widened observation storage is a real format consequence; no observed-path speed claim follows. |

The constructor verifies triple geometry, resume flags/target, completion
recipient, throw context and normal/exception/source ingress. Runtime ownership
guards remain necessary because local encoding validity does not prove actual
activation ownership. No new cached chain, outcome allocation, borrowed
long-lived delegate, alternate evaluator or untraversed frame-owner ledger was
found.

The retained GDB mechanism evidence in `runtime-post-review.md` establishes two
successful parent transitions in one suspension invocation with the same mortal
owner at refcount one, empty intermediate transfer homes and real detached
suspended frames. It supports the mechanism, not general semantic coverage or
throughput. Existing focused evidence for the followup identifies `e851719`,
clean source and return code zero for eight release modules. Earlier release,
debug, stats and value-API evidence belongs to its recorded earlier identities;
it must not silently become fresh qualification of this followup.

## Should fix: state the compiler trust boundary precisely

Original finding; the documentation claim is resolved in the final update below.

`canonical_frame_transfer_input` matches the actual DSL guard/TAKE/terminal
producer body, and `delegate_yield_guard_proven` matches the actual C predicate,
including preprocessor arms. Their mutation negatives are useful and fail
closed. The backend `IREmitter.suspend` remains trusted compiler implementation;
no source/AST admission gate validates that emitter's implementation. This is a
normal compiler trust boundary, not itself a defect. The stronger documented
claim that terminal emitted-state mutations are bound is only partially
supported. [Astra finding 7](astra-review.md) requires porting the proof to
"the terminal primitive plus its bounded emitter" with mutations for NULLing,
PC/recipient, callbacks and guards. The [design](../../design/generator-delegation.md)
states that the proof binds "the emitted producer and transfer bodies" and
the "actual emitted AST/token body"; [resolution row 7](review-resolution.md)
repeats that obligation. `test_delegate_terminal_generation_and_metadata`
checks initial NULLing, LLTrace order, one publication, generation guards and
cold-landing order. It does not independently reject deletion/change of the
emitted resume-PC assignment, incoming recipient, exception-link detach,
previous-link detach or release-store of suspended state inside `suspend()`.

This is a proof-coverage/documentation gap, not a demonstrated runtime defect:
the reviewed emitted writes are present. Before describing the full fusion
terminal as mutation-checked, add meaningful bounded final-state contract
checks and mutation negatives, or state explicitly that the generator terminal
is trusted and manually reviewed while its producer/predicate are mechanically
bound. A second textual copy of every emitter line would offer little assurance;
the useful boundary is the physical owner/continuation state and absence of an
escaping operation before publication. Do not build a general third-edge
framework for this fix.

## Why dispatch removal can lose

The saved work is intermediate B+2 publication/reload and parent SUSPEND
dispatch. Every level still pays real input transfer, exact-child admission,
executing-state exclusion, frame/exception/current-frame linking, recursion
entry/exit and suspended-state publication. Resume still descends through the
actual parents. Coroutine Fibonacci commonly completes without yielding, so
this increment adds machinery to a path with no forwarding benefit.

New work includes owner checks, the continuation/recipient/NULL/image/epoch/
observer predicate at each parent, terminal-miss generation/landing checks,
broader resume-kind handling, and entry wrapper recognition. The followup removes
the demonstrated premature owner/frame-object/generation preparation from the
ordinary no-hook release entry; it does not remove those other costs. Converted
virtual/asend sites also lose their previous dedicated SEND specializations.
Plain yields now share the new terminal even when their caller cannot forward.

More/larger handlers and changed control flow can alter register allocation,
branch placement and instruction-cache behavior across unrelated handlers.
They are plausible explanations to test, not an attribution of DeltaBlue's loss.
The initial generator PMU ratios (1.622455 instructions and about 1.5094 user
cycles versus upstream) establish substantial whole-workload work. Golden51's
earlier independent PMU campaign is useful context, but subtracting those ratios
does not produce a matched causal cost estimate. Sample percentages and symbol
sizes likewise provide no removable budget or cycle proof.

## Ranked followup options

1. **Keep the selected runtime and attribute transition costs before another
   implementation.** If the bounded followup still loses, preserve this design
   and evidence as an experiment. Obtain matched phase/hit/miss/chain-depth
   evidence for creation, resume, suspension and completion on the unchanged
   kernels. This distinguishes saved home traffic from per-frame work and
   avoids optimizing a sample location on assumption.
2. **Reduce proven failed-check and cold-landing overhead.** Split cheap actual
   continuation/owner discrimination from expensive eligibility/landing work,
   or isolate a cold miss tail, only after native and weighted evidence identify
   it. Keep observation, epoch, redirect and input-ownership authority unchanged.
   This offers bounded scope and covers shallow/plain/FOR_ITER consumers.
3. **Restore provider-specific DELEGATE specialization where measured.** Reuse
   virtual/asend provider admission with the same semantic completion and real
   materialized yield route. Require misses before effects and no provider
   replay. This addresses an explicit lost optimization; it cannot explain or
   repair an exact-generator regression by itself.
4. **Design iterative resume-chain entry without a persistent cache.** Walk real
   exact children through one entry loop while retaining each frame's input,
   executing-state, recursion and observable-link transaction. Stop at hooks,
   providers, redirects and failures. This can address the currently untouched
   descent side, but needs separate architectural review and measured weighting.
5. **Consider lazy delegation-frame materialization or native JIT lowering only
   as a later architectural increment.** Larger savings require reducing actual
   per-level state work, with explicit materialization, logical recursion,
   introspection/GC and callback-failure contracts. Explicit continuations help
   express that work; they do not establish those proofs today.

No item authorizes further runtime changes in this review. A final admission
decision requires repeated matched improvement from the exact candidate,
broad-29 and regression guards, focused release/debug qualification and coherent
full diagnostics with established debugger/LLTrace failures retained explicitly.

## Entry-followup screen and current recommendation

The coordinator reports that `e851719` release builds/imports 117 modules and
passes 462 focused tests. Compiled inspection confirms that actual wrapper
recognition precedes generation metadata work and that the latter is cold.
The two-pair direct golden51 screen in
`E/entry52-screen-hot-golden/summary.json` records the following elapsed
candidate/control ratios, where E is
`BASE/pyjit-link-wave1/generator-delegation52`:

| Kernel | Geometric mean |
| --- | ---: |
| Generators | 1.015324 |
| Coroutines | 1.115300 |
| DeltaBlue | 1.122527 |
| Logging silent | 0.917079 |
| Comprehensions | 0.982248 |

Both pairs lose in each of generators, coroutines and DeltaBlue. The screen
therefore supplies no basis for promotion. Reject this candidate for selection
unless the running six-pair confirmation materially changes that conclusion
and the remaining admission gates pass. Keep golden51 and retain this round's
design, source and evidence; no further implementation is queued this round.

The successful compiled placement check establishes that the bounded entry
factoring was implemented. It does not establish that entry preparation caused
the initial loss, that the followup caused the larger coroutine/DeltaBlue screen
losses, or that the remaining predicate is their cause. These short campaigns
cannot isolate those explanations. Compare the exact candidate with golden51
using the confirmation campaign; do not subtract separate campaign ratios to
estimate the cost removed by the followup.

The coordinator will correct the canonical design and review-resolution wording
to reflect the implemented trust boundary: producer-body and C-predicate gates
are mechanically bound, while the backend terminal emitter is trusted compiler
implementation supported by source/generated/native review and selected contract
checks. That wording resolves the unsupported general mutation-binding claim;
it does not turn the emitter into a separately validated proof or imply that
every terminal-state mutation is rejected. No runtime repair or line-by-line
mirror tests follow from this documentation correction.

## Final admission: reject both candidates

The final [benchmark tables](benchmark-tables.md) and their retained analysis
inputs confirm that the entry followup fails the target and breadth gates.
Its six-pair direct golden51 ratios are:

| Kernel | Geometric mean | Losing pairs |
| --- | ---: | ---: |
| Generators | 1.009530 | 6/6 |
| Coroutines | 1.113011 | 6/6 |
| DeltaBlue | 1.118550 | 6/6 |
| Logging silent | 1.007040 | 4/6 |
| Comprehensions | 0.979620 | 1/6 |

The followup's two-pair all29 geometric mean is 1.020473 versus golden51, with
only 8 of 29 cases faster by their paired geometric means. Its separate direct
upstream mean is 1.172901. The comprehensions improvement does not compensate
for repeated target losses and broader regression. The first candidate already
failed its six-pair target gates. Neither candidate qualifies for promotion;
golden51 remains the selected runtime.

Qualification limits remain explicit. The followup has a release build with
117 imported modules and 462 focused tests passing. Its debug/stats trees were
configured only; there is no followup debug/stats compilation or full-suite
claim. The first candidate has focused release/debug/stats evidence and a
306-unit value-API pass, but no full-suite pass. Stopping further qualification
after the performance failure is appropriate; incomplete gates cannot become
qualification by inheritance from an earlier identity.

The canonical design and resolution now accurately state that Astra finding 7
is only partially met: mechanical gates bind the producer and eligibility
predicate, while the terminal emitter remains trusted compiler implementation.
This resolves the overstated proof claim identified above. Bounded post-emission
contract verification remains future work. No demonstrated semantic failure is
being asserted, and no new runtime work is required to make this rejection
decision.

Preserve the ranked followup options as planning input and retain the failed
experiment's source and evidence. No further implementation is queued this
round. Final timing establishes rejection, not causal attribution: do not infer
which retained transition, changed branch layout or predicate caused the losses
by comparing separate campaigns or subtracting their ratios.
