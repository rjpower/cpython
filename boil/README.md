# Boil the ocean: start here

A performance fork of CPython (upstream base `321925c92c9`, main of
2026-10-23) that replaces the execution substrate wholesale: a
fixed-width register IR produced by the compiler, one NaN-boxed value
type in every frame and container, shapes in the object header, and a
tracing collector with no reference counting. Tiers 0 and 1, a moving
collector and a ports tree of rebuilt packages follow once the C API has
settled. The user's goal statement is the Loom artifact `goal`; the
first analysis and plan is the artifact `plan`.

This file is the hand-over point. It says what the design is, where the
work stands, which document is authoritative for what, and how a change
is made. It is kept current at every landing by the coordinator. Last
revised 2026-10-30 after the main-only GC integration. Latest runtime merge
is **75e8a1e0c01**, integrating qualified source4da292b1294 over safetyff6.
Qualification, closure and both post-merge smokes pass; final12f1ea31 and
postmergef56e0b6b retain the exact evidence and source/native bridges. Shared
tooling e02e177903f, verification launcher 2db451b1b65, v2 identity/result
component 9dd24bf42c4 and worker setup 093b9d2a1c3 are landed.
Current focus is interpreter performance: the independently reviewed
[matched baseline](design/work/pre-I-post-GC-timing.md) measures0.8243× upstream
throughput. Fresh [cost attribution and experiment decisions](design/work/pre-I-gc-policy-and-experiment-decisions.md)
prioritize selective death processing and archival of H1/H2 for this milestone.
This update separates implemented work
from landed work; prior delivery dates are superseded by PROCESS §0.4.

## 1. The design in one page

The pipeline (note 06 sections 1 to 5; the two whole-project reviews
draw it as mermaid diagrams, `boil/ARCHITECTURE_REVIEW_2.md` sections 2
and 3):

1. **Compiler.** `compile.c`/`codegen.c` build the usual stack-shaped
   CFG, which `regcfg.c` converts to a register CFG; `regcfg_passes.c`
   runs the register passes (copy propagation, folding, compaction; note
   07); `assemble.c` emits the IR words, the feedback initialisers, the
   line and exception tables. There is no runtime lowering from
   bytecode.
2. **IR.** One 64-bit word per instruction: a 16-bit handler and four
   12-bit fields naming frame slots. Registers are frame slots. The code
   object holds the immutable words; `ir.c` makes the mutable executing
   copy and the feedback vector that specialisation and instrumentation
   write (`specialize.c`, `instrumentation.c`).
3. **Handlers.** `bytecodes.c` is the source; `generated_ir_cases.c.h`
   is generated from it and regenerated on every merge. The eval loop in
   `ceval.c` dispatches by handler index.
4. **Values.** `PyValue` (`Include/pyvalue.h`): exact int32 and non-NaN
   doubles inline, everything else an object pointer below 2^48. Frames,
   lists, tuples, dict entries, cells, instance slots and call argument
   arrays hold `PyValue`. The `PyObject *` C API remains as a
   compatibility layer that boxes on the way out (note 06 section 3.5,
   note 12).
5. **Objects.** The header is one word: shape pointer and flags. Shapes
   carry the attribute layout; instance attributes are positional slots;
   `__dict__` materialisation converts the instance to a terminal
   dictionary mode (note 09, `shapeobject.c`, `lookupplan.c`). Type and
   descriptor slots take values (note 12).
6. **Memory.** mimalloc heaps with an object-start map (`obmalloc.c`);
   a full-heap mark and sweep (`gc_trace.c`, `gc_live.c`) with a
   referrer-first sweep for deallocators that read their referents;
   weak references in a side table; finalizers and destroy hooks queued
   and drained at permitted execution points. Owner-correct routing is
   landed as 052d8342e56 (note 13). The supported main interpreter now uses
   one rooted drainer and acquisition readiness for ordinary buffer release;
   foreign-owner adoption and parallel callback progress are deferred.
   Roots are precise
   where frames
   are marked `Py_GC_PRECISE` and checked by the C-mode checker, and
   conservative elsewhere (an attributed DWARF stack walk, PyMem blocks,
   data segments; note 11). Verification modes: `PYTHON_GCVERIFY=retain`
   and `kill` compare a strict root policy against the retained one;
   `PYTHON_GCSTRESS=N` collects every N allocations.

