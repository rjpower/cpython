# PyValue and register-interpreter checkpoint closure

Coordinator plan, updated 2026-11-06. This effort began at `e1229448be3`; the
current committed runtime is `6a0e4c12c17` on `boil/pyvalue-rc-universal`. Canonical
documentation is in this checkout. The live queue is the final section; earlier
queues are historical records. The coordinator owns interfaces, assignments,
commits, merged qualification and measurement.

Current user direction supersedes the original zero-box completion criterion:
keep GCC fixed, prioritize measured throughput and useful PyValue ports, and
retain the full R27 census as a diagnostic. See the
[narrowed slowdown summary](slowdown-summary-6a0e4c12c17.md) and live queue.
Correct Python behavior and trustworthy measured-workload accounting remain
required; a second compiler and an exhaustive zero-box sweep are not selected.

## Outcome

Deliver the universal eight-byte PyValue register interpreter, with the JIT off,
and measure it against upstream `3a440b65695` and interim RC `eba351fee7d`.
The full R27 box census is a diagnostic for identifying useful ports, not an
admission veto or required zero count. Require correct Python behavior and
trustworthy workload identity, event and accounting evidence for performance
decisions. A few zero-box kernels remain useful evidence but do not describe
whole-application throughput.

The checkpoint report must distinguish implemented, independently reviewed,
integrated, qualified and measured work. It must retain regressions and unknowns.
Stack-bytecode serialization recovery and the new method/tracing JIT follow this
checkpoint; old collector experiments remain closed.

## Initial takeover evidence and preservation (historical)

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

## Initial ownership and dispatch (historical)

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

## Original execution sequence (superseded by the live queue below)

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
3. Static/API findings have reviewed dispositions, and the declared workload
   has trustworthy subprocess/crash accounting. The full zero-box R27 census
   remains tracked as a porting diagnostic; residual boxes do not veto a
   performance decision when they do not compromise correctness or workload
   evidence.
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

Report upstream/candidate elapsed time per kernel (above one favors candidate),
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

## Historical integration queue after H3

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

## Historical handoff at 6aec17eefeb

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

## Historical closure sequence after the second full integration run

1. **Coordinator:** freeze the 38-packet Context/iterator/math candidate, retain both full-suite results (54,301 tests per profile), and correct the new bisect test's error-message assumption. Debug also exposed `_bisect` failing import because the slot-row validator was not exported. Export that existing debug contract, enforce the expected 117-module roster in the landing script, then rebuild and rerun bisect/value-call plus the compiler arithmetic probe. Land only after independent review and all affected checks pass.
2. **Coordinator:** integrate reviewed struct packing/provider, the value Unicode formatter after source review, property fields plus additive review corrections, and static inventory hardening plus its comment-stripping correction. Regenerate affected products, build all profiles, and run the corresponding narrow tests plus the integration smoke set. Property has syntax checks only until this merged runtime run.
3. **Terra medium, Sol design review:** fix ctypes foreign-pointer keepalive ownership, then migrate private scalar/field callbacks, constructors and consumers. The preparatory `b_objects` conversion is rejected: canonicalizing the sole raw owner can invalidate a pointer in foreign memory. Preserve its artifact. Solve raw FFI ownership, GC, sharing, replacement and allocation-failure semantics before admitting inline internal keepalive tokens. Normal mutable constructor dispatch is required; the existing value-preserving type-call tuple can carry inline arguments to a ported initializer.
4. **Luna medium:** after property, port cold `get_const_value`, deferred annotations and import attribute continuations from the latest raw-kernel PCs; return bounded proposals and minimal checks. Continue the manual API and indirect-slot census from actual compiler results rather than historical counts. Sol reviews ownership changes; the coordinator owns shared headers and integration.
5. **Coordinator and Sol:** after these coherent closures, run a new full stats census, resolve invalid-owner rows using the reviewed allocator-independent reporter fix, and finish finite child/fork/intentional-exit accounting. Rank the remaining core, core-owned thunk and adapter tail. Complete the API/provider inventory and indirect-slot audit before setting completion metadata. Current full stats at committed `6aec17eefeb` passes 54,290 functional tests but records 41,038,468 debt boxes; this is failing admission evidence.
6. **Coordinator:** repeat complete release/debug qualification and strict raw all-29 kernel/full-suite gates on the final clean source. At that candidate, the capture had nine raw-zero kernels and all 29 zero loop deltas; [the exact census](kernels-context-candidate.md) remains a failing gate. Do not count core-owned adapters as acceptable boundaries.
7. **Coordinator:** stop worker builds, verify the matched controls' source/compiler/options and actual standard-library/C extension imports, warm all binaries, then run three rotating interleaved U/I/C kernel timing rounds, memory measurements and the established pyperformance subset. Retain spread and host conditions, and profile the measured gap. Publish per-kernel ratios, aggregate time/RSS and limitations against upstream `3a440b65695` and interim `eba351fee7d`. Historical 1.73x/1.83x results do not apply to this source.
8. **Coordinator:** make the evidence-driven follow-up plan: interpreter cost and IR encoding choices (including deferred G2), remaining throughput/memory issues, serialization recovery, then method/tracing JIT. Keep accepted design, implementation status and deferred alternatives separate. Preserve old worktrees and sealed evidence throughout.


