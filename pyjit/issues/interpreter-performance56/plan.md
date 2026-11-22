# Interpreter performance: combined plan

## Current wave: hot85, the remaining slow cases and hot-code size

Approved 2026-11-21. The user promoted parity84's IP2 as golden84
(`4be2eb5d31f`, `pyjit/round84-golden`, tag
`pyjit-golden-20261121-parity84`) and asked:
- what, by opcode and code structure, still makes deepcopy and the other
  slow cases slower than upstream;
- for a cleanup of the hot code size, with rare branches in hot handlers
  moved out to functions.

The [hot85 record](../register-semantics59/hot85.md) holds the units.
Evidence `E59/hot85/`.

- **Phase A**, analysis on golden84:
  - LB: the library cases (deepcopy, logging, json, pickle) by opcode
    family, native symbol and Python line;
  - XK: the xml cases, nqueens, comprehensions, coroutines and
    regex_compile, the same way, and by source construct;
  - HC: cycles and front-end counters on CPU 15 for all 29, spectral_norm's
    op-cache mechanism, the hot footprint, the rare paths inside hot
    handlers and helpers, and one prototype family.
- **Phase B**, implementation units from HC's plan, from golden84;
  integration on `pyjit/hot85`; full suites, instructions for all 29 and the
  timed all-29 against golden84 and upstream.

Gates are correctness and the allocator limit (50M; golden84 is at 44.84M).

Phase B1 is integrated as IP3 (`2a9ea8c4f69`): seven units, with RT not
merged. It runs 0.949 of golden84's time and 0.900 of upstream's, with
known failures only and an allocator product of 44.39M. Next, if the user
promotes it:
- **Phase B2**, the items B1 deferred:
  - the C-to-Python entry;
  - generator creation, resume and yield, which is nqueens' genexpr;
  - int-subclass operands;
  - `CALL_KW_PY`;
  - the `_json`/`_pickle` ports;
  - regcfg folding of `LOAD_FAST_BORROW` across a generic `LOAD_GLOBAL`, so
    that more call sites qualify for `IR_F_CALL_LOCALS`;
  - an exact-bytes exit in `richcompare_bool_same_type`.
- **Design list:**
  - constants as an operand class read from the exec image (RT);
  - mimalloc retire hysteresis for one-page size classes (CP's json_loads
    finding);
  - the op-cache sensitivity of small hot sets (spectral_norm, nbody,
    coroutines). Their decoder µops range from 0.03 to 1.75 of golden84's
    across one-unit builds, and neither the hint nor the smaller handlers
    made them robust. Any remedy must be structural, not placement tuning.

## Previous: parity84, the clean changes profile83 ranked

Approved 2026-11-21. The [parity84 record](../register-semantics59/parity84.md)
holds the units and their state. Base: golden82 (`5194e957c5d`); integration
branch `pyjit/parity84`; evidence `E59/parity84/`.

- **Phase 1**, four implementation units from golden82:
  - NB: native-call publication, one native→Python entry, thunks,
    exceptions, `isinstance`;
  - GL: the generators stall test, `GET_ITER` over a Python `__iter__`,
    creation at the call, plain finishing, resume;
  - IT: membership, compare exits, `FOR_ITER`, truth tests, the unpack loop;
  - RM: allocation entry, GC visits, dicts, `makeuniversal`, json ints, the
    borrow stack, big-int ops, `MAKE_FUNCTION`.
- **Integration point 1:** merge, full suites, instructions for all 29,
  timed all-29 against golden82 and upstream.
- **Phase 2**, two units from IP1:
  - BD: per-site bind plans, `CALL_FUNCTION_EX` with a dict, generator
    callees bound into the new generator;
  - GN: one generator creation path; GL's remainder.
- **Integration point 2:** full suites, timed all-29, the record, the report.

Gates are correctness and the allocator limit (50M; golden82 is at 45.45M).

