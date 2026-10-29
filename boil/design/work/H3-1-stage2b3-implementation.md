# H3 handler-entry statistics: first source checkpoint

This is branch-local development on `boil/h3-stats` from reviewed design
`81417aff0971fe621fec296a324c7a48315a6082`, in the managed
`.worktrees/boil-h3-stats` checkout. The coordinator explicitly authorized
this isolated exception to integration sequencing. Frozen H3, ROOTS and
primary native sources/products remain untouched. The future queue/ROOTS/H3
union still needs its own source reconciliation and qualification.

The source now sizes both counter axes by IR_NUM_HANDLERS, admits generated
dispatch entries and updates activation-local lastopcode even while aggregate
updates are off. Emission uses the actual target name before the existing
predicted label; semantic opcode, box and specialization sites are unchanged.
An ADAPTIVE property requires actual specializing DSL work and owned feedback;
specialized, instrumented and NOFB targets are excluded. Existing generic
metadata generators export that new bit to C and Python without new numbering.

The producer exports full named ID/base/flag facts, coverage version2 and
normal/tailcall mode. The reader validates every raw file or JSON before
aggregation, retains one identical metadata contract and rejects mixed
cohorts. Raw/family entry sums exclude events, A/S/O partition raw entries,
and semantic events retain their recorded names with explicit field selection.
The branch-local benchmark reader renames dispatches to
counted_handler_entries, preserves coverage/mode facts and validates both
samples before differencing. Historical JSON is untouched.

Host-only validation used `/usr/bin/python3` through uv, pinned to CPUs2,3:
15 reader/benchmark tests and five focused generator tests pass. These cover
100-not-190 totals, truth-hit attribution, guard misses remaining specialized,
NOFB/IR entries, CACHE-0 pair sources, mixed box/event fields, aggregation,
raw/JSON missing/duplicate/corrupt facts, mode conflicts, report rendering,
benchmark metadata/differencing, actual target names/predicted ordering and
adaptive property/export. The host's test.support/import_helper are loaded
before adding this checkout's test directory; this avoids importing its
3.16 lazy-import syntax into the 3.14 host. An initial host setup error and
an incorrect instrumentation test name were corrected; neither executed a
target interpreter. The three intended products were regenerated on the host;
opcode IDs and dispatch target tables have no diff. No native build/test ran.
The design-listed run_reader_controls.py also checks five copied defects:
overlapping family totals, miss subtraction, inferred truth attribution,
unfiltered event fields and erased duplicate JSON facts. Each fails the
intended assertion with no harness error; the 15 positive tests still pass.

This checkpoint requests source review before any native build. The added
Py_STATS functional test compiles legitimate per-operation, NOFB and relaxed
loop targets, witnesses their executing IDs, then checks raw entries/pairs
after clear; it remains unexecuted. Still required: a matching isolated
Py_STATS build, actual size/request evidence, primitive private-counter
lifecycle/disabled/clear/activation controls, copied old-cutoff/emitter and
normal predicted-label controls, live frame/call and stats/thread/exit checks,
and the bounded affected/regen/ID checks in the reviewed design. A real
tailcall Py_STATS build is needed for any tailcall-runtime claim. The
230,368-byte growth remains source arithmetic, not measured sizeof or RSS.
No native gate, landing or performance result is claimed here.
