# Interpreter performance56: architecture review

Status: **completed source review and proposed sequence**, 2026-11-13. No source
implementation, full generation, build, native candidate run or agent delegation
was performed. This review does not qualify any current candidate. The
[combined design](../../design/proposals/interpreter-performance56.md) contains
the architecture, ownership contracts, diagrams and validation plan.

## Recommendation to root

Adopt one checked value/ownership/effect contract in the existing generator and
register optimizer. Use it for stronger borrowing, PyValue transport, call and
return forms, and specialization. Sequence small ordinary-path increments before
requiring complete static storage authority. Preserve the current frame ABI's
useful parts and qualify each pending55 packet at its actual scope.

PyValue transport is a sound early foundation because canonical homes already
exist and the generator already carries partial value facts. It is not a new
representation migration and need not delay the independent CALL_EX tuple probe.
Borrowing, call preparation and specialization then consume the same facts;
they must not invent separate arbitrary-C-body parsers. A shared contract slice
must have an immediate consumer, rather than become a framework project.

The initial thesis and open questions were recorded here and sent through the
Loom child-session channel as `interpreter-performance56:thesis:1`. Initial
sandbox access failed; the direct authorized coordinator message succeeded.
Open questions remain the bitmap monitoring diagnosis and any new same-head
8232 PMU/all29 evidence. Neither is assumed resolved in this review.

## What the evidence supports

Let `C=/home/power/code/cpython/.worktrees/python-jit`,
`BASE=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`,
`R=BASE/worktrees`, and `E=BASE/pyjit-link-wave1/call-model55`.
Canonical C documentation was read; C runtime sources were not used. Source
anchors in the design use these exact inspected heads:

| Tree | Source inspected | Qualification boundary |
| --- | --- | --- |
| I: call55-integrated | ccb96ccff6c9156430de205fb545b9895ddaf0fd | Current source, not the measured 8232 native |
| W: call55-ir | 3cfc808d8eb130b1614a956c8bb486dffb98f3f1 | Contains IRSC2 9c297510ac3 and isolated empty-family repair; source only here |
| B: call55-bitmap | 0e918ddc25ba5fcf0c7bdc9fef88b3ada5712c2e | Root reports both 117/79 builds; no current timing or complete qualification |
| P: call55-pins | 80b3de991c6c99d3afbfaf83ddf58ab9885e04c3 | Presence-adapted retained-owner lifecycle source |
| G: call55-gc | 0b447e8235bb056939fa7ad1376252b3c894402f | Monitoring/read-role investigation is active; HEAD alone does not identify uncommitted investigation work |
| U: upstream-3a440b6 | 3a440b65695615e9fd9912342b79cdd54fced0d2 | Immutable upstream control supplied by root |
| Selected54: frame54-cold-construction | 61015a9452d9069646fb5fede9ff963896833b89 | Immutable selected control with its recorded diagnostic limits |

The numerical table in the design comes from
`C/pyjit/issues/call-model55/report.md:7–51`, read directly. Native8232's retained
ELF SHA256 is `2aba9d15fc2092083917d8998083a59cf4e3d038c131801084b2c9b49827223a`.
Evidence is `E/scalar-storage-*`, including `scalar-storage-host` and
`scalar-storage-hot-screen-golden`. The two-pair comparison is mixed. Generators
are about 43% slower than upstream, DeltaBlue about 18%; logging is faster.

The broad ed08 comparison and matched PMU are in the same report at
`:1195–1217`: `E/m2-phase-all29-upstream`,
`E/m2-phase-perf-cycles-instructions` and `E/m2-phase-perf-record`.
The all29 geometric mean 1.165467 and six wins are historical, not current-ccb
qualification. DeltaBlue's matched instruction ratio 1.368099 and cycle
ratio 1.220915 establish substantial extra execution at that identity. They do
not identify a single cause or an additive time budget.

The latest design cannot claim a 40% universal gap, a frame-zeroing explanation,
or a new performance winner. Golden54's all29/upstream 1.156543 is likewise a
different campaign. No multiplication of independent ratios or pooling of those
campaigns is justified. Source actions, generated C, native instructions,
dynamic frequency and elapsed time are five different levels of evidence.

## Critical current audit

