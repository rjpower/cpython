# Conditional admission review

2026-11-06. Read-only critical synthesis while broad suites run. Runtime is
frozen at `7780e32e0e2`, following A `8a8fdc91368` and B `ba37cc10058`.
This is not the final measurement audit and makes no final performance claim.
No runtime, build, test, generator or profiling work was run by the reviewer.

## Recommendation now

Keep the source frozen through the already scheduled broad correctness and
final measurement work. Do not revert midcampaign, change thresholds, or keep
running small screens until a favorable result appears. Source acceptance and
focused passes permit this evaluation; they do not admit a speedup.

Treat B's initialization elision as provisional performance work. The shared
primitive and held-owner proof are sound, but computing a caller extent and
adding a conditional memset path is additional executed work. If the scheduled
evidence still cannot establish a repeatable benefit, prefer removing the
clear-elision optimization from the selected runtime and retaining its reviewed
patch/evidence. Preserve the independent declined-provider assertion fix and
precise ownership comment from B; a blanket commit revert would remove those
useful corrections too. Root owns that decision and any resulting validation.

A lower instruction count without a throughput gain can establish a mechanism,
not a user-visible speedup. For a neutral result, weigh that fact against B's
extra hot-path arithmetic and maintenance cost. It is not enough that a logical
write was redundant. If B is retained despite an unresolved elapsed result,
describe it as a bounded known-NULL reuse change with unproven throughput
benefit, not an admitted performance win.

C has a different maintenance tradeoff: it reuses an existing completion
postcondition and removes repeated work without adding runtime state or a
second path protocol. A neutral C result may justify keeping that clear,
truthful contract as a small deduplication change, provided no material
regression appears. That still supplies no elapsed-time improvement claim.
The new common-call path in A likewise needs observed workload benefit; its
reviewed ownership protocol is not itself a timing result.

## What the screens show and cannot show

I read the three-round summaries/vectors under
`build/pyjit-link-wave1/interpreter-throughput-20261106/` and independently
recomputed the following equal-weight seven-kernel ratios. These are
sequential integration screens with alternating roles, not matched simultaneous
parent/child measurements or the full 29-kernel/subset result.

| Screen | Candidate elapsed / prior candidate | Upstream elapsed / prior upstream | Change in candidate/upstream ratio |
| --- | ---: | ---: | ---: |
| A / 6a | 0.982103 | 0.971868 | 1.010531 |
| B / A | 1.003425 | 1.025898 | 0.978094 |
| C / B | 0.994414 | 0.992116 | 1.002317 |

The final column is a sensitivity check, not a statistical correction for host
drift. It can change the sign of the apparent result. For C/B, raw logging
candidate time rises 4.07%, while its candidate/upstream ratio falls 10.37%
because the upstream control rises 16.12%. For float, the candidate falls
2.14%, while its relative ratio rises 17.52% as upstream falls 16.73%.
Neither calculation alone identifies C's causal effect.

The C screen has upstream round-median max/min spreads of 25.42% for deltablue,
17.85% for logging and 17.06% for nbody; candidate generators spread 13.83%.
Candidate C/6a raw seven-kernel geomean is 0.979962, while the upstream control
geomean changes to 0.989177. Do not turn that roughly 2% raw reduction into a
batch speedup claim, a significance claim, or a 29-kernel aggregate.

B's raw kernel B/A ratios are deltablue 1.0107, logging 1.0333,
comprehensions 1.0006, Richards 1.0373, nbody 0.9900, float 1.0095 and
generators 0.9455. This is a mixed screen, not evidence that B is uniformly
harmful or that its generator improvement is caused by skipped zeroing.
Small candidate spreads within one stage do not remove between-stage control
drift. No new resampling protocol is proposed by this review.

## Source and GCC evidence

Independent cold A/B/C reviews accept source correctness. Focused release/debug
passes and all three builds are recorded in the integration receipts; broad
suite completion is still pending at this review point. Source is clean and
frozen. These facts justify finishing evaluation, not bypassing it.

