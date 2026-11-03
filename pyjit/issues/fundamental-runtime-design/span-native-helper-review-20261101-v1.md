# Stable-span focused native helper review

2026-11-01, coordinator. **ACCEPT one bounded first-feedback activation** for
clean source `7f982b3ecb6ae8e7f1a5c0433daa9acde870b7e4`. No native execution
or performance credit follows from this readiness review.

Reviewed the actual helper delta and four literal rows: fresh GCC computed
O1/Py_DEBUG build/test, then matched default O3 build/test. Release uses the
inherited effective O3 recipe without an extra CFLAGS override; debug uses
`CFLAGS=-O1 -g`. Both retain frame pointers, zero initialization and existing
root/inline audits, with GIL/mimalloc and no stats/JIT/PGO/LTO/tail backend.
The source-only proposal's explicit release CFLAGS spelling is superseded by
this matched inherited recipe; it is not a different optimization comparison.

Each test row selects all twenty span methods and six existing root/lifecycle
controls. Debug requires all 26 without skips; release requires the same exact
roster and precisely four named debug-only skips, leaving 22 ordinary cases.
All other cases remain protected. Exact skip identity/reason checking is the
only behavior-validator change; unexpected or missing skips are rejected.
The existing observer, whole-worker contract, product loader and shell worker
are unchanged. Children retain the source's explicit quarantine/KILL and
expected-fatal controls; those are not successful ordinary child executions.

Root rehashed all 29 capsule artifacts and 4,611 source/test/helper inputs.
The nineteen retained host controls include actual prior worker/root contract
replay and wrong-roster/skip/mode/source/fallback cases. Positive unpinned
diagnostics remain report-only. Root additionally loaded the actual launcher
dependency closure with process creation forbidden, checked all local import
origins and proved missing activation blocks before a process launch.

Plan SHA256 `5479201a64871fb411e67924cfe102180bb75248971dc983979733d7ae771c6c`;
manifest `0d78c003cef7c608cb1a73ee1c2eec5214228064a2f53526f5fadd9d54154d22`;
capsule `081c9ffaf38152a22ff296ca635ce8e9c34051f619af37bfda4f19ff81089ed3`.
Root receipt: `build/span-native-helper-review-20261101-v1/receipt.json`.

The existing run-row/build-lock/machine-slot/serialization and terminal-cleanup
protocol is retained. Activation is root-only, at most 45 minutes and clamped
to the 15:14:44 UTC GC cutoff, including the existing 210-second reserve.
Stop at the first failure; unused time grants no retry. Verify actual source,
configuration, tools, products, test IDs/modes/origins, positive collections,
zero unwind fallback and idle cleanup before credit. Optional extension
omissions must retain their actual names and cannot hide a selected module.
Expensive qualification and elapsed/RSS screening remain separate later gates.
