# The plan, our position in it, and how a change is made

Status: revised 2026-10-29 after the takeover audit and the user's
direction to resume parallel implementation, shorten feedback gates
where justified, and establish interpreter performance before the JIT.
This file is the
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

| Step | What | State (2026-10-29 09:34 PDT; local commits and retained records) |
|---|---|---|
| 1, 2a, 2b, 3a, 3b | the 64-bit word IR, the compiler passes, the handlers | landed |
| 4, 5a, 5b, 5c | the collector: verifier mode, live mode, refcounts deleted | landed |
| 6a, 6b | `PyValue` in the core and the value C API | landed |
| 6c | the object protocol (note 09): shape in the header, positional slots, dictionary mode as a shape, split tables deleted | landed, gated; pre-I hand-offs: FOR_ITER_RANGE spill to H1/H3 attribution, nbody +5.3% to refreshed attribution, linear str += to string/compiler design with uniqueness/escape proof |
| 6d | the remaining object slots on values (note 12): rung 1 class versions, capacity rule, poly stores, lazy-dict loads; rung 2 attribute and descriptor slots; rung 3 `callfunc` and wrapperfunc; rung 4 getsets | complete: rungs 1 to 4 landed (c66c69d55cf, 7ad194aa883, de1533e8b1a; every landing review returned fix first and was fixed); both getset families converted, 465 entries on the value signature; the Clinic numeric getters left for 6e are listed per module in note 06 section 21; C-mode hand-off table of 25 getset functions to 10a item 5 (results/6d-rung4-cmode.txt) |
| 10a | precise roots (note 11): attributed DWARF walk, C checker, kill/retain verification and stack-hit evidence | rungs0,1,1b landed; rung2b unlanded, queue integration reconciled as cb0b6c21b97. Provenance/list-transfer repair committed82165f8a2a9. Original carrier helper fails eight origin subcases; zero-seed-sort control fails both orderings; old list transfer traps10/10, fixed passes10/10. Bounded O1 checks pass; first31-case/11-module feedback iteration took541.745s. Cleanup repair172b406be0c and actual-helper regression cleared bounded review; tooling merged67477efa986. Settled O1 controls pass; whole11 timing109.008s/2041tests passes. Shared-heap callback unskip passes20 selections per build, frozen8ce9c57c662. Final release/O1debug matrices are running with2,041-case whole-module contracts,68/83 intrinsic skips and32 protected selectors. ASan14 passes3,616/91skips with its exact accepted allocator exclusion; unfiltered ASan gate0 remains FAIL. Fresh release/tailcall stale100 and compiled-walk controls pass. Full gates/review and O1 adoption remain pending. Rung3a writers/PyMem,3b checker/precision residue,3c queued releases/detached-state roots and rung4 measurements/support follow |
| 7a | per-interpreter queues, owner-correct callbacks, lifecycle and completion ordering (note 13) | rung1 candidate6f416bc4582 cleared independent round3 review and landed as052d8342e56. Normal suites atd43c8b4dd32 pass51,102 each/0fallback; frozen5cc full debug kill passes51,104/65,601 collections/0fallback. ASan14 passes3,589 with exact AddressSanitizerTests exclusion; original unfiltered failure remains FAIL. Stress11 passes3,046, subprocess371 three times and40 probes pass. Integration smoke1,924 and queues22 per build, actual-kill22 and unchanged regeneration pass. Rung2a design/probes preserved but agent held by automatic classifier; no native fix applied. Owner metadata2b and retirement/adoption/fork2c remain open |
| compiler-copyprop | prevent local-load folding across operations that can run Python; specializers must run no Python code | landed: ab18af7d778 cleared independent source/artifact review and merged as 99435658509. Detached default/debug smoke 1,924 each, targeted 1,783 each, debug-kill 20 tests and release/debug full suites 51,081 each passed; zero fallback in 60,014/61,865 collections. Gate3's cext 16 tests, unchanged regeneration and traversal audit passed. Prior three review findings closed; residual performance attribution belongs to checkpoint I |
| audit-2 | `Tools/boil/traverse_audit.py` report 3 (members that neither the traverse nor a release names) triaged; report 3 part of `--check` | landed 2026-10-28 12:20 PDT (9fe9241bf77): 5 tool defects fixed, 2 real traverse omissions fixed (the SRE scanner's string, the CJK stream writer's pending), 9 allow entries by exact member path; remaining checker limits (last-component matching, duplicated header names, the depth-2 cut-off, hashtable keys and values) to an audit-3 pass by the coordinator after 7a rung 2 |
| 6e | remaining numeric/container/comparison/hash slots, Clinic residue, inline numeric receivers, identity and pointer bounds | open; design after the 10a root/checker contract settles, as note 06 specifies |
| H | handler contract (note 14): dependency-based frame access, canonical guards, build policy; complete root maps and helper spans replace collector clearing stores | H1/H2 remain design, maps wait for10a rung3. H3-1 stage1 landed6c724c29408 and integration-validated. Stage2a domain/pristine changes pass smoke1,924 per build, affected974 per build and actual-kill974; frozen624c75f6455 cleared LAND_STAGE2A, native landing held behindROOTS2b. Shared worker.main correction3dff5879df8 isolated for integration. Stage2b.1 names/inspection/NOFB c5cf6ba6d288 cleared LAND_2B1. Immutable SafeAddr2Line prerequisite has independent design clearance; storage2b.2b still requires separate review/grants. H1 frame-passing design is commissioned. Monitoring/stats, transport closure and final renumbering follow; typed float truth is separate |
| 7 | generational collection over the precise root set | open; decides header flag bits and side bitmaps |
| 8 | tier 0 over the IR, x86-64; uses H3's generated handler domain | open, after checkpoint I |
| 9 | tier 1 | open |
| 10b | moving collector: nursery, evacuation, compaction, handle discipline | open |
| P | the ports phase: Cython, cffi, pyyaml, numpy on the settled API | open, after 10b |
| B | the internal benchmark set (note 10) | done; the record for every gate 4 |
| I | interpreter performance checkpoint, with all JITs off | new user requirement, 2026-10-29: after root/lifecycle correctness, 6e, H and generations; before tier 0. Matched release wall-time comparison against upstream, with instructions, allocations, boxes, RSS and pauses explaining the result. The target is no kernel-geomean slowdown and explained individual regressions; it is not established by the existing microbenchmark instruction gains |

