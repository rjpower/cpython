# Development process

Updated 2026-10-31. This file defines procedure and gates. Current assignments,
dependencies, next actions and landing state live in the [work index](issues/README.md)
and each issue's `status.md`. [Documentation rules](DOCUMENTATION.md) define
where designs, plans, reviews and results belong. Existing work records remain
valid at their linked paths; new records use the issue layout.

The previous combined plan/status/procedure is preserved as a
[historical snapshot](archive/process-2026-10-31.md), including its scope
rulings, compatibility decisions and qualification history. Its §0 is no longer
a current task queue. Existing references to §0.1/§0.3 retain their historical
meaning; new follow-ups use linked issues. Current support limits are in the [GC design](design/work/gc-architecture-next.md)
and the linked landing records.

## Standing constraints

- The coordinator owns integration; implementation agents own bounded changes
  and separate worktrees. Independent reviewers check significant changes.
- Never amend, rebase, reset or force-push committed history. Fixups are new
  commits. Do not use any form of `git stash`; its stack is shared by worktrees.
  Published topic branches remain retained after local worktree retirement.
- Before implementing an intentional compatibility break, the coordinator records
  its scope, reason and rejected alternative in the linked design/decision record.
  Document the resulting behavior in the applicable API or library reference.
- Surface automatic approval-review denials with the action and stated reason;
  do not route a denied action through another agent.
- Integration tests primarily check useful liveness/reclamation, readable data,
  usable resources, callback effects and memory growth over a declared interval.
  Critical rollback, root handoff and ownership unit invariants remain appropriate.
- Preserve original failures and actual source, build, mode and evidence identities.
  A passing predecessor or component does not become a successor/union pass.
- Use finite, source-frozen validation campaigns. Costly repetition needs a named
  hypothesis, exposure budget and stopping rule. Do not restart whole ladders for
  unchanged source. Applicable gates and scoped qualification exceptions remain
  recorded in the issue's results; a status transition grants no waiver.
- Keep large builds and retained evidence in project storage. Retire worktrees
  only after preserving unique commits and auditing live evidence references.
- Full suites and perf-map work serialize: test_perf_profiler removes shared
  `/tmp/perf-*.map` files. Other eligible jobs use the existing two test slots.

## 1. The loop

Design, review, implement and test, landing review, commit, integrate,
quick-test. Each rung goes through all of it.
The separate files below suit substantial changes. For a small change, the
issue's `status.md` can hold the design, plan and result; link the actual input
to review rather than creating empty companion files.

1. **Design.** The agent writes a short design for the rung in
   `boil/issues/<id>/design.md`: what changes, why, the invariants
   it relies on, what it deletes, the risks, the tests and measurements
   that prove it, and the file-ownership list (section 2). Half a page to
   two pages. A step-level design note (`boil/design/NN-*.md`) is the
   specification; the work doc says how this rung realises it and records
   the decisions the note left open.
2. **Review.** `boil/tools/review.sh boil/issues/<id>/design.md
   [context files]` sends the doc to a peer reviewer (codex, model
   gpt-6.1-sol at xhigh reasoning, read-only sandbox over the repository)
   and writes `boil/issues/<id>/reviews/design-review.md`. Preserve that output
   under a unique round name before invoking the helper again. The
   agent addresses every blocking issue in the doc (a change or a written
   rebuttal under a "Review" heading) before writing code. The codex run
   has a 40-minute ceiling; past it, or on any failure, the script sends
   the same prompt to a cold claude session (Opus, xhigh effort,
   read-only tools) and the review file names its reviewer. Run the
   review in the background, never under a foreground tool timeout. At
   most two rounds; the coordinator rules on what is still open.
   Non-blocking issues are addressed or carried to linked follow-up issues. The
   review is advisory; the coordinator rules on disagreements.
3. **Implement and test.** Small commits on the step branch, messages
   starting `boil: `, each preceded by gate 1 on the rung's targeted set.
   Never amend, rebase or force-push anything once committed; fixups are
   new commits. No attribution trailers. Subagents may do mechanical
   conversions and never commit; the step agent reviews and commits their
   work. The rung ends with the gate ladder of section 3 through level 3
   and the as-built record in the issue's `results.md`: what was built against what
   was designed, every deviation with its reason, the gate lines each
   with the commit it was produced at, the measurements against the
   before records, the hazards found and how each was traced, the
   linked follow-up issues with an owner per item. Before the landing branch the
   agent rereads every new or changed test and writes in the record what
   failure each would not detect; a test records its evidence at the
   event (the interpreter a callback runs on, the allocation that
   failed, the order two things happened in), not at setup, and asserts
   the invariant, not a symptom of the mechanism (7a rung 1's landing
   review found three tests that passed on the wrong behaviour). Every
   safety test names its negative control in the record (the span left
   out, the allocation made to fail, the traverse edge removed, the
   callback routed to the wrong owner) and the record shows the control
   failing; a safety test without a control is a gap, not a pass
   (review 2 §7: the checkers and the tests agreed on the same mistake
   three times).