## Historical queue after runtime `812cba08f4b` (2026-11-05)

This section supersedes the earlier queues above. Keep their records as
history; do not treat their owners or branch identities as current assignments.
At update time the runtime tree also has uncommitted `_operator`, factorial and
accounting-gate candidate edits. They are not part of `812cba08f4b` and must
pass review and merged qualification before landing.

### Landed and verified

- `b9888ea3b40` carries values through imports, descriptors, audit hooks and
  saved iterator slots. The subsequent dictionary-constructor and `math.dist`
  packet landed as `812cba08f4b`.
- The `812cba08f4b` landing passed all three debug/release/stats builds with
  the expected 117 modules, lint, and 6,193 focused tests per release/debug
  profile. These are landing checks, not full-suite qualification.
- The capture labelled `kernels-b9888ea3b40` records 15/29 kernels with zero
  startup-inclusive boxes and zero loop deltas for all 29. The JSON embeds
  `7baf96ed017` dirty as the build identity, but the coordinator confirmed the
  captured source matches committed `b9888ea3b40`. Preserve this caveat; no
  matching ELF for the original PCs is available, so use the fresh capture for
  source attribution.
- The selected legacy-attribute-owner probe in
  `build/pyjit-link-wave1/checkpoint-closure/legacy-owner-b988/` ran
  `test_xxlimited` (12 tests). Its two dumps report zero invalid owners,
  accounting errors, strays, or leftover runs. This focused result verifies
  only that probe; it is not the full R27 census.
- The latest complete suite census remains the frozen `7baf96ed017` report:
  54,309 tests, 25,119,440 debt boxes, 586 unfinished markers, 423 uncounted
  launches, and one invalid owner. R27 therefore still fails. A new full
  census on the composed post-`b988` runtime is required.

### Next candidate packets

1. **`_operator` value calls:** source review is approved. Root is rebuilding
   the exact integration patch from the current runtime and isolated S files
   after a fuzzy test hunk was found in the earlier artifact. Integrate only
   the reconstructed patch; run merged `test_operator` and appropriate build
   checks before marking it landed. The runtime test has not yet been run.
2. **Factorial continuation:** focused differential evidence exists in
   `r27-factorial-receipt.md`; source integration, independent review and
   merged qualification remain outstanding.
3. **Accounting gate:** finish normalization and process-accounting repairs,
   retain exact-shape and negative evidence, and independently review the gate
   before counting a passing result. Then run a fresh full stats suite on the
   exact clean runtime and resolve every invalid owner, missing child/process,
   crash dump and unfinished marker.
4. **Final kernel and functional qualification:** after the preceding packets,
   recount all 29 startup-inclusive raw processes and loop deltas; rebuild all
   three profiles with the full expected module roster; run full release and
   debug suites. R27 admission requires the complete declared test suite to
   have zero core, core-owned adapter, core-pending or unresolved boxes, with
   complete subprocess and failure accounting.
