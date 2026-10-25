# Boil-the-ocean work log

Branch `weaver/python-boil-the-ocean`. Plan artifact: Loom `plan` rev 2.
Entries are appended in order; each milestone is also a commit.

## 2026-10-23

- Reviewed shellsim's ports tree and wrote the plan artifact (rev 1), then
  rev 2 after review: added the internal IR (idea 0), dropped the
  copy-and-patch JIT in favour of our own tier 0 and tier 1, V8 embedder API
  as the handle-API bootstrap.
- Phase 0 started. Layout decisions:
  - `boil/` holds this log and design notes (`boil/design/`).
  - `ports/` is the package ports tree, modelled on shellsim's.
  - Out-of-tree builds live under `build/` (gitignored):
    `build/baseline-default`, `build/baseline-jit`, `build/baseline-ft`.
  - Benchmark results go to `boil/bench/` as JSON plus a short summary.
- Machine: 16 cores, 62 GiB, gcc 15.2. LLVM 21 runtime libs present but no
  `clang-21` binary; installing `clang-21 llvm-21` for the JIT baseline.
- Ports tree landed (`ports/`, about 1000 lines of driver). Verbs: `build`,
  `check`, `bench`. Six ports pass `check` against the baseline interpreter:
  six, attrs, pycparser, markupsafe (C speedups), pyyaml (pure loader only,
  no libyaml headers on the host), cffi (ABI mode). One patch so far: cffi
  needs `PyConfig_Get("filesystem_encoding")` because 3.16 removed
  `Py_FileSystemDefaultEncoding` (10 patch lines). Everything is keyed by
  `abi_id` in `ports/sdk.json`, currently `cpython-3.16-refcnt-v0`.
  Host has no OpenSSL headers, so the target interpreter has no `ssl`; the
  host fetches `build_requires` wheels and the build venv installs offline.
- Design note `boil/design/02-tagged-values.md` landed. Decisions: JSC-style
  NaN-boxing of `_PyStackRef` now (raw pointers, doubles offset by 2^49,
  int32 at `0xFFFE_...`), inline floats in Phase 1, NaN stays boxed, value
  identity for inline ints and floats, internal loop indices move to their
  own region. Measured today's costs with `perf stat`: an int add is 164
  instructions per iteration, a float add 130. Seven-step sequence with an
  overhead gate at step 2. Coordinator decisions recorded in section 12.
- Design note `boil/design/01-internal-ir.md` landed. Decisions: 64-bit IR
  words (16-bit handler index plus three 16-bit fields, optional second
  word), 15-bit slots with a borrow bit, one typed feedback block per code
  object with seven kinds absorbing all 15 inline-cache structs, lazy
  lowering at first execution, in-place quickening of the handler field.
  Measured maxima on the stdlib: 803 slots per frame, 32 percent of
  instructions specialisable. Nine-step sequence; step 1 deletes tier 2 and
  the copy-and-patch JIT. Coordinator decisions in section 10.
- Baselines recorded in `boil/bench/`. On the 24-benchmark subset without
  PGO: the upstream JIT is 0.834 geomean against default (richards 0.32,
  nbody 0.60), free-threaded is 1.144. Peak RSS at startup is about 10 MiB
  for default and jit, 11.6 MiB for ft. The JIT number is the bar our tier 0
  must clear; it is larger than the 7 to 12 percent upstream reports for
  PGO builds, because our default build has no PGO.
- Measurement harness in `boil/tools/` reproduces both design notes'
  numbers. Started IR step 1 (delete tier 2 and the copy-and-patch JIT) in a
  subagent.
- Ports: added `native/libyaml` (new `configure-make` kind, static, linked
  into pyyaml's Cython extension) and `python/numpy` 2.5.4 (meson-python,
  `-Dallow-noblas=true`, no patches). Numpy builds in about 90 s on 16
  cores. `bench_ufunc.py` stresses small-array ufunc calls as a C API
  boundary benchmark: 2.40 s median on the baseline.
- Design note `boil/design/04-shapes.md` landed. Shape pointer replaces the
  `PyDictValues` header word; 30 slots and 64 shapes per type before
  dictionary mode; `__dict__` stays a live view; per-property kind byte with
  I32 and F64 reserved. Finding: the interpreter gain is neutral (15 versus
  16 instructions per hit), so shapes move to the end of Phase 1 as the
  foundation for tier 1 and unboxed fields. Separate finding to fix now:
  objects with a materialised `__dict__` never specialise attribute access
  (328 instructions per load instead of 71).
- Design note `boil/design/03-deferred-refcounting.md` landed. Hybrid model
  (deferred set as in the free-threaded build, existing local borrows kept,
  bias of 2^30 plus a flag bit, exact accounting only in full collections).
  Finding: about 1 percent fewer instructions and no measurable time change
  in Phase 1, so item 1c moves to the start of Phase 2 as the tracing GC's
  root enumeration. Phase 1 is now 1z, 1a, 1b, 1d, 1f, 1e in that order.
  Process note: add paths explicitly when committing; `git add boil` swept
  an in-progress draft into the shapes commit.
