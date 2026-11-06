# PyValue and register-interpreter checkpoint closure

Coordinator plan, 2026-11-05. Runtime starts at `e1229448be3` on
`boil/pyvalue-rc-universal`; canonical documentation is in this checkout.
This packet resumes the existing work rather than replacing it. The coordinator
owns interfaces, assignments, commits, merged qualification and measurement.

## Outcome

Deliver the universal eight-byte PyValue register interpreter, with the JIT off,
and measure it against upstream `3a440b65695` and interim RC `eba351fee7d`.
The accepted R27 requirement is zero boxing attributable to core operations on
the declared kernels and regression-suite run, with explicit boundary accounting.
A normal lint pass or a few zero-box kernels is insufficient.

The checkpoint report must distinguish implemented, independently reviewed,
integrated, qualified and measured work. It must retain regressions and unknowns.
Stack-bytecode serialization recovery and the new method/tracing JIT follow this
checkpoint; old collector experiments remain closed.

## Starting evidence and preservation

- The runtime worktree is clean. W2b, W4b, W4c and UUID have landed.
- `w5` preserves 28 modified files at the runtime base, including staged math
  changes. `rfix` preserves nine modified files at the same base.
- G2 A4, B3 and control remain uncommitted at older `a1ecc0eb4f3`. Their timing
  exists; neither is integrated. Old conflicted trials are not runtime conflicts.
- Record each packet's base, full staged-plus-unstaged diff, touched files,
  checks and remaining limitations. Do not reset, clean or overwrite old work.
- Reuse existing build directories, the development-header sysroot and shared
  caches. External runtime/build writes require sandbox escalation, justified by
  the user's authorization to resume this work. Record any actual rejection.

## Ownership and first dispatch

There are three worker slots besides the coordinator. Reuse workers as packets
finish; keep the queue explicit instead of launching more concurrent workers.

| Packet | Initial owner | Scope | First deliverable |
| --- | --- | --- | --- |
| K: W5 kernel closure | Terra, medium | Existing `w5` changes; finish and validate all 29 kernels, beginning with pickle, regex and XML | Final-diff/build audit and file ownership within 10 minutes; first reviewed kernel increment within a 60-minute initial budget |
| F: required review fixes | Luna, medium | Existing `rfix`: thread aliases, mmap/CJK overflow behavior; document posixshmem decision | Focused tested patch within 30 minutes; optional sizeof changes excluded from required patch |
| R: independent closure review | Sol, high | Gate completeness, G2 choice, shared migration rules and later ready patches | Blocking findings and G2 recommendation within 20 minutes |
| I: integration and evidence | Coordinator | Shared runtime/builds, source identities, plan/status, integration gates, benchmark window | Review incoming packets promptly; integrate useful increments |

Budgets are reporting and scope-cut deadlines, not promises of completion.
Workers report actual start/deadline, update progress about every 20 minutes and
return a bounded ready increment before taking more work. They do not commit.

## Execution sequence

### 1. Rebuild the preserved state and agree interfaces

K inspects source newer than its last progress note, rebuilds the actual W5 diff
and reports current kernel counters. R reviews the existing approved W5 rules:
value-native calls, explicit ownership, no local recreation of marked object
helpers, no box-then-wrap workarounds, generator changes at the producer.

R audits `check_r27.py` and static lint for fail-open paths: ownership rows,
unmarked object twins, direct object-slot calls, excluded test units, unresolved
dynamic attribution, crash dumps and subprocess coverage. Agree repairs before
bulk migration. Enumerate legitimate compatibility boundaries; do not relabel
core boxes to make the gate pass.

### 2. Land the required review fixes

F verifies the preserved thread/mmap/CJK changes and regression coverage using
release compiles and narrowly selected module tests. R independently reviews the
actual diff and evidence. The coordinator applies only the approved files/hunks
to the clean runtime and runs the landing check.

Accept upstream Clinic argument wording for posixshmem if the audit confirms
the reported differences are only parser checks/messages; record the decision.
Treat exact `object.__sizeof__` quirks as an optional compatibility issue. Preserve
the drafted change separately and do not make it a checkpoint blocker.

