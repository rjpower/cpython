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
revised 2026-10-29 18:11 PDT. Last qualified runtime merge is 052d8342e56;
local f2a5 GC predecessor is held; combined successor5a45f9bb6a7
prepares fresh O3/O1-debug qualification in boil-pre-i-union. Shared
tooling e02e177903f, verification launcher 2db451b1b65, v2 identity/result
component 9dd24bf42c4, worker setup 093b9d2a1c3 and
the active step worktrees. This update separates implemented work
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
   landed as 052d8342e56 (note 13); dependent-call completion remains open.
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

PROCESS.md section 0 is the position of record and is updated at every
landing and hourly during active work. This is the takeover audit as of
2026-10-29 18:11 PDT; evidence is from local commits and retained logs.

The coordinator accepts the independently reviewed scoped ROOTS2b exception:
the inherited HTTP/finalizer-progress failures stay explicit final GC and
performance blockers. The successor tail/extras checks now pass. ROOTS2b and the reviewed
bytes-writer3a compose as detached candidate f2a5; its O3/Og compilations
complete and all 16 early jobs pass. Release coverage is assembled from
563 valid original module cohorts plus one corrected-environment whole
test_cmd_line; the original full stays failed. Both debug fulls pass51,134
tests, including actual kill verification, with zero observed fallback.
Release stress11 then fails one stack-hit diagnostic assertion (tuple versus
Probe); debug stress, ASan and extras have not started. Qualification is held
for diagnosis, with no automatic retry. The original stress100k full remains
failed; there is no all-seven-PASS claim. The minimal reader/test repair7c703
clears independent source review. H3 integration3 now also passes; reviewed
Unicode ownership2f37 and finalizer eligibilitya285 compose with H3 and the
repair in successor5a45. Its native qualification is unstarted; f2 receipts
remain at their actual identities. PROCESS records the scheduling rulings.

| Done | In flight | Open |
|---|---|---|
| 1, 2a, 2b, 3a, 3b: the IR, the compiler passes, the handlers; 7a rung 1 owner routing merged as 052d8342e56 and integration-validated | | 7a rung 2a safety/completion order: design/probe artifacts preserved, agent held by automatic classifier; 2b owner metadata, 2c retirement/adoption/fork |
| 4, 5a, 5b, 5c: the collector; refcounts deleted | 10a rung 2b native branch remains ce2ac, executed source c679/docs handback837; f2a5 predecessor is held; combined H3/GC successor5a45 awaits qualification. The precise-Park repair and two raw-mutex classifications clear review and all three builds. Fresh affected24, three full normal/kill runs, both stress11 sets and ASan16 pass with zero fallback. Final stress100k fails with an interpreter assertion, GC timeout and HTTP unraisables; four focused diagnostics support trigger-dependent test eligibility. HTTP/trashcan public controls establish inherited exposure; scoped incremental exception accepted with final GC/perf debt retained. Tail11/0, cext16/seven declared skips, unchanged regen/traversal and19 headers pass; exact union source review clears. Fresh union early16 pass; release coverage is assembled563+1 with original env-failed full retained; debug normal/kill fulls pass51,134 each; release stress11 fails one stack-hit diagnostic assertion; debug stress/ASan/extras stopped pending diagnosis | 10a 3a remaining writers/PyMem, 3b checker/precision residue, 3c queued buffer release and detached-state roots; rung 4 measurements/platform support |
| 6a, 6b: `PyValue` in the core and the value C API; compiler reentrancy fix merged as 99435658509 and integration-validated | | 6e: remaining slot families, inline numeric receivers, identity and pointer-bound contracts |
| 6c: the object protocol; 6d: its selected slot families on values (complete 2026-10-28); H3 generator stage1 merged as 6c724c29408 and integration-validated | H3 stage2a domain/pristine runtime changes pass bounded release/debug and actual-kill gates; frozen at 624c75f6455 and cleared for stage2a only; native landing follows ROOTS2b. Stage2b.1 names/inspection/NOFB frozen at c5cf6ba6d288 cleared independent review. Fixed-int/uint16 monitoring storage native76274/evidenceffc548603b8 is pushed and clears frozen-checkpoint review; actual primary union qualification follows ROOTS. Stats implementation in isolated boil-h3-stats from81417 clears independent source review; owned build and bounded lifecycle/reader/generator checks pass, private controls pass. Actual H3+GC source union e9a1 clears composition review and Py_STATS real producer/reader checks pass. Owned O3/Og builds and eight bounded smoke/H3/stale jobs pass; integration3 also pass, and combined successor5a45 now awaits fresh qualification, with transport later | H1 handler/build rules, H2 root maps after 10a rung 3, H3 typed operations and dispatch; 7 generations; I interpreter performance checkpoint before 8 tier 0; 9 tier 1; 10b moving collector; P ports |
| 10a rungs 0, 1, 1b; audit-1, audit-2 (traverse completeness) | | audit-3 (coordinator, after 7a rung 2) |