What was deleted: tier 2 and the copy-and-patch JIT, the stack
interpreter, per-opcode inline caches, object reference counts, free
lists, the cycle collector, split dict tables, managed dict pointers.

## 2. Where the work stands

Current checkpoint: **2026-10-30, integrated75e8**. PROCESS section0 records the
plan and admission rules; WORKLOG preserves the gate history. This summary
distinguishes source implementation, validation and primary landing.

**Revised GC scope.** The first milestone supports one main interpreter per
process, with ordinary threads and the GIL. Subinterpreters are deferred;
their creation must fail explicitly before allocating interpreter state.
The landed runtime enforces early rejection, declares its capability and
keeps unsupported creation tests visibly skipped. Use explicit
process workers for isolation;
do not replace interpreter APIs transparently with subprocesses. Defer foreign
owner/adoption and interpreter-deletion machinery from this milestone.
The [holistic review](design/work/reviews/gc-holistic-simplification-review.md)
and [independent second look](design/work/reviews/gc-holistic-simplification-second-look.md)
favor retaining the nonmoving collector and hybrid roots, simplifying callback
delivery, and separating actual process exit from embedding finalization.
Ordinary buffer lifetime remains a correctness obligation. Generic native
cleanup slots and a completion DAG are not required by the qualified finite
managed-owner/readiness policy; broader lifecycle machinery remains deferred.

The latest runtime merge is **75e8a1e0c01**, integrating qualified4da:
runtime library/tests dac6, ordinary native3c6 and ASan nativef945 retain
their actual identities. The merge differs from4da only in15 declared
project Markdown paths. Both O1-debug normal/kill fulls pass51,240 tests;
release/debug stress11, sanitizer coverage, explicit release/ASan assemblies
and closure clear independent review. Post-merge release/debug smokes pass
1,935 tests with exact80/50 skips and zero fallback. The
[landing review](design/work/reviews/gc-main-only-final-landing.md) records
the finite support boundary, original failures and historical ASan limits.
Supported callback/buffer lifetime is closed for this milestone; precision,
foreign lifecycle and additional platforms remain separate obligations.

Predecessor safetyff6 integrated the sweep-OOM, snapshot/debug-output and
work-stack/MARKED rollback repairs. Its six original passes, corrected debug
stress composite, closure and1,930-case smokes remain at their true
native168/test9e identities in the [safety review](design/work/reviews/gc-safety-union-landing.md).

The previous qualified runtime merge is **d6a23b9fe0c**; the compiler reentrancy
repair is landed99435658509. Its frozen candidate **5a45f9bb6a7**, branch
boil/pre-i-union in `.worktrees/boil-pre-i-union`, combines ROOTS2b, bytes
and Unicode writer ownership, H3 handler/storage/statistics work, the
stack-hit diagnostic repair and finalizer-order eligibility optimization.
Independent review approves this incremental landing under the retained GC
exception. The integration differs from the candidate only in19 project
Markdown documents. Release and O1-debug post-merge smoke each pass1,930
reported tests, reusing the qualified code-identical products explicitly.
This lands the increment, not final GC correctness or interpreter parity.

| Work | Current evidence | Remaining |
| --- | --- | --- |
| Register IR/compiler, NaN-boxed values, shapes, tracing without refcounts | Core implementation landed; compiler reentrancy repair qualified | 6e remaining slot/receiver/value bridges; interpreter performance target |
| GC/IR integration5a45 → d6a23 | All19 affected jobs, both ASan groups, all three corrected fulls, two stress11 sets and closure audits pass. Independent LAND_INCREMENTAL review and release/debug post-merge smoke pass. Original failed profiles remain preserved | Later75e8 closes supported completion/resource ordering; precision/platform work remains separate |
| Three GC safety repairs and supported-runtime closure | Qualified4da/runtimeDAC6/native3c6+ASanf945 integrated75e8. Final index12f1 and postmergef56e bind all profiles/composites, closure and1,935-case smokes. Original release/traversal failures remain failed | Supported Linux/main-interpreter milestone complete. Keep foreign/adoption, platform and precision work distinct; see the [closure plan](design/work/gc-closure-plan.md) and [independent LAND review](design/work/reviews/gc-main-only-final-landing.md) |
| H3 handler domain, monitoring storage and statistics | Included in qualified d6a23. Actual Py_STATS producer/reader checks pass on predecessor e9; fresh5a45 checks pass1,376 cases in each of three modes, followed by combined qualification | Later transport/renumbering and typed operations |
| H1 dispatch/frame passing | Isolated native62456/tool-test94ab; audits/CFI,103 runtime cases per build and strict6×100 stale controls succeed. Later review finds informational platform-header errors in both combined logs; retain that harness limitation beside the case/root evidence. Original audit/cleanup failures and corrections remain explicit. Static code changes are mixed | Selected for archival for current GCC milestone; affects Clang tail dispatch only. Reopen a bounded matched comparison only for that active backend; no speedup measured |
| H2 physical liveness/root maps | Inert decoder5df336 declares27 supported/126 explicitly unsupported pristine handlers. Test-parser fixb3e6847, corrected release generator module79 cases and debug-kill eight modules748/10skips succeed; zero fallback | Selected for archival: no immediate consumer. Reuse the tested patch for a scheduled usable liveness slice; no runtime maps or clearing changes |
| Nonmoving generations | Reviewed direction plus allocator, frame, container and runtime-store ledgers | Remaining type/shape/native/iterator/root coverage, concrete barrier/promotion/young-index/reentry interfaces and independent oracle; no minors or barriers implemented |
| JIT, movement and ports | Deferred under the current plan | Interpreter checkpointI first; subsequent API/handle/ABI and package migration work |