### 3. Land the W5 kernel increment

Finish every declared kernel, including the tail groups. Validate changed code
with release builds and corresponding module tests. Re-run all 29 kernel box
counts after the final diff. R checks ownership, callbacks, error exits, subclass
dispatch, result identity and the counters. The coordinator integrates the
reviewed increment, then repeats the kernel gate on the merged stats build.

### 4. Complete the bulk sweep in bounded batches

K supplies a fresh compiler-driven census and freezes explicit file ownership.
The historical estimate is about 1,200 sites; replace it with actual remaining
errors. Reuse the approved mechanical processor before assigning hand work.

| Batch | Worker preference | Boundaries and checks |
| --- | --- | --- |
| AST generator | Luna medium | `Parser/asdl_c.py` and its generated output together; regenerate, release-compile and parser/AST tests |
| Compiler/parser residue | Terra medium | Explicit file list disjoint from K; compile and compiler/AST tests appropriate to the changes |
| Core/runtime residue | Terra medium | Explicit file list and ownership recipe; compile plus relevant container/call tests |
| Module residue | Luna medium for mechanical patterns, Terra medium for ownership changes | Small explicit module groups; compile and each affected module's tests |
| Decimal core | Terra medium; Sol only for a specific design/debugging problem | Value-native core with approved object-form leaves; decimal tests and subclass/error probes |
| Gate enforcement | Terra medium, independently reviewed | Static checks with representative positive/negative fixtures; dynamic accounting remains coordinator-owned |

Use the two implementation slots after F finishes; retain an independent review
slot. If R finishes early, free its slot for an approved mechanical packet and
bring it back when review is needed. Shared headers and generated files have one
owner. A worker stops at cases outside the recipe and returns them for a shared
decision. Never run full suites per file batch.

### 5. Defer G2 until the merged baseline is measured

The old timing gives base/candidate geomeans of 1.0100 (A4) and 1.0119 (B3),
with per-kernel variation larger than their separation. Independent review
recommends deferral: neither route has a robust measured advantage, and both
would need validation on current HEAD. Preserve all three trees; the checkpoint
retains the run-time slot-class test. Class-bit encoding is a follow-up option,
not the current format. Compare the candidates after the merged baseline is
measured, using benefit, state clarity, encoding cost and merge risk.

### 6. Qualify at integration points

Each landing regenerates affected products, compiles debug/release/stats, runs
the lint, then the packet's tests and the fixed smoke set. The coordinator checks
actual exit statuses and logs: the legacy scripts can reach `done` after a failed
test or lint command. A log footer alone is not a pass.

After a coherent batch, run full release/debug regression suites once. Attribute
failures to the batch with focused reproducers or bisection. Run the expensive
stats-suite R27 gate on the completed migration, not after every small port.
Fix and re-run failed portions; repeat the full integration gate when required
by changed core behavior or unresolved coverage. Retain all failed attempts.

Checkpoint admission requires:

1. Reviewed commits integrated on one clean runtime source identity.
2. Debug, release and stats builds include expected modules; no silently skipped
   extension hides unfinished source.
3. Static R27 and all 29 kernel checks pass; the declared suite has zero core,
   core-pending or unresolved boxing, with complete subprocess/crash accounting.
4. Full debug/release outcomes are green or any genuine environment limitation
   is explicit and independently reviewed. No test bypass substitutes for a fix.
5. Designs, stage/workstream status and handoff distinguish built behavior from
   deferred work, including G2, serialization recovery and JIT interfaces.

### 7. Measure the admitted candidate

Freeze the candidate source and builds. Verify version, imported stdlib and
extensions, flags, allocator, shapes, JIT-off state and `pybuilddir.txt`. Use
matched GCC release configurations without adding PGO/LTO to one side only.
Reuse upstream and interim controls if their source/build identities remain
valid; rebuild only where required. Record configuration differences explicitly.