Shared prerequisite `2155f16f45f` is integrated and independently checked:
actual verifier state is exposed through a test accessor. Default/debug
smoke sets and the mode contract passed on integration. This does not
land root-lifetime changes; queue routing has separately landed as052d8342e56.
The roots lane's O1 debug
trial passed checker, stack-walk, actual-worker-mode and 100/100 stale-root
controls, but a focused PyMem-attribution assertion failed. B scans a
PyMem carrier under a precise origin first, then deduplicates away its
core-frame alias, losing that origin. Post-B enrichment is applied and
its fixture detects the old helper. The assertion must accept a PyMem
origin first or supplemental on the same killed-object line. Further
qualification exposed a list-to-tuple element-root lifetime bug; its
minimal reorder passes ten fresh reproductions, with an original ten of
ten failure control. Merged-head bounded checks pass; the first full
reclamation iteration took541.745s with all lifetime assertions passing.
Forkserver/spawn test_finalize also report leaked semaphore/unlink cleanup
errors. Queue-reconciled tracing confirms registry clearing loses semaphore
cleanup before tracker shutdown. Repair172b406be0c moves existing finalizers after forkserver stop and before
tracker stop. The actual-helper regression and old-order negative verify
tracker and forkserver/tempdir dependencies. Visible natural-case off/kill
reruns pass without the semaphore diagnostics.
The final matrices are running on settled8ce with frozen source/helpers and
exact module/case/skip contracts. No O1 adoption is claimed; integration
debug uses Og.

