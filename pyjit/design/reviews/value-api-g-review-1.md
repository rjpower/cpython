# Review 1: value API step G, the R27 gate owners (proposal)

Reviewed: [value-api-g.md](../proposals/value-api-g.md) (revision 1, 535
lines, uncommitted, 2026-11-04), against
[value-api-v2a0.md](../proposals/value-api-v2a0.md) §1.2 and §3,
[value-api.md](../value-api.md) (R26, R27) and the common step brief. Code:
`build/worktrees/pyvalue-rc-universal`, read-only, at `c71a3ca6aa2` with
V2a-0 part 2 staged. Probes: GCC 15.2 at `-O1`, `-Og`, `-O2` and `-O3`, x86-64,
in a scratch directory that has since been removed. Clang is not installed
on this machine, so no clang claim was tested.

## Verdict

**Accept with changes.** The core of G is sound. The owner is the code that
the box serves (G1). The kind and the site travel as one argument instead of
thread-locals (G2). Owners resolve offline by compile unit (G8), so `.c.h`
code goes to the unit that includes it. Within one ELF object, return
addresses are attributed correctly in the gate build: in-tree code has no
sibling calls, entries are `noinline`/`noipa`, and the `musttail` dispatch
can only move an owner to another core function (finding 13). The
proposal's claim in §6, "a gate run can fail falsely but cannot pass
falsely", does not hold. The gate passes falsely in three ways:

- **It classifies by object path.** A core box made in libpython, in
  `_testembed`, in a copied venv `python` or in anonymous memory is counted
  as external (finding 1).
- **Whole processes and interpreters go uncounted.** This covers
  subinterpreters, children that end in `os._exit` (every multiprocessing
  worker), and crashed workers (finding 2).
- **Boundary owners are unchecked.** Any core site can state
  `_Py_BOX_OWNER_FOREIGN` or a callee owner, and the box leaves the debt
  (finding 3).

All three can be fixed inside G for about 1.5 agent-hours.

## Findings

