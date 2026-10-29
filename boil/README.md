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
revised 2026-10-29 07:45 PDT at integration merge 6c724c29408, shared
tooling e02e177903f, verification registry fff4e0e9b08 and
the three active step worktrees. This update separates implemented work
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
   implemented on the unlanded 7a branch (note 13). Roots are precise
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
2026-10-29 07:45 PDT; evidence is from local commits and retained logs.

| Done | In flight | Open |
|---|---|---|
| 1, 2a, 2b, 3a, 3b: the IR, the compiler passes, the handlers | 7a rung 1 at 5cc31bacee1: normal suites pass 51,102 each at d43c8b4dd32; full debug kill passes 51,104 with zero fallback; ASan14 passes 3,589 with its exact exclusion; three subprocess runs pass 371 each; stress11/probes and review remain | 7a rung 2a safety/completion order, 2b owner metadata, 2c retirement/adoption/fork; design and probes only |
| 4, 5a, 5b, 5c: the collector; refcounts deleted | 10a rung 2b at 0b13048a633: provenance repair and list-transfer lifetime fix committed as 82165f8a2a9, integration reconciled; bounded O1 checks pass; first complete reclamation iteration running; final checks and review pending | 10a 3a remaining writers/PyMem, 3b checker/precision residue, 3c queued buffer release and detached-state roots; rung 4 measurements/platform support |
| 6a, 6b: `PyValue` in the core and the value C API; compiler reentrancy fix merged as 99435658509 and integration-validated | | 6e: remaining slot families, inline numeric receivers, identity and pointer-bound contracts |
| 6c: the object protocol; 6d: its selected slot families on values (complete 2026-10-28); H3 generator stage1 merged as 6c724c29408 after independent clearance | H3 stage1 detached integration checks running; stage2 fixed-number runtime/monitoring/stats/inspection consumers beginning under explicit ownership | H1 handler/build rules, H2 root maps after 10a rung 3, H3 typed operations and dispatch; 7 generations; I interpreter performance checkpoint before 8 tier 0; 9 tier 1; 10b moving collector; P ports |
| 10a rungs 0, 1, 1b; audit-1, audit-2 (traverse completeness) | | audit-3 (coordinator, after 7a rung 2) |

Shared validation prerequisite landed independently as `2155f16f45f`:
`_testinternalcapi.gc_verify_mode()` reads actual collector state.
Integration's existing release/debug smoke sets passed 1,924 tests each,
and its mode contract passed on both builds. The queue forwarding fix
and roots final gates still belong to their unlanded lanes.

Performance position, instructions on the internal benchmark set
(`boil/bench/results/6d.json` over `baseline.json`; recorded, not tuned):
kernels 1.161, microbenchmarks 0.693, memory workloads 1.699. The plan
expects generations (7) and tier 0 (8) to recover the kernel and memory
costs; that is a plan, not evidence. The user now requires a pre-JIT
performance checkpoint: matched release wall-time measurements must
establish interpreter parity or better on the kernel set, with individual
regressions reported and resolved or explicitly dispositioned. Micro
instruction gains alone do not establish that result.

The compiler correction is landed. Three independent agent lanes now
close the queue landing, finish 10a rung 2b, and implement H3's handler
domain. The coordinator owns integration, shared gate
policy, dispatch/performance scope and hourly documentation/status.
PROCESS §0.4 gives the dependencies and checkpoint rules. The `-O1`
debug trial reduced smoke time from 3m41s to 2m37s, but qualification
stopped at a PyMem-attribution oracle failure in `test_verify_kill`.
Logging confirmed that a precise carrier root scans a PyMem block before
its core-frame alias; the block's second origin is then lost. Death and
reachability assertions behave correctly. The proposed fix enriches
post-mark provenance without changing collection policy. The existing
oracle must retain all distinct origins; its assertion must accept PyMem
as either the first or a supplemental origin on the same report line.
Further O1 checks exposed a real list-to-tuple lifetime bug: clearing the
list before tuple allocation removes its element roots. A minimal reorder
is committed in rung 2b, brought forward from rung 3a. All ten fresh
reproductions pass after the fix; the original failed ten of ten.
`-O1` has not been adopted. Integration
debug validation still uses `-Og`. Release measurements and sanitizer
coverage remain required.

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
unfinished; the roots agent is preparing a separate launcher checkpoint.

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