- IR step 1 landed: tier 2 and the copy-and-patch JIT are deleted
  (143 files, about 68,700 lines removed, 509 added). Base opcode numbers
  and cache layouts are unchanged, so pyc files and `dis` offsets are
  stable. `sys._jit` is a stub reporting no JIT. Full suite passes on the
  default build except `test_remote_exec_deleted_static_executable`, which
  also fails on the baseline. Debug and free-threaded builds pass their
  targeted sets. `jit_unwind.c`, `jit_publish.c` and the perf trampolines
  are kept for our own tiers. The ports recipe loses its `jit` variant.
- Added `boil/tools/pyperf_run.sh`: seeds the pyperformance venv with the
  host `uv` (the target interpreter has no ssl, so its pip cannot reach
  PyPI) and runs the plan's subset. A first neutrality run of the
  post-deletion build landed at 1.044 geomean, but it ran while two agents
  were compiling and testing with 16 jobs; outliers of 0.81 and 1.63 on
  unrelated benchmarks mark it as noise. Kept as `*.noisy.json`; the real
  measurement runs on an idle machine before the next code commit.
  Rule from here: benchmark only when no build or test run is active.
- Tagged-values step 1 landed: internal loop and `lasti` indices are now
  `PyStackRef_*Index` with `Py_INDEX_TAG`; `Py_INT_TAG` is free for real
  ints. Pure rename plus three header tightenings (`PyStackRef_TYPE` and
  `LongCheck` no longer claim an index is an int). Full suite passes on
  default; debug and free-threaded targeted sets pass.
  Finding: the `Py_STACKREF_DEBUG` build is broken upstream before any test
  runs: `take_ownership` in `Python/frame.c` closes the source frame's
  executable ref and restores it on the wrong frame (from GH-127875), and
  `BINARY_OP_INPLACE_ADD_UNICODE` steals a borrowed local (gh-131527).
  The tag design relies on that build as a checker, so step 2 fixes both.
- Cherry-picked the attribute-specialisation fix for instances with a
  materialised `__dict__` (plan item 1d). Loads after `vars()` go from 292
  to 35 instructions, stores from 378 to 63; combined-dict stores pay about
  4 more instructions for the split-or-combined choice. Covered by new
  `test_opcache` tests including a free-threaded race test.
- Rewrote the branch history to drop agent attribution trailers, which the
  user's global instructions forbid. Nothing had been pushed.
- Tagged-values step 2 (NaN-box encoding, no producers) is parked on branch
  `boil/tagged-step2-wip`: it failed the instruction gate at 5.8 percent
  geomean (richards, go, nqueens, chaos). Cost breakdown: `PyStackRef_Borrow`
  needs a pointer-space test because an inline float's bit 0 is mantissa
  (+4 instructions on every `LOAD_FAST_BORROW`), guard exits for inline
  values on every guarded input (+3/+2), and the 64-bit mask loads. Time
  measurement on an idle machine is running before the encoding decision.
  The same branch carries two fixes that make the `Py_STACKREF_DEBUG` build
  usable again; those will be split out regardless.
- Clean timing on an idle machine (`boil/bench/step1-delete-tier2.json`,
  `tagged-step2-encoding.json`): the tier 2 and JIT deletion is 0.980
  geomean against baseline (neutral to slightly better). The step 2
  NaN-box encoding with no producers is 1.047 against step 1 (nbody 1.13,
  chaos 1.08, go 1.07, richards 1.03), above the 3 percent line, so it does
  not land as is. Decision: prototype the low-bit int tag with floats boxed
  (`Borrow`, `CLOSE`, `DUP` unchanged; pointer-space test is one `test`
  instruction) and measure it the same way; and proceed with the IR, whose
  static borrow bit removes the `Borrow` cost that dominates the NaN-box
  overhead, before revisiting inline floats.
- The `Py_STACKREF_DEBUG` build works again as a checker: four fixes landed
  (`take_ownership` executable handoff, `_BINARY_OP_INPLACE_ADD_UNICODE`
  close-before-steal from gh-131527, `PyAPI_FUNC` on the debug index
  helpers, and a checker allowance for the one place where a borrowed
  local legitimately outlives its slot's close: `framelocalsproxy_setitem`
  with `f_overwritten_fast_locals`). Targeted tests pass in that build.
- Low-bit int tag prototype (`boil/tagged-lowbit-prototype`): +2.5 percent
  instructions against step 1 versus +5.8 for NaN-box; `Borrow`, `CLOSE`,
  `DUP` unchanged from step 1. Remaining cost is box-in-place address
  taking (spills), guard exits and code growth. Decision recorded in design
  note 02 section 13: Phase 1 uses the low-bit tag with floats boxed; two
  generator fixes (by-value boxing, unlikely branch) run next on that
  branch, then it rebases onto main after the feedback vector lands.