The19 affected jobs are a declared coverage set, **not19 repeated full
suites**: four stack-hit checks, two Unicode/consumer runs, two smoke runs,
three H3 runs, two whole-GC runs, three integration walks and three strict
stale-root runs. The stale checks retain their existing cheap six-times100
inner hazard loop per mode; there is no automatic20-iteration campaign.
See [the roadmap](design/work/pre-I-performance-roadmap.md) for why costly
repetition now requires a named hypothesis, exposure and stopping rule.

Both whole-GC runs skipped two byte-trigger tests because the launcher set
truthy string `PYTHON_GCSTRESS=0`, although the actual getter reported0.
Those original skips remain recorded. Corrected normal fulls omit the variable,
assert actualN0 and require both tests unskipped; all three fulls recover them.
The f2 predecessor's stress11 tuple/Probe diagnostic failure remains failed.
The reader/test repair fixes confirmed lifetime assumptions; its one empty
prefix diagnostic did not establish that failure's cause. The earlier
stress100k interpreter/HTTP/trashcan failures also remain preserved.

The first5a45 release full remains **FAIL**, with two failed modules.
The launcher disabled bytecode writes and placed its cache across a filesystem
boundary from temporary test sources. This breaks pydoc/compileall and skips64
bytecode-writing import cases. Its later four profiles did not start.
The corrected environment permits bytecode writes and uses a disposable /tmp
cache. All three fresh fulls pass, requiring all64 previously suppressed
cases and both byte-trigger cases to execute. Original native products remain
unchanged; generated cache additions are recorded separately.

H2's original release selection also remains failed: an existing test parsed
two generated tables into one dictionary. The focused test-only correction
passes its old-parser negative control. Seven unaffected release modules are
carried forward explicitly, with the corrected whole generator module; debug
runs all eight modules. Native binaries are unchanged. These bounded results
qualify the inert slice, not complete liveness semantics or activation.

**Performance.** The [post-GC matched29-row diagnostic](design/work/pre-I-post-GC-timing.md)
measures **0.824299× upstream throughput**, equivalent to about21% more elapsed
time. Twenty-one rows are slower and eight faster. Coroutines0.449,
comprehensions0.510 and generators0.574 remain large deficits; float2.039,
Richards1.865 and nbody1.668 are strong wins. Both runtimes have matched
GCC/O3/eval flags and checked workload origins. Five serial samples give an
initial estimate; several fork rows have substantial spread. Keep all rows
and raw values. Fresh counter windows put GC at47%/54%/30% of the three
slow allocation/resume rows. A native profile points to heap classification,
sweeping and repeated cleanup eligibility checks. Selective death processing
is the first code candidate; budget sensitivity remains a diagnostic before
new ABI or generational work.

The historical first29-row release elapsed diagnostic on unlanded ce2ac
measured **0.828243× upstream throughput**, about21% more elapsed time;
22 rows were slower and seven faster. Coroutines0.365, comprehensions0.528
and generators0.592 were the largest regressions; float1.923, Richards1.897
and nbody1.603 the strongest wins. [Raw results and limits](design/work/pre-I-first-quiet-timing-plan.md)
include fork-only eval-fno-gcse, origin/SDK limits and unresolved correctness.
Historical6d instruction ratios1.161/0.693/1.699 are not elapsed performance.
Roughly**2× is soft guidance**, especially for obvious arithmetic, not a
required application aggregate or numerical gate. Supported GC is landed;
the detailed interpreter report and independent reflection now guide clean
general improvements, including parameterized operation families.
Keep individual regressions, RSS and pauses visible with all JITs off.

