# V8 review and architectural comparison checkpoint

2026-11-08. The coordinator prioritized call preparation before completing a
comparative case. The user challenged that narrowing. Selection is now held
while the competing mechanisms are evaluated. The qualified runtime remains
R5 `0953757f4e7`; a three-line refresh cleanup is preserved only in the
isolated task worktree, uncommitted, unbuilt and untested.

## Eight recommendations and their scope

The [V8 review](v8-review.md) compares pinned primary sources with our actual
runtime, not an assumed missing frame stack. Its ranking is advisory; the
[decision ledger](recommendations.md) owns selection.

| Recommendation | Concrete opportunity | Current assessment |
| --- | --- | --- |
| Preserve specialized call facts | Stop rediscovering type/provider/preparation facts before any escape; share final prepare/commit | Structural candidate. Does not remove fresh kwargs or post-allocation guards |
| Preserve known code-object types | Avoid generic numeric canonicalization at executable-home initialization | Small cleanup; correct shared tag and ownership domain required |
| Remove duplicate successful refresh validation | Successful refresh already proves current frame/code/PC | Small cleanup. Held diff preserves the potentially reentrant failure path |
| Model state publication by execution phase | Reuse published/canonical state until a callback or owner retirement invalidates it | Structural candidate; shape/method leaf behavior already exists |
| Make register equivalence ownership-aware | Place producers into final argument windows and remove avoidable transfer dispatches | Compiler candidate. Evaluated snapshots and destruction order remain required |
| Assess operand-class/window representation | Remove demonstrated local/temporary classification costs through generated forms or layout changes | IR candidate. Encoding changes need range, instrumentation and fallback migration |
| Improve dispatch/helper machine interface | Remove surviving spills/helper boundaries; consider observer-preserving fusion and density separately | Broad candidate under the same GCC. Compact words or a custom backend require evidence |
| Share recovery contracts with native execution | Reuse canonical homes, prepared owners and once-only effect/transfer state | Accepted future JIT boundary; cannot improve JIT-disabled parity |

Five are structural interpreter/compiler candidates, two are local cleanups,
and one belongs to the later JIT stage. Implementing the two small cleanups
would not constitute incorporating the architectural review.

## Concrete competing designs

The [independent alternatives review](alternatives-review.md) compares call
entry, publication phases, compiler/window construction, operand classes,
dispatch/fusion and frame/constant policy. A further Sol proposal and two
independent review rounds made three alternatives more concrete:

- A dictionary-owned leaf-hit primitive with checked generated completion,
  preserving canonical result ownership and sequential input retirement.
  Compare lookup savings separately from completion/reload savings. Also
  assess proven leaf temporary producers without assuming all PC stores can
  disappear.
- Return/result-discard and selected branch/NOP continuations that retain
  actual observer-boundary words and fallback paths. Recheck instrumentation
  after child clearing; a destructor can enable tracing. A complete observer
  predicate is still required before implementation.
- Dictionary feedback with a proved callback-free probe prefix. A stable
  keys version and matching final entry alone cannot justify skipping earlier
  collision equality callbacks. Mutation/version proofs remain open, and a
  cache may be unnecessary if the first probe already hits.

These designs are competing proposals, not accepted implementations or
additive speedup estimates. See [mechanisms](mechanisms-proposal.md),
[first review](mechanisms-peer-review.md) and
[second review](mechanisms-peer-review-round2.md).

## Measured starting point

The independently checked six-round pinned `logging_silent` baseline is
**1.657289x upstream**, with ratios spanning 1.636744–1.689003. Matching it
requires about **39.7% less candidate elapsed time**. It is a different
protocol from the older 1.743034 result, with no runtime source improvement
between them. The prior qualified perf campaign found 1.720317x instructions
and 1.629600x cycles per workload loop; those counters are different
measurements and are not per-bytecode ratios.

Existing stats, independently checked against raw dumps and exact input
scaling, give **161 dispatches and 20 frames per loop**: ten general calls,
ten exact calls, twenty method loads, twenty shape loads and ten dictionary
subscriptions. Nine result POPs and ten line-boundary NOPs execute. The tenth
logging result is already discarded; the extra static POP inventory site
belongs to pre-loop setup. Dispatch shares do not measure elapsed savings.
Both hot callees have zero constant homes; frame constant relocation does not
address this workload.

The runtime already has frame stacks, known-NULL overlap reuse and locals-only
ordinary clearing. Constant copying does not occur in these hot callees.
Further frame work must address actual remaining setup/retirement. Evidence
supports investigation of repeated entry work, generic helper completion and
dispatch/state overhead. It does
not yet assign a causal fraction to any of them. See
[diagnostic results](diagnostic-results.md) and
[independent baseline audit](baseline-evidence-review.md).

## Next gate

Use bounded diagnostics and release GCC inspection to compare covered machine
work, semantic proof, shared implementation cost and regression risk. Resolve
the observer and dictionary proof obligations before implementing those
mechanisms. Select the next coherent batch in the ledger, document why other
alternatives wait, then follow the normal implementation, focused checks,
post-review and integration cycle. Preserve target/control timing at integration
and full qualification at the coherent runtime checkpoint. No parity or new
runtime performance gain is claimed at this design-comparison checkpoint.
