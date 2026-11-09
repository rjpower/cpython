# Epoch and nbody-ledger source peer

During native26 freeze, reviewed the exact Future27 LINE epoch predicate and
mask transitions in `class-init-line-epoch-proposal.md`. Approved as a bounded
source design; no runtime edits were made by this review.

ClearToolId assigns a strictly newer global version, and successful force stores
that version on the code. Overflow is refused. Comparing relevant LINE tool
identity versions against the code's last successful version therefore detects
uninstalled reuse without repurposing local tool-version semantics or adding
fields. The effective LINE union must first clear stale local identities using
the same rule as update_instrumentation_data. Epoch invalidation precedes wrapper
mutation and covers LINE mask changes, relevant identity reuse and active LINE
restart. Unrelated INSTRUCTION-only changes preserve delivered LINE state.

For a reused bit, explicit removed-old plus added-new LINE masks are necessary
to reset old physical/source DISABLE decisions. An epoch-only change would not
close that hole. Callback replacement alone remains unversioned and does not
restart previous DISABLE decisions; the proposal and tests state this boundary.

The nbody ledger plan and capture script preserve the unchanged workload and
separate handler counts from native instruction/cycle attribution. Empty
subtraction and raw nonlinearity reporting are appropriate. Before capture,
record/assert monitoring global and selected-code local masks in addition to
sys.gettrace/profile, and compare selected routes across 0/N/2N or retain their
differences explicitly. The coordinator owns source/build/environment identities,
all stats execution, and subsequent matched release measurements.

## Native27 bounded postreview

Coordinator native27 receipts report both 117-module builds passing, 21 focused
tests across four modules and 51 existing tests across three modules passing in
each build. The faithful original class-init probe passes both its base and
opcode forms on both builds, where the previous opcode form failed. Existing
source/epoch/redirect assertions were retained. The actual epoch diff matches
the admitted predicate and removed/new mask transition. Approved for an
engineering checkpoint; broader debugger and performance admission remain open.
