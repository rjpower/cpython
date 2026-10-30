# The plan, our position in it, and how a change is made

Status: revised 2026-10-30 after the holistic GC review and the user's
direction to defer subinterpreters, land the focused GC repairs and then
establish interpreter performance before the JIT.
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

| Step | What | State (2026-10-30 after integration75e8; actual source and retained outcomes) |
|---|---|---|
| 1, 2a, 2b, 3a, 3b | the 64-bit word IR, the compiler passes, the handlers | landed |
| 4, 5a, 5b, 5c | the collector: verifier mode, live mode, refcounts deleted | landed |
| 6a, 6b | `PyValue` in the core and the value C API | landed |
| 6c | the object protocol (note 09): shape in the header, positional slots, dictionary mode as a shape, split tables deleted | landed, gated; pre-I hand-offs: FOR_ITER_RANGE spill to H1/H3 attribution, nbody +5.3% to refreshed attribution, linear str += to string/compiler design with uniqueness/escape proof |
| 6d | the remaining object slots on values (note 12): rung 1 class versions, capacity rule, poly stores, lazy-dict loads; rung 2 attribute and descriptor slots; rung 3 `callfunc` and wrapperfunc; rung 4 getsets | complete: rungs 1 to 4 landed (c66c69d55cf, 7ad194aa883, de1533e8b1a; every landing review returned fix first and was fixed); both getset families converted, 465 entries on the value signature; the Clinic numeric getters left for 6e are listed per module in note 06 section 21; C-mode hand-off table of 25 getset functions to 10a item 5 (results/6d-rung4-cmode.txt) |
| 10a | precise roots, writers, checker and lifetime qualification | Qualified5a45 landed as d6a23; later safetyff6 and supported main-only75e8 close the reviewed snapshot, drainer and finite consumer lifetime repairs. Hybrid roots, reader/checker precision and platform obligations remain explicit; no conservative coverage is removed by passing suites |
| 7a | queues, snapshot/allocation safety and callback delivery (note 13) | Supported main-interpreter union qualified4da/runtimeDAC6/native3c6+ASanf945 is integrated75e8. Normal/kill O1 fulls each51240 tests, stress2, ASan/composites, closure and postmerge1935-case smokes clear independent review. Final12f1/postmergef56e retain source/resource/skip limits and original FAILs. Foreign/adoption and extra platforms are deferred, not supported-profile completion blockers |
| compiler-copyprop | prevent local-load folding across operations that can run Python; specializers must run no Python code | landed: ab18af7d778 cleared independent source/artifact review and merged as 99435658509. Detached default/debug smoke 1,924 each, targeted 1,783 each, debug-kill 20 tests and release/debug full suites 51,081 each passed; zero fallback in 60,014/61,865 collections. Gate3's cext 16 tests, unchanged regeneration and traversal audit passed. Prior three review findings closed; residual performance attribution belongs to checkpoint I |
| audit-2 | `Tools/boil/traverse_audit.py` report 3 (members that neither the traverse nor a release names) triaged; report 3 part of `--check` | landed 2026-10-28 12:20 PDT (9fe9241bf77): 5 tool defects fixed, 2 real traverse omissions fixed (the SRE scanner's string, the CJK stream writer's pending), 9 allow entries by exact member path; remaining checker limits (last-component matching, duplicated header names, the depth-2 cut-off, hashtable keys and values) to an audit-3 pass by the coordinator after 7a rung 2 |
| 6e | remaining numeric/container/comparison/hash slots, Clinic residue, inline numeric receivers, identity and pointer bounds | open; design after the 10a root/checker contract settles, as note 06 specifies |
| H | generated handler domain and orthogonal dispatch | H3 storage/names/statistics qualified in d6a23. Isolated H1 native62456/tool-test94ab passes four bounded runtime/root rows and independent review; no timing or landing. Matched Clang timing plan parked during GC. H2 inert decoder5df/test-only b3e completes bounded native validation,27 supported/126 unsupported; no active maps or clearing change. No opcode threshold |
| 7 | nonmoving generations | Reviewed direction; allocator/frame/container/runtime ledgers bind current source. Types/shapes/native/iterator/root/port coverage and concrete barrier/constructor/promotion/reentry/young-index interfaces remain. No barrier/minor implemented |
| 8 | tier 0 over the IR, x86-64; uses H3's generated handler domain | open, after checkpoint I |
| 9 | tier 1 | open |
| 10b | moving collector: nursery, evacuation, compaction, handle discipline | open |
| P | the ports phase: Cython, cffi, pyyaml, numpy on the settled API | open, after 10b |
| B | the internal benchmark set (note 10) | done; the record for every gate 4 |
| I | interpreter performance checkpoint, all JITs off | Revised user guidance2026-10-30: roughly2× is soft guidance, especially for simple arithmetic, not an aggregate gate. Finish GC fixes first; then detailed interpreter status report and subagent design reflection for clean general improvements where results disappoint. Evaluate parameterized operation families alongside value flow, calls and dispatch. Report fair release elapsed time, regressions, allocations, boxes, RSS and pauses; no local score chasing |

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
The8ce matrices are completed historical evidence; full debug then exposed
Park retention. This historical checkpoint used ce2ac8991ee with the precise-Park repair and
two raw-mutex annotations. Fresh affected24 and six of seven profiles pass under plan7d40. The final
stress100k is terminal FAIL64448666: interpreter assertion, test_gc trashcan
timeout and HTTPENV_CHANGED. Missing terminal worker evidence prevents a
qualified root aggregate. Source/helpers/native products stay frozen for
bounded primary-versus-candidate attribution; four once-only interpreter
diagnostics are complete. No retry or broad exclusion follows.
Four focused controls support the interpreter test's threshold-trigger
eligibility mismatch; its test-only annotation passes normal/stress checks,
while the scheduler stays unchanged. A partial observer does not establish
the buffer closer. Public primary/current comparisons establish inherited
HTTP exposure and trashcan noncompletion, with causes/interactions unresolved.
At that checkpoint these findings blocked final GC/performance readiness.
The current75e8 normal/kill/stress coverage is recorded below; it does not
relabel the old failed stress100k run. See the triage and independent landing-readiness records. O1 is allowed
for the isolated bytes-writer qualification; no global replacement is claimed;
that historical integration used Og. The supported75e8 qualification uses
O1/Py_DEBUG, with assertions, frame pointers and actual mode witnesses. Bounded tail make/stale checks may
run in parallel,
but its complete StackWalk waits for all fulls under their serialization lock
because perf-map cleanup otherwise interferes. Extras and landing review follow.

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
boil/tools/verification.md. The v2 identity/result component landed as9dd24bf42c4 after independent review:
explicit build-source/runtime/tool roots, actual imported origins and complete
source/output bindings. Controlled clean052 smoke passes1,924/20skips,
1,314collections/zero fallback;226unpinned hits are report-only. Its initial
105-entry receipt is superseded because extension aliases lacked
content hashes. Strict alias/target repair and the separately reviewed172-entry
supplement now bind all67 targets; independent metadata checks and command
history establish no intervening output write. All seven bounded component
positives pass, including exact off/kill,203-case module-order/5source skips,
separate repetitions, cext16/7source skips and auxiliary kill. Seven isolated
failure controls and11 evidence-corruption checks pass. All105 indexed artifact
hashes and six raw driver cohorts independently reconcile. This is not a
named/full profile or current-ROOTS result
or completed run/gate hook. Those remain separate.

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
| review 2 §5.2: unsafe heap-order fallback on sweep-order OOM | no-sweep repair and named copied fallback failure; safetyff6 and supported75e8 qualification | closed for the supported profile; original failed controls preserved |
| review 2 §5.4 and §5.5, confirmed checker holes: argument evaluation order assumed left to right (`FunctionChecker.eval`); `_Py_hashtable_\w+` exempted as a family though `foreach`, destruction and lookup run callbacks | 10a rung 2b before its landing branch; negative fixtures for both textual orders and for a collecting foreach callback; any precise function the rules now reject fixed in the same series | 2026-10-29 |
| review 2 §5.7: gate 3's regen check compared status, not content; `root_stats.py check` passed on a missing file | coordinator | done 2026-10-28 (this commit); the manifest row above carries the rest |
| review 2 S1/S3: competing drainers and Python output over an unrooted dead snapshot | rooted interpreter-wide drainer, deferred byte output, raw-garbage span and registry-lock repair; independent source/causal/union evidence | closed in supported75e8; parallel callback progress and foreign lifecycle remain deferred |
| review 2 S2: type and shape versions are allocated per interpreter while objects, shapes and executing code can be shared across the domain | deferred with foreign-interpreter support; restore a domain-wide guard contract and native stash control before allowing foreign sharing | future foreign-interpreter milestone |
| review 2 §4: the traverse audit credits a member that is merely mentioned (`MEMBER_RE.findall`); a null check is not a visit | coordinator, audit-3 (the 0.3 row widened) | after 7a rung 2 |
| review 2 §4: dead verifier residue in `gc_trace.c` (`remove_garbage`, `find_holes`, the `(void)` casts in `verify()`); stale header prose at `Include/object.h` line 19 and `Include/refcount.h` line 76; `_uuid`'s weaklist field and `tp_weaklist` | coordinator, one cleanup commit after the 7a rung 1 landing (no gate runs from integration then) | 2026-10-29 |
| review 2 §6: the whole-workload record (6d over the upstream baseline: kernels 1.161, micro 0.693, memory 1.699 instructions) stands as the honest position; wall time, RSS and pause distributions against upstream at step 7's start | coordinator, with the documentation row | step 7 |