4. **Landing review.** The coordinator runs `boil/tools/landing_review.sh
   CHECKOUT BASE HEAD RESULTS.md` on a detached checkout of the landing
   branch's head; the reviewer reads the as-built record, the gate lines
   and the diff against the integration head, and the script writes
   `boil/issues/<id>/reviews/results-landing.md`, committed on the
   landing branch. RESULTS.md links the governing design and plan; the review
   brief includes those contracts. Preserve the helper's output under a unique
   round name before invoking it again. The reviewer
   reads what was built, not the design: deviations without reasons,
   gate lines produced before a code change, hazards dismissed rather
   than traced, deleted invariants, tests that assert less than the
   design promised or that would pass on the wrong behaviour. Blocking findings go back to the agent as new commits
   on the rung's landing branch; the rung lands when the review has no
   open blocking finding or the coordinator has ruled on each. Each
   round's review file is kept (`-landing.md`, `-landing-2.md`), never
   overwritten, and the record closes with a disposition table in the results record: every
   blocking finding against its fixing commit, its written rebuttal or
   its linked follow-up issue.
5. **Commit and integrate.** The coordinator merges the rung's landing
   branch (section 2) into the integration branch with a merge commit,
   resolving conflicts by regenerating generated files and keeping both
   sides of records, and never by hand-editing generated code. Design
   notes, the worklog and the hack register are updated on the step
   branch as part of the rung, so landing carries the record; the
   coordinator adds a short outcome with results links to `boil/WORKLOG.md`
   and updates the issue status and work index.
6. **Quick-test.** On a detached checkout of the merge commit with fresh
   builds: gate 0 on both builds, gate 1 on the rung's targeted set, and
   the collector probes the rung added. The full gate 2 on default and
   gate 3 on debug run on the same checkout when the merge resolved a
   conflict in code, when more than one branch landed since the last full
   run, when the rung touched the collector, the roots or the object
   layout. Existing source-identical qualification may be reused only through
   an explicit, independently reviewed binding; a calendar day alone does not
   require another full campaign. The integration branch is pushed when
   the quick-test passes. A failure is a fixup commit on the step's
   branch, merged again; the merge commit is never amended.
7. **Whole-branch review at a step's end.** When a step completes (not
   each rung), the coordinator runs the whole-project review of
   `boil/design/reviews/architecture-review-2-prompt.txt`, updated to the
   head, on a detached checkout (codex at xhigh, read-only, a 2.5-hour
   ceiling, in the background) and files it as a loom artifact and under
   `boil/`. Rung-local review reads a diff; the two reviews so far found
   defects a diff cannot show (the copy-propagation fold across a call,
   the sweep's heap-order fallback, the checker's evaluation-order
   assumption). Its confirmed findings become linked issues the same
   day.

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
- Shared with every worktree: the git stash stack (never use `git stash`
  in any form), the mimalloc and uv caches, and the machine. A test run and a
  rebuild of the same build directory never overlap: `gate.sh` holds a
  lock per build directory.
- Machine admission uses `/tmp/boilgate-slot.N`, regardless of a command's
  `TMPDIR`. The coordinator fixes the shared launcher to use that global path.
  Frozen older launchers must leave outer `TMPDIR` unset and set any private
  target scratch directory inside the gate. Do not change a running cohort's
  launcher; take shared tooling at the next safe source handoff. Historical
  private-slot tests retain their actual results and admission limitation.
- Every gate command runs under `timeout` in its own process group with a
  ceiling per level (`GATE_LIMIT` overrides), and the build's python
  processes are swept after each command, so test workers die with the
  run even when the shell that started it is killed. Run gates 2 and 3 in
  the background and poll; a foreground tool timeout that kills the shell
  is how workers were orphaned before. Never run the suite by hand
  outside `gate.sh`; if you must, wrap it in `timeout`. Edit nothing
  under `Lib`, `Modules`, `Tools` or the generated tables while a gate
  runs from the same tree; draft outside the tree and apply after.
- Retire a checkout only after checking dirty files, preserving unique commits
  and auditing evidence references. Use ordinary `git worktree remove PATH`;
  if it refuses, investigate instead of forcing removal. The existing
  `worktree.sh drop` uses forced worktree/branch deletion and is not the
  retirement procedure. Retain published branches and evidence-required checkouts.

## 3. The gate ladder

Run the cheap host traversal audit on a frozen composition before broad
native suites whenever types, traversal, acquisition ownership or its allow
metadata changes. Resolve missing edges and stale allowances first. Use the
existing checker through uv; do not wait for the final closure batch to find
source-only failures. An exact-input early receipt may carry into closure;
repeat it only if relevant source/checker/allow inputs change. An allowlist-
only correction does not invalidate unchanged runtime profiles: preserve their
actual source identities and bind the narrow successor explicitly.