| # | Severity | § | Problem | Evidence | Change |
|---|---|---|---|---|---|
| 1 | must-fix | 1.8, G9 | **"External" is decided by object path: a false pass.** Core code linked into any object other than the build's `python` and `build/lib.*/*.so` is classed as external, so it is not debt. That covers: libpython in an `--enable-shared` build (the whole core); `Programs/_testembed`, which links libpython and runs Python with stats on; a venv created with `--copies` (`test_venv`), whose realpath is not the build's `python`; and code in anonymous memory, where `dladdr` fails (perf trampolines, any JIT), for which the dump format has no case | `_testembed.c:908` `putenv("PYTHONSTATS=1")` under `Py_STATS`, called by `set_all_env_vars()` in six test functions. G9 table, row "external". §1.4's dump example has no line for an address that resolves to no object | Classify by compile unit, not by object. An object any of whose DWARF CUs name a file under `$SRC` is in-tree, wherever it is loaded from; identify the build's `python` by its GNU build-id. Class an address in no object, or in an object under `$SRC`/`$BUILD` without DWARF, as unresolved (debt). Class an object as external only when none of its CUs lies in the tree. Add `Lib/test/test_cext/` and `Lib/test/test_cppext/` to the compat-test list, or their in-tree CUs become core-pending (a false fail). Until the self-test covers libpython, configure refuses `--enable-shared` with `--enable-pystats`. Run the self-test through a real dump: a runtime address of `PyNumber_Add` → `Objects/abstract.c` |
| 2 | must-fix | 1.4, 6, Q5 | **Lost processes and interpreters: a false pass.** (a) Q5 is already settled: a subinterpreter's counters are freed without being merged, and only the main interpreter dumps. (b) The dump runs only at finalization, so any child that ends in `os._exit` writes nothing: multiprocessing workers under fork and under forkserver (the Linux default since 3.14) and `concurrent.futures` process pools. (c) A child created by fork inherits the parent's counters and would count them twice if it dumped. (d) A worker that crashes, times out or overflows the C stack also leaves no dump | (a) `pystate.c:480–484` frees `interp->pystats_struct`; `ceval_gil.c:566` `_PyEval_Fini` dumps only `tstate->interp`. (b) `os._exit` calls `_exit` directly; `popen_fork` and `forkserver` children call `os._exit(code)` | (a) At interpreter deletion, merge the counters and the owner table into a runtime-level accumulator under a runtime mutex. Subinterpreters can have their own GIL, so the interpreter lock is not enough. The dump adds the accumulator. (b) Under `Py_STATS`, `os._exit` dumps before it calls `_exit`. (c) `PyOS_AfterFork_Child` clears the counters and the table. (d) At stats start, each process creates `<rand>.run` in the dump directory, and its dump replaces it. `suite` lists every `.run` left behind with the test that owned it, and `--gate` fails on any that is not on a named list (crash tests). New `test_pystats` cases: an `_interpreters` box, a fork child that calls `os._exit`, and a forkserver worker, each counted exactly once |
| 3 | must-fix | 1.1, 1.7 | **Boundary owners can be stated anywhere: a false pass.** `_PyValue_AsObjectFor(v, at)` accepts any owner, and `_Py_BOX_OWNER_FOREIGN` is a constant that classes a box as non-debt. A wave worker who is asked to reach zero can restate a core box as a foreign box, or as a callee box, and the gate passes. `test-site` guards `BOX_SITE_TEST`, but nothing guards these | §1.1 table: `For` forms "any boundary kind, stated"; G9: foreign "no" debt | Add a `boundary-site` text rule next to `test-site`. `_Py_BOX_OWNER_FOREIGN` may appear only in `Modules/_ctypes/` and in the `_testinternalcapi` helper. `_PyValue_AsObjectFor` and `_PyBorrow_*For` may appear only in a fixed list of files: `call.c`, `native_scope.c`, `getargs.c`, `borrow.c`, `pycore_value_slots.h`, the adapter units and `_ctypes`. Adding a file to the list needs a line in the design. Self-tests for both |
| 4 | should-fix | 1.4 G10, Q2 | **G10 breaks `test_embed` on every stats build.** `_testembed` sets `PYTHONSTATS=1`, and five isolated-configuration tests expect `_pystats` to stay false. Separately, `remove_python_envvars()` strips `PYTHON_PYSTATS_DIR`, so `_testembed` writes its dumps to `/tmp/py_stats`, which is shared with other tasks | `test_init_isolated_flag`, `test_preinit_isolated1`, `test_preinit_isolated2`, `test_init_isolated_config` and `test_preinit_isolated_config` call `set_all_env_vars()`; `CONFIG_COMPAT['_pystats'] = False` (`test_embed.py:860`) | Keep G10, and set the expected `_pystats` of those five tests to 1 under `Py_STATS`. List `test_embed` in §4 and §2. State the gap: children started with a cleared environment (`_testembed`, `env={}`, `script_helper`'s `__cleanenv`) are not counted. Measure the gap once with an audit hook on `subprocess.Popen` in the gate run |
| 5 | should-fix | 3.1, 4 | **Stats resets in the suite.** `sys._stats_clear()`, `_stats_off()` and `_stats_dump()`, which clears after dumping, erase a worker's earlier boxes and stop counting later ones. `test_pyvalue_box_sites` also hard-codes `/tmp/py_stats` and asserts exactly one new file there, so under `PYTHON_PYSTATS_DIR` it fails, and outside it it races other tasks | `sysmodule.c:2285–2286`; `test_sys.py:1438–1441`; `test_pyvalue_box_sites.py:20`, `:34–38` | Print `Stats resets: n` in each dump. `check_r27.py` lists the processes that reset. Move the in-process measurements of `test_sys` and `test_pyvalue_box_sites` into a subprocess, as `test_pystats`'s `run_test_code` does, and read `PYTHON_PYSTATS_DIR` there |
| 6 | should-fix | 1.7, G11 | **The test class rejects V2a-0's own adapter tests.** `_testinternalcapi/value_slots.c` defines types that have only object slots, and `test_value_slots` drives them through the slot adapter. Those boxes carry the adapter's family class, and their owner lies in `_testinternalcapi`, so the rule "a `_testinternalcapi` box of any other class fails" fails the suite | `value_slots.c:375–380` (`VIntCopyAdd`), `:514` (`notimpl_add`) | Apply the TEST-site rule to CORE boxes whose site lies in `_testinternalcapi*`. Class boundary boxes owned there as "test" and let them pass |
| 7 | should-fix | 1.3, G5 | **Two marks for one set.** At W5, §3.1 requires every ownership row to be both `Py_VALUE_COMPAT` and `_Py_COMPAT_ENTRY`. A rule that requires two marks on the same functions shows that they mark the same thing. The real difference is strength: an entry exists before its core callers are swept. The attributes do not need the definition: `noinline` and `noipa` on a declaration apply to the definition in GCC, and clang's `annotate` is inherited by redeclarations | §3.1 static check; pyport.h `Py_VALUE_COMPAT` | Use one mark per function in two strengths. Add `Py_VALUE_ENTRY` on the declaration (G5's attributes), and make `Py_VALUE_COMPAT(R)` expand to `Py_VALUE_ENTRY` plus `unavailable` in value-native units. A wave's marking replaces the token on the same line. Only definitions without a header declaration keep `Py_VALUE_ENTRY` there permanently: the thunk macro families and the value wrappers, whose units take their addresses. compat-body then has one attribute to look for, and the W5 check becomes "carries `Py_VALUE_COMPAT`" |
| 8 | should-fix | 1.1 | **The identical-`.text` test is too narrow, and `annotate` is not free.** A clang function annotation creates `llvm.global.annotations`, which takes the function's address. A static marked function then keeps an out-of-line copy and loses optimisations that need all its callers to be known. G also edits about 20 units, and the test compares five of them | §1.1 test; §2 lists `object.c`, `value_ops.c`, `call.c`, `native_scope.c`, six container files and `getargs.c` | Make `_Py_COMPAT_ENTRY` (or `Py_VALUE_ENTRY`) empty in every non-stats build. The check pass already adds `-DPy_STATS` (§1.3). Diff `objdump -d` of every object in both release builds (gcc and clang), and allow a difference only in `getargs.o` |
| 9 | should-fix | 1.4 | **The table pointer inside `PyStats` is erased by clear.** `stats_zero_all` uses `memset` on the whole `PyStats`, so `sys._stats_clear()` would leak the table and drop it. Free-threaded builds keep a `PyStats` per thread, so the table either races or would need one per thread | `pystats.c:549`, `:461`; `_PyStats_ThreadInit` | Keep the table in `PyInterpreterState` beside `pystats_struct`, and clear its contents explicitly in `_Py_StatsClear`. In configure, refuse `--disable-gil` together with `--enable-pystats`, because free threading is a non-goal |
| 10 | note | 1.2, Q8 | **Q8: drop the NULL sentinel.** The sentinel is a second meaning for NULL, and it needs a debug assertion to catch misuse | §1.2 | Let `_PyBorrow_TupleItems` and `_PyBorrow_ValueVector` build their own `at` from `_Py_BOX_CALLER()` and call a static helper that they share with the `For` forms. NULL then always means "no owner", which classes as unresolved |
| 11 | note | 9, Q6 | **Q6 needs neither W2 nor an exemption.** The box is an artefact. `PyObject_GenericHash` already hashes a number's value identity, so the inline arm can compute the same result from the word without a box | `value_ops.c:168–183`; `pyhash.c:146–150`; `pyvalue.c:151` `_PyObject_ValueIdentity` | Add `_PyValue_Identity(PyValueBorrow)` to `pyvalue.c`, mirroring `_PyObject_ValueIdentity` (R26 holds). Then `_PyValue_GenericHash` returns `Py_HashPointer((void *)_PyValue_Identity(v))`. Do it in G-1: 10 lines and a test that the result equals the result for a box |
| 12 | note | 1.4 | **`dli_fbase` offsets are ELF virtual addresses only when the first `PT_LOAD` has `p_vaddr` 0.** That holds for PIE executables and shared objects. A non-PIE `python` would be misresolved in either direction | §1.4 "Offsets from `dli_fbase` are ELF virtual addresses" | Record each object's lowest `p_vaddr` through `dl_iterate_phdr` and add it to the offset, or refuse a non-PIE build. Finding 1's runtime self-test catches either error |
| 13 | note | 1.6, 6 | **Tail calls and the return address (item 1).** Within an object, attribution holds. With `-fno-optimize-sibling-calls`, `musttail` still compiles and runs under GCC 15.2 with `no_callee_saved_registers`; `preserve_none` is ignored by GCC 15.2. Handlers `musttail` only to handlers (`ceval_macros.h:105–123`), so an entry's return address is a core call instruction or a core activation: debt, never a pass. Inlined `PyValue_AsObject` sites record their own unit. An external sibling call can only move an owner to its core caller | Probe: `caller()` → `ent()` returns `caller+9` with the flag and main's call site without it | None for G. Clang is unverified here. configure's `musttail` check (`configure.ac:7915`) runs after the pystats branch, so an incompatibility would stop configure rather than pass silently |
| 14 | note | 1.1, item 2 | **The argument's cost in stats builds.** `_PyBoxAt` is 16 bytes, passed as two integer registers under SysV. The new owner parameter adds a stack argument only to `vgetargskeywordsfast_impl`, which already takes eight (`getargs.c:2340`). The stack growth that matters comes from the lost sibling calls and the `noinline` entries. `test_call.TestRecursion` asserts `RecursionError`, not a depth, so it is unaffected. A C stack overflow that crashes a worker is covered by finding 2(d). `_PyCall_BoxOwner(callable)` runs on every adapter call in stats builds, not only when a box is made | `test_call.py:1236–1264` | State that `_PyCall_BoxOwner` runs no Python and increments no counter |
| 15 | note | 1.3 | **The clang-query pass with `-DPy_STATS`** checks only the stats arm of `#ifdef Py_STATS` code for the other rules. Examples are `_PyValue_AdapterBox`'s two macro arms and `TAIL_CALL_PARAMS` | `pycore_value_slots.h` staged diff | Run compat-body and compat-capture as a second pass with `-DPy_STATS`, over the units that define entries only |
| 16 | note | 1.3 | **An entry that calls another entry**, or a shared helper that does not pass the owner down, makes the outer entry the owner. That is core debt for an extension's call, so a false fail | §1.3, the dict helpers | compat-capture also reports a call from an annotated definition to an annotated function |
| 17 | note | – | **Outside G: a GCC 15.2 miscompile.** Debug builds use `-Og`. At `-Og` and `-O1`, GCC drops a value that is live across a call to a `no_callee_saved_registers` function. At `-O2` the code is correct. This is the GCC convention of the tail-call handlers. `-fno-ipa-ra` does not help | `long c = caller(); long r = f(100, 0); printf(c)` with `f` `no_callee_saved_registers`: `c` prints a stack address at `-Og`/`-O1` and 1 at `-O2`/`-O3` | The coordinator should check that ceval's GCC debug build keeps no value live across its first handler call, or build the handlers at `-O2` |

