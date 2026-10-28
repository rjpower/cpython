# The plan, our position in it, and how a change is made

Status: in force from 2026-10-27 (user ruling); revised 2026-10-28 on the
user's direction (landing review, single branching point, standing
verification gates, the rerun rule, file ownership). This file is the
working record of the plan and the procedure. Note 06
(`boil/design/06-architecture-v2.md`) holds the design and the per-step
decision sections; the work docs under `boil/design/work/` hold each
rung's design, review and as-built record; `boil/WORKLOG.md` holds the
gate entries. The coordinator (the orchestrating session) owns the
integration branch and this file; each step's agent owns a worktree and a
branch.

## 0. The plan and where we are

The programme turns CPython into a register-IR interpreter over NaN-boxed
values with a tracing collector and no reference counting, then adds
tiers 0 and 1 and a moving collector, then ports the packages. Steps are
numbered as in note 06 section 6; the summary below is the position of
record and is updated at every landing.

| Step | What | State (2026-10-28) |
|---|---|---|
| 1, 2a, 2b, 3a, 3b | the 64-bit word IR, the compiler passes, the handlers | landed |
| 4, 5a, 5b, 5c | the collector: verifier mode, live mode, refcounts deleted | landed |
| 6a, 6b | `PyValue` in the core and the value C API | landed |
| 6c | the object protocol (note 09): shape in the header, positional slots, dictionary mode as a shape, split tables deleted | landed, gated; hand-offs: FOR_ITER_RANGE spill and nbody +5.3% to step 8, `str +=` to step 9 |
| 6d | the remaining object slots on values (note 12): rung 1 class versions, capacity rule, poly stores, lazy-dict loads; rung 2 attribute and descriptor slots; rung 3 `callfunc` and wrapperfunc; rung 4 getsets | rung 1 landed; rung 2 gated, landing; rung 3 in progress; hard stop 2026-10-31 12:00 PDT, cut order getsets then the lazy-dict handler |
| 10a | precise roots (note 11): the attributed DWARF walk, the C-mode checker, kill and retain verification, the stack-hits tool | rungs 0, 1, 1b landed (29a358b492d); rung 2 (the call layer and ceval helpers) in progress; rungs 3 (PyMem spans, kill with pymem and structs) and 4 (gate 4, write-up) open; hard stop 2026-11-02 15:30 PDT |
| 7a | the per-interpreter callback and finalizer queue (finding B of note 08; note 11 question 18): callbacks and `__del__` run on the interpreter that owns the object, never during another interpreter's init | moved up from step 7 on 2026-10-28: a collection during a shared-obmalloc subinterpreter's init runs the main interpreter's finalizers on it and the process dies; design note next, after 10a rung 2 |
| 6e | the remaining slot families on values; inline int and float receivers | open, after 6d |
| 7 | generational collection over the precise root set | open; decides header flag bits and side bitmaps |
| 8 | tier 0 over the IR, x86-64; first frees the specialised-handler space | open |
| 9 | tier 1 | open |
| 10b | moving collector: nursery, evacuation, compaction, handle discipline | open |
| P | the ports phase: Cython, cffi, pyyaml, numpy on the settled API | open, after 10b |
| B | the internal benchmark set (note 10) | done; the record for every gate 4 |

Standing concerns, with owners: conservative-retention artefacts in tests
until 10a rung 2 marks the C API entry paths (trace with the stack-hits
tool, fix with a span and a directive); per-rung instruction deltas of 1
to 2% on the kernels are within the register-allocation noise of the
single eval-loop function and are recorded, not acted on, until step 8;
test_ssl, `_decimal`, `_uuid` and `_sqlite3` do not build on this machine
and gdb is absent (user action); `PYTHONMALLOC=malloc` is not honoured
(hack register, owner 10a rung 2).

## 1. The loop

Design, review, implement and test, landing review, commit, integrate,
quick-test. Each rung goes through all of it.

1. **Design.** The agent writes a short design for the rung in
   `boil/design/work/<step>-<rung>.md`: what changes, why, the invariants
   it relies on, what it deletes, the risks, the tests and measurements
   that prove it, and the file-ownership list (section 2). Half a page to
   two pages. A step-level design note (`boil/design/NN-*.md`) is the
   specification; the work doc says how this rung realises it and records
   the decisions the note left open.
2. **Review.** `boil/tools/review.sh boil/design/work/<step>-<rung>.md
   [context files]` sends the doc to a peer reviewer (codex, model
   gpt-6.1-sol at xhigh reasoning, read-only sandbox over the repository)
   and writes `boil/design/work/reviews/<step>-<rung>-review.md`. The
   agent addresses every blocking issue in the doc (a change or a written
   rebuttal under a "Review" heading) before writing code. The codex run
   has a 40-minute ceiling; past it, or on any failure, the script sends
   the same prompt to a cold claude session (Opus, xhigh effort,
   read-only tools) and the review file names its reviewer. Run the
   review in the background, never under a foreground tool timeout. At
   most two rounds; the coordinator rules on what is still open.
   Non-blocking issues are addressed or carried to the hand-off list. The
   review is advisory; the coordinator rules on disagreements.