Result (2026-11-21):
- IP1 (`5a51846ce16`) runs 0.948 of golden82's time and 0.949 of upstream's.
- IP2 (`4be2eb5d31f`) runs 0.961 and 0.953, and executes 0.962 of golden82's
  instructions.
- Both pass the full suites with golden82's known failures only.
- IP2's difference from IP1 is spectral_norm's op-cache placement. Without
  spectral_norm, IP2 runs 0.950 of golden82's time and IP1 0.948.

The record lists the open items. Hot-code size (profile83 group F) is the
structural lever for the placement sensitivity.

The user promoted IP2 as golden84 on 2026-11-21.

## Earlier: golden82 and the profile83 pass

On 2026-11-20 the user promoted gap82's integration to golden and asked for a
profiling pass: where is there still room for clean improvements? Golden82 is
`5194e957c5d`, branch `pyjit/round82-golden`, tag
`pyjit-golden-20261120-gap82`. Its source (`worktrees/gap82`, detached at the
tag) and builds (`BASE/gap82-{release,debug}`, aliased as
`BASE/golden82-{release,debug}`) are frozen controls.

Profile83 is analysis only, in `E59/profile83/`. Three agents compare
golden82 with upstream:
- P1, the evaluator kernels: categories, per-call costs and handler ledgers;
- P2, the library-heavy cases and startup: per-symbol excess;
- P3, cycles and the front end on CPU 15: each case's gap split into
  executed work and cycles per instruction, the hot footprint and the
  structural levers that shrink it.

**Result (2026-11-20).** The [profile83 record](../register-semantics59/profile83.md)
holds the ranked list. The remaining gap is executed work, not stalls: over
the 19 cases with more cycles than upstream, instructions explain 2.02 of the
2.03 summed log-excess. Ordinary calls, returns, attribute accesses and
generator steps are at parity or ahead. What remains is fixed cost per
event at boundaries:
- native-call publish;
- C code entering Python (eight layers against four);
- generator creation and finishing, and `GET_ITER` over a Python `__iter__`;
- the non-exact binder;
- membership tests and the generic `FOR_ITER`;
- allocation entry, GC visits and the dict's first insert.

Generators' extra 13% of cycles against unit G is a back-end stall, probably
K2a's branch-free `IR_SKIP_RESUME`; that is untested. The next wave is the
user's choice.

## Previous wave: gap82, the gap to upstream after H5

Approved 2026-11-20. After H5, the user asked to pursue every area of the
post-H5 analysis, with analysis and implementation fanned out to agents and
root integrating, benchmarking and reporting. The
[gap82 record](../register-semantics59/gap82.md) holds the units, their
state and, later, the results. Base: `pyjit/handlers81` at `d6d1ed5ac93`;
integration branch `pyjit/gap82`; evidence `E59/gap82/`.

- **Phase 1:**
  - analysis: A1 for the evaluator against upstream, A2 for the
    library-heavy cases;
  - implementation: H6 (layout), CS (constant slots with no fixed cost) and
    TX (delete the call transaction).
- **Integration point 1:** merge, full suites, timed all-29 against H5.
- **Phase 2:** G (generators and iteration) and K (attribute and call
  handler costs), both briefed from A1, and N (library side), briefed from
  A2.
- **Integration point 2:** full suites, timed all-29 against H5, golden73,
  upstream and checkpoint78, then the report.

Gates are correctness. H6 alone must pay for itself in time.

**State (2026-11-20): both integration points done.** `pyjit/gap82` at
`5194e957c5d` holds every unit except H6, which was parked with no commits.
Its full suites fail exactly H5's counts. Timed all-29 geomeans:
- 0.992 against upstream (H5: 1.065);
- 0.928 against H5;
- 0.884 against golden73;
- 0.899 against checkpoint78.