## Answers to the open questions

| Q | Answer |
|---|---|
| Q1 | Accept. Either way, the owner is the Clinic wrapper's unit: `_Py_BOX_CALLER()` and `_PyCall_BoxOwner` give the same unit. So only the per-kind reports move, and the family class keeps them comparable with §1.2 |
| Q2 | Accept for stats builds, together with finding 4's `test_embed` change and the measured gap. Without G10, every `script_helper` child that runs with `-I` would go uncounted |
| Q3 | Accept one configuration. Also refuse `--enable-shared` until finding 1 lands, and refuse `--disable-gil` (finding 9). Keep `annotate` in stats builds only (finding 8). Users of stats builds lose sibling calls in the extensions they build, which nobody times |
| Q5 | The counters are lost today (finding 2a). Make the merge part of G, under a runtime mutex, and do not leave it as a check |
| Q6 | Neither W2 nor an exemption. Remove the box in G-1 (finding 11). R27's text admits no exemption for identity, and none is needed |
| Q8 | Reject the sentinel (finding 10) |

## Recounts

| Claim | Recount (staged tree, definitions included) | Verdict |
|---|---|---|
| `_PyBorrow_TupleItems` 217 calls in 77 files | 221 in 78 | holds |
| `_PyValue_AsObjectAt` 75 calls in 22 files | 84 in 25 | holds |
| 29 thunk exits; 8 Kind calls; 5 `TupleItemsAt` in `getargs.c` | 29; 12 including the 4 definitions; 5 | holds |
| `getargs.c` sites `:388`, `:600`, `:1780`, `:2438`, `:3008`; keyword reads `:1867`, `:1990`, `:2471`, `:2714` | identical | holds |