1. **Borrowing is partly implemented.** I `Python/flowgraph.c:3068–3310`
   uses per-block reference analysis to mark LOAD_FAST_BORROW. I
   `Python/regcfg_passes.c:155–305` removes eligible single-use loads and
   substitutes local operands, with family borrow roles, source-line/EH and
   intervening mutation/callback restrictions. Some hot loads have no word left.
   Any savings estimate must count post-pass surviving instructions and
   runtime materializations. Multi-use and CFG propagation remain real options.
2. **A surviving borrowed load still owns a temporary.** I
   `Python/bytecodes.c:245` DUPs, whereas U `Python/bytecodes.c:290` borrows.
   This is a deliberate persistent-home ownership rule, not proof that every
   such count is necessary. Eliminate the home by virtualizing safe uses; do not
   silently turn all frame temporaries into unencoded borrows. I
   `Tools/cases_generator/ir_generator.py:4136`, `take_view`, correctly obtains
   ownership when a local view must survive effects. A callback can mutate an
   ancestor's locals, capture its frame or replace function code.
3. **Canonical PyValues are complete; transport proof is partial.** I
   `Include/internal/pycore_value.h:141` normalizes an owning native result when
   needed. `:815` has a specific canonicalizing object-steal constructor;
   `:825` DUP preserves bits; `:1250` AsObject returns an owning object reference.
   These are different contracts. I `ir_error_effects.py:45–64` already separates
   result kind, ownership, canonicality and nonnullness. Expand checked facts,
   not a `VALUE means canonical owner` shortcut. Canonical producer proofs at
   `ir_generator.py:6122–6549` already avoid some Publish checks; measure the
   unproved remainder, migrate each proof once, and remove its redundant parser.
4. **Calls already have major ABI improvements.** Exact thread calls overlap
   arguments, reserve once and return inline. U `Python/bytecodes.c:4573` copies
   arguments into child locals. I `Python/ceval.c:4215` has an existing known
   binder, so replacing it with another generic call object would add work.
   Target snapshots, repeated guards/refresh, unsupported binding shapes and
   remaining completion adapters. Preserve independent function/code owners.
5. **The early tuple candidate is concrete but bounded.** I
   `Python/ceval.c:1510–1556` DUPs args and function and creates a new object
   reference before testing exact tuple. U `Python/bytecodes.c:5587` tests a
   borrowed tuple first. A callback-free exact-object-tuple probe can avoid up
   to three mortal refcount pairs while keeping final refresh and the nonexact
   owning path. An empty immortal tuple or immortal callable reduces that count.
   Frequency, not source attractiveness, determines benchmark relevance.
6. **Specialization exists and needs consistent facts.** I
   `Python/specialize.c:1186`, `:1641` already includes property/classmethod
   handling. `Python/ir.c:455–565` constructs code-owned execution/feedback and
   frees it only at code teardown. Unify guards, effects, cache payload lifetime
   and deopt contracts across these families. Do not describe existing patterns
   as missing or promise a new framework will itself improve execution.
7. **Cold exception progress is useful, not a full allocator switch.** W
   `Include/internal/pycore_ir.h:656–670` gives HomeView retained ranges and a
   release limit; its 32-byte size is cold. W `Python/assemble.c:63` names interned
   transfer IDs. The IRSC2 packet removes EH slot-as-allocation-floor coupling,
   while the allocator remains canonical and transformed handler state may
   still use CONTROL3. The 112-byte hot frame and 40-byte root scope are reported unchanged.
   It opens allocation work but does not itself prove full static readability.
8. **Bitmap failures expose contract gaps, not a winner.** Root reports B removes
   scratch fills/frontier/copy gaps/known clears and conservative arena scanning,
   using non-NULL presence/read roles. Required reads must follow nullable guards
   and preserve conditional aliases. `_MONITOR_CALL` losing `maybe_self`'s role
   is a source hypothesis awaiting root's native diagnosis, not an established
   crash cause here. B's current build count is not lifecycle or performance
   admission; older presence-plus-NULL timings do not price this source.

## Disposition of pending55 lanes

