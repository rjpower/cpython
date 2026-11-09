# Independent review: attribute result publication contract

2026-11-07. Reviewed Sol's amended `contract-proposal.md` against runtime
`7780e32e0e2`. This is a design review; it does not approve source edits or
shared header changes.

The original concern was whether generic `IR_COMPLETE_ATTR_READER` could be
reported as `DST_TEMP_CANONICAL_BEFORE_RETIRE`. My first read incorrectly
placed generic publication after receiver retirement. The source disproves
that: `value_reader_complete()` validates the result home, calls
`_PyValue_Publish()`, and stores the temporary result before
`value_operation_close3()`, `_PyValueTemps_Close()`, and refresh
(`Python/ceval.c:1849–1864`). The method form publishes and stores both
outputs before receiver/temporary release (`1904–1917`). Generic lookup can
call Python before it creates the result, but that does not change the phase
of this first output relative to retirement. One shared semantic phase is
therefore accurate; the contract correctly limits it to the first output and
does not claim publication before entering the helper or whole-handler
purity.

The amended proposal also fixes the suffix proof. It validates the named
result against the `ROLE_DST` scalar output, maps subsequent uop positions
through existing layout/role facts, and admits a suffix only when its
read/write positions are disjoint and its effects cannot release, call out,
deopt, error, or switch frames. The companion output at `dst+1` is accepted
by position and effects, not by an opcode-name exception. This avoids making
future producers depend on a special-case whitelist.

The proposed contract is coherent for both destinations. A temporary result
is published into its home before retirement; a folded-local result remains
C-owned while the old local stays rooted, is passed as `failed_result` on
retirement failure, and is installed only after successful retirement. The
phase metadata is success-path information, not a root map or recovery
snapshot. Error cleanup, callback refresh, and the generic helper's validation
failure behavior remain implemented by their existing helpers and are not
inferred from the phase bit alone. The generic helper's first-output
postcondition is checked by the same source contract consumer, while its
second method output remains outside this fact.

Before implementation, tests should cover the source-level contract and its
rejection boundaries: correct result/owner and first-output phase; folded
local and temporary emission; unchanged companion output; wrong output or
missing owner; nested/duplicate/conditional marker; result reassignment or
address-taking; escaping or output-overlapping suffix; ambiguous join; and
ordinary retirement without a result fact. Keep the existing `OWNER()` escape
check. Metadata generation should report the same phase for validated
specialized markers and the validated generic helper flow, and `NONE` for an
ordinary retire-only handler.

Disposition: accept the amended semantic design for bounded generator/analyzer
implementation, subject to root's integration review. No additional phase
enum is needed. No runtime optimization or throughput claim is supported by
this contract alone.