Historical [four profiles](design/work/pre-I-shared-cost-attribution.md) put collection
at54%/47%/31% of coroutine/comprehension/generator hot time and zero in
Richards. That motivates refreshing shared GC costs, including death processing;
it does not predict generations savings. The small finalizer-eligibility
patch passes semantic and ordering-negative controls. Its [first matched
four-row diagnostic](design/work/pre-I-finalizer-order-checkpoint.md) gives
parent/candidate elapsed ratios1.250/1.055/1.070/1.033. All40 values remain;
process/layout/sample limits and Richards' no-hot-GC control prevent complete
causal attribution. This supported its earlier broad qualification, not a refreshed29-row
score, upstream parity or the2x target.

**Next work, in dependency order:**

1. The supported GC union is qualified and integrated75e8, including early
   main-only admission, one rooted drainer, retained unready acquisitions,
   sealed consumers, same-owner queue/channel views and ordinary SharedMemory
   disposal. Preserve its products and explicit composite evidence. Remaining
   root precision, foreign/adoption and platform obligations stay separate;
   passing suites do not authorize removing roots or claiming arbitrary ports.
2. The provisional [interpreter source report](design/work/pre-I-interpreter-status-and-opportunities.md)
   inventories seven shared opportunities. A source-only comparison is also
   published. The matched upstream eval-nogcse comparator has passed its build,
   18-module correctness and both29-row workload origin sets. The matched release
   diagnostic is complete; no proposed interpreter ABI is implemented. The [first-slice comparison](design/work/pre-I-first-slice-decision.md)
   selects by current shared cost; selective collector death processing now
   precedes a numeric NOARGS ABI expansion. H1 and the inactive H2 patch are
   selected for archival, with no runtime merge justified now. The prepared
   matched Clang dispatch/H1 experiment can reopen for that backend; later inspect
   moves/spills/code size and real root ownership. H3 categories use generated
   properties, never numeric opcode thresholds. Keep coherent operations and
   calling conventions; no benchmark-specific fusion target.
3. When liveness is the selected slice, reuse archived H2 work to produce and
   validate maps from final checked IR before changing roots or
   clearing. Its [readiness contract](design/14-handler-contract.md) covers
   exceptions, suspension, aliases, early results, unpack and introspection.
   Producer work can proceed while reader/span contracts are closed.
4. Close the [generations design](design/work/7-nonmoving-generations-design.md)
   with [allocator](design/work/7-generations-domain-ledger.md),
   [frame](design/work/7-generations-frame-store-ledger.md),
   [container](design/work/7-generations-container-store-ledger.md) and
   [runtime](design/work/7-generations-runtime-store-ledger.md) ownership.
   Then implement the young index, owner barriers and independent omission
   oracle before enabling minors, retaining a finite major-collection policy.
5. Remove demonstrated value/receiver boxing and common frame/call costs;
   improve call windows and linear string growth only under their root,
   initialization and alias/escape proofs. Re-measure each coherent change.

**Integrated supported GC:** source4da/runtimeDAC6/native3c6+ASanf945 is
qualified and integrated75e8. Main-only admission, the rooted drainer, unready
buffer retention and the finite consumer/resource ports are no longer a
pending composition. ND keeps original metadata separate from transforms;
same-owner queue/channel receipt uses an ordinary managed memoryview.
SharedMemory automatic disposal preserves outstanding views while explicit
close stays strict. Final12f1 and postmergef56e retain exact product and source
identities, both51,240-case debug fulls and all actual composite scopes.

The unknown ambient in-source build remains preserved in the old checkout;
no payload from it is used. Original scanner, release, traversal and launcher
failures remain failed. The [owner ledger](design/work/10a-buffer-safeguard-removal.md)
names private ND reinit policy, live Python-buffer-wrapper re-export debt and
platform limits. Passing cycle tests do not qualify unrelated routes or
arbitrary external native holders.

