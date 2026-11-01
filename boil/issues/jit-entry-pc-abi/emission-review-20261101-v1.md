# By-value entry ABI: offline machine-code result

The fresh release image for source `599897b6b5d0bb973b1eea12f8eb41957004b464`
recovers a register-resident IP on the inspected ordinary handler paths. All 14
entry calls remain, and both debug/release raw witnesses retain the exact same
94-byte automatic and 234-byte pressure leaves. This confirms the targeted
machine-code mechanism on these paths. It establishes no elapsed benefit or
cause of the previous campaign's losses.

The release build receipt pins image SHA256
`2cc78bf8b2d5894bbd30a8f8b409656db10bb90dbfd60bd3c316991d8eb80466`.
The pinned PIE reader and objdump retain the full evaluator range
`0x2a1300..0x2ca9b2` (169650 bytes) and TryEntry
`0x33f160..0x33f3ef`. No separate named evaluator partition is present. The
historical parent evaluator is 169325 bytes and rejected evaluator 173097 bytes;
these sizes are diagnostic only.

| Handler | Parent entry | Rejected entry | Fresh entry | Fresh continuation evidence |
|---|---:|---:|---:|---|
| LOAD_FAST_BORROW | 0x2b01a3 | 0x2a8712 | 0x2b2942 | `lea 8(%r15),%rbx` at 0x2b2956; `mov %rbx,%r15` at 0x2b29b7 before dispatch |
| STORE_FAST | 0x2b4538 | 0x2a52cc | 0x2b19d8 | Advance at 0x2b19ea; dispatch transfer at 0x2b1a88 |
| FOR_ITER_RANGE | 0x2b229f | 0x2a7004 | 0x2ad22e | Advance at 0x2ad240; dispatch transfer at 0x2ad2e7 |
| UNPACK_SEQUENCE_TUPLE | 0x2a8f91 | 0x2ab515 | 0x2a9fd6 | Guard 0x2aa026 reaches fast body 0x2b836d and dispatch 0x2b83cb..0x2b83dc using rbx/r15 |
| UNPACK_SEQUENCE_LIST | 0x2b358f | 0x2ab435 | 0x2b1429 | Matching list body reaches dispatch 0x2b14ec..0x2b14fd using rbx/r15 |

The tuple immortal-element branch at 0x2b83a8 reaches 0x2bbcc1 and returns
to the same loop/dispatch. These inspected successful paths no longer repeatedly
load/store the rejected image's IP home at `-0x160(%rbp)`. Frame and slot-base
loads from stack homes remain; their removal is not claimed. Error, deallocation,
type-mismatch and every other handler path are not presented as fully audited
CFGs. Full named evaluator assembly is retained for further branch inspection;
the five prefix files are explicitly incomplete CFGs.

All 14 call windows are saved in `release/all-14-entry-calls.asm`. Each passes
the current PC by a register move into rdx, then tests the returned rax and
conditionally transfers it into the continuing PC register. For example,
0x2a17be passes rbx, call 0x2a17d3 returns to test 0x2a17d8 and
`cmovne %rax,%rbx` at 0x2a17db. This replaces the rejected call's IP-local
address argument (`lea -0x160(%rbp),%rdx`). Other stack addresses in the full
evaluator remain legitimate unrelated uses; a blanket no-address-escape claim
is not made.

The original offline reader failed before objdump because it counted only
`&&TARGET_` labels: there are 289 such labels and 13 unknown-opcode slots.
`offline-v2.py` preserves the original and counts all 302 initializer slots.
`historical-attribution.json` applies that complete mapping to both historical
images and verifies every previously nominated handler address unchanged.
The fresh and rejected complete captures use this corrected mapping. The
no-prefix parent has no TryEntry; its existing complete capture is reused and
its complete table independently decoded, rather than forcing the 14-call
adapter onto it. The failed parent adapter and original reader remain recorded.

Both actual focus logs contain one automatic and one pressure witness. Complete
code bytes equal the frozen leaves (automatic SHA256 `6eaf1f20...`, pressure
`26869ad8...`), so their existing complete binary disassemblies also apply.
Automatic witnesses report success_seen=1, zero spills and an RX/non-W range
covering the mapping; pressure witnesses report six spills and explicitly
describe a mapping already freed before fixture return. Witnesses do not prove
workload frequency. Whole-cohort behavior, provenance and cleanup acceptance
remain the independent outcome reviewer's responsibility. Root reported actual
terminal exit 0 and cleanup; this report adds no target or compiler invocation.