Stop worker builds and tests for the timing window. Run three independent
rounds over the unchanged 29-kernel roster, rotating upstream/interim/candidate
order. Retain raw samples, medians, extrema, spread and sampled host load;
use a predeclared load guard of 4 and retain any rejected attempt. The initial
window is 45 minutes; if it cannot produce valid comparable data, record the
specific obstruction rather than choosing favorable samples or changing rows.

Then measure the existing pyperformance subset and selected micro diagnostics,
separately from the kernel aggregate. Measure RSS outside timing after warming
every build's executable and extensions into the page cache. Report anonymous
and file-backed memory where relevant. Collect boxing/dispatch counts and sample
the largest remaining regressions outside the elapsed-time window. Hardware
instruction counters remain optional if host permissions still block them.

Report candidate/upstream elapsed time per kernel (below one favors candidate),
the geometric mean, interim comparison, memory and the largest regressions.
Explain practical limits; do not attribute the total gain to one patch.

### 8. Follow-up plan and handoff

Rank follow-ups by measured cost, expected benefit, implementation risk and
smallest useful experiment. Include stack-bytecode serialization recovery and
the shared optimizing IR/method/tracing JIT, but let the measured runtime costs
set the immediate optimization order. Publish the checkpoint report, raw result
paths, updated designs/status and a bounded next queue in the repository and
the Loom plan artifact. Stop at the checkpoint recap requested by the user.

## Current assignment log

- K: `/root/w5_finish`, Terra medium; Decimal conversions, paired value slots
  and a reviewed selective Clinic method interface in W5.
- F: `/root/review_fixes`, Luna medium; compiler census and bounded ports for
  the 15 manual API pairs in the isolated bulk tree.
- R: `/root/closure_review`, Sol high; startup-box debugging and independent
  design/source reviews. The complete-inventory audit remains open.
- I: `/root`; combined runtime landings, qualification and measurement.

## First reconciliation

- Terra rebuilt the preserved W5 diff: release checked 117 modules with none
  missing or failing, and 4,290 focused tests passed. A fresh all-29 stats run
  reports zero differenced boxes. Sol is reviewing the full preserved patch;
  this does not yet establish startup-inclusive R27 kernel admission.
- Sol found fail-open static, roster, suite-return-code and attribution checks.
  Terra now owns their repair and focused negative tests in a separate increment.
- G2 is explicitly deferred as recorded above. The current design now labels
  the class-bit format as a candidate rather than the running interpreter.

## Integration log and current queue

- `f84d73164fa`: required review fixes landed; three builds, lint and 4,202
  focused/smoke tests per debug/release build passed.
- `98a26bb6218`: reviewed preserved kernel paths, repaired gate and first
  startup ports landed; three builds, lint and 8,826 tests per build passed.
  Two initial pickle/range review concerns were challenged and withdrawn;
  a real borrowed-array lint violation was corrected before landing.
- `4b2a6926d2a`: AST/marshal/two-site I/O landed; three builds, lint and
  5,937 focused/smoke tests per debug/release build passed. The AST port's
  real error-path double-close was fixed
  and exercised by the existing seven-case `test_none_checks` sequence.
- `58db4324fec`: reviewed I/O audit, LOAD_NAME, SystemExit and OSError
  value homes landed; three builds, lint and 5,561 tests per debug/release
  build passed. The first landing command wrongly named `test_fileio`;
  the corrected list uses `test_io`, with both profiles rerun successfully.
  Windows paths received source review only. A per-attempt `--dumps` option
  preserves stats evidence from earlier integration attempts.
- `e983958bede`: 17 direct-slot ports, their checker, the mechanical sweep and
  the first two Decimal helper increments landed in 139 files. Three builds,
  lint and 9,511 tests per release/debug build pass. Review fixed a real float-ratio box and
  replaced chain's object Clinic slot wrapper with a true compatibility thunk.
  The claimed bool-box issue was challenged and withdrawn: bools are object
  references. Stored-pointer slot invocations remain a separate open audit.