## Estimate (item 7)

The fan-out assumption helps G little. Most of G is new code: the table
and the dump, `check_r27.py`, and two rules. Only G-1m and the
`_testinternalcapi` sites are mechanical. The parts of G-2 that workers can
take are the pass-down through the six `getargs.c` functions and the
configure and Makefile hunks, about 0.5 h. Moving them shortens the lead's
chain.

| Part | Proposal (h) | Review (h) | Why |
|---|---:|---:|---|
| G-1, lead | 1.75 | 1.5 | the 47 exit, getter and Kind sites go by script from V2a-0's site table; +Q6 |
| G-1m | 0.25 | 0.25 | |
| G-2, lead | 2.0 | 1.75 | getargs and configure go to a worker; +findings 2 and 9: merge, fork, `_exit`, markers |
| G-2w, worker | – | 0.5 | `getargs.c` pass-down, `configure`, `make check-r27` |
| G-3a | 1.5 | 2.0 | CU- and build-id-based classes; `.run` accounting (findings 1, 2) |
| G-3b | 0.75 | 1.0 | `boundary-site` rule (finding 3) |
| G-3c | 0.75 | 1.0 | `test_embed`, `test_sys`, `test_pyvalue_box_sites` (findings 4, 5) |
| Acceptance, lead | 1.0 | 1.5 | the first gate-build suite run will surface stats-only failures, so expect two runs at 8–10 min each; `.text` diffs on two compilers |
| **Total** | **8.0** | **9.5** | lead 5.0, workers 4.5 |