Shared stress-state prerequisite02bcd2f6132 is integrated and validated:
default/debug smoke1,924 each, focused state tests3 each, debug kill2 and
stress1000 two tests pass. The accessor reads actual stress state; the
reftracer fixture restores its prior interval. These coordinator runs
do not substitute for the lanes' worker-level mode/stress witnesses.

Shared tooling e02e177903f fixes cleanup lock ownership: fd8 remains locked
through sweep, and a signaled waiter sweeps only after acquiring the lock.
Five repetitions of the controlled checks pass; both old cleanup and
unguarded-waiter controls fail. The new runner's mode-contract gate1 passed
using the 99435658509 binary. Committed refs are pushed; Loom/GitHub
authorization is explicit. Registry/validator/plan display fff4e0e9b08
is landed after independent safety-control and actual-inventory checks.
The ASan filter names only the established failing AddressSanitizerTests
case, retaining five passing same-named allocator tests. Launcher-only
execution cleared independent review. Orchestration and gate integration
remain outstanding in
boil/tools/verification.md. The v2 identity/result component is in progress:
explicit build-source/runtime/tool roots, actual imported origins and complete
source/output bindings. Controlled clean052 smoke passes1,924/20skips,
1,314collections/zero fallback;226unpinned hits are report-only. Its initial
receipt is superseded as incomplete: extension aliases lacked target-byte
hashes. Actual control rejected that inventory before test execution. Strict
alias/target binding, mutation controls and a separately reviewed supplement
are required before further runtime controls. Independent checks confirm all67
current targets match retained metadata and predate the build completion;
no intervening output writes are recorded. No v2 component positive, named/full
profile or current-ROOTS result is claimed. Run/gate hooks remain separate.

Standing concerns, with owners: conservative-retention artefacts in tests
until 10a rung 2 marks the C API entry paths (trace with the stack-hits
tool, fix with a span and a directive); per-rung instruction deltas of 1
to 2% on the kernels are within the register-allocation noise of the
single eval-loop function and are recorded at intermediate rungs. Checkpoint I
refreshes attribution and resolves persistent interpreter deficits before tier0;
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
| 7a ahead of further collector features; handle discipline before movement (10b) | as planned | as planned |
| linear str += with alias-safe behavior and linear-growth acceptance evidence | coordinator commissions a pre-I string/compiler design; uniqueness/escape proof is required | before I; no automatic tier1 deferral |
| review 2 §5.1: copy propagation crossed reentrant operations and observed rebound locals | compiler correction99435658509; semantic controls and full integration checks recorded in WORKLOG | closed; no pending compiler correctness work from this finding |
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
| an opcode-level trace function (`f_trace_opcodes`) or a `sys.monitoring` INSTRUCTION callback that rebinds a local between two words of one expression sees the rebound value where upstream's stack kept the old one; documented in `Doc/library/sys.monitoring.rst` and `Doc/reference/datamodel.rst` | compiler fix (copy propagation) | the pass folds a local's load into its reader when no op between them can run Python code; per-instruction events fire before every word, so no compile-time rule covers them | no fold under any instrumentable word, which is every fold |
| `PyDict_Watch()`, `PyType_Watch()` and `PyUnstable_Code_SetExtra()` refuse an object another interpreter owns (`ValueError`, `ValueError`, `SystemError`); `PyUnstable_Code_GetExtra()` on a foreign code object reports no extra; the reserved dict watcher ids are exempt (note 13 section 3.4) | 7a rung 2 | a watcher bit or extra slot is an index into one interpreter's table; set by another interpreter, it names a different watcher or free function in the owner's table | the registering interpreter kept beside each bit or slot: a field per watched object, and no answer to which interpreter runs a foreign watcher when the object dies on its owner |
| Sampling records use a new format generation, 32-bit opcode with UINT32_MAX for None and verified target handler-schema identity. Remote debug offsets use a new cookie/layout protocol and size-checked prefix; incompatible layouts, target schemas and files are rejected | H3-1 design decision, 2026-10-29; not implemented | IDs above 254 are already lost. Same Python version can hide different numbering; identity must come from the target, be checked before interpretation and retained by the writer | Byte/sentinel fallback, sampler-local identity without target proof, cross-schema remapping or old-layout decoding |
| Remove C HAVE_ARGUMENT/MIN_SPECIALIZED_OPCODE/MIN_INSTRUMENTED_OPCODE/IR_FIRST_ONLY_HANDLER and their existing Python exports/re-exports inventoried in H3-1 section 5; preserve unrelated EXTENDED_ARG/name APIs | H3-1 final renumber; coordinator decision 2026-10-29, not implemented | Numeric separators cannot describe the generated domain; HAVE_ARGUMENT=41 already misclassifies WITH_EXCEPT_START. Existing hasarg/_opcode.has_arg and generated kinds express the properties | An approximate separator, silently changing its meaning or an old-ID compatibility fallback |