| Level | What | When |
|---|---|---|
| 0 | build the tree, `python -c pass`, a smoke set of test files (grammar, opcodes, builtin, dict, list, tuple, class, generators, capi) | after every change that builds; a minute |
| 1 | the rung's targeted test files (`gate.sh 1 BUILD test_a test_b ...`) | before every implementation commit; documentation-only changes use document review |
| 2 | the full suite on the release build; the root counters summed with no stack-walk fallback | at each milestone within a rung |
| 3 | the full suite on the debug build, `-u cpu test_cext`, `make regen-cases clinic` leaving the tree unchanged, `Tools/boil/traverse_audit.py --check` (every type with references and no or a partial traverse is in `Tools/boil/traverse_allow.txt` with a reason), the root counters with no fallback; and for a rung that touches the collector, the roots, the object layout or the eval loop, the verification set: the full suite on debug under `PYTHON_GCVERIFY=kill`, the ASan build on the hazard list of note 11 section 8.2 under kill, `PYTHON_GCSTRESS=1000` on the stress set, and the affected eval-loop root controls, with additional repetition justified by a specific failure hypothesis | at the rung end, before the landing review |
| 4 | the internal benchmark set recorded (`boil/bench/run.py`, label = the step) and the measurement section written with attribution | at the step end |

Modules this machine does not build (`_ssl`, `_hashopenssl`, `_decimal`,
`_sqlite3`, `_bz2`, `_zstd`, `_uuid`) are type-checked against borrowed
headers with `boil/tools/check_unbuilt.sh` (`gcc -fsyntax-only`; 6d rung
4 found a PyValue assigned to a `PyObject *` in `_hashopenssl.c` this
way) at gate 3 of every rung that edits `Modules/` or a slot typedef;
`_tkinter` stays unchecked for want of Tcl headers and its callers are
read by hand.

The verification set's exclusions (a test that cannot run under kill,
ASan or stress) live with their reasons and owners in note 11 section 8.2
until 10a rung 2 moves them into a file under `boil/tools/` that `gate.sh`
reads; until then the set runs through `gate.sh` levels 1 and 2 with the
environment variables set, as 10a rung 1b did.

Fast feedback builds (user-authorized, 2026-10-29): an existing debug
build may be replaced at a quiescent checkpoint by `--with-pydebug`
with `CFLAGS='-O1 -g'`, retaining assertions, symbols and the same root
verification modes. Record actual effective flags, compiler and commit;
do not label it the old `-Og` build. First validate the negative checker
fixtures, compiled-stack attribution, kill/stale-root controls and the
affected tests, since optimization changes stack roots and unwind data.
Measure build-plus-test turnaround before adopting it for routine gates
0/1. Release gates and performance records still use release builds;
ASan and the tail-call/walk matrix remain required where applicable.
This does not waive a gate, an assertion or a failing test. The existing
`-Og` debug configuration remains the baseline until that qualification
is recorded; avoid duplicating all build trees merely to try a flag.

Coordinator ruling, 2026-10-29 17:50 PDT: select O1 plus Py_DEBUG as
this machine's debug configuration for the next coherent GC/IR candidate.
Keep the existing f2/e9 Og trees and receipts frozen; do not rebuild them
merely for a flag comparison. Bind actual compiler commands and effective
flag order (configure's CFLAGS can follow OPT), Py_DEBUG, assertions and
unwind data. Fresh negative/compiled-root/stale controls and normal/kill
qualification are required before accepting that candidate. Historical O1
fulls took about607/617s versus current Og718/702s, but source/load differ:
this supports a turnaround experiment, not a calibrated speedup claim.
Do not add a second Og full solely to compare scheduling configurations.

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
Intermediate measurements are recorded without unrelated tuning (note
06, plan revisions of 2026-10-27): a rung records instruction, allocation and box counts
against the before records, attributes any change it did not design, and
leaves what a later step throws away.
Checkpoint I publishes pre-JIT interpreter performance and prompts design
reflection where results disappoint; roughly 2× is soft guidance, not a
numerical admission gate. See the [current plan](design/work/gc-architecture-next.md). Levels 2 and 3 accept
`--python WORKER_COMMAND` for a verifier-asserting regrtest worker;
other extra arguments are rejected to preserve full-suite coverage.

## 4. Reporting and tracing

The agent reports to the coordinator every 30 minutes or at every rung
end: first line `<step> rung N: <state>`, then commits since the last
report (SHA and one line), gate results as counts with the commit each
was produced at, what is next, and anything to decide. Times come from
`date`. Blocked more than 15 minutes: message the coordinator. Each step
has a clock deadline per rung and a hard stop; at the hard stop the agent
commits the completed work, records any remaining follow-up issues, and reports.
The coordinator verifies claims against `git log` and the gate output, nudges
after 45 minutes of silence, runs the landing review and the quick-test,
writes a short outcome with evidence links in `boil/WORKLOG.md`, updates
the issue status and index, and keeps the hourly check.

A test that fails once and passes on rerun is traced, not dismissed. Under
the live collector a collection can land anywhere, so a timing-dependent
failure is usually a missing root, a missing traverse or a callback run
at the wrong moment; on 2026-10-27 three such "flakes" were real bugs
(test_buffer: a missing traverse; test_io and test_bdb: the callback
queue). Record the test, the assertion and the rate in the work doc's
results, and read the code it exercises before calling it unrelated.
Repeat only with a named hypothesis, exposure budget and stopping rule;
there is no automatic ten-run campaign.

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
