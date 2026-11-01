# H1 tail dispatch

- State: held for integrated architecture review
- Updated: 2026-11-01
- Owner: coroutine_design_review prepares helpers; coordinator reviews and integrates
- Dependencies: exact compiler/backend plan and independent review before native execution
- Prior grant: host-only VM0 helper preparation, 01:09–01:39 UTC; suspended at strategic steering
- Current managed helper source: `boil/vm-compiler-backend` at `417a9443a59`
- Worktree: `build/worktrees/vm-compiler-backend`; no runtime source changes

## Now

The [integrated review](../fundamental-runtime-design/status.md) supersedes the
assumption that the current IR and collector should remain unchanged. VM0 is
still a useful compiler/backend control, not the architectural answer. Helpers
are unfinished and unready; no compiler or target has run. Resume only with a
new explicit remaining scope and clock. Earlier H1 source remains retained on
`boil/h1-frame` at `94ab98e8fd3`.

The [new candidate](../../design/vm-design-candidate.md) selects a matched
GCC-computed/Clang-computed/Clang-tail comparison on unchanged runtime source,
then one frame-argument ABI experiment. Three parallel source reviews agree
that this tests a concrete difference in existing machinery. The initial
planning grant authorized no compiler, target, timing, runtime edit or worktree.
The earlier slot-base-only increment is rejected on its actual elapsed result.
The [revision 2 plan](compiler-backend-plan.md) preserves revision 1 and corrects
PIE policy and actual stats-file capture. Independent coordinator
[source/plan review](reviews/compiler-backend-plan-20261101-v2-root.md)
accepted held host-helper preparation. One managed worktree from exact 417 is
authorized for helper storage; runtime source changes and all native execution
remain held. Actual compiler-selected tools are captured only in a later
reviewed build row, not guessed from GCC's tool manifest.

The passed-frame tail-dispatch runtime source `62456cc59bd` is unmerged and has
no measured current benefit. It does not change maintained GCC dispatch.
The exact `strrchr` effect declaration and control from `94ab98e8fd3` are
included in qualified `0b15b4bbc9b`; that does not land the runtime convention.

## Next

Freeze exact helpers and finite literal native rows using existing build/root/
quiet protocols, strict per-arm policy and actual retained stats witnesses.
Review the helper capsule independently before execution. Then reconcile the retained five-state convention with
current frame/root/observer contracts. Do not combine that ABI change with
slot-base derivation, IR-word removal, helper-effect changes or GC changes.

## Completion

Land a useful convention with preserved call/return/yield/unwind behavior, or
close with a reported negative/inconclusive result. Deferred is not abandoned.
A source patch or bounded correctness result is not a speedup claim.

## References

- [Handler and observer contract](../../design/handler-and-observer-contract.md).
- [Compiler and frame interfaces](../../design/compiler-and-ir.md).
- [Measurement contract](../../design/measurement.md).