Shared validation prerequisite landed independently as `2155f16f45f`:
`_testinternalcapi.gc_verify_mode()` reads actual collector state.
Integration's existing release/debug smoke sets passed 1,924 tests each,
and its mode contract passed on both builds. Queue rung 1 cleared independent
review: full kill 51,104, ASan14 3,589, stress11 3,046, subprocess 371 three
times and 40 probes pass at their recorded revisions. Its integration
passed smoke 1,924 and queue tests 22 per build, actual-worker-kill 22 and
unchanged regeneration. Roots final gates remain unlanded.

Performance position, instructions on the internal benchmark set
(`boil/bench/results/6d.json` over `baseline.json`; recorded, not tuned):
kernels 1.161, microbenchmarks 0.693, memory workloads 1.699. These historical
instruction ratios do not establish current elapsed-time performance.
The first release elapsed diagnostic on unlanded ce2ac now measures0.828×
upstream throughput across29 kernels (about21% more elapsed time):22 slower
rows/seven faster. Coroutines, comprehensions and generators regress most;
float, Richards and nbody are the strongest wins. All rows and raw-sample
limits are in the [timing record](design/work/pre-I-first-quiet-timing-plan.md).
This initial diagnostic has unresolved GC stress failures and source/build
comparison limits; it establishes neither correctness nor the2× target.
Generations, 6e and the handler/interpreter work precede checkpoint I;
the JIT follows it. The target is roughly 2× upstream interpreter throughput
on the declared application/kernel set, measured with release wall time and
individual regressions reported and resolved or explicitly dispositioned.
Micro instruction gains alone do not establish that result.

[Four bounded native profiles](design/work/pre-I-shared-cost-attribution.md)
now measure collection at54%/47%/31% of coroutine/comprehension/generator hot
time, versus zero hot collections in Richards. This prioritizes shared GC
work, without proving generation savings. A small finalizer-order eligibility
patch is implemented in isolation at a285. Its [O3/Og semantic checks](design/work/pre-I-finalizer-order-checkpoint.md)
and copied reversal negative pass their intended oracles. One matched four-row
diagnostic measures1.250/1.055/1.070/1.033 parent/candidate elapsed ratios for
coroutines/comprehensions/generators/Richards. Forty raw samples and limits
are in that checkpoint; this is not a refreshed upstream/29-row score. Actual
combined qualification remains. Nonmoving generations has a reviewed
direction but needs the closed store/constructor/domain ledger before minors.
The [revised roadmap](design/work/pre-I-performance-roadmap.md)
sets the measurement contract, GC landing path and structural performance work.

The [writer readiness audit](design/work/pre-I-gc3a-writer-readiness.md)
now has a bounded bytes-writer implementation on isolated branch
boil/gc3a-bytes at f5e818b0454: the embedded owner root and lifecycle fixture
clear coordinator source and actual-result review. Release52/10 declared skips,
O1-debugkill52/zero skips, stress writer38 per build and ASan writer38 pass;
both omitted-registration cases fail the intended root-class assertion.
All completed positives report zero fallback. Integration remains due; earlier
private admission and invalid releasekill limits stay in the linked review
`design/work/reviews/gc3a-bytes-writer-1-checkpoint.md`. Unicode ownership2f37
now clears independent source review and composes into5a45; compilation and
lifecycle/root qualification remain due. The [receiver audit](design/work/pre-I-inline-receiver-audit.md)
separates immediate calls, getsets and bound-method storage; no new ABI is
approved. H3's [owner checklist](design/work/H3-owner-readiness.md) is ready
for the actual integrated-source handoff.