Wall clock stays at about 5–5.5 h. The lead's chain is G-1, G-1m, G-2 and
acceptance. G-3a can be written against §1.4's dump format while G-2 runs.

## What holds

- **G1 and G14.** Naming the code that a box serves makes the classes
  checkable. When an owner is unknown, it defaults to core, so errors
  overstate debt and never hide it.
- **G2.** The argument replaces V2a-0's thread-local swaps. It vanishes as a
  macro argument outside stats builds, so release builds evaluate no owner
  expression.
- **G3, G4 and level 0 only.** Storing return addresses minus one, and never
  using `__builtin_return_address(n > 0)`, is right. 65,536 entries at about
  15% load per process is ample. Drops are counted per kind and fail the
  gate.
- **G8.** Mapping by compile unit, rather than by `addr2line`'s file, puts
  Clinic and generated `.c.h` code in its including unit.
- **G13.** G moves no box, and the acceptance run proves it on the kernels.
- **The kernel part.** It checks absolute zero in every process, startup
  included, so it needs no attribution and none of findings 1–3 affect it.

## Review 2

Reviewed: revision 2 of [value-api-g.md](../proposals/value-api-g.md)
(605 lines, 2026-11-04), its "Changes after review 1", and §1.2–§1.8, §3.1,
§6 and §9. Probes ran in a scratch directory, since removed, with clang 21
(`/usr/bin/clang-21`, `clang-query-21`) and GCC 15.2. All clang claims in
the revision were checked.