| Packet | Decision | Required next evidence |
| --- | --- | --- |
| I 0df VALUE facts + ccb error-copy | Qualify as bounded foundation, not performance promotion | Root reports 32 narrow tests and FULLCASES; full metadata/generation must pass after the isolated repair, then exact-head native focused checks |
| W 3cfc empty-family repair | Qualify separately from the broad W packet | Empty emitted-family metadata regression, full emission by root, exact diff showing no ordinary handler semantics change |
| W 071d + 4c646 + 9c297 IRSC2 | Qualify independently; freeze interface while ordinary lanes start | Full products, reader/magic 64423 consistency, malformed metadata/position-stripping and retained-EH runtime/GC/debugger cases; then coherent suite gate |
| P80b retained lifecycle for B | Peer and qualify against the exact B composition | Sparse-release cursor/retained-root visibility, nested callbacks, escape and active/heap providers |
| B0e918 complete presence rival | Keep isolated and unselected until diagnosis closes | Retest three corrected fixtures from older 31/34 result; close monitor/profile failures; root reports 480 tests across the diagnostic run: seven modules pass and two fail, not a complete gate; then matched timing |
| Full static M4 | Continue as its own bounded storage workstream | Positive array, CALL_WAIT, suspension, observer, transformed-handler and read proofs before stale cells or conservative arena scanning are removed |

These newer packet statuses are from the coordinator's task handoff, except the
heads and source inspected above. At review time, canonical STATUS/HANDOFF still contained older B
progress; root subsequently updated both in f3f195af48d. This review deliberately does not rewrite them or upgrade source-only
work. Root must attach final evidence paths and identities when integrating.

## Recommended order and alternatives

The design's lane table is the implementation plan. Root should first select
and freeze a sound integrated source for experiments. Admit the small generator
repair and VALUE/error-copy foundation through their actual gates, without
waiting for IRSC2 or complete storage authority. Then propose the CALL_EX probe
and one shared-fact transport family in parallel. They establish early measurable
work removal. Broader borrowing, call completion and specialization consume that
same contract and can proceed independently once their needed facts exist.

IRSC2 is a prerequisite for allocation freedom involving non-prefix exceptional
survivors, not for every canonical store or borrowed local. Cross-line borrowing
needs observer reconstruction, and cross-callback virtual values need owning
snapshots or a stronger specific proof. Full M4 remains separate. The sequence
should be revised by measured frequency and source/native removal, not locked
to either PyValue-first or frame-first ideology.

| Option | Decision and reason |
| --- | --- |
| Multi-use virtual local reads, owned edge snapshots | Implement first borrowing slice; no persistent borrowed encoding or GC migration |
| Retain facts through exact guarded operations | Implement after shared effects; facts end at mutation/reentry or miss materialization |
| More common binding and direct recipient forms | Implement where counts expose adapters/checks; reuse existing binder and generated completion |
| Property/classmethod improvement | Allowed general pattern, under ordinary lookup and capture semantics; existing forms must be counted first |
| Global persistent borrowed homes | Reject as initial design: requires another owner authority across every reader and callback |
| VALUE return implies canonical/owned/nonnull | Reject: helper type and representation/ownership are independent |
| Another canonical PyValue rollout | Reject: one format already spans homes, containers and extensions |
| Delete source-line barriers wholesale | Reject until equivalent observer/exception state can be reconstructed |
| Every optimization waits for full M4 | Reject: local proofs work with initialized-NULL homes |
| More body-name/token whitelists | Reject as architecture: migrate toward one checked action/effect producer |
| New universal call/continuation object per operation | Reject: exact paths already avoid generic records and decode |
| Remove code pin or hot environment caches | Reject without full lifetime/dependent-load replacement; mutable function code remains independent |
| Smaller frame, zero stores or zero boxes as selection target | Reject: total ordinary cost and elapsed breadth are the objective |
| Retain all values until frame return | Reject: loops/generators can retain large graphs/resources without bound |
| Direct final-home generator construction | Optional only with a distinct hypothesis; earlier broad failures remain evidence |
| Bitmap is already qualified/current winner | Reject: current functional issues and missing timing remain |
| Native JIT as answer to interpreter gap | Out of scope; shared contracts prepare future consumers without implementing them |

## Quantitative hypotheses and falsifiers

The following are **proposed decision rules**, not measured results or claims
that root's existing process already uses these thresholds. Root should fix
them before each experiment. Count runs and throughput runs are separate.

For elapsed ratio r against upstream, the fraction of candidate time that must
disappear to reach parity is `1 - 1/r`. Thus generators at 1.430017 require about
30.1% of candidate time, and DeltaBlue at 1.177991 about 15.1%. A 40% slowdown does
not imply 40% of current work is removable. An isolated operation with exclusive
time fraction p cannot save more than p even if made free. Flat profile buckets
are not exclusive measured p and cannot be added or used to establish that bound.

