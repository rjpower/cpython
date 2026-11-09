# P1/P2 post-review

2026-11-08. Verdict: accept this bounded increment for coordinator landing.
This combines the independent [source review](p2-source-review.md) with the
coordinator's executed evidence. No must-fix finding or performance claim.

The retained `E8/p1p2.patch` and build/focus JSON describe the five-file P1/P2
diff against R5 `0953757f4e7`; the temporary call-route counters are excluded.
The typed code-home helper preserves canonical pointer bits, mortal ownership
and immortal tags in both initializers. The generic entry's NULL and numeric
canonicalization paths remain unchanged before the extracted pointer tail.
P1 retains failure-path revalidation after returning immediately on success.

Both release and debug builds returned zero and report 117 modules: 37 built
in, 79 shared, one platform-inapplicable, none missing or failing import. The
focus receipts identify those build directories and the same five-file diff;
both six-file runs returned zero. The logs report 540 tests on each build,
with **11 release skips and 9 debug skips**. Covered files are value API,
calls, code, frames, generators and monitoring. This corrects the informal
message that gave both skip counts as 11.

The retained release `call_transaction_refresh` assembly publishes the PC at
`0x333275`, then zeroes the result and returns at `0x33327c`; successful refresh
no longer executes the duplicated executable/current-frame validation. The
second validation remains on the error path. This is direct evidence of the
intended P1 machine-code change, not evidence of elapsed-time improvement.

Evidence read: `p1p2-build-checks.json`, `p1p2-focus-checks.json`, both build
logs, both focus logs and `p1p2-call_transaction_refresh.asm`, under the
coordinator-owned `build/pyjit-link-wave1/interpreter-parity-20261108/` directory.
No builds, tests or native workloads were run by this reviewer. Full merged
qualification, fresh committed identities and target/control measurements
remain coordinator responsibilities; this bounded pass does not replace them.