### 0.3 Hand-off ledger

Every item one step hands to another is a row here when the coordinator
rules it, with the evidence that closes it; the closing landing removes
the row into the worklog. Opened 2026-10-28 after eight hand-offs in one
day travelled through messages and work-doc paragraphs.

| Item | From | To | Closes when |
|---|---|---|---|
| Py_buffer consumers (PickleBuffer, _pickle.Unpickler, SRE scanner, _struct.unpack_iterator, _testbuffer.ndarray) release in the sweep, so a Python exporter's `__release_buffer__` is skipped or runs in the sweep | audit-2 review | 10a rung 3, after 7a rung 2 (the release runs Python code, so it is a queued call on the owner) | a test shows exactly-once release per consumer with a retained exporter and an allocating and a collecting callback, on both builds and under kill; and no Python code runs under an allocation-only op (the compiler's copy-propagation rule relies on it: an allocating op does not kill a pending local load) |
| the symtable entry's PyMem roots | audit-2 | 10a rung 3 | the span lands and the allow entry goes |
| the traverse audit's remaining limits (last-component matching, duplicated header names, the depth-2 cut-off, hashtable keys and values, a mention credited as a visit) | audit-2, review 2 | coordinator, audit-3 after 7a rung 2 | audit-3 lands |
| the rung 1 fork-child hang (6d-1.md's hazard record: where the parent waited, not why the child hung) | 6d | 7a rung 2 (the fork path) | on recurrence, the hung child's native and Python stacks; otherwise carried as not recurred |
| test_gc's test_verify_kill under stress: the compiler unit's u_metadata dicts held only through a PyMem_Calloc block (kill:pymem finds the holder by design; the live collector is unaffected) | 7a | 10a rung 3 | the compiler unit's metadata has a root or a span; the stress exclusion row goes |
| the bytes and unicode writer buffers under stress: exclusions `*test_resize*`, `*Writer*` (widened from `*WriterTest*` on 2026-10-28: PyUnicodeWriterFormatTest's test_recover_error segfaults and test_from_format corrupts an argument under stress, on the pre-item-6 build as on the head, so the class is the writers, not 10a items 5 or 6) and test_capi.test_unicode's test_from_format | 10a rung 1 (note 11 6.2), counts from 7a and 10a rung 2b | 10a rung 3 | the writer buffers are rooted; the exclusions go |
| a conservative word under a thread state detached by `PyThreadState_Swap()` in `new_interpreter()` (an unpinned record; the walk attributes the main thread's frames to `_start`); seen as a stale argument tuple in `_PyObject_MakeTpCall`'s frame retaining one object | 7a rung 1 review | 10a rung 3 (ruled 2026-10-28: the fix is in pystate.c's swap path, 7a's file; the effect is retention, which the conservative scan keeps safe) | test_progress_during_finalization gives 20 of 20 at its first check on ASan; the swap path pins the C frame extent as a detach does |
| test_stress_shared_heap_callbacks skipped (the B class) | 7a design | 10a rung 2b item 7, after 7a rung 1 lands | the skip is removed with the manifest and the test passes under stress |
| the SLOT macro family (`slot_mp_ass_subscript`, `slot_nb_power`, `slot_nb_bool`, `slot_nb_inplace_power`, `slot_tp_repr`, `slot_tp_hash`) the C mode cannot parse; `slot_inherited` with `update_one_slot` | 10a item 5 | 10a rung 3 | the checker parses the family or the functions are written out, and they are precise |
| the C mode's block-local false positive (an object local declared in one branch reported as held across the sibling branch's call when results go to a frame pointer); expected failure test_block_local_beside_frame_result | 10a item 3 | 10a rung 3 | the expected failure passes |
| comprehensions +2.19% instructions at 7a rung 1, unexplained | 7a | checkpoint I attribution | refreshed matched measurements explain the delta; a persistent interpreter deficit gets bounded pre-JIT work |
| the Py_NO_INLINE boundaries of 10a item 6 (the dict insertion core under `_PyDict_FromItems`, `update_slot_after_setattr`, `object_set_class_world_stopped`, `PyType_Ready`, `_PyType_Modified_Unlocked`) | 10a rung 2b | 10a rung 3 | the callees are precise and the boundaries go |
| a saved exception surviving a collecting `__del__` has no test (Python code cannot leave one pending; a C fixture is needed) | 10a rung 2b test reread | 10a rung 3 | the fixture lands |
| the sweep-order fallback on allocation failure (review 2 §5.2); finalizer completion order across threads (S1); Python output between mark and sweep (S3) | review 2 | 7a rung 2 | §5.2: the single failure path and its test; S1 and S3: a probe each, and a fix or a written negative result in the record |
| the checker's argument-order and hashtable-family holes (review 2 §5.4, §5.5) | review 2 | 10a rung 2b | both negative fixtures fail before the fix and pass after; the hazard list names the commits |
| the contract "a specializer runs no Python code" (specialize_dict_hint() ran a colliding key's `__eq__` and the re-dispatched word reread a rebound local; the guard is the compiler fix's commit) enforced by the checker | compiler fix review | 10a tooling | a checker fixture with a reentrant `_Py_Specialize_*` callee fails; the specializers pass |
| a reader's folded operand (a local read in place by the op) needs an anchor across the reader's own collecting calls once the eval loop frame is precise (`x + o` where `X.__add__` rebinds x, collects and returns NotImplemented; today the conservative C frame holds the original) | compiler fix review | 10a rung 3 | an oracle that injects a collection at a point where only the reader's precise frame holds the operand, and fails with that root omitted; the compiler fix's test 10 (`x + o`) is a semantic regression only, since `self` and the opaque C frames below hold the operand today |
| standalone forkserver/spawn test_finalize returns SUCCESS but reports a leaked semaphore followed by SemLock._cleanup FileNotFoundError | coordinator/ROOTS feedback matrix1 logs08/10 at0b130; cause confirmed after queue merge | ROOTS before final2b matrix | actual _cleanup_tests helper runs finalizers after forkserver stop and before tracker stop; deterministic old-order negative, active forkserver/tempdir control and clean standalone cases; no exclusion |

### 0.4 Execution plan from the takeover

User direction, 2026-10-29: the coordinator may split and reorder work
for clarity; significant implementation belongs to subagents. Each
agent owns an existing step worktree and its files. The coordinator
owns integration, shared tooling, cross-step decisions and status.

The first wave resumes completed code rather than rebuilding it:

| Lane | First assignment | Next boundary |
|---|---|---|
| compiler | Correctness/copyprop landed99435658509; reviewer now cleared manifest launcher component | Independent review of subsequent landing checkpoints |
| queues | Rung1 landed052d8342e56; round3 review and integration checks complete | Rung2a design/probes held by automatic classifier, including read-only handoff; preserve artifacts, do not retry stopped work elsewhere. Then2b owner metadata and2c retirement/adoption/fork; each gets a separate record and gate line |
| roots | Reconcile integration tooling, finish rung-2b evidence and missing checks, prepare its landing review | Rung-2b landing. Then 10a-3a writers/PyMem first; 3b checker/precision residue; 3c buffer-release and swap paths after the relevant 7a changes. Removing conservative mitigations is a final proved step, not an assumption |

Before changing code each owner supplies a short file list, current
head, first failing control and checkpoint estimate. Report at least
every 30 minutes or at a checkpoint; after 45 minutes without progress
the coordinator checks the cause. No agent merges another lane. Dirty
design notes are preserved. A new integration commit is merged into each
step at a safe point before its next landing, never during a gate.

In parallel the coordinator prepares one verification manifest, examines
the faster debug configuration below, and commissions H3's dispatch
inventory when an agent slot frees. H1 may proceed independently once
its checker/build dependencies are clear; H2 waits for 10a's spans and
reader contract. H3 must account for the 8-bit handler-space assumptions
inside the 16-bit IR, canonical typed arithmetic, condition/branch
dispatch, frame passing, range iteration and the recorded call-window
costs. A proposed operation must have a reusable semantic rule, generated
metadata, normal/exception behavior, monitoring and root coverage, and a
generic guard-failure path through the same protocol. Benchmark-specific
fused sequences are not the plan. The inventory decides which existing
hand-offs move before tier 0; it does not preapprove numeric-loop
lowering or a second loop representation.

The first dispatch inventory is complete. H3-1 designs the handler domain:
IR fields are 16-bit but numbering, deopt metadata, instrumentation and
statistics still assume 256 handlers. One generated domain definition
must replace those assumptions before adding operations. H3-2 may then
add exact-float truth specialization under the existing branch protocol;
POP_JUMP already includes truth conversion. H1 frame passing requires matched
timing/spill evidence and compiled root attribution; it need not wait for
H2's full maps. Call-window producer placement additionally requires
H1/H2 alias, liveness and initialized-span contracts.
Compare/branch encoding needs its own semantic and field-width review.
Scalar range-loop lowering remains outside this bounded assignment.

The pre-JIT dispatch/performance backlog has explicit owners:

| Work | Owner and prerequisite | Evidence before checkpoint I |
|---|---|---|
| Handler-domain closure; exact-float truth using the existing branch protocol | H3-1, then H3-2 | Generated properties, complete consumers, normal/guard-failure/exception/monitoring/root controls |
| Frame passing, dependency-based frame loads, canonical numeric guards and compiler/build policy | H1 after ingress and walk/checker qualification | Matched release elapsed time and spill/code-generation evidence; compiled root attribution and per-word diagnostic position remain correct |
| Inline numeric receivers, remaining slot/Clinic boxing | 6e | Canonical ingress and value-call contracts; boxing counts plus correctness and release performance |
| Call-window producer placement and remaining starred-call entry boxing | H3 follow-up with H1/H2 root/liveness/span contracts | Alias-safe initialized windows across normal and exception paths; allocations/boxes and matched performance |
| Range register-base spill and historical nbody/comprehension regressions | H1/H3 code-generation investigation; coordinator attribution | Refresh historical results on frozen C/stdlib sources; fix persistent interpreter deficits without assuming scalar-loop lowering |
| Linear string concatenation | Coordinator commissions string/compiler design | Uniqueness/escape proof preserving aliases and immutable strings; linear-growth evidence. Tier1 is not the default owner |
| Remove collector clearing stores | H2 after complete maps/helper spans | Root-state oracle and omitted-root negative controls; no removal from an assumed calling convention |

These are semantic and measurement obligations, not an opcode-count target.
An operation must solve a reusable problem under the same execution protocol.

H3-1's second independent design review returned IMPLEMENT after closing
actual-target schema provenance, pristine-word/runtime-state admissibility,
and threshold-export decisions. Begin its first stage under existing IDs;
close consumers and remote transport before final renumbering. The
reviewed design and both rounds are in work/H3-1.md and work/reviews/.
Stage 1's generator ownership must remain separate from ROOTS's C checker
and queue/roots test-hook changes. No typed operation is part of this rung.

The next phase completes 6e, H and generations, with file ownership
allowing independent parts to overlap. Checkpoint I then compares the
interpreter with upstream on the recorded kernels and representative
call, allocation, generator, attribute and string workloads. Match
compiler/build policies, state PGO/LTO/tail-call choices, separate warm
execution from startup, and repeat elapsed-time runs in quiet windows.
Bind each interpreter to its own frozen C and standard-library sources.
The retained baseline binary identifies 15e96bf639d, whose only change
from upstream 321925c92c9 is a worklog, but its configured source directory
is the active fork worktree. Reusing that binary does not by itself prove
an upstream standard-library environment. Establish that source binding
before collecting checkpoint-I measurements; preserve historical records.
Use instruction, allocation, box, memory and pause data for attribution.
Do not drop difficult workloads to meet the target. Persistent deficits
become bounded runtime/dispatch work before tier 0, with new measurements.
This checkpoint supersedes the older rule that every performance result
is merely recorded; intermediate correctness landings still need no
unrelated tuning. JIT tiers, movement and settled ports follow.

Stage1 may land independently as generator/tooling work because all seven
generated products remain byte-identical. Its affected-tool gates do not
establish runtime domain closure. Stages2–4 and their final verification
remain required before H3-2 introduces an operation.

H3 stage2 owns IR/domain/pristine construction, then monitoring, stats and
inspection consumers. ROOTS retains the C checker, GC sources, test_gc and
shared _testinternalcapi.c; a C scalar-boundary fixture is an explicit
handoff. Bounded exception to whole-file ownership: H3 may edit separate
HandlerDomain/metadata classes in Lib/test/test_generated_cases.py while
ROOTS's checker fixtures are frozen on its own branch. H3 must leave those
checker fixtures and imports unchanged. Integration preserves both blocks
and reruns the affected module. No other shared-file exception is implied.
Stage2b.1 separately permits only get_ir's name selector in
Modules/_testinternalcapi.c to use the generated full-domain table. ROOTS
and coordinator approved that exact hunk; no other fixture or entry is
included in the grant.

Stage2b.2a separately permits only code_safe_addr2line(code,int) beside get_ir
and its method-table entry in _testinternalcapi.c, using the existing
pycore_code include. ROOTS and coordinator reviewed the exact additive
proposal and approved it on2026-10-29. No monitoring-storage helper,
instruments include or GC fixture changes are part of this grant. The
corrected design is SHA256
33cef3ae191078a1f578df57f9ac9dae3fee7143f4d214b6510ca9add310bda7;
only immutable lookup2b.2a implementation is authorized.
The internal SafeAddr2Line declaration may additionally change from extern int
to PyAPI_FUNC(int), following adjacent line-table exports, so that the real
test extension can call it. This grants private-symbol visibility, not a new
stable public ABI contract.

Second bounded exception, reviewed by ROOTS: H3 may add only the proposed
pycore_ir.h include, ir_scalar_boundary primitive fixture beside the
specialization fixture, and its method-table entry in _testinternalcapi.c.
ROOTS retains its GC fixtures/entries. The fixture lands with H3's checked
production helpers and boundary controls, not as a cross-step merge.

For the final reclamation matrix, each of31 selected cases still runs
alone in a fresh process per iteration. The11 complete physical modules
may run through one regrtest -j8 driver per iteration: each worker runs a
whole module in its normal internal order. No parallel-threads or match
narrowing. Retain exact scheduled/returned modules, case counts/skips,
seed, driver/worker identities, actual-state witnesses and root counters.
Measure this harness on settled source before projecting its remaining
cost. The serial feedback iteration is not relabeled as this harness.

Final ROOTS2b qualification uses frozen native/test source8ce9c57c662.
Its restored shared-heap regression passed20 fresh selections per build
with actual stress witnesses; source-skip retirement is local to that
branch until landing. The final contract distinguishes debug/release skip
allowances and protects32 selectors, including the restored regression.
The coordinator permits only precise-entry-paths implementation availability
to be promoted in a hash-bound qualification copy of the registry, with
current checker/negative/control evidence. Preserve its original and exact
diff. This establishes availability of the implementation under test, not
success of its own qualification or availability on primary. No other
pending prerequisite or test condition changes. Build-owned adapter runs
do not claim execution through the unfinished verify.py run/gate hook.
O1 debug is approved for this qualification matrix with assertions and
all required controls; adopting it as shared gate policy waits for the
complete required evidence. Freeze every imported adapter/contract helper.
For current ROOTS full gates, the coordinator permits the frozen build-owned
actual-driver/worker adapter under existing gate_run locks, two test slots,
timeouts and fresh caches. Review exact commands and bindings before running;
preserve full gate2/3 suite semantics and perform cext, regeneration/content
comparison and traversal audit separately. This is not execution through
verify.py run or a new shared gate hook. No source or helper changes during
the repeated matrix, and no overlapping full test_perf_profiler suites.
Candidate O1 full normal/kill suites may follow its complete repeated matrix
as further qualification; global O1 gate-policy adoption follows the required
full evidence. ASan kill completion adds test_sort and test_functools to the
separate14-module evidence, with no stress interval or additional exclusion.

The once-only full-release stress100k policy is now approved for qualification
on8ce: complete default module roster, actual mode(0,0)/stress100000 in driver
and workers, four existing stress exclusions plus the machine remote selector.
Use the current *test_resize*, *Writer*, unicode test_from_format and
*test_verify_kill* selectors unchanged; writers/compiler spans remain3a debt.
No test_misc/B-class or ASan-only exclusion applies. A separate qualification
registry copy may promote only full-stress-policy and extend those existing
exclusion IDs/applicability to this profile, retaining the original, exact diff
and hashes. Do not edit the frozen matrix registry. Outer ceiling7200s,
worker1200s and30-minute progress check apply. Success completes the explicitly
filtered scheduled profile, with full cohort/root evidence; it does not close
the underlying unfiltered writer/compiler obligations. Unexpected failures
remain failures and get no automatic new selector.

H3 monitoring is split into an immutable SafeAddr2Line prerequisite2b.2a,
then coherent uint16 storage/relations2b.2b. Independent2a design clearance
confirms the active/data-stack frame->code->linetable root chain, including
detached domain states. Immutable lookup does not fix emergency dead-frame
heuristics. Direct synthetic-word lookup deliberately follows co_linetable;
ordinary saved instruction positions must remain equivalent. Require exact
shared-CAPI hunk approval before implementation; no storage change is covered
by that grant. H1-1 is a separate bounded frame-passing prerequisite design.

Planned native landing order is QUEUES1, ROOTS2b, then H3 runtime stage2.
H3 implementation proceeds independently in its worktree; integration
holds its native changes while ROOTS's final matrix uses settled source,
avoiding duplicate qualification. Tooling/docs with unchanged runtime
sources and products use the explicit affected-level rerun rule.

Hourly reporting: while work is active, approximately every hour the
coordinator updates this state table and `README.md`, appends a dated
progress entry to `WORKLOG.md`, and gives the user a summary of heads,
landings, evidence, blockers and the next hour's work. Re-estimate missed
checkpoints from evidence; the pre-takeover dates are historical, not
current promises. The first update after this restart is recorded at
2026-10-29 06:45 PDT; the second is recorded at 07:41 PDT, and the third at08:44 PDT. The next is
due about09:45 PDT while work continues.

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
Checkpoint I is the explicit exception: it must demonstrate pre-JIT
interpreter performance, as section 0.4 specifies. Levels 2 and 3 accept
`--python WORKER_COMMAND` for a verifier-asserting regrtest worker;
other extra arguments are rejected to preserve full-suite coverage.

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