5. **Performance and follow-up:** only after admission, freeze source and
   controls and run the matched interleaved upstream/interim/candidate timing,
   pyperformance subset, RSS and profiling window from step 7 above. Current
   performance is unknown. The 1.73x kernel and 1.83x pyperformance figures
   are historical measurements of `5de7bd62ef9`, not this source. Rank the
   subsequent interpreter, memory, serialization-recovery and JIT work from
   the new measurements.

Ctypes and LRU implementation are pending explicit approval after automatic
review; do not bypass that hold. G2 remains deferred until after the qualified
baseline measurement. Keep landed packets distinct from the three candidate
packets above and do not claim checkpoint completion until all gates pass.

## Historical queue through runtime `42696e9fcca` (2026-11-06)

1. **Coordinator:** the 15-file operator slot
   correction, formatting/I/O, direct `__new__`, dict getitem and SQLite
   binding/adaptation batch. Independent source reviews and worker module
   checks passed, all three profiles contain 117 modules, and 6,684 tests per
   profile plus actual C object-call/lifetime probes passed. It landed as
   `66981c963be`; regex/getattr followed as `357baeea402` with 4,633 tests per
   profile. Type assignment followed as `42696e9fcca` with 4,499 tests per
   profile and upstream probes. Next integrate coherent process accounting
   stages and paired value construction. Both preceding
   full functional runs had 54,311 tests and one operator crash; preserve them.
2. **Terra medium, bulk ports:** regex, getattr and type assignment are landed.
   Implement the reviewed new-only paired constructor increment, then its
   mandatory generic type-call/init continuation. Continue ranked
   call/constructor/descriptor families in bounded packets.
   Worker checks are release compilation and relevant small tests; bulk suites
   belong to the coordinator. The API/provider and copied-slot inventory is
   still incomplete; never assert completion from a normal lint pass.
3. **Luna medium, accounting and documentation:** first fix structured audit
   argument boundaries and silent audit failures, with negative fixtures and
   a failure signal even if writing an error ledger also fails. Retain
   predicted-count events for diagnosis. Keep shell commands explicitly opaque
   where their child argv is unknown. Exact admission rules stay empty until
   source-backed review; duplicate audit events are not duplicate children.
4. **Sol, design/review:** specify the C producer record needed to preserve
   completed-process pid/ppid/tag/argv, then bounded post-success child-pid
   receipts. Review actual port diffs and injected accounting failures.
   A reader cannot recover finished metadata that the producer removed.
   Ctypes and LRU ownership implementation remain on explicit approval hold;
   preserve their designs and WIP without integrating incomplete code.
5. **Coordinator, final qualification:** freeze a clean coherent source;
   refresh all build-info objects and complete builds, then repeat full
   release/debug suites and strict startup-inclusive all-29/full-stats gates.
   Current stats has 6,513 dumps, 14,211,307 debt boxes, zero invalid owners
   and drops, 545 unfinished markers and 423 predicted-uncounted audit events.
   These are failing counts, not a controlled performance comparison. Close
   every remaining core-owned box, unknown process and provider/slot gap.
6. **Coordinator, measurement and follow-up:** stop worker builds, verify the
   reviewed measurement preflight on all three matched builds, and follow the
   declared three rotating timing rounds/load-four/45-minute window. Warm ELF
   pages before separate RSS measurements. Report raw vectors, spread,
   upstream/candidate and interim/candidate elapsed ratios, pyperformance and
   a profile of the largest costs. Timing does not establish R27 admission;
   any earlier diagnostic run must say so. Rank subsequent interpreter/G2,
   memory, serialization-recovery and shared JIT work from measured evidence.

Initial worker budgets are 15–20 minutes for proposals and 20–30 minutes for
the first implementation packet. They trigger progress reports and scope cuts,
not broad testing or unreviewed changes. The coordinator owns shared headers,
source integration, full tests, measurement and the final checkpoint recap.

## Live integration and measurement queue (2026-11-06)