- The coordinator owns a separate mechanical-sweep worktree under this
  checkout's ignored `build/` directory, based on `98a26bb6218`. It has 48
  reviewed safe rows (29 surveyed pairs and 19 scalar extractors), with nine
  structurally incompatible pairs explicitly recorded as manual metadata.
  A real compiler census found 475 errors/465 classified sites; the existing
  processor converted 447 of 465 classified sites. Provider includes reduced
  the next compile to 23 errors; Terra is fixing aliases and generator
  producers before review. The isolated unqualified snapshot is
  `abf97121249`, followed by compiling candidate `f65d64a3987`. The latter
  includes the nonconflicting `4b2a6926d2a` merge and residue repairs. Its
  main-runtime merge retained the newer value-native exception consumers.
  Integration also refreshed the SQLite arraysize setter from its producer,
  removed obsolete numeric opt-ins, and repaired Length aliases that had
  called other compatibility entries. No global new marks have been activated
  in the working W5 source.
- The isolated worktree now starts at qualified `e983958bede`, with its later
  manual API edits preserved. The task stash
  `e4cf1a9101556e1c296374457a1e6a1c32590589` remains recoverable. Fifteen
  manual API marks are intentionally confined to this tree until callers are
  ported. The original 459-diagnostic census is an upper bound, not a new
  compiler count at this base. First packets: LZMA/subprocess/atexit (505
  focused tests) and time/random/faulthandler (237), independently reviewed
  and awaiting coordinator integration.
- `4fffee855c9`: 12-file startup/selective-Clinic/Decimal increment landed;
  three builds, lint, 483 Clinic tests and 6,649 tests per release/debug
  build pass. All 65 Decimal macro-family methods are value-native. Manual
  methods, constructors, slots and the final module-wide gate remain open.
  Reviewed Context manual/string/power/canonical layers are queued; workers
  continue the next bounded families while the coordinator freezes builds.
- `f81ebbd646d`: 10-file Decimal/manual-call integration landed, with three
  builds, lint and 6,001 tests per debug/release build passing. It includes
  Context manual/string/power/canonical methods; instance sign, 19 further
  unary/predicate methods, quantize/integral and string/classmethod entries;
  LZMA/subprocess/atexit, time/random/faulthandler and CJK callback ports.
  The unavailable marks for the broader manual API census remain isolated.
- A no-op Linux startup at this HEAD records zero boxes. The first kernel
  raw sample still has 237 CORE boxes: compiler 179, marshal tuple 49, slice
  field 1 and frame keyword values 8. Exact PCs and source attribution are in
  `benchmark-parser-receipt.md`. The first all-29 attempt rejected an identical
  repeated metadata flag; the parser repair permits only repeated
  `opcode[...].specializable: 1` and still rejects duplicate counters.
  The repeat exposed missing handler-coverage metadata and is under review.
- The fresh full-suite stats census at frozen `4fffee855c9` finished under
  `checkpoint-closure/suite-4fffee855c9-census`: 54,270 tests, 31 failures in
  eight files and 6,326 dumps. Gate debt is 50,463,865 with accounting defects;
  see [census and next actions](fullsuite-4fff-census.md). This is triage,
  not final admission; no comparison with an older survey claims equal coverage.
- H3 full-domain instrumentation is now in merged validation after `f81`.
  The old arrays also caused the reproduced allocator-debug startup crash:
  `UPDATE_MISS_STATS` indexed handler 306 into a 256-entry array. The reviewed
  producer expands both axes, generates flags/bases and counts actual entered
  handlers. Rebuilt crash probes and strict reader validation remain required;
  metadata alone does not establish coverage. A separate incremental patch
  aligns the specialization family API and adds the crash regression.
- Next worker queue: Terra owns the exact-base value GCD provider, coherent
  Decimal round/int/ratio consumers, selective constructor generation and
  remaining constructor/Context/slot closure. Luna owns the three deque saved
  iterator calls, then remaining collection calls and itertools value producers,
  followed by bytes-iterator and math/bisect consumers. Sol owns H3 producer/
  reader alignment and critical debugging. Full-suite tail and inventory
  completion are still required; do not declare R27 complete from lint alone.