| Hypothesis | Observable prediction | Falsifier / stop rule |
| --- | --- | --- |
| CALL_EX tuple fast probe matters | Exact hits skip up to 3 mortal pairs and the owning object projection; nonexact paths retain behavior | No exact hits in hot6 means no hot6 claim; zero native work removal rejects implementation mechanism. If measured savings are below 1% with confirming ranges spanning 1, retain only with an explicit small-cleanup rationale, not as a stage performance result |
| Borrowing removes ordinary traffic | Fewer surviving load words/homes, fewer dynamic acquisitions and releases after all edge snapshots | Net removed pairs/words <=0 rejects that transform. A gain only in synthetic patterns with negligible all29 frequency rejects the broad-performance hypothesis |
| Transport facts remove repeated normalization | Lower dynamic Publish/AsObject entries or tag checks for identical semantic outputs | Generated C changes but optimized native body is unchanged: no native cost claim. Native work falls but repeated elapsed regression exceeds 2%: inspect spills/code layout, revise or reject |
| Shared specialization facts reduce checks | Hot hit executes fewer guards/bridges; misses preserve inputs and callbacks run once | Higher miss rate, repeated warmup or invalidation work cancels savings; unchanged hit instructions means consolidation is maintainability only |
| Call convention cleanup lowers boundary cost | Fewer snapshots/refreshes/adapters per call or resume, with overlap and direct return preserved | Fewer stores but more indirect calls, descriptor loads or cold-scope entries negates hypothesis; confirm total cycles, not nominal ABI size |
| Full storage authority is worthwhile | Removed fills/scans exceed presence/map/phase/read maintenance at actual frequency | Complete same-config rival loses confirmation or broad gate; select safe baseline and retain evidence, not partial-composition ratios |

Require an expected dynamic operation delta before implementing each candidate.
If two bounded revisions produce neither native removal nor measurable benefit,
close that experiment and choose the next highest-frequency unresolved cost.
This bounds speculative work without abandoning the four requested workstreams.

Suggested promotion screen: two balanced hot6 pairs against the frozen parent
and controls; promising compositions get six-pair confirmation. A repeatable
>2% regression in any hot case blocks promotion pending diagnosis. Results within
noise are inconclusive, not wins. Final all29 geometric mean should improve on
the chosen baseline, preserve logging's existing advantage, and have no unexplained
repeatable >2% case regression. A smaller gain can be accepted only with explicit
root judgment and adequate repeat precision; these thresholds are practical
tripwires, not statistical significance tests. Always report individual ratios
and pair ranges. Final direct golden54 and upstream campaigns use the same
candidate/configuration; historical ratios cannot substitute for them.

## Evidence still required

- Same-head8232 PMU and all29 are absent. A newer source head must obtain its own
  native identity and evidence, not inherit those screens.
- Dynamic post-pass load/use/escape counts, bridge/Publish frequency, binding
  plan hits, refresh count and cold-record frequency are not established here.
  They decide which concrete broad increment comes next.
- Complete static M4, complete B functional qualification, current B timing,
  W full-generation/native qualification and composed I/W/P/B behavior are open.
- The latest full55 foundation diagnostics found allocation-injection failure;
  the positive-readiness repair passed its original OOM case and 837 focused
  cases per role, but no green full55 run follows. Selected54 debugger/LLTrace,
  GC-verifier instability and the unexplained profiling finding remain recorded.
  Only the existing GDB pretty-print class is quarantined.
- Source-only review cannot establish compiled spills, inlining, instruction
  cache effects or exclusive operation costs. Root owns those observations and
  serializes native work and quiet timing.

## Review provenance and handoff

Read applicable AGENTS, the linked CPython AI policy, C's README/GOAL/STATUS/
HANDOFF/PROCESS, existing model55 target and the five requested as-built designs.
Inspected bounded runtime source and compared pinned upstream LOAD_FAST,
exact-call entry and CALL_EX behavior. Primary V8 and LuaJIT sources actually
read are linked in the combined design; no collector or performance conclusion
is imported from them. The unavailable pinned V8 fetch is not cited as read;
the successful moving-main optimizer page is labeled accordingly.

The deliverables are these two Markdown files only. Review covered their
contracts, Mermaid source, source anchors, evidence attribution and final diff.
No runtime tests were run for this documentation task. Root should peer the
combined architecture, integrate the approved documents, update canonical
navigation/status, and assign Sol's bounded proposal/peer/implementation cycle.