### 0.2 Intentional compatibility breaks

Coordinator decision,2026-10-30 09:35 PDT: the first GC/interpreter milestone
supports one main interpreter per process, including ordinary threads.
Subinterpreters are deferred. Creation entrypoints must reject unsupported
use before creating interpreter state. The early rejection, existing capability flag and finite success-only test
classification are integrated and qualified as75e8 (source4da).
Process workers are an explicit alternative, not a transparent implementation
of interpreter handles, C thread states or shared native state. The supported boundary preserves main bootstrap, inspection, rejection and
normal-thread tests; unsupported creation remains visible through explicit skips.
Foreign-owner registration/adoption and repeated interpreter deletion no
longer block this milestone. Actual process exit may retain safe late state
for OS reclamation; returning from Py_FinalizeEx to a live embedding host is
a separate support contract and cannot silently use that assumption.

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
| `_PyCode_Deinstrument` changes from byte-to-byte to `int(int)`; `_Py_Instrumentation_GetLine` takes `(data, index)` and gains private export visibility; monitoring LINE/INSTRUCTION storage changes layout | H3-1 stage2b.2b; qualified in runtime merge d6a23b9fe0c | Validate original scalars before narrowing and return invalid/no-counterpart/canonical results explicitly. Fixed absolute int lines remove an invalid relative-line assumption; the decoder no longer needs a code argument. External private/JIT users rebuild | Truncating byte adapters, retaining an unused decoder argument or claiming stable ABI compatibility. All five production decoder calls and bounds must be updated together |
| Python GC debug writes run after the stop callback and collecting reset, before queued finalizers | S3 repair, qualified through safetyff6 and supported75e8 | No Python writer may borrow dead or raw garbage snapshots. Text, flags, per-record exception behavior and within-pass order remain | Immediate Python writes over collector snapshots; global ordering across nested collections |
| Private weakref table metadata uses libc allocation outside public RAW/tracemalloc hooks | S3 repair, qualified through safetyff6 and supported75e8 | All table acquisitions retain the GIL. Fixed native allocation removes the RAW/tracemalloc lock dependency from holders | A partially converted lock protocol or replaceable allocator callbacks inside that private mutex |


### 0.3 Hand-off ledger