**Verdict: accept. Implementation can start with G-1.** Findings 1 and 2
are closed. Finding 3 is closed for the named forms, but the rule can be
bypassed through a parenthesised call. The fix is a change to one rule
that G-3b can make (R2-1 below), so it does not block G-1 or G-2. The rest
of revision 2 holds as written.

### The three false-pass risks

| Review 1 | State | Check |
|---|---|---|
| 1. External by object path | **closed** | Classes come from the compile units of an object found by build-id. A `--copies` venv and `_testembed` resolve as core. An address in no object (`?`) is unresolved. Shared builds are refused. Copied objects keep test-built extensions resolvable. Precedence for objects without DWARF is still open (R2-3) |
| 2. Lost processes and interpreters | **closed** | Interpreter deletion merges into an accumulator under a runtime mutex. Order checked: `finalize_subinterpreters()` (`pylifecycle.c:2327`) runs before `_PyEval_Fini()` (`:2510`). Fork clears the state and writes the child's own `.run`. `_exit` and `exec*` dump first. Leftover `.run` files fail the gate unless the crash list names them. Resets are counted and allowed only in `test_pystats`. `subprocess._args_from_interpreter_flags` does copy every `-X` option (`subprocess.py:469–478`), and multiprocessing spawn and forkserver use it (`spawn.py:93`, `forkserver.py:205`) |
| 3. Owners stated anywhere | **open in one form** | The rule keys on the names of the `For` forms. The `For` body is itself `(PyValue_AsObject)(ref, at)`, so a core site can write `(PyValue_AsObject)(PyValue_Borrow(v), _PyBox_AT(s, BOX_KIND_CALL_ADAPTER, x))`, or call `(_PyValue_BoxInlineValue)` with an `at`. Neither names a listed token. §6's claim "owners can be stated only in the boundary-site files" is therefore not yet true (R2-1) |

### Findings

