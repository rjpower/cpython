# Next steps after the frame/call round

These are the coordinator's five priorities from two independent reviews of
frozen runtime `4d666e1b482`. They are proposed work for the next round, not
requirements that extend this round. Final benchmark evidence belongs in the
[closing report](closing-report.md). The reviewers used a senior engine-design
perspective; neither represents V8 or LuaJIT.

The selected foundation stays: one canonical value format, initialized owning
homes, overlapping arguments, explicit recipients and continuations, and a
commit-before-close transaction. Python semantics govern behavior. Expanding
CPython's debugger jump matrix and matching tooling display formats are separate
work, with explicit limitations rather than an interpreter promotion veto.

| Priority | Change | Why | First admission gate |
| --- | --- | --- | --- |
| 1 | Derive ownership effects and canonical producer facts from the emitter's structured representation | Lowering, generated bodies, observer outcomes and cold validation currently repeat knowledge of the same transfers. This makes new optimizations expensive to prove and easy to get wrong. | Migrate one existing proof with identical generated code. Mutation/alias/error tests must reject changed transfers; no new hot metadata walk. |
| 2 | Attach retirement to actual semantic outcomes | Remaining release-only operations can cost dispatches. A branch, return or restore should describe the owners it retires rather than inherit frontend stack cleanup. | Count executed classes first. Remove one bounded class while preserving GC roots, overwrite, unwind and reentry. Compare removed dispatch work against added decoding and closing. |
| 3 | Bind certified generator/coroutine calls directly into their owning heap frame | The current creation route reserves a thread activation, constructs a heap frame and copies ownership. Avoiding that work has more potential than tuning an already-small final store. | Separate creation from first-send/resume costs. Certify prolog, defaults/keywords, monitoring, code replacement, partial failure and GC visibility before a fast path. |
| 4 | Derive recipient classes without duplicating return transactions | Empty scratch, replacement local and discard have different costs. A proved recipient may avoid unnecessary old-owner work. | Count classification updates and deliveries: they are often one-to-one. Compare whole entry plus return assembly, spills, text, instructions and elapsed time. No new frame field without measured benefit. |
| 5 | Tune compiled hot paths and feedback placement from current profiles | Source simplicity and lower instruction counts do not guarantee lower cycles. Inlining, code size and placement can change CPI and spill cost. | Use retained matching ELFs and balanced workloads. Outline only demonstrably cold work; measure total text and important kernels. Avoid duplicating ownership implementations or keeping tuning switches. |

The first item improves the architecture without claiming an immediate speedup.
The others are falsifiable performance experiments. Do not implement all five
as one batch. Each gets a proposal, independent review, bounded implementation,
focused tests, post-review, integration and a basic performance check. Reserve
full suites and the complete benchmark matrix for coherent checkpoints.

Source/detail references:

- [Independent architecture review](closing38-cold-architecture-review.md)
- [Independent implementation review](final-engine-architecture-review38.md)
- [Recipient alternatives and full cost ledger](return-recipient-options-after36.md)
- [Lifetime policy](../../design/lifetimes-and-language-semantics.md)

## Deferred work, outside the measured runtime

The retained owner-bundle draft, bounded coroutine ingress proposal and partial
GDB port are not integrated. Their complete source patches and manifest are in
`E11/deferred-after39-source/`; the authoritative isolated coroutine patch is
`E11/coroutine40-frozen-source.patch`. The task-created isolated worktree was
removed after verifying that the complete dirty patch was preserved.

The coroutine packet passed its minimal release run, but debug asserted while
instrumenting the synthetic exception-handler-to-RESUME negative. The exact
packet was reversed before final qualification. This is an unresolved
constructor/instrumentation boundary, not evidence of a hot resume slowdown.
The owner draft has no native admission. The GDB reader has a concrete value
struct/argument-provider port to finish, with no runtime throughput dependency.

Nine ordinary frame-jump tests remain capability gaps. Revisit them through a
shared retained-owner/context proof if debugger support is prioritized later;
do not add per-test setter rules. None of these drafts changes the final candidate
benchmark or qualification inputs.
