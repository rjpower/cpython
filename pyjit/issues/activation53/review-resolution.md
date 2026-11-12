# Activation53 review resolution

One broad [Astra review](astra-review.md) is complete. This resolves design
choices; implementation and measured admission remain pending. The corrected
[design](../../design/activation-transfers.md) supersedes the alternatives in the
original [thesis](thesis.md).

| Review finding | Decision |
| --- | --- |
| Exact calls and resumes already enter direct interpreter paths | Preserve existing overlap/single initialization and resume flow; no fictitious C-recursion removal. Native vector generator creation does have an evaluator entry that direct construction can avoid. |
| Arbitrary prologue in heap storage needs a constructing phase | Excluded. Require physical word zero RETURN_GENERATOR with actual validated recipient/successor; reject prefixes before effects. |
| One ordinary branch cannot be avoided without a different opcode | Accept one measured generative eligibility branch; no common allocation, transaction or metadata stores. |
| Shared constructor must not change frame-returning API | Exact IR and validated known positional adapters plus eligible native object/value vector boundaries share a tri-state owner constructor above frame push. Unsupported generic/keyword calls retain existing frame API. |
| Allocation and code mutation | GIL allocation schedules GC without invoking Python. Pin explicit selected code, allocate valid CLEARED shell, NULL/init final homes, then move owners and seal without failure/callback. No speculative post-GC replay. |
| Observation and origins | Conservative decline before effects for unsupported observer/PEP523/origin/DTrace/LLTrace/concurrency requirements. Preserve recursion/capacity admission. |
| Kind-only/full descriptor is poorly weighted | Reject: every resume would store facts to save a small decode only at completion; no new frame fields. |
| Completion opportunity is C boundary, not missing canonical specialization | One shared canonical terminal after child cleanup; retain immutable operand decode and existing cold observed/error path. Preserve ordinary inline return. |
| Return-local move needs exposure/frequency proof | Deferred; not required for this round. |
| Phase census does not establish fast hits or cycle budgets | Add separate path diagnostics; use unchanged quiet elapsed comparisons and native work checks for admission. |

Two Sol lanes own isolated `activation53-final-homes` and
`activation53-completion` worktrees from golden51. Each writes a concrete
proposal and cross-reviews the other before source implementation. Root owns
regeneration, builds, native tests, measurement, commits and golden selection.
No result is currently selected. Detailed backend owner/effect proof boundaries
must be recorded honestly; selected output checks are not a general emitted-body
mutation proof.
