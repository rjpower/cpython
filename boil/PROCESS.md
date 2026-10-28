# The plan, our position in it, and how a change is made

Status: in force from 2026-10-27 (user ruling); revised 2026-10-28 on the
user's direction (landing review, single branching point, standing
verification gates, the rerun rule, file ownership). This file is the
working record of the plan and the procedure; `boil/README.md` is the
starting point for a successor and maps the documents. Documents that
no longer describe what is being built are deleted, not kept as history;
the worklog and the commits are the history. Note 06
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
| 6d | the remaining object slots on values (note 12): rung 1 class versions, capacity rule, poly stores, lazy-dict loads; rung 2 attribute and descriptor slots; rung 3 `callfunc` and wrapperfunc; rung 4 getsets | complete: rungs 1 to 4 landed (c66c69d55cf, 7ad194aa883, de1533e8b1a; every landing review returned fix first and was fixed); both getset families converted, 465 entries on the value signature; the Clinic numeric getters left for 6e are listed per module in note 06 section 21; C-mode hand-off table of 25 getset functions to 10a item 5 (results/6d-rung4-cmode.txt) |
| 10a | precise roots (note 11): the attributed DWARF walk, the C-mode checker, kill and retain verification, the stack-hits tool | rungs 0, 1, 1b landed (29a358b492d, 4bb672e0bf4; gated clean), the kill record keeps every word (de6f193f905); rung 2b at a43efa83112 (boil/10a): items 1a (generator send path; its hazard, a stale word in the opaque PyGen_am_send frame after its callees became precise, fixed with the async-gen family), 1b, 3 (UnpackDict in value form), 5 (slot functions, site 13), 6 and 4b (attribute protocol and dict entry points precise) and 7 (13 skips removed) committed; end gates running since 15:36 PDT (gate 0 on three builds and gate 2 default passed, 51,073 tests, 0 fallbacks); before the landing branch, the two checker holes of review 2 (section 0.1: argument order, the hashtable family) are fixed in the same rung; rung 3 (PyMem spans, the writer row, the Py_buffer release skipped in the sweep for a Python exporter, the symtable roots, kill with pymem and structs) and rung 4 (gate 4, the platform matrix, write-up) open; hard stop 2026-11-02 15:30 PDT |
| 7a | the per-interpreter callback and finalizer queue (note 13; finding B of note 08; note 11 question 18): a call runs on the interpreter that owns the object, never while that interpreter cannot run Python code; a shared-heap interpreter's heap passes to the main interpreter at its close; one GIL per domain | rung 1 implemented 2026-10-28 (four commits on boil/7a: node queue and batch, open and close, owner routing with the pin and queue_ready, OWN_GIL with a shared heap refused); landing review returned fix first (two defects: queue_ready unset on a local splice, a NULL dereference after the close; three weak oracles) and the fixes are 3db9ff26c4b and 75840fe4b9e on boil/7a-rung1; the full ladder is running at 75840fe4b9e and the new landing head is due 2026-10-29 09:00 PDT. Rung 2 takes three items from review 2 (section 0.1): the sweep-order fallback, finalizer completion order, debug output between mark and sweep. Moved up from step 7 on 2026-10-28; design adopted 2026-10-28 10:50 PDT after two review rounds, with three rulings (no fallback forms, section 0.2's two compatibility breaks, two rungs): rung 1 (node queue, owner routing, the pin, delivery at execution, the opening and close, one GIL per domain) due 2026-10-29 13:00 PDT; rung 2 (order across owners, metadata by owner and the registration refusal, retirement, adoption and absorption, the fork child, the latency tool) gate 3 by 2026-10-30 12:00 PDT, hard stop 2026-10-30 18:00 PDT |
| audit-2 | `Tools/boil/traverse_audit.py` report 3 (members that neither the traverse nor a release names) triaged; report 3 part of `--check` | landed 2026-10-28 12:20 PDT (9fe9241bf77): 5 tool defects fixed, 2 real traverse omissions fixed (the SRE scanner's string, the CJK stream writer's pending), 9 allow entries by exact member path; remaining checker limits (last-component matching, duplicated header names, the depth-2 cut-off, hashtable keys and values) to an audit-3 pass by the coordinator after 7a rung 2 |
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
(hack register, owner 10a rung 2); test_perf_profiler's tearDown deletes every /tmp/perf-*.map it did not see at setUp, so two suites running at once on this machine can fail each other's test_pre_fork_compile (seen once at de1533e8b1a's gate 2 under load 35, passed 4 of 4 alone; a cross-run artefact of the shared /tmp, rerun rather than excluded); the rung 1 fork-child hang of 6d-1.md's hazard record has not recurred and is owned by 7a rung 2 with the fork path (evidence required on recurrence: the hung child's native and Python stacks).

### 0.1 Cleanup inputs from the architecture reviews

The whole-project reviews are inputs to the plan, not gates. The first
(2026-10-28 at 4bb672e0bf4, no builds run) supplied the rows below
without a label and is deleted; its disposition is review 2's section 2.
`boil/ARCHITECTURE_REVIEW_2.md` (5f74a991472, 2026-10-28, "review 2"
below; its prompt is in `boil/design/reviews/`) is the current one. A review's
confirmed finding becomes a row here with an owner and a time; a
suspected hazard becomes a probe owned by the rung nearest the code.
Owners, as of 2026-10-28:

| Item | Owner | When |
|---|---|---|
| C identity agrees with value identity (`Py_Is`, the identity contract in `pyvalue.h`) | 6e | with the remaining slot families |
| native ABI identity (wheel tags, untagged `.so` acceptance, an ABI-generation check at module init) | phase P's design, before the first port | after 10b |
| the below-2^48 pointer bound enforced where addresses enter a value or header; the supported platform, compiler and unwind matrix published | 10a rung 4 (the matrix, with the precise/opaque/unsupported inventory); 6e (the bound) | this week |
| one verification manifest (modes, exclusions, reasons, owners, removal conditions) that `gate.sh` executes, and landing summaries tied to commit and build | coordinator, `boil/tools/verify.manifest` read by `gate.sh`, content from note 11 section 8.2 (10a rung 2b item 7 supplies the rows) | by 2026-10-30; moved up on 2026-10-28 because each agent re-derives its stress and ASan lists from prose and two landings today needed the lists restated |
| compatibility accessors that allocate and abort on failure (`_PyValue_ObjectOrAbort`): migrate core callers, document the residual contract | 6e | with the remaining slot families |
| documentation: the Phase 1 description and notes 01 to 05 deleted (2026-10-28), note 06's opening reconciled with its as-built record, the 4,095-slot limit documented, wall-time, RSS and pause measurements against upstream | coordinator at each step end; the measurements at 10a rung 4 and step 7 | ongoing |
| 7a ahead of further collector features; `str +=` linear path with its own acceptance criterion (step 9); handle discipline before movement (10b) | as planned | as planned |
| review 2 §5.1, confirmed and reproduced (`f` returns 2 on the fork, 1 upstream): `_PyRegPass_CopyProp` folds a local's load into a reader past a call on the same line, so a call that rebinds the local through `frame.f_locals` changes an operand already evaluated | coordinator, a bounded compiler-fix agent (`boil/compiler-copyprop`) from integration 40cfac6fe3a: a pending load does not survive an op that can run Python code; same-line tests for calls, descriptors and instrumentation; the folding counts recorded before and after | started 2026-10-28; lands before 7a rung 2, independent of both running rungs (compiler files only) |
| review 2 §5.2, confirmed: `order_dead_first()` returns NULL when its table fails to allocate and `sweep()` then frees in heap order, abandoning the referent-reading order | 7a rung 2, its first commit (same file as the queue; same rule as its node table: one failure path, undo the dead, free nothing); test fails the ordering allocation and then collects | 2026-10-29 |
| review 2 §5.4 and §5.5, confirmed checker holes: argument evaluation order assumed left to right (`FunctionChecker.eval`); `_Py_hashtable_\w+` exempted as a family though `foreach`, destruction and lookup run callbacks | 10a rung 2b before its landing branch; negative fixtures for both textual orders and for a collecting foreach callback; any precise function the rules now reject fixed in the same series | 2026-10-29 |
| review 2 §5.7: gate 3's regen check compared status, not content; `root_stats.py check` passed on a missing file | coordinator | done 2026-10-28 (this commit); the manifest row above carries the rest |
| review 2 S1 (several threads of one owner drain the queue; a referrer's finalizer that releases the GIL can be overtaken by its referent's) and S3 (`debug_write()` runs a Python writer between mark and sweep; the pin covers interpreter storage, not the dead set) | 7a rung 2: probe each as the review describes; the completion rule for dependent calls goes into the cross-owner order; no Python call while a dead snapshot is held | with rung 2's design, before its code |
| review 2 S2: type and shape versions are allocated per interpreter while objects, shapes and executing code can be shared across the domain | step 7's domain design, before tiers 0 and 1 trust the guards; probe with the native stash fixture | step 7 |
| review 2 §4: the traverse audit credits a member that is merely mentioned (`MEMBER_RE.findall`); a null check is not a visit | coordinator, audit-3 (the 0.3 row widened) | after 7a rung 2 |
| review 2 §4: dead verifier residue in `gc_trace.c` (`remove_garbage`, `find_holes`, the `(void)` casts in `verify()`); stale header prose at `Include/object.h` line 19 and `Include/refcount.h` line 76; `_uuid`'s weaklist field and `tp_weaklist` | coordinator, one cleanup commit after the 7a rung 1 landing (no gate runs from integration then) | 2026-10-29 |
| review 2 §6: the whole-workload record (6d over the upstream baseline: kernels 1.161, micro 0.693, memory 1.699 instructions) stands as the honest position; wall time, RSS and pause distributions against upstream at step 7's start | coordinator, with the documentation row | step 7 |

### 0.2 Intentional compatibility breaks

Each break is ruled before its rung starts, recorded here with its
reason and the alternative that was rejected, and documented in the
C API or library reference by the rung that makes it.

| Break | Rung | Reason | Alternative rejected |
|---|---|---|---|
| `PyInterpreterConfig` with `use_main_obmalloc` and `PyInterpreterConfig_OWN_GIL` is refused with `InterpreterError` (note 13 section 7.2) | 7a rung 1 | a shared heap is one collection domain; a domain with two GILs would need a stop-the-world across them, which belongs to step 7's collector policy, not to a queue fix | one GIL per domain by stopping the other GIL's threads at each mark (step 7's problem) |
| `PyDict_Watch()`, `PyType_Watch()` and `PyUnstable_Code_SetExtra()` refuse an object another interpreter owns (`ValueError`, `ValueError`, `SystemError`); `PyUnstable_Code_GetExtra()` on a foreign code object reports no extra; the reserved dict watcher ids are exempt (note 13 section 3.4) | 7a rung 2 | a watcher bit or extra slot is an index into one interpreter's table; set by another interpreter, it names a different watcher or free function in the owner's table | the registering interpreter kept beside each bit or slot: a field per watched object, and no answer to which interpreter runs a foreign watcher when the object dies on its owner |

### 0.3 Hand-off ledger

Every item one step hands to another is a row here when the coordinator
rules it, with the evidence that closes it; the closing landing removes
the row into the worklog. Opened 2026-10-28 after eight hand-offs in one
day travelled through messages and work-doc paragraphs.

| Item | From | To | Closes when |
|---|---|---|---|
| Py_buffer consumers (PickleBuffer, _pickle.Unpickler, SRE scanner, _struct.unpack_iterator, _testbuffer.ndarray) release in the sweep, so a Python exporter's `__release_buffer__` is skipped or runs in the sweep | audit-2 review | 10a rung 3, after 7a rung 2 (the release runs Python code, so it is a queued call on the owner) | a test shows exactly-once release per consumer with a retained exporter and an allocating and a collecting callback, on both builds and under kill |
| the symtable entry's PyMem roots | audit-2 | 10a rung 3 | the span lands and the allow entry goes |
| the traverse audit's remaining limits (last-component matching, duplicated header names, the depth-2 cut-off, hashtable keys and values, a mention credited as a visit) | audit-2, review 2 | coordinator, audit-3 after 7a rung 2 | audit-3 lands |
| the rung 1 fork-child hang (6d-1.md's hazard record: where the parent waited, not why the child hung) | 6d | 7a rung 2 (the fork path) | on recurrence, the hung child's native and Python stacks; otherwise carried as not recurred |
| test_gc's test_verify_kill under stress: the compiler unit's u_metadata dicts held only through a PyMem_Calloc block (kill:pymem finds the holder by design; the live collector is unaffected) | 7a | 10a rung 3 | the compiler unit's metadata has a root or a span; the stress exclusion row goes |
| the bytes and unicode writer buffers under stress (`*test_resize*`, `*WriterTest*`) | 10a rung 1 (note 11 6.2), counts from 7a | 10a rung 3 | the writer buffers are rooted; the exclusion goes |
| a conservative word under a thread state detached by `PyThreadState_Swap()` in `new_interpreter()` (an unpinned record; the walk attributes the main thread's frames to `_start`); seen as a stale argument tuple in `_PyObject_MakeTpCall`'s frame retaining one object | 7a rung 1 review | 10a rung 3 (ruled 2026-10-28: the fix is in pystate.c's swap path, 7a's file; the effect is retention, which the conservative scan keeps safe) | test_progress_during_finalization gives 20 of 20 at its first check on ASan; the swap path pins the C frame extent as a detach does |
| test_stress_shared_heap_callbacks skipped (the B class) | 7a design | 10a rung 2b item 7, after 7a rung 1 lands | the skip is removed with the manifest and the test passes under stress |
| the SLOT macro family (`slot_mp_ass_subscript`, `slot_nb_power`, `slot_nb_bool`, `slot_nb_inplace_power`, `slot_tp_repr`, `slot_tp_hash`) the C mode cannot parse; `slot_inherited` with `update_one_slot` | 10a item 5 | 10a rung 3 | the checker parses the family or the functions are written out, and they are precise |
| the C mode's block-local false positive (an object local declared in one branch reported as held across the sibling branch's call when results go to a frame pointer); expected failure test_block_local_beside_frame_result | 10a item 3 | 10a rung 3 | the expected failure passes |
| comprehensions +2.19% instructions at 7a rung 1, unexplained | 7a | step 8 | attributed or absorbed by tier 0 |
| the Py_NO_INLINE boundaries of 10a item 6 (the dict insertion core under `_PyDict_FromItems`, `update_slot_after_setattr`, `object_set_class_world_stopped`, `PyType_Ready`, `_PyType_Modified_Unlocked`) | 10a rung 2b | 10a rung 3 | the callees are precise and the boundaries go |
| a saved exception surviving a collecting `__del__` has no test (Python code cannot leave one pending; a C fixture is needed) | 10a rung 2b test reread | 10a rung 3 | the fixture lands |
| the sweep-order fallback on allocation failure (review 2 §5.2); finalizer completion order across threads (S1); Python output between mark and sweep (S3) | review 2 | 7a rung 2 | §5.2: the single failure path and its test; S1 and S3: a probe each, and a fix or a written negative result in the record |
| the checker's argument-order and hashtable-family holes (review 2 §5.4, §5.5) | review 2 | 10a rung 2b | both negative fixtures fail before the fix and pass after; the hazard list names the commits |
| copy propagation across a reentrant call (review 2 §5.1) | review 2 | coordinator's compiler-fix agent | the probe returns 1; same-line tests land; the record shows the folds lost |

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
   hand-off list with an owner per item. Before the landing branch the
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
   CHECKOUT BASE HEAD WORKDOC` on a detached checkout of the landing
   branch's head; the reviewer reads the as-built record, the gate lines
   and the diff against the integration head, and the script writes
   `boil/design/work/reviews/<step>-<rung>-landing.md`, committed on the
   landing branch. The reviewer
   reads what was built, not the design: deviations without reasons,
   gate lines produced before a code change, hazards dismissed rather
   than traced, deleted invariants, tests that assert less than the
   design promised or that would pass on the wrong behaviour. Blocking findings go back to the agent as new commits
   on the rung's landing branch; the rung lands when the review has no
   open blocking finding or the coordinator has ruled on each. Each
   round's review file is kept (`-landing.md`, `-landing-2.md`), never
   overwritten, and the record closes with a disposition table: every
   blocking finding against its fixing commit, its written rebuttal or
   its hand-off row in section 0.3.
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
7. **Whole-branch review at a step's end.** When a step completes (not
   each rung), the coordinator runs the whole-project review of
   `boil/design/reviews/architecture-review-2-prompt.txt`, updated to the
   head, on a detached checkout (codex at xhigh, read-only, a 2.5-hour
   ceiling, in the background) and files it as a loom artifact and under
   `boil/`. Rung-local review reads a diff; the two reviews so far found
   defects a diff cannot show (the copy-propagation fold across a call,
   the sweep's heap-order fallback, the checker's evaluation-order
   assumption). Its confirmed findings become section 0.1 rows the same
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