The [interpreter report](design/work/pre-I-interpreter-status-and-opportunities.md)
records the implemented architecture, isolated H1/H2 and seven shared
opportunities. The matched upstream eval-nogcse build,18-module correctness,
both29-row origin sets and matched release timing are complete. The current
diagnostic is0.824299× upstream throughput; no numeric NOARGS ABI is implemented.
Acceptance favors liveness, valid data,
resource reuse and bounded memory; critical unit invariants remain separate.

Shared verifier/worker components are landed; full profile/gate orchestration
is still separate work in `boil/tools/verification.md`. All jobs retain real source/config/product,
actual mode/stress, skip and failure identities. Fulls and perf-sensitive walks
serialize because test_perf_profiler removes shared `/tmp/perf-*.map` files.

## 3. The documents, and what each is authoritative for

| Document | Authoritative for |
|---|---|
| `boil/README.md` | this: the starting point |
| `boil/PROCESS.md` | the plan of record and position (§0), cleanup inputs from the reviews with owners (§0.1), intentional compatibility breaks (§0.2), the hand-off ledger (§0.3), the loop (§1), worktrees and branches (§2), the gate ladder (§3), reporting and tracing (§4) |
| `boil/design/06-architecture-v2.md` | the architecture (sections 1 to 5), the step plan as first written (section 6), and one decision section per landed step (sections 9 to 21): what was built, what deviated, measurements |
| `boil/design/07` to `14` | step design notes: 07 compiler passes, 08 the review at 6a and the hack register (section 3: every expedient kept, with its owner), 09 the object protocol, 10 the internal benchmark set and the ports deferral, 11 precise roots, 12 object slots on values, 13 per-interpreter queues (lands with 7a rung 1), 14 the handler contract |
| `boil/design/reviews/` | peer reviews of the design notes; the whole-project review prompt |
| `boil/design/work/<step>-<rung>.md` | one work doc per rung: design, review response, as-built record, gate lines with commits, hazards traced, hand-offs |
| `boil/design/work/reviews/` | each rung's design review (`-review.md`) and landing review (`-landing.md`), kept per round |
| `boil/WORKLOG.md` | the gate entry of every landing, chronological, with the test counts, collections and fallbacks |
| `boil/ARCHITECTURE_REVIEW_2.md` | the current whole-project review (2026-10-28 at 5f74a991472); an input to the plan through PROCESS.md §0.1, never a gate. Each step-end review replaces it; the previous one's open items live on as §0.1 rows |
| `boil/bench/` | the internal benchmark set (`run.py`, `compare.py`, `results/<step>.json`), gate 4 of every step |
| `boil/tools/` | `gate.sh` (the ladder), `gate_run.sh` (the lock and the two machine-wide test slots), `review.sh` and `landing_review.sh` (the codex reviewers), `worktree.sh` (new, push, land, drop), `check_unbuilt.sh`, `code_stats.py`, `measure/` |
| `Tools/boil/traverse_audit.py`, `Tools/cases_generator/root_check_c.py` and `call_effects.py`, `Tools/build/gc_inline_audit.py` | the checkers the gate runs: traverse completeness, precise-frame roots, inlining of precise functions |
| Loom artifacts `goal`, `plan`, `architecture`, `status-2026-10-28`, `architecture-review-2` | the user's goal, the first plan, the architecture summary, the latest readout, the latest whole-project review |

Reading order for a successor: this file; PROCESS.md in full; note 06
sections 0 to 6; the design note of each step in flight (11, 12, 13);
the open work docs and their reviews; the last five WORKLOG entries;
ARCHITECTURE_REVIEW_2.md sections 5 and 7.

## 4. How a change is made

The loop (PROCESS.md §1): design in a work doc, peer review by codex with
at most two rounds, implement in small gated commits on a step branch,
landing review on a detached checkout, a `--no-ff` merge into
integration by the coordinator, a quick-test of the merge on a fresh
detached checkout, and a whole-project review at each step's end. One
implementation agent per step owns a worktree and a branch; the coordinator owns
integration and PROCESS.md. Every brief carries a clock deadline per
rung, a hard stop, a test ladder (small first), a report every 30
minutes and an explicit scope cut.

The coordinator's standing duties: an hourly check (`date`, `git log`,
`git ls-remote`, the agents' gate logs); nudge an agent silent for 45
minutes; verify every claim against the commit and the gate output; push
an agent's committed head when it is ahead of the remote (fast-forward,
explicit refspec); land rungs; re-gate each landing on the detached
checkout `/tmp/boilgate` (current builds are integration's
`build/integration-default` and `build/integration-debug`, keeping large
outputs in project storage);
write the worklog gate entry and PROCESS.md §0; set the Loom status;
give the user a one-page readout with a process assessment when asked,
and run the whole-project review at each step end.

