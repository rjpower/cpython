# Generator delegation: rejected performance experiment

Both candidates are rejected. **Golden51 remains selected at
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`**, published as
`pyjit-golden-20261111-runtime`. Neither experimental branch is golden.
The design, one broad Astra review, incorporated feedback, implementation,
focused qualification, profiling and independent cold review are complete.
No further runtime implementation is queued this round.

The [design](../../design/generator-delegation.md) records the experimental IR
and state contract. Start with the [Astra review](astra-review.md),
[resolution](review-resolution.md), [cold review](cold-review.md) and
[next plan](next-plan.md) for its architectural limits and next decision.
The [selected-runtime report](../interpreter-round51/report.md) remains the
authority for golden51's qualification and earlier measurements.

## Decision and elapsed performance

Ratios below are candidate/control elapsed time; lower is faster. All campaigns
use unchanged kernels, one GCC 15.2 configuration, CPU15, hash0 and JIT disabled.
Balanced role order and seven samples per invocation are retained in the raw
vectors. Source, executable, extension and configuration identities are checked
before and after. Campaigns are direct comparisons: do not multiply their ratios
or subtract independent campaigns to infer causal savings.

| Six-pair confirmation / golden51 | First candidate | Entry revision |
| --- | ---: | ---: |
| Generators | 1.043694 | 1.009530 |
| Coroutines | 1.046551 | 1.113011 |
| DeltaBlue | 1.026699 | 1.118550 |
| Logging silent | 0.926142 | 1.007040 |
| Comprehensions | 0.988748 | 0.979620 |

Every pair loses on generators, coroutines and DeltaBlue for both candidates.
The entry revision's logging pairs span 0.924343–1.056680 with four losses;
its earlier two-pair screen's apparent logging gain did not survive confirmation.
Comprehensions improves, but cannot justify the repeated target regressions.

| All29, two balanced pairs | Geometric mean |
| --- | ---: |
| First candidate / upstream | 1.167121 |
| Entry revision / golden51 | 1.020473 |
| Entry revision / upstream | 1.172901 |

Only eight of 29 cases improve against golden51 in the entry revision's broad
campaign. Its direct upstream ratios are 1.454185 for generators, 1.389850 for
coroutines, 1.302541 for DeltaBlue, 0.700901 for logging silent and 1.343673 for
comprehensions. The [complete tables](benchmark-tables.md) preserve every case,
pair range and confirmation result. These are failed experiments, not an update
to the selected runtime's performance. Golden51's earlier independent all29
upstream mean was 1.156535; that older campaign is context, not a newly rerun
golden/upstream comparison.

## What changed, and what remained expensive

The compiler represents delegation with real DELEGATE, SUSPEND and
RESUME_DELEGATE instructions and explicit completion/resume continuations.
It removes the routing backedge after allocation, retaining actual exception
and observation states. Source positions do not authorize the execution path.
All affected construction, disassembly, compiler, monitoring and runtime
consumers were ported. The six added handler IDs require 16-bit observation
storage; silently truncating them to the previous byte-sized representation
would have been incorrect.

The yield terminal can forward one canonical C-held owner through eligible
exact-generator parents. It avoids intermediate recipient-home publication,
reload and another parent SUSPEND dispatch. Native GDB evidence demonstrates
two successive forwarding hits with the same mortal owner at refcount one and
empty intermediate homes. This establishes the implemented mechanism, not a
throughput win or exhaustive semantic proof.

Each actual frame still pays its executing/suspended-state transition, input
transfer, recursion accounting, exception-chain work and frame/current-link
updates. Resume still descends through the actual parents. Every forwarding
level adds eligibility checks for owner, continuation, recipient, image,
generation and observation state; misses retain a coherent materialized route.
Generic virtual/asend sites lose their former provider-specific specialization.
That loss cannot explain the exact-generator benchmark's regression by itself.

The unchanged generators kernel really delegates through exact child generators.
The coroutine Fibonacci kernel completes recursive awaits without yielding:
yield-side forwarding offers it no benefit. The design therefore removed a
visible routing cost without removing enough of the underlying activation work.

Native inspection also found common resume-entry metadata preparation before
recognizing an actual instruction/line wrapper. The sole bounded
[entry revision](entry-followup.md) moves that preparation behind wrapper
recognition in no-hook release builds, preserving debug/DTrace sequencing.
Compiled placement confirms the intended work moved cold. Application timings
nevertheless reject the revision. Handler sizes grew; register allocation,
branch placement and instruction-cache effects remain hypotheses, not established
causes of the coroutine or DeltaBlue regressions.

## PMU and sampled profiles

Post-warmup counters have 24 valid intervals per candidate campaign, all running
100%. Counts are normalized by unchanged kernel work; the table is the geometric
mean of three direct paired candidate/upstream ratios.

| Kernel | First instructions | First user cycles | Entry instructions | Entry user cycles |
| --- | ---: | ---: | ---: | ---: |
| Generators | 1.622455 | 1.509404 | 1.593718 | 1.444295 |
| Coroutines | 1.414235 | 1.299289 | 1.411233 | 1.393074 |
| DeltaBlue | 1.357659 | 1.219988 | 1.356641 | 1.301178 |
| Logging silent | 0.967516 | 0.677166 | 0.963213 | 0.712772 |

The revision still executes about 59% more instructions than upstream on
generators, 41% more on coroutines and 36% more on DeltaBlue. Moving one metadata
sequence is insufficient. Different instruction and cycle movements do not
identify a hardware cause or isolate the cost of that sequence.

The first candidate's four sampled profiles have zero lost records. Generator
flat samples place 29.88% in DELEGATE_GEN and 16.63% in SUSPEND, including 11.41%
overall in the inlined eligibility predicate. Coroutine samples instead show
CALL_PY_EXACT_ARGS (10.30%), RETURN_GENERATOR (7.47%), gen_dealloc (7.37%),
CompleteGeneratorReturn (7.20%), DELEGATE_GEN (5.91%), GET_AWAITABLE (5.59%) and
FrameClearLocals (4.98%). These locations guide phase attribution; sample shares
are not removable time budgets. There is no sampled-profile claim for the
entry revision, which has counter evidence and native inspection instead.

## Qualification and review limits

First candidate `58f9ea6cfee47536893ec59780a0f719b4c856ef` has separate
release/debug/stats builds, each importing 117 modules. Focused release coverage
runs 1,035 tests with ten skips at its integration predecessor. Debug completes
928 tests with nine skips before a remaining async-for assertion is fixed;
the affected 107 monitoring tests then pass separately in both final builds.
Do not describe these as one complete final-head suite. The stats delegation
suite passes 20 tests with one debug-only skip.

Generated/ownership admission checks and targeted negatives cover the compiler
and consumer changes. An initial 102-test generated suite finds one overly
broad address-rule fixture, repaired with a bounded Publish exception and
targeted followup checks. The value-API gate passes 306 units with zero failing
findings or parse errors; permitted notes remain. Its rebased reference
compilation database is documented, not claimed as a newly intercepted database.

Entry revision `e85171923c08b0d68ee169119e761d7583bbcb26` builds release,
imports 117 modules and passes 462 focused tests with one skip. Its debug/stats
trees were configured only. It has no native debug/stats qualification or full
suite. Full suites were deliberately not run after the performance gate failed.
Golden51's established debugger-jump/LLTrace diagnostic failures and GDB
pretty-printer quarantine remain explicit in its own report.

Integration repaired a valid short completion-target rejection, an uninitialized
entry-trampoline frame-object read, stale byte-sized handler assertions,
async-for admission and resume-wrapper synchronization before the first
observer callback. Source/native component reviews and an independent cold
review found no remaining demonstrated runtime blocker; they do not turn the
failed performance result into an acceptable candidate.

Astra finding 7 is **partially met**. Mechanical gates bind the DSL producer
and C eligibility predicate. The terminal emitter remains trusted compiler
implementation, supported by selected generated-contract checks and source/native
review. A general emitted-body mutation gate was not implemented. The design
and resolution now state that boundary rather than claiming a stronger proof.

## Evidence, publication and retention

Canonical packet: `pyjit/issues/generator-delegation52`. Raw evidence root:
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/generator-delegation52`
(E). The campaign stem `final52-all29-upstream` names the first rejected
candidate; it does not imply selection.

E retains campaign identities, raw vectors, build/check logs, GDB evidence,
zero-loss profiles, `analysis.json` and `pmu-analysis.json`. The reviewed
[analysis tool](analyze.py) checks exact heads, input hashes, pair counts,
stable identities and summaries against raw vectors before producing the tables.
The [initial](capture-perf.py) and [entry](capture-entry-perf.py) capture wrappers
remain unchanged from their recorded provenance hashes.

Experimental code is preserved on branches `pyjit/generator-delegation52` and
`pyjit/generator-delegation52-entry`; selected source remains
`pyjit/round51-golden`. Task-created source/build directories are retired after
retaining patches, configurations, native binaries/extensions and SHA256
manifests under E/`retired-experiments`. Controls, selected builds and unrelated
work remain intact. [HANDOFF](../../HANDOFF.md) records final execution state.

The next architectural hypothesis is a shared resume/transfer contract covering
descent and completion as well as yield. Phase and eligibility/depth weighting
must precede implementation; neither another opcode patch nor a persistent
chain cache is justified by this experiment alone.
