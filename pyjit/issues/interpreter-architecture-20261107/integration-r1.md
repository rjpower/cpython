# R1: remove write-only call transaction state

Integrated runtime `22f49d676db`, parent `7780e32e0e2`, 2026-11-07.
`Python/ceval.c` deletes the unused transaction-record field, initializer and
five writes and corrects its comment. The handler's live `CALL_EFFECTS` and
replay guards, owner transfers and refreshes remain unchanged.

The [proposal](calls-proposal.md), [proposal review](calls-proposal-review.md),
[actual-source post-review](calls-cleanup-review.md) and
[fresh cold review](cold-r1-review.md) accept the change. The cold review's
optional direct-return polish is not selected: the named status keeps the
existing error conversion clear and needs no unrelated edit.

Release and debug builds pass and each check 117 modules. Both focused runs
pass 916 tests across `test_call`, `test_ir`,
`test_pyvalue_native_call_transaction`, `test_sys_settrace` and
`test_monitoring`, with one skip each (7s and 35s). Release has the same 17
distinct warning lines as the prior C build, with zero new warning lines.
Build-info relinking ties both profiles to the new commit. Stats is not yet
rebuilt; full suites remain for the coherent batch boundary.

The seven-kernel screen uses three alternating upstream/candidate rounds and
seven values per invocation, at unchanged source/executable identities. Raw
candidate elapsed geomean versus the fresh C baseline is 0.992732, while the
upstream control changes by 1.007705. Individual rows are mixed; this is not
a measured broad speedup. The cleanup is admitted for removal of misleading
dead state, not an elapsed-time claim.

Evidence root:
`build/pyjit-link-wave1/interpreter-architecture-20261107/` under
`python-boil-the-ocean`: `r1-checks.json`, build/test logs,
`r1-identities.json`, `baseline-7780-basic/` and `r1-basic/`.