Every item one step hands to another is a row here when the coordinator
rules it, with the evidence that closes it; the closing landing removes
the row into the worklog. Opened 2026-10-28 after eight hand-offs in one
day travelled through messages and work-doc paragraphs.

| Item | From | To | Closes when |
|---|---|---|---|
| the symtable entry's PyMem roots | audit-2 | 10a rung 3 | the span lands and the allow entry goes |
| the traverse audit's remaining limits (last-component matching, duplicated header names, the depth-2 cut-off, hashtable keys and values, a mention credited as a visit) | audit-2, review 2 | coordinator, audit-3 after 7a rung 2 | audit-3 lands |
| the rung 1 fork-child hang (6d-1.md's hazard record: where the parent waited, not why the child hung) | 6d | 7a rung 2 (the fork path) | on recurrence, the hung child's native and Python stacks; otherwise carried as not recurred |
| test_gc's test_verify_kill under stress: the compiler unit's u_metadata dicts held only through a PyMem_Calloc block (kill:pymem finds the holder by design; the live collector is unaffected) | 7a | 10a rung 3 | the compiler unit's metadata has a root or a span; the stress exclusion row goes |
| the bytes and unicode writer buffers under stress: exclusions `*test_resize*`, `*Writer*` (widened from `*WriterTest*` on 2026-10-28: PyUnicodeWriterFormatTest's test_recover_error segfaults and test_from_format corrupts an argument under stress, on the pre-item-6 build as on the head, so the class is the writers, not 10a items 5 or 6) and test_capi.test_unicode's test_from_format | 10a rung 1 (note 11 6.2), counts from 7a and 10a rung 2b | 10a rung 3 | the writer buffers are rooted; the exclusions go |
| a conservative word under a thread state detached by `PyThreadState_Swap()` in `new_interpreter()` (an unpinned record; the walk attributes the main thread's frames to `_start`); seen as a stale argument tuple in `_PyObject_MakeTpCall`'s frame retaining one object | 7a rung 1 review | 10a rung 3 (ruled 2026-10-28: the fix is in pystate.c's swap path, 7a's file; the effect is retention, which the conservative scan keeps safe) | test_progress_during_finalization gives 20 of 20 at its first check on ASan; the swap path pins the C frame extent as a detach does |
| test_stress_shared_heap_callbacks requires successful subinterpreter creation | historical ROOTS control | any future restoration of foreign-interpreter support | main-only policy keeps the explicit source capability skip; it is not a supported-profile blocker or a new selection filter |
| the SLOT macro family (`slot_mp_ass_subscript`, `slot_nb_power`, `slot_nb_bool`, `slot_nb_inplace_power`, `slot_tp_repr`, `slot_tp_hash`) the C mode cannot parse; `slot_inherited` with `update_one_slot` | 10a item 5 | 10a rung 3 | the checker parses the family or the functions are written out, and they are precise |
| the C mode's block-local false positive (an object local declared in one branch reported as held across the sibling branch's call when results go to a frame pointer); expected failure test_block_local_beside_frame_result | 10a item 3 | 10a rung 3 | the expected failure passes |
| comprehensions +2.19% instructions at 7a rung 1, unexplained | 7a | checkpoint I attribution | refreshed matched measurements explain the delta; a persistent interpreter deficit gets bounded pre-JIT work |
| the Py_NO_INLINE boundaries of 10a item 6 (the dict insertion core under `_PyDict_FromItems`, `update_slot_after_setattr`, `object_set_class_world_stopped`, `PyType_Ready`, `_PyType_Modified_Unlocked`) | 10a rung 2b | 10a rung 3 | the callees are precise and the boundaries go |
| historical requested precision fixture for a saved exception surviving a collecting __del__ | 10a rung2b reread | separate10a precise-frame/control coverage | preserve the remaining fixture question separately from the now-qualified active drainer raised-exception span |
| the checker's argument-order and hashtable-family holes (review 2 §5.4, §5.5) | review 2 | 10a rung 2b | both negative fixtures fail before the fix and pass after; the hazard list names the commits |
| the contract "a specializer runs no Python code" (specialize_dict_hint() ran a colliding key's `__eq__` and the re-dispatched word reread a rebound local; the guard is the compiler fix's commit) enforced by the checker | compiler fix review | 10a tooling | a checker fixture with a reentrant `_Py_Specialize_*` callee fails; the specializers pass |
| a reader's folded operand (a local read in place by the op) needs an anchor across the reader's own collecting calls once the eval loop frame is precise (`x + o` where `X.__add__` rebinds x, collects and returns NotImplemented; today the conservative C frame holds the original) | compiler fix review | 10a rung 3 | an oracle that injects a collection at a point where only the reader's precise frame holds the operand, and fails with that root omitted; the compiler fix's test 10 (`x + o`) is a semantic regression only, since `self` and the opaque C frames below hold the operand today |
| note11's once-only tailcall and release-retain 8.2 recommendations | ROOTS2b qualification | 10a rung4 build support / H1 compiler convention handoff | qualify the actual builds and use a retain-specific reclamation oracle; pair tailcall qualification with the H1 convention experiment. These recommendations do not add two new ROOTS2b landing blockers or reuse the off-mode oracle |

### 0.4 Execution plan from the takeover

User direction, 2026-10-29: the coordinator may split and reorder work
for clarity; significant implementation belongs to subagents. Each
agent owns an existing step worktree and its files. The coordinator
owns integration, shared tooling, cross-step decisions and status.

The first wave resumes completed code rather than rebuilding it:

| Lane | First assignment | Next boundary |
|---|---|---|
| compiler | Reentrancy/copyprop99435658509 and shared verification components landed; H3 qualified in d6a23 | H1/H2 isolated with bounded evidence. Timing parked. Detailed interpreter report and holistic operation-family reflection follow GC fixes |
| queues | Supported single-drainer/readiness policy is qualified and integrated75e8; old failure and causal controls retained | No supported callback-completion blocker remains. Foreign/adoption, parallel callback progress and repeated interpreter lifecycle remain deferred; no generic completion DAG |
| roots | ROOTS/writers and supported snapshot/buffer repairs are integrated through75e8 | Preserve hybrid coverage. Complete separately named reader/checker/PyMem precision and platform work before removing conservative roots; no full-suite pass substitutes for that proof |

Before changing code each owner supplies a short file list, current
head, first failing control and checkpoint estimate. Report at least
every 30 minutes or at a checkpoint; after 45 minutes without progress
the coordinator checks the cause. No agent merges another lane. Dirty
design notes are preserved. A new integration commit is merged into each
step at a safe point before its next landing, never during a gate.

In parallel the coordinator prepares one verification manifest, examines
the faster debug configuration below, and commissions H3's dispatch
inventory when an agent slot frees. H1's bounded frame-passing implementation
and runtime checkpoint are complete in isolation; measurement is parked
during GC implementation. It does not depend on H2 maps or a new
frame-pointer policy.
Do not branch from an unlanded step branch or edit H3-owned generator files
concurrently. H2 waits for 10a's spans and
reader contract. H3 must account for the 8-bit handler-space assumptions
inside the 16-bit IR, canonical typed arithmetic, condition/branch
dispatch, frame passing, range iteration and the recorded call-window
costs. A proposed operation must have a reusable semantic rule, generated
metadata, normal/exception behavior, monitoring and root coverage, and a
generic guard-failure path through the same protocol. Benchmark-specific
fused sequences are not the plan. The inventory decides which existing
hand-offs move before tier 0; it does not preapprove numeric-loop
lowering or a second loop representation.

Coordinator exception,2026-10-29 15:20 PDT: stage2b3 handler-statistics
implementation may branch from reviewed H3 HEAD81417aff097 into a new managed
worktree/branch, preserving the frozen H3 and ROOTS trees/products. Its
full-domain counters, generated adaptive property and producer/reader semantics
have no technical GC prerequisite. The designated owner receives the
design-listed files, shared generated-cases hunks and branch-local benchmark
reader/docs rename; historical results stay unchanged. First finish the
current independent diagnostic/timing reviews, then change roles to stats
implementation. Root or a different reviewer reviews that code. Native builds
and tests wait for release of the first quiet measurement window. This permits
isolated development, not primary landing, a cross-lane merge or qualification
of a union that has not been built. ROOTS/H3 integration still requires the
actual shared-source composition, regeneration and fresh union gates.

The same isolated-development exception permits the bounded bytes-writer3a
lane in `boil-gc3a-bytes`, branch `boil/gc3a-bytes`, from ce2ac8991ee. Its owner
first fixes the concrete embedded-root/finish-handoff design and additive
fixture ownership, then implements only bytes/bytearray writer lifecycle.
Unicode, general PyMem retirement and held Queue2 work stay outside scope.
It must compose the eventual ROOTS successor before landing; its unlanded base
and failed stress profile remain explicit. No native execution during the
quiet window and no mutation of the original ROOTS source/build are permitted.

Coordinator ruling, 2026-10-29 16:09 PDT: accept the independently reviewed
scoped ROOTS2b exception in
`design/work/reviews/10a-2b-landing-readiness.md`. Six passing
profiles, the preserved failed stress100k profile and its bounded disposition
may support a successor tail-WALK/extras plan. The old all-seven-PASS condition
was not met; neither that plan nor its failed validator/result is rewritten.
The test-only eligibility successor must bind its actual source separately
from unchanged ce2ac native products. At this historical checkpoint, inherited HTTP resource/finalizer ordering
and queue progress blocked final GC/performance readiness. Their later supported
profile closure is recorded in the current75e8 ruling; the old failures remain
failed. This historical ruling grants no new work.

ROOTS2b and the independently coordinator-reviewed bytes3a may compose into
one detached integration candidate after required tail/extras and exact source
review. Run one fresh mandatory merged ladder on that actual union, including
default/debug fulls, debug-kill and the applicable verification checks; retain
both changes' focused receipts and frozen parent products for attribution.
This replaces duplicate integration ladders, not coverage. Unexpected failures
stop the candidate for diagnosis; no automatic full retry or new filter follows.
Existing integration debug remains Og. H3 runtime composition follows this
checkpoint separately.

Coordinator ruling, 2026-10-29 17:16 PDT: f2's first release full remains
FAIL/native2. All 564 modules returned; only test_cmd_line failed, with two
source-confirmed effects of the launcher's unnecessary PYTHONPATH and
PYTHONNOUSERSITE. The unchanged validator clears the other 563 module cohorts
(audit8c60d1b0). Permit one whole unchanged test_cmd_line run with those two
variables removed for the owned build. On its qualified PASS, an explicit
assembly must revalidate both cohorts under their real plan/environment
identities and prove their disjoint union is the exact 564-module roster.
This is assembled release coverage, not a uniform full-suite PASS; the red
full and all its raw evidence remain. Its all-run root counters include the
failed module and must not be apportioned to credited modules. No successful
module is repeated just to repair the launcher. The five unattempted broad
profiles keep their selections/criteria, with corrected owned-build environments;
ASan retains its separately necessary runtime-origin overlay. Gate3 closure
requires this explicit assembly plus those five fresh qualified profiles.
This narrow ruling waives no source/native guards or final GC/lifecycle debt.

While that frozen GC candidate runs its gates, isolated H3 source composition
and one reviewed Py_STATS build may proceed in `boil-h3-union` from f2a5.
Original H3/stats products stay frozen. Exact union e9a1bcc65cf passes the
coordinator's source-composition review; native results remain due. This
overlaps preparation with testing and does not land H3 on primary. Root also
permits its owned O3/Og builds and bounded smoke/affected/kill/stale
checks before primary landing. Keep all three cohorts frozen; serialize the
perf-trampoline integration jobs with f2's broad suites, and keep H3 fulls
after f2's campaign. These are reviewed isolated-development grants, without
changing f2's source or products.

Separately, the bounded finalizer-order eligibility optimization may be implemented from
f2a5 in `boil-gc-finalizer-order`: a local existing-predicate refactor and one
Python transit-graph regression only, with native execution separately reviewed.
It changes no queue/deallocation semantics or held Queue2 scope.

Coordinator checkpoint, 2026-10-29 18:11 PDT: f2 remains a held predecessor,
not a completed landing. H3's three remaining integration checks now pass
on e9 (54 reported each, exact11/1/1 skips, zero fallback). Source-reviewed
reader/history repair7c703 and Unicode writer ownership2f37 compose with H3
and finalizer eligibilitya285 in managed boil-pre-i-union5a45f9bb6a7. Run one
fresh O3/O1-debug combined ladder rather than complete f2 and immediately
repeat its broad gates for these changes. Original successes and failures
retain their own identities; new code receives fresh qualification.
The four-row a285 diagnostic was allowed on the explicitly unqualified f2
parent after passing assembled release/debug/kill coverage, despite unresolved
stress/queue debt. It yields encouraging initial ratios, not correctness or
upstream/29-row/2x qualification. All40 raw values and source/native cleanup
reconcile; the timing record states process/layout/noise limits. No blanket
repeat or additional profile is commissioned. H1 source preparation may
proceed independently, with actual matched compiler/dispatch experiments
following the coherent baseline; no H1 native result exists yet.

Coordinator checkpoint, 2026-10-29 18:50 PDT: frozen5a45 builds on GCC O3,
O1 Py_DEBUG and ASan; all19 affected jobs pass. Their whole-GC receipts retain
two extra skips caused by truthy PYTHON_GCSTRESS='0', not actual stress.
FutureN0 target environments omit that variable while asserting getter0;
normal fulls protect both byte-trigger tests. Do not rewrite old skip counts.
ASan16 plus Unicode consumers run before the four matched Clang baseline
runtime/root checks. Five fresh broad profiles follow serially, then closure
and final review. A failure stops its chain without a new skip or retry.
The H1 source patch and inert H2 producer work proceed independently of those
frozen runtime inputs. The historical result remains below the soft2× guide; no numerical gate is imposed.

A successful configure/make with a demonstrably defective post-build parser
may use a separately reviewed host-only supplement over unchanged sealed
products, original successful command statuses, actual config/tool/source and
retained reports. Preserve the original wrapper failure. Complete the missing
checks and cleanup; do not relabel it a passed build receipt or rerun make
merely to remove red history. This covers Clang goto's line-ended CIE token,
not absent CFI or an actual compile/runtime failure.

Coordinator landing ruling, 2026-10-29 20:10 PDT: exact5a45 completes all
seven profiles, closure and independent LAND_INCREMENTAL review. The merge
into primary changes only a frozen list of project-prose paths relative to
that tested candidate: every other tracked file must remain byte-identical.
For this merge, reuse the qualified5a45 binaries and their own frozen source/
stdlib roots for one post-merge smoke per GCC release/O1-debug configuration.
Bind the actual integration commit/tree and exact changed boil Markdown
paths separately from compiled/runtime source5a45. This is not a claim of
recompiling the integration head. Retain the seven-profile/closure evidence,
source/config/tool/native checks, actual mode/N0 and fresh-cache witnesses.
No further compile or full-suite repeat follows a proven project-prose-only
merge. A runtime, test, generated, build or tool change invalidates this
exception and requires its own reconciliation. Primary publication waits for
both smoke results; inherited final-GC/performance debt remains open.

Queue2a's original classifier-held probes stay preserved, without a native
retry. The newly reviewed source design separates three safety changes.
The coordinator authorizes isolated source implementation of sweep-order
preparation/rollback first; its focused implementation and failure seam
need review before native validation. Debug-output/root-lifetime design is
next. Completion scheduling is not implementation-ready: eligibility,
sparse graph storage and running-call close/fork behavior still need proof.
This does not reopen the old raw-address fixture or grant a probe matrix.

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
| Linear string concatenation | String/compiler S0 clears readiness; H3/code-layout and H1/ROOTS interfaces precede mutation-disabled S1; Unicode S2 and observer S3 precede S4 | Uniqueness/escape proof preserving aliases and immutable strings; enabled-path overhead and linear-growth evidence. Tier1 is not the default owner |
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
Do not drop difficult workloads to meet the target. User clarification,
2026-10-30: roughly2× is soft guidance, particularly for simple arithmetic,
not a required aggregate or numerical gate before tier0. Finish GC fixes,
then publish the interpreter status report. Deficits commission bounded
independent design reflection for general, orthogonal improvements, including
parameterized operation families; they do not mandate local score tuning.
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

Stage2b.2b fixed-int/uint16 storage design84e518cb872e and exact additive
CAPI patch71ec68cb7da7 clear independent ROOTS/coordinator review at10:18.
Grant only its pycore_instruments include, three functions beside the safe
helper and three VARARGS entries, coherently with the new header/storage.
All five production GetLine callers, including legacy tracing, change together.
The state hook copies mutable fields before allocating its result. Preserve
existing GC/compiler/scalar fixtures. This supersedes the storage hold above
only for this bounded implementation; later monitoring/stats/transport work
and runtime landing remain separate.

New test hooks must use this fork's method typedefs. In particular, METH_O
requires PyValue for both the argument and result, with the method cast;
ordinary PyObject argument hooks may use the existing VARARGS convention.
Boolean pointer-bit coincidence does not validate an incompatible signature.

The54b broader gate exposes an inherited CO_NO_MONITORING_EVENTS refresh loop
when ordinary flagged code executes RESUME after a global version change.
Preserve that failed gate. Eligibility proposal-v3 d5d88a7b8f80 clears independent
review: guard only the existing version-mismatch refresh in bytecodes.c, return
early in instrument_lock_held, and return early in ceval.h monitor_handled.
The latter exact cold-path hunk is an additional bounded ownership grant.
Regenerate cases and retain no-local-data handled-exception and executing-code
controls. No version acknowledgment, static-code write or sanity weakening.
Qualify one frozen combined storage/eligibility candidate; this grant does not
authorize an independently landed or independently advertised prerequisite.

During ROOTS's critical matrix/full chain, H3 runs only one gate transaction
at a time. Do not queue a second transaction for the same build: gate_run takes
a test slot before its build lock, so that waiter can consume ROOTS's lane.
Active jobs finish unchanged; scheduling changes apply to idle submitters.

Second bounded exception, reviewed by ROOTS: H3 may add only the proposed
pycore_ir.h include, ir_scalar_boundary primitive fixture beside the
specialization fixture, and its method-table entry in _testinternalcapi.c.
ROOTS retains its GC fixtures/entries. The fixture lands with H3's checked
production helpers and boundary controls, not as a cross-step merge.

For the historical8ce reclamation matrix, each of31 selected cases runs
alone in a fresh process per iteration. The11 complete physical modules
may run through one regrtest -j8 driver per iteration: each worker runs a
whole module in its normal internal order. No parallel-threads or match
narrowing. Retain exact scheduled/returned modules, case counts/skips,
seed, driver/worker identities, actual-state witnesses and root counters.
Measure this harness on settled source before projecting its remaining
cost. The serial feedback iteration is not relabeled as this harness.

The historical ROOTS2b qualification below uses frozen source8ce9c57c662;
the post-Park ruling later in this section governs fresh qualification.
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
avoiding duplicate qualification. H3 storage candidateffc548603b8 forks from
6c724c29408 and lacks queue052's13 native/test-file changes, including the
shared CAPI additions. An incoming primary merge is therefore a native change,
not a docs/tool-only refresh. Preserve the frozen pushed candidate and review
its exact storage delta/evidence now; this is not joint runtime qualification.
Explicitly defer the live-branch integration merge until ROOTS2b is settled,
then merge primary normally and qualify the actual union before landing.
Only next-stage stats design proceeds meanwhile; no next H3 native edit.
Tooling/docs with unchanged runtime sources and products use the explicit
affected-level rerun rule.

The Park prerequisite is independently approved under design1cbc5217 and
reviews/10a-2b-parking-lot-design-review.md. Scope: Park's precision marker,
borrowed-storage header contract, the gc_walk part/test_gc fixture, plus a
DWORD-only TYPE_WORDS correction and TestRootCheckC controls. Keep every caller
opaque; the exact caller output slot must remain covered across unpark while
unattached. Future3a must explicitly anchor that slot if converting its caller.
V1's METH_O declaration mismatch and actual release inline-audit failure remain
preserved. The parser failure is the Windows DWORD cast, not PyTime_t. No
inactive-branch omission, effect waiver or outlined allowance is approved.
The subsequent full-body check exposes a CPP-spanning else. Exact proposal
688badc429a7 is approved: initialize used_monotonic=false in the timed branch,
set it after sem_clockwait returns regardless of outcome, then independently
run the realtime fallback only if false. This makes feature selection explicit
without changing the single-wait/error contract. Require copied actual old/new
function controls for feature absent/0/1, zero/positive/negative timeout and
success/EINTR/ETIMEDOUT/fatal outcomes, including exact wait kind/deadline/count.
No Windows/pthread body or root/effect policy changes; copied controls establish
no native platform coverage. Recheck complete source, linked inline audit and
actual native builds after refreeze. General partial-statement CPP AST support
is outside this bounded change; no parser branch is omitted.
V3 six-file patch94d68f30d309 clears source review; both gate0s and linked
audits pass1,924tests. Its71 bounded checks pass with no skips,5,857collections
and zero fallback/unpinned. Classification-negative/restoration reportdb936
passes. The bounded summary632414b6 is not final qualification.
Clean fixup1bfe1a57145 is pushed; bridge604ef3fb preserves the six tested
inputs and542 release/debug native paths, with only two documentation changes.
Strict stale100 passes release/debug off and debug kill; compiled31 checks
pass with their existing tailcall skip and intentional unpinned-counter event.
Exact pilot26a55 clears review40a38bae and passes six transactions per build:
five1/0 selectors plus complete four-module501tests/116release or101debugskips,
no protected skip/zero fallback. The first iterations take51.203/144.224s;
these qualification timings are not performance measurements. Reviewed
nineteen-only plan0d84e7d2/adapter9ace5493 now runs114 new transactions per build,
with pilot iterationzero bound unchanged. At12:45 release81/114 and debug35/114
pass, with no failure/fallback or exact-contract drift. Complete markers must
re-audit all original journals. No full profile or O1 adoption follows yet.
At13:00 release completion63235055 independently re-audits all114 new jobs;
with its pilot this is120transactions/20iterations,10,120tests/2,320 recorded
skips and zero fallback. Debug71/114 new jobs pass at13:00 and96/114 at13:12;
both campaigns subsequently complete. Debug completion172f480d has120 total
transactions/20iterations,10,120tests/2,020 recorded skips including pilot and
zero fallback. Its new nineteen record264,498collections/61,810 report-only
unpinned. All old source identities remain unchanged in those receipts.

Separate make-only support plan71340975/a6f4ba2b clears review after a named
Makefile/pyconfig pre/post guard closes its original configuration gap. ASan
make stops BUILD_FAIL at the actual inline audit after73.901s: precise Park
calls outlined raw-mutex Lock/Unlock wrappers lacking precision classification.
No smoke/test ran; this is separate from the known allocator-test failure.
Proposal34b5e35e adds only two wrapper precision markers; designfb291817 is
source review clears as recorded below. Current1bfe repetitions stay frozen;
no audit exception, support retry or tailcall make is granted. The conditional
transition ruling below governs the later source edit.
Copied source/compiler controls2d80/a6c pass and clear independentb7d review:
both real header bodies are checked, each held-local negative reaches its Slow
call, linked old2/new0/omit1/1 findings and symbols/sections/DWARF agree.
Transition8efe clears reviewdacaf. Root grants a one-time source/linked-control
gated commit for ONLY two markers after both completed matrices/quiescence and
preserved boundary/exact executor review; it establishes a clean new identity,
not nativegate1/PASS or landing. Fresh make/inventory/comparison must stop for
root's affected-coverage decision. No blanket rerun or automatic reuse.
Actual quiescent boundary8b7c/validation7996 preserves1,144 expanded product
paths and597 copied payloads, with962 original receipt/artifact paths. Review8740
clears exact executor4e44/spec ef5f. Root grants the sole two-marker clean
successorce2ac8991ee under the recorded exception, binds new-plan1074, then
sequential make-only default/debug/ASan. Every new product remains unqualified;
actual source/code/precision/CFI/root-DWARF comparison and root disposition must
precede affected checks. No standalone smoke/full/tail or landing grant follows.

The coordinator explicitly carries unchanged legacy31-selector/11-module
evidence forward at8ce, supported by body correspondence42350872, all seven
opaque caller contracts, new real-Park/class controls and the required fresh
full chain. No old receipt is renamed as new-head20/20; this is a bounded
ruling, not a general reuse rule. Revised proposal4b87878d preservesca275v1.
After clean fixup commit and source/native/helper rebinding, each completed default and
O1-debug repeat plan required20 fresh individual selections of EACH of three
thread-pool regressions and BOTH eval-probe methods, plus20 complete combined
orders of the three thread modules and test_gc:120 transactions per build.
Protect39 actual selectors. The four-module listing has501 cases; measure
actual counts/skips and freeze the first iteration per build for coordinator
review before launching the other19. Run whole changed test_generated_cases
once/default, debug and debug kill; it was not part of the old11-module cohort.

Fresh strict stale100/compiled checks, ASan14+2 at kill/N0 and tailcall bounded
support remain required. New ASan runs need complete pre/post native inventories;
the old operational provenance variance is not prospective permission.
Fresh release full, debug normal+kill full and stress11 on both builds remain.
Rebind the same filtered release full N100000 policy to the exact new candidate;
do not add filters. Cext expects16 tests with its seven exact existing skips.
Extras must bind consumed compiler/headers and all19 optional receipts, with
four unavailable-header modules explicit. Review exact plans and pilot scope
before any full profile; serialize full suites, keep two slots and frozen
inputs, and do not queue a same-build waiter. O1 adoption remains separate.

Source-only H3 handoff062da730 identifies five overlapping paths that textually
compose but still require semantic review and regeneration on the actual union.
Prospective serial ownership is storage union, H3 statistics, bounded H1 frame
passing, then H3 transport/renumbering. Fixing the known op<256 counter omission
first improves attribution and stats/non-stats interface qualification; it is a
scheduling choice, not a new H1/H2 prerequisite. The baseline binding audit
6bd23aca confirms old upstream srcdir points at the active fork; preserve old
results and prepare a separately owned exact-upstream environment and runner
identity. No baseline checkout/build/run or measurement is approved by that
source-only document. A subsequent owned-source grant establishes clean321925
and5806 tracked inputs. Revised runnerd853/plan419a closes launch blockers;
the exact configure-only grant succeeds with final2dd5/inner1107/products379f.
Joint closure8908 clears actual-configuration reviewde24: O3/NDEBUG, frame
pointers, zero-initialization and disabled JIT/stats/PGO/LTO/BOLT match policy.
Separate make-only V3a3f271/plan d08d enforces accepted final/config closure;
independent reviewa6c3 clears it and root grants exact make-launch74e1 once.
Normal bootstrap/freezing/sysconfig/checksharedmods are disclosed build steps.
No standalone qualification, origin witness or timing is included. Supplementb960
binds341 pre-make external candidate paths; it does not prove every consumed
header/library. Remaining dependency closure must close before final timing.
Upstream `CFLAGS_CEVAL` is empty; the fork deliberately adds `-fno-gcse`. Keep this
source-specific difference explicit rather than claiming every flag matches
or isolating object-model effects. Before checkpoint-I parity, require upstream
same-toolchain `-fno-gcse` sensitivity or select a predeclared fully matched eval
policy from evidence. No extra configure/build is required now for that later
comparison; H1 parent/changed timing still requires identical eval policy.
No parity or speedup is claimed from setup or configure.

Hourly reporting: while work is active, approximately every hour the
coordinator updates this state table and `README.md`, appends a dated
progress entry to `WORKLOG.md`, and gives the user a summary of heads,
landings, evidence, blockers and the next hour's work. Re-estimate missed
checkpoints from evidence; the pre-takeover dates are historical, not
current promises. The first update after this restart is recorded at
2026-10-29 06:45 PDT, followed by07:41,08:44,09:45,10:45,11:45 and12:45 PDT.
The 13:45 update records the new 2× target and revised validation policy.
The14:49 update records six successful branch profiles and the final
stress100k interpreter/HTTP failures, holding landing. The15:45 update adds
terminal timeout/trigger diagnosis, the first29-row elapsed diagnostic0.828×,
isolated writer builds and H3 statistics implementation/source review.
The next is due about16:45 PDT while work continues.

The revised [pre-JIT performance roadmap](design/work/pre-I-performance-roadmap.md)
sets roughly 2× whole-workload interpreter throughput as the target before JIT.
It also supersedes the blanket twenty-pass rule prospectively. Use fail-old/
pass-new controls, one affected-module pass per relevant configuration and
broad coverage early; reserve expensive repetition for a named hypothesis,
recorded exposure and bounded stopping rule. Existing 20/20 receipts remain
historical. No completed campaign is automatically repeated after a small
source change, and no old receipt is relabeled as a new-source pass. Current
ROOTS2b's fresh mode checks and six profiles now pass; finalstress100k fails
and must be diagnosed before tail/extras/landing. ASan14+sort/functools ran
once as16 with the exact accepted allocator filter and actual kill/stress0.

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
- `worktree.sh drop <step>` removes the worktree and branch after the step
  has landed.

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
| 1 | the rung's targeted test files (`gate.sh 1 BUILD test_a test_b ...`) | before every commit |
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

### Current coordinator ruling, 2026-10-30 after integration75e8

**Supported GC milestone complete.** Qualified source4da292b1294 is integrated
as75e8a1e0c01, tree8a309fd40dd. Runtime library/tests remain dac6; ordinary
native images are3c6 and ASan images f945. The actual integration differs
from4da only in15 declared project Markdown files. Final index12f1ea31 binds
276 references; postmergef56e0b6b binds20. Independent source/evidence LAND
review and both release/debug off/N0 smokes clear:1,935 tests each, exact80/50
skips, zero fallback/unpinned and verified cleanup. No image is called built
from primary or relocated from its qualified serving tree.

Both fresh O1-debug normal/kill fulls pass51,240 tests/3,739 skips,564 modules,
zero fallback. Release/debug stress11 pass3,104/148 and3,104/113 at actual
N1000, zero fallback. Unpinned counts remain report-only. Release coverage is
an explicit557+fresh7 assembly (51,240/3,794), not a rewritten uniform full;
ASan16 is15+fresh memoryview (4,026/141), plus separate local4 changed-path
coverage. Original Unicode7/owner15 retain their true f945 scope. Parent
multiprocessing sanitizer skips do not prove processes-leaf execution.
No root aggregate is apportioned to carried module rows.

Cext16/exact7 skips and unchanged regeneration pass at dac6. The original
traversal batch failed on stale ownership metadata and stays failed. Exact
one-file correction4da changes no runtime/test/checker algorithm; corrected
traversal and first19 borrowed-header checks pass with four unsupported
modules explicit. The original release nativeSUCCESS/qualificationFAIL,
scanner oracle failures, ambient source build and all earlier invalid
launches remain preserved. Source4da and all three original588-product maps,
configuration/tool/alias bindings and2,915 borrowed headers are checked.

The declared scope is Linux x86-64, one main interpreter, ordinary threads
and the shared GIL. Main-only admission, rooted active callback/error state,
one nonwaiting drainer, acquisition readiness/retention, sealed consumers,
same-owner XID and ordinary SharedMemory disposal are supported. No stale
callback/buffer-completion blocker remains for this profile. Generic cleanup
slots, dependency DAGs, foreign/adoption, parallel callback progress and
Windows persistent holders are not implicitly promised. Host-returning
finalization is distinct from actual process exit. Further root precision,
reader/checker coverage and platform work remain separate; hybrid coverage
cannot be removed by implication. The [owner ledger](design/work/10a-buffer-safeguard-removal.md)
and [landing review](design/work/reviews/gc-main-only-final-landing.md) name the limits.

**Current work: matched upstream eval-nogcse qualification.** The pinned
owned upstream321925 source and its stock products are preserved. The separate
O3 upstream cohort with CFLAGS_CEVAL=-fno-gcse has passed configure/make and
independent build review. Bounded correctness/origin checks precede any quiet29 refresh.
The ordinary fork O3 products are reused with their actual native/runtime
bindings. No refreshed performance score, ABI implementation or new H1/H2
landing is claimed. H3 is landed; H1 frame passing and the inert H2 decoder
remain isolated. The [status report](design/work/pre-I-interpreter-status-and-opportunities.md),
[first-slice decision](design/work/pre-I-first-slice-decision.md) and
[timing plan](design/work/pre-I-qualified-timing-refresh.md) precede new shared
interpreter work. Roughly2× remains soft guidance, not a numerical gate.

**Observable behavior first.** Integration tests primarily check useful
liveness/reclamation, readable data, usable resources, callback effects and
memory growth over a declared interval. Critical rollback, root handoff and
ownership unit invariants remain appropriate. Do not prescribe accidental
collection timing or internal layout, weaken an unexplained regression,
relabel a failed run, or restart whole ladders for unchanged source. New
costly repetitions require a named hypothesis, exposure budget and stopping
rule. Full/perf-map work serializes; normal admission remains two slots.

### Historical predecessor qualification rulings

The following records retain their actual source/configuration and exception
scope; they do not add supported callback/buffer blockers to the completed75e8
milestone.

Qualified runtime merge d6a23b9fe0c has exactly the tested5a45 native/test/tool
source; its original19 differing files are project Markdown prose. Both
required post-merge smoke rows pass on the explicitly reused qualified images.
No rebuild or repeated full suite is required for this documentation update.

At the historical pre-safety checkpoint, Queue2a eager preparation remained
HOLD and unexecuted. The then-revised source-only
late-preparation candidate keeps successful ordering at the baseline boundary
and aborts before deallocation on ordering OOM. Already routed calls retain
the existing partial-delivery contract. Independent source review precedes
bounded native validation. Its finite lock audit identifies inherited weakref
and range-registry detach/handoff hazards; changing one waiter alone is unsafe.
Those snapshot/debug and completion concerns required separate fixes; later
safetyff6 and supported75e8 qualification close the declared profile. This
earlier checkpoint itself earned no such credit.

Coordinator checkpoint, 2026-10-30 08:00 PDT: V2 sweep-OOM7f445 completes
its bounded checks: release/debug-kill focus6 each and four affected modules
1,561 tests each, zero fallback; copied old fallback fails exactly the named
no-sweep assertion. Root independently checks12 receipt/cleanup/plan hashes
and16 raw artifacts. Global qualification remains false pending the actual
integration candidate. S3 release9/0 plus affected modules pass; its debug
audit correctly rejects an unmarked new helper. Reviewed annotation-only
009fa rebuilds both cohorts; old evidence/failure stays unchanged. Separate
work-stack/MARKED-OOM source implementation is authorized from that clean
successor, with no native grant yet. Completion lifecycle and buffer-release
protocols remain design work. The finite repair sequence is in
`design/work/gc-closure-plan.md`; no interpreter timing has resumed.
