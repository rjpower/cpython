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
revised 2026-10-29 19:07 PDT. Last qualified runtime merge is 052d8342e56;
local f2a5 GC predecessor is held; combined successor5a45f9bb6a7
is undergoing fresh O3/O1-debug qualification in boil-pre-i-union. Shared
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

Current checkpoint: **2026-10-29 19:07 PDT**. PROCESS section0 records the
plan and admission rules; WORKLOG preserves the gate history. This summary
distinguishes source implementation, validation and primary landing.

The last qualified runtime merge is queue owner-routing052d8342e56; the
compiler reentrancy repair is landed99435658509. The primary branch also
contains local, unqualified ROOTS/bytes mergef2a5. Its successor is frozen
**5a45f9bb6a7**, branchboil/pre-i-union in `.worktrees/boil-pre-i-union`.
That successor combines ROOTS2b, bytes and Unicode writer ownership, H3
handler/storage/statistics work, the stack-hit diagnostic repair and the
finalizer-order eligibility optimization. It has not landed or been pushed
as a qualified runtime.

| Work | Current evidence | Remaining |
| --- | --- | --- |
| Register IR/compiler, NaN-boxed values, shapes, tracing without refcounts | Core implementation landed; compiler reentrancy repair qualified | 6e remaining slot/receiver/value bridges; interpreter performance target |
| GC/IR integration5a45 | Fresh GCC O3, O1 Py_DEBUG and ASan builds complete. All19 affected jobs pass. At19:07, ASan16 passes3,983/92skips and Unicode7 passes1,037/24skips; zero observed fallback, report-only unpinned counts retained | Three full suites (release now running), two stress11 sets, gate3 closure and independent final review |
| H3 handler domain, monitoring storage and statistics | Included in5a45. Actual Py_STATS producer/reader checks pass on predecessor e9; its11 bounded jobs pass. Fresh5a45 H3 checks pass1,376 cases in each of three modes | Actual combined broad qualification; later transport/renumbering and typed operations |
| H1 dispatch/frame passing | Matched Clang21 computed-goto and preserve_none tail baselines built on5a45; both bounded runtime selections and stale-root checks succeed. Tail ownership case executes. Frame-argument patch committed62456cc on isolatedboil/h1-frame | Matched elapsed comparison, H1 native/CFI/spill/semantic validation; no H1 speedup measured |
| H2 physical liveness/root maps | Inert scalar-effect decoder passes source review:27 pristine handlers supported,126 explicitly unsupported; current clearing and root readers unchanged | Native decoder validation, complete final-word semantics/CFG/maps, independent compiler projection, all reader/initialized-span transitions before activation |
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
Those skips are retained as gaps. Future normal jobs omit the variable,
assert actualN0 and require both tests unskipped in the fresh full suites.
The f2 predecessor's stress11 tuple/Probe diagnostic failure remains failed.
The reader/test repair fixes confirmed lifetime assumptions; its one empty
prefix diagnostic did not establish that failure's cause. The earlier
stress100k interpreter/HTTP/trashcan failures also remain preserved.

**Performance.** The first29-row release elapsed diagnostic on unlanded ce2ac
measured **0.828243× upstream throughput**, about21% more elapsed time;
22 rows were slower and seven faster. Coroutines0.365, comprehensions0.528
and generators0.592 were the largest regressions; float1.923, Richards1.897
and nbody1.603 the strongest wins. [Raw results and limits](design/work/pre-I-first-quiet-timing-plan.md)
include fork-only eval-fno-gcse, origin/SDK limits and unresolved correctness.
Historical6d instruction ratios1.161/0.693/1.699 are not elapsed performance.
The target remains roughly**2× upstream with all JITs off** on the declared
application/kernel suite, with individual regressions, RSS and pauses visible.

[Four profiles](design/work/pre-I-shared-cost-attribution.md) put collection
at54%/47%/31% of coroutine/comprehension/generator hot time and zero in
Richards. That prioritizes shared GC work, including death processing;
it does not predict generations savings. The small finalizer-eligibility
patch passes semantic and ordering-negative controls. Its [first matched
four-row diagnostic](design/work/pre-I-finalizer-order-checkpoint.md) gives
parent/candidate elapsed ratios1.250/1.055/1.070/1.033. All40 values remain;
process/layout/sample limits and Richards' no-hot-GC control prevent complete
causal attribution. This supports broad qualification, not a refreshed29-row
score, upstream parity or the2x target.

**Next work, in dependency order:**

1. Qualify and review5a45 once, then land the incremental GC/H3 work if its
   gates support it. Keep inherited HTTP/finalizer-completion and queue-progress
   debt explicit: the accepted ROOTS2b exception does not finish GC correctness.
   Writers/PyMem residue, checker/readers, queued payloads and detached-state
   roots remain10a3a/3b/3c work; platform/SDK coverage remains10a4.
2. Measure matched Clang dispatch and the uniform H1 frame argument; inspect
   moves/spills/code size and real root ownership. H3 categories use generated
   properties, never numeric opcode thresholds. Keep coherent operations and
   calling conventions; no benchmark-specific fusion target.
3. Produce/validate H2 maps from final checked IR before changing roots or
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

Queue rung2's old probe/design handoff remains held by its automatic classifier;
that is not missing user authorization and no native fix is claimed. Existing
artifacts remain. Loom and GitHub use are authorized. Shared verifier/worker
components are landed; full profile/gate orchestration is still separate work
in `boil/tools/verification.md`. All jobs retain real source/config/product,
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
