# DeltaBlue: integration and next work

The round-50 candidate preserves canonical method owners and separates observer
recovery from source attribution. Its [report](report50.md) and
[tables](benchmark-tables50.md) own measurements; [status](../../STATUS.md) owns
qualification. Exact source/build/job state is in [HANDOFF](../../HANDOFF.md).
Golden `4d666e1b482`, frozen48 and upstream remain immutable.

## Current integration

Astra's [grouped review](astra-proposal50-review.md) preceded implementation.
A preserves guarded canonical function/receiver facts through all three method
layouts. The cold [recovery change](source-error-recipe50-proposal.md) copies an
explicit exception recipe into the current evaluator activation; unwinding does
not decode source maps. A regression requires canonical method and exact CALL
specialization with all caller/callee source positions absent.

The adjacent method-to-frame C experiment and its direct-overlap follow-up were
implemented, reviewed and tested. Both lose to A on DeltaBlue; the final follow-up
also loses on logging. They remain rejected experimental commits. Do not expand
their arity/layout coverage or restart the older C2/leaf queue to rescue them.

The batch is complete: release/debug/stats builds, focused semantic/observer
checks, strict value API checks, all 29 kernels against three controls, six-pair
target confirmations and matched post-warmup PMU captures. Both full diagnostic
runs execute 54,708 tests with the established failing module groups and counts;
these remain failed diagnostics, not all-green suites. Independent final review
admits the increment with explicit performance losses. The report owns details.

One GCC configuration, CPU 15, hash seed 0, JIT off and fixed workloads govern
the comparison. Keep quiet timing separate from builds and tests.

## Next performance lanes

Choose the next lane from the final profile, rather than implementing all of
them together.

| Priority | Hypothesis and bounded deliverable | Admission evidence |
| --- | --- | --- |
| Attribute lookup | Cache the inline-values displacement already established by the exact type-version proof, using existing feedback padding. Preserve current shape/validity/mutation guards and payload geometry. This is B in the reviewed proposal; it is unimplemented. | Sol proposal and independent review of layout invalidation, every payload copier and overflow; minimal cache/descriptor tests; actual dependent load removal and matched DeltaBlue/broad screen. |
| Broad regressions | Isolate spectral_norm, coroutines and output logging against frozen48 before attributing losses to this increment. Follow the expensive value/control paths; prefer shared representation or completion fixes. | Direct balanced confirmation and workload profiles; one general fix with focused tests, then a broad screen. No benchmark-specific behavior. |
| Value facts | Extend positive canonical/kind/ownership facts through existing generated boundaries only where native evidence shows repeated work. Avoid optional guards on unrelated ordinary calls. | A concrete producer/consumer proof, fail-closed generator negatives, owner/reentry review and measured benefit over the simpler path. |

Ordinary exact calls already overlap arguments and reuse initialized storage.
They do not copy every argument or sweep every temporary on successful return.
Keep one owned call window, frame ABI, recipient and continuation. Do not add a
second evaluator or owner ledger. Native method/tracing JIT tiers remain later
work; interpreter improvements must stand on their own.

## Source-independent lowering

Ordinary dispatch and call admission have no coordinate lookup or equality gate.
The unwinder now consumes semantic recovery directly. Compiler line predicates
and the serialized observer container remain separate limitations.

A future lowering increment must define logical origin composition and
late-observer/frame-state reconstruction for one transform, then remove that
transform's line restriction. Preserve actual exception edges and ownership
proofs; do not substitute line equality for them or delete observer behavior.
Compare equivalent source with different wrapping while checking execution,
callbacks, redirects and attribution separately. This is a design lane, not a
claimed DeltaBlue speedup.

Sol handles bounded design/debug and independent review; Terra/Luna handle
mechanical/documentation updates. Astra is reserved for a genuinely new grouped
architecture review. The coordinator integrates, measures and owns admission.
