# How a step is done: design, review, implement, gate, land

Status: in force from 2026-10-27 (user ruling). Applies to every step and
to every rung within a step. The coordinator (the orchestrating session)
owns the integration branch; each step's agent owns a worktree and a
branch.

## 1. The loop

1. **Design.** The agent writes a short design for the rung or step in
   `boil/design/work/<step>-<rung>.md`: what changes, why, the invariants
   it relies on, what it deletes, the risks, the tests and measurements
   that prove it. Half a page to two pages. A step-level design note
   (`boil/design/NN-*.md`) is the specification; the work doc says how
   this rung realises it and records the decisions the note left open.
2. **Review.** `boil/tools/review.sh boil/design/work/<step>-<rung>.md
   [context files]` sends the doc to a peer reviewer (codex, model
   gpt-6.1-sol at xhigh reasoning, read-only sandbox over the repository)
   and writes `boil/design/work/reviews/<step>-<rung>-review.md`. The
   agent addresses every blocking issue in the doc (a change or a
   written rebuttal under a "Review" heading) before writing code. The
   codex run has a 40-minute ceiling; past it, or on any failure, the
   script sends the same prompt to a cold claude session (Opus, xhigh
   effort, read-only tools) and the review file names its reviewer. Run
   the review in the background, never under a foreground tool timeout.
   Non-blocking issues are addressed or carried to the hand-off list. The
   review is advisory; the coordinator rules on disagreements.
3. **Implement.** Small commits on the step branch, messages starting
   `boil: `. Never amend, rebase or force-push anything once committed;
   fixups are new commits. No attribution trailers. Subagents may do
   mechanical conversions and never commit; the step agent reviews and
   commits their work.
4. **Gate.** `boil/tools/gate.sh LEVEL BUILDDIR` runs the ladder, smallest
   first (section 3). A rung is done when level 3 passes; a step is done
   when level 4 is recorded.
5. **Land.** The agent pushes its branch after every gate-1 pass
   (`boil/tools/worktree.sh push <step>`). The coordinator merges the
   branch into the integration branch with a merge commit after each
   rung's level-3 gate (`boil/tools/worktree.sh land <step>`), re-runs
   the gate on the merged tree when the branch was not alone, and
   pushes. Design notes, the worklog and the hack register are updated on
   the step branch as part of the rung, so landing carries the record.

## 2. Worktrees and branches

- Integration branch: `weaver/python-boil-the-ocean`, checked out in
  `/home/power/code/cpython/.worktrees/python-boil-the-ocean`. Only the
  coordinator commits there (merges, worklog gate entries, notes).
- Step branch and worktree: `boil/tools/worktree.sh new <step>` creates
  `.worktrees/boil-<step>` on branch `boil/<step>` from the integration
  branch, and `worktree.sh build <step>` configures `build/default` and
  `build/debug` in it (`CC=gcc`, and `--with-pydebug`). The agent never
  touches another worktree or its build directories.
- Shared with every worktree: the git stash stack (never use bare `git
  stash`), the mimalloc and uv caches, and the machine. A test run and a
  rebuild of the same build directory never overlap: `gate.sh` holds a
  lock per build directory.
- Every gate command runs under `timeout` in its own process group with a
  ceiling per level (`GATE_LIMIT` overrides), so test workers die with
  the run even when the shell that started it is killed. Run gates 2 and
  3 in the background and poll; a foreground tool timeout that kills the
  shell is how workers were orphaned before. Never run the suite by hand
  outside `gate.sh`; if you must, wrap it in `timeout`. Edit nothing
  under `Lib` or `Tools` while a gate runs from the same tree.
- `worktree.sh drop <step>` removes the worktree and branch after the step
  has landed.

## 3. The gate ladder

| Level | What | When |
|---|---|---|
| 0 | build the tree, `python -c pass`, a smoke set of test files (grammar, opcodes, builtin, dict, list, tuple, class, generators, capi) | after every change that builds; a minute |
| 1 | the rung's targeted test files (`gate.sh 1 BUILD test_a test_b ...`) | before every commit |
| 2 | the full suite on the release build | at each milestone within a rung |
| 3 | the full suite on the debug build, `-u cpu test_cext`, and `make regen-cases clinic` leaving the tree unchanged | at the rung end, before landing |
| 4 | the internal benchmark set recorded (`boil/bench/run.py`, label = the step) and the measurement section written with attribution | at the step end |

Every level uses a fresh `PYTHONPYCACHEPREFIX`. Any change to compiler or
specialiser output, the handler set or the feedback layout bumps the pyc
magic in the same commit. The CPython suite is the correctness gate;
there is no ports gate until the ports phase (note 10). Measurements are
recorded, never tuned for (note 06, plan revisions of 2026-10-27).

## 4. Reporting

The agent reports to the coordinator every 30 minutes or at every rung
end: first line `<step> rung N: <state>`, then commits since the last
report (SHA and one line), gate results as counts, what is next, and
anything to decide. Times come from `date`. Blocked more than 15 minutes:
message the coordinator. Each step has a clock deadline per rung and a
hard stop; at the hard stop the agent commits what is correct, writes the
hand-off list, and reports. The coordinator verifies claims against
`git log` and the gate output, re-runs the level-3 gate on a detached
checkout of the rung's last commit when it lands, writes the gate entry
in `boil/WORKLOG.md`, and keeps the hourly check.