- Review correction: the encoding decision in note 02 section 13 was made
  on a worst-case measurement (dynamic borrows, no producers, no IR). It is
  withdrawn and deferred to IR step 4, where NaN-box and low-bit are
  re-measured with static borrows and the first producers. Order returns to
  the plan's: IR first, tags on top. Both branches are kept as data.
- Direction from review: no prototype branches; develop incrementally on
  main with tests at each step and commit to the well-known technique.
  Final: NaN boxing. Landing order on main: feedback vector (in progress),
  NaN-box encoding with the by-value boxing fixes, inline ints, inline
  floats, then the IR interpreter.

## 2026-10-24

- Overnight both running agents (feedback vector, generator boxing fixes)
  were stopped without reporting. Relaunched the feedback vector from its
  partial diff. An hourly wakeup now checks agent liveness and keeps the
  pipeline moving until Phase 1 is done.
- Phase 1 deliverable, per review: a PR on rjpower/cpython from this branch
  with a detailed description of the new pipeline (mermaid diagrams of the
  flow and object layout, the changes made) and the final results table.
- IR step 2 landed: typed feedback entries replace all 15 inline-cache
  structs. One block per bytecode copy, placed after the code units in the
  same allocation; cache unit 0 holds the distance to the entry so a handler
  reaches it with one load. Seven kinds; branches keep their 16-bit history
  inline because an entry cost 5 instructions per branch. Instruction gate:
  +0.67 percent geomean (richards +1.2, go +0.4, nqueens +0.4, chaos +0.7),
  micro loops neutral. Memory: code objects go from 424 to 875 bytes on
  average because the block is allocated eagerly; RSS +5.6 percent on the
  import workload, +1.4 at startup. Accepted: IR step 4's lazy lowering
  creates the block lazily. Full suite green. Deviations from note 01
  section 3 (BRANCH inline, `misses` and `arg` reserved) to be folded into
  the note at step 4.