3. **Implement and test.** Small commits on the step branch, messages
   starting `boil: `, each preceded by gate 1 on the rung's targeted set.
   Never amend, rebase or force-push anything once committed; fixups are
   new commits. No attribution trailers. Subagents may do mechanical
   conversions and never commit; the step agent reviews and commits their
   work. The rung ends with the gate ladder of section 3 through level 3
   and the as-built record in the work doc: what was built against what
   was designed, every deviation with its reason, the gate lines each
   with the commit it was produced at, the measurements against the
   before records, the hazards found and how each was traced, the
   hand-off list with an owner per item.
4. **Landing review.** The coordinator runs `boil/tools/landing_review.sh
   CHECKOUT BASE HEAD WORKDOC` on a detached checkout of the landing
   branch's head; the reviewer reads the as-built record, the gate lines
   and the diff against the integration head, and the script writes
   `boil/design/work/reviews/<step>-<rung>-landing.md`, committed on the
   landing branch. The reviewer
   reads what was built, not the design: deviations without reasons,
   gate lines produced before a code change, hazards dismissed rather
   than traced, deleted invariants, tests that assert less than the
   design promised. Blocking findings go back to the agent as new commits
   on the rung's landing branch; the rung lands when the review has no
   open blocking finding or the coordinator has ruled on each.
5. **Commit and integrate.** The coordinator merges the rung's landing
   branch (section 2) into the integration branch with a merge commit,
   resolving conflicts by regenerating generated files and keeping both
   sides of records, and never by hand-editing generated code. Design
   notes, the worklog and the hack register are updated on the step
   branch as part of the rung, so landing carries the record; the
   coordinator adds the gate entry to `boil/WORKLOG.md` and the position
   in section 0 of this file.
6. **Quick-test.** On a detached checkout of the merge commit with fresh
   builds: gate 0 on both builds, gate 1 on the rung's targeted set, and
   the collector probes the rung added. The full gate 2 on default and
   gate 3 on debug run on the same checkout when the merge resolved a
   conflict in code, when more than one branch landed since the last full
   run, when the rung touched the collector, the roots or the object
   layout, and at least once a day. The integration branch is pushed when
   the quick-test passes. A failure is a fixup commit on the step's
   branch, merged again; the merge commit is never amended.

## 2. Worktrees and branches

- Integration branch: `weaver/python-boil-the-ocean`, checked out in
  `/home/power/code/cpython/.worktrees/python-boil-the-ocean`. Only the
  coordinator commits there (merges, worklog gate entries, notes, this
  file).
- One branching point. Every concurrent step branches from the
  integration head, and after each landing every live step branch merges
  the new integration head once, as its next commit, before further
  work. Step branches never merge each other; a step that needs another
  step's unlanded work waits for the landing, or the coordinator lands
  the dependency first. (6d rung 2 merged boil/10a directly on
  2026-10-28 to unblock; that is the exception that produced this rule.)
- Step branch and worktree: `boil/tools/worktree.sh new <step>` creates
  `.worktrees/boil-<step>` on branch `boil/<step>` from the integration
  branch, and `worktree.sh build <step>` configures `build/default` and
  `build/debug` in it (`CC=gcc`, and `--with-pydebug`). The agent never
  touches another worktree or its build directories.
- Landing branch per rung. When a rung's gate 3 passes, the agent creates
  `boil/<step>-rung<N>` at that commit and pushes it; the landing review,
  any fixups and the merge happen on that branch, while the next rung
  continues on `boil/<step>`. After the landing, `boil/<step>` merges the
  integration head (the branching-point rule).
- File ownership. The work doc of every rung lists the files and
  generated tables the rung will edit. Two concurrent rungs never edit
  the same file; when one needs a change in the other's file (a table
  entry, a span, a directive), it records the request under a hand-off
  heading the owner can find (`<thing>: hand-off to <step>`) and the
  coordinator relays it. A checker-appeasing form written to avoid a
  hand-off is not kept; it is recorded and removed once the owner's
  change lands.
- Shared with every worktree: the git stash stack (never use bare `git
  stash`), the mimalloc and uv caches, and the machine. A test run and a
  rebuild of the same build directory never overlap: `gate.sh` holds a
  lock per build directory.
- Every gate command runs under `timeout` in its own process group with a
  ceiling per level (`GATE_LIMIT` overrides), and the build's python
  processes are swept after each command, so test workers die with the
  run even when the shell that started it is killed. Run gates 2 and 3 in
  the background and poll; a foreground tool timeout that kills the shell
  is how workers were orphaned before. Never run the suite by hand
  outside `gate.sh`; if you must, wrap it in `timeout`. Edit nothing
  under `Lib`, `Modules`, `Tools` or the generated tables while a gate
  runs from the same tree; draft outside the tree and apply after.
