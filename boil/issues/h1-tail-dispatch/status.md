# H1 tail dispatch

- State: deferred
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: an active Clang-tail consumer and a bounded matched question
- Branch/head: archived `boil/h1-frame` at `94ab98e8fd3`
- Worktree: retired; archived source and selected evidence retained

## Now

The passed-frame tail-dispatch runtime experiment remains unmerged. Its
historical native source is `62456cc59bd`; neither that implementation nor its
bounded correctness evidence establishes a current performance gain. It does
not change the maintained GCC dispatch path.

The exact `strrchr` noncollecting declaration and tool regression originally
recorded at `94ab98e8fd3` are included in qualified `0b15b4bbc9b` through the
separately reviewed prerequisite. That inclusion does not land H1's runtime
frame-passing convention or import the archived branch wholesale.

Historical audit failures and informational platform-header harness limitations
remain attached to their evidence. Archived native and source identities are
not current build or benchmark authority. No checkout, native job or timing
window is assigned to H1 now.

## Next

Reopen only for an active Clang-tail backend with a measured consumer and a
specific calling-convention question. Assign ownership and review the retained
patch against current source before changing code or preparing a matched
comparison. Do not reopen it merely to complete an old schedule or chase an
unmeasured dispatch gain.

## Completion

On reactivation, preserve frame/root/publication semantics and compare the
coherent convention under matched build policy. Land only a supported useful
change, or close with a reported inconclusive/negative result. Deferred is not
abandoned; no new qualification or speedup is claimed by this status.

## References

- [Original experiment and invariants](../../design/work/H1-1.md).
- [Independent design readiness](../../design/work/reviews/H1-1-design-readiness.md).
- [Handler contract](../../design/14-handler-contract.md).
- [Archive and evidence retention](../../design/work/worktree-closure-plan.md).