- Tagged-values step 2 landed on main: NaN-box `_PyStackRef` encoding
  (section 2.1 of note 02) with no producers. Generator emits by-value
  boxing behind `Py_UNLIKELY` with the cold body out of line, guard exits
  that test the stack slot in memory, and `known_object` tracking so
  close/dup/steal on proven pointers test only bit 0. Overhead against the
  feedback commit: +3.38 percent instructions geomean (richards +2.8,
  go +4.1, nqueens +2.8, chaos +3.9), down from +5.8 for the straight port.
  34 percent of the remainder is the `LOAD_FAST_BORROW` pointer-space test
  (floor for a dynamic borrow; the IR's static borrow bit removes it), the
  rest guard exits that step 4 turns into the type test itself. Accepted on
  record per the review direction. Full suite green on default; debug,
  stackref-debug and free-threaded sets pass. Prototype branches deleted.
- Tagged-values steps 3 and 4 landed: value identity for exact int32 ints
  and non-NaN floats through one choke point (`_PyObject_ValueIdentity`;
  `id()` of such values is their inline encoding, which cannot collide with
  an address), and the first inline-int producers: `LOAD_SMALL_INT_INLINE`,
  `LOAD_CONST_INLINE_INT`, ADD/SUB/MUL_INT with overflow builtins,
  UNARY_NEGATIVE, FOR_ITER_RANGE; tagged-aware COMPARE_OP_INT, TO_BOOL_INT,
  IS_OP, list/tuple/str subscripts and list stores by inline index.
  `PYTHON_TAGGED_VALUES=0` disables producers for bisecting. Micro loops:
  int add 173 to 75 instructions per iteration, range loop 144 to 69, int
  compare 102 to 82; floats unchanged. Benchmarks against the encoding
  commit: chaos -0.7, nbody -1.0, spectral_norm -1.0, richards +1.4, go and
  nqueens +2.1, fannkuch +1.3 percent instructions. Py_STATS box counters
  show where ints still escape: STORE_ATTR_INSTANCE_VALUE, STORE_GLOBAL,
  SET_ADD, slices, and BINARY_OP_EXTEND (`//` in spectral_norm). Full
  suite green with and without the switch; all ports pass with abi id
  `cpython-3.16-refcnt-v0-tagged`; the remote profiler now skips frames
  being torn down. Handler code size grew (ADD_INT 50 to 129 instructions
  of code), to be revisited with the IR.
- Drafted `boil/PR.md` (pipeline, layout and lazy-boxing mermaid diagrams,
  change list, compatibility, gates); results table and float section are
  placeholders until the measurements and the float commit exist.
  History flaw to fix at the final cleanup: the 36 tier-2 file deletions sit
  in the design-notes commit `ac60a9c3c73` instead of the deletion commit,
  a side effect of the soft reset; rewrite when the tree is clean.
- Tagged-values steps 5 and 6 landed: inline floats (`LOAD_CONST_INLINE_FLOAT`,
  ADD/SUB/MUL/TRUEDIV_FLOAT with int-to-double promotion, UNARY_NEGATIVE,
  COMPARE_OP_FLOAT, NaN always boxed) and the wider int set (`//`, `%`,
  `&`, `|`, `^`, `<<`, `>>`, slices with inline bounds on list/tuple/str
  without a slice object, CALL_LEN producing inline, CALL_ISINSTANCE on
  inline args). BINARY_OP_EXTEND loses its int/float and bitwise entries.
  Micro loops: float add 139 to 79, truediv 205 to 83, int `//` 325 to 86,
  int `<<` 484 to 86. Benchmarks against the inline-int commit in
  instructions: spectral_norm -26.4, fannkuch -8.7, nbody -5.7, chaos -4.9,
  nqueens -2.5, float -2.1, go -0.9, richards -0.4 percent. Freelist
  allocations: spectral_norm 4.04M to 0.09M, nbody 5.47M to 2.17M. Heap
  stores still box (nbody's list stores, float's slot stores). Full suite
  green with and without the switch; all ports pass (abi
  `cpython-3.16-refcnt-v0-tagged2`).
- Clean timing of the tagged-values build (`boil/bench/tagged-values.json`)
  against the baseline: 0.989 geomean. spectral_norm 0.647, chaos 0.844,
  generators 0.907, comprehensions 0.900; but nbody 1.027, fannkuch 1.038,
  richards 1.040, go 1.055, nqueens 1.071, deltablue 1.108. Time diverges
  from the instruction counts (nbody was -5.7 percent instructions): the
  eval loop grew from 58 KB to 86 KB and the dynamic borrow test adds a
  branch per local load, so instruction cache and branch behaviour, not
  instruction count, now set the pace on object-heavy code. The IR step is
  where both are addressed (static borrows, smaller handlers).

### IR step 3 landed (2c2c886f091)

The cases generator now emits a second interpreter from `Python/bytecodes.c`:
`Python/generated_ir_cases.c.h` and `Python/ir_targets.h`, with the word
layout in `Include/internal/pycore_ir.h`. The stack interpreter's generated
files are byte-identical. `--with-ir-interp` compiles ceval a second time with
`Py_IR_INTERP` but does not link it yet. 11 new generator tests. Decisions that
depart from design note 01 are recorded in its new section 11: IR_BORROW in
bit 0, 32-bit immediate/jump and feedback fields in word 1, stack pointer kept
through step 4, absorbed loads keep their word. Step 4 (lazy lowering, pcmap,
static borrows) is in progress.

### 2026-10-24 17:30 — final approach for Phase 1

Course correction from the user: agents were wandering for hours. From here every
brief carries a time budget, a smallest-path-first test ladder, a 30-minute
report cadence, and a scope cut to take if the budget runs out. The coordinator
checks each agent at least hourly and kills or redirects it when it misses a
report.

Remaining steps, in order, each one commit with instruction counts in its message:

1. IR step 4, 1:1 lowering and execution behind `--with-ir-interp`. Budget 2h.
   Ladder: one-liner through the IR → test_ir → six targeted test files → full
   suite once → release build instruction counts on micro_loops.py.
2. IR step 5, register form: dead temporaries dropped, true register reads, no
   bytecode shadow for instrumentation. Budget 3h. Same ladder.
3. IR step 6, fused compare-branch and oparg folding. Budget 2h. Gate: fewer
   instructions on micro_loops and on nqueens, go, richards than step 5.
4. IR made the default build; stack-interpreter `--with-ir-interp` flag removed.
   Budget 1h. Gate: full test suite, ports tree `check`, clean pyperformance.
5. Tier 0 baseline JIT: x86-64 assembler, one handler call per IR word with
   inlined dispatch, feedback-guarded int and float fast paths. Budget 6h in
   three commits (assembler + call-threaded code; inline fast paths; tests).
   Ladder: one function JIT-compiled and compared to the interpreter →
   test_ir → full suite → micro_loops → pyperformance.
6. Shapes behind a flag, step one only (hidden class per type, LOAD_ATTR
   instance fast path). Budget 3h. Gate: instruction counts; off by default
   if neutral.
7. Close out: fix ac60a9c3c73 history split, results table, PR.md, open PR.

Steps 5 and 6 are independent and run as parallel agents once step 4 lands.

### IR step 4 landed (92cc151177c); step 5 and shapes launched

`--with-ir-interp` now runs code objects as the register IR with lazy lowering,
a pcmap and static borrows for absorbed loads. Micro-loops at parity with the
tree before the commit (0.93 to 1.02); pyperformance user instructions +2 to
+6% (richards +5.8, go +5.4) because absorbed-load words still dispatch and
borrowed inputs go through an extra indirection. Full suite passes apart from
the machine-local test_sys remote-exec failure; all 7 ports pass.

Launched at 17:50 with 3h budgets and 30-minute reports: IR step 5 (register
form, absorbed words removed; gate is fewer instructions than ref-head on
richards, go, chaos, float) in this tree; shapes step one behind
`--with-shapes` in an isolated worktree. Tier 0 starts once step 5 fixes the
word format.

### Shapes step one done in its worktree (fb39384892c, 52 minutes)

`--with-shapes`: per-type shape table hung off the shared keys, a one-byte
shape index in the PyDictValues padding, LOAD_ATTR_SHAPE / STORE_ATTR_SHAPE /
STORE_ATTR_SHAPE_ADD guarding type version then shape. 22 tests; full suite
passes. Instructions: richards 0.997, go 0.995, deltablue 0.996, raytrace 0.999
versus the same commit without the flag; attribute store 115 -> 111 per
iteration. Neutral in the interpreter as predicted; the payoff is one pointer
compare per access in the JIT. Off by default. Limitations in design note 04:
no shape for objects with a materialised dict, no polymorphic handler, method
loads unchanged, not available with --disable-gil. Cherry-pick onto this tree
after IR step 5 lands, then regenerate the IR files.

### IR step 5 landed (33e727d9745)

Absorbed-load words removed; handler fields read as 16-bit loads from memory
instead of shift-decoded; lowering trimmed. Instructions versus ref-head: go
-1.6%, chaos -0.6%, float -0.9%, richards -0.2% per loop but +0.6% over an
8-loop run from one-time lowering cost (66M vs 13M for quickening). micro_loops
0.93-0.99 on every row. Full suite found a real bug the targeted files missed:
unwinding from a frame pushed by a consumer of an absorbed load looked up the
wrong word, so `try: o.prop except` did not catch; fixed, with no absorption
across exception-range edges, and a regression test. All 7 ports pass.

Queue for the IR agent: FT instrumentation hang fix (45 min), cherry-pick
shapes fb39384892c (30 min), step 6 (2 h: fused compare-branch, oparg folding,
re-lower 1:1 when opcode events are enabled, lowering within 2x of quickening;
gate richards, go, nqueens, float below ref-head including one-time cost).

### Shapes on main (b36c99a7593); step 6 re-cut; tier 0 launched

Shapes cherry-picked with only generated IR files regenerated; its worktree
removed. IR step 6 did not fit 2 hours: lowering costs 44M instructions on a
richards run against 13M for quickening, and the per-loop gain needs the fused
compare-branch, which needs 1:1 re-lowering when branch events are enabled so
coverage keeps its BRANCH events. Re-cut as three commits by 00:00: 6a re-lower
on monitoring (1h), 6b fused COMPARE_OP+POP_JUMP_IF with int/float fast paths
(1.5h), 6c single-pass lowering (1.5h; gate: lowering within 2x of quickening
and richards, nqueens, go, float below ref-head including one-time cost).
Oparg folding dropped from Phase 1. FT IR build: double free when
sys.monitoring CALL events toggle across 10+ threads, deferred with an ASan
build in .worktrees/boil-ft-asan; throw-path instr_ptr fix goes in with 6a.

Tier 0 JIT agent launched 19:45 in an isolated worktree from b36c99a7593, 6h,
three commits: call-threaded code from the tail-call handlers with a
return-to-caller dispatch mode and an own x86-64 emitter behind --with-tier0;
inline fast paths for locals, constants, jumps, feedback-guarded inline int and
float arithmetic and compares, and the shape guard; tests and measurements.
IR word layout frozen for it.

### IR 6a (0c20d49484d) and 6b (d05ff3faf24) landed; tier 0 commit 1 in its worktree

6a: blocks with absorbed loads re-lower one word per instruction when
INSTRUCTION events turn on; frames in the old block move over through IR_REMAP
words; expectedFailure removed. The suspected second FT bug did not exist.

6b: IR_SUPER_COMPARE_OP_{INT,FLOAT,STR} fuse a specialised compare with the
following POP_JUMP_IF, the comparison feeding the jump as a C int, with a deopt
guard on the next handler so BRANCH events survive. Interpreter effect is
small, as expected: richards 562.8M -> 562.2M per loop, nqueens 872.7M ->
869.0M, go 5157M -> 5120M (-l 4). Pairs are under 1% of executed instructions;
the value is that tier 0 can emit cmp+jcc for a fused word.

6c in progress: lowering 34M instructions on richards -l 1 vs 13M for
quickening; acceptance at or below 1.5x with per-loop below ref-head.

Tier 0 commit 1 (ca33452e860 in its worktree): generated_ir_cases.c.h compiled
a second time in "call mode" where dispatch returns the next frame and word;
Python/jit_x86.h emitter; Python/tier0.c emits per word: fields as
immediates, call through the handler table, compare returned word, fall
through or jump table. Specialisation needs no recompile; instrumentation
invalidates. Full suite green on the JIT build. Commit 2, inline fast paths,
in progress.

### IR 6c (118e1e274ab) and IR-only (180a3244ba5) landed

6c: one forward pass for decode, depth, jump marking, handler choice and
feedback slots; static stack-effect table; cheaper emit. Lowering 37M on
richards -l 1 against 12.3M for quickening (3.0x; structural: ~160
instructions per code unit to build word, fields, pcmap and feedback entry,
versus in-place opcode rewrites). Per loop below ref-head: richards 562.2M vs
563.5M, nqueens 870.0M vs 879.6M, go 1190.2M vs 1229.9M, float 764.3M vs
780.3M.

IR-only: --with-ir-interp removed, Py_IR_INTERP unconditional, stack
interpreter and its generator, test interpreter and regen targets deleted
(+196/-36,732). Full suite passes; all 7 ports pass on the release build.
Free-threaded build compiles but does not specialise the IR, so test_dis,
test_opcache and test_thread_local_bytecode fail there (74 failures); FT is
not a Phase 1 target and this is recorded as a limitation. Thread-local
bytecode machinery kept inert for a separate removal.

Pending: clean pyperformance timing of the IR-only build once the machine is
idle (tier 0 suite and ASan run in progress).

### Tier 0 finding: fewer instructions, more cycles

Commit 2 of tier 0 (inline fast paths) cuts instructions 2-15% on
pyperformance and 3-4x on micro loops, but on a quiet machine cycles rise:
richards 241M -> 266M, go 552M -> 623M, nbody 516M -> 479M (the only win).
Code size is not the cause: a shared call stub shrank words from 53 to 28
bytes and made it worse (+5 instructions per call), and L1i misses are 80% in
the C handlers, which exist twice (interpreter and call mode). Threshold
changes only drop startup blocks. Decision: tier 0 ships behind --with-tier0,
off by default, with the honest table; commit 3 adds a cycle diagnosis
(branch misses, iTLB, frontend stalls) and the tier 1 list: register-cached
stack top, inlined LOAD_ATTR/CALL fast paths, one handler copy.

### Regression found: Py_STACKREF_DEBUG build aborts at the IR-only head

Re-verifying the PR's compatibility claim: build/dev-stackrefdebug at
2566fce0b03 aborts during freezing with "StackRef closed with 2 borrowed refs
at frame.c:148, opened at generated_ir_cases.c.h:12040". It passed at
2d3e30a6cab. Suspect: static borrows from absorbed loads are not registered
with the checker or outlive the local's close. Handed to the IR agent as its
own commit, 1 hour, after the ASan run; the PR claim stays unverified until it
passes.

### FT heap corruption left open (7093ffb2235); tier 0 cycle diagnosis

ASan could not see the FT double free because mimalloc owns the blocks; the
debug FT build still reproduces it in 1 of 32 runs, detected at thread exit in
_mi_page_thread_free_collect, far from the first free. Finding it needs
MI_TRACK_ASAN or a debugger (none on this machine). Decision: the free-threaded
build is unsupported for toggling monitoring or tracing while other threads
run; test_free_threading.test_monitoring skips on Py_GIL_DISABLED; design note
01 item 27 holds the numbers.

Tier 0 diagnosis on a quiet machine: (1) calls between compiled frames hit two
store-to-load interlocks because clang merged the {instr_ptr, stackpointer}
stash into one 16-byte store; a volatile store takes a call loop from 128.9 to
117.5 cycles per iteration (base 105.7), worth 1-3% on benchmarks, goes into
commit 3. (2) The remaining gap is front-end: L1i misses 0.32M -> 0.89M on
richards, 1.23M -> 3.23M on go; op-cache misses up 24-35%. (3) Not causes:
branch and return mispredicts are lower in tier 0; _PyTier0_Continue and the
interpreter's handler copy are under 1% of cycles.

Clean pyperformance timing of the IR-only snapshot (fed4b13e878) started 21:46.

### IR-only timing (0fb54f52d09) and stackref-debug fix (d3af3926dbc)

Clean 24-benchmark timing of fed4b13e878: geomean 0.994 vs baseline, 1.006 vs
the tagged-values build. Wins: spectral_norm 0.567 (0.88 vs tagged), chaos
0.794, raytrace 0.939. Losses vs tagged values: unpack_sequence 1.19,
generators 1.11, deepcopy family 1.05-1.08, coroutines 1.04. Time does not
follow the 1-3% per-loop instruction gains; the IR agent has 1 hour to find
where the cycles go (one-time lowering in short worker processes, generator
resume through the IR, handler growth) and take a cheap fix or document it.

Stackref-debug abort was a checker gap: a specialised handler's IR_BORROW_KNOWN
borrow of an absorbed local was tracked, and a failed guard jumped to the
family head without closing it. ir_generator now emits IR_DROP_BORROW before
every deopt or exit for held borrows (51 sites; no-op outside the debug
build). dev-stackrefdebug boots and passes the targeted set again.

### Tier 0 complete in its worktree (ca33452e860, f86efd25c98, 86d85955acc)

Three commits: call-threaded code; inline fast paths for locals, constants,
jumps, bool branches, int32/double add, sub, mul and compare, FOR_ITER_RANGE,
with invalidation by patching and frame switches inside the machine code;
tests (24), design note 05, and a volatile store that breaks a 16-byte stash
copy clang had merged (call loop 128.9 -> 117.5 cycles; interpreter 105.7).

Result, release clang-21 builds, base / tier 0: nbody 1490M / 1269M
instructions but 467M / 482M cycles; richards 646M / 638M, 248M / 264M; go
1432M / 1407M, 532M / 600M. Micro loops: int add 103 -> 29 instructions per
iteration, empty loop 87 -> 36. Fewer instructions, more cycles: L1i misses
rise 2.6-2.8x and op-cache fetches 24-35%, explaining roughly half of go's
gap; indirect and return mispredicts fall; the handler copy and frame switch
are under 1%. Code-size experiments (shared stub, higher threshold,
contiguous arena) did not help. Ships behind --with-tier0, off by default.
Open: test_tier0 leaks under -R 3:3 (stabilises after ~9 runs). Design note 05
lists what tier 1 needs: values in registers across instructions, inline
attribute/global/call paths, one handler copy, state in registers, hot-only
compilation laid out together.

Next: rebase onto head (IR-only deleted tier1_generator.py; IR_DROP_BORROW),
fix or narrow the leak, land, then history fix, results table, PR.

### IR timing diagnosis: yield cost fixed (e55f2ce2586), the rest is layout

YIELD_VALUE found its return point through five dependent loads; the resuming
instruction now records INSTRUCTION_SIZE in a padding byte of the generator
header. pyperf generators: stack/tagged 32.0 ms, IR 35.8 -> 32.8 ms.
unpack_sequence (1.19), spectral_norm and deepcopy differences are eval-loop
code placement: same hot machine code, 13K vs 2.3K cycles per loop with an
empty uop queue, no extra cache or branch misses; -falign-jumps=32 or any
unrelated edit moves single small benchmarks by up to 20%. One-time lowering
is not a factor in pyperf's timings. Lesson recorded in design note 01 item 32:
compare builds by instruction counts or across several layouts.

Tier 0 rebased cleanly onto bb1c8b7bca9; regen check, build and targeted tests
running before the fast-forward.

### Tier 0 landed on the branch (299f9fe2026, bd6347d1fc6, 6023aeccc47)

Rebased twice without conflict; regen stable; dev-t0 rebuilt and 272 targeted
tests pass; the tier 0 worktree and branch are removed. PR.md tier 0 section
now carries the measurements. Remaining: IR agent's final handback, history
fix for ac60a9c3c73, final clean timing of the head, results table, PR.

### 2026-10-24 23:00 — all agents done; close-out

IR agent final handback: 50,733 tests pass on dev-debug at bb1c8b7bca9 (one
machine-local failure); stackref-debug passes its targeted set; all 7 ports
pass on the release build at f0a16213124 with zero new patches; RSS within
+1% to +4% of ref-head (loops 10.9 vs 10.5 MiB). Remaining items it lists for
Phase 2: TLBC removal on FT, the FT monitoring heap corruption, lowering cost
(3x quickening), generator output coverage after dropping the tier-1 output
tests, unexamined logging_simple and xml_etree_iterparse, and a layout-noise
policy for build comparisons.

Decision: the ac60a9c3c73 history split is documented in PR.md rather than
rewritten, because a rewrite would invalidate every later commit id cited in
this log, the design notes and the commit messages.

Final clean timing of f655ef62814 (build/final-snapshot) started 23:00.

### Final timing and results (Phase 1 close)

Final head f655ef62814: geomean 0.991 vs baseline (1.002 vs inline-values
build, 0.996 vs IR-only). spectral_norm 0.593, chaos 0.809, comprehensions
0.845, generators 0.946; nqueens 1.094, fannkuch 1.095, nbody 1.073, float
1.070. Numeric losses versus the earlier tagged-values timing (nbody 1.027)
are within the layout swing documented in design note 01 item 32. Results
table filled in PR.md. Scratch build dirs removed; snapshots used in
boil/bench kept.

## Phase 2 design: architecture v2 (2026-10-25)

Review of PR #1 by the user: the variable-size IR encoding and 16-bit fields
look like avoiding a design rather than making one, and NaN boxing inside a
`_PyStackRef` that still carries ownership bits beside raw `PyObject*` in the
heap is two representations where one should do. Asked for a concise
pipeline description, a redesigned IR and interpreter, a rethought object
layout, and a step plan for Opus 5.5 agents, without deference to how
CPython does things today.

Measurements taken for the redesign (Py_STATS build, Phase 1 head):
data-movement dispatches (`LOAD_FAST*`, `STORE_FAST`, `COPY`, `SWAP`,
`POP_TOP`, `TO_BOOL_BOOL`, `UNARY_NOT`) are 45% of richards' 63.3M
dispatches and 42% of go's 179.2M; mortal increfs and decrefs in richards are
25M over 63M instructions. Over all 99,269 code objects in `Lib/`: largest
frame 803 slots, most constants 4,048, most feedback sites 1,180, longest
jump 5,458 words, largest call 27 arguments. 246 handlers, 21 instrumented.

Design note 06 written. Decisions it proposes: a fixed 64-bit word with a
16-bit handler and four 12-bit fields, emitted by the compiler and stored in
the pyc, with the stack bytecode, lowering, pcmap and absorption deleted;
locals as operands so that the data-movement instructions disappear; one
`PyValue` type everywhere with no ownership bit; reference counting removed
in favour of a non-moving mark-sweep collector over mimalloc with precise
frame roots and conservative C stacks, which also removes the need for a
handle API; eight-byte object header; frames without a stack pointer and
with overlapping argument windows; shapes default on with unboxed slot
kinds. Nine steps with gates and budgets; note 03 (deferred refcounting) is
dropped. Awaiting the user's decisions on section 7 before any agent starts.

### 2026-10-25 — Phase 2 approved; step 1 launched

User approved note 06 with two amendments (conservative scanning is a first
pass, precise moving collector becomes step 10; shape-in-header decided at
6c). Step 1 (fixed-width encoding and handlers on the existing lowering)
launched with one Opus agent, budget 2 days, in this worktree. Tier 0 is
deleted in step 1 rather than carried through; `Python/jit_x86.h` is kept for
step 8. Hourly wake cadence resumed.

### 2026-10-25 — Step 1: encoding compiles; targeted tests pass

The v2 word (16-bit handler, four 12-bit fields, CD as 24 bits) replaces the
Phase 1 format on the existing lowering. dev-debug builds; a hand check of
`def g(o): return o.x + o.y` gives `LOAD_ATTR_INSTANCE_VALUE 1,0,1,0` (dst
slot 1, owner read from local 0, entry 1, name in the entry) and
`BINARY_OP_ADD_INT 1,1,2,3`. Rung 2 (test_ir, test_dis, test_opcache,
test_generated_cases, test_monitoring, test_sys_settrace, test_generators,
test_exceptions) passes, as do test_generators, test_coroutines,
test_asyncgen, test_sys_settrace, test_monitoring and test_contextlib_async
with every FOR_ITER and SEND relaxed (`set_ir_max_forward(0)`). Tier 0 and
the compare-and-branch superinstructions are deleted. Borrowed-local
absorption is kept under the static rule (an input is borrowed when its field
differs from its canonical slot), so it was not turned off.

### 2026-10-25 — Step 1: full suite passes

Full suite on dev-debug and dev-default: only the known
test_remote_exec_deleted_static_executable fails (test_sys's frame size
check now counts `ir_words` and `ir_feedback`). Rung 2 passes on
dev-stackrefdebug. dev-ft builds; its rung 2 fails only in the known places
(test_dis and test_opcache, 72 failures, and test_monitoring's
test_no_unwind_for_shim_frame).

### 2026-10-25 — Step 1: measurements

Instructions per loop (worker at 2 and 10 loops, differenced) against Phase 1
(f655ef62814). On gcc with computed goto, the register decode of section 2.1
(v0) ran 8 to 14 percent more instructions: richards +10.0, go +14.0, nbody
+11.3, float +8.1. GCC had put `next_instr` in a stack slot, which cost a
load and a store in every handler and a reload at dispatch. Handlers now read
their fields from the word in memory at `this_instr`, and debug builds check
each read against `ir_word`. That leaves richards +2.4, go +4.6, nbody +1.7
and float +2.1 percent; LOAD_ATTR_INSTANCE_VALUE runs about 8 more
instructions, which the layout accounts for (note 06 section 9). The
clang-21 tail-call build, which still decodes `ir_word`, runs fewer
instructions than Phase 1 built the same way on all four: richards -1.8, go
-2.5, nbody -10.3, float -3.8 percent. Accepted as the step 1 result, with
go over the 3 percent allowance for the computed-goto fallback.

Two other changes did not explain or close the gap and are not kept:

- With the compare-and-branch superinstructions disabled, Phase 1's go
  rises by only 0.8 percent and the other three by under 0.2 percent.
- A 32-byte feedback stride was 0.3 percent worse on go.

The full suite on dev-debug and dev-default fails only in the known
test_remote_exec_deleted_static_executable. Rung 2 passes on
dev-stackrefdebug and dev-tailcall. dev-ft fails
only in the known places. All seven ports pass against dev-default.