Instructions are 0.912× H5, and startup is 63.6M instructions against
H5's 530M. Code placement now limits the call-heavy cases with small hot
sets: logging_silent is 1.15× H5, and generators lost time while running
the same instructions. The [record's results](../register-semantics59/gap82.md#results)
list what is still open.

## Previous rollout: handlers81, then calls80 (landed)

Approved 2026-11-19. The user asked to land
[handlers81](../../design/proposals/handlers81.md) and then
[operand-encoded calls](../../design/proposals/operand-calls80.md) in its
shape, and to keep the result even if the first performance gain is modest.
Gates are correctness; measurements are recorded and attributed, not gated.
The H4 shape checks (handlers81 §6) are stop-and-diagnose checks on the
mechanism.

Branch `pyjit/handlers81` from checkpoint78 `99a04733128`; builds
`BASE/handlers81-{release,debug}`; evidence `E59/handlers81/`.

| Step | Content (handlers81 §6) | State |
| --- | --- | --- |
| H0 | Shape tool `Tools/pyjit/handler_shape.py`; checkpoint78 baseline | done, `ea164548495`; handler bodies are 55–69% of kernel instructions |
| H1a | Canonical by construction; no publication at stores | done, `ef2efa83c95`..`3f6cb5e89aa`; kernels −1.8 to −4.9% alone |
| H1b | No `phase_state` in handlers; GC scans all slots | done; the field and all lifecycle bits are deleted; kernels −1.6 to −2.4% alone |
| H2 | Ownership rules 3, 5–7; `IR_F_DIES_*`; discard slot; upstream frame clear; transition roots deleted | done, `81a2cea8cc2`. H2f: retirement overhead 166 → 3 instructions per call. H2 core: every handler emits effect, detach, store, release; a hidden `.discard` local replaces `IR_NO_DST` (magic 64429); the release protocol is deleted. Kernels −6.6 to −14.6% against checkpoint78; hot-set fast path 1,776 → 1,670; ownership harness unchanged. The full suite found 164/166 failures that the ladder missed (C-built code objects without the discard slot, `test_dis` registers, magic number, order pins, discard-slot liveness in five stdlib tests); fixed by eight commits through `aa0b5f87f7d` (pushed); full suites at `aa0b5f87f7d` fail only the six known modules, release and debug (`E59/handlers81/full-h2b/`) |
| H3 | Bridge records and canaries out of handlers | done, `9bbadd75cc6`..`6aac41de421`, merged with the H2 fixes as `cfcec785504` (pushed). No handler boxes an inline value; the value record, `finish_value_bridges` and the error path's `value_temps` are deleted; canary handlers 46 → 0 of 326; no handler takes a local's address (two inlined helpers excepted). Kernels within 0.1% of H2 except coroutines −1.15%; hot-set fast path 1,670 → 1,654. Ladder (93 modules, both roles): known failures only. H2's dead markers remain (droppable) |
| H4 | One computed-goto evaluator; tail-call backend deleted | done, landed as `744d4ad992a` (pushed). First conversion correct but slower (kernels +7 to +11% against H3); diagnosis: zero guessed profile, GCC's allocator cliff (pseudos × blocks ≥ 2^26), six values across dispatch. H4b: computed-goto entry, no `slots` state, held advance against PRE (the structural dispatch advance measured +1% and was reverted), rules 12 and 13, 77 handlers outlined by a declared criterion. Allocator 31M (full), canary 0, frame 168 B, hot fast path 1,590 → 1,434 (−9.8%), hot stack references 41 → 33. Kernels against H3: deltablue +1.3%, raytrace −1.3%, richards +0.9%, coroutines +2.2%, deepcopy_memo −2.1%; attributed to outlined generic heads run at unspecialized sites and the generator steps' calls (follow-ups, not gated). The first full suites found an H3 regression (a scalar `__exit__` reached an always-object assert in `WITH_EXCEPT_START`), fixed by `53a0ca63988`..`744d4ad992a`; full suites at `744d4ad992a` fail only the six known modules, release and debug (debug also `test_profiling`, a load flake that passes alone; `E59/handlers81/full-h4b/`) |
| H5 | Calls80 steps 1, 2a–2c, 3 written in the new shape | done, landed as `d6d1ed5ac93` (pushed). Step 1 (`d2a24599732`): every Python child frame at the arena top. Step 2a (`600b4237187`): operand words. Steps 2b/2c (`8e70a4cdf82`): every call's inputs are trailing operand words; the handlers stay generated from `bytecodes.c` and read them; native vectorcalls borrow the operands through one helper; exact pushes fill the child from the words; the window form is deleted. Step 3 (`1337df6ded3`): constant slots for every inline or immortal register input, call operands included, at most eight per code object; `return None` reads the None slot; the frame copies a ready image. Magic 64432. Kernel instructions against step 1: deltablue +2.1%, raytrace +2.6%, richards +3.2%, coroutines +0.5%, deepcopy_memo +6.4%; against checkpoint78 −7.9% to −10.4%. Attribution: after H4 an argument load costs about what its operand fill costs (3 per word, 9 per local, 7 per temporary), so the design's per-load savings, estimated on checkpoint78's handlers, did not appear; `CALL_PY_EXACT_ARGS`' fast path grew 122 → 156; the inline result publication tests each operand; each constant slot costs 13–15 instructions per call even unused (NULL-tested and closed at exit). H5c (`pyjit/handlers81-h5c`, parked) cut that to 2 per slot but raised the first slot's cost; kernels a wash. Allocator product 41.9M of 50M. Full suites: the six known modules, plus two recorded jump refusals in `test_sys_settrace` (`test_jump_backward_over_async_listcomp` and `_v2`; `E59/handlers81/full-h5/`). Timed all-29 geomeans (time ratios; [record](../register-semantics59/handlers81.md)): 0.958 against checkpoint78, 0.944 against golden73, 1.065 against upstream (checkpoint78 was 1.112). logging_silent is 1.312 against checkpoint78: cycles at H4 (+21% at −3.8% instructions, frontend stalls doubled), a layout follow-up for H6 |
| Report | Kernel instructions, PMU, timed geomean against checkpoint78, golden73 and upstream | |

Each step is one or more Sonnet workers with a time budget, a small-first test
ladder and a progress log in `E59/handlers81/`. Root integrates: generation,
both builds, the step's ladder, kernel instruction counts (`E59/handlers80/instr.sh`)
and the shape tool. Full suites, release then debug, run after H2, H4 and H5.

Deferred until after H5: the lean normal-return root record, `__init__` shim
certification, `RETURN_NONE`, list-subclass `append`/`pop` specialization,
`_PyValue_RichCompare` spills, and hot-code layout (H6).

## Latest checkpoint: execution-boundary73 and RETURN isolation

The [grouped review](../register-semantics59/review73.md), exact source peers and
root approved the [complete boundary contract](../../design/proposals/execution-boundary73.md).
Sol workers implemented and cross-reviewed; root integrated, generated, built,
qualified and measured. The complete generator paths and guarded callback-free
receiver close remain in the candidate; none was replaced by a generic fallback
to satisfy the checkpoint.

Boundary10 `dee619e80ab` improves coroutine/generator throughput about 6% and
spectral_norm 24.69%, but loses logging 10.44% and raytrace 1.97%. Preserved
specialization and three matched PMU pairs show logging's extra cycles without
a comparable increase in executed instructions. Native review found enlarged
shared RETURN bodies, so root adopted a bounded source correction: one private
GEN_RETURN_ROUTE, with ordinary return state and all generator consumers retained.

That correction is complete at `44f066f5fd5`. Ordinary RETURN/EMPTY text falls
below selected57; arithmetic and exact-call whole-path counts match selected57.
The extra generator hop is charged. Protocol, owner, native and error controls
pass. The final protected screen improves coroutines 8.70%, generators 6.73%,
comprehensions 3.49% and raytrace 2.61%, but loses logging 13.73%. GM 0.985010
passes; logging protection fails. Independent review recorded NO-GO under the
original rules. The user subsequently accepted this specific regression and
selected golden73 on 2026-11-16. Branch `pyjit/round73-golden` and tag
`pyjit-golden-20261116-generator-boundaries` are pushed at this exact commit.
The [report](../register-semantics59/report.md) owns
both complete results and their evidence limits.

## Next implementation priorities

Golden73 is the new comparison baseline. Selected57 remains the recovery target
for logging and an immutable control. This landing accepts one measured tradeoff;
it does not turn failed historical screens into passes or authorize regressions
in future candidates.

1. **Ordinary call/return effect separation.** Keep the ordinary THREAD return
   and result consumer together; enter callback-capable retirement through a
   terminal private entry with explicit owned inputs. Preserve exact/general CALL
   specialization, destination semantics, polling and observer/error state. The
   objective is to avoid preserving hot VM state across cold returning C calls,
   not merely to shrink handlers. Golden73 already isolates generator routing;
   repeating that outline or reusing rejected generic completion is not new work.
   Before source adoption, compare final73 against selected57 on matched logging
   counters and complete hot call/consumer paths. The earlier boundary10 PMU is
   not a diagnosis of final73. Reject a proposal without a concrete boundary it
   removes; code size and instruction counts alone do not explain the regression.
2. **Specialized native result-to-consumer lowering.** Extend the successful
   owned-result/effect interface to one complete native CALL family and its real
   destination/discard consumer. Carry guarded provider facts into that consumer;
   avoid a home store/reload only where the whole path no longer requires it.
   Keep callback roots in the reached effect path and retain all existing
   specialization fallbacks. This needs a fresh source contract proving net work
   removal beyond the rejected Invoke/provider/compact attempts, including the
   added dispatches and nonparticipant cost. It is not an approved broad rollout.
3. **Lifetime-driven frame allocation, after execution boundaries.** Separate
   stable locals from short-lived values in compiler liveness, reuse temporary
   homes with disjoint lifetimes, and let cold exception/observer maps describe
   that allocation. Do not reserve universal scratch or add a per-operation bitmap
   tax solely to simplify teardown. Source adoption requires an actual reduction
   in frame extent or initialization for unchanged workloads. The earlier map-only
   layout attempt did not achieve this; it is not a prerequisite to resurrect.

These are ordered design directions, not implemented gains or an assumed parity
budget. Sol 6.1 source/peer workers prepare bounded independent units; Astra
reviews a grouped architectural change rather than the component loop. Root
owns integration and quiet benchmark windows. Use focused opcode/ownership
checks during implementation, merged protocol and hot8 checks at integration,
and broader qualification at the next coherent checkpoint. Protect the newly
selected generator/coroutine gains while recovering logging.

The next measurement checkpoint also needs all29, full qualification and a fresh
direct upstream comparison of golden73. None has run yet; the accepted landing
does not imply those results. Keep current source/build controls frozen and use
fresh evidence tags. Do not combine the best cases from boundary10 and final73.

## Selected baseline and closed work

Golden82 is selected (2026-11-20); the [gap82 record](../register-semantics59/gap82.md#results)
owns its measurements. Golden73 (`44f066f5fd5`) remains a comparison control;
the [trial report](../register-semantics59/report.md) owns its measurements and
accepted tradeoff. The [selected57 report](../comprehensions57/report.md)
owns the prior control's qualification and upstream comparison. The
[trial report](../register-semantics59/report.md) owns rounds59–73, their
mechanism successes, incomplete gates and measured rejections. Those packets
are evidence, not prerequisites or runtime components selected for73.

Boundary73 starts from selected57. It does not import72's continuation map,
carried-arithmetic mode or frame layout. Source reviewers must preserve existing
CALL/arithmetic specialization and close every admitted generator transfer,
including live HOME results and callback retirement, before root qualification.
The bounded frozen72 diagnosis is complete; it establishes specialization losses
in raytrace but does not establish the cause of logging's extra cycles.

Read the [goal](../../GOAL.md), this plan,
[runtime architecture](../../design/runtime-architecture.md), [status](../../STATUS.md)
and [handoff](../../HANDOFF.md) for direction, contracts and execution state.
The active workstream is [IR semantics59](../register-semantics59/README.md).

## Outcome

Improve the interpreter across DeltaBlue, generators, coroutines, Richards,
comprehensions and silent logging, then qualify the whole benchmark set.
Preserve Python behavior, explicit ownership, useful observation and the gains
already delivered by canonical PyValues. Select qualified improvements against
immutable golden54 and upstream controls. Do not attribute the generator or
DeltaBlue gap to frame clearing without evidence.

## One architecture, four workstreams

| Workstream | Required design and implementation |
| --- | --- |
| Values | Distinguish canonical homes, native private results and typed borrows. Carry established canonical/object facts through producers and native interfaces; eliminate redundant conversion and ownership work. |
| Borrowing | Extend the existing borrow-selection and copy-propagation passes with an explicit use/def, effect and lifetime model. Establish where a borrowed view is legal, when it becomes an owner, and what callbacks, local mutation, escape and suspension invalidate. |
| Calls and returns | Preserve exact-call overlap and inline return. Define a common argument, binding, recipient, continuation and retirement contract for ordinary calls, generators and coroutines, with exceptional/observer work paid at reached boundaries. |
| Specialization | Use one checked description of guards, effects, ownership, result facts, cache validity and fallback across lowering and interpreter specializations. Reuse existing adaptive caches; avoid parallel handwritten proof systems. |

PyValue is already canonical throughout runtime homes, containers and rebuilt
extensions. Native numeric results can still be private boxed temporaries until
publication. That boundary is the cleanup target; this is not another value
format rollout. A borrow pass also already exists: the frontend selects safe
borrow candidates and register copy propagation folds many local reads into
consumer operands. Strong snapshots at callback and ownership boundaries remain
necessary unless a stronger lifetime proof replaces them.

## Completed checkpoint

Selected56 is 883524c180e, qualified over byte-identical cb3 runtime sources.
The [report](report.md) owns results, qualification limits and selection judgment;
[benchmark tables](benchmark-tables.md) retain every case and observed range.
Astra reviewed the architecture; Sol peers reviewed proposals and actual changes;
root generated products, ran native integration/full diagnostics and measured.

| Workstream | Completed result |
| --- | --- |
| Values | Scalar replay/receipts plus canonical list/tuple transport and exact-coroutine owner transfer; physical NULL-control remains |
| Calls | Exact-tuple CALL_EX preparation, direct native completion entries and checked generator-recipient admission; ordinary overlap/inline return preserved |
| Specialization | Two-to-four homogeneous SHAPE PIC rows share checked guard/read/acquire contracts; actual DeltaBlue/Richards use confirmed |
| Borrowing | Reproducible21-file/426-CFG census found zero eligible surviving callback-free COPY webs; no redundant pass added |
| Qualification | Full 55,023 tests per role, six baseline debugger errors; final stale trace fixture fixed with focused checks; builds 117/79 and 5+27 bootstrap/compile pass |
| Selection | All 29/golden .984655, 17 wins; upstream 1.141077, 7 wins; all six focus cases improve in every confirmation pair |

Root explicitly accepts Raytrace 4.50%, spectral_norm/nqueens~3.5%, XML iterparse
2.7% and chaos2.5% regressions as an incremental tradeoff, overriding the audit's
suggested unexplained>2% tripwire. Cold selection review supports that judgment.
Golden54 stays immutable. These losses are unresolved debt, not noise.

Bitmap execution readers, IRSC2 retained transfers and full static M4 remain
independent gates. Do not complete all historical experiments as a prerequisite
for every ordinary-path improvement. Preserve their source/evidence.

## Measurement and selection

Use unchanged workloads, matched GCC configurations, fixed affinity/hash and
JIT disabled. Record source/native identity, validity checks and pair ranges.
Measure the actual count and cost of remaining conversions, count operations,
dispatches, call adapters and storage maintenance. Historical flat samples or
instruction ratios are clues, not additive removable cycle budgets.

Each integration reports what work disappeared and its hot-set result. Use
larger confirmation only when the initial evidence warrants it; run all 29 and
matched upstream profiles for final selection. Retain the current golden if no
qualified composition earns selection. Startup RSS and zero-box counts are not gates.

## Documentation cleanup

The goal, active plan, architecture and status now identify this stage.
The checkpoint report owns current comparison evidence; model55's report and
reviewed source remain historical inputs. Four duplicate progress/staging pages
are retired with their exact committed contents retained in git and the stage56
evidence directory. Ten superseded Loom review/inventory/progress mirrors have
been exported with revision metadata and empty thread inventories before
retirement. All ten are now retired; 12 current mirrors retain the goal → plan →
architecture/status chain. Canonical design/review evidence remains in git.


## Execution and delegation

The [trial report](../register-semantics59/report.md) distinguishes actual
mechanism evidence, incomplete qualification and measured rejection. Rounds59–69
remain unselected; round70 is stopped at source adoption. Preserved evidence is
not a dependency list.

| Owner | Current task | Integration boundary |
| --- | --- | --- |
| Root | Boundary73 and return-isolation checkpoints complete | Preserve selected57 and candidate evidence; own changed-candidate validation |
| Sol 6.1 compiler worker | Boundary selection, public validation and focused tests complete | Peer the next concrete source contract before implementation |
| Sol 6.1 execution worker | Complete generator transfer and callback retirement complete | Return isolation complete; preserve all consumers in successors |
| Sol 6.1 cold reviewer | Native, ownership, specialization and screen reviews complete | Audit actual whole paths and protection for any successor |

Both72 ABI implementation lanes are frozen. The layout72 and both71 screens remain closed. Fresh
diagnostic profiles are retained under `E59/architecture-diagnosis72-fresh`;
they do not establish a timing result or a removable cycle budget.
A losing screen stops the unit. A qualified broad winner may be selected before
parity; repeat grouped review and implementation against the remaining gap.

For each increment: propose → peer → implement → focused tests → actual-diff
review → root integrate/hot comparison. Full suites belong at coherent protocol
or final composition gates. Retain stable exact source/native identities and
balanced pairs, and compare all29 directly before final selection. Preserve
selected57, golden56, golden54 and upstream controls. Do not repeat a losing
screen to search for a favorable result, tune handler padding, or weaken owner,
readability or semantic admission checks.

The hot set is comprehensions, DeltaBlue, generators and coroutines; logging,
raytrace, spectral_norm and nqueens are regression sentinels. If a coherent cycle
still misses upstream parity, commission another grouped Astra review of the
actual remaining IR/interpreter costs and continue implementing measured winners.
Only existing GDB PrettyPrintTests is quarantined; six debugger-jump errors
remain explicit. Startup RSS and zero-box counts are not throughput gates.

## New comprehension evidence

The [focused diagnosis](comprehensions-analysis.md) identified repeated method
redispatch, dictionary wrappers, lost list result facts and unconditional append
calls at golden56. Its workload executed fewer inner-loop opcode words than
upstream but 46.44% more native instructions. Those concrete costs motivated
the four-lane experiment, rather than establishing that any one fix would close
the gap. Selected57 still executes 43.64% more comprehension instructions.

The [comprehension workstream](../comprehensions57/plan.md) is complete. All four
changes were implemented and the composition qualified and measured, but logging and
coroutine regressions prevent selecting it. Selected57 retains only the shared
dictionary provider and callback-lifetime repair; its small broad mean benefit
comes with explicit coroutine/raytrace and generator debt. Golden56 stays frozen.
The [report](../comprehensions57/report.md) and
[tables](../comprehensions57/benchmark-tables.md) own current comparisons.
