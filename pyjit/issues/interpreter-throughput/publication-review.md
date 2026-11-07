# Attribute publication: source post-review

2026-11-06. **ACCEPT** the final small producer/generator/analyzer diff for
normal integration. No remaining must-fix source finding. This review ran no
runtime, tests, builds, generators or benchmarks. Generated-product comparison
and executed checks remain coordinator/implementation-agent work.

The selected API replaces the earlier proposed publication cache. There are
no new stack-state flags or copy/merge/assignment invalidation machinery.
`IR_RETIRE_PUBLISHED` and its pair form now ensure input consumption and
result publication themselves: validate retired values, `clear_inputs`, then
`flush`, then perform existing retirement and callback refresh/error handling.
This gives one local semantic boundary instead of remembering a prior flush.

The actual diff removes precisely the five redundant `SYNC_SP` statements in
`LOAD_ATTR_INSTANCE_VALUE`, `LOAD_ATTR_SHAPE`, `LOAD_ATTR_MODULE`,
`LOAD_ATTR_WITH_HINT` and `LOAD_ATTR_SLOT`, and deletes the entire specialized
`sync_sp` override. It preserves the first temporary result publication before
receiver release and the final folded-local store after retirement. It does
not call `stack.clear` at this boundary, which would lose the C-held result
needed by the folded-local path. Existing generic attribute completion and
its publication postcondition are unchanged.

All ten source retirement callers were checked: those five loads; paired
retirement in stores INSTANCE_VALUE, SHAPE, WITH_HINT and SLOT; and single
retirement in STORE_ATTR_SHAPE_ADD. The store producers retain their existing
`SYNC_SP`; their inputs are already cleared, so the macro's new `clear_inputs`
has no pending inputs to consume. Receiver/old-value ordering, failure result,
pending-owner checks and refresh after an actual callback are unchanged.
Retired-view validation occurs before clearing input metadata.

Removing DSL `SYNC_SP` must not erase synchronization metadata. The actual
analyzer diff recognizes both retirement macros in `properties.sync_sp`.
No-pop error inference and release/effect classification already recognize
these macros and remain unchanged. Operand signatures, formats and deopt
guards do not change.

The focused tests check one temporary store before retirement, the retained
final destination store, synchronization inference without explicit `SYNC_SP`,
and rejection of an escape between `OWNER` and retirement. Their execution is
pending at this source-review point. Before merged integration, require the
real generated handlers to differ by exactly five duplicate pre-retirement
store deletions, with generated synchronization/error/effect metadata unchanged,
then the coordinator's release compile and focused semantic checks. No speedup
is established by the source change alone.
