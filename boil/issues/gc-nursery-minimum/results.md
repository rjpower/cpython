# Minimal young collection: current evidence

2026-10-31. Source preparation only; no build, native control or performance
measurement has run. Nothing in this issue is landed or qualified.

- Base: `b3f2bcc605c0174909f2b6df8a92b51995fc070a`.
- Branch/worktree: `boil/gc-nursery-minimum`,
  `/home/power/code/cpython/.worktrees/boil-gc-nursery-minimum`.
- Existing allocator ownership inspection selected header age plus all-old
  conservative roots, avoiding new heap transfer/delete/abandonment semantics.
- 201GiB available before creating the one managed checkout. Shared repository
  objects/caches reused; serving0b15, baseline4da and upstream remain untouched.
- First decision budget began15:15UTC; checkpoint15:45, decision16:15.

The [design](design.md) bounds the consumer and remaining source work; the
[three-stage plan](plan.md) separates later coverage/reclaim/barrier decisions.
Record actual source freeze, review, first failure or accepted controls here
when available. No historical full or timing result counts as this prototype's
evidence.
