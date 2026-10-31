# Bulk frame payload transfer

- State: closed
- Outcome: abandoned
- Decision: reject this implementation
- Owner: stats_design_review; coordinator accepted the result
- Updated: 2026-10-31
- Budget: 90 minutes; stopped after the two declared comparison rounds
- Candidate: 5b4d7d9126c, retained on boil/frame-transfer-bulk; unmerged

## Hypothesis

Replacing the shared scalar frame payload move with memcpy plus source memset
would produce simpler/faster native execution while preserving all three callers.

## Result and decision

The one-header candidate passed source review and release OFF behavior checks.
Native code contains two libc calls. Two matched process rounds showed no
repeatable target benefit: coroutine ratios were 1.0023 and 0.9652, generator
ratios 0.9966 and 0.9925 (parent elapsed / candidate elapsed). Other rows were
noisy. These data justify rejecting the implementation, not a precise causal
regression estimate or a claim that frame transfer cannot improve.

The coordinator independently replayed all 80 values and verified 190 evidence
references. Preserve the separate release-KILL startup configuration failure;
it earned no test/kill credit. No further build, debug/ASan/full qualification
or timing is assigned to this rejected candidate.

## Next

Continue the independent [shape-admission experiment](../shape-admission-reuse/status.md).
Keep this candidate and evidence separate from primary; no runtime change landed.

## References

- [Results and retained evidence](results.md).
- [Source/lifetime proof](design.md).
- [Executed finite plan](plan.md).