The pre-JIT backlog includes H1 frame passing, numeric guards and build
policy; 6e inline-receiver boxing; H3 exact-float truth and call-window
placement with root/liveness contracts; range-spill attribution; and a
string/compiler design for linear concatenation with an alias/escape proof.
PROCESS §0.4 assigns these explicitly. None is deferred automatically to
the JIT, and scalar-loop lowering remains a separate unapproved design.

The compiler correction and queue rung 1 are landed. Active agents finish
10a rung 2b and H3's handler domain, with independent review at each boundary.
The queue rung 2 agent was stopped by an automatic cybersecurity classifier,
including its read-only handoff; no native rung 2 fix was applied. Existing
artifacts are preserved. This is not a missing user authorization.
The coordinator owns integration, shared gate policy, dispatch/performance
scope and hourly documentation/status. PROCESS §0.4 gives the dependencies.
ROOTS2b fixes both lost PyMem-origin attribution and list-to-tuple element
root lifetime. Original-helper and old-lifetime negative controls remain
recorded. Its multiprocessing cleanup repair runs existing finalizers after
forkserver stop and before tracker stop; actual-helper, old-order and visible
natural-case controls pass. No skip hides those cleanup failures.

ROOTS2b native products remain at `ce2ac8991ee`, executed test/docs source
c679 and documentation handback837. Local union f2a5 includes bytes3a; fresh
qualification is held at the new stack-hit diagnostic failure. The precise-Park repair removes stale
alignment-padding retention, and two raw-mutex precision annotations close
ASan's linked-audit failure. Release, O1-debug and ASan builds pass. The
native/CFI/root-location comparison and four-semaphore supplement clear
independent review. The old1bfe campaigns each completed120 transactions;
the coordinator accepts them as historical reliability evidence under that
reviewed bridge. They are not new-head results and will not be repeated20
times automatically. The full causal history and original failures remain
in WORKLOG and the rung records.

Fresh affected qualification passes all24 transactions across release off,
debug off and debug kill:243 regrtest case-runs/three tail-only skips, plus
1,800 stale-root attempts across six cases and three modes. Release normal,
debug normal and debug kill each pass51,132 tests, with zero fallback.
Both stress11 sets pass3,062 tests; filtered ASan16 kill passes3,976. Fresh
tail make and six strict stale100 cases also pass. These are source-bound
branch results, not primary landing or performance results.

**Inherited final-GC blocker:** the branch stress100k full is terminal FAIL: an assertion in
interpreters' test_destroyed_by_gc, a20-minute test_gc.test_trashcan_threads
timeout and ENV_CHANGED in httpservers with two HTTPResponse unraisables.
Receipt64448666 retains native exit2 and50,722 reported tests; the timed-out
worker prevents a qualified aggregate root-counter result. No new filter or
automatic full retry follows. Source inspection
and four once-only diagnostics support a threshold-trigger mismatch: original
stress fails/off passes, copied stress charges no root phase/off charges one.
The documented stress override stays unchanged; an exact test-eligibility
annotation passes its normal case and declared stress skip. This is not
demonstrated post-collection retention. Unchanged HTTP103/8 reproduces two finalizer errors; the separate
Python observer identifies a response created by the colorized logging error
test, then times out. Its partial timeline does not establish the buffer
closer or queue cause. Four unchanged HTTP cases reproduce the same error on
primary052 and ce; the unchanged trashcan case times out on both at the bounded
diagnostic deadline. They establish inherited exposure, not identical causes
or absence of interaction. Independent review and the coordinator permit an
incremental ROOTS landing while retaining both issues as final GC/performance
debt. The [triage record](design/work/10a-2b-stress100k-triage.md) preserves
all failures. Successor tail walk11/0, cext16/seven existing skips, unchanged
regeneration/traversal and19 headers pass, and exact source review clears.
The fresh merged default/debug/kill ladder remains. Fulls and perf-sensitive walks
serialize because their perf-map cleanup interferes.