- A combined worker sample reduced richards low-process startup CORE boxes
  from 238 to 16; THUNK remains 19. This is composition evidence, not current
  runtime admission or a patch-specific effect. A later combined sample
  (`w5-out/oserror-richards.json`) has four CORE and zero other boxes:
  three file-descriptor projections in `_io_open_impl` and one dictionary
  projection. Complete static/suite gates and all fresh kernel raw counters
  remain required; `_decimal`, manual API ports and suite tail are open.

## Live integration queue after H3

- **Committed:** `d6eae4b1c1c` fixes full handler-domain instrumentation,
  strict reader metadata and calibrated counter fixtures. Three builds and
  merged landing checks pass. The allocator-debug reproducer is fixed.
- **Frozen in full qualification:** 21-file compiler unary/cache, marshal
  writer and frame keyword ports; exact-base GCD and Decimal constructor,
  conversion, rounding, numeric slots and tuple sequence paths; deque,
  bytes iterator and fileno consumers. Regeneration, lint and three builds
  precede full release/debug suites. The first attempt is retained; the
  separate creator-allowlist repair attempt is the active one.
- **Terra medium:** finish shared Context setters and selective initializer
  adoption; getsets and paired custom attribute slots are frozen for review.
  Finish remaining format/converter paths, then enable the full Decimal core
  mark and return the actual compiler census. Use focused Decimal/Clinic and
  callback probes, not another full suite.
- **Luna medium:** six itertools iterator producers are frozen for independent
  review. Next are coherent fsum, bisect-right and math.integer gcd/lcm value
  consumers. Keep each ownership family in a separate reviewable packet.
- **Sol:** review iterator and Context/Clinic contracts, trace Rational callback
  order to the actual upstream base, and audit stats-suite process accounting.
  Resolve concrete fail-open findings rather than accepting aggregate totals.
- **Coordinator:** integrate approved packets only, preserve worker layers,
  finish full functional qualification, and recount all 29 raw kernel samples.
  Refresh the finite direct/indirect slot and ownership inventory, then use a
  fresh full stats census to rank and close the remaining core-owned tail.
  No internal compatibility exit may conceal a core operation's boxes.
- **Measurement:** after final admission, stop worker builds, freeze source
  identities and run the interleaved upstream/interim/candidate kernel and
  pyperformance comparison. Publish per-kernel time, spread, memory and
  explanatory profile breakdown. Rank the next plan from that evidence;
  historical 1.73x/1.83x ratios do not describe this candidate.

## Current handoff at 6aec17eefeb

The 28-file cold-path/Decimal/iterator increment is committed. Full release
and debug runs exercised 54,289 and 53,808 tests, finding fixture mismatches
and a debug profiler NULL-to-steal assertion. Separate reviewed fixes passed
6,291 focused tests per build; three builds, lint and generation pass. Final
full suites remain required after the remaining migration. The new complete
stats census is running frozen under `suite-6aec17eefeb-census`, with bounded
invalid-owner diagnostic samples retained before dump deletion.

Next integration order: approved binary compiler fold; original six itertools
producers plus their additive review corrections; Decimal richcompare,
getsets, attribute slots, selective initializer/shared setters/Context adoption,
EXTRA getsets, integer methods, formatting/allocation, Context ownership and
v2 lifetime regression, then final miscellaneous/reduce/core-mark layers.
Keep the exact recorded prerequisites and review receipts. The original
subprocess lifetime fixture is rejected for indentation; integrate v2 only.
Math/prod release-order and cached bisect value-slot fixes remain worker-owned.
Luna then owns `_abc`, StoreName and mappingproxy; Terra takes long-range
iterator value homes after Decimal. Sol audits complete ownership/provider
and indirect-slot coverage and investigates exact stats accounting defects.

Eight kernels in the predecessor capture are zero in every raw process; all
29 loop deltas are zero. This is progress, not admission. Replacement controls
`upstream-release-devlibs-3a440b6` and `interim-release-devlibs-eba351` now
load C Decimal/SSL/SQLite and preserve the old builds. They are the planned
measurement controls; final freeze must verify actual stdlib/module imports.
