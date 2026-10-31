# Bytes-writer root checkpoint review

Coordinator review, 2026-10-29 16:09 PDT. Verdict: bounded source and actual
results clear; integration remains pending. The reviewer did not author this
change. Candidate native source is f5e818b0454764be06968b463b600820553ccbe3,
based on ce2ac8991eebb1bbb10698ff2d07a79bd4cba452. Documentation successor
08953c89274 changes no native source.

The private writer gains an embedded root for its stable owner slot. Creation
registers after initialization and before buffer allocation. Discard and the
common successful finish tail each unlink before freeing writer storage;
initial allocation errors reach discard. Existing resize protections remain.
The finish tail's RemoveRoot lock/unpark and MEM-domain free are noncollecting
under their current contracts, so a redundant allocating handoff is unnecessary.
Public writer storage is opaque; private consumers rebuild. This review does
not extend that proof to arbitrary locks, Unicode writers or a moving GC.

The additive VARARGS fixture uses a distinct thread root to protect diagnostic
objects and checks the INTERP root class. Conservative survival cannot satisfy
that assertion. Returned objects enter a fixture root before diagnostics or
collection. Both bytes and bytearray exercise heap creation, promotion, growth,
oversized errors, finish/content and cleanup. The omitted-registration copy
removes Add and both Remove calls together: both new cases fail with the named
`create: writer owner not registered` assertion, rather than a node crash.
Discard checks exercise freed-node cleanup but do not independently prove
arbitrary cross-thread or cross-interpreter lifetime behavior.

Actual receipt index is `build/gc3a-bytes/checkpoint-summary.json` in the owned
bytes tree, SHA256 dc428e4b95235ae67e9bfffbcc6a5aa368274c41ab1d8ac9a8a858d364ab4214.
The reviewer verified all seven check receipts, four build receipts and every
artifact hash in the check receipts, and read the negative JUnit assertions.

| Check | Cases / skips | Result |
| --- | ---: | --- |
| O3 release, full bytes module | 52 / 10 existing debug-only | PASS |
| O1 debug, full module, actual kill | 52 / 0 | PASS |
| Release, writer classes, stress1000 | 38 / 10 | PASS |
| O1 debug, writer classes, kill/stress1000 | 38 / 0 | PASS |
| Owned ASan debug, writer classes, kill/stress0 | 38 / 0 | PASS |
| Independent omitted-registration build | 2 / 0 | Expected assertion failures |

All completed positives have unchanged native product maps and zero reported
fallback/unpinned hits. ASan disables fake stacks and leak detection; no leak
proof follows. The attempted release-kill startup is preserved as invalid
configuration with zero tests because kill requires Py_DEBUG. Earlier positives
and the negative used task-private admission slots; their correctness results
remain with that limitation. Stress/ASan used global admission. Repeating them
solely to erase this history would add no useful evidence.

The exact ROOTS eligibility successor and primary must still compose, with
private-header/CAPI review and fresh default/debug/kill integration checks.
One merged 2b+3a ladder is sufficient under the coordinator's scoped exception.
No general PyMem mitigation, global zero-initialization, conservative scanning
or verification exclusion is retired. The inherited HTTP/finalizer-progress
debt remains a final GC/performance blocker.