The39 protected selectors and actual worker mode/stress/counter contracts stay
in force. Detached-state unpinned hits remain report-only debt assigned to3c.
The current frozen integration debug remains Og. The next coherent GC/IR
candidate will use O1 with Py_DEBUG, assertions/symbols and fresh compiled
root-walk, negative, stale, normal and kill qualification. This changes the
next build configuration, not the identity of these completed Og gates. Whole-tail
and retain recommendations belong to10a rung4/H1, separately from the bounded
tail check required for2b. Writer/PyMem and remaining root-reader work follow
in10a rung3. The step-end benchmark remains distinct from this landing gate.

H3's monitoring storage design now uses fixed absolute int lines and uint16
handler tokens. Bounded probes confirmed that valid backward AST locations
break the old line codec. Corrected design84e518cb872e and exact fixture71ec68cb7da7
clear independent review, including all five decoder callers, the fork's
test-hook ABI and allocation-safe snapshots. Implementation and its bounded
qualification clear independent frozen-checkpoint review. Actual native union
qualification and primary landing review remain.
Development line/membership/OOM controls pass;
the no-monitoring-flags bookkeeping correction clears source review at
manifest54b2832334af. Its broader gate then exposed an inherited refresh loop
when valid flagged code executes RESUME after a monitoring-version change.
That failed run is retained. An independently reviewed eligibility prerequisite
guards the version-mismatch refresh, direct refresh and handled-exception
callback paths. It does not acknowledge versions or write shared static code.
The combined candidate is frozen at manifest a6c07b4ce7b2 for requalification;
earlier evidence is not relabeled as final qualification. The exact same
watchdog probe fails on the old release candidate and passes on the combined
candidate, with ordinary and suppressed callbacks distinguished; broader
combined release smoke1,924/affected1,370 pass. Independent source review
clears a6c07b4ce7b2, including the three guards and generated expansions.
Copied-helper narrowing/alias controls fail their named semantic assertions;
debug smoke1,924/affected1,370 and actual-kill1,370 also pass. Regeneration,
checker/traversal and19 optional syntax checks complete. Frozen candidate
ffc548603b8 is pushed and clears independent evidence review. It predates queue052;
its results do not qualify the queue/ROOTS/H3 union. Merge/requalification waits
for ROOTS2b, preserving the existing native landing order. The source-only
[handoff preflight](design/work/H3-post-ROOTS-handoff.md) identifies five shared
paths requiring semantic composition and regeneration even without textual
conflicts. Prospective serial order: storage union, statistics, H1 frame passing,
then H3 transport/renumbering. This schedules ownership; it adds no H2 dependency
or native implementation grant.
The next statistics design81417aff097 clears its revised independent review:
raw entries and semantic events remain separate, with dispatch-mode-specific
prediction counting and producer metadata. It is design only; no next native
change precedes the integrated-source handoff.
The H1 frame-passing design clears
[bounded readiness review](design/work/reviews/H1-1-design-readiness.md),
with source/build handoff required before implementation.
The private API/layout decision is recorded in PROCESS section0.2.
The pre-I [string design](design/work/pre-I-linear-string-concat.md) now has a
[reviewed S0 proof/owner prerequisite](design/work/pre-I-linear-string-proof-owner.md).
The first REVISE and subsequent S0_READINESS_CLEAR are preserved in its
[review](design/work/reviews/pre-I-linear-string-concat-design-review.md).
It specifies compiler fixed points/final-word checks, runtime epoch transitions
and immutable code-owned metadata installed before CREATE. Cross-owner interface
review and a bounded mutation-disabled S1 plan remain required. The
[Unicode storage/ABI audit](design/work/pre-I-string-storage-audit.md) remains
source analysis; storage and cross-owner interfaces need separate review.
No string implementation is approved; monitoring-storage landing remains pending.
The source-only [baseline binding plan](design/work/pre-I-baseline-binding.md)
confirms that the old upstream executable's configured library root is the
active fork. Preserve its historical results; a separately owned exact-upstream
source/build/import environment and stronger runner identity are required
before checkpoint-I measurements. One owned exact321925 detached source tree
is now clean and inventoried. The revised configure runner closes its admission,
tool and cleanup gaps; configure succeeds and actual configuration closure8908
clears independent reviewde24. Make-only V3 succeeds; its 584-product closure
dddc clears independent review5344. The diagnostic/origin and single18-module
qualification now pass and clear independent review9bac:111 recorded modules,
100 traced imports, three valid wrong-origin controls correctly rejected, and
5,145 tests/109 transcript skips. The584 native products remain unchanged.
All50 actual benchmark-driver setup/run origin rows now pass and clear
independent actual-scope review, closure9e725b7f, with unchanged inputs. The
[first quiet timing plan](design/work/pre-I-first-quiet-timing-plan.md) and
its paired adapter have completed the first bounded elapsed diagnostic:
all29 rows/290 samples, geometric mean0.828243 upstream/fork. Independent
review clears its actual scope, retaining the failed-stress/unlanded and
source-policy limitations. Native before/after/cleanup maps agree; no retry.
Common compiler/O3/frame/
zero-initialization policy matches; upstream's eval-unit flags are empty while
the fork deliberately adds `-fno-gcse`. Report that source-specific difference;
upstream flag sensitivity or a predeclared fully matched eval policy is required
before claiming parity. This comparison cannot isolate object-model effects.