| # | Severity | § | Problem | Change |
|---|---|---|---|---|
| R2-1 | must-fix (small; in G-3b) | 1.7, 6 | `boundary-site` keys on helper names, which a parenthesised call bypasses (above) | Key the rule on what makes a box a boundary box: the tokens `BOX_KIND_THUNK`, `BOX_KIND_SLOT_ADAPTER`, `BOX_KIND_CALL_ADAPTER` and `_Py_BOX_OWNER_FOREIGN` may appear only in the listed files, `pystats.[ch]` and `pycore_value*.h`. In the `-DPy_STATS` pass, add an AST rule: a `_PyBoxAt` compound literal whose `kind` is not the constant `BOX_KIND_CORE` fails outside those files. That also catches a literal `1`. Keep the name list as documentation |
| R2-2 | should-fix | 1.5, Q10 | **Q10: the audit hook has blind spots, and an unseen launch is neither counted nor listed.** (a) It matches a build product only as the executable, so `shell=True`, `sh -c "…/python -I …"`, `env -i …/python` and `os.system` launches escape. `os.system` is not among the hooked events. (b) `sys.addaudithook` is per interpreter, so launches from subinterpreters are unseen. (c) A launch can be wrongly predicted to count: the environment has `PYTHONSTATS`, but the argv has `-E` or `-I` and no `-X`. Q10's own cases are rarer than these: audit hooks cannot be removed from Python, and C-level execs go through hooked `Popen` | (a) Match `sys.executable`, its realpath and the other build products anywhere in argv and in a shell command string, and add the `os.system` event. (b) In stats builds started with `pystats_dir`, register the hook from C with `PySys_AddAuditHook`, which covers the whole runtime. (c) Keep the `.spawn`/`.run` cross-check as a gate check, not only for the first run. A launch predicted to count needs a `.run` whose ppid is the launcher's pid, or its exec chain's. A missing one fails the gate |
| R2-3 | should-fix | 1.8 | **Two cases have no class or two classes.** (a) An object without DWARF matches both "external" (none of its CUs lies under `$SRC`, vacuously) and "unresolved". Read as unresolved, a `ctypes.pythonapi` call through system libffi is a false fail. Read as external, an in-tree source built without `-g` in a temporary directory is a false pass. (b) A CU outside `$SRC` inside an in-tree object matches no row: `Modules/config.c` is generated in `$BUILD`, and `test_peg_generator`'s generated `parse.c` lies in a temporary directory | (a) An object without DWARF is unresolved if it is a build product by build-id (already refused), or if it lies under `$SRC`, `$BUILD` or the suite's `--tempdir` (`suite` sets it inside `$BUILD`). Elsewhere it is external. (b) Treat a CU under `$BUILD` by `unit_kind` like one under `$SRC`. Class any other CU of an in-tree object as unresolved |
| R2-4 | note | 1.4 | `_Py_PrintSpecializationStats` dumps `tstate->interp`, so an `os._exit` in a subinterpreter would dump only that interpreter | The `_exit`/`exec` dump and the final dump both write the main interpreter, every live subinterpreter and the accumulator |
| R2-5 | note | 1.5 | `PYTHON_PYSTATS_TAG` is ignored under `-I`, so an isolated child's leftover `.run` has no test name and cannot match the crash list. That is a false fail | Add `-X pystats_tag`, or let `collect` find the test through the ppid chain |
| R2-6 | note | 1.4 | Up to 16 workers copy the same unknown object to `objects/<build-id>` | Write to a temporary name, then `link()` or `rename()`, and keep the first copy |
| R2-7 | note | 1.1 | The `.text` test covers "every object G edits". In revision 2 that set also includes `posixmodule.o`, `pystate.o`, `initconfig.o` and the two `*_structs.h` users | State that each new field and hook is under `#ifdef Py_STATS`. The test already catches a slip |

### One mark in two strengths: sound

The ruling works, and the attribute claims it rests on hold.

- **Attributes on the declaration reach the definition.** With the
  attributes only on the declaration, and an `always_inline` capture inside
  the definition, `caller()` → `ent()` returns `caller+6` under clang-21
  `-O3 -fno-optimize-sibling-calls`. Under GCC 15.2 `noinline, noipa` it
  returns `caller+13` at `-O2` and `-O3`, and inside `caller` at `-Og`.
- **clang-query sees the inherited attributes.**
  `functionDecl(isDefinition(), hasAttr("attr::Annotate"))` and
  `hasAttr("attr::NoInline")` both match a definition that carries no
  attribute itself.
- **Both strengths together compile.** A definition whose declaration
  carries `noinline`, `annotate` and `unavailable` compiles with clang-21.
- **Q4 holds for clang.** `preserve_none` with `musttail` compiles under
  `-fno-optimize-sibling-calls` and runs 10⁸ tail calls without growing the
  stack. Without the flag, the return address moves into `main`, the
  sibling call that G7 removes.

One function then carries one token. The W5 check reduces to "carries
`Py_VALUE_COMPAT`". A falsely marked entry still resolves to its real
caller. In stats builds, `noinline` on functions marked later is harmless,
because their core callers can no longer compile.

### Estimate

10.0 agent-h and 5.5 h wall clock: agreed. R2-1 to R2-3 add about 0.5 h to
G-3a and G-3b together and do not lengthen the lead's chain.
