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

## First matching native checkpoint

The reviewed core is committed as d462e8338ce on isolated boil/h3-stats.
A fresh owned build/pystats configured with --enable-pystats CC=gcc and made
with -j4 all under ordinary global admission/build locking. Both commands
returned0; complete products/config/compiler/source were frozen before tests
and rechecked after gate cleanup. Actual configuration is Py_STATS1,
nondebug/GIL/non-tail GCC/O3 with retained FP/zero-init/fork eval policy.
Checksharedmods reported116 checked and0 failed imports. Bootstrap/build
checks establish products; they are not a broad qualification suite.

The first bounded target transaction ran21 tests: the new legitimate high
handler test and all15 reader tests passed, but five legacy lifecycle tests
failed because they read only stderr and the producer used existing shared
/tmp/py_stats. Its failure remains in build/H3-1-stage2b3/tests-result.json;
no remaining generator cases ran. A task launcher import failure before that
transaction ran no target and is retained separately.

A reviewed test-only helper correction at b39cc421b19 read private output
files. Five of six stats tests passed; dump failed because the helper wrongly
required equal snapshots. Actual sysmodule.c _stats_dump prints then clears,
so those files represent disjoint intervals. The failed attempt is retained;
it is not a native counter failure or a passing module. The final helper
896d4c4cee5 sums interval counts and rejects missing/duplicate fields. Root
reviewed its source bridge from d462 (only test_pystats.py changed) and granted
only the remaining dump test and previously unexecuted five generator/domain
cases:1/0skip and5/0skip pass. Other passing stats/reader receipts are retained
without repetition. Actual verifier state was off/extras0/stress0. Original
complete native/config/tool maps remain identical after final cleanup.

Private fixture/driver source is now prepared for review, not executed.
It consumes exact current/historical counter spans, the actual generated
LOAD_CONST predicted span, and renamed historical C typedefs. Planned numeric
controls cover full-domain rows/both pair axes, disabled-phase local advance,
clear and C-local activation lifetime, normal predicted bypass, actual
sizeof/RawCalloc and old-cutoff/counter-order defects. A host-only copied old
emitter uses the existing per-operation/NOFB oracle. Neither the C model nor
source inspection alone qualifies real frame transitions or tailcall runtime.
The native request/size and these controls remain pending; +230,368 remains
arithmetic until their matching compiled output exists. Full union, broad
modes and performance gates remain separate.

## Private controls: actual bounded outcome

Control source froze at b1ce242f584. Its first positive compile failed before
any target probe because pycore_stats.h's _PyThreadStateImpl declarations need
actual pycore_tstate.h first. This is retained under
build/H3-1-stage2b3/counter-controls-compile-failure-v1 and its adjacent log;
it receives no counter/negative-control credit. Root reviewed the exact
compiler-input repair: include that header and the existing Makefile mimalloc
include path (two control files,3 insertions/1 deletion). It is committed as
17fbcacea87; no interpreter rebuild or native-source change occurred.

The corrected three variants each compiled and returned normally once.
Current macro passed all numeric entry/pair/off/clear/C-local/predicted checks.
The copied historical 256 cutoff failed exactly nine named entry/pair/local
assertions; moving the actual generated counter after its predicted label
failed exactly predicted.entries and predicted.self_pair. Neither negative
uses compilation failure, malformed code or execution of synthetic handlers.
The separate host copied inst.name emitter failed the existing per-operation
and NOFB oracle (68 subtest failures, no errors). Actual native mode for the
three probes was off/extras0/stress0 on the matching non-tail Py_STATS build.

Actual compiled sizes are PyStats895,544 bytes and renamed historical
PyStats665,176 bytes; OpcodeStats2,960 and historical2,592. The measured layout
difference is230,368 bytes. One actual RawCalloc request for895,544 bytes
succeeded and was zeroed. This measures C layouts/request success, not RSS,
old-process allocation behavior or memory cost of extra interpreter instances.
Production still has one stats aggregate per interpreter. The primitive
same/nested C-local activation checks do not execute actual frame transitions;
normal predicted control is not tailcall-runtime qualification.

Evidence in this owned tree:

- build/H3-1-stage2b3/build-result.json and post-cleanup.json: fresh native
  d462e8338ce build/config/compiler/full product closure.
- tests-result.json and tests-v2-result.json: both actual failed test attempts;
  tests-v3-result.json and tests-v3-post-cleanup.json: only the granted remaining
  dump and five generator/domain cases, with unchanged native/config/tools.
- counter-controls/plan.json: corrected frozen plan d76d4919487ac0aa40e5a0e6e6f276e0af0396122490b49caf9545df282ba327.
- counter-controls/result.json: dd540696bff2c6d630559e3522cc668e1516b4efae273bebeda39e31bec104c9;
  raw per-variant compile/probe logs and extensions are beside it.
- counter-controls/post-cleanup.json: 7bed4e6022bfc60e2067fc11308ebf84a2ae1a35d2ddf1cbecc358e2591d000e.

Post-gate cleanup verified the same clean17fbc source, complete original
product roster/bytes/aliases, configuration and compiler/tool bytes. Native
sources remain exactly d462 throughout the test/control-only bridges. All
receipts remain qualification=False: this is a coherent branch-local stats
checkpoint, not broad-suite, union, landing, actual-frame, tailcall or
performance qualification. Full-domain producer/reader live-data closure,
actual-union regeneration and the bounded union gates remain review items.