Shared gate cleanup now holds the build lock until its sweep finishes;
an interrupted waiter cannot sweep another run's workers. The controlled
process checks pass, with both deliberately broken versions failing.
Loom and GitHub use are explicitly authorized; integration through
02bcd2f6132 and the queue/H3 heads above are pushed. That prerequisite
exposes actual stress state and restores the prior interval after the
reftracer fixture; integration smoke passed 1,924 tests per build, focused
state tests passed on both, and debug kill/stress checks passed. The
verification registry, validator and plan display are landed as
fff4e0e9b08: 15 profiles, seven safety-control groups and actual case
inventory validation pass. Execution and numeric-gate integration remain
unfinished. The launcher component landed as 2db451b1b65 after independent review;
it does not establish orchestration, complete coverage or source snapshot
enforcement. The v2 identity/result prerequisite is landed; profile
orchestration remains pending. The controlled052 rebuild passes smoke1,924/20skips,
with 6,188 unchanged source inputs and zero fallback. Its initial105-entry
receipt missed extension target bytes and is superseded. The strict repair
and independently reviewed172-entry supplement bind all67 targets as well as
aliases, without a new build. All seven bounded v2 positives now pass: off/kill,
whole context/weakref modules, separate repetitions, one-worker cext and the
auxiliary launcher. Exact existing source skips are recorded. Seven isolated
failure controls and
11 evidence-corruption checks also pass. Independent review cleared the
component, landed as9dd24bf42c4. No named/full profile or run/gate hook is claimed. Shared worker-launch correction
093b9d2a1c3 preserves regrtest's temporary cwd and exit timeout by calling
worker.main().

Opcode categories will use explicit generated properties. H3 removes
inherited numeric category cutoffs such as HAVE_ARGUMENT; instruction
numbering must not imply argument, specialization or monitoring behavior.
Stage1 preserves existing IDs only to prove a coherent migration before
consumer changes and final renumbering. Its worktree reuses
`.worktrees/boil-compiler-copyprop` on branch `boil/h3`; the compiler
landing ref stays frozen.

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
  attribution. Checkpoint I separately requires pre-JIT interpreter
  parity; an unresolved deficit cannot simply be deferred to the JIT
  (PROCESS.md §§0.4 and 3).
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