The independently reviewed [follow-up queue](follow-up-6a0e4c12c17.md) gives
packet boundaries, worker roles, focused checks and integration exits. The
diagnostic campaign is complete; throughput work can proceed on correct
behavior and trustworthy workload accounting without waiting for a zero-box
census. R27 remains a porting diagnostic.

Runtime HEAD is `6a0e4c12c17` (constructor and format work), following
`05286694774` (native child accounting). All five build-info objects match
their respective source HEADs: upstream `3a440b65695`, interim `eba351fee7d`,
and candidate `6a0e4c12c17`. Module rosters are 116/116/117. Both full
profiles passed 54,351 tests with no failures (release: 2,531 skips; debug:
2,469). The strict whole-source R27 census failed: 6,512 dumps contain
11,028,818 core/core-pending/unresolved boxes, and process identity/accounting
is incomplete. The raw capture completed with 87 records over 29 kernels:
18/29 startup-inclusive raw-zero and 29/29 loop deltas zero, but strict kernel
admission failed. See the [kernel receipt](kernels-6a0e4c12c17.md) and [full
census receipt](full-census-6a0e4c12c17.md). The matched diagnostic campaign
is complete and does not show upstream parity. The independent R27 census
also fails but is not a performance admission veto. Candidate/
baseline elapsed-time factors are 1.287/1.075 for kernel geomeans, 1.096/0.913
for the 12-result pyperformance subset, and 1.259/1.062 for peak RSS against
upstream/interim; lower is better. See the [diagnostic report](../../bench/results/checkpoint-6a0e4c12c17.md).
Targeted differentials remain supporting evidence, not replacements for the
full gates.

1. **Functional qualification is complete.** Both profiles passed 54,351 tests
   with no failures (release: 2,531 skips; debug: 2,469). The earlier full-v2
   outcome remains recorded (54,347 tests per profile; two AST failures, one opcache failure,
   and one `test_tools` `sys.path` environment change). The targeted repair set
   passes and the latest batch reports no environment change.
2. **Keep workload accounting trustworthy.** Fix process/event defects that
   affect workload identity, counter conservation or test correctness. Preserve
   the 11 raw-nonzero kernel rows and all census errors as diagnostic evidence;
   do not require all 29 kernels or the full suite to reach zero boxes before a
   bounded performance packet. Ctypes and LRU ownership implementation remain
   on explicit approval hold.
3. **Source/build identity is confirmed.** All five build-info objects match
   their respective source HEADs, with 116/116/117 module rosters; the exact
   retained executable IDs are recorded in the census, kernel and diagnostic
   receipts.
4. **Diagnostic measurement is complete; upstream parity remains open.** The
   report used JIT-off GCC `-O3` builds without PGO/LTO and unchanged source.
   Pyperformance warned on 35/36 results about one-percent precision; six
   kernel rows were noisy. The completed source review and hardware counters
   point to GCC source overhead, Python-call preparation and frame traffic,
   but do not establish causal boxing cost or total gain. Use existing evidence
   to start bounded throughput work; add counters only for a specific patch
   question. Correctness and trustworthy workload accounting are required, not
   a zero-box census. P5 in the [follow-up queue](follow-up-6a0e4c12c17.md)
   contains measurement-driven hypotheses, not fixes. Keep the one-minute load guard
   (reject at load >= 4) and full 1/5/15-minute tuple for future campaigns.
   Historical 1.73x/1.83x ratios refer to `5de7bd62ef9` only.
   Prioritize steady-state throughput; RSS is secondary. The completed
   [hardware evidence review](hardware-perf-review-6a0e4c12c17.md) accepts all
   54 counter runs, 18 empty calibrations and six user-only DWARF captures.
   The three selected regressions retire 1.63–2.05 times upstream instructions
   with comparable or lower CPI. P5 now proposes GCC-only source overhead,
   bounded attribute/accessor work, Python-call preparation and frame traffic;
   no optimization speedup has been measured yet.
5. **Write the checkpoint recap from qualified evidence.** Ctypes and LRU
   implementation remain approval-held; G2 stays deferred. Serialization
   recovery and the shared optimizing method/tracing JIT follow the checkpoint.