## 5. Rules that bite

- Never amend, rebase, reset or force-push anything once committed.
  Fixups are new commits. The coordinator pushes hourly and merges
  `--no-ff`; a rewritten history breaks the fast-forward.
- No `git stash` of any kind: the stash stack is shared across
  worktrees and sessions. No attribution trailers in commits.
- One principled form. No fallback paths, option flags for two
  behaviours, variable-width encodings or dual representations; a fixed
  form with a measured limit (the 4,095-slot shape limit, the 12-bit
  field) is the pattern. A design that proposes a fallback is sent back.
- Intermediate gates establish correctness and record performance with
  attribution. After GC fixes, checkpoint I reports interpreter status and
  prompts design reflection on deficits. Roughly2× is soft guidance;
  neither parity nor2× is a mandatory numerical gate (PROCESS §0.4).
- Tests are the oracle: a safety test names its negative control and the
  record shows the control failing; evidence is recorded at the event,
  not at setup; the test asserts the invariant, not a symptom.
- A test that fails once and passes on rerun is traced, not dismissed
  (PROCESS.md §4 gives the tracing order and the tools).
- Any change to compiler or specialiser output, the handler set or the
  feedback layout bumps the pyc magic in the same commit. Every test run
  uses a fresh `PYTHONPYCACHEPREFIX` (`gate.sh` does this).
- Never rebuild a build directory while a test runs on it; edit nothing
  under `Lib/`, `Modules/`, `Python/` or `Tools/` in a worktree while a
  gate runs from it (check `pgrep -af build/` first). Two breaches on
  2026-10-28, both recorded.
- A hand-off between steps is a row in PROCESS.md §0.3 with the evidence
  that closes it, ruled by the coordinator; a compatibility break is a
  row in §0.2 before its rung starts.
- Generated files are regenerated on merge, never hand-edited.
- The permission classifier refuses remote branch deletion; merged
  landing branches stay on the remote and the user is told. An agent
  denied an action is never done for it by the coordinator; the denial
  is surfaced to the user.

## 6. The machine

- Repository `/home/power/code/cpython`; integration worktree
  `.worktrees/python-boil-the-ocean` on `weaver/python-boil-the-ocean`;
  step worktrees `.worktrees/boil-<step>` on `boil/<step>` (made with
  `boil/tools/worktree.sh new <step> <base>`); remote
  `https://github.com/rjpower/cpython.git`, pushed with `gh`'s
  credential helper (`worktree.sh push`).
- Builds live in `build/<name>` under each worktree: `default`, `debug`,
  `asan`, `tailcall`; `build/*/config.log` has the configure lines.
- `boil/tools/gate_run.sh` limits the machine to two concurrent test
  runs (`GATE_SLOTS`; lock files `/tmp/boilgate-slot.N`). A third rung
  queues. Plan rung starts around that.
- Modules this machine does not build: `_ssl`, `_hashopenssl`,
  `_decimal`, `_sqlite3`, `_bz2`, `_zstd`, `_uuid`; they are
  type-checked by `check_unbuilt.sh`. gdb is absent.
- `/tmp` is shared: test_perf_profiler can fail another run's
  `test_pre_fork_compile` (PROCESS.md §0 standing concerns).
- ASan runs with `detect_stack_use_after_return=0:detect_leaks=0`
  (`gate.sh` sets it): fake stacks hide locals from the conservative
  scan.
- Reviews use `codex exec -m gpt-6.1-sol -c model_reasoning_effort=xhigh
  --sandbox read-only` on a detached checkout, in the background under
  `timeout`; `review.sh` falls back to a cold Claude session past 40
  minutes.

## 7. Taking over

First hour: read this file and PROCESS.md; run `git log --oneline -20`
on integration and `git ls-remote` for every `boil/*` branch; read the
last WORKLOG entry; list the agents' worktrees and their heads; read
PROCESS.md §0's standing concerns and §0.3's open rows. Then message each
running agent for a report (PROCESS.md §4 says what it contains), and
set the Loom status. Check actual dirty files and branch ancestry rather
than assuming a clean tree or a completed review. At this takeover note
14 had a draft response and three review files were untracked; other
worktrees retain their own dirty design notes. Some final 10a results
exist only in retained logs and must be reconciled into its work doc.
