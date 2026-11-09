# Call frame progress

2026-11-06: Read project coordination and CPython AI policy, inspected generic
transaction, consuming binder, frame stack and exact CALL overlap. Proposal
written; awaiting coordinator and independent review before implementation.
No runtime files changed and no shared builds or tests run.

The coordinator accepted the proposal after independent review and captured a
clean timing baseline. An attempted write to the separate runtime worktree was
rejected by automatic approval review as an unreviewed memory-safety-sensitive
change outside the authorized current worktree. The rejection expressly bars
workarounds or indirect execution. No runtime source was changed; no compile or
tests were run. The coordinator was notified. The proposal remains the
reviewable artifact; source implementation requires resolving this restriction.

Prepared a concrete C and focused test patch within the canonical workspace,
without touching the runtime worktree. Sol's source review accepted the final
artifact after cleanup and test revisions; see `call-frame-review.md`. The final
artifact passes `git apply --check` against the recorded runtime source. It has
not been compiled or executed; coordinator owns any subsequent integration and
validation. Tracing tests cover observer fallback, not mutation during kwargs
allocation.

The coordinator applied the reviewed integration and corrected the empty-tuple
assignment to create an owning `PyValue`. The release unit syntax check passed
(`call-release-unit-v2.log`). Shared builds and focused execution are pending.
The separate `frame-overlap-init.patch` is unapplied and under review.

Sol accepted the separate overlap initialization source artifact; the final
comment distinguishes generic caller responsibility from overlap's already-NULL
prefix. The artifact includes widened caller windows and mortal hidden constant
coverage. It remains unapplied and unexecuted pending coordinator integration.
