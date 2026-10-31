# H1 tail dispatch

- State: deferred
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: an active Clang-tail consumer and a bounded matched question
- Branch/head: retained `boil/h1-frame` at `94ab98e8fd3`
- Worktree: none active

## Now

The passed-frame tail-dispatch runtime source `62456cc59bd` is unmerged and has
no measured current benefit. It does not change maintained GCC dispatch.
The exact `strrchr` effect declaration and control from `94ab98e8fd3` are
included in qualified `0b15b4bbc9b`; that does not land the runtime convention.

## Next

Resume for an active Clang-tail consumer. Reconcile the retained implementation
with current frame/root/observer contracts, then compare one coherent calling
convention under matched policy. No work is assigned to this issue now.

## Completion

Land a useful convention with preserved call/return/yield/unwind behavior, or
close with a reported negative/inconclusive result. Deferred is not abandoned.
A source patch or bounded correctness result is not a speedup claim.

## References

- [Handler and observer contract](../../design/handler-and-observer-contract.md).
- [Compiler and frame interfaces](../../design/compiler-and-ir.md).
- [Measurement contract](../../design/measurement.md).
