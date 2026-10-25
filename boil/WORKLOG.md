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