- `worktree.sh drop <step>` removes the worktree and branch after the step
  has landed.

## 3. The gate ladder

| Level | What | When |
|---|---|---|
| 0 | build the tree, `python -c pass`, a smoke set of test files (grammar, opcodes, builtin, dict, list, tuple, class, generators, capi) | after every change that builds; a minute |
| 1 | the rung's targeted test files (`gate.sh 1 BUILD test_a test_b ...`) | before every commit |
| 2 | the full suite on the release build; the root counters summed with no stack-walk fallback | at each milestone within a rung |
| 3 | the full suite on the debug build, `-u cpu test_cext`, `make regen-cases clinic` leaving the tree unchanged, `Tools/boil/traverse_audit.py --check` (every type with references and no or a partial traverse is in `Tools/boil/traverse_allow.txt` with a reason), the root counters with no fallback; and for a rung that touches the collector, the roots, the object layout or the eval loop, the verification set: the full suite on debug under `PYTHON_GCVERIFY=kill`, the ASan build on the hazard list of note 11 section 8.2 under kill, `PYTHON_GCSTRESS=1000` on the stress set, and twenty runs of each eval-loop probe | at the rung end, before the landing review |
| 4 | the internal benchmark set recorded (`boil/bench/run.py`, label = the step) and the measurement section written with attribution | at the step end |

The verification set's exclusions (a test that cannot run under kill,
ASan or stress) live with their reasons and owners in note 11 section 8.2
until 10a rung 2 moves them into a file under `boil/tools/` that `gate.sh`
reads; until then the set runs through `gate.sh` levels 1 and 2 with the
environment variables set, as 10a rung 1b did.

The rerun rule. A gate line is produced at a commit and names it. A code
change after the line invalidates it; the affected levels run again at
the new head. A change confined to tests or tooling reruns the levels it
can affect (gate 1 on the tests it touched, and the verification set if
it touched the verifier or the test helpers) and the record says what
the change touched and why the other lines stand.

Every level uses a fresh `PYTHONPYCACHEPREFIX`. Any change to compiler or
specialiser output, the handler set or the feedback layout bumps the pyc
magic in the same commit; an attribute change on the handlers that leaves
the numbering unchanged does not. The CPython suite is the correctness
gate; there is no ports gate until the ports phase (note 10).
Measurements are recorded, never tuned for (note 06, plan revisions of
2026-10-27): a rung records its instruction, allocation and box counts
against the before records, attributes any change it did not design, and
leaves what a later step throws away.

## 4. Reporting and tracing

The agent reports to the coordinator every 30 minutes or at every rung
end: first line `<step> rung N: <state>`, then commits since the last
report (SHA and one line), gate results as counts with the commit each
was produced at, what is next, and anything to decide. Times come from
`date`. Blocked more than 15 minutes: message the coordinator. Each step
has a clock deadline per rung and a hard stop; at the hard stop the agent
commits what is correct, writes the hand-off list, and reports. The
coordinator verifies claims against `git log` and the gate output, nudges
after 45 minutes of silence, runs the landing review and the quick-test,
writes the gate entry in `boil/WORKLOG.md`, updates section 0, and keeps
the hourly check.

A test that fails once and passes on rerun is traced, not dismissed. Under
the live collector a collection can land anywhere, so a timing-dependent
failure is usually a missing root, a missing traverse or a callback run
at the wrong moment; on 2026-10-27 three such "flakes" were real bugs
(test_buffer: a missing traverse; test_io and test_bdb: the callback
queue). Record the test, the assertion and the rate in the work doc's
hazards, run it alone ten times, and read the code it exercises before
calling it unrelated.

Tracing order for a retention or refcycle failure (an object alive that
the test expects dead): `PYTHON_GCSTACKHITS=1` with
`_testinternalcapi.gc_stack_hits(addr)` first, called with the address
of the retained object (the no-argument form lists nothing), which names
the owning frame and offset of the conservative word that holds it; then `PYTHON_GCVERIFY=
retain` to show the object is retained rather than freed; then the fix,
which is a span over the parameter and `@gc_precise` or `Py_GC_PRECISE`
on the frame, proven by the same tool showing no hit (2026-10-28,
test_taskgroups: one stale word in the `gc.get_referrers` wrapper's
frame, fixed by a one-slot span and the directive; 2026-10-28,
test_inline_collection_skips_eval_loop at the 10a landing: a stale word
in the opaque frame of `_PyList_FromValues`, exposed by the merge's stack
layout, fixed by making the allocator entry precise). A probe's result
depends on the stack layout under the frames it exercises, so a landing
can re-expose one that passed on both branches. For a crash under
kill or stress: the trap type's record names the owner and the killing
collection; a death in `PyErr_FormatUnraisable` during another
interpreter's init is finding B (step 7a), identified by the C stack and
a probe naming the callback's interpreter, not by mechanism alone. An
attribution without that evidence is not accepted.