The retained B GCC ELF has build ID
`ae5418ac38daab3c6f2c9fd74d84778c8700df8e` under `integration-b-gcc/`.
In `_TAIL_CALL_CALL_PY_EXACT_ARGS`, the overlap path computes and clamps the
known prefix (`0x363ad7..0x363aec`), and branches around the tail memset
(`0x363b97..0x363bbd`) when no new cells need clearing. Other entry/transfer
paths still contain memsets. This confirms the intended conditional lowering;
it does not count its executed calls, bytes avoided, or cycles saved.

B does not introduce liveness metadata or weaken all-homes-valid semantics.
The skipped region was already NULL by held-owner ordering, not merely dead
to future reads. Constant owners, new tail initialization, frame moves,
generators, error cleanup and ordinary locals-only retirement retain their
contracts. That makes the experiment defensible; it does not make its cost
automatically worthwhile.

C's generated C restricts the five final destination stores to locals; their
temporary paths become unreachable while folded-local stores remain. Smaller
generated C, fewer source stores, smaller
ELF text, or removed canonicalization instructions are mechanism evidence.
None alone proves that critical-path elapsed time improved. Likewise, call
argument copies removed on A's supported shapes do not quantify how often
those shapes run or what fraction of the whole workload they cost.

## Final decision evidence

The final audit should check the actual frozen source/build/module identities,
load and command journals, failed-attempt exclusion, sample arithmetic and
declared roster before discussing outcomes. Report elapsed and retired native
instructions **per workload loop** for the selected regressions. Lower counts
can support reduced executed work; they do not identify per-bytecode cost or
prove one source mechanism caused the difference. Native cycles/instruction
and control variability are useful limits on interpretation.

A final C-versus-6a campaign evaluates the composition. It cannot isolate B
or C from A by subtraction. Retain that attribution limit if no same-window
parent/child comparison exists. Correctness acceptance, structural cleanup and
throughput acceptance are separate conclusions. Do not claim the register
interpreter is competitive with upstream unless the actual workload results
support that scope, and do not use zero-box census debt as a throughput veto.

## Next IR contract priorities

1. **Frequency before another mechanism.** Determine which call shapes and
   generic attribute/helper paths remain hot after the batch. Small diagnostic
   counters may count supplied/default/varargs shapes, initialized words and
   consume/publish/refresh events separately from release timing. Avoid a broad
   instrumentation project or new runtime switches. Preserve representative
   workloads and distinguish changed instructions from changed semantic work.
2. **One operation contract for phases and effects.** Centralize ownership
   consume, publication, retirement, may-escape and error/observation obligations
   in the existing generator metadata. The `IR_RETIRE_PUBLISHED` sync-metadata
   amendment shows why marker spelling cannot be the only source of semantics.
   Improve one reusable macro/helper class at a time; do not build a second
   hand-maintained opcode model or distributed publication cache.
3. **Separate future-read liveness from owned homes.** Keep NULL validity now.
   Dead-but-owned references, observer-visible locals, hidden constants and
   cleanup order must survive any future map export. Before adding metadata,
   identify an actual root/copy/unwind cost that it can remove. A PC-only map
   cannot describe partial instruction ownership or suspended C-local owners.
4. **Keep interpreter and optimizing IR separate.** A method/tracing compiler
   may use typed SSA, explicit effects and recovery snapshots, but must restore
   the interpreter's home/PC/exception contract. No second compiler, encoding
   restart, GC redesign or held ctypes/LRU implementation follows from this
   batch's measurements. Tighten baseline contracts and measured work first.

The next useful packet should have a concrete hot path, a small shared contract,
an ownership proof, focused correctness checks and a falsifiable performance
expectation. Source correctness and an attractive byte count are insufficient
reasons to keep accumulating performance-only complexity.
