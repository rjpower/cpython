# Full-suite census at 4fffee855c9

This is a failing mid-integration census, not checkpoint qualification or a
performance measurement. Source and shared binaries stayed frozen throughout.
The stats runner exited 1 because its gate failed; regrtest itself exited 2.
The attempt is retained under the external build root at
`pyjit-link-wave1/checkpoint-closure/suite-4fffee855c9-census/`.

The run took 7 min 54 sec and reported 54,270 tests, 31 failures and 2,428
skips. Eight files failed: OS, opcode stats, C API, command line, embedding,
GC verification, pystats and the box-site tests. There were 6,326 folded
dumps, no malformed dumps and no dropped owner records. Accounting remains
incomplete: 41 invalid owner-attribution errors, 598 leftover run markers,
685 recorded launches, two resets and newly uncounted test tags. Existing
reviewed expected-termination exceptions do not explain all markers.

The gate reports 50,463,865 boxes at core/core-pending/unresolved owners,
including core-owned thunk and adapter exits; 23,917,323 boxes have the
literal `core` kind. These are different quantities. The accounting defects
and crashes prevent treating either total as complete. Do not compare this
to an older survey as a measured improvement with identical coverage.

The [generated report](fullsuite-4fff-census-generated.md) retains the site,
kind, ownership and top-symbol breakdown. Leading actionable owners include:

| Site or owner | Reported boxes | Next work |
| --- | ---: | --- |
| bytes iterator conversion | 6,389,976 | Value-native iterator items and byte extraction |
| itertools core release site | 6,010,360 | Resolve the PC to producer/consumer paths; paired value iterator slots |
| deque known-object transfer | 3,786,971 | Remove cached object iternext calls in extend/consume |
| property setter | 2,459,596 | Audit paired descriptor set/value handling |
| ctypes scalar setter | 2,367,946 | Audit value getset/field continuation |
| struct argument projection | 1,812,714 | Port actual parser/packer continuation |
| math.fsum | 1,665,186 | Keep yielded numeric items as values |
| bisect-right saved slot | 1,276,610 | Audit exact slot semantics before replacing cached calls |

Symbol names such as a decref or inline projection identify machine-code
attribution, not a sufficient source diagnosis. Port recipes must inspect
the enclosing operation and ownership before editing.

The startup crash is now diagnosed independently: with both
`PYTHONMALLOC=malloc_debug` and `PYTHONSTATS=1`, importing `test.support`
reaches `COMPARE_OP_JUMP_FLOAT`, where `UPDATE_MISS_STATS` writes
`opcode_stats[306]` into the old 256-entry array. The retained debugger log is
`checkpoint-closure/malloc-debug-triage/gdb-rebuilt.log`. H3 expands both axes
to the generated 319-handler domain; its rebuilt exact reproducer remains a
required check. Normal allocator-only probes do not exercise this defect.

The box-site failure has a separate stale negative control: list membership
now correctly makes zero boxes. The reviewed replacement explicitly calls
the test-only owning bridge; the counter threshold remains unchanged. The
stats family API also has a stale roster, handled separately from raw
per-handler accounting. Re-run the eight failed files on the repaired stats
build, triage any remaining failures, and retain this census unchanged.
