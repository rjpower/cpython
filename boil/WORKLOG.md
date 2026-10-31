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

### 2026-10-25 — Step 2a, day 1: the compiler emits the words

The assembler now emits one IR word per instruction (Python/regalloc.c maps
depth d to slot co_nlocalsplus + d, and Python/assemble.c encodes the words,
numbers the feedback entries and relaxes FOR_ITER and SEND). co_code holds
the words, followed in the code object by a feedback table of one 32-bit
entry per site, the kind and the static operand. The first call copies the
words into the executing copy with its feedback vector
(`_PyCode_GetExec()`, Python/ir.c, which replaces Python/ir_lower.c). Sites
past the 4095th entry run NOFB handlers that carry the operand in the word.
Line, exception and monitoring offsets are word indices. The pyc magic is
3707.

`def g(o): return o.x + o.y` compiles to seven words on dev-debug, which
match note 06 section 2.2 before folding: RESUME, LOAD_FAST_BORROW r1 r0,
LOAD_ATTR r1 r1 fb1, LOAD_FAST_BORROW r2 r0, LOAD_ATTR r2 r2 fb2,
BINARY_OP_ADD r1 r1 r2 fb3, RETURN_VALUE r1, with the names in the feedback
table. After 100 calls the executing copy runs RESUME_CHECK,
LOAD_ATTR_INSTANCE_VALUE twice and BINARY_OP_ADD_INT. test_grammar and
test_builtin pass; test_generators and test_exceptions do not yet.

### 2026-10-25 — Step 2a, days 2 and 3: the tests read words

test_ir, test_dis, test_opcache, test_generated_cases, test_monitoring,
test_sys_settrace, test_generators, test_exceptions, test_code, test_compile
and test__opcode expect the words. Changes the tests forced:

- The IR-only handlers are numbered after the pseudo-instructions, so that
  `opcode.opname` names one thing per number (`IR_JUMP` is 267).
- `IR_JUMP`, the trampoline word of a relaxed FOR_ITER or SEND, keeps the
  instruction pointer at the loop, so BRANCH events name the loop.
- The assembler emits the inline constant handlers in every build; the
  executing copy runs the boxing forms where values stay boxed.
- The compiler builds annotation dicts 15 pairs at a time, so that
  test_stack_3050_2 stays within the slot limit.
- `test.support.bytecode_helper.encode_words()` builds words for tests that
  hand-assemble code. test_dis has 145 tests and 0 failures on dev-debug.
  On dev-ft it has 8, because the free-threaded build does not specialise
  those sites; the MONO check of show_caches is skipped there.
- The gdb reader takes `f_lasti` from `instr_ptr - ir_words`. gdb is not
  installed here, so test_gdb was skipped and the file is only compiled.

### 2026-10-25 — Step 2a, day 4: suites and ports

The full suite on dev-debug and dev-default fails only in the known
test_remote_exec_deleted_static_executable. Three failures surfaced on the
way:

- test_import's test_foreign_code replaces a constant with a code object.
  The word kept `LOAD_CONST_INLINE_INT` and aborted. The code constructor now
  demotes such a word to `LOAD_CONST`.
- test_embed's test_frozenmain needed Programs/test_frozenmain.h
  regenerated.
- numpy and pyyaml failed to import: Cython passes zeroed code in 2-byte
  units, 4-byte aligned, for each function's code object. The constructor
  pads it to whole words of CACHE. All seven ports then pass against
  dev-default (`--interpreter build/dev-default/python`; the default
  baseline-default interpreter cannot run the current Lib).

Rung 2 passes on dev-tailcall. On dev-stackrefdebug it passes too;
test_embed's test_asyncio_task_during_interpreter_clear reports leaked
stackrefs there, but it does so at the step 1 head as well. dev-ft fails only
in the known places: test_dis and test_opcache (72) and test_monitoring's
test_no_unwind_for_shim_frame.

### 2026-10-25 — Step 2a, day 5: measurements

Recorded in note 06 section 10. Per loop, 2a is within 1 percent of
step1-noabsorb on all four (richards +0.4, go +0.5, nbody -0.2, float +0.4
percent). Against step 1 it is richards +3.4, go +5.5, nbody -0.2 and float
+3.0 percent: the absorption 2a does not do, left to 2b. Removing the lowering
saves about 40M instructions on a richards `-l 1` worker and 6.8M at startup.
Peak RSS is +1.2 percent at startup, but +4.8 percent after twelve stdlib
imports and +2.9 percent on richards `-l 10`: code objects hold 8-byte words
where they held 2-byte units. The step 1 builds are VPATH builds of this
tree, so they ran the current Lib; none of the workloads imports dis or
opcode, the only modules that differ.

### 2026-10-25 11:50 — step 2a landed; coordinator gate check

Spot check on dev-default at 3abc1ca9651: test_ir, test_dis, test_compile,
test_monitoring pass; richards per loop 1548M against 1489M for the step 1
snapshot (+4.0%, the absorption left to 2b; the agent measured +3.4%). The
ports SDK interpreter now names build/dev-default, since baseline-default is
a VPATH build of this tree and can no longer create a venv against the old
Lib. Step 2b (folding) launches now.

### 2026-10-25 — Step 2b, day 1: loads fold into their consumers

Item 1 landed (e06e93ea0db, 9fe2885ef6e). A LOAD_FAST_BORROW keeps no word
when the next word reads it through an input field, on the same line and in
the same block (option A, confirmed by the coordinator). Handlers that may
read a local this way resync stack_pointer from dst, from a new TOP field,
or by counting local fields. The LOAD_FAST and STORE_FAST superinstructions
are no longer built. `def g(o): return o.x + o.y` is four words after RESUME,
as note 06 section 2.2 shows; the numeric loop is nine words per iteration,
six once STORE_FAST folds (item 2) and the pending local under a call is
handled (step 3).

Rung 2 and the full dev-debug suite pass, but for the known
test_remote_exec_deleted_static_executable. Fixes on the way: an error that
unwinds out of a frame resumes the caller one word later, since the calling
word may now start a try block; the frame-state simulation in
frameobject.c pushes the folded inputs; Programs/test_frozenmain.h is
regenerated (magic 3708).

Py_STATS dispatches, `--worker -l 2`:

| benchmark | 2a | item 1 | change | LOAD_FAST* 2a | LOAD_FAST* item 1 |
|---|---|---|---|---|---|
| richards | 83.50M | 67.74M | -18.9% | 19.80M (23.7%) | 4.03M (6.0%) |
| go | 238.06M | 197.69M | -17.0% | 62.19M (26.1%) | 21.55M (10.9%) |

The loads left are those option A does not fold: under RETURN_VALUE, a
window format (31% of richards' remaining, 19% of go's), under a call or a
global load, under COPY (item 3) and under TO_BOOL (item 4).

### 2026-10-25 13:30 — step 2b: folding regresses instructions; operand-kind variants

Items 1 and 2 (load and store folding) are committed and correct: dispatches
on richards -23%, go -24% against 2a. But per-loop instructions rose 18 to
50%: every register input now pays a field decode, canonical-slot
arithmetic, a borrowed-or-consumed test and borrow tagging at run time, plus
a stack-pointer sync on entry and a dst-is-local test, and ceval's spills
nearly tripled. All of it is the stack pointer (step 3) and ownership
(step 5) showing through. Decision: encode the operand-kind pattern (input
k is a local, dst is a local) in spare bits of the handler index and
generate a variant per pattern for the hot families, Lua 5.4's ADDK/ADDI
technique, so a folded word runs code with no runtime tests. RESUME removal
(item 5) moves to step 3. Deadline for 2b moves to 2026-10-29 12:00.

### 2026-10-25 — Step 2b, end: truth tests fold into jumps; the variants go

The operand-kind variants were removed, as decided at the policy change
(note 06 section 6), and items 3 and 4 landed. These are the commits:

- 4ea1b59d7bf: the cases generator puts the specialised opcodes that do
  not fit their range into free numbers below RESUME.
- d755da1b3e9: a specialising head drops its field-input borrows before it
  runs the word again. The Py_STACKREF_DEBUG build hit a fatal error at
  frame clear without it.
- 82b2b301236: the conditional jumps test the truth of any value. They
  specialise as TO_BOOL does, the flowgraph drops TO_BOOL before them and
  inverts them for UNARY_NOT, and `COPY 1` of a just-loaded local becomes
  a second load. The pyc magic is now 3711.

Dispatches at `-l 2` are down 29.8% on richards and 29.0% on go against
2a. The plan's 35% target is not met; what remains is listed for step 3
in note 06 section 11.

In the gcc computed-goto build, items 3 and 4 add 3 to 5% instructions
per loop. In the clang tail-call build they save up to 1.2%. GCC spills
the interpreter state across the eval loop, and the larger handlers made
that worse: %rbp references went from 7448 to 10176. Section 11 has the
tables, the attribution to steps 3 and 5, and the experiment with the
operand machinery stubbed out.

Tests: the full dev-debug suite passes but for the known test_sys failure.
Rung 2 passes on dev-default and dev-tailcall. dev-stackrefdebug fails one
test_embed case, which leaks a never-run coroutine's frame references.
dev-ft has 78 failures, which is the 73 known before plus the new
specialisation subtests. All seven ports pass `check`.

One measurement pitfall: Python/import.o is not rebuilt when
pycore_magic_number.h changes. An incremental build then keeps the old
magic and loads stale pycs, which invalidated one full-suite run. Builds
that share `Lib/__pycache__` but have different magics also overwrite each
other's pycs, which corrupts per-loop numbers unless each build gets its
own PYTHONPYCACHEPREFIX.

### 2026-10-25 15:00 — step 2b landed; step 3 split; 3a launched

Coordinator gate check at 23fd882f9bd: targeted tests pass on dev-default,
all 7 ports pass. Step 3 is split: 3a frames and calls (static frame, dead
temporaries NULL, no stack pointer, frame header and value stacks with
overlapping argument windows, RESUME into frame entry), 3b the register CFG
and passes of note 07. 3a launched, budget 4 days.

### 2026-10-25 16:20 — step 3a, milestone 1: no stack pointer, dead temporaries NULL

Items 1 and 2 of step 3a landed (d867022133c, b59b5ea5676). Every frame
slot holds a value or NULL; handlers keep the slots in a register and
have no stack pointer. The cases generator models the slots in IRStack
(Tools/cases_generator/ir_generator.py), behind address/load/store hooks
in stack.py; the TOP role, the SYNC modes, canonical-slot arithmetic,
IR_CLEAR_IF_BORROWED, IR_STORE_RESULT and the pop_N_error labels are gone.
The frame keeps `return_slot`, the slot that receives a callee's result or
a generator's sent value. The exception table names the exception's slot;
the pyc magic is 3712.

Two decisions differ from the brief's wording and go into note 06
section 12:

- The unwinder releases temporaries from the handler's first slot up, not
  all of them: a handler runs with live temporaries below it, such as the
  iterator of an enclosing for loop or the with-block's exit pair.
- LOAD_SPECIAL became window style. Its output pair overlaid its input,
  which a fold of `COPY 1` could make a local, and the outputs then
  overwrote the local. `with` blocks lose that one fold.

The generator found one bug it could not see: a window handler cleared an
array input's slots before the statement that closed an element
(BUILD_INTERPOLATION's format spec). The element now moves to a C variable
first.

Tests: rungs 1 and 2 pass on dev-debug, dev-default, dev-tailcall and
dev-stackrefdebug; dev-ft has the known 78 failures. The full dev-debug
suite passes but for test_remote_exec_deleted_static_executable. The full
dev-stackrefdebug suite fails 15 files. A build of 1b71d70deb4 with the
same flags fails the same 15, and where counted (test_faulthandler,
test_threading, test_interpreters, test_embed) the same number of tests:
stackref leak reports at exit, and faulthandler dumps that record a
borrow without the GIL.

### 2026-10-25 17:20 — step 3a, milestone 2: frame and value stacks, overlapping calls

Item 3 landed (93055610963 to 13bd5fe37d5). A frame's header goes on the
frame stack and its slots on the value stack, at `frame->base`. Both
stacks live in the data-stack chunks and grow towards each other, so one
bounds check covers both. A Python-to-Python call by `CALL_PY_EXACT_ARGS`
or `CALL_BOUND_METHOD_EXACT_ARGS` overlaps the callee's frame with the
argument window; the keyword, default and vectorcall paths and generator
callers copy. A return or a yield resumes the caller at the word after
its call, and `return_offset` and `gi_yield_offset` are gone. Generators
keep their slots in the generator object.

On the call benchmark, the split cost 27 instructions per call. The
overlap won back 33, and clearing only the locals of a thread frame won
back 21; the frame's temporaries are already NULL at that point, which
debug builds check. That was the coordinator's suggested fix for
milestone 1's 40-instruction rise. The overlap adds a constraint for 3b:
temporaries live across a call must sit below its window, or the callee
must copy. Note 06 section 12.2 records it.

### 2026-10-25 17:45 — step 3a, milestone 3: RESUME, -fno-gcse, tail-call signature

Items 4 and 5 landed (5119930d271, 4ed1d4ab1d1, 8cde9e61f7c), and
InternalDocs/frames.md was rewritten (f4983b9da5e).

- **RESUME.** Frame entry, inline calls and generator resumption skip a
  `RESUME` that would do nothing. RESUME_CHECK is deleted, with magic
  3713. The word stays: it still runs when the eval breaker is set,
  after `MAKE_CELL` and `COPY_FREE_VARS`, and at a generator's first
  start, about 6.8K dispatches per benchmark run.
- **GCC.** `-fno-gcse` is set through `CFLAGS_CEVAL` for gcc with computed
  gotos. configure was edited by hand to match configure.ac, not
  regenerated.
- **Tail calls.** The handlers take the slots, the word pointer and the
  thread state, and read the frame from `tstate->current_frame`. That
  costs 0.3 to 0.5 percent of instructions in the clang build.

### 2026-10-25 18:45 — step 3a done: suites, ports, measurements

At f4983b9da5e:

- dev-debug and dev-default pass their full suites, except for
  `test_remote_exec_deleted_static_executable`.
  test_external_inspection's `test_cache_per_thread_isolation` failed
  once in the dev-debug run, under load; alone it passed three times.
- dev-stackrefdebug fails the baseline's 15 files.
- dev-ft rung 2 has the known 78 failures.
- dev-tailcall rung 2 passes.
- All seven ports pass.

Against 2b, per-loop instructions fall by 2 to 8 percent in the gcc
build. In the clang tail-call build they move between -3.3 and +1.4
percent. gcc is still 22 to 47 percent above step 1. Dispatches fall to
14.57M on richards and 41.35M on go, 33 and 32 percent below 2a.
`f(a, b)` costs 438 instructions per call against 453 in 2b and 353 in
step 1. The loop's `%rbp` references fall from 10176 to 3736. Stubbing
the run-time local test brings them to 2021, so step 5 should remove most
of the rest. Peak RSS is 0.3 to 1.0 MB above 2b, and the cause has not
been investigated. Note 06 section 12.4 has the tables.

### 2026-10-25 18:10 — step 3a landed; 3b and 4 launched in parallel

Coordinator gate check at 03a9bccb5f9: targeted tests pass on dev-default,
all 7 ports pass. Step 3a took one day of four. Step 3b (register CFG and
passes, note 07) launches in this worktree; step 4 (collector in verifier
mode) launches in worktree .worktrees/boil-step4 on branch weaver/boil-step4,
to be rebased and merged here when it lands. SSH agent has no identities;
pushes go over HTTPS with the gh token.

### 2026-10-25 19:30 — step 3b, milestone 1: the register CFG, no new passes

084e8468ed5 replaces Python/regalloc.c with Python/regcfg.c (conversion,
liveness, compaction, verifier, driver) and Python/regcfg_passes.c (the
passes). The conversion runs after `optimize_load_fast`, so the borrow
decisions are in place. Load and store folding are still 2b's two
peepholes.

- **Equivalence.** I compiled every code object in Lib (99,294) with both
  compilers. 71 differ, all of them in unreachable exception handlers.
  These handlers are left behind when the NOP that named them is removed
  (`try: pass`, `assert True`). 3a gave them an arbitrary depth; 3b gives
  them the depth the handler implies. Two test_dis expectations change.
- **Slots are held until taken.** A temporary keeps its slot until an op
  consumes its value, not just until its last read. A loop that never
  exits keeps the values below it: the iterator in
  `locale._grouping_intervals`, and the exception in a `finally` that
  never ends. Freeing by liveness would overwrite them. The verifier no
  longer treats an unread value as an error.
- **Tests.** dev-debug passes rung 2 and the full suite, except for
  `test_remote_exec_deleted_static_executable`.
- **Pitfall.** Without its own PYTHONPYCACHEPREFIX, the step3a-snapshot
  build loads the new build's pyc files, and test_dis appears to fail.

### 2026-10-25 20:40 — step 3b, milestones 2 and 3: passes a to f

The passes in Python/regcfg_passes.c run in this order: SWAP renaming,
copy propagation, store elimination, POP_TOP removal and truth folding
(f6eb57ce376 to f46ed901d93). Compaction follows. The magic number is
3715.

- **Copy propagation (a).** A LOAD_FAST_BORROW folds into its one reader
  if the reader is on the same line and nothing in between writes the
  local. The reader may be under a constant, a global or a call. Only
  borrowed loads fold, because a register-style handler reads a borrowed
  field without taking a reference. A STORE_FAST from a folded LOAD_FAST
  would store a borrowed reference.
- **Store elimination (b).** A STORE_FAST becomes the dst of the op that
  made its value. Nothing in between may raise, escape, jump, write a
  local or read the value. The producer must have the STORE_DST format,
  and CALL does not, so `x = f()` keeps its STORE_FAST.
- **COPY, SWAP and POP_TOP (c).** The flowgraph turns a COPY of a local
  loaded earlier in the block into a second load, so that the borrow
  analysis sees it. A SWAP whose outputs one reader pops is renamed away.
  A POP_TOP goes with its producer if the producer is pure and has only
  local inputs. A POP_TOP of a folded local goes on its own.
- **Truth folding (d).** COPY, POP_JUMP_IF_* and POP_TOP become
  JUMP_IF_FALSE_OR_POP or JUMP_IF_TRUE_OR_POP, as does the same pattern
  after a LOAD_FAST. The new instructions keep the value where they jump
  and release it where they fall through. They have no feedback entry,
  because the fields fill the word. UNARY_NOT now tests the truth of any
  value itself, so TO_BOOL remains only for `not not x`.
- **Liveness and compaction (e, f).** `co_stacksize` is now the number
  of temporary slots that compaction assigns, not the deepest stack.
  After compaction the verifier uses liveness to check that no two live
  temporaries share a slot. Over Lib (99,328 code objects) the
  temporaries total falls from 476,883 to 473,896 (0.6%). The frame-slot
  maximum stays at 803: it is a function with 800 locals. The frame does
  not shrink because call and BUILD_* windows need their operands
  consecutive at the top, which is what the stack depth measures.
  Disabling the placement rule and the folded-load reservation as an
  experiment gained only 0.4% more.
- **Py_STATS against 3a.** richards falls from 14.60M to 12.99M
  dispatches (-11.0%) and go from 41.35M to 38.28M (-7.4%).
  - richards: COPY 0.704M to 0.069M, POP_TOP 0.579M to 0.125M,
    TO_BOOL_BOOL 0.249M to 0, LOAD_FAST_BORROW 0.711M to 0.555M.
  - go: LOAD_FAST_BORROW 3.489M to 1.483M, SWAP 0.753M to 0.022M.
  - The loads that remain are mostly window operands: of the
    LOAD_FAST_BORROWs on go, 1.0M are followed by CALL_PY_EXACT_ARGS.
  - The largest remaining item: STORE_FAST or POP_TOP after a
    RETURN_VALUE, i.e. the result of a Python call stored or dropped. It
    runs 1.5M times on go (3.9%) and 0.31M on richards. A CALL that
    returns into a local would remove it.
- **Tests.** dev-debug passes rung 2 and the full suite, except for
  test_sys (known).
- **Pitfalls.**
  - The opcode renumbering needs Programs/test_frozenmain.h regenerated.
    `make regen-test-frozenmain` writes it into the build directory, so
    it must be copied back to the source tree.
  - Without a magic bump, cached pycs keep the old compiler's output.
    Every run now uses a fresh PYTHONPYCACHEPREFIX.

### 2026-10-25 21:35 — step 3b, milestone 4: call results into locals, measurements, docs

- **A call stores its result in a local** (691c92537c6, magic 3716). Store
  elimination now folds the STORE_FAST after a call:
  - the CALL, CALL_KW and CALL_FUNCTION_EX families gain STORE_DST;
  - a member that pushes a frame names the local as `return_slot`;
  - `_PyFrame_SetReturnValue` releases the local's old value;
  - generator frames still leave to the head when dst is a local,
    because a generator hands that slot every value it yields and,
    at the end of a FOR_ITER, its return value.

  STORE_FAST on go falls from 1.51M to 0.36M dispatches.
- **Against 3a:**
  - dispatches: richards -12.5% (12.77M), go -10.2% (37.12M);
  - gcc instructions per loop: richards -3.6%, go -3.4%, nbody -6.8%,
    float -1.9%;
  - clang instructions per loop: -2.8%, -3.2%, -6.5%, -1.6%;
  - `-c pass`: unchanged;
  - compiling Lib: +16%.
- **Pass g is cut.** No private memory is retained: heap and anonymous
  RSS are 92 KiB below 3a. Peak RSS is 0.5 to 1.1 MB above 3a, and all
  of it is in the executable's file-backed pages.
- **Tests at 691c92537c6:**
  - dev-debug and dev-default, full suite: only test_sys fails;
  - dev-stackrefdebug, full suite: the same 15 files as before;
  - dev-tailcall, rung 2: passes;
  - dev-ft, rung 2: the known 78, plus test_sys;
  - ports: 7/7 pass.
- **Docs.** Note 06 section 13 and note 07 section 5.
- **Pitfall.** A magic bump does not rebuild Python/import.o. Delete it
  in every build directory, or `importlib.util.MAGIC_NUMBER` keeps the
  old value.

### 2026-10-25 21:45 — step 3b landed; coordinator gate check

Targeted tests pass on dev-default at 0ba2538907c; all 7 ports pass. The
agent's measurement scripts (per-loop differencing, Py_STATS mix, slot
distribution, compile-time) move from /tmp into boil/tools/measure/ so the
next steps reuse them. Step 5 waits on step 4 (verifier), still running in
its worktree.

### 2026-10-25 18:30 — step 4, milestone 1: mimalloc default, precise roots, first hole count

- **Allocator.** mimalloc is now the default object allocator in the GIL
  build (ec3a296179d). Each thread state gets the free-threaded build's
  object, GC, GC-pre and mem heaps, and exited threads abandon their
  segments to a per-interpreter subprocess. The verifier can therefore
  enumerate every object block.
  - test_cmd_line now expects `mimalloc` and `mimalloc_debug`.
  - test_embed's leak check counts only the thread-state heaps.
  - `python -c pass` costs 47.15M instructions, against 47.45M on
    base-default.
  - Peak RSS rises by 0.6 to 1.6 MB.
- **Mark phase.** Python/gc_trace.c (7819b9342dc).
  - Mark state lives in side tables (address sets) and the mark stack is
    iterative.
  - Every tp_traverse is followed.
  - Precise roots: thread frames, thread-state and interpreter fields,
    the GC lists, static types (registered at PyType_Ready) and immortal
    heap objects.
- **First counts.** `-c "import gc; gc.collect()"` on dev-debug showed
  3292 holes with all roots and 5009 with precise roots only. Almost all
  of them were strings and constants that the default build's code,
  dict-key and type traverse functions skip, because they cannot form
  cycles.

### 2026-10-25 19:00 — step 4, milestone 2: conservative roots, zero holes on -c and test_grammar

- **Conservative roots.**
  - C stacks are scanned, along with callee-saved registers. Inline asm
    captures the registers, because glibc's setjmp mangles rbp, rsp and
    pc. They are captured first thing in PyEval_SaveThread,
    PyEval_ReleaseThread and the detach path.
  - Writable data segments are scanned through dl_iterate_phdr.
  - Interior pointers mark their block.
  - An untracked GC object reached only conservatively is marked without
    being traversed.
- **Traverse fixes.**
  - Code objects, dict keys, type names and slots, timezone, descriptor
    names, and the PyArena's object list (5a210627510, c803fb1b4cd,
    a45c4732fe9).
  - The arena list is the first user of `_PyGCRoot`: a registered root
    for references in non-object memory.
- **Hole counts.**
  - `-c "import gc; gc.collect()"`: 0 with all roots, 146 with precise
    roots only (mostly objects that only C statics reach).
  - test_grammar: 0.

### 2026-10-25 21:00 — step 4, milestone 3 (in progress): audit, core fixes, test file, rung 2

- **Audit.** Tools/boil/traverse_audit.py (2596feb2368, e00fe928264)
  parses the struct definitions, the static type initializers and the
  PyType_Spec slots. It started with 31 types that hold object references
  and have no tp_traverse, plus 7 traverse functions that skip members
  their dealloc releases. Now 10 remain: test and example types, Tcl_Obj
  and ssl.SSLError. SSLError inherits OSError's traverse at run time,
  which the parser cannot see. Section 14 of note 06 lists them.
- **The verifier's own fixes.**
  - Garbage is now separated from holes. Cyclic garbage created by
    finalizers or by another thread during a collection is unreachable,
    so it is counted but not reported. Members still held by an object
    being deallocated, and free-list objects, are handled the same way.
  - New roots: the trashcan's deferred chain and the thread data stacks.
    A returning frame is unlinked from `current_frame` before its slots
    are cleared.
  - Verification is skipped during finalization.
- **Real leak found.** importdl leaked the module on the non-ASCII
  single-phase error path; fixed in 51c8f326e36.
- **Tests.** Lib/test/test_gc_verify.py has 20 tests and passes on
  dev-debug and dev-default.
- **Rung 2.** All the rung-2 tests pass with zero holes, except
  test_sys's known `test_remote_exec_deleted_static_executable`.
- **First full-suite attempt.** It was spoiled: I rebuilt dev-debug while
  it ran. It did find two more sources of holes: the _hmac hash-name
  table, now fixed, and a transient C-held dict. The full suites now run
  on a separate build/run-debug.

### 2026-10-25 20:50 — step 4, milestone 3: the full debug suite is clean

- **Full dev-debug suite with the verifier.** The only failure is test_sys's
  known `test_remote_exec_deleted_static_executable`. Four runs found these
  sources of holes, and each is fixed:
  - **In the verifier:**
    - PyMem blocks that a C stack points into, such as the JSON encoder's
      PyUnicodeWriter, are now scanned (4fb07737fc3);
    - new precise roots: atexit, the errno map, the cross-interpreter state
      and the cached objects (3f258c5b81e).
  - **Missing traverse functions:**
    - coroutine `cr_origin` and ZoneInfo (5e5ca23c511);
    - symbol-table entries, and datetime and time `tzinfo` (51205a6593e).
- **The audit had a bug.** It took an include guard's `#ifndef` as false,
  which hid every struct that a header defines; the symbol-table entry and
  datetime structs were among them. With the fix (0d7b76c1f41) it also
  found module `md_name` and the multibyte encoder's `pending`.
- **The hole log** records `sys.argv`, which names the test file a
  regrtest worker runs. That is how the last two files were found.
- **Startup cost.** In `-c pass`, the new visits cost about 2.0M
  instructions in the three startup young collections before
  `_Py_VISIT_ATOMIC`, and about 1.4M with it (e8e2a43a3d3), which skips
  immortal strings: the interned and static ones.

### 2026-10-25 22:40 — step 4, milestone 4: AUTO mode, ports, measurements

- **AUTO mode** verifies after every collection of generation 1 and up,
  at arbitrary points, so it found holes that explicit collections
  cannot reach. Fixed:
  - the blocks of shallow-marked objects are scanned conservatively,
    for sets being filled (a7a92ab8d3a);
  - scandir's `path_t` (0e5e79ce2bb);
  - lru_cache's links (d89d6b5438f);
  - the unpickler's stack (43a37af34db).
- **The audit** now follows embedded plain structs and flags the generic
  type-only traverse.
- **The report** lists the holes with unexplained references first, and
  names the object that owns a referring PyMem block.
- **Final suites.** These are clean apart from the known test_sys
  failure:
  - dev-debug, verifier on;
  - dev-default with `-X gcverify`;
  - dev-default with `PYTHON_GCVERIFY=auto`, except for one compile in a
    pip subprocess. Its cause is the compiler's capsule-held enclosing
    units, which are out of scope; see section 14.5.
- **Ports.** All seven pass. With the verifier, five are clean. pyyaml has
  14 holes, from Cython's PyMem code cache behind a static. numpy has
  about 6,100, from DTypeMeta's `dt_slots` and its libc-malloc tables.
  By decision these wait for step 5's public root API (option B). The
  brief's `ports check` command never enabled the verifier, because it
  drops PYTHON* variables and runs with `-I`; the smoke tests were run
  directly instead.
- **Measurements** (section 14.6):
  - per-loop instructions are within 0.5 percent on all four
    benchmarks;
  - `-c pass` is +2.3 percent;
  - peak RSS at startup is +1.3 MB, from mimalloc and the per-tstate
    heap split;
  - a verified full collection of 1M objects takes about 10 times as
    long (415 ms against 41 ms).

### 2026-10-25 23:00 — step 4 merged; step 5a launched

weaver/boil-step4 rebased onto this branch (two append-only conflicts in
this file and note 06, both kept) and fast-forward merged at 4e9d25010a7.
Merged tree: debug and release builds, zero holes on a bare interpreter,
targeted tests pass on both builds (verifier on), all 7 ports pass. Step 4
worktree and branch removed. Step 5 is split: 5a collector live behind
--with-gc; 5b full suite and ports on it; 5c default on and refcount code
deleted; the 8-byte header and weakref side table move to 6a with the rest
of the object layout. 5a launched, budget 4 days.

### 2026-10-25 23:45 — step 5a milestone 1: the collector is live behind --with-gc

`configure --with-gc` defines Py_GC. Under it nothing is reference
counted and Python/gc_live.c owns memory: an allocation that passes the
byte threshold marks (step 4's roots, with the mark bits in the header),
flags unmarked objects dead, handles weak references and finalizers, and
sweeps in two phases. A dealloc runs while the dead blocks are still
allocated; the blocks are freed afterwards. build/gc-debug (`--with-pydebug
--with-gc`) builds; `-c pass` runs; `gc.collect()` marks and sweeps (4,967
objects and 2 MB freed at startup, 5 ms). Ten million short-lived tuples
run in 111 MB peak RSS with 38 collections, and the verifier
(`-X gcverify=auto`) finds no dangling reference after each one.
`__del__` with a weakref callback, resurrection (finalized once), a
finalizer cycle, a suspended generator's `finally`, `__slots__` with a
dict subclass, and a thread allocating while the main thread collects
all work. dev-debug still builds, and its tests pass apart from the
known test_sys remote-exec failure.

Found on the way:
- `_PyLong_Negate` assumed that a shared fresh int is a small one;
- `PySet_Add` on a new frozenset checked the count;
- deallocs checked buffer exports, which the sweep order breaks;
- `type_dealloc` allocated a key to remove itself from its bases'
  subclass dicts, which were perhaps already swept.

Next: the day-2 tests. Their first run shows refcount-timing and
`is_tracked` failures to adjust, and an overflow in a dead bytes object
to find.

### 2026-10-26 01:50 — step 5a milestone 2: the day-2 tests pass on both gc builds

The ten day-2 test files (test_gc_verify, test_grammar, test_builtin,
test_dict, test_list, test_set, test_tuple, test_weakref,
test_finalization, test_gc) pass at -j8 on build/gc-debug and
build/gc-default (1,391 tests, 62 skipped with a Py_GC reason).
TracingCollectorTests covers the byte trigger, `gc.disable()`, the extra
`get_stats()` keys, non-cycle garbage, finalizer order, weak reference
callbacks and `PyGC_AddRoot()`. gc.rst and gcsupport.rst document them.

Fixed on the way:
- `PyBytesWriter` lost its debug canary across a copying resize;
- a popped compiler unit capsule traversed its freed unit (SAVEALL
  segfault);
- finalizers ran in the wrong order (a FileIO closed before its
  TextIOWrapper flushed); they now run referrer first. Legacy `tp_del`
  ran in the sweep; it is queued, or goes to `gc.garbage` in a dead cycle.
  An explicit collection frees, in a second pass, what its finalizers did
  not resurrect. Shutdown collects once more after the modules are
  cleared, so `__del__` output at exit appears;
- `_testcapi`'s static object was write-only under Py_GC, so -O3
  removed it.

False retention was the largest source of failures: stale words in live
C frames kept `del x; gc.collect()` garbage alive. In order of effect:
scan the collecting thread from the collection's entry; skip freed
mimalloc blocks; zero the builtin call scratch array;
`-ftrivial-auto-var-init=zero` under `--with-gc` (gc-debug: 13 failures
to 0); clear 16 KB of stack below the entry before and after a
collection (gc-default: 10 to 8). What remains is a word in the padding of
`gc_collect`'s own frame; seven tests that need precise collection are
skipped with `requires_precise_gc`. test_trashcan_threads takes 2 minutes
on gc-default: two threads create objects whose `__del__` sleeps, faster
than the queue runs them.

Next: the dealloc debug check, then the day-3 tests.

### 2026-10-26 05:00 — step 5a milestones 3 and 4: day-3 tests, measurements, note 06 §15

The day-2 and day-3 lists pass on build/gc-debug and build/gc-default at
-j8: 34 files and 6,390 tests. The day-3 files are test_io, test_threading,
test_generators, test_capi, test_sys, test_embed and ten asyncio files.
dev-debug and dev-default pass the same files plus the rung list. Every
build fails only test_sys's remote-exec timeout, as dev-debug did before
the step. six and attrs pass `ports check` on both gc builds.

Fixed on the way:
- Collector and dealloc fixes:
  - the version caches of a legacy subinterpreter named code objects that
    another interpreter's sweep had freed (the threading SIGSEGV);
  - the debug sweep check now aborts when a dealloc allocates or runs
    code. It found the BytesIO export error and `module_dealloc`'s `-v`
    output. The memoryview export assertion was a separate sweep-order
    bug;
  - managed buffers are swept first, and the sweep reports to
    `PyRefTracer`;
  - the asyncio C task lists were scanned as roots, so pending tasks
    never died.
- Scheduling: automatic collections run at the eval breaker, with an
  inline fallback one threshold later (decision 3 revised, §15.2).
- False retention, for gc-default (17 failures to 0):
  - clear the stack before `gc.collect()` enters the collector;
  - clear popped C stack references;
  - clear 1 KB below each C entry into the eval loop;
  - clear the eval loop's C argument arrays;
  - clear stale saved registers.
- Speed: the collection's passes now skip dead objects by kinds recorded
  in the mark. nbody went from +45% to +28% against dev-default.

Measurements (§15.6), against dev-default:
- per loop: richards -7.7%, go -4.4%, nbody +27.9%, float +30.1%. 99.2%
  of nbody's dead objects are floats;
- `-c pass`: 80.9M instructions against 48.6M, from seven shutdown
  collections;
- full collection of about 1M objects: 41 ms;
- peak RSS for 10M garbage tuples: 57 MB;
- zero-init and the 1 KB entry clear: each under 1%.

§15 records the decisions, the dealloc fix list, the retention sources
with their runtime cost, the test adjustments, the 5b items (watchers in
the sweep, shutdown collections) and the 5c items (sweeping trivial
deallocs by block, the 52 uniqueness sites).

### 2026-10-26 05:15 — step 5a landed; step 5b launched

Step 5a handed back at 0edda3738fa and the gate was re-run here:
gc-debug on test_gc, test_weakref, test_finalization, test_io,
test_capi, test_threading, test_generators and test_asyncio.test_tasks
(16 files, 4,044 tests, all pass); dev-default rung 2 plus test_gc_verify
(9 files, 1,067 tests, all pass); six, attrs and markupsafe rebuilt and
checked on gc-default under abi_id cpython-3.16-gc-v0 (PASS, 0 patch
lines). Step 5a took six hours of a four-day budget.

Step 5b launched: full suite on both gc builds, the watcher and
destructor notifications moved out of the sweep, shutdown collections
reduced to one mark, and all seven ports on the gc builds (Cython
code-cache root registration for pyyaml, DTypeMeta traverse and table
roots for numpy). Budget 3 days.

### 2026-10-26 06:50 — step 5b milestone 1: rung 1 done

Watchers, module `m_free` and capsule destructors run in the queue phase
before the sweep (ac9a9a0062c); the interpreter's own dict watchers,
which neither allocate nor run code, stay in `dict_dealloc`
(e05e10575ef). The seven watcher dealloc tests are un-skipped. Shutdown
marks five times instead of seven (8cfb3a08aaf): the two remaining
intermediate marks observe reachability states that module-level
finalizers can see, so they stay. `-c pass` went from 83.9M to 69.4M
instructions (dev-default 48.6M). Two retention bugs on the way: a
`PyDict_Next` out-parameter kept the last module alive through shutdown,
and cross-interpreter data held objects the collector did not scan
(38e41caef2e). The 5a lists pass on both gc builds. The full suite is
running on gc-default and dev-default for the rung 2 inventory.

### 2026-10-26 08:40 — step 5b milestone 2: the full suite passes on gc-default

Full suite at 9c6de456c5a on build/gc-default, -j8: 556 files, 50,815
tests, 4 min 59 s. One failure, test_sys's remote-exec timeout, which
dev-default also has. The rung 2 inventory had 121 failures in 55 files;
they split into two C bugs (the remote unwinder's untraversed caches, the
gc stats ring), one retention cause (borrowed IR and feedback pointers in
scanned memory, now in a no-scan heap, 6bfce02ef1e), three collector
fixes (every collecting thread drains the finalizer queue, unlinked
thread states stay roots, MemoryError no longer reuses the singleton),
and tests that assumed reference-count timing or `sys.getrefcount`,
adjusted one commit per file with the assumption named. The mark finds
pages through a segment map (bacb0d499ec): 17% fewer cycles per
collection. Rung 3, the gc-debug full suite, is next.

### 2026-10-26 10:05 — step 5b milestone 3: the full suite passes on gc-debug; eight ports pass

Full suite at 8a85454903f on build/gc-debug (verifier and sweep check
on), -j8: 556 files, 50,818 tests, 15 min 22 s, one failure (test_sys's
remote-exec timeout, pre-existing). The gc-debug run found six crash
classes: two missing traverse visits (zlib, the C Unpickler), two
deallocs that ran code in the sweep (co_extra free functions, the
cross-interpreter buffer view), and pyexpat's subparser reading a parent
the sweep had freed. The verifier over the whole suite on dev-debug
reports no stdlib traverse omissions beyond those. `requires_precise_gc`
is gone: seven tests run under Py_GC, one is skipped on debug builds for
a stale -Og frame word. Ports: all seven plus the new cython port pass
`ports check` on gc-v0, gc-debug-v0 and refcnt-v0; patch lines numpy 128
(uncommitted traverses and roots), cython 43, cffi 10, others 0. Open:
numpy's array dealloc reads its descr, which the sweep may free first;
ruled a type flag "dealloc reads its referents" whose dead objects are
freed in the sweep's first pass, the first stage of the 5c ordering.

### 2026-10-26 12:26 — step 5b done: the full suite passes on both gc builds; eight ports pass with the verifier

Full suite at 12619dbe138, -j8, 564 files, 50,819 tests: gc-default
4 min 31 s; gc-debug 12 min 50 s; dev-default 2 min 53 s for
comparison. The one failure on every build is test_sys's remote-exec
timeout, which predates 5b.

Since milestone 3:
- `Py_TPFLAGS_DEALLOC_READS_REFERENTS` (bit 21, 9333cb51722) makes the
  sweep deallocate a flagged type's dead instances first. It replaces the
  managed-buffer special case; numpy's `ndarray`, `nditer` and `mapiter`
  set it. Ordering among flagged objects is 5c's.
- nbody's +54.6M against 5a was the destroy-hook type-check chain in the
  heap walk, confirmed with perf by symbol. It is one type bit now
  (f5a7bfc4ba0), and nbody is back to 1800.1M.
- numpy: most of its core types had no traverse, so views' bases,
  descriptors, dtype fields and object items were freed in use. The patch
  adds traverses to every reference-holding type and allocates
  reference-holding iterator buffers in PyMem; patch lines 535.
- `ports check` runs each test script under the verifier on reference-
  counting interpreters, with the globals alive at a final collection.
  It found the nditer buffer bug and Cython's Py_GC-only root at once.
  Cython now roots wherever `PyGC_AddRoot()` exists
  (`Py_HAVE_PYGC_ADDROOT`, c018c6cdf9c).
- test_weakset collects after deleting an iterator that held the
  destroyed item (6ffa07821d9). The multiprocessing teardown collects
  again when a queue's feeder thread stopped after the first collection
  (9deb1ea266c); that was the env change both gc builds reported.
- Three busy-loop children of test_profiling, orphaned by worker crashes
  before e49b3dd476b, kept three cores busy during the measurements; the
  wall times and pauses in §16.10 carry that note.

Note 06 §16 records the decisions, the ports and the measurements.
Step 5b took about seven and a quarter hours of a three-day budget.

### 2026-10-26 13:15 — step 5b landed; step 5c launched

Step 5b handed back at f58c4aeb94a (pushed) and the gate was re-run here:
dev-default rung 2 plus test_gc_verify (9 files, 1,067 tests, pass);
all eight ports rebuilt from scratch and checked on gc-default under
cpython-3.16-gc-v0 (PASS; patch lines numpy 535, cython 44, cffi 10,
others 0); full suite on gc-default, -j8 (556 files, 50,819 tests,
5 min 21 s). The full suite failed test_sys (pre-existing remote-exec
timeout) and test_profiling. test_profiling passes alone (481 tests) and
fails when four copies run concurrently: test_profile
`test_calling_conventions` reports no `:0(max)` entry and a negative
total time for `max(*([0],), key=int)`, so the profile hook missed or
mis-paired the C call. Repro: `build/gc-default/python -m test
test_profiling -j4 -F --timeout 300` fails in the first round. The 5b
agent's three full runs did not hit it. Recorded as an open item and
handed to 5c, rung 1, since it will be a flaky test on the default build
once the collector is default. Step 5b took about 7.25 hours of a
three-day budget.

Step 5c launched: the collector becomes the only build, the reference
counting code is deleted, the 52 uniqueness sites are resolved, flagged
objects are swept referrer first, trivial deallocs are freed by block,
the ports are rebuilt on the new ABI, and the performance is measured
against step 1 and the 5b snapshot. Budget 4 days.

### 2026-10-26 14:30 — step 5c milestone 1: the flip, and the eval loop's frames are no longer roots

The tracing collector is the only build (834938a4ffa): `--with-gc`,
`--disable-gil` and `--with-trace-refs` are gone, with gc.c,
gc_free_threading.c, brc.c, uniqueid.c and index_pool.c, about 10,200
lines. The tests whose only subject was reference counting are deleted,
one commit per file (rung 1c, 86 commits). The test_profile flake was an
upstream bug: a dead cProfile object reset the thread's profile function
(820e883eca3); 47 stress rounds clean after the fix.

`_PyStackRef` owns nothing (4ce392155b3). That exposed stale copies of
locals in the eval loop's C frame as exact pointers, so the first
installment of step 10's precise roots went in: every eval-loop
activation records its C frame extent and the scan skips it, C stack
references are registered precise roots (3d91660d780), and the cases
generator fails the build if a handler keeps an object only in a C
local across an escaping call (a1ddf08f03a; five handlers fixed). The
inline-fallback collection still scans everything. Stale-root probe:
1/100 to 100/100 on debug. Per loop against step 1, 5b refcount and 5b
collector (M instructions): richards 577/675/625 -> 518, go
1244/1624/1549 -> 1226, nbody 1030/1407/1800 -> 1455, float
780/949/1197 -> 1051. The three commits were built and tested as one
tree; bisect them as a unit.

### 2026-10-26 16:45 — step 5c milestone 2: reference counting is gone; rung 2 gate passes

Rung 2 of step 5c is complete and the full suite passes on dev-default
and dev-debug, with only test_sys's pre-existing remote-exec timeout.
Deleted: the ownership operations on `_PyStackRef` (plain copies, then
the names themselves, 21 files), the refcount helpers and immortal
refcount constants (immortality is one header bit), the object free
lists and their statistics, the trashcan, Py_REF_DEBUG and
Py_STACKREF_DEBUG, `sys.getrefcount` and `gettotalrefcount` with their
docs, and every branch `_PyObject_IsUniquelyReferenced` guarded (the 52
sites of §15.7 take their shared path). Measured consequences, recorded
not fixed: iterators cost about 2.2x the refcount build (10-25% of it
the lost tuple reuse), and `s += 'x'` is quadratic (1,204 to 230,353
instructions per append at 200k), a 6a design item. Verification: dispatch
counts match step 3b within 0.2%; hot handlers have no reference
increments or decrements. Rung 3a: flagged dead objects are swept
referrer first in reverse postorder (e638f8018da); pyexpat drops its
finalizer. The alias-fold rule that existed for borrowing is gone
(337416c0d6d), measured at zero on richards and go.

### 2026-10-26 17:15 — step 5c done: one build, no reference counts, ports on cpython-3.16-gc-v1

Rungs 3a, 4 and 5 are complete; note 06 section 17 has the record. The
sweep deallocates flagged objects referrer first, in reverse postorder of
their direct references (e638f8018da), and builds a graph only when one
flagged dead object refers to another (23c4d99af63). pyexpat's finalizer
is gone. 300,000 dead memoryviews pause 85 ms against 67 ms before, and
20,000 expat parser pairs 25 ms against 32 ms. The sweep's loop body is
inlined again: out of line it cost nbody 1.9% (0c5f6ac3aa8). The alias
fold (337416c0d6d) deletes the borrow-era no-fold rule and measures zero
on richards and go. All eight ports pass on cpython-3.16-gc-v1 and
cpython-3.16-gc-debug-v1 with the verifier pass (ff6750d3e3a); numpy's
patch drops its one Py_GC condition. Full suite on both builds: only
test_sys's remote-exec timeout.

Per loop against 5b's reference-counting build (M instructions):
richards 675.1 -> 520.0, go 1623.8 -> 1176.4, nbody 1407.1 -> 1427.2,
float 948.5 -> 1022.1. Of rung 2's gain on go, 36.6M came with the plain
stackref operations and 13.5M with the header bit and the free lists.
`-c pass` 68.4M (5b collector 69.5M). RSS and pauses are within 4% of
5b's collector build. Open for 6a and later: `s += t` is quadratic, the
box placement, the per-object sweep cost (3b was dropped), retention
through C callee frames (step 10), and the reference-counting text in
InternalDocs/garbage_collector.md and the C API docs.

### 2026-10-26 18:30 — step 5c landed; step 6a launched

Step 5c handed back at 6cef60fb57e and the gate was run here. The first
run failed test_dis, test_code and test_monitoring on both builds: the
alias fold (337416c0d6d) changed what the compiler writes without a pyc
magic bump, and the agent's runs had reused pycs compiled before it, so
its passes of those files were invalid. The fold is correct; the
expectations were stale. Fixed by b31d8b00b81 (magic 3717) and
ecdc7879253 (expectations), recorded in §17.6 and §17.8 (255ad409e03).
The dev-debug run also lost a worker to the agent relinking the binary
under it; two clean reruns had no worker death. Final gate at
255ad409e03 with fresh pyc caches: full suite on dev-default (556 files,
50,675 tests, 6 min 14 s) and dev-debug (14 min 34 s), each failing only
test_sys's pre-existing remote-exec timeout; all eight ports PASS on
cpython-3.16-gc-v1 (numpy 539 patch lines, cython 44, cffi 10, others
0); per loop against the step 1 snapshot (M): richards 577.7 -> 518.2
(-10.3%), go 1245.3 -> 1176.0 (-5.6%), nbody 1030.3 -> 1427.2 (+38.5%),
float 779.4 -> 1022.2 (+31.2%). nbody and float are float boxing at
container and attribute stores plus the per-object sweep, which 6a
addresses. Step 5c took about 5.5 hours of a four-day budget; the step 5
series (5a, 5b, 5c) took about 19 hours.

Lesson recorded for briefs: any commit that changes what the compiler
emits bumps the pyc magic in the same commit, and every test run uses a
fresh PYTHONPYCACHEPREFIX.

Step 6a launched: PyValue replaces _PyStackRef; list, tuple, dict, set,
cells, globals and instance slots hold PyValue with canonical inline
storage; boxing only at the PyObject* conversion; 8-byte header;
weakrefs in a side table. Budget 5 days.

### 2026-10-26 23:05 — step 6a milestone 1: tuples, lists, cells, dicts, globals and instance values hold PyValue

`_PyStackRef` is `PyValue`, public in Include/pyvalue.h, with the helper
set reduced to the minimum and no aliases (98f38e1da76). Every PyValue is
canonical: construction from a PyObject* inlines an int32-fitting int or
a non-NaN float, so identity is a bit compare and a box lives only in a C
local rooted by push and pop around the escaping call (a495d4cbdc2). The
interim cost of canonicalising loads from containers that still held
pointers was richards +18% and go +26% per loop; the container
conversions then removed it in turn: tuples and lists (20693f43a82;
nbody 1427M -> 999M per loop, below step 1's 1030M, the 1.5M list-store
boxes per loop gone), cells (e94834f371d), dicts, globals and instance
values (0389837b3fc; richards 590M, go 1353M; attribute-store boxes on
go 447k -> 0). Full suite green on both builds at each commit, fresh pyc
caches. Ruled: a box allocation inside an accessor with no error channel
(PyList_GET_ITEM, PyDict_Next and the like) collects, retries once, then
aborts naming the accessor, because a silent early end would be a latent
bug in every port. Found on the way: the allocator never collected on
failure, only at the threshold (recorded for step 7/10). Remaining:
__slots__, sets, the hot callers that still box (reserved dict watchers,
the unspecialised attribute lookup, enumerate over lists), the 8-byte
header, the weakref side table, ports categorisation, §18.

### 2026-10-27 01:50 — step 6a milestone 2: every container holds PyValue; the header is one word

Since milestone 1: a boxing accessor with no error channel collects,
retries once, then aborts naming the accessor (b164b4ad838); __slots__
hold PyValue (ab31d98bf69; float 1266M -> 859M per loop, -32%); sets
hold PyValue (0b8cbe360c5); the hot callers take values (d408626d7af,
8b9e4756fd4: list append, the core dict watchers, generic getattr,
enumerate over list and tuple iterators, STORE_GLOBAL; richards makes
zero boxes per loop, spectral_norm 1420M -> 1179M); float ** float with
a positive base has a per-operation inline handler (85449792f97, magic
3719; nbody 1007M -> 778M, zero boxes). Per loop now against step 1:
richards 580M vs 578M, go 1265M vs 1245M, nbody 778M vs 1030M, float
859M vs 779M (its 300k boxes per loop are arguments to C math functions,
inherent until 6b). The object header is one word, type pointer and
sixteen flag bits (76232f16b53): getsizeof of int/float/3-tuple/10-list/
small instance 32/24/72/136/48 -> 24/16/64/128/40; the collector's
"is this block an object" test moves from a header magic to an object
map, one bit per 8-byte word per 32 MiB chunk. Ruled: PyGC_Head, now a
zeroed 16-byte pre-header, goes in 6a after the weakref side table.
Ports on cpython-3.16-value-v0: six, attrs, pycparser, markupsafe and
cython (pure) pass; cffi and pyyaml fail on `&PyTuple_GET_ITEM` as an
lvalue and `ob_item` as `PyObject**`; numpy is blocked on the same
Cython utility code. That is 6b's input.

### 2026-10-27 03:20 — step 6a done: PyValue in the core, a one-word header, no PyGC_Head

Since milestone 2:

- Weak references live in a table keyed by address. 43 core and stdlib
  types lost their weaklist field and set `Py_TPFLAGS_MANAGED_WEAKREF`,
  which no longer requires GC support (628da0d299a).
- PyGC_Head went, with the generation list heads and everything that
  served them. A collected object starts at its block (f08adc371f1).
- The managed-dict pre-header stays 16 bytes, a dict word and a pad. A
  one-word pre-header leaves objects 8-aligned, and ctypes' aligned stores
  then crash.

getsizeof of int, float, 3-tuple, 10-list and a small instance went from
32/24/72/136/48 before 6a to 24/16/48/112/24.

Per loop against step 1:

| | step 1 | now |
|---|---|---|
| richards | 575M | 594M |
| go | 1244M | 1303M |
| nbody | 1030M | 787M |
| float | 781M | 903M |
| spectral_norm | 802M | 1212M |

The three header commits cost +1.1% to +6.0%, almost all of it the header
commit's: the object map's set, test and clear, and Py_TYPE as a 48-bit
mask (347 masks in the eval loop). Smaller objects partly offset it.

Ruled: the trigger's requested-byte count goes to step 7, and the boxing in
LOAD_NAME and STORE_NAME is recorded, not converted. Those two explain why
peak RSS on the module-level tuple loop rose from 60 MB to 101 MB.

Both full suites are green at every commit; the only failure is test_sys's
remote-exec timeout. Ports on cpython-3.16-value-v0:

- six, attrs, pycparser, markupsafe and cython (pure) pass;
- cffi fails on 6 `ob_type` reads and 3 item arrays;
- pyyaml and numpy fail in Cython's utility code: `m_weakreflist` and
  `&PyTuple_GET_ITEM`. That is 6b's input.

Note 06 section 18 has the record. Step 6a took about 9 hours of a
five-day budget.

### 2026-10-27 05:50 — step 6a landed; review pause before 6b

Step 6a handed back at 0db53649e9c and the gate was run here on an idle
tree with fresh pyc caches: full suite on dev-default (556 files, 50,692
tests, 5 min 4 s) and dev-debug (12 min 16 s), each failing only
test_sys's pre-existing remote-exec timeout; six and attrs rebuilt and
checked on cpython-3.16-value-v0 (PASS, 0 patch lines); per loop (M)
step 1 / 5c / now: richards 576.5 / 519.5 / 593.1, go 1244.3 / 1179.7 /
1302.5, nbody 1030.3 / 1424.0 / 786.6, float 780.5 / 1030.6 / 903.0.
Step 6a took about 11 hours of a five-day budget.

6b is not launched. Note 08 (boil/design/08-review-at-6a.md, loom
artifact review-6a) pauses the plan at this boundary: it lists the
interim mechanisms with the step that retires each (the hack register),
proposes 6b as "the PyValue C API" (value-typed protocols as the core's
interface, the PyObject* API as the boxing compatibility layer, then the
Cython and C ports) rather than patching ports against boxing accessors,
states the rule that a specialised handler exists for a feedback kind the
JIT guards on and not for a benchmark (BINARY_OP_POWER_FLOAT's
positive-base guard is the case in point), and proposes splitting step 10
so that precise roots (10a) run after 6c and before the generational
collector and the JITs. A pyperformance timing run against the Phase 1
baseline starts now as the first wall-clock measurement since Phase 1.

### 2026-10-27 06:40 — plan revised at the 6a review; 6b launched

The user ruled on note 08: 6b is the PyValue C API; step 10 splits into
10a (precise roots, after 6c, before the generational collector and the
JITs) and 10b (moving); and from here no step tunes for its own numbers,
each step is rigorous and correct and leaves on the table performance work
a later step would discard. Note 09 answers the question the user put
about user classes: the shape moves into the header word (type through the
shape, root shape per type, flags and a mutable type_version in the shape),
every slot holds a PyValue with the slot kind as a promise, and lookup is a
pure plan from (shape, name) that the generic getattr, the specialiser,
tier 0 and tier 1 all consume, so ordinary lookup is inspectable data
rather than the side effect of tp_getattro. Note 06 sections 3.2, 3.3 and
6 carry the revision; 6c is redefined around note 09.

The first pyperformance run died at import in every benchmark: pyperf's
psutil came as a prebuilt abi3 wheel, which reads the old header and
segfaults. psutil rebuilt from source works. 6b therefore also removes the
stable ABI (.abi3 out of the extension suffixes, Py_LIMITED_API an error)
so an incompatible wheel fails to import instead of crashing. The timing
run is in progress. 6b's agent is launched with a hard stop of 2026-10-31
12:00, rungs: value-typed protocols with tag dispatch (POWER_FLOAT
removed), vectorcall and METH_FASTCALL over PyValue arrays, the
compatibility layer and abi3 removal, the eight ports on value-v1, record.

### 2026-10-27 06:05 — first wall-clock measurement since Phase 1

pyperformance subset at 6a against the Phase 1 baseline: geomean 1.46x
slower. nbody 1.47x faster, richards 1.03x and go 1.16x slower; the
allocation-heavy benchmarks carry it (coroutines 2.75x, generators 2.55x,
comprehensions 1.98x, pickle 1.81x). perf puts 25 to 30 percent of the
generator and coroutine samples in the collector; a non-generational mark
of the whole live heap at an allocation-proportional rate is the cost, and
step 7 over 10a's roots owns it. Recorded in note 08 section 7.1 with the
attribution; no tuning per the plan revision. generators and
comprehensions join the recorded per-loop set. The 6b agent reported its
rung 1 plan and was told to hold builds until the run ended; builds are
released.

### 2026-10-27 08:05 — 6b rung 1 gate (value protocols) clean

Gate run here on a detached checkout of ac2dbf90677 (rung 1's last
commit), fresh builds and pyc caches, while the agent went on with rung 2:
default 556/564 files, 50,726 tests, 2 min 48 s, only test_sys's
pre-existing remote-exec timeout; debug 9 min 15 s, only test_sys. The six and attrs check ran against
the gate build but its result lines were not captured (only the tail of
its JSON was kept); it is re-run at the rung 2 gate. Rung 1 delivered the
value-typed number, object, item and iteration protocols with every
generic handler on them, slices holding PyValue, BINARY_OP_POWER_FLOAT
removed (magic 3720), the specialiser reading values, a box audit tool,
26 test_capi tests, and the FOR_ITER_GEN fix: the specialised generator
loop had missed on every execution since the register IR's store folding,
because yields and the return value shared one slot; the return value now
goes to the slot END_FOR consumes. Two more release-build retention tests
joined hack-register row 7. Rung 2 (vectorcall, METH_FASTCALL and METH_O
over PyValue, clinic regenerated) is under way with the agent's three
proposals accepted.

### 2026-10-27 10:40 — 6b rung 2 gate (calls on values) clean

Gate run here on a detached checkout of 3832173d942, fresh builds and pyc
caches: default 556/564 files, 50,729 tests, 3 min 10 s, only test_sys's
pre-existing remote-exec timeout; debug 8 min 53 s, only test_sys. The
six and attrs check could not run against the gate interpreter: the
working tree's ports/sdk.json had already moved to cpython-3.16-value-v1
for rung 3, so the checker refused the value-v0 builds. The agent's own
check at the rung end (build --force then check on value-v0) had six and
attrs PASS with 0 patch lines; the ports are re-checked here at the rung 4
gate on value-v1. Rung 2 landed in three commits: vectorcall takes and
returns values (057a2b29eba), the eval loop returns a value (ff137bee8b2,
PEP 523's frame evaluation function changes signature), and METH_O and
METH_FASTCALL take and return values with Argument Clinic reading inline
ints and floats directly and boxing only object-typed parameters
(3832173d942, 345 files; the limited-API modules became core modules).
Two defects found and fixed on the way: a return converter with a custom
conversion bypassed by the value path (posix.makedev), and dict subclasses
losing BINARY_OP_SUBSCR_DICT because an inherited slot was recognised by
pointer equality with a METH_COEXIST method that now takes a value. Four
more tests carry conservative-root skips with probe evidence, including a
new kind: a one-past-the-end argument pointer in a callee-saved register
aliasing the next block, which has its own hack-register row (retired by
10a for core frames, 10b or allocator slack for extensions). Rung 2
finished about 20 hours ahead of its deadline; rung 3 is under way.

### 2026-10-27 12:45 — 6b rung 3 gate (compatibility layer) clean

Gate run here on a detached checkout of 921352f5653, fresh builds and pyc
caches: default 556/564 files, 50,730 tests, 2 min 49 s, only test_sys's
pre-existing remote-exec timeout; debug 9 min 26 s, only test_sys;
test_cext under -u cpu 16 run, 7 skipped, success; ports six 1.17.0 PASS
and attrs 26.1.0 PASS on cpython-3.16-value-v1, 0 patch lines each. Rung 3
landed as: no limited C API or stable ABI (d1ec4c4f6db; .abi3 out of the
extension suffixes, Py_LIMITED_API an error, the limited modules became
core modules); the PyObject* protocol as a layer over the value forms
(b05e1556892); public headers C90, C++03 and -Wcast-qual clean, which
fixed a test_cext failure unseen since step 5 because only -u cpu runs it
(5ab886ef2af); the core and hot modules calling the value protocol
(4df049cdd1e, with an itertools.batched segfault on an exhausted iterator
fixed); a release call keeps its object alive up to that point
(28f6a199518: reference-correct C that reads owned memory and then
releases the owner was collector-incorrect since 6a, shown by a
million-item str.join; the barrier is a hack-register row retired by
10b's handle discipline); the 4,095-slot limit of the register IR is a
SyntaxError and is recorded (8e7337f92d4, 0d8458a0e35); tp_iternext
returns a value across the core and all 37 Modules iterators
(921352f5653, with map() rooting its overflow items and a hack-register
row for the C-local PyMem arrays that only the transitive scan keeps
alive). An ABI flag "v" was tried and handed off: packaging's vendored
copies in pip and setuptools reject any CPython ABI tag they do not know,
so wheel builds fail; the SDK's abi_id marks the ABI until the ports tree
carries a packaging port. Rung 3 finished about 30 hours ahead of its
deadline. Rung 4 (ports) is under way with three port agents.

### 2026-10-27 14:05 — B: the internal benchmark set, instr and rss of record

boil/bench/run.py runs note 10's set against one build and writes one JSON;
compare.py prints the ratios and geomeans; boil/bench/README.md documents
both. The kernels are pyperformance's 29 benchmarks from the 22 files of
the Phase 1 subset, copied in-tree under MIT with pyperf removed (go is
seeded; untimed preparation moved to setup()). The micro set is 43 files:
note 10's object-protocol and calls list, and micro_loops.py's 16 loops
unchanged, which reproduce note 02's values on baseline (skeleton 80.7,
int add 164.0, float add 130.1). The memory group is 10M tuples, 1M
instances, 200k dicts, start-up, imports, and the collector benchmark:
L MB of (k, [k]) live and 1 GB of short-lived batches per loop, at L of
16, 128 and 512, sized in object counts fixed from the baseline's
allocation, so every build runs the same objects. Metrics: instr
(differenced at loops and 3*loops, median of 4), time, rss and boxes (on a
Py_STATS build; tested on build/step5c-pystats-snapshot). Children run
with PYTHONHASHSEED=0 and no PYTHON* variables, since -E ignores the seed:
dict_subscript's spread across pairs fell from 30 instructions to 0.3.
micro_loops.py, rss.py, perloop.py and boxes.py are folded in and deleted.

Results of record (boil/bench/results/), geomean against baseline, step1
then step5c: instr on the kernels 1.012 and 1.151, micro 0.894 and 0.781,
memory 1.159 and 1.998; peak RSS on the kernels 1.080 and 4.123, memory
1.143 and 1.774. step5c's collector costs 2.92x, 3.24x and 4.18x the
baseline's seconds per allocated GB at 16, 128 and 512 MB live (a smoke run
at load 1.5, not of record), against the baseline's flat 0.52 to 0.58 s:
that is the number step 7 must move. The boxes metric accounts for three
step5c micro regressions: math.sin 2.724 (a float and an argument array
boxed per call), generator resume 2.349 (the yielded int) and attribute
store 1.352 (the stored int). Kernel peak RSS grows most where allocation
is heaviest (nbody 8.19, fannkuch 7.63, coroutines 7.52). The time metric
is not yet of record: it is taken only from a run whose 1-minute load
stays below 4 throughout, and the 6b gate and the 6c builds have kept the
machine busy; a watcher takes it in the first quiet window.

### 2026-10-27 14:45 — B: time of record

The time metric of record ran in quiet windows with --max-load 4 (highest
1-minute load sampled 2.08, 2.15 and 2.4; one step5c attempt refused at
5.52 and repeated). Geomean against baseline, step1 then step5c: kernels
1.039 and 1.309, memory 1.181 and 2.113. step5c's time exceeds its
instructions most where allocation is heaviest (coroutines 2.458 against
1.354, comprehensions 2.135 against 1.361, generators 1.749). The
collector benchmark of record: baseline 0.533, 0.580 and 0.547 s per
allocated GB at 16, 128 and 512 MB live; step5c 2.859, 3.237 and 4.248
times that, growing with the live heap. boil/bench/results/ now holds
instr, time and rss for all three labels of record.

### 2026-10-27 15:10 — 6c rung 1 gate (the shape in the header) clean

Gate run here on a detached checkout of eb4249ad318 with fresh builds:
gate 2 on default 556/564 files, 50,754 tests, 2 min 49 s, only test_sys's
machine-local remote-exec timeout; gate 3 on debug 9 min 52 s, only
test_sys; -u cpu test_cext success on both builds after the fix below;
regen-cases and clinic leave the tree unchanged. Rung 1 puts the shape in
the header word: every type embeds its root shape, Py_TYPE reads
shape->type, the Check macros read the shape's flag copy, exact checks of
builtin types compare the header with the root shape, shapes are
unconditional with --with-shapes gone, and the feedback entry carries the
shape pointer (magic 3721). Recorded: richards +0.78%, go +1.40%,
json_loads +0.58% per loop, 1,836 Py_TYPE sites, 278 header masks in the
eval loop. Note 09 went through two review rounds (verdicts revise, revise)
and its sections 2, 4, 5, 6 and 9 were rewritten with the decisions in
section 11; the rung 2 work doc was reviewed and revised before code.

Two coordinator fixes: test_cext's C++ virtual object was allocated with
new and tripped 6b's debug heap check (29cbc5795a2); it now gets its
storage from the object allocator, which is the rule for any C++ object
that is a PyObject. gate.sh ignores the test_sys remote-exec test so the
later checks of a level run.

## 2026-10-27 15:30 PDT: 10a design adopted (note 11)

Note 11, precise roots, landed on the integration branch (22a38cc021a)
after two codex review rounds (revise, then accept with the kill-mode
landing rule). Decisions: an attributed DWARF unwind walk; frames are
precise or opaque by a section marker; the precise set is the eval loop
and handlers, the entry paths, the call layer (about 80 functions) and
about 25 ceval helpers; the rest of core C and all extensions stay opaque
and conservatively scanned, with PyMem blocks followed only from extension
frames; allocator slack rejected on its measured RSS cost (+8.5% geomean);
a frame is a root from push to pop; tier 1 gets safepoint tables; step 7
inherits the root set and the barrier rule. The coordinator added open
question 10 at adoption: the C-mode checker (a CFG, liveness and
provenance analysis over a declared C subset) is the largest tool in rung
1, so it has a one-day box, with the kill-mode verifier, the retain gate
and the inline audit as the proof if it overruns, and no 8.2 skip removed
on the dynamic proof alone. Note 06 rows 10a and 7 updated. The 10a
implementation starts in its own worktree after 6c lands, so that the two
do not contend for the eval loop.

## 2026-10-27 16:05 PDT: 6c rung 2 gate (the lookup plan)

Rung 2 landed as 80ba2edf94c (magic 3722). Gate run here on a detached
checkout of that commit with fresh builds: gate 2 on default 556/564
files, 50,789 tests, 2 min 38 s, SUCCESS; gate 3 on debug 9 min 28 s,
SUCCESS, -u cpu test_cext 16 tests, regen-cases and clinic leave the tree
unchanged. The rung adds the lookup plan of note 09 section 4
(`Include/internal/pycore_lookup.h`, `Objects/lookupplan.c`): the (shape,
name) cache yields a plan, the specialiser reads plans instead of walking
the MRO, and LOAD_ATTR_PROPERTY guards the getter. The debug gate found a
specialiser bug before the commit: a Python `__getattribute__` found under
type version 0 was being specialised and could never deopt; the specialiser
now declines it, covered by test_shapes. Recorded by the agent against
b65f081239d: kernels geomean 0.997 instructions per loop (richards -3.13%,
the others within 0.6%); micro geomean 0.993 (attr_load_poly -10.0%,
property_load +1.8%); boxes per iteration unchanged; plan cache 15,888
bytes over richards' 14 classes. Five departures from the rung 2 doc are
recorded under "As built" in boil/design/work/6c-rung2.md and move to rung
3: the prepare/commit split of stores, the storage adapter's precondition,
stores over non-data class attributes, the set-only descriptor test's
type, and names. Rung 3, the instance layout, starts with its work doc.

### 2026-10-27 16:25 — 10a rung 0 gate (baseline counters, c_roots, ASan) clean

Rung 0 of 10a (note 11 section 12) landed on boil/10a as: the work doc and
its review (54e5548a7d0); root-phase counters, words and hits per scan
class plus phase times, read by `_testinternalcapi.gc_root_stats()` and
dumped per process and interpreter to `$PYTHON_GCROOTSTATS` (b7aa17bfc76);
an ASan build with the collector, mimalloc kept as the allocator with
`MI_TRACK_ASAN` and the conservative scan unsanitized (34db6e245ec);
`boil/tools/measure/c_roots.py`, the token-level probe (56ffe6f6d79); note
11 questions 11 and 12, as ruled, and the measurements (e20e241f5c9).
c_roots counts 6,644 of 16,251 functions holding an object across a
collecting call, more than note 11's estimate because the item and cell
readers box in this tree (question 12). Over gate 2's suite, 51,301 marks
spend 13.6 ms each in the root phase, 13.0 of it in the precise structs
pass; per mark the conservative scan reads 905 stack words, 154,360 data
words and 9,805 struct words; the counters cost at most 0.02% instructions
on the collector benchmark. Under ASan test_gc runs 68 tests with two
retention failures (the conservative scan's), and poisoning reports
use-after-poison. Gates: gate 0 1,898 tests; gate 1 1,456 on both builds;
gate 2 556/564 files, 50,734 tests, only test_sys's pre-existing
remote-exec timeout; gate 3 the same on build/debug, regen unchanged.
test_cext under -u cpu failed 5 of 16 on VirtualPyObject (a C++ `new`
object) under 29cbc5795a2's object-heap check, which is in 10a's base;
after merging the integration branch at 15:10's 6c rung 1 and rung 2
(a6288f5e776, which brings 0a1d762a998's fix), build/debug runs 16, 7
skipped, success, and gates 0 (1,898) and 1 (1,456) pass again. The rung 1 work doc and its review are
887f403e01a; the C mode started 16:04 and has until 2026-10-28 16:04.

## 2026-10-27 20:05 PDT: traverse audit landed (boil/audit)

Triggered by 6c's test_buffer failure, a missing traverse in _testbuffer's
ndarray (49580514d90). The finding behind it: Tools/boil/traverse_audit.py
had listed that type all along, and the audit was never a gate, so its
output was not read. The tool now follows pointers and arrays to plain
structs two levels deep, learns bases from tp_base assignments and
FromSpecWithBases, reads same-file callees for the partial check, and has
a --check mode with an allowlist (Tools/boil/traverse_allow.txt, each entry
with a reason) that gate.sh level 3 runs. Fixed, each with a test that
collects, fills freed memory and uses the referent: _ssl's socket traverse
(visited only its type; not built here, checked by reading), _testcapi's
awaitType and GenericAlias, xxmodule.Xxo (not built), _remote_debugging's
BinaryReader strings and BinaryWriter hash keys (the tool cannot see
hash tables; found by grep). Handed to 6c and ruled there: the collector
marks the dict word at tp_dictoffset and the runtime-managed storage of
every instance before tp_traverse, which removes the heaptype.c trio from
the allowlist once it lands. Gate 2 on the branch: 556/564 files, 50,851
tests, no failures; one env-changed in test_inspect (an unraisable
KeyError '__import__' while a coroutine was finalised), not seen before,
to be watched in the next gates. OpenSSL headers are absent on this
machine, so test_ssl runs in no gate.

## 2026-10-27 21:10 PDT: 6c rung 3 gate (the instance layout)

Rung 3 landed as eight commits ending at a9e02dccada (magic 3724). The
agent's gate 3 passed on both of its builds (50,853 tests each, test_cext,
regen and the traverse audit clean). The coordinator's gate on a detached
checkout of a9e02dccada with fresh builds: gate 2 on default 556/564
files, 50,853 tests, SUCCESS; gate 3 on debug FAILURE with two files.
test_io.test_signals test_reentrant_write_buffered hung to the 20-minute
timeout with the traceback showing the signal handler's RuntimeError
raised inside a weakref callback (_weakrefset._remove) that the collector
ran, the hazard 10a found and fixed on its branch (17060af2a4a, the
callback queue defers to the eval breaker while a signal is pending); the
fix reaches the integration branch when 10a lands. test_bdb
test_next_command_in_generator_for_loop saw a 'line' event from a
callback named cb (line 501) between its expected events: a deferred
Python callback ran under sys.settrace during a traced step. It passed 5
of 5 reruns alone on the same debug build; it fails under suite load.
Both are the collector's callback-queue timing, present before rung 3,
and neither touches rung 3's code; the rung is accepted on that basis and
the test_bdb case is handed to 10a as a hazard to examine with its own
gate 3. Rung 3 content: positional shape slots with dictionary mode in
slot 0, no pre-header or values header, capacity as a type constant
(recomputed from __static_attributes__ before the first instance, which
dataclasses now set), LOAD_ATTR_SHAPE_POLY over two to four shapes, split
tables and PyDictValues deleted (PyDictObject one word smaller), the
collector marking every instance's attribute storage before tp_traverse,
and dataclass and str_concat_loop micro benchmarks. Measurements against
rung 2 (instructions): kernels 1.004, micro 1.012, memory 0.954; RSS
kernels 0.997, memory 0.968; richards -31%, attr_load_poly -68%, go +24%
(polymorphic stores generic, decision 4), +2 instructions per iteration
on every micro (gcc spilling the register-file base in FOR_ITER_RANGE),
logging +11 to +14% (dictionary-mode conversion). Wall time not measured
(other agents' gates on the machine). gdb is not installed, so the
libpython.py change is parse-checked only.

## 2026-10-27 22:10 PDT: 6c rung 4 gate (slot kinds)

Rung 4 landed as 57b7ed315fe (work doc), 0c0ae6f6fcb (shape and type
versions paired by consumer, magic 3725), e35552a3343 (slot kinds I32, F64
and BOXED as promises, generalised in place with a fresh version, one
_GUARD_SLOT_KIND uop, magic 3726) and 371c9c93078 ("As built"). The
agent's gate 3 passed on default; on debug it failed test_import and
test_compileall because of the coordinator's gate.sh change 42274a6aee8
(pyc prefix moved off TMPDIR's filesystem; test_import renames a pyc
across devices), reverted in 3ac12dd5a6e, which also makes gate.sh parse
whole before running so an edit during a run cannot break it. Gate run
here on a detached checkout of 3ac12dd5a6e (rung 4's code plus that fix)
with fresh builds: gate 2 on default 556/564 files, 50,876 tests, SUCCESS;
gate 3 on debug 50,876 tests, SUCCESS, -u cpu test_cext 16 tests, regen
and the traverse audit clean. The test_io.test_signals hang did not
recur this run. A row's shape pointer is never dereferenced, only
compared (stated at _PyFb_ShapeRow), and shape versions come from the
per-interpreter type-version counter and are never reused; the
address-reuse test reached a real reuse.

## 2026-10-27 21:55 PDT: 10a rung 1 (the tools of precise roots) closed

Branch boil/10a at 88f96f19459; the record is boil/design/work/10a-1.md.
All eight items landed: the C mode within its day, spans, the inline
audit, the walk with question 16's pinned and unpinned detach records,
the loader generation, the data-stack and chain visits, the verifier's
retain and kill modes (rung 1's exclusions by default, `:pymem` and
`:structs` for rung 3), and injection sites with `PYTHON_GCSTRESS`.
Gates at 88f96f19459: 0 and 1 on both builds (test_bdb, the trace and
monitoring tests, test_pdb, test_gc); gate 2 on build/default, 556 files,
51,932 collections, 0 fallbacks; gate 3 on build/debug, 52,865
collections, 0 fallbacks, 886 unpinned records visited (0.02 a
collection); the suite under plain `PYTHON_GCVERIFY=kill` on build/debug,
556 files, 52,926 collections, no failures. Found and fixed on the way:
the collector read `ob_type` of a static type that `PyType_Ready()` was
still readying (by stress); the kill mode's quarantine thread state was a
global that own-GIL interpreters raced on (by the kill suite); a queued
callback ran under a trace function installed after its object died
(test_bdb on the integration branch), now drained by the trace, profile
and monitoring setters. Recorded, not fixed: shared-heap subinterpreters
run the main interpreter's callbacks (step 7; note 08 section 3, note 11
question 18); the compiler's units and `PyBytesWriter` (rung 3's sites).

## 2026-10-27 23:25 PDT: 6c complete; 10a rung 1 landed; gate of the merged head

Step 6c closed with rung 5 (f70da2cad2a counters, 2b0c0a2a9f2 write-up;
note 06 section 20 has the decisions, measurements and the hand-off list
by owner). boil/10a's rung 1 (the precise-roots tooling: C mode and inline
audit, the attributed stack walk with pinned and unpinned detach records,
the data-stack frame visit, retain and kill modes, injection and stress,
the callback-queue fixes for signals and tracing hooks) and rung 2's
vectorcall core landed as the merge 3c2f253232c; the only conflict was the
worklog, both sides kept. Gate run here on a detached checkout of
6086cccc9c2 (the merge plus a gate.sh rename) with fresh builds: gate 2
on default 556/564 files, 50,931 tests, SUCCESS, 52,216 collections, 0
walk fallbacks, 881 unpinned detaches; gate 3 on debug 50,931 tests,
SUCCESS, 53,094 collections, 0 fallbacks, -u cpu test_cext 16 tests,
regen and the traverse audit clean. The test_io.test_signals hang that
failed 6c's rung 3 and rung 5 debug gates did not recur with 10a's
callback-queue fix in the tree. Eval-loop file ban on 10a lifted; 10a
finishes rung 2's call layer, then rung 1b promotes the eval loop. The 6d
design agent (object slots on values, note 12) launched in its own
worktree.

## 2026-10-28 00:20 PDT: 6d design adopted (note 12)

Note 12, object slots on values, landed as the merge of boil/6d after two
codex review rounds (revise, revise; the second's request for a new ABI
mark per rung was rebutted: no binary is built against an intermediate
rung). Decisions: five slot types change in place to values (getattro,
setattro, descr get and set, a callfunc for tp_call) with getset and
wrapperfunc, no object-form twins; self stays PyObject*; the object-form
protocol functions are the compatibility layer; three catches for
unconverted code (a debug canonical-form check on slot results, a tracer
check for inline values in PyObject* fields, the SOABI mark boil2);
Python descriptors call the raw __get__ unbound; STORE_ATTR_SHAPE_POLY and
LOAD_ATTR_METHOD_LAZY_DICT take the opcodes freed by folding the _NO_DICT
loads; the block-filling capacity rule (block bytes -24.1% over 21
kernels, -4.5% without generators; no block grows); class_version on
shapes; the dictionary conversion presizes. Coordinator rulings on its
open questions: inline int and float receivers go to a new step 6e with
the remaining slot families (plan row added); 6d rung 1 may start beside
10a's rung 1b, rungs 2 to 4 from the integration commit that holds 10a's
rung 2; 10a's rung 1b marks the typeobject.c bridges 6d writes to the C
mode's subset; the handler-space cleanup goes to step 8's design (row
updated); the capacity rule is accepted with the stated fallback;
tp_getattr and tp_setattr stay object slots until phase P. The design
agent continues as 6d's implementation agent in its worktree.

## 2026-10-28 02:40 PDT: 6d rung 1 gate (the hand-offs and the boil2 mark)

Rung 1 of 6d landed as the merge fc926ebe3aa of 6b5d93bc95b (commits
92ce784a6a4 class_version on shapes and the member-site guard, SOABI
boil2, magic 3727; 42406f734b5 capacity by unpadded size class and the
presized conversion; 73e5daa8b28 the _NO_DICT loads folded into the shape
handlers, STORE_ATTR_SHAPE_POLY and LOAD_ATTR_METHOD_LAZY_DICT on the
freed opcodes, magic 3728; c0e0607ee35 test_frozenmain.h regenerated;
6b5d93bc95b "As built"). The agent's gates: gate 2 on default 50,957
tests, 0 fallbacks; gate 3 on debug 50,957 tests, SUCCESS. Gate run here
on a detached checkout of fc926ebe3aa with fresh builds: gate 2 on default
556/564 files, 50,957 tests, SUCCESS, 52,249 collections, 0 fallbacks;
gate 3 on debug 50,957 tests, SUCCESS, 53,128 collections, 0 fallbacks,
-u cpu test_cext 16 tests, regen and the traverse audit clean. Recorded by
the agent against the rung's before records: attr_store_poly 0.306,
method_lazy_dict 0.516, go 0.863 (441k poly stores per loop), richards
0.941, logging 0.95, kernel geomean 0.989; boxes unchanged; logging
allocations -30 per loop from the presize; capacity dictionary-mode
entries over the Py_STATS suite 26,080 against the fallback threshold of
53,084, so the size-class capacity rule stands. One hazard recorded: a
first-run hang in test_concurrent_futures test_shutdown_deadlock_pickle
(the forked child never exited; 3/3 passes alone under load). 6d's branch
now contains boil/10a through item 4, so its later rungs land after 10a's
rung 1b.

## 2026-10-28 05:12 PDT: 10a rung 1b (the eval loop precise) closed

Branch boil/10a at 3d90f6ac9af; the record is boil/design/work/10a-1b.md.
All six items landed:
- the return order, with the entry frame's visitor and injection sites;
- allocations as safepoints in the handlers;
- the entry paths precise;
- the eval loop marked, with the tail-call handlers;
- the deletions: the eval loop's activations, its stack clears and the
  value-root clear;
- the trap type, which turns a use of a killed object into an abort that
  names the word and the collection that killed it.

The eval loop's frame is no longer a root: test_gc's probe frees a dead
super() receiver 20 times in 20 on both builds, where the unmarked loop
kept it.

The first end gates, at d5d83aad9f7, failed three lines:
- the kill suite: a verifier ordering defect exposed by the eval loop
  going precise, not a collector defect. The quarantine was released
  after the page lists were built. Fixed in 349cd951673, with a
  regression test proven against the old order in 70ac80bf3f3. The other
  kill-suite failure, test_manager's barrier timeout, was load-induced at
  a load average of about 20.
- ASan: two test helpers whose redzones the scan or the scrub must not
  see, and the verify tests' ASan branches;
- stress: finding B, a shared-heap subinterpreter running main-interpreter
  callbacks and finalizers during its own start-up. It is step 7's, and
  test_capi.test_misc is excluded from stress until then.

Reruns at 349cd951673:
- gate 3 debug: 55,179 collections, 0 fallbacks;
- the suite under kill: 556 files, 0 fallbacks;
- ASan test_gc;
- stress test_capi.

All pass, as does gate 1 test_gc under kill at 70ac80bf3f3.

Measured, not gated:
- kernels: instructions geomean 1.014, memory 1.017;
- the allocation fast path: +3.0% and +4.5%;
- stack words per collection: 204 down to 150;
- micro geomean: 1.043.

The micro regression bisects to item 2's handler changes, which the
benchmarks do not run. The reading is code generation in the single
eval-loop function.

## 2026-10-28 06:15 PDT: 10a rung 1b landed; gate of the merged head

Landed as two merges: 29a358b492d (boil/10a at 3d90f6ac9af, the six
items, the end-gate fixes, the regression test and the call_effects
hand-off) and 4bb672e0bf4 (boil/10a-rung1b at 4bd1cf172e0, one fix found
by the landing gate). The first merge's conflicts: shapeobject.c's
dict_from_slots (the rooted dict plus 6d rung 1's debug keys assertion,
the form 6d had used at b05bb9fe068), ir_targets.h and
generated_ir_cases.c.h regenerated, the worklog with both sides kept.
Handler numbering unchanged apart from the precise attribute, so no
magic bump.

Gate run here on a detached checkout of 29a358b492d with fresh builds:
gate 2 on default FAILED one test, test_gc's
test_inline_collection_skips_eval_loop (2 of 20 receivers freed; 3 of 3
alone; debug passes). Traced by the 10a agent with the by-address
stack-hits tool: a stale copy of the receiver in the opaque frame of
_PyList_FromValues, BUILD_LIST's allocator, exposed by the merge's stack
layout under that frame (both branches passed alone, and so did the
tree with 6d rung 2 on top). Fixed by promotion: PyList_New and
_PyList_FromValues precise (4bd1cf172e0; probe 20 of 20 runs, gate 0 and
1 on both builds, kill on test_gc and test_list). Gate run again on a
detached checkout of 4bb672e0bf4: gate 2 on default 556/564 files,
50,978 tests, SUCCESS, 54,257 collections, 0 fallbacks; gate 3 on debug
50,978 tests, SUCCESS, 55,255 collections, 0 fallbacks, -u cpu test_cext
16 tests, regen and the traverse audit clean.

Hack-register rows retired by this rung (note 08 section 3; the 10a agent
marks them with the commits in its next record): the 16 KB and 1 KB stack
clears, _PyGC_CLEAR_ARRAY and the zeroed scratch arrays, the popped
_PyValueRoot clear and the saved-register clears, and the eval-loop
activation ranges with Py_NO_INLINE on the helpers. The eval loop's frame
is a mapped frame, not an excluded range. Still open on the register for
10a: -ftrivial-auto-var-init=zero (rung 3), the release-build test skips
and the C-local PyMem arrays (rung 3), the inline-fallback scan.

Also landed on the integration branch today: PROCESS.md rewritten on the
user's direction (0af778a77ca, df0bb37edff): the plan and the position in
section 0; the loop is design, review, implement and test, landing
review, commit, integrate, quick-test; one branching point; a landing
branch per rung; file ownership per work doc; kill, ASan and stress as
standing gate 3 items for rungs touching the collector, roots, layout or
eval loop; the rerun rule; the tracing order. boil/tools/landing_review.sh
(8c923667245) runs the landing review; its first use, 6d rung 2 on
e03ab939a4a, returned "fix first" with four findings (reviews/
6d-2-landing.md on boil/6d-rung2), being fixed. Step 7a (the
per-interpreter callback and finalizer queue) moved up from step 7 after
finding B killed the process under stress in a shared-obmalloc
subinterpreter's start-up.

Correction (06:30 PDT, from the 10a agent's register commit 6124ebafd70):
the saved-register clears were retired in 10a rung 1 (ce81b2828cd), not
1b; Py_NO_INLINE stays on the helpers by design, since the inline audit
relies on it; and the "inline-fallback collection scans everything" row
was also retired in a3c01b61e52. The release-build test skips go to 10a
rung 2b item 7, or rung 3 where a PyMem or struct root holds the object.

## 2026-10-28 06:58 PDT: 6d rung 2 landed (attribute and descriptor slots on values); gate of the merged head

Landed as c66c69d55cf, the merge of boil/6d-rung2 at cc81ca59f1e: rung 2's
five commits (tp_getattro, tp_setattro, tp_descr_get and tp_descr_set take
and return PyValue; PyMember_GetOneValue/SetOneValue; the object-form
protocol functions as the compatibility layer; gc.get_referrers precise
after the test_taskgroups trace), then the first landing review of the
new procedure (reviews/6d-2-landing.md, codex: fix first, four findings)
and its fixes: ctypes stores save the previous native bytes and restore
them when keeping the new value fails (upstream leaks there; we would
have read freed memory), with a deterministic allocation-failure test
over a struct field, an array item and py_object.value; the promised
collecting-conversion and descriptor-mutation tests and failure
injection at four boxes; measurements redone on clean source at
bb59ea53031 with the integration head f89d8e9c4d3 as the control, build
lines in the JSON; every gate line with its commit.

The agent's quick-test at e03ab939a4a and bb59ea53031: gate 0 both
builds, gate 1 on 31 files, 8,794 tests on each build. Gate run here on
a detached checkout of c66c69d55cf with rebuilt trees: gate 0 both
builds; gate 1 on test_gc, test_asyncio.test_taskgroups, test_descr,
test_capi and test_ctypes, 2,363 tests on each build; gate 2 on default
556/564 files, 51,013 tests, SUCCESS, 54,366 collections, 0 fallbacks;
gate 3 on debug 51,013 tests, SUCCESS, 56,185 collections, 0 fallbacks,
-u cpu test_cext 16 tests, regen and the traverse audit clean.

Measured against the control (the landing's own base): every micro row
lower, boxes per iteration 1 to 0 on the getattr, setattr, descriptor and
property rows, kernels geomean 0.944 with no kernel above +0.3%; against
rung 1 1.017, against the 6d before records 1.006. One attribution
recorded, not acted on: the control spends 9 more instructions per
empty-loop iteration than the branch with identical loop source; the eval
function is 161,349 bytes on the control and 143,577 with the rung, so
it is code generation in the single eval-loop function (step 8's input;
10a's rung 4 write-up cites it). Residues with owners: the super
receiver box (minimum), object.__getattribute__'s box and the
__length_hint__ box (rung 3, done in 04ffa14a62b), the C-mode spans for
the slot bridges (10a rung 2b, hand-off table in 6d-2.md).

## 2026-10-28 08:05 PDT: the kill record keeps every word (10a tooling fix found by 6d rung 3's landing)

Landed as de6f193f905, the merge of boil/10a-record at 44b9107ad1e (one
commit on a3206a397d9). 6d rung 3 commit 4, which removes the
hand-through helper in call.c, moved a stale copy of the killed object
into gc_collect's Clinic wrapper frame, and the kill record named only
the first word it found, so test_verify_missed_root's three subtests
failed on debug while the collector behaved correctly. The record now
keeps up to eight words that reached the object with address, owner pc
and kind (gc_trace.c verify_other_words; note 11 section 8.3), the
retain and kill reports name them all, and the test asserts that
missed_root's precise frame is among them. The 10a agent's gates on the
branch: gate 0 and gate 1 test_gc on both builds, test_gc under kill on
debug, ASan test_gc under kill with detect_stack_use_after_return=0.
Quick-test here on a detached checkout of de6f193f905: gate 0 and gate 1
(test_gc, test_call, test_descr) on both builds, test_gc and test_call
under kill on debug, all SUCCESS. The full gate 2 and 3 run with 6d
rung 3's landing, which carries this merge.

Two findings from the same gates, recorded in note 11 8.2 by the 10a
agent: ASan's fake stacks (detect_stack_use_after_return) hide escaped
locals from the conservative scan, so the ASan line of record sets the
option to 0 and 10a rung 3 decides between scanning fake frames and
keeping the option; and test_import's SinglephaseInitTests is finding B's
second creator path (a shared-queue weakref callback drained in a legacy
subinterpreter's half-initialised thread state), with a new tool,
boil/tools/cstack.sh, for a C stack without gdb.

## 2026-10-28 09:35 PDT: 6d rung 3 landed (tp_call, wrapperfunc and the call layer on values); gate of the merged head

Landed as 7ad194aa883, the merge of boil/6d-rung3 at 555a63c59b3: rung 3's
four commits (callfunc, wrapperfunc and wrapperfunc_kwds return PyValue;
PyObject_CallValue and _PyObject_CallValue take the callable as a value;
PyVectorcall_Call and _PyObject_Call_PrependValue return values; all 27
tp_call functions converted; CALL_FUNCTION_EX calls the value form with
the monitored result rooted across C_RETURN; PyObject_LengthHint reads a
value; lru_cache keys and results as values with known-hash dict value
forms; the hand-through helper removed once 10a's call_effects change
landed), its landing review (reviews/6d-3-landing.md, codex: fix first,
four findings) and the fixes: the prepend allocation-failure test now
proves which allocation fails (a three-call argument through
_testcapi.call_with_nomemory) and requires MemoryError; the lifetime
cases collect in the callee and read every argument and keyword value;
float-result rows in the box audit with before and after budgets against
the a3206a397d9 control; the C-mode record extended by 61 functions.

The agent's lines at 0457f2a3c2c: gate 2 default 51,025 tests, 58,559
collections, 0 fallbacks; gate 3 debug 51,025 tests, 60,573 collections,
0 fallbacks, test_cext, regen and the audit; test_gc under kill and under
ASan (detect_stack_use_after_return=0); stress N=1000 on the stress set
2,121 tests; the inline probe 20 of 20. The kill suite and the 8.2 ASan
list stand from a725b9ab5f2 for every file but test_gc. Gate run here on a
detached checkout of 7ad194aa883 with rebuilt trees: gate 0 both builds;
gate 1 on test_gc, test_call, test_descr, test_capi, test_functools and
test_iterlen, 2,169 tests on each build; gate 2 on default 556/564
files, 51,025 tests, SUCCESS, 58,581 collections, 0 fallbacks; gate 3 on
debug 51,025 tests, SUCCESS, 60,403 collections, 0 fallbacks, -u cpu
test_cext 16 tests, regen and the traverse audit clean.

Recorded, not acted on: object_setattr +25 instructions per call, in the
wrapper value forms on the tp_call path (wrapperdescr_call +34,
wrap_setattr +20) with layout noise elsewhere; pickle +3.3% because
PyDict_Next stops inlining _PyDict_NextValue after the known-hash value
functions joined dictobject.c; the perf-record period that matches perf
stat (1,000,003; 20,000 throttles) is the measurement note of record.
Residues with owners: the second starred-callee box (its parameter half
removed by 10a rung 2b item 4 in d98f6dc8495, the generator's entry-box
rule to step 8); int.__add__'s operand box (6e); super's receiver box
(the minimum). Found while landing: the kill record's one-word limit
(landed separately, 08:05 entry); ASan's fake stacks hiding escaped
locals (10a rung 3); gate.sh's ignore list before the caller's -m made a
probe run the whole file (fixed in 8a52226ee12); the unbuilt modules
can be type-checked against borrowed headers (6d rung 4's tool,
boil/tools/check_unbuilt.sh, now a gate 3 item in PROCESS.md).

Addendum (09:50 PDT): the unbuilt modules type-check clean at the landed
rung 3 head 555a63c59b3 with 6d's check_unbuilt.sh (gcc -fsyntax-only
against borrowed headers): _decimal plain and EXTRA_FUNCTIONALITY, _ssl
with its included files, _hashopenssl, all nine _sqlite files including
connection.c's tp_call adapter, _bz2, the four _zstd files and _uuid; 0
errors, 0 warnings. Unchecked for want of headers: _tkinter, _dbm, _gdbm,
readline. The tool lands with 6d rung 4's docs commit.

## 2026-10-28 12:20 PDT: 6d rung 4 landed (getsets on values; step 6d complete) and audit-2 landed (the traverse audit's blind spots); gate of the merged head

Landed as de1533e8b1a, the merge of boil/6d-rung4 at b551c275607, and
9fe9241bf77, the merge of boil/audit2 at 8f9994d3633, gated together.

6d rung 4: every getter and setter takes and returns a value (465
entries, 425 getter and 108 setter functions; Argument Clinic's @getter
and @setter emit the value signature; the typedefs lose their casts),
with two new micro rows (getset_number 824 to 391 instructions per
iteration, cell_contents 842 to 378), check_unbuilt.sh (the unbuilt
modules type-checked against borrowed headers, now checking gcc's exit
status too), and two defects found on the way: the CJK stream writer's
traverse did not visit its pending text (upstream omits it too and
leaks; here a collection between two writes freed it, test_codecencodings_jp
segfaulted on debug), and test_descr's _fail_each_allocation never failed
an action's first allocation (two rung 2 tests passed only because of
it). Landing review (reviews/6d-4-landing.md, codex: fix first): the
classmethod and staticmethod annotation accessors and
type_abstractmethods still boxed around object-form dict helpers, one
box per operation against the rung's own rule E.0 and unrecorded; fixed
in 80eddd41953 with twenty box-audit cases at 0 boxes and 0 allocations
(18 of them boxed once per iteration before). The boxes in
type_set_module and type_set_doc stay: the audit event takes an object.
Both families converted, so the cut to 6e was not needed; the Clinic
numeric getters left for 6e are listed per module in note 06 section 21.

audit-2: Tools/boil/traverse_audit.py report 3 (members that neither the
traverse nor a release names) triaged: of 16 entries, 5 were tool
defects (const stripped inside a name; LOCAL(void) return-type macros
hiding a function; header accessors; visit callbacks; delegation to a
base traverse), 2 were real (the stream writer above, and the SRE
scanner's traverse did not visit the scanned string or its buffer's
exporter: test_re's test_scanner_keeps_its_string failed 3 of 3 on
both builds before the fix), 9 are borrowed or aliased with the path
that keeps them alive in traverse_allow.txt (exact member paths; a
listed path no longer reported is stale). Report 3 is part of --check.
Landing review (reviews/audit-2-landing.md, codex: fix first): delegation
credited an alias derived from a member (now only the object itself or a
cast of it, until reassigned); the helper walk marked a function at its
first depth so a diamond depended on set order (now breadth first, tested
under six hash seeds); gates 2 and 3 run at the rung head (51,039 tests
each, 0 fallbacks); the audit's runtime 4.4 to 5.0 s and 53 to 60 MB,
attributed to the header index and the wider walk, not tuned.

Gate run here on a detached checkout of 9fe9241bf77 with rebuilt trees:
gate 0 both builds; gate 1 on fourteen files (the getset, codec, clinic,
tools and re files of both rungs) run=4,389 tests on default and run=4,389 on
debug; kill on test_re, test_multibytecodec, test_codecencodings_jp,
test_tools and test_gc 562 tests; gate 2 on default 556/564 files,
51,057 tests, SUCCESS, 59,926 collections, 0 fallbacks; gate 3 on debug
51,057 tests, SUCCESS, 61,839 collections, 0 fallbacks, -u cpu test_cext
16 tests, regen and the traverse audit (with report 3) clean.

Traced at the first gate 2 run of de1533e8b1a alone:
test_perf_profiler.test_pre_fork_compile asserted /tmp/perf-PID.map
existed and it did not; the file passed four of four reruns alone. The
test's tearDown unlinks every /tmp/perf-*.map that was not present at
its setUp, and three other suites were running on this machine at the
time (load average 35), so a concurrent copy of the test in another
checkout deletes this run's maps. A machine-level cross-run artefact
through the shared /tmp (perf requires that path); recorded in
PROCESS.md's standing concerns, not excluded.

Hand-offs made today: the Py_buffer release skipped for a Python
exporter when the consumer dies in the sweep (audit-2's review; 10a rung
3, acceptance: exactly-once __release_buffer__ with a retained exporter
and an allocating callback); the symtable's PyMem roots (10a rung 3);
the remaining checker limits (coordinator, an audit-3 pass after 7a
rung 2); the rung 1 fork-child hang (6d-1.md's hazard record shows where
the parent waited, not why the child hung; owner 7a rung 2 with the fork
path, evidence required the hung child's native and Python stacks, on
recurrence). The remote branches boil/6d-rung4 and boil/audit2 are left
in place like the earlier landing branches.

## 2026-10-29 05:45 PDT: takeover audit and resumed plan

Integration remains d7f3003874f before this documentation change; no new
runtime landing is claimed. Three read-only agents audited branch
ancestry, saved gates, reviews and the remaining roadmap. Compiler head
bf6635fd0a4 and queue head a0338fd7634 both have completed fix-first
landing reviews, now preserved here. Root head b471a1bb9bc includes the
checker and watcher fixes but lacks a landing record and final checks.
The prior status prose overstated completion; README and PROCESS now
separate implemented from landed work. No full gates were rerun by this
audit. Remote head freshness was not established (SSH configuration and
sandbox network access prevented the read-only check).

The user authorized parallel implementation, plan subdivision, faster
debug feedback where useful, and interpreter parity before the JIT.
PROCESS 0.4 assigns the three resumed lanes, splits later queue/root
work, adds H3's orthogonal-dispatch inventory and checkpoint I, and
records hourly documentation/status updates. Section 3 permits an O1
debug configuration only after root/walk/control qualification. Existing
debug builds use Og; no faster configuration has been qualified yet.
Note 14 incorporates the second design review's response as a design
specification; its required dynamic evidence is not a passed result.

### 05:50 PDT: verifier worker support and shared prerequisite

All three implementation lanes have started. ROOTS needs active-mode
assertions in regrtest workers, so levels 2 and 3 now accept the single
optional --python worker-command pair; other extra arguments fail before
a run, preserving full-suite coverage. Shell syntax and both rejected-
argument cases were checked. ROOTS owns the runtime worker demonstration
and will retain its launcher source with the gate record.

QUEUES is isolating an actual-state verifier getter and focused test
before its larger fixture fixes. Once gated, the coordinator will review
and integrate that independent prerequisite so ROOTS can merge it from
integration; this is not a merge between step branches. No queue-runtime
landing is implied by the getter. The documentation commit 7af7ce89618
was pushed to the existing integration branch after recovering the user's
explicit prior authorization to commit and push as needed.

## 2026-10-29 06:00 PDT: shared verifier-state prerequisite integrated

2155f16f45f is the isolated cherry-pick of QUEUES 0f8a019b846, five files
with the read-only getter, test hook, contract test and its record. The
coordinator read the patch and COMPILER independently reviewed it with
no blocker: it reads parsed interpreter state, and the isolated-child
control distinguishes that from the unchanged environment. No queue
routing change is included. Integration remained frozen at 2155f16f45f
while its existing builds ran gate 0 and the focused gate 1: release
1,924 tests/50 skips in 44.9s; debug 1,924 tests/20 skips in 2m58s;
mode test 1/1 on each, including the debug kill/extra combinations.
Logs and exact start/head record: build/coordinator-20261029/shared-hook-*.
Source-lane unbuilt-module syntax checks also passed. No full root-safety
suite is claimed for this test accessor; ROOTS will use it in its final
verification gates.

ROOTS's prior checkpoint at f8baf84affb passed default/debug/tailcall
smoke sets (1,924 each) and checker/walk targets (29 each); ASan's only
failure remains the recorded test_buffer_overflow. It is preserving Og
flags and qualifying O1 in its existing debug build before the final
suites. COMPILER's guard-removal phase controls now fail exactly in the
two dict and six MRO cases; current code passes the new phase oracle.
Unexplained benchmark rows remain owned by checkpoint I, and the existing
box metric's kernel/micro coverage is stated explicitly rather than
silently expanded to memory workloads.

### 06:15 PDT: gate startup cleanup ownership

QUEUES observed two gate-start exits with empty logs, then normal runs
under shell tracing. The shared-pyc find/prune was the only identified
startup command with errors hidden under set-e, but the original failing
command was not captured; the exact cause remains a hypothesis. Remove
that cross-run prune: it can race with another run's cleanup and its
age test cannot prove ownership or inactivity. Each gate's trap still
removes its own prefix. A startup banner now names level, build and head
before commands run. Shell syntax and diff checks pass; existing targeted
integration gate1 will exercise the changed startup path. No test failure
is dismissed by this tooling repair.

### 06:25 PDT: compiler merged; qualification and dispatch boundaries

Independent source/artifact review returned LAND for compiler b59ab985ae7.
The author preserved that normalized review and the three prior blocker
dispositions in documentation-only ab18af7d778. Coordinator merged it as
99435658509; detached integration validation is now building new default
and Og debug configurations from /tmp/boilgate, with outputs under the
project's build/integration-default and build/integration-debug. The merge
is not yet claimed as validated. The startup-tooling focused gate1 passed
on integration90773db0ffd, exercising its new banner and local cleanup.

The roots O1 trial cut smoke time from3m41s to2m37s and passed checker,
compiled walk, active worker-mode and100/100 stale-root controls. One
focused test_verify_kill case lacked the required PyMem-holder attribution
despite an active kill:pymem state. Qualification stopped; the owner is
tracing fixture storage versus collector attribution, without changing
the oracle. Og remains the integration debug configuration.

QUEUE's fixture fixup a6d88557dfa and clean lane970e14edbde retain failing
delivery/readiness controls and actual mode assertions. Module10/11 remap
differences are now explained by owner-unlink order with event traces.
The feeder-finalization hazard remains unclosed while bounded diagnostic
attempts and current end gates run. No new queue runtime has landed.

ROOTS supplied the H3 inventory. The compiler owner now drafts H3-1's
generated handler domain before any new typed operation; existing256
assumptions survive inside the16-bit IR. Typed float truth is a bounded
follow-on, while frame passing, call-window placement and compare/branch
need their own root/liveness or encoding proofs. Interpreter parity stays
a measured pre-JIT checkpoint.

The push of existing integration/compiler/queue/roots refs was rejected by
automatic approval review: recovered session history was not accepted as
authorization to export repository contents. Explicit approval for the
same GitHub destination and fast-forward payload is pending; committed
local work and validation continue. No workaround push was attempted.

### 06:37 PDT: cleanup lock and explicit service authorization

The user confirmed Loom is the internal session server and explicitly
authorized Loom use and GitHub pushes. README is now handover revision5;
the committed integration/compiler/queue/roots refs pushed successfully
to the existing repository. The prior export approval block is closed.

QUEUES found a build-lock lifetime bug in gate_run.sh: command-form flock
released the lock before sweep, allowing cleanup to kill the next run's
workers. The coordinator keeps fd8 locked through sweep and only sweeps
from a signal trap after acquiring the build lock. Slot-before-build
ordering is unchanged; failure status is preserved. Three process-level
checks cover cleanup ownership, a terminated waiter and nonzero command
status, with a fake pkill so no real test worker is targeted. Five runs
pass. The old runner fails the cleanup invariant; removing only the new
ownership guard fails the waiter invariant. An initial harness attempt
signaled before the flock child finished exec; synchronizing on its comm
fixes that test race. Exact controls/logs are in build/coordinator-20261029.

Detached compiler integration at99435658509 passed gate0 default/debug
(1,924 each), gate1's16 modules (1,783 each), debug kill's20 copyprop
tests with active mode assertion, and release gate2 (51,081 tests,
60,014 collections,0 fallbacks). Og debug gate3 remains in progress.
These runs use the prior runner; no queued same-build overlap occurred.

## 2026-10-29 06:45 PDT: first hourly coordinator update

Compiler ab18af7d778 is landed and integration-validated at 99435658509.
Detached release and Og debug suites each passed 51,081 tests, with
60,014/61,865 collections and zero fallbacks. Gate3 completed at filing:
cext 16 tests (7 skipped, 59.3s), unchanged regeneration and traversal audit
all passed. Shared cleanup
fix e02e177903f passed independent QUEUES review, its three process checks
five times and the existing mode-contract gate1; it is pushed. The binary
for that last focused run was built at 99435658509, the runner at e02.

QUEUES release at 970e14edbde passed 51,079 tests/62,173 collections with
zero fallbacks. It merged integration as e37fa1f966f, retaining both
additive test-extension blocks byte-for-byte through their insertion
conflict. The new common-deadline feeder wait is under validation: real
old teardown warns while sentinel shutdown is completing; real fixed
teardown waits and completes; a deliberately blocked feeder still warns.
The historical warning's exact state remains unknown. Combined full and
verification gates precede review; no queue-runtime landing is claimed.

ROOTS diagnosis at d65a03fc96c proves precise-carrier-first provenance
loss in both retain/kill. Source logging was restored. Its proposed
post-B enrichment keeps ordinary marking unchanged and handles aliases,
nested carriers and cycles, with capped unique origins. Review also
caught an empty-seed sorting invariant and per-origin hash-clear cost;
the fixture proposal was revised to cover them. The agent's inherited
approval context rejected the concrete fixture twice as insufficiently
authorized high-impact work. The bounded repair/fixture approval is
pending; no workaround or rejected mutation executed. O1 remains
unqualified and final roots gates remain outstanding.

H3-1's domain design is preserved in integration and independently under
review. It covers generators, typed IDs, monitoring storage, stats,
inspection and sampler version/schema. Producer renumbering follows sink
widening. Review is checking actual target-to-sampler schema transport;
same Python version cannot establish matching handler numbering. No H3
source work is authorized before the design response. Typed float truth
follows; H2-dependent compare/branch and call-window work stays separate.

Next hour: complete/review combined queue gates,
resolve roots fixture approval and validate provenance repair, answer H3
review, then assign the unfinished executable verification manifest when
an implementation slot frees. README/PROCESS and Loom status reflect
these boundaries. Next hourly update due about 07:45 PDT.

### 07:00 PDT: handler-domain design accepted; stress witness gap

H3-1 round1 returned REVISE on actual-target schema transport, pristine
construction/runtime-state distinction and misleading threshold exports.
The coordinator ruled explicit threshold removal and sampler protocol
compatibility changes. The author revised the design; independent round2
returned IMPLEMENT, no remaining design blocker. Both source/artifact-only
reviews are preserved. First implementation stage preserves emitted IDs;
consumer/transport closure precedes renumbering and its pyc magic change.

ROOTS's manifest design found that parsed verifier state does not reveal
stress_every. Coordinator source review also found gc_alloc_in_reftracer
sets stress to1 then0 inside the test_gc worker, silently disabling an
ambient stress interval for subsequent tests. QUEUES is preparing an
isolated read-only getter and restoration fixture outside its frozen tree.
Current normal gates continue; complete stress11 coverage is not claimed
until this prerequisite is fixed. Earlier source-restored queue controls
remain scoped evidence, rather than proof of uniform worker stress.

### 07:10 PDT: permission context synchronized; three active implementations

The user's fresh steering identified the held roots fixture and directed
work to continue. The existing ROOTS agent then received the current
unrestricted developer permissions and applied the same reviewed v2 patch
successfully in its own worktree. No executor switch or indirect mutation
bypassed the rejection. The prior hold is closed; normal validation and
the old-helper control are now running. O1 remains unqualified.

H3 now works on branch boil/h3 in the reused compiler worktree, with the
compiler landing ref frozen. Its first regeneration matches all seven
baseline products byte-for-byte. Stage1 preserves IDs as a migration
invariant; the final design removes inherited category cutoffs and uses
explicit generated semantic properties. Synthetic-fixture collisions are
being traced rather than weakening duplicate validation.

QUEUES d43c8b4dd32 passed combined default/debug full suites, 51,102 tests
each, zero fallback, plus debug cext/regeneration/audit and default probes.
Its standalone stress prerequisite's old-zero-reset control fails with
(1000,0); the getter and save/restore positive gates are running. Remaining
verification gates wait for that shared prerequisite. Manifest discovery
resolved 23 selector rows to31 test IDs across11 physical modules; the
registry/parser remains unfinished, and no execution claim is made.

The copy-propagation semantic handoff is closed by landed99435658509 and
its validated probes/phase controls/counter record; its row is removed from
PROCESS0.3. Specializer-checker and precise folded-reader proofs remain
separate open handoffs to ROOTS.

### 07:30 PDT: stress prerequisite validated; H3 stage1 review

Integration02bcd2f6132 imports the independently reviewed shared stress
getter/restoration59790b545bcc. Default/debug smoke passes1,924 tests each;
state tests3 per build and debug kill2/stress1000 two pass. Logs and exact
commands are in build/coordinator-20261029/stress-state-*. These focused
coordinator runs are not lane-wide actual-worker stress attestations.
Integration02bcd, QUEUES5576cdbd755 and H3e7cc59743e1 are pushed.

H3 stage1's independent review clears the generator ownership refactor,
all seven byte-identical generated products and retained negative-control
evidence. It requires a saved control-runner fix: variant writes precede
lock acquisition and restoration follows release. Historical quiescent
runs remain valid, but the replay script must not expose broken live
source to another gate. The author is fixing that artifact before landing.
Stages2–4 and runtime domain closure remain separate unfinished work.

ROOTS's provenance fixture is running in the synchronized permission
context. O1 also exposed a genuine list-to-tuple lifetime bug: the source
list is cleared before tuple allocation, losing its elements' roots.
The bounded reorder moves forward from3a into2b. The PyMem assertion is
being corrected to require its origin on the same killed-list report line
whether first or supplemental; old-helper failure remains required.
No O1 adoption or ROOTS landing is claimed.

## 2026-10-29 07:41 PDT: second hourly coordinator update

QUEUES remains frozen at 5cc31bacee1. Full debug kill passes 51,104 tests,
65,601 collections and zero fallback; 3,764 unpinned words are reported,
not a suite failure. The explicit 14 ASan inputs expand to 24 test files:
3,589 tests/90 skips pass in 6m43s with only the established exact overflow
case excluded. The original unfiltered ASan smoke remains FAIL. Three
debug subprocess runs pass 371 each. Stress11/probes and landing review
remain; no queue-runtime landing is claimed.

ROOTS provenance/list-transfer fix 82165f8a2a9 is committed and reconciled
with integration as 0b13048a633. The list reorder passes ten fresh children;
the original traps ten of ten. The corrected origin assertion accepts the
fixed supplemental-origin report and rejects the retained old-helper
report. Moving the zero-seed return before sorting fails both fixture
orderings. Merged O1 bounded checks pass: smoke 1,924, affected modules 186,
plain-kill focused 51, checker 41 files/five negative controls, compiled
inline audit and stale100 off/kill with 705 collections each and zero
fallback/unpinned. Actual driver/worker records agree with required
states. The first full 31-case/11-module iteration is running; its cost and
remaining 19 iterations precede an O1 adoption decision and final gates.

H3 stage1 code cleared independent review. Its corrected saved runner
uses copied generators/inputs, verifies copied module paths and hashes in
actual driver/workers, has a timeout and removes its scratch tree. The
positive seven tests and all seven deliberately broken variants meet their
oracles; live Og debug generator tests pass 62. It explicitly handles
regrtest's platform-reporting auxiliary call. Final commit and integration
checks follow; stages2–4 remain open. Stage1 is independently landable
tooling with byte-identical products, not proof of runtime domain closure.

Registry/validator/plan-display stage1 landed as fff4e0e9b08 after coordinator
review. Seven safety-control groups and actual normalized case inventory
validation pass; 15 profiles are defined. Review narrowed the ASan filter
to preserve five passing same-named allocator tests and distinguished the
raw discovery log from normalized inventory. Execution is not implemented.
ROOTS will prepare launcher contracts first, then orchestration/gate hook,
without delaying its runtime qualification. Integration and ROOTS heads
through fff4/0b130 are pushed; no permission hold remains.

Checkpoint I must bind both C and standard-library sources to frozen
upstream/fork revisions. Retained baseline 15e96bf639d differs from
upstream 321925c92c9 only by a worklog, but its configured source path is
the active fork. This binding must be established before new parity
measurements; historical records are preserved. Next hour: finish queue
stress/probes and review, integrate H3 stage1, obtain ROOTS matrix cost,
and qualify the manifest launcher. Next hourly update about 08:45 PDT.

### 07:45 PDT: H3 generator stage1 merged

Independent rereview of b203defe758 returned LAND_STAGE1: copied sources,
actual driver/worker path/hash checks, bounded runner, preserved negative
controls and unchanged live snapshots close the saved-runner finding.
The verdict is retained in reviews/H3-1-stage1-landing-round2.md. Merge
6c724c29408 integrates stage1; all seven generated products remain
unchanged. Detached gate0/affected gate1 on both existing integration
builds plus regeneration/product equality are now running from
/tmp/boilgate. No final integration pass is claimed yet.

H3 stage2 begins fixed-number runtime, monitoring, stats and inspection
consumers. ROOTS retains gc_trace/gc_live/test_gc/C-checker/shared CAPI
ownership; H3 supplies its bounded scalar fixture as a handoff. Sampler
transport, renumbering and new operations remain later stages. The roots
matrix in0b130 and queue validation in5cc are untouched by this merge.

### 07:55 PDT: H3 merge validated; final roots source scheduling

Detached H3 integration at6c724c29408 passes release/Og debug gate0
(1,924 each) and affected gate1 (790 each,3 skips). Regeneration/Clinic
leave source unchanged; all seven product hashes equal the stage1 base.
Commands/logs are build/coordinator-20261029/h3-stage1-*. The final
regeneration was also replayed under an explicit1200s timeout. No full
runtime-suite or wide-domain closure is claimed for this tooling stage.

QUEUES5cc passes stress11:3,046 tests/65 skips in4m58s. Exactly11 worker
PID pairs have start/end interval1000, verifier(0,0), exact selected names
and matching frozen source/config hashes. All40 probes and subprocess
371 three times pass. Evidence commit1f2a7ac7b88 and reconciliation5c4124ea9ff
preserve all native sources and generated products; affected generator
checks precede review. Historical normal/full-verification lines retain
their original commits and scopes.

ROOTS feedback iteration at0b130 takes541.745s:31 individual cases and
11 full modules/2,018 module tests. Its lifetime assertions and85 actual
process state pairs pass. It emitted no per-job root stats, so it cannot
replace final matrix counter coverage. Two individual test_finalize cases
(forkserver/spawn) return SUCCESS but emit a leaked-semaphore warning and
SemLock cleanup FileNotFoundError. Registry clearing may lose a weakref
callback before tracker shutdown; this remains a source-based hypothesis.
ROOTS will trace it on queue-reconciled source before a cleanup fix.

The remaining19 iterations are held for settled native source. The final
harness may run whole modules concurrently in one -j8 driver, preserving
each module's full internal order and separate individual cases, with all
identity/count/mode/counter evidence. Measure it before projecting cost.
Planned native landings: QUEUES1, ROOTS2b, H3 runtime stage2; parallel
implementation continues. ROOTS reviewed H3's disjoint scalar C fixture;
coordinator grants only that include/function/method-entry block, with GC
fixtures remaining ROOTS-owned. No O1 adoption or matrix waiver is claimed.


### 08:25 PDT: queue rung1 landed; roots cleanup cause confirmed

Independent round3 review cleared6f416bc4582 for owner routing, with its
historical normal/full-kill/ASan/stress scopes preserved. Merge052d8342e56
passes detached release/debug smoke1,924 each, queue tests22 each and
actual-worker-kill22. Regeneration/Clinic leave source unchanged. Evidence
is build/coordinator-20261029/queue-rung1-*. The first coordinator kill
launch omitted BOIL_MODE_EVIDENCE and failed before running any tests;
that harness failure is retained separately. Corrected launch records
actual worker(2,0). Review: work/reviews/7a-1-landing-round3.md.

ROOTS merged queue integration as cb0b6c21b97 and reconciled the opaque
queue-body classification to run_queue_ex, leaving its thin wrapper
precise. Bounded checker/compiled/debug/kill checks pass. Causal traces
confirm that test_finalize clears the finalizer registry, loses semaphore
weakref cleanup, then restores a finalizer after tracker shutdown unlinks
the semaphore. Existing finalizers will run after forkserver stop and
before tracker stop. Actual-helper regression and old-order/forkserver
controls precede the settled-source repeated reclamation matrix. No O1
adoption or matrix waiver is made.

H3 stage2a is frozen at624c75f6455: smoke1,924 per build, affected974 per
build and strict actual-kill974 pass. Ten worker witnesses and38 process
root records show808 collections/0fallback/0unpinned. Independent review
is running; monitoring/stats/inspection remain separate work. Tooling-only
3dff5879df8 corrects retained launchers to call worker.main(), preserving
regrtest's cwd and exit timeout. Fresh135-test controls pass on the stated
H3 draft source; historical records are not relabeled. Coordinator review
is complete; isolated integration follows before ROOTS final qualification.

The frozen manifest launcher cleared LAND_LAUNCHER_ONLY review. It checks
fixed launch forms, actual process state and scheduled worker identity;
source snapshots, full reconciliation, counters, repetition and gate hooks
remain unimplemented. The queue2 design/probe agent was stopped by an
automatic cybersecurity classifier; even its read-only handoff failed.
The only supplied reason was possible cybersecurity risk. No native queue2
fix was applied; artifacts remain preserved and stopped work is not retried
through another executor. Other lanes continue. Next hourly report08:45 PDT.


### 08:30 PDT: launcher and shared worker setup integrated

Launcher2db451b1b65 cleared independent LAND_LAUNCHER_ONLY review at the
recorded final hashes. Retained launcher correction3dff5879df8 is isolated
as integration093b9d2a1c3. Both change only boil tooling/artifacts; native,
standard-library/test and generated sources match052d8342e56. Primary093
and H3 checkpoint624c75f6455 are pushed. The ROOTS launcher already enters
worker.main() through runpy and did not share the bypass defect.

Queue-owner-routing is promoted to available with052 landing/review/gate
evidence. This does not attest an arbitrary selected binary. Inventory
rediscovery is still required after test-source changes. Fifteen combined
registry/launcher controls pass. Promotion exposed the CLI control's
assumption that stress11 would always be unavailable; it now supplies a
controlled pending prerequisite, preserving its failure oracle. The first
edit missed its json import; that control error was corrected before the
passing run. No profile orchestration or gate-hook claim is made.

ROOTS cleanup repair passes three targeted tests off and kill, including
the actual-helper ordering regression and natural forkserver/spawn cases,
without leaked semaphore or double-unlink diagnostics. Retained old-order
control fails in both contexts. Its source record, reconciliation and
parallel module-iteration measurement follow before the full matrix.


### 08:31 PDT: bounded handler and roots repair reviews cleared

Independent LAND_STAGE2A review of624c75f6455 confirms unchanged numeric
IDs/base semantics, shared pristine admission and reference ownership,
checked scalar construction/rewrite, recorded hashes and strict-kill
counters. The bounded verdict is retained in reviews/H3-1-stage2a-landing.md.
Native integration still follows ROOTS2b; monitoring/stats/inspection,
transport, renumbering and final H3 gates remain open.

Cleanup/queue-annotation repair172b406be0c also cleared bounded review;
its four files are unchanged by tooling merge67477efa986. This is not
ROOTS2b final clearance. The actual-helper regression directly checks
child stderr. Earlier quiet natural-case passes do not prove clean
successful stderr because -W hides worker output; visible-output reruns
are in progress before any such claim. This limits the08:30 statement.
Source/gate evidence remains with its original revision.


### 08:36 PDT: pre-JIT handoffs reconciled; faster matrix measured

A read-only dispatch/performance audit found active handoffs still assigning
interpreter deficits to steps8/9. PROCESS, README and notes06/12/14 now
assign frame passing/numeric guards/build policy toH1, inline receivers
to6e, handler domain and exact-float truth toH3, call-window/starred-call
boxing toH3 with H1/H2 alias/liveness/spans, and range-spill attribution
beforeI. Coordinator must commission the string/compiler design with a
valid uniqueness/escape proof. Historical measurements and rejected-rope
rationale remain intact. Scalar-loop lowering and compare/branch encoding
are not approved by this reassignment. Intermediate rungs record results;
checkpointI still requires matched release performance before any JIT.
No opcode-count target or benchmark-specific fusion is introduced.

ROOTS timing-only whole-module iteration atf1f93c6f0e2 completes in109.008s:
11 complete modules,2,041tests/69skips,12 actual-state process pairs,
439 root records/16,467collections/0fallback/3,007unpinned. Full source
snapshot remains unchanged. Exact scheduled/returned PID/config/seed and
all69skip events are retained; coordinator independently confirms no
reclamation selector is skipped. Report-only unpinned attribution and the
final exact count/skip contract remain required; this is not full20 proof.

Before repeated qualification the coordinator found the shared-heap
callback source skip still present despite queue1 landing. ROOTS must
close its existing item7 handoff, or document the precise failure/ruling,
before final source freeze. No20-iteration run has begun. Visible natural
cleanup controls now pass off/kill; three inherited asyncgen warnings in
the whole-module log remain visible and are not filtered. H3's next design
splits names/inspection, monitoring, statistics and remaining widths into
separate checkpoints, with first-scope independent review now starting.


### 08:44 PDT: hourly coordinator checkpoint

Landed runtime remains queue1 merge052d8342e56, with detached integration
checks complete. Shared launcher2db451b1b65, worker setup093b9d2a1c3 and
pre-I ownership corrections eeb96dfc543 are integrated/pushed. No new
native change in primary is claimed. H3 stage2a624c75f6455 cleared bounded
landing review; integration follows ROOTS2b. Its next design d387aca48a9
cleared IMPLEMENT_2B1 and names/inspection/NOFB implementation is building.
ROOTS/coordinator approved only the exact additional get_ir selector hunk.
Later monitoring, raw statistics, widths, transport and renumbering remain.

ROOTS sourcef1f93c6f0e2 includes cleanup172b and shared tooling. Current O1
checks pass: source41/0, five negative controls, compiled audit, stale100
off/kill705collections each/0fallback/0unpinned, affected216/3skips and
focused-kill52/1skip. The restored shared-heap regression passes stress
and kill+stress;20 separate stress selections per build are still running.
Final repeated31-case/whole11-module qualification has not begun; the
source skip must close before freezing its expected count/skip contract.

The109.008s timing run has439 interpreter records across367physical PIDs,
not439physical processes. Its3,007unpinned hits occupy73 nonzero rows
across17PIDs:2,984 non-main-interpreter hits and23 main-interpreter hits,
including the deliberate counter fixture. The scan/swap/dump functions
match the earlier0b source. Existing detached-main-state attribution stays
with3c; no zero-unpinned suite claim or new exception is introduced.
Eight real-evidence replay corruptions are rejected: missing scheduled
worker/result/end, positive narrowed count, changed config, absent counters,
fallback and skipped protected selector. This is replay evidence; final
contracts use current discovery and protect the restored B case as well.

The manifest run/gate-hook design now explicitly separates selected build
source (/tmp/boilgate for integration), active primary tools, actual Lib
module origins and build receipts. Snapshot hashes alone do not prove
compiled-binary provenance. No run/hook implementation is claimed.

Next hour: finish source-skip closure and final matrix contract/controls,
start settled release/debug qualification, reach H3's first bounded runtime
checkpoint, and review the manifest source/build prerequisite. Debug O1
remains qualification work, not adopted policy. Queue2 stays held by the
automatic classifier with preserved artifacts; no stopped probe is retried.
Next hourly documentation/user report is due about09:45 PDT.


### 08:54 PDT: ROOTS source settled; next handler oracle exposed a bug

ROOTS8ce9c57c662 retires the shared-heap source skip locally. All40 fresh
stress selections pass (20release/20O1debug), zero skipped, with80 paired
actual driver/worker witnesses and child state/end-stress assertions.
Kill+stress also passes. Precommit evidence bindsf1 plus the exact retained
patch; its test_gc hash equals8ce. Primary keeps its source-skip row until
ROOTS lands. Current discovery finds2,041 cases. Final contracts distinguish
68 debug versus83 release intrinsic skips, with32 protected selectors.
Replay controls now reject zero collections and protected subtest skips
in addition to missing/substituted/partial evidence.

Coordinator approves final release/O1debug qualification on frozen8ce;
O1 is not adopted as shared policy. Precise-entry-paths availability may
change only in a hash-bound qualification registry copy with current
implementation/checker/control evidence, not by claiming its matrix has
already passed. Original registry, exact diff and qualifying scope remain
visible. No other pending requirement changes. Build-owned adapter results
must not be labeled verify.py run/gate-hook execution. First current
individual/full-module qualification per build precedes the remaining20.

H3's real NOFB monitoring oracle found inherited missing CALL events:
the shared body compared a derived raw ID against its semantic instruction.
Coordinator authorizes the generic uses_opcode assignment to inst.name;
raw targets/dispatch and existing stats remain unchanged. Regeneration
changes exactly two SUPER_ATTR body assignments. Missing-event baseline
is retained; CALL/C_RETURN/C_RAISE and collecting callback controls are
required, along with exact twin/field restoration. The oracle is not relaxed.

Manifest identity/result design is approved for a separate v2 prerequisite,
not the run/gate hook. It adds explicit source/runtime/tool roots, build
receipts, actual module origins and result/skip witnesses. Existing
/tmp/boilgate at clean052 and integration-debug are reserved for a later
reviewed bounded receipt rebuild; no source checkout/reconfiguration is
authorized. Host controls and exact build plan review precede that step.


### 09:18 PDT: frozen matrices active; handler checkpoint reviewed

Primary native integration remains052d8342e56. ROOTS8ce9c57c662 is frozen:
the shared-heap regression passes20 fresh stress selections per build, and
final20-iteration release/O1debug qualification is running. First whole11
pilots pass2,041 tests each,68 debug/83 release skips, with none of32 protected
selectors skipped. Contracts bind exact scheduled/returned workers, state,
configuration, seed and concrete skip IDs/reasons;11 rejection controls
include protected subtests and zero collections. No completed matrix claim.

Current8ce ASan14 passes3,616 tests/91skips, actual driver+14workers in
kill mode(2,0), stress0:342 interpreter records/162physical PIDs,
16,712collections/zero fallback/3,478 report-onlyunpinned. Its exact accepted
allocator exclusion remains visible; unfiltered gate0 retains its original
AddressSanitizerTests failure. Asyncgen/coroutine and file warnings remain
in the evidence. Fresh release/tailcall stale100 controls pass all six cases,
705collections/zero fallback/unpinned per build. Checker/compiled-walk29cases
pass each, release1skip/tailcall0,296collections/zero fallback and one
deliberate counter-test unpinned hit each. Two targeted harness failures
(flag placement, uv PID indirection) remain recorded; corrected v3 passes.

H3c5cf6ba6d288 clears independent LAND_2B1: full-domain names/properties,
disassembly and exact NOFB restoration, including the inherited SUPER body
CALL-event bug. Candidate smoke1,924 per build, affected889 per build and
actual-kill889/nine workers pass;37root records/764collections/zero fallback
or unpinned. All55 source and8 build-product hashes match. This is bounded
checkpoint evidence, not final H3 closure, committed-head reruns or a native
integration. Runtime landing remains after ROOTS2b.

H3's next draft separates immutable safe-line lookup2b.2a from coherent
monitoring storage2b.2b. Independent2a design review confirms the actual
detached frame->code->linetable roots and requires explicit synthetic-word
behavior, ordinary-position equivalence and exact shared-hook approval.
No2b.2b implementation is authorized. A separate H1 frame-passing design
is commissioned; no dispatch-count threshold is introduced.

Coordinator independently qualifies the separate controlled052 build receipt
for v2 component controls. Exact clean/build/startup/smoke commands pass;
6,188 pre/post source inputs are identical and105 current build outputs match.
Smoke1,924/20skips produces162 interpreter records across59physical PIDs,
1,314collections/zero fallback/226 report-onlyunpinned. Actual source/runtime
origins, GCC15.2/Py_DEBUG1/Og configuration and compiler/log hashes match.
Ten optional missing modules remain explicit. Original unreviewed receipt
is unchanged; qualified copy and coordinator review are separate. No full
profile, O1 adoption or current-ROOTS evidence is inferred. ROOTS release
matrix was paused only between gates for the receipt and has resumed.

For remaining ROOTS full gates, use the frozen actual-driver/worker adapter
under normal locks/slots/timeouts/freshcache after exact command review;
preserve full gate2/3 semantics, with cext/regen/traversal separately. Shared
verify.py orchestration/gate hooks remain unimplemented. Final v2 component
controls/review remain. Queue2 classifier hold is unchanged; no stopped work
is retried. Next hourly user/documentation checkpoint remains about09:45 PDT.


### 09:22 PDT: remaining full-gate policy and bounded H3 scope

Reviewed ROOTS's build-owned remaining full-gate plan. Full release, normal
debug and kill debug preserve the complete default suite and sole existing
remote-exec exclusion, exact actual worker cohorts and positive/zero-fallback
counters. Candidate O1 full suites may follow its repeated qualification;
global gate-policy adoption waits for complete evidence. Cext16, regeneration
status/content comparison, traverse and optional-extension checks remain
separate. No new shared gate hook is inferred. ASan sort/functools kill-only
completion is approved to cover stress-set modules absent from ASan14.

Coordinator approves the once-only full-release stress100k qualification
policy with the complete module roster and only the four existing stress
selectors plus the remote-exec selector. No test_misc/B/ASan exclusion.
Actual driver/workers must retain mode(0,0)/stress100000; source/helper/native
bindings, complete results,1200s worker/7200s outer ceilings and30-minute
checkpoint are required. Preserve a qualification-only registry diff promoting
full-stress-policy and extending only those existing exclusion IDs. Frozen8ce
and the running matrix registry remain unchanged. A successful filtered
profile does not close writer/compiler rooting debt; no run has begun yet.

H3 corrected monitoring design33cef3ae191078a1 and exact additive safe-lookup
hook are approved by ROOTS/coordinator. A one-line private-symbol export in
pycore_code.h is also approved for the actual test extension. Immutable2b.2a
implementation begins; no storage2b.2b implementation is authorized. Separate
H1 frame-passing/checker/build design617bea5 is preserved with no execution.


### 09:34 PDT: full harness cleared; component receipt correction

ROOTS full-harnessv2 plan c9e05359d7afd3a1 clears coordinator review after
closing absent/empty PASSED statistics and protected case/subtest/setup/module
skip gaps. All20 negative replays and accounted class-skip/normalized-name
positives pass. Both564-module rosters and helper hashes match; first plan62988
and original helpers remain preserved. Approved full profiles still wait for
each build's20 repetitions. No full run or source/helper change to the running
matrix is implied. ASan sort/functools completion separately passes359/1skip,
actual driver+2workers kill/stress0,124collections/zero fallback/one report-only
unpinned. ASan14+2 cover the required module union as separate runs.

The v2 component's first attempt failed before native execution because the
host harness created pycache in the strict frozen tool directory. Retained
retry disables those host writes. Its four fresh native discoveries pass;
the first actual off control then rejects the extension-origin inventory
before executing the test. Original105-output receipt records symlink names
but omits67 actual extension target contents. Coordinator's initial receipt
review missed that binding gap; the original qualification is superseded,
with both original receipt files unchanged. No component positive is claimed.

Approved repair binds each in-build alias and its resolved target bytes,
rejects outside targets, and adds meaningful target-mutation/link controls.
Independent check confirms67 unchanged aliases/targets, exact hashes/inodes/
sizes/ctime/mtime; latest mutation16:10:00.623679UTC predates original build
completion16:10:23UTC. No intervening build/output writes are recorded. A
new manifest and separately reviewed post-build supplement are required;
no redundant rebuild is authorized while that evidence is assessed.

Coordinator also found cext's instrumented renderer omitted -j while adding
--python, which regrtest rejects. Approved correction explicitly uses one
isolated module worker (-j1), the same16 cases/cpu resource/no filters, and
canonicalizes its short scheduled name for actual-origin checks. This differs
from ordinary sequential gate3 argv; numeric gate behavior remains unchanged.
No frozen launcher changes occur during an active attempt, and no failure is
relabeled as passing. H3's bounded safe-line qualification continues while
the component's replacement receipt/tools/plan are reviewed.


### 09:45 PDT: hourly coordinator checkpoint

Primary native integration remains052d8342e56. ROOTS8ce is unchanged and
both final matrices progress:8 complete release iterations and7 debug of20
each, with no validated failures/fallbacks. Release's recent pause in results
was test-slot contention, not a stopped host/native hang: compiler component
held one slot, ROOTS debug the other; release resumed forward progress.
The component has now released its slot. Owner estimates, contingent on
restored capacity, release completion10:10–10:20 and debug10:45–11:00 PDT.
These are scheduling budgets, not interpreter performance measurements.

Full-harnessv2 c9e05359d7afd3a1 is cleared after20 negative replays and explicit
fixture-skip/protected-case accounting. Both564-module rosters match. The
release-full/stress11-release host waits for default's validated640-job
completion. Full suites remain serialized; debug full/kill follow debug20
as candidate O1 qualification. O1 is not globally adopted. ASan14 passes3,616
tests and separate sort/functools359 cover its required union; original
unfiltered ASan failure and exact accepted exclusion remain visible.

H3 immutable line checkpointdc02f76c0ccf clears independent LAND_2A after
release/Og smoke1,924 each, affected1,519 each and actual-kill1,519. Source56,
products8 and copied controls match;156 root records/156PIDs give1,498collections
and zero fallback/unpinned. Earlier names/inspectionc5cf also remains cleared.
No H3 runtime merge precedes ROOTS2b. Storage2b.2b preparation is design only:
hard membership controls must not prohibit documented spurious DISABLE events,
and local-mask OOM/retry behavior plus exact primitive hooks need review.
H1 frame-passing design remains preserved; no H1 implementation began.

Shared v2 component's corrected frozen batch completes0 on controlled052.
All seven positives pass: exact case off/kill,203-case module order with5 exact
source skips, two separate repetitions, cext16/7 exact source skips with one
isolated worker, and auxiliary kill. All seven isolated negatives return their
expected failure/status markers;11 actual-evidence corruptions are rejected.
Final full source/tool/172-output identity postflight passes. This is component
qualification only; no named/full profile or run/gate hook is implemented.
Final source/evidence review precedes landing. The initial105-output receipt
and failed/unexecuted harness attempts remain superseded and preserved.

Next hour: finish matrix qualification as capacity allows, begin approved full
release work, close the v2 component review, and review monitoring storage's
bounded design/grants. Commission the separate alias-safe pre-I string design
after the component checkpoint. Queue2 remains held by the earlier automatic
classifier; no stopped probe is retried and no user approval is pending.
Next hourly documentation/user report is due about10:45 PDT.


### 09:48 PDT: verification identity/result component landed

Primary9dd24bf42c4 lands the bounded v2 identity/result prerequisite after
coordinator LAND_COMPONENT review. All105 indexed artifact hashes, final
source/test/document hashes and complete source/tool/172-output identity match.
Coordinator independently reconciled all six raw driver cohorts from concrete
discovery, exact source skips, scheduling/results, origins, XML and counters.
Seven positives, seven isolated expected failures and11 actual-evidence
corruptions pass their contracts;36 host controls pass. Batch ends16:45:01UTC.
Old helper/receipt/harness failures remain retained; no committed-head native
rerun is claimed or needed for this boil/tools-only landing.

Primary native sources, production registry, gate.sh, gate_run.sh and worker
shell adapter remain unchanged. Native integration is still052. No full-suite,
ASan or named profile coverage, orchestration or numeric gate hook is inferred.
The cleared H3 runtime checkpoints remain behind ROOTS2b. Both ROOTS matrices
continue on8ce without source/helper changes.

The component agent now owns design-only
work/pre-I-linear-string-concat.md: alias/escape proof, frame/monitoring/f_locals
and C-data lifetime constraints, failure behavior and linear-growth acceptance.
No string code/test/benchmark/build work begins before design review. H3's
storage design remains with its owner; no other lane is merged during gates.


### 10:08 PDT: monitoring storage review and string design

All three agents remain active. At10:06, ROOTS release has16 complete matrix
iterations/525 validated jobs and O1debug10/349 of20/640 each. Zero fallback
in completed jobs; release-full host waits for its validated completion.
Source8ce and frozen matrix/full-v2 helpers are unchanged. No full run began.

H3's eight authorized bounded children on dc02 confirm valid-AST failures in
the old line codec: backward lines corrupt/drop release events and abort Og;
INT_MAX also aborts Og. Immutable locations agree before monitoring. The
single proposed replacement is fixed int absolute lines plus uint16 originals.
Source/layout memory forecasts are explicit; actual new allocations are unrun.
Coordinator records the private API/layout decision before implementation.

Initial storage design2738f4d3/fixture1bf0f9cd needs revision. Coordinator found
two omitted active decoder calls in legacy_tracing.c; ROOTS found two pointer
METH_O hooks incompatible with this fork's PyValue method ABI. Owner is fixing
the inventory, bounds, ownership and exact hooks. No storage implementation or
shared-hunk grant is issued while independent review continues.

The separate pre-I string design1616f77f is frozen for review, with mutation
disabled in its proposed first rung. Its author now audits Unicode capacity
storage/ABI and certificate placement in a separate document. No string native,
test or benchmark work is authorized. Queue2 classifier hold is unchanged.
Once-only tailcall/retain recommendations are assigned to10a4/H1, without new
ROOTS2b blockers; retain needs its own reclamation semantics.


### 10:18 PDT: release matrix complete; storage implementation cleared

ROOTS release completes all640 jobs (620 individual,20 complete-module):
41,440tests/1,660 intrinsic skips,9,141 interpreter records,330,157collections,
zero fallback and42,466 report-only unpinned hits. Completion receipt
default-complete.json SHA256 d68e077bd9d63a41ce9df0bb1f742d0061f5ce65ba9207fa63cd2b04b5dc8362
binds plan17a8ba83. Coordinator independently checks that receipt. Every job
validated frozen source/native/helpers before/after;8ce remains clean.
Release-full starts17:16:13UTC under c9e full-v2, actual(0,0)/stress0.
No full-suite outcome is claimed. O1debug continues through repetition13.

Independent ROOTS and coordinator clear corrected monitoring storage design
84e518cb872e and exact CAPI proposal71ec68cb7da7 as IMPLEMENT_2B2B. Five decoder
callers/bounds, cached-None/synthetic initialization, proper VARARGS ABI and
stack snapshots close the initial review findings. The exact additive include,
three helpers and three entries are granted only on H3 with coherent storage.
Old rejected proposal/probes remain preserved; other shared fixtures are untouched.

Owner begins the bounded fixed-int/uint16 representation, generated canonical
relations and prepare/commit implementation. First failing-control checkpoint
is due within45minutes; full implementation estimate2–3hours before qualification.
No native integration precedes ROOTS2b. String capacity/ABI audit continues
without native changes. Next hourly report remains about10:45 PDT.


### 10:25 PDT: full-suite observer repair; old monitoring controls

ROOTS release-full reports native SUCCESS:51,130tests/3,458skips,564 returned
modules (94 whole-module skips,8 resource-denied),4m11. Qualification then
correctly rejects an unaccounted skip count; no result receipt or stress11
run follows. The original attempt, helpers and c9e plan remain preserved.
Two independent agents trace the mismatch to inner TestResult objects used
by meta-tests: doctest4/15, support3/6, unittest2/48, zipimport_support0/7
(reported skips/raw addSkip events), exactly67 excess events overall.
No native failure is established and no qualification PASS is claimed.

Existing rows lack result identity, so re-audit cannot prove exact outer
membership. The reviewed repair direction uses separate full-only helpers,
captures the actual scheduled test_func result before stats conversion, and
binds raw events with monotonic weakref-backed result tokens. This avoids
address reuse and retaining inner results. Exact outer counts/skips and all32
protected-selector checks remain; no >= rule or per-module exception.
Draft helper/plan/control review precedes one fresh full rerun. The four
matrix helpers and native8ce stay frozen while debug continues.

H3 design/evidence commit878647973ba is pushed. Six bounded old-control children
finish on release/Og: same-event and cross-event additions both lose tool A's
existing INSTRUCTION sites; local setter OOM raises but changes requested
mask0 to96. All are semantic assertion failures on both builds. Native
implementation now begins under the already reviewed bounded grant.

String storage audit73fae302 is retained with its frozen parent. It identifies
explicit Unicode flag initialization/accounting, writable-writer/resize
exclusions, ASCII-to-Latin-1 header changes, permanent metadata after seal,
and pre-CREATE certificate publication. This is source evidence, with no
string native execution or implementation approval. S1 shadow tracking is
a development checkpoint, not an independent production overhead landing.


### 10:45 PDT: hourly coordinator checkpoint

Primary native integration remains052d8342e56; identity/result component9dd
is landed, but production run/gate orchestration is not. ROOTS source8ce stays
clean. Release's20 repetitions complete640jobs/41,440tests/1,660skips with
330,157collections/zero fallback. The independently checked main0/domain
supplement binds all640 inputs and1,480 required PID occurrences. O1debug has
16 complete repetitions and is finishing17; remaining matrix estimate is
about18–22minutes from10:41, contingent on shared slots, not a speed result.

The first full native SUCCESS remains qualificationFAIL because global
addSkip included67 inner meta-test events. Separate full-only v3 repair14739d
clears independent review after exact outer-result/configuration bindings,
weakref-backed tokens, protected skip checks and main0/counter-domain checks.
Four observer controls and45 validator negatives plus positives pass. The
original failed attempt/helpers and earlier proposals are preserved.

Fresh release-full native+audit passes51,130tests/3,458outer skips,564modules,
565 actual driver/workers,63,597collections/zero fallback;2,889unpinned visits
are report-only. Coordinator verifies receipt195c64bee179667a. Stress11-release
also passes3,061tests/100skips,11modules,44,828collections/zero fallback,6,519
report-only unpinned; receipt68feb053300b33f7. Thirteen driver visits remain
unattributed, rather than being labeled interpreter swaps. No filter is added.

Debug's guarded host waits for640/17a8 plus its read-only main0 supplement,
then only normal-full, kill-full and stress11 with candidate-after20. O1 is
not adopted globally; integration debug remains Og. Fullstress100k follows
the debug fulls. Cext/regeneration/traversal/optional closure, an explicit
idle benchmark window and landing review remain. No native integration date
is promised before those results.

H3 implementation is underway at8786479 plus frozen production diff2245622b /
source manifest603366f4. Development controls pass both builds for valid
backward/INT_MAX lines, existing-tool membership and local OOM rollback/retry.
Early450-case gates pass; broader/smoke qualification is still in progress.
Real allocator interception measures equal-n low/high requests[28,6,6],40bytes
peak; failure prefixes free all unpublished bytes and preserve state. This
is requested-byte evidence, not RSS or final-candidate qualification.

Independent source review finds one remaining blocker: no-monitoring-flags
code now loses configured local-request bookkeeping. Coordinator rules to
preserve the old request while keeping installation absent. A valid code.replace
negative is retained; fix/refreeze and affected reruns follow current gates.
Prior hashes/results stay distinct. Synthetic-cache/copied-helper controls
and final allocation/kill/source review remain before landing behind ROOTS2b.

String design and storage audit are retained as source analysis only. Capacity
bit/trailer, code certificate backing and H1/ROOTS observer interfaces still
need decisions before implementation. Queue2 remains held by the earlier
automatic classifier; no user approval is pending or stopped probe retried.

Next hour: finish debug20 and start its approved full chain; close H3's bounded
bookkeeping finding and qualify the refrozen candidate. Keep shared full
orchestration's outer-result handoff explicit. Next report is due about11:45 PDT.


### 10:49 PDT: corrected storage source cleared; H1 readiness review

H3 refreezes the no-monitoring-flags bookkeeping correction at source manifest
54b2832334af6148e6852799333361e1e089ffc6fc7642dbec92503ab84084a9 and full patch
7c64784ac0481248cdf88f5d381da355d028246a008615531768c8af8a0c907d.
Independent peer verifies all12 file hashes and the bounded two-file delta,
and returns SOURCE_REVIEW_CLEAR. The valid code.replace regression preserves
configured request/tool version while leaving installation absent. This is
source clearance, not landing approval;603 smoke/affected evidence is historical.
Final affected/kill and isolated controls run under the new source identity,
with versioned measurement copies preserving earlier outputs.

The compiler reviewer now audits preserved H1-1 for the smallest frame-passing
experiment, exact transitions and checker/compiler attribution prerequisites.
No H1 code/build/test/benchmark work is authorized by that read-only assignment.
Final H3 evidence review takes priority when available. Root maps, clearing
stores, build/frame-pointer policy and string certificate choices stay separate.

At10:48, both test slots were in active H3 jobs while ROOTS debugi18case07
waited normally. Active jobs finish untouched; H3 then uses one lane and yields
the other to ROOTS, preserving landing-critical progress. No source/gate policy
or filter changes. The hourly schedule remains11:45 PDT.


### 11:04 PDT: eligibility prerequisite and bounded H1 readiness

H3's54b broad gate fails after1,262 completed tests because executing a valid
CO_NO_MONITORING_EVENTS code object after a global monitoring-version change
loops at RESUME. The2s pre-edit probe independently reproduces the old failure
with its marker and stack. Preserve both failed records; this is inherited
behavior exposed by executing the stronger regression, not a reason to weaken it.

Independent review clears eligibility proposal-v3 d5d88a7b8f80: guard refresh
inside the existing RESUME version mismatch, return early from direct refresh,
and guard the cold monitor_handled callback. The exact ceval.h hunk is granted.
No version acknowledgment or shared static-code write. Ordinary callback,
flagged on-stack/request-state, generator.throw and separate no-local-data
handled-exception controls accompany the fix. The owner applies/regenerates and
freezes combined native13-file manifest
 a6c07b4ce7b2932a82c2bc8bcc187f8940359fe1674951f075ee1b98773e8557
and production patch
 b209836201b0fbdc9356dae76ec1fbe4a2e8fc259acc702be025ccfe6503abc6.
Combined-candidate native qualification starts; no independent prerequisite
landing or runtime success is claimed. Earlier storage source clearance and
measurement identities remain historical.

At11:00 ROOTS debug has599/640 jobs and18 complete repetitions with no validated
failure/fallback. The guarded matrix-to-main0-to-full chain remains armed.
Reserve its test lane: H3 submits one transaction at a time, with no same-build
waiter consuming the second slot before acquiring its build lock.

The compiler peer clears H1-1 bounded frame-passing design readiness and records
its exact transition/interface/compiler-root controls in the new review. It
needs an integrated baseline and explicit H3 generator/build handoff before
implementation; H2 maps and frame-pointer policy are separate. No H1 source or
build is started. The peer now inventories ROOTS landing receipts and remaining
gates read-only, avoiding duplicate native runs. Queue2 remains held by the
prior automatic classifier; no permission question is pending. Next hourly
report remains11:45 PDT.


### 11:09 PDT: debug matrix complete; first combined eligibility pass

ROOTS frozen8ce debug completes620 individual selections and20 whole-module
runs:640jobs/41,440tests/1,360allowedskips,374,245collections, zero fallback,
65,072report-onlyunpinned. Summary81d65e028d1a and640-row results5a561e5b2ac6
match the independently read hashes/counts. The completion marker contains only
jobs/global plan17a8 and is byte-identical across builds by design; each build's
summary/results/native bindings supply the distinct identity. Read-only debug
main0 supplement07d7f4455aa3 passes all640jobs/1,480requiredPIDoccurrences.
The unchanged guarded armer starts debug-full at11:08:13, then kill-full and
stress11 only after native+audit success. O1 shared policy remains unchanged.

Coordinator accepts a bounded ASan14 provenance deviation: its retained plan
has actual worker source/Makefile/sys.version/origin/state but lacks pre/post
native-byte inventory. The later2-module completion manifest's271 native paths
match current bytes, and the peer checks file/alias/target ctime+mtime: latest
15:56:27.980870UTC precedes ASan14 start16:07:58.320878UTC. Source stays8ce and
there are no intervening build/config writes. This operational bridge supports
existing bounded evidence without a redundant rerun; it is not retrospective
cryptographic proof. Preserve the limitation, require stronger manifests for
future runs, and reopen only on an identity conflict or source change.
Repeated full rootfile PID/interpreter keys remain separate finalization events;
no unsupported exact fixture attribution or lifetime deduplication is claimed.

H3 first combined release probe passes at11:05. Coordinator reads both receipts:
identical probe4dee9c0f235f and outere2333a581754; old7c child exits1 after2s
watchdog, combineda6c child exits0. Before/after18old and19new inputs match and
both log hashes verify. New binaryc96ddb788f3c records actual optimize0/debugTrue,
verifier(0,0)/stress0. Suppressed/on-stack/generator paths complete without
callbacks; ordinary code gets17 instruction sites and handled callback[0,6],
while suppressed no-local-data state remainsNone. Broader gates remain pending.

Gate4 is explicitly step-end under PROCESS section3; do not turn earlier
'pending measurements' shorthand into a new full benchmark rung blocker.
Retain rung-specific as-built attribution and later checkpoint-I obligations.
No loaded-machine measurements start. Next hourly report remains11:45 PDT.


### 11:12 PDT: debug full exposes thread-pool lifetime blocker

Active debug-full reports fork and forkserver test_threads failures in
WithThreadsTestPool.test_release_task_refs: three CountedObjects remain
weakref-live at _test_multiprocessing.py3442. The test deletes its input list,
waits for threaded cleanup and calls support.gc_collect before asserting task
arguments/results are released. Native8ce/helpers remain unchanged. Preserve
the unsignaled full run and let its armer stop on failure; no kill/stress launch,
new exclusion, weaker oracle or optimization rollback is approved.

ROOTS traces the actual lifetime read-only before proposing a bounded fix.
Its640-job matrix remains valid for its selected process-pool cases; it does
not establish thread-pool coverage. The independent landing inventory records
NOT READY and the exact remaining gates, plus accepted ASan provenance limits
and the step-end gate4 distinction. No runtime integration is approved.
H3 progresses independently: combined release smoke1,924 and affected1,370 pass,
with allocation/synthetic/request/global-OOM controls. Copied-helper compiler
setup failed under default GNU23, not a semantic negative pass; the untouched
failed logs/helper are retained. Its versioned successor matches production
C11/assert context without changing copied bodies or native source. Final
controls and debug/kill qualification continue. Next report remains11:45 PDT.


### 11:15 PDT: combined H3 source cleared; ROOTS diagnostic boundary

Independent review96e493f68d59 clears H3 combineda6c/b209 source after verifying
all13 live hashes, binary diff and exact reverse of the approved three guards,
including both generated expansions. The new executing-code, generator.throw
and no-local-data handled-exception controls preserve the intended oracles.
The copied-helper C11/assert-context correction clears review: all positive
compiles/runs succeed, and token/line narrowing plus raw-alias variants fail
named semantic assertions. The earlier compiler setup failure remains failure.
This is source/control clearance; final debug/kill/allocation/restoration gate
evidence remains separate and native integration stays held behind ROOTS2b.

ROOTS full now reports the same thread-pool task-reference failure in fork,
forkserver and spawn. Python worker/result loops already clear task locals and
support.gc_collect performs three collections; no forgotten-test-collection
claim is supported. Owner prepares a bounded diagnostic, with independent peer
review of observer-created reference/collection effects, before source edits.
No debug diagnostic is queued behind the active same-build full lock. Source,
helpers, failed full evidence and exact original assertion are preserved.
Next hourly report remains11:45 PDT.


### 11:28 PDT: task-root owner identified; H3 candidate preserved before union

ROOTS debug full finishes with51,130tests/3failures/3,401skips, exactly the
fork/forkserver/spawn thread-pool release-task-reference failures. Failure
artifactfb74bc3f79da records nativeRC2/armerRC1,565actual driver/workers,
66,950collections/zero fallback,4,009report-onlyunpinned and post-run unchanged
source/native/helper bindings. Coordinator reads it independently. No qualified
receipt exists and no chained kill/stress/full100k run starts.

Independent peer clears bounded diagnostic18ffce654859, preserving the original
failed comparison and a single pre-entry-prefix stack-hit snapshot before later
inspection. Ten unmodified fresh fork-thread selections fail identically; the
separate trace fails with zero dropped records. All three survivors have direct
conservative stack hits at offset0 in the176-byte _PyParkingLot_Park frame,
PCdebug/python+0x36b2fa, stack-hit IDs16–19. These IDs are not asserted to be
exactly the test's three collections. Later frame locals remain supplementary.
The owner audits disassembly and all park-argument/callback/caller contracts
before a bounded fix. No production edit, blanket precision annotation or
symptom-specific word clearing is approved yet.

H3 storage code76274ed06ec and evidenceffc548603b8 are clean/pushed on boil/h3
and boil/h3-monitoring-storage-land. Source a6c/b209 is unchanged; release and
Og smoke1,924 each, affected1,370 each, actual-kill1,370 with11worker.main
witnesses/839collections/zero fallback and unpinned all pass. Allocation, valid
AST, synthetic lines, OOM/retry and copied semantic controls pass both builds.
Regeneration/Clinic, frozen IDs,13marked-file C checker, traversal and19optional
syntax checks complete; four header-unavailable modules remain untested.
The earlier runpy audit-launcher import failure and corrected launch remain
separate. Final evidence review proceeds on immutableffc, not a new native run.

Ancestry corrects the coordinator's proposed safe tooling merge: common ancestor
with primary64eb is6c724c29408, so H3 lacks queue052 native/test changes, not just
9dd tooling. The merge was held before any action. Explicitly defer the normal
primary merge until ROOTS2b settles, preserving the existing landing order and
avoiding duplicate union qualification. Frozen H3 receipts qualify their own
source only; final native union review/gates remain required. The owner may
write stage2b3 raw-stats/handler-pair design only; sampler transport is separate,
and no next H3 native work starts. H1 still waits for integrated generator/build
handoff. Next hourly report remains11:45 PDT.


### 11:31 PDT: frozen storage evidence clears; parking padding isolated

Independent FROZEN_STORAGE_CHECKPOINT_CLEAR atffc548603b8 is recorded in the
new H3 evidence-readiness review33169a5e2981. It reconciles240indexed artifacts,
13changed/46unchanged source inputs,7generated products, both native/config/CAPI
identities, actual-kill worker/main0 counters, all short controls and exact
requested-byte accounting/restoration. Allocation failure sizes remain unmeasured;
no RSS, high-canonical execution, full suite, ASan or merged-runtime claim follows.
The coordinator reads the full record. No detached review tree is created while
the native-union base remains unsettled. H3 proceeds only with raw-stats design.

ROOTS disassembly isolates each direct survivor hit as uninitialized alignment
padding at rsp0/rbp-a0 in a176-byte _PyParkingLot_Park frame. Timeout begins-a98
and wait_entry-a80; no instruction uses the padding word. The owner audits all
seven callers, callback arguments and producer-to-parked-consumer result
anchoring before proposing a single-function precise classification. No pad
zeroing, walkrange heuristic or blanket precision is approved. The compiler
peer is assigned that forthcoming bounded design review. Next report11:45 PDT.


### 11:45 PDT: hourly coordinator checkpoint

Primary native integration remains052d8342e56; shared identity/result9dd is
landed but run/gate orchestration remains unfinished. H3 frozen storage
native76274/evidenceffc548603b8 clears independent source/evidence review,
including240artifact hashes, release/Og smoke1,924 and affected1,370 each,
actual-kill1,370/839collections/zero fallback and unpinned, allocation and
semantic negatives, regeneration/checker/traversal and19optional syntax checks.
The candidate predates queue052. Preserve its branch and receipts; primary
union merge/requalification remains behind ROOTS2b. No merged LAND is claimed.

ROOTS8ce release/debug each finish640jobs/41,440tests, with zero fallback;
fresh release full/stress11 pass. Debug full remains FAILURE:51,130tests and
exactly three thread-pool task-reference failures. Ten original fresh controls
and the separate trace identify all three survivors in unused Park alignment
padding. The seven-caller/handoff audit clears a bounded precision annotation,
header contract and structural fixture; every caller stays opaque. Future
queue-reader precision still needs an explicit output-slot anchor.

At the quiescent boundary ROOTS merges primary64ab asd8be2e2c36d:27incoming paths
all underboil,1084native product paths unchanged, old verifier bytes and8ce
receipts preserved. Park implementation-v1 changes only its approved four files.
Peer finds incompatible METH_O declaration before fixture execution; Boolean
pointer-bit layout may conceal it, so no crash is claimed. ProperPyValue
argument/result/cast correction is authorized. Release gate0 fails its inline
audit because inlined Semaphore_Wait's inactive Windows DWORD cast is not in
the parser type inventory. PyTime_t was already recognized; preserve that
refinement and failed gate. Exact extension grants only DWORD plus realistic
cast-positive/unanchored-object-negative TestRootCheckC controls. No branch
omission, effect exemption, noinline workaround or outlined allowance.
Refreeze/review and first bounded controls are expected around noon, contingent
on current builds; no new full profiles or source-qualified success is claimed.

Independent stats design review first returns REVISE for double-counted family
arithmetic and unrecoverable truth-event attribution. Docs-only81417aff097
closes both and clears round2 IMPLEMENT readiness. Original draft/review history
stay preserved. Raw family totals sum entries once; semantic events retain
recorded owners. Computed-goto/switch prediction bypasses the counter, whereas
tailcall re-enters and counts; producer dispatch-mode identity and separate
expectations preserve both. No statistics native/build work starts yet.
H1 frame-passing design is also ready after integrated generator/build handoff;
H2 maps, numeric guards, typed truth, value receivers, call-window placement,
range spills, string concatenation and interpreter parity remain pre-JIT work.

Next hour: close Park fixture ABI/parser findings, prove old-fail/new-pass with
actual frame/caller-slot and omitted-root controls, then rule on the smallest
sufficient fresh qualification plan. Old matrix rows remain historical rather
than new-native PASS; no arbitrary count threshold, new exclusion, Og rollback
or full gate4 benchmark is introduced. Keep H3 candidate frozen and design
handoffs ready. Queue2 remains held by the prior automatic classifier; no user
permission question is pending. Next hourly report is due about12:45 PDT.

At the report boundary, DWORD old-fail/new-pass controls succeed, but forcing
the complete Semaphore_Wait source exposes a second parser limitation: a
CPP-spanning else. The owner proposes a three-scalar-statement selection flag;
that production-body edit is not yet granted. Compare parser support and exact
feature/error-path equivalence before ruling. Preserve failed attempts and
continue checking every platform branch. This refines the pending audit work;
it is not a runtime success or a new exclusion.


### 11:48 PDT: structured semaphore selection approved

The coordinator reads exact proposal688badc429a7 and independent source
assessment, then grants the three-scalar-statement timed-wait refactor in
parking_lot.c. A bool records whether monotonic wait was selected; it is set
after sem_clockwait returns even on error. The realtime wait runs only when
not selected. Feature absent/false therefore uses realtime once, feature true
uses monotonic once, and negative timeout still uses sem_wait once. No new
errno write/callback/root effect or platform-backend change is introduced.

A proper parser continuation solution would stitch partial if/else across
macro alternatives and duplicate common AST tails; that broader analysis is
not replaced with directive skipping. The permanent explicit-selection source
form must pass copied actual-old/new-body feature×timeout×error controls,
old-form-parser failure/new-form success and the inactive-branch root negative,
then full source and linked inline audits plus native builds/fixture tests.
No runtime success or native Windows coverage follows from the proposal.
Combined refreeze/peer review remains required before final qualification.


### 11:59 PDT: Park source clear and first real old-fail/new-pass

V3 six-file patch94d68f30d309 and mapb82a7c2a14a1 clear independent source
review. The reviewer checks every current hash/diff, properPyValue fixture
ABI, DWORD-positive/exact-root-negative controls and structured semaphore
hunk. The36unique semantic combinations/72old-new observations agree on wait
kind/count, clock/deadline, result/fatal and errno, including stale EIO success.
Both native gate0s and linked audits now pass1,924tests (50release/20debugskips).

The real Park fixture passes debug off and actual kill, checking the exact
caller slot and pinned capture across unpark. All three unchanged thread-family
reclamation selectors pass off; representative fork also passes actual kill.
This is the first actual old-fail/new-pass after10/10old8cefork failures.
The first off fork receipt records1test/0skip,77collections/zero fallback and
unpinned. Ten-per-family off/kill repetitions remain in flight; no full/matrix
launch is approved from these first successes alone.

Independent CLEAR_CONTROL_SOURCE coverscf6d4740ae2e/plan686f9f4e: isolated
process-local real-Park CORE override, exact typed ABI/FDE/origin, same positive
fixture's class assertion failure, cleanup before unregister and restored
positive classification. Prior wrapper's subtest-continuation issue was found
before execution, not a native failure. Uniform failfast and explicit loaded
CAPI membership close it; source/binaries stay unchanged. Normal planbdbddc1
binds6,322source entries/271native paths per build and separate immutable helpers.
The class control and restoration have not yet been credited as runtime PASS.
Final source-qualified evidence and post-fix qualification policy remain open.
Next hourly report stays12:45 PDT.


### 12:13 PDT: bounded Park proof complete; fresh qualification scope ruled

Summary632414b6 records71 bounded targets/71tests/0skips,142 actual driver-worker
occurrences,184 finalization records and5,857collections/zero fallback/unpinned.
All three original thread-pool regressions pass ten fresh off and ten actual-kill
selections each on O1-debug. Default originals, exact-slot/omitted-caller fixture,
detached-register and DWORD controls also pass. Runtime reportdb936 verifies
class-only CORE override fails the same fixture, cleanup precedes unregister,
and restored precision passes. This is bounded repair evidence, not a final
matrix/full PASS or O1 adoption.

The coordinator reads amended proposal4b87878d, preservingca275v1, and explicitly
carries unchanged legacy31/11 evidence forward at8ce using body correspondence
42350872, seven opaque caller contracts and the new Park controls plus required
fresh full chain. No old receipt becomes new-head20/20; no general reuse rule.
Each fresh default/O1-debug plan instead requires20 independent selections of
each three thread regressions and BOTH eval probes, plus20 complete orders of
the three thread modules and test_gc:120 transactions/build. Inventory supplement
e14c supplies39 protected selectors;501 listed four-module cases are not runtime
counts. Freeze/review first actual iteration per build before the remaining19.

Changed generated-cases suite runs once per default/debug/debug-kill. Fresh
strict stale100/compiled checks, ASan14+2 kill/N0 with prospective native guards,
tail bounded support, release full/debug normal+kill, stress11 both, unchanged
filtered release full N100000, cext16/7 exact skips and bound optional closure
remain. Exact plans require review. Compiler peer now reviews the policy/plans;
owner closes bounded extras and clean fixup/source bridge first. The first stale
extra wrapper fails because its added stdout mode witnesses violate the existing
six-line oracle; all six native stale100 probes report100/100. Preserve that
harness failure and move witnesses to stderr before a refrozen bounded retry.
No full profile or matrix has launched. H3/H1 native union handoff still waits;
next hourly report remains12:45 PDT.


### 12:20 PDT: clean Park fixup pushed; scope review and string prerequisite

ROOTS commits/pushes1bfe1a57145. Postcommit bridge604ef3fb binds the same six
implementation/test inputs and542 unchanged release/debug product paths;
only two work/design documentation hashes differ from the bounded snapshot.
Fresh strict six-case stale100 passes release off, debug off and actual debug
kill. Compiled checker/walk coverage passes31 tests per mode with its existing
tailcall-required skip; deliberate test_unpinned_counter accounts for one
unpinned event and does not weaken stale100's zero rule. Failed stale wrapper
attempt stays retained; corrected stderr witnesses preserve its native oracle.

Independent qualification scope reviewbcc3b44e agrees with the exact historical
carry-forward,39 protected IDs,501 listed cases and120 transactions/build.
The first iteration means five fresh individuals plus one four-module job per
build, not just the module order. Exact new clean-source plans and actual pilot
receipts still need review. The prior compiler reviewer thread was evicted at
the four-thread limit; the independent stats reviewer takes over bridge/plan
review using the preserved history. No pilot/full execution has started.

String review72b53832 returns REVISE before S1: define concrete certificate
transfers/joins/exception/final-word admission and activation/slot retirement,
then choose one immutable owner. It finds no current copying-path FIX_FIRST
defect. The coordinator assigns a docs-only S0 revision to a separate author,
directed toward dedicated code-owned pure-data metadata installed before
CREATE, preserving lazy exec and actual-deallocation lifetime. H3/code-layout
and H1/ROOTS interfaces still require review; no native or capacity change is
granted. Keep later S2/S3 closure separate. Next hourly report12:45 PDT.


### 12:28 PDT: first repaired-source pilots pass; string S0 ready for interfaces

Independent review40a38bae clears exact pilot26a55, adapter1d624 and14-file
manifest00e230 after all6,322 source/542 native hashes, sourceclean1bfe and
eleven host corruption controls. The coordinator launches only six transactions
per build. Both pilots pass and STOP; all ten individual selections run1test
with no skips. Four complete modules run501tests,116release/101debug skips,
four PASSED each. Per-thread module120/33 and test_gc141/17release or141/2debug
are measured, not assumed from listing. Existing process/thread-inapplicable,
cpu/walltime resource, free-threaded GC/debug-kill/tailcall skips are preserved
by exact ID/reason. None covers a protected selector.

Root reads all twelve current receipts directly: exact1bfe/26a55 provenance,
RC0 and pre/post source/native guards agree. Module results6a1745f2/e69d7fd0
record11,609/13,369collections and zero fallback;2,133/3,254unpinned remain3c
report-only. Release module33.098s, whole iteration51.203s; debug97.033s and
144.224s, with no concurrent support build. Nineteen more at that load estimate
16.2/45.7minutes, not a benchmark or a promised finish. New nineteen-only plans
must bind immutable pilot iterationzero, exact observed contracts and current
source/products/helpers. No remaining repetition, full profile or support
build launches from the pilot ruling alone.

Separate string author supplies focused S0 f3a94cf3, preserving firstea38 draft.
Independent round2 review5d333c2f clears S0 readiness for cross-owner interfaces:
concrete conservative joins/transfers/exception and final physical-word checks,
one code-owned pre-CREATE plan, explicit new complete decoder requirement,
activation/local retirement and separate fresh-allocation grant versus checked
single-use private continuation. Root's continuation finding is closed without
adopting arbitrary old pointers. H3 layout/decoder and H1/ROOTS lifetime interfaces
still require review; no S1/native/layout/storage/mutation grant follows.
The coordinator reads the revision and both review rounds. Next hourly12:45 PDT.


### 12:45 PDT: hourly checkpoint; repetitions progress, ASan audit prerequisite

Primary native integration remains052d8342e56, ROOTS clean1bfe and H3 live-docs
81417aff097/frozen-storageffc548603b8 are pushed. Root checks remote heads.
Park source review,71 bounded checks, class negative/restoration, strict stale
and compiled controls pass at their recorded bindings. Both six-job pilot
cohorts (twelve jobs total) pass under26a55, with five1/0 selectors and four-module501/116release
or101debug. Independent c2908399 clears nineteen-only0d84/9ace/d8fb with exact
pilot Counters, all39 protections,48 immutable pilot paths and114-journal
completion replay. Both continuations run; latest owner snapshot81/114release
and35/114debug pass,13/5 complete new module orders, zero fallback and no
source/product/count/skip drift. Root's earlier direct snapshot71/29 agrees.
These counts exclude the separately accepted pilot; no new full PASS exists.

Support make-only review first finds missing post-make configuration identity.
V1 artifacts7e257/302 stay retained. V2 exact Makefile/pyconfig checks and four
named host drift negatives close it;71340975/a6f4ba2b/091c6213 clears review
3d9d2634. Root launches ASan make only, with prospective complete new products
captured under normal slot/build lock before qualification. It stops BUILD_FAIL
after73.901s at the actual linked audit: newly precise Park calls outlined
_PyRawMutex_Lock and Unlock, unmarked static wrappers. MakeRC2/wrapperRC1;
source/config/default-debug guards remain PASS. Log27b2a2c8 and build9c7dfe4b
stay failures, with no smoke/test coverage. This is not the old allocator oracle.
Tailcall does not start. Owner and independent peer inspect raw native-only
wrapper/slow/platform ownership; exact two-marker proposal34b5e35e/designfb291817
is unapplied. No effect waiver, outlined allowance, forced-inline workaround,
source edit or retry is granted. Finish current frozen repetitions; decide
copied controls and source/native reconciliation from that review, without
automatic redo or reuse. Full plans stay source-only until final source settles.

The coordinator reads H3 preflight062da730: five shared native/test paths merge
textually, but precise monitoring/root spans, args-iterable signature, CAPI ABI,
checker cases and generated expansions need actual union review. Queue052's13
non-boil paths are12 native/test plus one C-API document. Keep H3ffc frozen;
union waits for ROOTS landing and needs fresh gates/primary landing review.
Prospective serial handoff is storage union -> statistics -> H1 -> remaining
transport/renumbering, so complete handler counts and real tailcall stats
baseline precede the calling-convention experiment. This schedules ownership,
not a new H1 technical dependency or native grant. Matched nonstats release
timing remains H1 benefit evidence; op<256 currently omits higher entries,
not a proven memory-corruption claim.

String S0f3a94/review5d333 stays ready for cross-owner interfaces, without S1,
layout, storage or mutation approval. Baseline audit6bd23aca confirms retained
baseline source/stdlib config resolves into the active fork; old runner results
lack full import/native identity. Preserve old artifacts and the clean main
checkout. Plan a separately owned exact321925 detached source/build and new
result identity, with matched and stock-policy cohorts distinguished; no new
checkout/build/interpreter/benchmark ran. Bound runner updates before I.

Next hour: finish frozen repetitions, close raw-wrapper design/controls and
choose the narrow source-qualified continuation; freeze fresh full/kill/ASan/
stress/extras plans afterward. O1 shared policy remains pending required full
evidence. Queue2 remains held by its prior automatic classifier, with no
permission question pending. Next hourly report about13:45 PDT.

At12:47 independent7fcca37c clears the exact raw-wrapper design for two
precision markers, with all ten static-Bucket calls, semaphore platforms,
caller-owned roots and default-reentrant slow calls audited. Root reads the
full review and grants only task-copy source/compiled oracle controls while
1bfe repetitions continue. Real header edit/rebuild still waits for quiescence,
control outcomes and a reviewed native bridge/affected rerun plan. No extra
marker, slow-body edit, effect exception or support retry is covered.


### 13:00 PDT: release twenty complete; raw controls and transition ready

Release completion63235055 re-audits114 fresh receipts/journals at1bfe/0d84;
with the immutable pilot it completes120transactions/20iterations,
10,120tests/2,320 exact recorded skips and zero fallback. New nineteen alone
record230,148collections/40,464report-only unpinned. Root reads completion
and fresh progress directly: debug71/114 new jobs pass,11new whole-module
orders, no failures/fallback; no debug completion yet. Full profiles stay held.

Task-copy raw controls2d80/a6c pass: actual complete header definitions/bodies,
exact held-Slow negatives, four live linked GCC/ASan fixtures with old2/new0/
omit1/1 findings, sections and DWARF. No target interpreter/fixture executed.
All813 current product entries/source/compiler/helper inputs stay unchanged.
Independentb7d validates32 artifacts. Root reads the result; real ASan audit
failure remains unclosed. Transition8efe/dacaf preserves all1bfe products and
receipts, establishes new guards, fixed-config make/new-product-before-smoke,
and actual code/precision/CFI/root-DWARF comparison before a separate coverage
decision. Root explicitly permits a one-time source/linked-control-gated
two-marker commit after both completions, quiescence, preserved snapshot and
reviewed executor. It is unqualified native source, not gate1/PASS/landing.
No actual header edit/rebuild, automatic120 reuse/redo or support retry yet.

The separate owned upstream source checkout is now cleanexact321925 with5806
tracked inputs; root independently checks its clean HEAD. No configure/build
or target interpreter ran. Baseline review accepts source/concept, but finds
runner blockers before configure: slot lifetime/duplicate admission, missing
inherited-lock proof, incomplete claimed compiler-tool closure and final outer
cleanup receipt. Author revises task-owned artifacts with harmless host controls;
source and all old baselines stay intact. Match fork's actual auto-var-init=zero
as well as compiler/frame-pointer policy in the attribution cohort; no stock
production claim follows. Source-setup workrecord and native-phase plans await
their frozen review. Next hourly report remains13:45 PDT.

### 13:14 PDT: upstream configuration bound; debug repetitions continue

Debug reaches96/114 new transactions with no failures/fallback; release120/20
remains complete. The two-marker source change still waits for both completions,
quiescence, preserved expanded products and the exact transition executor.
Draft review identifies inherited-lock proof, post-cleanup complete inventory
and explicit tool/environment guards; owner is closing those bounded gaps.
Historical test-generated loader libraries keep source/literal-command/toolchain
provenance plus actual load/unload assertions. Their old bytes were not retained;
no retrospective hash claim, new compiler wrapper or old-matrix rerun is needed.

After independent V2 configure-plan review8fbce, root reads the exact launch
and grants configure only. It succeeds in27.660s, with outer/gate/inner0 and
final post-cleanup verification. Joint closure8908 binds all14 configured paths,
source/tools, final2dd5, inner1107, logs and product379f; independent actual
configuration reviewde24 clears it. Owned source remains clean321925; no target
interpreter, extension, test or benchmark runs. Make awaits a separate executor
that enforces accepted final success and joint configuration closure through
cleanup, rather than relying only on the inner configure receipt.

Common GCC/O3/NDEBUG/frame-pointer/zero-init policy matches current fork release.
Actual source-specific eval policy differs: upstream has empty `CFLAGS_CEVAL`;
the fork deliberately adds `-fno-gcse`. Root preserves this valid configure and
reports the difference; it cannot isolate object-model performance. Before claiming checkpoint-I parity,
require upstream `-fno-gcse` sensitivity or select a predeclared fully matched eval
policy from evidence. No extra build is ordered now solely for that comparison.
H1 parent/changed benefit retains identical eval policy. Next hourly13:45 PDT.

### 13:27 PDT: both campaigns complete; bounded rebuilds granted

Debug completion172f480d closes its114 new jobs plus pilot:120 transactions,
20 iterations,10,120 tests/2,020 recorded skips, zero fallback. New nineteen
record264,498 collections and61,810 report-only unpinned. Release63235055 stays
complete with10,120 tests/2,320 skips. Root reads the actual debug completion.
At quiescence, boundary8b7c and validation7996 preserve962 original receipt paths,
all6,322 tracked inputs and1,144 expanded product paths across four builds.
The597 deduplicated payloads total1,207,028,085 bytes. Old271-path maps stay
separate; ASan retains BUILD_FAIL and tailcall its historical support status.

Independent executor/boundary review8740 clears16 focused host controls, actual
inherited locks, complete raw journal replay, tool/environment guards and full
post-cleanup product/alias checks. Root grants the exact two annotations under
the recorded one-time source/control-gated commit exception. New clean commit
ce2ac8991ee changes only the two Py_GC_PRECISE prefixes; root reviews that diff.
New-plan1074 binds the unqualified identity. Sequential default/debug/ASan make
is granted and default starts. No standalone smoke/full/tail starts; comparison
of actual code/precision/CFI/root-DWARF must inform the affected-coverage ruling.

Baseline make-only V3a3f271/d08d and exact launch74e1 clear independent reviewa6c3.
Root grants that once-only make-j4 all with normal bootstrap/freezing/import
build steps and unchanged14-file accepted configuration. No standalone target
qualification or timing is granted. Read-only supplementb960 hashes341 external
candidate paths before make; it does not retroactively establish full consumed
SDK identity. That remaining gap stays explicit before the final timing cohort.
The author prepares a minimal post-build correctness/origin plan in parallel.

A coordination delay was corrected: the reviewer had completed its prior turn,
so queued messages did not activate it. Root explicitly restarted both bounded
reviews and received separate verdicts. No evidence or source changed during
that wait. Next substantive hourly report remains13:45 PDT.

### 13:45 PDT: target roughly 2× before JIT; replace blanket repetition

The user sets roughly twice upstream interpreter performance as the pre-JIT
goal, using holistic, clean, established techniques. New coordinator roadmap
pre-I-performance-roadmap.md defines the elapsed application/kernel score,
individual regressions and memory/pause guardrails, the GC landing path and
the IR/value/collector work. It makes the first trustworthy wall-time baseline
an early milestone instead of waiting until every planned optimization lands.
No technique is assigned a speculative multiplicative speedup.

The old20 rule came from note11 section8.2, without measured failure rates or
a stopping model. Both large old matrices passed; the full suite found the
missing thread case. Its original failure, root attribution and classification
negative supplied the causal evidence. PROCESS gate3 and note11 now replace
blanket20 prospectively with fail-old/pass-new controls, one affected pass per
relevant configuration, broad coverage early and hypothesis-driven bounded
repetition. Existing receipts and cheap internal hazard probes stay unchanged.
The current2b campaign will not restart20 iterations. The ASan14+2 union may
run once as16 modules with the same exact allocator exclusion and actual mode.

ROOTS ce2ac's release, O1-debug and ASan make-only builds all pass; ASan's
original linked audit failure is closed at build level. All three expanded
inventories have281 unchanged paths, five changed and no alias changes.
Comparison67-artifact manifesta15b8 clears independent evidence review26e17:
default/debug function maps match; ASan changes only the two raw wrappers'
precision membership. Selected unwind/root-location equivalence and identical
error strings are derived evidence; raw whole-image code equality is not claimed.
An actual four-semaphore supplement2973/dcb0 closes a symbol-selection hole and
is under independent review. Current affected24-transaction plan is source-only;
full normal/kill/stress/ASan/extras and landing remain. No new source test runs yet.

Owned upstream make closuredddc independently clears53446:584 products, source,
tools and14 accepted configuration files unchanged, zero build-internal import
failures. A diagnostic/three valid wrong-origin controls/one18-module qualification
package is reviewed; a zero-executed JUnit guard needs a narrow host-tested fix
before launch. No standalone upstream qualification or elapsed benchmark ran.
Remaining SDK dependency and eval-flag comparison limits are explicit.

Agents continue the GC bridge/affected adapter, independent review and baseline
qualification helpers. Next ownership is actual H3 union/statistics then bounded
H1;6e and remaining GC correctness can overlap under distinct interfaces. H2
root maps wait for precise readers/spans. Generations, safe call windows and
linear private-string growth address structural costs before the JIT. Next
hourly update about14:45 PDT.

At13:48 the actual semaphore supplement clears independent review4b68. Root
accepts the1bfe120 results as historical reliability evidence under the complete
source/native/root bridge, without claiming new-head passes or whole-image byte
equality. Root reads the complete affected adapter681290 and plan8c7314, then
grants24 once-only checks. Default lane4754 and debug lane30431 start; debug off
then kill share one sequential lane. Unexpected failures stop further launches.

The upstream JUnit guard fix clears review1ae2 and root grants exact e907 launch.
Its first diagnostic process returns0, but the origin verifier rejects the
legitimate synthetic pyexpat.errors namespace. No wrong-origin controls or
correctness modules run. Failure is preserved; author prepares a narrow
parent-extension ownership classification and meaningful host negatives for
review, with no target retry yet. This is a verifier limitation, not evidence
of an upstream interpreter failure. No benchmark or speedup is reported.

### 14:14 PDT: current affected/release full pass; baseline qualification complete

ROOTS ce2ac fresh affected completioncf3d018a passes24 transactions:243
test-runs/three existing tail-only skips,1,800 strict stale-root cases,4,783
collections and zero fallback. The deliberate StackWalk unpinned-counter case
is separate from strict stale checks. Independent full-plan review6405 clears
seven once-only profiles7d40; root grants the sequential campaign and stops
on unexpected failure. Release full receipt7b57d7ec passes51,132/3,458 skips,
all564 scheduled modules returned,63,573 collections/zero fallback. Debug full
is active; remaining kill/stress/ASan16, extras and bounded tail support are
not yet credited. Old1bfe120 evidence stays historical under the reviewed bridge.

Fresh tail support plan8145 is frozen for review. Its make and strict stale
stages may use the free slot while fulls run. Full StackWalk must wait for
all seven full profiles and acquire their serialization lock: perf-trampoline
test maps collide with test_perf_profiler cleanup. No skip hides this host
interference. Whole-tail/retain recommendations remain separate10a4/H1 work.

Owned exact-upstream qualification closure058d9bfb passes the narrowed pyexpat
successor and clears independent actual-scope review9bac. Positive diagnostic
records111 modules/100 traced imports; three valid foreign-origin controls
are rejected. The single18-module run passes5,145 tests/109 transcript skips.
JUnit's101 skipped testcase records are a distinct reporting domain. All584
native products and accepted source/config/tool identities remain unchanged.
Two write-only test caches have owner-captured hashes and restored0200 modes;
the reviewer independently rereads1,099 of1,101 evidence files. Original V1
verifier failure remains a failure. Actual50-row benchmark-driver origin plan
4f1c is source-only; no elapsed measurement or current speedup is reported.

The bounded value-ingress audit finds no actual canonical-value leak. It
corrects note14's stale suspicion: _PyValue_FromObject already canonicalizes.
Value-native returns still trust their ABI and need focused debug assertions
before guard simplification. Raw root anchors intentionally retain actual
boxes and must not be canonicalized. Revalidate the audit on the integrated
ROOTS/H3 union; no native assertion or optimization is authorized by this audit.
Next substantive hourly update remains about14:45 PDT.

### 14:49 PDT: six branch profiles pass; final stress reveals blockers

Fresh ce2ac debug normal ac1a914c and debug kill2651f7b9 each pass51,132
tests/3,401 skips and zero fallback. Release stress11 62988cf3 passes3,062/100
skips; debug stress11 37edc97d passes3,062/64. ASan16-kill ef9155b4 passes
3,976/92 with the exact allocator exclusion/options and zero fallback. These
are branch/source-specific results, not primary landing. Fresh tail make7352
and strictstale5953 pass6×100 attempts,706 collections/zero fallback/unpinned;
independent actualtail review clears their bounded scope.

Final stress100k returns an interpreter failure (ListAllTests.test_destroyed_by_gc,
worker3667921) and HTTPENV_CHANGED/two HTTPResponse unraisables(worker3667375).
At the last snapshot563/564 returned; test_gc remains live. The original run
finishes unsignaled/frozen; Walk/closure conditional grants do not activate.
No retry/filter/source edit. Original interpreter test lowers threshold to1
but calls no explicit gc.collect; stress scheduling returns before ordinary
threshold scheduling. Missing critical-window collection is a source hypothesis,
not yet a measured causal result or confirmed retention. HTTP response flush
sees a closed referent; explicit closure, missing edge and queued completion
remain separate hypotheses. Existing held Queue2 work is not retried elsewhere.

Upstream actual50-run closure9e725b7f clears independent reviewcbcb:all50 rows0,
1,169 evidence files/584 native products unchanged,117 observed source/extension
files and118 import names. Initial diagnostic/correctness scope stays058d.
No elapsed measurement ran. Paired timing template source review418c clears
26 host controls after restoring clean-source-status checks; bindings/reservation
still required. Root allows source-only ce2ac release binding as a possible
early diagnostic with known stress failures explicit, after terminal cleanup
and focused diagnosis if a bounded quiet window fits. Ready GC fixes take
priority; no target/parity/correctness claim follows.

H3 source owner checklist is ready; native source preflight finds only a
note11 documentation conflict with current primary. The future merge preserves
current no20 policy and both evidence histories. No H3 native merge yet.
GC3a source audit narrows bytes-writer registration/finish and Unicode ownership
boundaries. 6e audit corrects chaos to10,000 bit_length calls per loop/20,000
per registered invocation and separates18 NOARGS instance entries from getsets
and bound-method storage; no ABI/typeflag approval. Historical JSON unchanged.
Fresh merged gates remain mandatory after eventual LAND; a source-only reuse
plan keeps small helpers in existing detached checkout and large outputs in
project storage. O1 remains a candidate, not silently adopted. Next hourly
update about15:45 PDT, or earlier for a material finding.

### 15:14 PDT: terminal stress failure and bounded interpreter diagnosis

Final stress100k receipt64448666 is FAIL/native2/host1,50,722 reported tests
and3,418 skips. It retains interpreter assertion and HTTP unraisables plus a
20-minute test_gc.test_trashcan_threads timeout. Missing end/returned/suite
witnesses for that worker correctly prevent a qualified root aggregate.
Source/native/tool guards remain unchanged; six preceding profiles keep their
valid scope. No retry, new exclusion, tail Walk or extras launch follows.

Four diagnostic commands d478fceb/runner51df97f7 clear independent review
569ec607 after a prelaunch failure-code bug was fixed: original regrtest
assertion failure is native2, copied unittest failure native1. Root grants
original-stress, original-off, window-stress and window-off once each, with
normal admission/locks/timeouts and stop on unexpected outcome or normal-mode
failure. No production edit, extra collection or stress setter. Snapshot and
hash/process-history limitations stay explicit. Existing stress contract
replaces byte thresholds; two existing GC tests already account for that,
but no new skip or scheduling repair is chosen before actual diagnostics.

The trashcan trace supports a distinct queue-progress hypothesis; HTTP cause
is unresolved and old held Queue2 work remains held. Optional early release
timing binding6da22295 preserves the failed full and normal-release PASS as
separate facts. Its reservation is unset and it has no execution grant.
Focused diagnostics and ready fixes take priority. Next hourly15:45 PDT.

At15:20 root removes a scheduling-only dependency for H3 statistics: its
reviewed81417 implementation may proceed in a new isolated managed worktree
while ROOTS stays frozen. The first owner checkpoint covers counters/emitter
and reporting obligations; source/generator/test/branch-local bench ownership
is explicit. Current independent diagnostic/timing reviews finish before that
agent changes roles. No native build during the quiet window, no cross-lane
merge or primary landing, and no old qualification claimed on the future
union. A different reviewer must review the stats implementation.

Root also authorizes isolated bytes-writer3a design/implementation sequencing
from ce2ac, with concrete field/finish/fixture design review before production
edits. Agent creates managedboil-gc3a-bytes/boil/gc3a-bytes at exactce2ac,
70MiB disk, clean; no build/cache duplication or frozen-tree mutation.
Scope is embedded registered-root lifecycle for bytes/bytearray writers,
not Unicode/general PyMem retirement or Queue2. Native execution waits for
quiet release and eventual landing must compose the ROOTS successor.

### 15:28 PDT: trigger diagnosis complete; first elapsed comparison starts

Four once-only interpreter diagnosticsbb50b7d3 complete: unchanged stress
FAIL2/offPASS0, copied stressFAIL1/offPASS0. Copied critical main-interpreter
charged root phases are10→10 under stress and16→17 off. First stress's
adapter incorrectly demanded a suite snapshot absent on failure; preserve
INVALID86008c1a/nativeFAIL2 and offline supplemented54cbe8. No rerun.
Independent actual-scope review clears these bounded facts. Root keeps the
documented threshold override and reviews a test-only stress-eligibility
annotation consistent with existing threshold tests, unapplied until timing
finishes. No scheduler patch, broad filter or post-mark retention claim.

Final quiet plane9a46e75/reservation7f647dab clears independent narrow review
and root's immediate host check. Exact launch939e1a59 starts once at15:28:28:
29 alternating kernel pairs/five serial samples after warmup, seed0, admission
30s/native900s, at most20-minute opportunity. Both native slots/builds remain
reserved through cleanup. No retry, extension or partial score. ce2ac remains
unlanded with stress fullFAIL64448666; fork-driver origin, eval-policy and SDK
limits remain explicit. This is an initial diagnostic, not target attainment.

Stats managedworktreeboil-h3-stats/boil/h3-stats starts at81417; its author now
implements the reviewed coherent domain-counting/reporting fix. Independent
review will come from another owner. Nontrivial host work pins2,3, native work
waits. Bytes3a owner prepares the concrete short lifetime/fixture checkpoint;
HTTP source proposal proceeds separately. No native integration has landed.

### 15:45 PDT: elapsed diagnostic and two implementation lanes

First quiet release comparison completes15:30 at unchangedce2ac, independent
actual-scope review clear:29 rows/58 fresh processes/290 positive timed values,
upstream/fork geometric mean0.828243 (about21% more fork elapsed time),22
slower/seven faster. Coroutines0.365, comprehensions0.528 and generators0.592
are the largest deficits; float1.923, Richards1.897 and nbody1.603 lead the
wins. Closure68da9ae2 retains all raw values; normal source/native/cleanup
guards pass, no retry. Unlanded GC failures, fork-driver origin/eval-policy/SDK
limits stay explicit. This is diagnostic throughput, not2× attainment or
correctness. Prioritize shared allocation/GC, frame transitions and value/call
movement attribution; no row-specific operation or arbitrary opcode cutoff.

The unchanged HTTP module reproduces both finalizer errors,103/8,
ENV_CHANGED/native3, receiptc0ca71ff with paired actual-state guards. Root
rejects passive-referrer introspection before execution because this fork's
gc.get_referrers explicitly collects. The corrected ordinary-Python observer
then times out at180s, receipt4ae5e6f9 with incomplete worker evidence; its
748 partial events associate one response with colorizedtest_err and a
closed-state transition during close/flush. Buffer closer/cause remains
unknown; no observer retry or claim of baseline timeout. Source-derived
four-case primary052-versus-ce plan is being prepared using existing owned
integration products, with no native launch yet. Current fullFAIL remains.

Bytes3a sourcef5e818b0454 is committed/clean in isolatedboil-gc3a-bytes.
Root clears the focused9-line production owner-root change and lifecycle
fixture/test diff. Owned release/O1-debug builds pass under normal slots/locks;
debug actual-kill affected module passes52/zero skips,73 charged collections
and zero fallback/unpinned. Release off is active. O1 keeps assertions/symbols and is permitted for this lane,
without rebuilding existing integrationOg. Once-only affected positives and
the copied omitted-registration negative follow successful builds; no broad
full campaign yet and no general PyMem/Unicode retirement.

Isolated H3stats code from81417 now covers the generated counter domain,
actual target entries, generated adaptive property and coherent producer/
reader/benchmark semantics. Host15 reader/bench and five generator tests pass;
copied reader defects fail intended assertions. Independent source review is
active while writer builds run. Native size/pair/lifecycle/entry controls and
actual union qualification remain. No new native change landed in primary.
ROOTS still needs HTTP/trashcan disposition, bounded tail/extras, exact landing
review and fresh merged fulls. Next hourly checkpoint about16:45 PDT.

### 16:00 PDT: inherited HTTP exposure, writer proof and shared admission fix

The same four existing logging cases reproduce HTTPResponse closed-fp
unraisables on both owned primary052 and ce: ENV_CHANGED/native3,4/zero skips
each, complete actual state and zero fallback. This establishes inherited
exposure under the recorded source/product conditions, not mechanism equality.
The two-line interpreter eligibility annotation now passes its one-case
normal check and declared stress skip; no native rebuild or fullFAIL relabel.
Existing public trashcan-thread comparisons are granted once per build with
60s worker/90s outer budgets; primary is active. No observer/custom C probe.

Writer releaseoff52/10 skips and O1-debugkill52/zero skips pass. A separate
three-call omission build fails both new tests with the intended missing
INTERP-class assertion. Stress1000 writer selections each pass38 cases with
the exact10/zero debug skip sets,1,405/1,519 charged collections and zero
fallback/unpinned. Owned ASan debug build and38 actual-kill/stress0 cases pass,
zero skips,73 collections/zero fallback/unpinned and no unexpected sanitizer
output. Leak detection/fake stacks remain off; no leak proof is claimed.
The attempted releasekill configuration was rejected before initialization:
kill requires Py_DEBUG. It remains invalid/no tests, not a new runtime defect;
no extra O3-debug cohort or repeat was added.

Earlier writer test launchers inherited task TMPDIR and therefore used private
slot files. Their exact correctness results remain, with that admission limit;
build admission was global. Prospective stress/ASan launches fix outer/inner
environment placement and use actual global slots. Root also fixes the shared
gate_run path itself so TMPDIR cannot split machine admission again. Shell
syntax and a bounded real-FD control pass: the old script locks private scratch,
the changed script exposes global /tmp slot2 from two distinct scratch dirs.
No native Python target ran in that control. Frozen step tools stay unchanged
until safe handoff; no tests were repeated solely to erase the limitation.

H3 statistics native build passes. Its new high-entry test and15 reader tests
pass. Five legacy lifecycle tests initially read empty stderr while producer
wrote files; fresh private output-directory handling is added. The first
helper fix incorrectly expected repeated snapshots to agree: sys._stats_dump
clears counters. The corrected helper sums disjoint intervals; remaining dump
test and five previously unexecuted generator/domain cases pass once. Earlier
failed receipts and passing lifecycle scopes remain exact; native C/products
stay atd462 while test-only source progresses. Private counter/lifecycle/
predicted controls remain due; no primary runtime landing yet.

### 16:09 PDT: scoped ROOTS ruling and bytes checkpoint clear

Independent review supports the incremental ROOTS2b exception after public
primary052/current controls reproduce HTTP resource errors and bounded
trashcan noncompletion. Root accepts that exception, retaining both as final
GC/performance-blocking debt. Six passing profiles and the failed full keep
their exact identities. A minimal test/docs source bridge may finish successor
tail-WALK/extras; the old conditional plan is untouched. The eligibility
decorator changes test scope only and passes its normal/stress contract.

Root independently reviews bytes3a source/results and verifies all seven check
receipts, four build receipts and every check artifact hash against summary
dc428e4b. The causal omission's two JUnit assertions match the intended missing
INTERP-root condition. Focused positives/stress/ASan clear with their existing
admission/config/leak-proof limits. One exact ROOTS2b+bytes3a candidate and one
fresh mandatory merged ladder replace two duplicate integration ladders;
coverage, failure handling and primary Og policy remain intact. No native
integration has landed. The independent reviewer authored3a and explicitly
reserves its approval to root; root's review is filed separately.

H3 primitive control sources are frozen and granted under normal global
admission. The first compile stops before target execution on a missing
pycore_tstate.h prerequisite. A minimal header/include-path correction is
granted, preserving that failed attempt. No counter-negative or sizeof credit
follows until actual successful compilation and named numeric results.
The performance owner now prepares shared-cost attribution using existing
tools, separating setup from hot work; no new profiling campaign runs yet.

### 16:24 PDT: measured shared GC cost and successor tail launch

Four bounded existing-ce profiling windows complete once/native0 in37.14s,
off/extras0/stress0, unchanged listed inputs and zero lost software samples.
Coroutines/comprehensions/generators spend54.0/47.0/30.8% of their hot window
inside collection; Richards has zero hot collections. Root time is a subset,
not added to GC time. Native leaf samples corroborate traversal/collector
work; unresolved ancestry remains explicit. This changes priority toward a
sound nonmoving generational/remembered-set design before optional H1 tuning,
without predicting speedup or claiming the release score improved. The owner
starts source-only generation/barrier analysis. No additional profiling run.

ROOTS c679 successor bridge8685676c binds its exact test/docs source separately
from ce native products and preserves all original receipts. Copied loader
guards pass; full inventory changes only the known test_api source hash.
Root reviews binderf3942050 and unchanged child-criteria diffs and grants one
tail WALK followed by extras only on WALK PASS. Tail is active; no full retry.
Independent composition preflight confirms one documentation conflict and
prospective native/build-input equivalence to writerf5. Actual merged identity
and fresh gates remain due. Conditional ASan reuse can avoid a duplicate build
only with exact product/config/source equality and merged runtime origins;
its incomplete historical backend/SDK provenance stays disclosed.

H3 private counter controls and post-cleanup closure pass, with actual
sizeof895,544 versus historical665,176 (delta230,368), intended nine old-cutoff
and two misplaced-counter assertion failures, and unchanged native products.
Root independently verifies variant log/extension hashes and source/plan
linkage in review1e82f399ddf. H3's six-path composition inventory and H1's
smallest coherent passed-frame readiness are source-only; no native union,
actual tailcall claim or runtime optimization has landed.

### 16:50 PDT: hourly checkpoint, actual GC candidate and first GC optimization

ROOTS branch handback837 adds only completion docs after executed c679/nativece.
Successor tail11/0 and extras complete under scoped exception: cext16 with its
seven exact declared skips, unchanged regeneration/Clinic, traversal and19
header receipts/four unsupported. Completion0e0a8f3b preserves old full100k FAIL,
all raw artifacts and inherited HTTP/progress debt. Root merges837 and bytes089
as local candidate f2a5, resolves note11's documentation conflict by retaining
both actual exclusions/handoffs and primary's hazard-driven future policy.
Independent actual composition review clears; the only non-boil delta versus
built writerf5 is the known two-line test_api eligibility annotation.

Detached /tmp/boilgate is frozen at f2a5. Both existing primary O3/Og makes
return native0. Original adapters then reject expected generated Makefile
changes; those FAIL receipts remain. Exactly the two source-template hunks
(header audit scope and ceval.h dependency) reconstruct old Makefile hashes;
pyconfig/config.status/compiler/flags are unchanged. Root accepts a locked
post-build supplement and complete152-path maps/aliases per build. No make
retry. Fresh campaign879c84e4/plan5c3177ea starts its16 early jobs: pass/smoke
per build, affected/eval/strict-stale once across default0/debug0/debug2.
Six full/stress/ASan profiles remain for separate final selection/origin review;
no full100k retry, new filter or broad tail matrix. No qualification/primary
push of the native candidate follows yet.

Isolated H3 union e9a1 composes from f2a5 plus reviewed stats5d108 without
textual conflicts. Root reviews the actual shared-source delta and seven
idempotent generated products. Host90 retains two stale NOFB snapshot failures
and one existing expected failure; only the two expectations change and both
pass once. One fresh owned Py_STATS build passes configure/make (25.4/87.1s),
116 checked modules/zero failed imports,587 products and cleanup closure.
Actual stats/producer-reader/frame/root checks remain; no old receipt is
relabeled. Nonstats integration follows the GC checkpoint separately.

The four-row profiling conclusion is now used for work: GC54/47/31% in the
three slow rows, zero hot collections in Richards. Candidate/death counts
do not measure allocation ages or isolate generation savings. A source audit
finds existing cleared-generator finalizer calls already skip queueing, but
the earlier ordering DFS still processes those roots. Small isolated change
a285 shares actual eligibility at root/emission/enqueue, preserves unrestricted
dead transit traversal, and adds a pure-Python closed-generator/code-constant
ordering regression. Root clears source; owned O3 build passes (108.5s,
67 shared modules/zero failed imports). Semantic tests, copied ordering
negative and matched parent timing remain due; no gain claimed.

Nonmoving generations direction clears independent review; activation needs
complete owner/store/constructor/domain closure, per-object safe-promotion
evidence, pre-callback remembered-set commit, omission oracle and bounded major
policy. Minors remain unimplemented. H1 is a later bounded matched-tailcall
experiment. The release application score remains the earlier0.828 diagnostic;
the roughly2× target and final GC/lifecycle correctness are not achieved.
Next hourly checkpoint about17:50 PDT; material failures are reported sooner.

### 17:26 PDT: current release coverage, bounded GC and IR checks

All16 early f2 integration jobs pass once across the three mode/build profiles:
5,712 reported cases/398 skips and1,800 strict stale attempts,
44,500 observed collections/zero fallback. Seven original successful jobs
retain their original plan identity. The first stale host wrapper failed
before the target because merged PYTHONPATH supplied incompatible encodings;
its successor removes that host-only overlay, without rerunning the seven.

The first actual-f2 release full completes all564 modules:51,134 reported
cases/3,470 skips and two failures in test_cmd_line only. Source confirms the
launcher's unnecessary PYTHONPATH defeats invalid-PYTHONHOME startup, while
PYTHONNOUSERSITE differs under a child -E. Original full/native2 remains FAIL.
Root accepts the explicit563+1 disposition in PROCESS: unchanged validation
clears563 module cohorts, and a fresh whole corrected-environment cmd_line
passes85/13. Assembly039eacec proves the disjoint564 roster with both real
plan/environment identities. It is not a uniform full PASS; the original
all-run63,544 collections/zero fallback include the failed module and are not
apportioned. No successful module is repeated. Frozen successor82a07005/
plandcc77606 runs the five unattempted broad profiles then unchanged extras,
serially and stopping on failure. Og normal full is active; primary native
candidate remains local and unqualified for push.

H3 union e9a1 now has all three owned builds: Py_STATS, GCC O3 and default
Og. Actual high-ID producer/reader checks pass; eight bounded target jobs
also pass once: smoke1,926/50release and1,926/20debug, eleven H3 modules
1,376/12 in each of normal release/debug and actual debug kill, and strict
six-times100 stale controls per profile. Summary535edd88 retains all real
counts/root witnesses/cleanup identities. Source-derived writer eligibility
is43 protected integration IDs on release and53 on debug, with exact existing
skips; the proposal's uniform53 claim was corrected before execution. The
three integration jobs await f2's shared perf-map window. No H3 full or landing.

Finalizer eligibility a285 passes whole GC and generator/coroutine/asyncgen
companions on O3 off and Og kill:142/17 plus268/0, and142/2 plus268/0.
All observed fallback counts are zero; whole-GC unpinned totals remain
report-only. The separate copied omit-reversal source025d fails exactly the
new event-order assertion, with no crash/import error; this proves that
ordering oracle, not every transit omission. Root verifies all five result
statuses and40 raw artifact hashes. The as-built checkpoint records shared
post-pair debug cleanup honestly. Source/native identities stay frozen for
one held four-row matched timing package; no benefit is claimed yet. A
one-expression weakref-work classification follow-on remains proposed only.

The generations allocator/domain ledger now binds ten byte-identical f2/e9
sources and closes the current reserve/publication/free/resize/abandoned-heap
boundaries. A young index must avoid enumerating all old blocks. Store and
constructor closure remains; per-object safe-promotion evidence must survive
mark clearing. Existing pre-sweep Python debug output is an earlier reentry
boundary than stop callbacks, so after-sweep metadata commit alone is unsound.
No minors/barriers or held Queue2 change is implemented. Frame/store ownership
source audit continues on an isolated frozen tree.

Offline source/assembly analysis reuses the four accepted profiles, without
new sampling. It resolves SEND/YIELD dispatch/state transitions and feedback/
shape guards; these GCC non-tail sites cannot be attributed to a tail-only
frame reload. A matched same-Clang dispatch comparison is a later bounded
question, separate from H1's frame-parameter experiment. Reaching2x upstream
needs about2.41x current throughput; a conditional Amdahl model shows why GC
and common frame/call/value/interpreter work both matter. The measured
29-row score remains0.828. Next hourly report remains about17:50 PDT.

### 17:50 PDT: broad GC results, diagnostic failure and next integration scope

Frozen f2's Og normal and actual-kill full suites pass51,134 tests each,
3,410 skips/564 modules and565 paired driver/worker cohorts. Receipts5623413d
and6133f9cb retain66,798/66,809 collections, zero fallback and3,889 report-only
unpinned hits each. Normal takes718s; kill702s. Assembled release coverage
remains039eacec, with the original launcher-environment failure preserved.

The following release stress11 is terminal FAILa10a4cc1:3,062 reported tests,
100 skips, one test_stack_hits assertion (type tuple where Probe expected),
ten passed modules plus failed test_gc; all12 mode/stress cohorts pair and
source/product guards pass. Later debug stress, ASan16 and extras did not
start. Candidate f2 remains local, unqualified for push. No partial stress
PASS or retry-to-green is recorded.

Source separates two diagnostic concerns. Journal filtering matches numeric
addresses over historical collections and cannot identify an allocation's
incarnation. Independently, gc_stack_hits captures its borrowed array before
PyList_New; that allocation can collect and realloc the array before its
first read. The reader is unchanged since its original implementation;
ancestry is not a waiver or proof of this failure's cause. One ordinary
prefix diagnostic952c57d9 runs once on the existing release binary: original
test/assertions pass, both pre-clear prefixes are empty. This is inconclusive
for stale-address attribution and does not establish pointer safety. A bounded
reader/test repair proposal follows; the native campaign stays stopped.

H3's eight bounded jobs and finalizer eligibility's four semantic checks plus
ordering negative remain complete. H3 integration3 and four-row GC timing
remain unrun. The next coherent accepted GC/IR union will use O1 plus Py_DEBUG,
with effective flags and fresh root/negative/stale/normal/kill qualification;
existing Og receipts stay frozen. Historical turnaround supports trying the
configuration but is not a matched speed comparison.

The frame/store generations ledger now records actual owner transitions and
constructor gaps; no barrier or minor is implemented. Unicode writer design
3cd46d77 closes21 private lifetime owners plus public Create, embedded-root
registration, rooted Finish, private-header placement and test-adapter migration.
Its isolated implementation is delegated; no native grant or new root fixture.
The accepted performance score stays0.828x upstream; no new speedup is claimed.
Next hourly documentation/status checkpoint about18:50 PDT.

### 18:11 PDT: first GC timing benefit and coherent next candidate

All three H3 integration jobs pass on e9, including actual debug kill:
54 reported tests each, exact11/1/1 existing skips,43/53/53 eligible protected
methods and528/612/612 collections, zero fallback and one report-only unpinned
hit per row. Compact all-eleven summarydc7ae1e1 retains eight earlier jobs
plus these three; native/source/cleanup guards hold.

Minimal reader/history source7c703fda1b7 clears root and independent review.
Unicode ownership2f37be1d059 also clears both source reviews: embedded stable
root, balanced cleanup, rooted finalization, private-header move and mechanical
public/private test-adapter migration. No native Unicode result exists yet.
Root composes H3+reader+Unicode+finalizer eligibility in managed next union
5a45f9bb6a7. Frozen f2/e9/a285 trees and results stay intact. One fresh actual
combined O3/O1-debug ladder replaces completing f2 then repeating broad gates.

The first matched four-row finalizer-eligibility timing completes in32s:
coroutines1.2502, comprehensions1.0550, generators1.0697, Richards1.0331
parent/candidate median elapsed ratios. Root independently checks all40 raw
values, normalization/medians/ratios, both actual JSON-tail witnesses and
cleanup flags. The earlier pre-admission host rejection is preserved; its
successor changes only the measured duplicate PATH expectation, with no
target retry. Score5d7ab61c and launch11990bcf bind planfc7ffab3.
This is an encouraging unqualified patch diagnostic, not a new upstream or
29-row score. Richards' earlier zero hot collections cautions against complete
GC attribution; serial/process/layout/noise limits remain. The small patch
proceeds to combined qualification. Native quiet reservation is released.

Root delegates next owned O3/O1 build preparation to the GC operator, H1's
uniform tail-call frame parameter source implementation to the handler owner,
and the finite list/dict/set generations ledger to the Unicode owner. No H1
native experiment, barrier or minor is implemented. Next hourly report18:50.

### 18:50 PDT: affected GC completion, dispatch baselines and IR work

Frozen combined5a45 builds and post-cleanup checks complete on GCC O3,
O1 Py_DEBUG and ASan. Effective O1 follows Og in actual commands; assertions,
frame pointers and zero initialization remain. All builds check116 modules,
67 shared and zero failed imports. ASan retains its declared fake-stack/leak
limits. No broad or primary-landing result is inferred from compilation.

All19 affected transactions pass and clean up, indexf77aebb0: stack-hit1/0
four times (normal andN1000 per build), Unicode consumers1,037 twice,
smoke1,930 twice, H3 modules1,376 three modes, GC142 twice, integration54
three modes, and three strict stale6x100 jobs. All observed fallback is zero;
unpinned counts remain report-only except strict stale's requiredzero.
Whole-GC142/19release and142/4kill include two unexecuted byte-trigger tests:
truthy stringPYTHON_GCSTRESS='0' activates existing decorators despite getter0.
FutureN0 jobs omit the variable and require both cases in fresh normal fulls.
Those old skip outcomes remain unchanged; no whole-GC retry hides the gap.

Both matched Clang21.1.8 baselines now exist on5a45. Goto configure/make and
module checks pass, but the wrapper's CIE substring guard rejects a newline.
Independent retained-report review finds1CIE/8,389FDEs and exact eval-frame
coverage. OriginalFAIL54a91b2a remains; offline supplement5b3f968e closes actual
config/tool/source/native cleanup without rebuilding. Tail4c743dda and
cleanupf118a88d pass, including293 emitted precise handlers/labels/UNKNOWN.
CFI presence/membership is compile evidence, not dynamic ownership. Four
bounded runtime/root checks are reviewed and wait for current ASan checks.
The ASan16+Unicode7 then Clang4 then five-broad sequence keeps shared perf-map
cleanup serialized. Each chain stops on failure; no full100k/20-repeat redo.

H1 source78ac84e5709 merged the current baseline without history rewriting;
clean candidate62456cc differs from5a45 in only six frame-argument files.
Host generation/limited controls pass; there is no native H1 result or gain.
H2 now has an isolated inert effect/decoder implementation underway from5a45;
its initial scalar coverage is explicitly partial, complex edges unsupported,
and runtime constructors/readers/clearing remain unchanged. Final-word maps
must follow layout/exception emission and check_words normalization, with
virtual-liveness projection independently checked before activation.

Root reviewed/copied the13-file container and33-file runtime generations
ledgers, verifying their exact5a45 source hashes. Corrected the latter's
function-field total to14 after checking header/traversal; its listed fields
were already correct. Types/shapes/native/iterator/root/port and concrete
promotion/barrier/reentry obligations remain. No minor or barrier exists.

README's current-state section is shortened to separate landed, implemented,
validated and remaining work; detailed historical receipts stay in WORKLOG
and linked records. The measured upstream diagnostic remains0.828243; four-row
patch timings remain1.250/1.055/1.070/1.033 with their stated limits. No new
upstream score or target claim. Next hourly documentation/status19:50 PDT.

### 19:07 PDT: sanitizer and Clang baseline checks complete

Both actual5a45 ASan transactions pass and verify after cleanup. ASan16
reports3,983 tests/92 skips,25,373 collections, zero fallback and6,651
report-only unpinned hits; result2ca504c8. Unicode7 reports1,037/24,
1,176 collections, zero fallback and6report-only unpinned; resulte8e8188e.
Root independently verifies both result/cleanup records and artifact hashes.
ASan16 finishes within the original600-second module deadline; no timeout
extension or target retry occurs. Existing allocator/remote filters remain.

Clang goto and tail each complete the103-case bounded selection and existing
stale6x100 checks. Goto has the exact one tail-only skip; tail has none and
executes precise handler ownership. Actual modeoff/N0 and post-cleanup guards
pass, with zero fallback; combined rows report one unpinned hit each and
strict stale rows zero. These are bounded successes, not full qualification.
The direct native-window handoff starts release-full, then debug-full,
debug-kill-full and the two stress11 profiles, once each and stop on failure.
The existing final closure helper is reviewed and conditionally authorized
only after all seven current broad/ASan profiles pass and clean up.

H2's inert scalar decoder clears independent source review and coordinator
inspection. It covers27 pristine handlers with126 explicitly unsupported;
temporary-slot effects are not complete semantics or a purity model.
Boundary/property negatives and coherent native validation are next; no
constructor, root reader or clearing changes are active. H1 native build
preparation follows its validated baselines. No new performance score exists.

### 19:19 PDT: broad launcher failure and bounded correction

Original release full terminates native2 after6m4s:51,181 reported tests,
3,533 skips;564 returned modules comprise460 passed,94 skipped,
8 resource-denied and two failed. Result ee01b433 remains FAIL. The inherited
focused-test environment wrongly sets PYTHONDONTWRITEBYTECODE and uses a
project-filesystem bytecode cache. Pydoc cannot find its fresh imported os
cache; compileall reports two cross-device renames and two hardlink-import
assertions. The bytecode flag also suppresses10 import and54 importlib cases,
independently checked by root from the skip journal. These passing modules
therefore cannot be credited as ordinary bytecode-enabled coverage.

The original exact-product host guard also fails: six generated sysconfig
cache files were added, while every original588 release path/hash/alias
remains intact. Debug/ASan originals stay unchanged. No later broad profile
starts. Root chooses one corrected release full followed by the four unrun
configurations, instead of building a retrospective partition of all
environment-sensitive cases. The successor omits the bytecode-disable flag,
uses a disposable /tmp cache on the same filesystem as test sources, and
retains all original native/config/source/tool bindings. Only added regular
in-build cache files may be recorded separately. No test exclusion or native
repair is proposed. Corrected packets require source review before launch;
the original helpers, plan, failure and journals remain immutable.

H2 commits its inert scalar decoder and boundary/property controls as5df336.
Its owned GCC O3 build completes with116 modules checked,67 shared and zero
failed imports; cleanup verification precedes the granted O1-debug build.
H1's matching release/assertion-enabled Clang tail builds are conditionally
authorized after H2 releases the spare slot. No target/performance claim
follows from those builds. Next hourly checkpoint remains19:50 PDT.

### 19:50 PDT: three full suites pass; inert H2 validates

All three corrected5a45 full suites pass and verify after cleanup. Release
reports51,181 tests/3,469 skips,63,643 collections, zero fallback and2,832
report-only unpinned hits (28cf749d). O1-debug reports51,181/3,412,
67,079 collections, zero fallback and4,013 report-only unpinned (6d822e0b).
Actual debug-kill reports51,181/3,412,67,061 collections, zero fallback and
4,009 report-only unpinned (2a2f5b85). Each564-module roster returns
462 passed,94 skipped and8 resource-denied;565 driver/worker cohorts.
All64 previously bytecode-suppressed cases and both byte-trigger cases
are protected and execute. Original invalid-environment failure remains
ee01b433; old helpers/plans/results stay immutable. Original native/config/
source/tool products remain unchanged, with generated caches recorded.

Stress11 release is active with10/11 returned passed at the last safe
boundary; stress debug and the conditional C-extension/regeneration/
traversal/header closure follow. Independent final GC review is underway,
explicitly excluding its reviewer's H3 authorship and using the prior
independent H3 source reviews. No primary runtime landing or push occurs yet.

H2 inert scalar decoder5df336 compiles in owned GCC O3/O1-debug cohorts.
The original release selection fails an existing test's table parser; its
seven successful modules remain explicitly carried evidence. Test-only
b3e6847 scopes the parser and passes27 host controls plus its old-parser
negative. No C or native product rebuild occurs. Corrected whole release
generator module passes79 cases/zero skips/one expected failure; debug
actual-kill runs all eight modules,748/10/one expected failure. All seven
new tests execute. Coordinator independently verifies both raw artifact
sets, cleanup links, cache removal and exact one-file source bridge.
This completes bounded inert-decoder validation, not maps or activation.

H1 release compile/293 precise symbols/CFI complete. A concurrent foreign
test scratch directory trips its cleanup guard; unchanged products are
accepted through a separate read-only supplement, retaining the failure.
O1 Clang debug then compiles all native modules but fails inline audit:
strrchr is missing from the noncollecting leaf table. Source-reviewed
tool/test-only94ab98e fixes that leaf and proves scanner propagation,
while unknown/allocating substitutes remain conservative. Fresh inline
audit/config/293-symbol/CFI/native postchecks b7be93b8 and cleanup093ca866
pass on the unchanged image; original make2 at audit remains failed.
No compiler/configure/relink or release rebuild follows. Candidate runtime
checks, spill/code analysis and matched quiet timing remain.

Queue2a source-only design resumes in parallel. Current findings require
ordering preparation before batching, distinct prebatch/postbatch failure
handling, and no Python debug callbacks before raw garbage state is rooted.
The finalizer graph must count every unfinished predecessor, not owners;
unrelated calls retain concurrency. No native Queue2a fix or probe is claimed.
The measured upstream diagnostic stays0.828243; no new speedup score exists.
Next hourly documentation/status checkpoint about20:50 PDT.

## 2026-10-30

### 07:40 PDT: GC integration qualified; remaining safety work prioritized

Frozen5a45f9bb6a7 clears independent LAND_INCREMENTAL review and lands as
d6a23b9fe0c. All seven declared profiles complete: affected19, ASan16 kill,
ASan Unicode7 kill, corrected release/debug/debug-kill fulls and both
stress11 sets. The original environment-failed full, predecessor stress
failures and retained exception stay explicit. Fulls report51,181 tests
each with zero fallback; these are reported totals, including skips.
Release stress11 reports3,065/100skips,44,771 collections/0fallback;
debug stress11 reports3,065/64,49,107/0. Cext16/exact7skips, unchanged
regeneration/Clinic, traversal and19 optional-header receipts pass.

Final-receipts SHAe8f95745 binds the terminal packet. Independent review
checks49 indexed/nested references; the coordinator rechecks its30 direct
references and both integration smoke receipts. Merge runtime/test/tool
source equals5a45 exactly; only19 project Markdown documents differ.
Qualified owned binaries/source roots are explicitly reused under PROCESS's
prose-only ruling, not relabeled as rebuilt integration products.
Release smoke b5f08128 passes1,930 reported tests/50skips,1,191 collections,
0fallback/221 report-only unpinned. O1-debug smoke d8152545 passes1,930/20,
1,232/0/227. Cleanup9a9dd699/00d43729 passes. No full suite is repeated
for documentation-only publication.

H1 four bounded runtime/root jobs pass and independent review clears their
source/evidence; actual native maps are release588/debug587. Its static
code changes are mixed. The matched29-workload Clang goto/tail/H1 plan is
prepared but parked without timing. H2 remains a bounded inert decoder,
not active liveness/maps or a primary landing.

Queue2a's eager-order proposal remains source HOLD and unexecuted: registry
detachment invalidates the stable-edge premise. V2 instead preserves the
baseline late graph boundary, aborts before deallocation on ordering OOM
and retains honest partial delivery. Independent V2 review clears bounded
native validation; the operator is freezing/committing the four-file
candidate and reusing existing build/worker helpers. No native result is
credited yet. Separate source owners are closing the coherent snapshot/
debug/registry-lock design and dependency-completion scheduling readiness.
The original held probes are preserved, without reruns.

User guidance supersedes the hard aggregate target: roughly2× is soft,
especially for simple arithmetic. Finish outstanding GC fixes first; then
publish the detailed interpreter report and commission holistic design
reflection for simple orthogonal improvements. Parameterized LOAD_ATTR
families are an example to evaluate, not a preapproved new opcode list.
README, PROCESS and the roadmap record this priority and softened target.

### 08:05 PDT: sweep-OOM controls complete; snapshot successor rebuilds

Sweep-OOM candidate7f4459bf259 completes bounded validation. Release and
debug-kill focus each execute six cases without skips;123/132 collections,
zero fallback/unpinned. Whole GC/pyexpat/XML/IO passes1,561 reported tests
per build,52/37 skips,12,099/13,892 collections,zero fallback;2,093/3,252
report-only unpinned. The copied original fallback at5ae1269 fails the one
unchanged no-sweep test with native2/host1 and the intended
`ordering OOM swept objects` assertion. It is not a crash or startup error.
Terminal index78e685fd, positiveea715cc6 and negative audita091be06 bind
the preserved evidence. Root verifies the11 terminal references,12 positive
receipt/cleanup/plan hashes,16 positive raw artifacts and seven negative
raw/plan artifacts. Global qualification remains false; no broad fulls or
primary landing are claimed. The focused source branch is pushed.

The first focused run's six cases/native0 remain preserved as harness
INVALID: its old wrapper rejected regrtest's informational `-m platform`
child. The existing narrow auxiliary protocol is reused after independent
review; the corrected focus runs once without rebuild. Nonquiet H1/baseline
combined logs share that header limitation; their case/root evidence stays
separate from a clean transaction claim. Source/evidence review confirms
all corrected GC full/stress and integration smoke commands use quiet mode,
never invoke that header child and contain no such error. Their prior
qualification stands; no full-suite rerun follows this header defect.

S3 source5410cf3 passes independent source review, release focus9/0 and
four affected modules594/25skips,13,320 collections,zero fallback/2,120
report-only unpinned. Debug inline audit correctly fails on an outlined,
collecting debug_flush helper without a precision marker. Its live values
are only owned libc bytes/scalars; annotation-only successor009fa2dcbe8
passes the function checker. Both configurations rebuild/refreeze and pass
their audits; corrected runtime checks are active. Old5410 results and
debug failure remain frozen. No unchanged-native bridge or audit exception
is used. A later source check finds that six new S3 children use
script_helper's default isolated mode, ignoring the inherited verifier
environment. Their assertions pass off-mode; parent/worker kill results
do not supply child-kill proof. A focused test-only helper correction
passes parsed mode explicitly and asserts mode/extras/N0 in each child.
Its corrected focused replacements, copied immediate-write negative and
actual-child-kill ASan case remain due. Unchanged module evidence is retained
with this limitation; no C rebuild follows a test-only correction.

Separate work-stack/MARKED-OOM source is frozen for review: checked sticky
push, successful journal before MARKED, complete error propagation through
cycle/finalizer/resurrection walks, and safe next-collection recovery tests.
It has no native/build result. Completion lifecycle review confirms ordinary
dispatch must remain admitted during cooperative waits; latch admission only
at the successful world-stopped zero-running readiness check. Foreign waiting
nodes require an explicit adoption/metadata policy; fork cannot acknowledge
a vanished callback as completed. Buffer readiness also shows why five local
tp_finalize additions would miss Unpickler overrides/reacquisition and native
release dependencies. These remain design obligations, not implemented fixes.

Finite closure, snapshot, work-stack, lifecycle and buffer-readiness plans
and their reviews are now in the repository. Interpreter timing remains
parked. Next hourly documentation/status checkpoint about08:35 PDT.

### 08:35 PDT: three bounded safety repairs complete; combined qualification starts

Clean168f30a96ff composes sweep-OOM7f445, snapshot native009fa/test07af
and work-stack repaira73f. All bounded positive jobs pass. Each copied old
behavior fails its intended assertion: unsafe sweep after ordering OOM,
debug writer before snapshot retirement, and missing finalizer on the very
next collection after work-stack OOM. These are causal controls, not native
crashes or startup failures. Earlier harness, precision-annotation and
child-mode limitations remain preserved beside their corrected successors.

S3 final indexccc94b63 binds29 references, checked by the coordinator. Its
six corrected children assert actual mode; release/off and debug/kill focus
pass9/0 each, and the actual-child-kill ASan case passes. Work-stack terminal
index7c03b8d5 binds six positive jobs, both build capsules and negative
audit49ca9676. Independent review checks65 positive artifacts and the named
negative failure, raw modes, auxiliary records and cleanup. Whole GC passes
154/17skips off and154/2 kill, zero fallback; unpinned2094/3254 remains
report-only. No component native jobs remain.

The coordinator grants fresh actual168 ASan configure/make/freeze/cleanup
using the existing helper recipe: buildhelper56c6e1ae, plan06d68cb0 and
launches77d39b1e. Owned release/O1-debug products remain588 each and are
reused explicitly. Seven once-per-profile qualification vectors and closure
are prepared; target launches await the actual ASan capsule binding and
short coordinator check. No primary landing or whole-GC correctness claim.

Queue2b's14-file owner-metadata draft is frozen for independent source review
in its own worktree. It implements symmetric registration guards, rejects
ownerless static types and selects destruction metadata from each object's
owner. No build, native execution or commit is credited. Foreign mutation
events, lifecycle/adoption/fork, completion dependencies and native buffer
release remain separate contracts. Interpreter timing stays parked. The next
hourly documentation/status checkpoint is about09:35 PDT.

### 09:35 PDT: narrow GC scope; preserve safety qualification

The user questions subinterpreter and shutdown complexity. Adopt one main
interpreter per process with ordinary threads for the first milestone.
Subinterpreter creation must fail explicitly; that restriction is planned,
not yet implemented. Explicit process workers are the alternative, with no
transparent API emulation. Two holistic reviews favor the existing nonmoving
collector/hybrid roots, one callback drainer and simpler actual process-exit
handling. Defer generic cleanup slots, completion DAG, foreign ownership,
adoption and borrow/lifecycle frameworks. Buffer lifetime during normal
execution remains mandatory; evaluate existing managed owners before banning
Python exporters. A source audit and finite buffer-readiness design are active.

Native168 combined qualification passes both ASan groups, all three full
suites and release stress11. Debug stress11 has ten passed modules and one
failed GC diagnostic counter test: an automatic collection occurred between
snapshots. Preserve the failure. Exact test-only9e223040 adds disable_gc and
an intent comment, with all assertions and native products unchanged. Root
reviewed the minimal source/native bridge and all16 successor plan inputs;
one whole-GC debug/off/N1000 successor plus verification/cleanup is authorized.
No rebuild/full/ASan repeat. Closure remains pending.

Owner63b completes bounded release/debug-kill positives and copied owner-
discovery negative, with earlier precision/audit build failures retained.
Its topic is pushed; integration is deferred under the narrower scope.
The primary runtime remains d6a23. Interpreter timing remains parked. Next
hourly documentation/status checkpoint about10:35 PDT.

### 09:46 PDT: three GC safety repairs merge

Qualified test-source9e223040/native168 integrates as ff6ed9d6bfb after
independent source/evidence review. The merge changes six source/test files;
all38 differing primary paths versus9e are project Markdown. Closure passes
Cext16/exact7skips, unchanged regeneration/Clinic, traversal and19 borrowed-
header receipts/four unsupported modules. Final index9bacd190 binds43 fresh
references, checked independently, plus the previously checked86 held refs.
Six original profiles and the explicit corrected debug-stress composite
remain distinct; the original diagnostic failure is not relabeled.

Bounded release/debug integration smoke preparation reuses the accepted
recipe and qualified native products explicitly, with no rebuild/full repeat.
Main-only admission and finite struct/memoryview release safety proceed in
separate source worktrees. The one-drainer slice follows this handback.
Buffer and callback completion remain open; interpreter timing stays parked.

Post-merge closure: release/debug smoke each passes1,930 reported tests,
exact50/20skips,1196/1234collections and zero fallback. Index68b4bba0 and
all15 references/raw/cleanup are independently checked. Qualified native168,
source9e and actual primaryff6 identities stay explicit. This safety increment
is complete. Main-only runtime683a407 and bufferd6bf312 clear source review;
buffer's first O1-debug build is active. Reuse the existing public capability
sys.implementation.supports_isolated_interpreters for honest unsupported-test
classification; no additional capability constant or creation probe is needed.


### 10:30 PDT: bounded main-only/buffer feedback; compose before further builds

Primary remains safety merge ff6ed9d6bfb. Main-only dbb5 passes O1-debug new5
and whole4:811 reported/76skips, including59 declared unsupported creation
cases,10660collections/0fallback. Root checked completion ce21d7d7 and all38
references. Existing main inspection and ordinary thread coverage remain.

Buffer d6bf's original five-failure profile remains failed. Source successor
87b6 selects all active memory acquisitions, including native-only ones, and
counts actual completion toward the existing second explicit pass. New5 pass;
unchanged whole4 remains FAIL479/23skips, only foreign-owner every_kind_routed
fails. Ordinary Python release and all three native cycle failures clear.
No new collection loop, universal depth guarantee or test assertion change.
Copied negative8d41 deletes only pre-release Py_CLEAR(so); one case fails the
named exhausted-iterator assertion, observing(3,(97,),b'abc') with live backing.
Root checks causal index e53f67f9/all10refs and raw failure. No retry or native
crash credited. Positive AS-BUILT6f34db41 retains all component limits.

Root composes main-only and buffer in managed boil/gc-simple c9587ad064b,
including a one-line support.__all__ export correction. Candidate is unbuilt.
Single-drainer source review identifies Popen(preexec_fn) as another fork
caller needing recoverable busy rejection, rather than the fatal public void
wrapper. Correct/review this, then compose and build once for O1-debug feedback.
Unpickler owner and sealed PickleBuffer/SRE source ports run in parallel;
ndarray's acquisition metadata/export accounting proof remains. No new native
grant for those ports. Final supported union needs release/ASan/broad coverage
and closure before landing. Interpreter timing remains parked. Next hourly
status checkpoint about11:30 PDT, with earlier material updates as needed.


### 10:48 PDT: drainer union passes; consumer oracle correction and sixth owner

Candidate925d O1 focus17/0 and whole9 1518/146skips pass,208/12632collections,
zero fallback. Sixty skips declare unsupported creation. Root checks terminal
index af128e61/all36refs plus raw/cleanup. This is bounded validation, not a
primary landing. Source627590 sealed consumers and48b021 Unpickler owner are
independently reviewed, committed/pushed and composed in dc7b; make-only O1
rebuild passes with116modules/0importfailures and preserved old925d payloads.

New12 focus stops at11passes/one scanner-oracle failure; whole7 never starts.
Match.group() reacquires the exporter, whose callback closes the idle scanner.
Two independent reviews confirm it. Sole test-source successor772 replaces
.group() with stored .span() offsets, keeping active/no-release and eventual
once-only assertions. No C/native changes or rebuild. Original failure index
0ba6b395/all18refs checked. Authorize one corrected case, then original whole7
on explicit native-dc7/test-772; carry other11 passed IDs, never relabel FAIL.

ND owner implementation proceeds source-only with original metadata separate
from transformed views and balanced export counts. Private constructor reuse
will explicitly reject successful reinitialization; named push/pop remain.
The owner audit finds sixth persistent holder xibuffer, still reachable in
same-owner queues/channels. Reuse the original MV's managed buffer on receipt
and existing XID disposal instead of porting foreign-wrapper finalization.
Source task is active. Final safeguard-removal ledger is independently read;
no coercion or silent callback suppression remains an acceptable final policy.
Three disjoint causal controls can share one negative build after source closes.
No release/ASan/full union or refreshed performance result is claimed.


### 11:20 PDT: final GC source composed; qualify from a clean existing worktree

Safetyff6 remains primary. Final3c6b8daa506 composes all main-only/drainer/
readiness/consumer ports and safeguard424. Independent source review clears
bounded validation, excluding approval of the reviewer's own P/SRE/ND code;
those have separate source reviews. No final-union native pass yet.

Consumer oracle corrections are test-only772/e48/86. Original failures
0ba6b395 and7a496840 remain failed. The corrected scanner passes after actual
callback return and later collection, with once-only assertions retained.
Whole7 b6dc5a04 remains failed only on acquisition accounting in test_re;
fresh whole test_re passes172/3skips at86, index0472ba31. Six original passed
modules carry explicitly, yielding1783/103skip composite coverage. Root
checked final indexes and references; C products remain native-dc7.

Final make-only at3c6 stopped at check-clean-src before any compiler/test:
unknown-origin in-source configure/build appeared in the source root. All588
owned debug products remained byte-identical. Preserve the ambient686-file/
446MB inventory and original failure9a545ccd; do not delete or attribute it.
Root reuses source-only boil-buffer-xid-main-only, switching new branch
boil/gc-final-validation at3c6 (same tree as424). Original branch and packets
remain. Accepted fresh O1 recipe changes only HEAD/path/profile bindings;
independent review checks all inputs and executable0755 worker modes.
New13 and conditional whole9 follow build, not another repeated campaign.
Four source-reviewed causal omissions share one later negative build; broad
release/debug/ASan and closure follow. No performance timing yet. Next hourly
summary around11:30 PDT, with material updates sooner.

### 11:22 PDT: behavioral testing guidance and parallel interpreter review

Fresh3c6 O1 configure/make and cleanup pass in98.03s,116module checks and
zero import failures. Independent review verifies4503855f/f57d66aa/a4d0b35e,
all588 products/aliases, source/config/tool bindings and executable worker.
New13 then conditional affected9 retain their existing grant; no target
result claimed yet. Broad preparation includes the seven existing profiles
plus one ASan-focused15 for Unpickler7 and XID8 not in ASan16. The sole
protection adjustment removes an explicitly unsupported subinterpreter case
from must-run requirements while preserving its visible skip; no new filter.

User asks for externally meaningful acceptance: liveness/reclamation, valid
data, resource reuse and bounded memory, with focused critical internal unit
invariants. Record this in PROCESS and the closure plan; leave the frozen
candidate untouched. A read-only interpreter quality review/prospective list
now runs alongside GC qualification, ahead of implementation and matched
timing after landing. Existing H1/H2 remain isolated. Primary runtime remains
safetyff6; final GC source is complete but not landed.


### 11:28 PDT: final bounded GC checks pass; interpreter quality report drafted

Actual3c6 fresh O1 new13 passes13/0skips (212collections/0fallback/0unpinned).
Whole affected9 passes1863/120skips (10435collections/0fallback/1report-only
unpinned). Cleanup passes; root and independent reviewer check completion
95d56f4c/all37 references, actualkill/N0/config/source and raw summaries.
No broad qualification or landing follows from this bounded result.

The reviewed eight-profile/closure recipe reuses accepted protocols, exact
filters and executable modes, adding focused ASan15 for previously uncovered
Unpickler/XID acquisitions. O3 and isolated four-omission O1 builds proceed
in parallel. Original O3 log-path host error opened no subprocess; a two-path
output-only correction preserves it and leaves argv/helper/plan unchanged.
Final broader execution waits for causal outcomes and scanner test-source
review under the user's external-behavior guidance. No active source edit.

Provisional pre-I-interpreter-status-and-opportunities.md reviews the actual
architecture and seven shared opportunities. Root reads it and checks all27
relative destinations. LandedH3 and isolatedH1/H2 stay distinct; historical
0.828243 timing is not a current score. No performance implementation or new
timing is authorized before GC landing. Behavioral testing policy is recorded
in PROCESS/closure plan; critical ownership/rollback units remain appropriate.


### 11:35 PDT: behavior-based scanner checks and four causal controls complete

Test-onlyf9457ef8494 changes onlyLib/test_re.py: removes reflective traversal
and exact acquisition/release counts, retains match/string data underGC and
allocation pressure, checks exporter live/dead and backing resize. Review
caught an initial expected-string alias that would retain the tested string;
corrected4c72 preserves independent expected construction. Source remains
frozen during native work. Fresh wholeRe passes172/3skips on native3c6,
107collections/0fallback/0unpinned; completion4bd06285/all19 refs checked.
Carry thirteen focused cases and eight original whole modules explicitly.

Fresh O3 configure/make and cleanup pass114.75s; capsuleaf7b/all10refs clears
independent review. Negative35c92 is exactly four runtime omissions from3c6,
with unchanged tests. Each produces its intended one-case assertion failure,
no errors/skips/crash/timeout/retry and zero fallback/unpinned; ordinary
nativeFAIL2/host1 results stay failed. Root and independent reviewer verify
e859dd0f/all54 refs and named causal observations. Broader ASan/debug/release
and closure recipe is granted after final actualsource/receipt binding.
Candidatef945 is pushed; primary stillsafetyff6. Merge-tree check is clean
without touching the index/worktree or preserved unrelated drafts.

Planning docs and the seven-opportunity interpreter source report are current;
Loom handover is synchronized. Next hourly update about12:30 PDT, with
material failures or completed gates reported sooner. No interpreter timing.


### 12:08 PDT: sanitizer passes; full qualification finds Python wrapper ownership

Atf945 ASan16 passes4022/141skips (13002collections/0fallback/1report-only
unpinned), Unicode7 passes1037/25 (718/0/0), owner15 passes15/0 (162/0/0).
All three clear independent raw/mode/config/cleanup review. Parent multiprocessing
packages in ASan16 do not prove processes-leaf coverage: their module-wide
HAVE_ASAN_FORK_BUG skips remain. Keep that source limit explicit.

Release full returns nativeSUCCESS0,51236/3795, all564 modules; qualification
is FAIL. Three processes modules report SharedMemory.__del__ BufferErrors.
The validator first rejects a legitimate release skip of the debug-only
scanner interruption unit; correct eligibility for release while retaining
debug/stress/ASan protection. Failure7736d428/all13 refs and cleanup7d341b5b
are checked; no later profile/closure starts. HTTP103/8 (SSL skips) and
GC176/35 pass; trashcan eligibility/no-skip is inferred from source/roster,
not a success-ID journal. Old100kFAIL stays failed.

Source review finds dead temporary slices queued after user finalizers.
Reviewed Python-only repair0d2116 drops automatic view/mapping ownership and
closes the owned fd, leaving strict explicit close unchanged. No C rebuild.
Four public behavior checks cover data/liveness, no unraisables, strict live
exports and actual Linux mapping/fd retirement. First test cleanup draft used
rawunlink without tracker unregister; fixed before execution with immediate
public unlink. Move these local tests to test_memoryview with track=False to
avoid the inherited ASan fork skip without disabling it. No new targets yet.

Operator prepares new4 across modes, old-policy control and fresh affected7
(processes3/misc3/memoryview); original release carry is557 (455PASSED/
94SKIPPED/8RESOURCE). Ordinary MV getscpu; sanitizer respectsfork limitation.
Remaining broad profiles and closure follow reviewed explicit assembly.
Interpreter source report gains concrete arithmetic/attribute/branch examples
and an existing-budget sensitivity question before generational complexity.
No new performance measurement or implementation.


### 12:15 PDT: SharedMemory successor frozen; targeted validation starts

Candidate dac6f98f953 moves the four local lifetime tests to test_memoryview,
restores the multiprocessing test source byte-for-byte to f945, and leaves
the reviewed destructor repair unchanged. No C/native products change.
Root and independent reviewer clear source, literal launches, actual mode/
resource/protection maps and the exact old-policy worker injection. Six
targeted positives start: new4 in release/debug-kill/ASan-kill, affected six
processes/misc modules, ordinary whole memoryview with cpu enabled, and
sanitizer whole memoryview with its existing fork boundary. First release4
passes4/0skips,62collections/0fallback/0unpinned; verify and cleanup pass.
Old-policy causal failure is separate; the original full remains failed.

README/PROCESS/closure/owner/performance docs now distinguish this successor
from f945 evidence. Prospective primary+dac6 merge-tree is clean; no merge
is performed. Parallel interpreter source review now narrows the seven
opportunities into a finite first-slice recommendation, without timing or
implementation before GC landing. Next hourly checkpoint remains12:30 PDT.


### 12:30 PDT: SharedMemory repair clears; remaining union gates run

All six bounded positives pass: release/debug-kill/ASan-kill each4/0;
processes3/misc3=814/79; ordinary memoryviewcpu=178/18; ASan memoryview=178/19.
Each has actual declared mode/N0, zero fallback/unpinned and verified cleanup.
Old-policy injection fails the unchanged discarded-view assertion with captured
BufferError,1test/1failure/0skip,native2/host1,47collections/0fallback/unpinned.
Root checks terminal58723869/all48 references; independent review replays all
six raw validators and reconciles exact returned rows and causal witnesses.

Release assemblyc13fdc23 retains557 original rows plus fresh7,564 modules,
51240tests/3794skips,462PASSED/94SKIPPED/8RESOURCE_DENIED. ASan162b81264e
retains15 plus fresh memoryview,4026/141. Root checks20/12 references and
recomputes disjoint totals; reviewer also verifies original-row equality.
Sources/resource contexts and parent ASan skip limits remain explicit.
The original release full remains nativeSUCCESS0/qualificationFAIL.

Remaining vectorsdb7353d7/closurea363bc6a bind only actual prior evidence;
helpers/rosters/filters/protections unchanged. Session23907 starts normal
O1-debug full, then kill full, release/debug stress11 and closure on success.
At12:30:15 the active normal full has497/564 returned,405PASSED/89SKIPPED/
3RESOURCE_DENIED and no observed FAILED/ENV_CHANGED. This live snapshot is
not terminal evidence; later profiles/closure have not started. Final LAND,
primary merge and postmerge smoke remain. Runtime primary is stillsafetyff6.

Interim independent HOLD review is publishedc8c714638ab. Source report and
conditional first-slice comparison118ca727d47 are committed/pushed; no
interpreter implementation/timing. Held smoke prep derives release1935/80
from the actual nine retained rows, with debug/final-qualification/merge
bindings unavailable. All affected planning docs update at this hourly
checkpoint. Next hourly13:30 PDT; material failure/landing reported sooner.


### 12:57 PDT: broad profiles clear; source-only closure metadata repair

Normal and kill O1-debug fulls each pass51240/3739,all564 modules,zero
fallback and3 report-only unpinned. Release/debug stress11 pass3104/148 and
3104/113,zero fallback and33/11 report-only unpinned. All four clear independent
raw validator/cleanup review; their actual source remains dac6. HTTP103/8
passes in both fulls; trashcan_threads is selected/unskipped under the unchanged
loader/filter contract, without a separate success-ID journal.

Closure cext16/7 and regeneration pass. Traversal stops on two metadata
findings; unbuilt19 never starts. Failure1cca11e9/all36 refs and postterminal
source/config/tools/3x588 product guards are independently checked. Source
review proves ND cached base.obj aliases are borrowed from the visited managed
owner→master.obj; producer nodes have NULL obj and released caches are inactive.
The removed XID wrapper also leaves a stale allowance. Root and independent
review clear exact metadata-only diff548487a3; successor4da292b1294 is committed
and pushed, changing only Tools/boil/traverse_allow.txt. No runtime/checker logic,
test or native product changes; the original closure remains failed.

Continue with existing execution bodies/locks: explicit four-profile plus
cext/regen carry, one corrected traversal and first unbuilt19 attempt. No
rebuild or broad retry is justified. Final independent LAND, primary merge and
postmerge smoke remain held. PROCESS now places this cheap source audit before
broad suites on relevant frozen compositions; exact unchanged receipts may
carry into closure. Matched timing preparationbdcef5daf0c is published, but
no interpreter performance run or implementation has begun.


### 13:18 PDT: supported GC integrated and post-merge checks clear

Qualified source4da292b1294 is integrated and pushed as75e8a1e0c01,
tree8a309fd40dd. Independent Git checks confirm exactly15 declared project
Markdown differences and code identity to4da. Runtime library/tests remain
dac6, ordinary O3/O1-debug native products3c6 and ASan/Py_DEBUG nativef945.
Final12f1ea31 binds276 references; all match independent review. LAND record
499b9 retains source authorship exclusions, mixed-source/resource composites,
platform/SDK limits and every original failed run.

Normal/kill O1-debug fulls each pass51,240 tests/3,739 skips, all564 modules
accounted for, zero fallback. Release/debug stress11 pass3,104/148 and
3,104/113 at N1000, zero fallback; unpinned counts remain report-only.
Release557+fresh7 gives51,240/3,794 and ASan15+fresh memoryview gives4,026/141,
with local4 ASan changed-path coverage separate. The original release
nativeSUCCESS/qualificationFAIL and sanitizer parent skip limits remain
explicit. HTTP103/8 passes both full modes; trashcan_threads is selected and
unskipped in normal/kill/stress under its existing loader contract. No old
stress100k pass or separate per-success journal is invented.

Original closure passed cext16/exact7 and regeneration, then failed traversal
metadata; unbuilt did not run. Exact one-file4da fixes only the stale XI
allowance and ND borrowed-owner metadata. Carried successful stages retain
dac6 identity; fresh corrected traversal and first19 header receipts pass,
with four unsupported modules explicit. Current source/config/tool/header
and all three original588-product maps verify after cleanup.

Post-merge smokes run release then debug once, actual off/N0, with1,935 tests
and exact80/50 skips. Both have nine PASSED modules, ten paired driver/workers,
zero fallback/unpinned and run/verify0; collections773/812. Independent raw
replay and exact skip counters agree. Final smoke indexf56e0b6b/all20 references
matches, then root pushes the actual merge. No native job remains from this
qualification. The LAND review receives a post-merge addendum; failed records
are not rewritten.

README, PROCESS, closure/owner ledgers, note13 and interpreter planning headers
now describe the enforced Linux/main-interpreter/ordinary-thread/shared-GIL
milestone. Supported callback/buffer completion is not a stale blocker;
foreign/adoption, extra platforms and precision/root-retirement work remain
separate. H3 is landed; H1/H2 remain isolated. Roughly2× is soft guidance.
The owned upstream eval-nogcse configure and make now pass, with independently
reviewed closure23c9ab25 and actual ceval flags. Stock products are unchanged.
One positive origin diagnostic and the bounded18-module correctness job are
active;29-row origins and quiet matched timing remain held until their checks.
No refreshed score or interpreter ABI implementation is claimed. Preserve
stock upstream and qualified fork products. Next hourly report13:30 PDT if
work is still active; material configuration, failure or measurement updates
are reported when they occur.

### 13:30 PDT: matched comparator checks pass; next slice follows shared cost

The owned upstream eval-nogcse configure/make, positive own-origin diagnostic,
18-module correctness and29 workload origins pass. Correctness reports5,145
tests/109 transcript skips; JUnit records5,044 executed cases/101 skips and
zero failures/errors. Those representations remain distinct. Fork29 origins
also pass with cleanup; no refreshed elapsed score or new interpreter ABI is claimed.
Qualified75e8 and both upstream images remain unchanged.

The first-slice recommendation now uses an evidence-based decision tree.
Numeric NOARGS has a finite18-method closure and a real receiver-boxing cost,
but is not automatically first. The inspected generator/coroutine kernels
have no direct calls to those methods; chaos's transitive bit_length traffic
does not explain their historical regressions. Current shared cost may favor
existing GC-budget sensitivity or the prepared matched dispatch/H1 comparison
before an ABI expansion. Distinguish creation from resume and receiver boxes
from result/storage allocations. A producer-fact slice must prove canonical
BOOL at all entries; checked int32 addition and its overflow path stay intact.
Origins admit the intended implementations, not performance conclusions.
Resolve only the attribution needed to choose one coherent change; roughly2×
remains soft guidance. No native work or historical outcome changed in this
documentation update.

### 13:44 PDT: first matched post-GC elapsed baseline complete

The once-only matched29 workload transaction completed in77.043 seconds,
with58 measured processes and290 timed values. Both RUNS0 JSON-tail origin
witnesses pass; native/outer return codes and final cleanup pass. No rejection,
retry, trimmed row or repeated timing campaign occurred. Quiet/native work is
finished. Closuree5b0a9b2 binds393 artifacts; root independently rehashed all
and recomputed the complete medians/ratios/geomean from retained raw vectors.
Independent review0f88e376 then clears normalization, all60 command/environment
records,58 paired load checks, both raw RUNS0 origin witnesses and actual
source/product/cleanup bindings. No native job remains.

Upstream/fork geometric mean is0.8242992363, equivalent to1.21315× elapsed
time:21 slower rows and eight faster. Coroutines0.4489, comprehensions0.5097
and generators0.5737 remain large deficits; float2.0394, Richards1.8655 and
nbody1.6681 favor fork. Several fork sample ranges are wide. These are five
correlated serial calls per fresh runtime/row, with no formal confidence
interval; the new [measurement record](design/work/pre-I-post-GC-timing.md)
retains all29 rows, raw vectors and explicit spread definitions. This is not
a before/after measurement of the GC landing: source and comparator policy
changed from the historical ce2 diagnostic.

The source/design reflection recommends current collection-cost attribution
and a bounded32/64MiB existing-budget comparison before new generational or
numeric ABI work. Use unchanged products and the existing four-row phase
probe, reporting RSS, pause and reclamation costs alongside elapsed time.
That follow-up is planned, not executed in this cycle. H1/H2 remain isolated;
the status report inventories seven shared directions. Tests continue to
favor observable lifetime, data and resource behavior, with focused critical
invariant units. Qualified GC is landed and unchanged; no new runtime patch
is part of this performance/documentation checkpoint.

### 14:27 PDT: current GC costs and experiment closure decisions

Four once-only current-source counter windows pass on unchanged qualified
release products. Timed GC/wall is47.1% coroutines,54.1% comprehensions,
29.7% generators and0% Richards. Mark/root closure is8.0%/3.7%/10.8%/0% of
wall and already inside GC, not extra root-discovery time. All modes are
off/extras0/N0, all native/cleanup checks pass, zero fallback. The existing
probe and50-call hot budget are unchanged; no threshold or code change.

One whole-process coroutine profile adds2,124 software CPU samples with zero
lost events and successful cleanup. It includes setup/warmup/shutdown, not
an exact hot window. Collector share45.76%, heap classification8.47% and
sweep10.40% are overlapping inclusive figures. Source-correlated samples
show repeated dead-object flag/eligibility checks, weakref handling and an
all-dead buffer-retention scan. Finalizer-order time is mostly eligibility
rejection, not evidence that ordering graphs dominate. Some libc symbols are
unresolved; sampled lines are not exact instruction costs.

Index40e9ea00 binds33 artifacts; root independently rehashes every artifact
and recomputes all four GC fractions from raw snapshots. The
[assessment](design/work/pre-I-gc-policy-and-experiment-decisions.md) compares
the policy with BDWGC/Go/Lua primary descriptions. Selective death processing
is now the first source-backed code candidate: actual work eligibility and
stage subsets, preserving resurrection/readiness/ordering/OOM semantics.
Budget sensitivity remains a diagnostic, not an assumed fix. No optimization
was implemented or speedup claimed.

The [inventory](design/work/worktree-closure-plan.md) classifies39 physical
worktrees:22 landed copies,8 intentional negative controls,1 deferred foreign
topic,2 dirty historical trees and6 retained including the repository anchor.
H1 is tail-only and unmeasured, so select archival for this GCC milestone.
H2 has no immediate consumer, so archive its tested partial patch rather than
merge unused code. Target three operational checkouts plus the Git anchor;
retain required receipts/refs first and handle the identified dirty/shell/
ambient-build exceptions separately. No worktree/ref was removed or runtime
experiment merged in this cycle. Unrelated user changes remain untouched.

### 14:56 PDT: historical checkout retirement and GC architecture review

The authorized housekeeping lane retired 32 historical worktrees through
ordinary removal, with 32 exact source tags pushed and remotely verified.
Eight registered roots remain: integration, serving runtime, comparator,
active selective-death topic, Git anchor and three ownership/dirty-state
exceptions. H1/H2 and deferred foreign-owner work are archived without merging
unused code. Selected raw evidence totals 918.53 MiB locally; closed binaries
and caches were excluded explicitly. This is not an offsite raw-data backup.
The final GC, matched timing and postmerge indices are unchanged and all
276/393/20 referenced artifact hashes still match. Negative4's 55 dependency
files remain byte-identical at an explicitly labeled evidence-only path;
the unchanged serving input guard passes. Unrelated reviews are preserved.

The selective-death prototype 788695 has independent source approval and a
successful O1/Py_DEBUG build, reusing the accepted native runner. Its initial
13-case debug-kill run passed all 12 existing controls and failed only the
new resurrection test. Wider affected modules did not run. Independent review
and root inspection of pinned upstream321925 show callback-free weakrefs are
intentionally retained through finalization for cache invalidation safety.
The correction will assert survivor identity before eventual clearing, keeping
callback/finalizer counts and usable data checks. Preserve the original failed
run; no runtime policy change, speedup or primary landing is inferred.

The independently reviewed [architecture plan](design/work/gc-architecture-next.md)
describes the current full-heap nonmoving collector and recommends selective
lifecycle work, GC-owned reclamation with explicit native cleanup contracts,
then measured nonmoving generations. Copying needs writable reference/root
locations, pinning and native API/ports contracts; it is a larger scope.
The generations proposal now marks the qualified S3 repair complete and
foreign lifecycle deferred, preserving the genuine young-index, mutator,
constructor/promotion, root and independent-oracle obligations. No generations,
barriers or movement were implemented in this cycle. Next hourly update is
15:30 PDT if work remains active; material findings are reported sooner.

### 14:59 PDT: selective-death affected behavior checks pass

Test-only correction69c34 changes only the new weakref resurrection oracle.
The focused13 pass with zero skips; the conditional four whole affected
modules pass599 tests with46 declared skips. Both native/host return0 and
post-command cleanup passes. They reuse O1 native788 unchanged, with explicit
test-source/native identities. The original failed13 remains preserved in
its14-reference index. No retry of unchanged code or new runtime fix occurred.
Focused roots record219 collections/zero fallback/zero unpinned; whole records
10,784/zero fallback/one report-only unpinned. Do not claim zero unpinned or
global qualification from this finite check. Release measurements, remaining
landing checks and primary runtime integration are still pending. Reuse the
existing four-row paired harness and build recipe for the next bounded
performance question; no new benchmark framework is needed.

### 15:30 PDT: feedback33, first release win and comparative GC profiling

The selective-death O3 build passes on source10d877 with unchanged runtime
code788. One bounded parent/candidate four-row diagnostic passes both actual
RUNS0 origins, retains all40 raw values, and passes cleanup. Independent replay
confirms median parent/candidate ratios: coroutines1.308649,
comprehensions1.062317, generators1.078140, Richards0.985113. Elapsed reductions
are23.59%/5.87%/7.25%/−1.51%. Coroutine sample ranges do not overlap; the two
other allocation-heavy ranges overlap substantially. Five serial calls per
process are not independent trials or an overall29-row/upstream score.
Paired completion34033f13 binds163 artifacts; no runtime merge occurred.

Feedback33 requests measured phase costs, a fair reference-count comparison,
concrete nursery/weakref/ordering decisions and allocation-budget evidence.
The source reviews separate actual callbacks from broad no-op eligibility;
the compact dead inventory is not an eagerly built full-heap callback graph.
Referrer-first cyclic finalization is a fork policy beyond Python's general
cyclic-order guarantee, while ordinary file/buffer lifetime remains required.
NaN-boxed inline values are compatible with reference-counted pointer objects;
restoring counts would require ownership and cycle/reuse work, not abandoning
the value representation. No restoration is selected from an upstream/fork
comparison that bundles other differences.

One finite13 comparative profiling batch is active under launch2e95c27a:
qualified32/64MiB on four existing rows, matched upstream defaults on those
four, and candidate32MiB coroutines because its measured cost changed. Existing
probe adapterf209 uses explicit runtime kinds, two bounded FIFO/ACK controls,
199Hz CPU-clock/FP records and hardware cycles/instructions. GNU time records
whole-process RSS; GC/root snapshots cover the hot interval. Actual setup
sets the floor before loading/warmup, without an explicit collection. No
instrumentation, runtime C edit, extra callback-heavy matrix or retry campaign.
Quiet CPU0/sibling8 is reserved; heavy decoding waits until release. Original
products and failed receipts remain intact. The revised architecture artifact
will incorporate these results and both focused reviews. Next hourly update
16:30 PDT if work remains active; material findings are reported sooner.


### 2026-10-30 15:50 PDT — review33 measured GC plan complete

The finite13 native comparison completed all13 processes with return0,
per-run/final source/product/tool/cleanup checks passing and quiet reservation
released. Completion9ef2649b binds97 references; the coordinator independently
rehashed all97 and replayed hot intervals and GC/root deltas. Disjoint analysis
5016ed98 reconciles all13 raw sample streams with zero loss/excluded samples;
phase counts/percentages independently sum to100%. All hardware counts have
100% running coverage. No additional native work or runtime edit followed.

Paired-four completion34033f13 retains163 references and40 raw samples.
Independent replay gives parent/candidate throughput1.30865/1.06232/1.07814/
0.98511 for coroutines/comprehensions/generators/Richards. Only coroutine sample
ranges do not overlap; this is no replacement29-row aggregate. Native candidate
coroutines retain301 collections/RSS67.5MiB while timedGC falls6.022→3.302s
and instructions85.275→79.338G. The32/64MiB experiment saves only3.24%/1.66%/
6.98% instructions on the three slow rows, while RSS rises52.09%/72.62%/84.41%.
Keep32MiB. Richards' identical instructions/nohotGC but17.76% wall variance
limits causal budget timing claims.

Exact comprehension preparation consumes24.36% sampledCPU (retention+ordering+
eligibility), versus4.06% drainer. The next five-file implementation handoff
separates existing proven native-only suspended-generator clear from Python
finalization, with whole-pass observer/weakref/resource fallback and all fallible
preparation before irreversible commit. It remains source-only. Selective-death
qualification/landing remains first; topic doc-onlyd3331398c624 is pushed with
all588 O3 products/aliases unchanged from measured source/native10d8773815c.

Rewrote gc-architecture-next.md for review33: concrete measured recommendations,
RC comparison, actual ordering/weakref semantics, sparse registry/dead-array
costs, policy comparisons and common movable nursery with stable native space.
The nonmoving-first prerequisite is superseded. Define space lookup/regions,
writable actual root slots, core barriers, scanned native owners, complete pin
discovery before evacuation and SHALLOW constructor/age publication.8MiB is
an explicit half-LLC experimental capacity, not a production default. Root-only
or readonly spans pin; interior aliases reload/recompute or pin their owners.
Independent cost/contract and nursery reviewers clear the revised plan after
these explicit proof clauses and a24.35→24.36 rounding correction. Entrypoint,
PROCESS and affected decision docs updated; original failures and unrelated
working files preserved. Next hourly checkpoint16:30PDT if work remains active.


### 2026-10-30 16:47 PDT — approved GC implementation starts

The user authorizes landing the framed GC design and requests a code-level
root/data-structure/organization review. AI policy and applicable instructions
reread; unrelated dirty review files remain preserved. Three existing agents
reactivated: roots_audit owns selective-death qualification in its current tree;
string_design owns five-file native-only cleanup in new repository-managed
boil-gc-native-cleanup from primary9c3c0d57424; stats_design_review owns the
independent code/root review and subsequent patch review. No unlanded cross-lane
merge; each runtime lane takes landed integration before final qualification.

Selective topic merged primary as its next commit with documentation-only
incoming changes. Existing O3/O1 images retain explicit native identities.
Its finite required release/O1normal+kill fulls, stress, ASan and closure/control
packet is being prepared from existing launchers; heavy work is coordinated by
that owner. Native-cleanup implementation is source-only pending source review.
No runtime optimization or nursery has newly landed at this checkpoint.

Architecture plan now includes current root recipe and concrete structures:
thread-local _PyValueRoot spans, interpreter _PyGCRoot lists, initialized-start
bitmap, local address sets/range vectors, trace context, contiguous dead records,
sticky-OOM stacks/order scratch, callback batches, stable weakref entries and
frame descriptor cache. Writable slot versus lifetime anchor, space-aware object
lookup, constructor publication, actual lifecycle facts and commit boundaries
are the shared concepts introduced with their first real consumers. Large file
splits and generic container/allocator frameworks are not prerequisites. Broad
performance summary follows landed improvements; next hourly checkpoint17:44PDT.


Independent code review `gc-code-organization-review.md` clears architecture§7
against actual source9c3c: root recipes, ownership and structure descriptions
are sound. It identifies existing call_effects as the shared allocation/reentry
authority and distinguishes fatal current object-map extension from returnable
future region reservation. First fresh selective release smoke passes1,935 tests/
80 skips with source94ae/native10d and cleanup/root/state checks. Affected whole4
and the fresh ASan build are active; full qualification is not yet complete.


### 2026-10-30 17:09 PDT — review35 reflection; implementation continues

The user asks whether root lists/traversal inherit unsuitable CPython machinery,
and explicitly keeps implementation moving. Roots qualification and stats review
were still running after the root read-only poll interruption; the native-cleanup
author was deliberately awaiting independent review. No native job was killed
or restarted. Ordinary IR register writes are assignments into existing frame
arrays; full physical slot extent is scanned at collection. Extra C roots cover
boxing/reentry temporaries, not every instruction. _PyValueRoot/_PyGCRoot/public
root registration are fork additions absent pinned upstream, not per-object
CPython generation lists. Internal durable registration is O(1), while public
slot-address removal searches O(R). Existing data does not isolate that cost.

Architecture§8 compares chunked transient shadow stacks and dense persistent
handles without conflating lifetimes. The selected managed path uses direct
slot/range scanning with one field definition for mark/update; native opaque
layouts retain tp_traverse/pinning. Trusted exact slots versus ambiguous lookup
are distinct contracts; current-domain/initialized identity cannot disappear
merely because a pointer is tagged. Independent source review clears§8 with
one wording refinement: historical subtraction remains diagnostic, while live
collection no longer uses the RC cycle-isolation algorithm.

Native-cleanup V1 source32d4e339 and narrow B/no-GIL successor70977878 are
committed/pushed after independent SOURCE_CLEAR. The latter preserves exported
return0 API while removing unreachable RC code and redundant generator
TRACK/stub/UNTRACK ceremony, keeping weakref/native cleanup. Host root/AST/child
checks pass; no native qualification claimed. Author prepares reused native
packets and will merge selective integration before final adaptation/qualification.
Region implementation packet4a3bd1ba names actual wrapper, publication, lookup,
free/resize/quarantine/reset and late-lifetime boundaries; existing-mimalloc
cleanup comes first. A direct managed scanner packet is being prepared.

Selective qualification source94ae/native10d (fresh ASan94ae) remains frozen.
ASan16 terminal passes4,027/141skips,13,015collections/0fallback/3report-only
unpinned; release full passes51,241/3,795skips,58,608collections/0fallback/1report-only
unpinned. Independent raw validator/ref replay clears frozen outcomes and
preserves warning/skip limits. Debug normal is active; debug kill, stress and
closure follow. No new runtime landing or full interpreter speed claim yet.
Next hourly checkpoint17:44PDT; material changes are reported sooner.


### 2026-10-30 17:15 PDT — direct scanner implementation assigned

Read and approved managed-scanner packetb186a768. Stats owner created
boil-gc-managed-scanning from integrationd0f467; owns shared ranges/accessors,
cell/tuple/frame traversal and gc_trace direct mark paths with focused tests.
The tiny gc_live generic-range hunk stays unapplied until native cleanup lands
and releases that file; no unlanded cross-lane merge or concurrent file edit.
First source checkpoint expected about25min, not a native/runtime promise.
Keep exact/current-domain lookup and debug checks; no faster pointer trust,
movement, unused updater, per-edge replacement callback or layout DSL.
The same owner will correct obsolete object.h header/refcount comments without
changing actual layout. No region runtime is being built yet; its per-object
metadata cost versus compact page/bitmap/stride knowledge needs explicit review.

Native author709778 prepares reused O1/O3/ASan/tail and finite affected/control
packets, without source/product bindings or execution before selective landing.
Debug-normal selective full now terminal PASS:51,241/3,739skips,60,800collections,
0fallback/1report-onlyunpinned;598.68s. Debug-kill is active, stress2 and closure
follow. Source94ae and products stay frozen. gc-designrev4 and handover73
contain review35 reflection; primary remains runtime75e8 pending landing.

### 2026-10-30 17:25 PDT — one cleanup/scanner candidate; storage by measured cost

Selective debug-kill full passes51,241/3,739skips,60,778collections/0fallback/
3report-onlyunpinned; result9f126439 and cleanup3b0c7dba independently replayed.
Stress2 and closure remain. Native709778 and scannerb9d829 both clear independent
source review, with no native validation claimed. Scanner's ordinary frame
fixture creates its payload inside the callee; the running-generator fixture
is documented as public behavior coverage rather than exclusive fast-path proof.

Coordinator authorizes one named composition after selective lands: native
owner merges primary and reviewed scanner history, applies the tiny held frame
hunk, freezes the actual union for independent review, then runs one coherent
qualification covering both focused sets and applicable backends. Stats stops
duplicate launcher preparation. This reduces redundant full ladders while
preserving source/native identity and the union's correctness obligations.

Independent storage review clears existing mimalloc pages as the first batch
investigation if storage return remains material after these changes. Raw
unpublished allocations count as occupied; native teardown, wrappers, remote
free ownership, debug/ASan/quarantine and last-journal-borrower boundaries remain.
Regions stay conditional on measured allocation/locality and compact metadata:
16-byte cells cannot justify24–32-byte records merely to obtain bump allocation.
No batch/region code or new speedup is claimed. PROCESS records prose-only
selective merge reconciliation; two fresh qualified-product smokes precede push.

### 2026-10-30 17:33 PDT — real warning-context bug; qualify one final GC union

Selective debugstress fails validation despite native exit0: asyncgen is
ENV_CHANGED from two unraisable KeyError('__import__') reports for one never-
awaited coroutine. Failure capsule287d7efd retains12 refs and independently
replays3,105/113,10PASS+1ENV_CHANGED,37,714collections/0fallback. No closure,
retry or selective landing follows. Source94ae and products remain intact.

One deterministic ordinary-API probe confirms the mechanism without asyncio:
under disabled automatic GC, drop an unawaited coroutine and collect from a
lambda whose legitimate builtins dictionary is empty. Expected RuntimeWarning
is lost; two import KeyErrors are reported. Result83123698/postcleanup PASS
preserve actualoff/N0 and original products. Warnings retrieval imports using
the interrupted frame's builtins, so delayed cleanup can inherit an unrelated
restricted namespace. The two reports can come from the helper and fallback,
not two finalizer deliveries. Namedtuple constructors provide this valid context.

Roots owner will repair runtime-owned warning-module retrieval with cached
module lookup/dynamic attrs and deliberate absent-module import semantics;
stats independently reviews it. No test suppression or forced close hides it.
Coordinator supersedes selective-first landing: native owner composes reviewed
selective/native/scanner histories, then reviewed warning repair, and freezes
the actual union for one qualification. This avoids qualifying successive
intermediate combinations while preserving failed history and all backend,
lifetime and external-behavior obligations. No union native run has begun.

### 2026-10-30 17:46 PDT — hourly checkpoint: composed GC source and first build audit

Final candidate4d8e690 composes selective94ae, native709778, scannerb9d829
and warningb184. All source histories are preserved; actual merges had no code
conflicts. Independent composition review clears actual-kind gating, full frame
range semantics and the final warning join. Scanner retains coordinator/string
independent source review rather than self-clear by its author. Host root checks
on8C units and early required traverse audit pass.

O1 configure/compile/link and116 module checks pass, but the build's required
linked audit rejects outlined gen_close_clears_frame as conservatively collecting.
The owner stops before targets and preserves the failed build/capsule; source
4d8 remains frozen. Agents inspect its actual NoToolsForUnwind/InstrOparg call
graph and shared effect metadata; no forced inline or opaque waiver is granted.
No union native target or performance result is claimed. Focused12 plus retained
warning5, whole10, release/debug full/stress, applicableASan/tail, closure and
independent landing review remain. Original selective stress FAIL is retained.

Read-only interpreter review39c1a956 is copied into integration documentation
without importing runtime changes. IR/compiler/value/call paths match75e8
apart from the GC range view. Existing inline int arithmetic, value attribute
loads, compare truth folding and overlapping call frames are present. Five
ranked cuts cover shared polymorphic shape match, measured native NOARGS
conversion, constant producer facts, compare/branch contracts and shared frame
transitions. Source duplication is not assumed to survive compiler optimization;
H1/H2 remain archived absent a consumer. No IR runtime edit or opcode quota.

README/PROCESS/GC design describe the actual final union and supersede the
selective-first sequence. gc-designrev5/handover74 publish warning/storage
reflection; the checkpoint refreshes their source state. Last matched29 score
remains0.8243; selective coroutine1.309× remains the isolated four-row result.
Stats prepares reuse of quiet29/paired-four/bounded profiles after landing;
no benchmark campaign is active. Next hourly checkpoint18:44PDT, with material
changes reported sooner.

### 2026-10-30 17:55 PDT — precise effect fix; first behavior feedback passes

Three source reads and host classifier replay identify exact missing effect
knowledge: unmarked ir.c is outside the linked audit inventory, so immutable
_PyCode_InstrOparg is conservatively reentrant. Candidate17efee adds its exact
shared noncollecting declaration and a critical wrapper/unknown/collecting
regression. No predicate C/root/precision/inlining change. Six established
generators produce all seven tracked outputs byte-identically; independent
review clears source and the held warnings-existing/5 guard correction.

Accepted make-only successor completes O1/audit/116modulechecks and cleanup;
original make2/postguard7aa99 remains immutable. Actual closurea1e6370d binds
compiled programC4d8 with17ef source/effect/test authority. First feedbackf7f1e03c
passes focused12/0 and retainedwarning5/0 actualkill/N0,225/135collections,
zero fallback/unpinned and both cleanup. Independent raw/22-ref replay clears.
Whole10debugkill and fresh O3 configure/make run in the two normal admitted
slots with separate locks. No performance or full-union landing claim.

The tracked post-landing performance plan reuses quiet29, parent75e8/union
paired-four and four candidate32MiB hot profiles. Include five existing micro
instruction/elapsed diagnostics outside the kernel score. No extra32/64 sweep,
new timer/runner/production counter or current measurement launch. All29 spreads
use the prior100*(max/min-1) definition; serial samples remain correlated.

### 2026-10-30 18:18 PDT — four builds pass; early runtime coverage advances

Frozen candidate33944 retains program C/headers4d8 and adds exact effect metadata
and classifier regressions. O1, fresh O3, ASan and Clang-tail builds pass. The
tail audit's sole missing leaf was libc strrchr; its already-reviewed H1 tool
prerequisite is reused without importing H1 runtime changes. The failed tail ELF
passes the corrected audit unchanged; its original make failure remains recorded.
The earlier O1 decoder-effect audit failure is likewise retained.

Release and debugkill focused12 and whole10 pass. Both whole runs report one
diagnostic unpinned object, with zero fallback; focused cases report neither.
The remaining early smoke/root/backend checks run before the held broad ladder.
The ASan smoke recipe inherited ordinary filters and admitted an already-known
failing sanitizer-only case; operator/reviewer preserve its actual terminal
outcome before a minimal recipe correction. PROCESS already authorizes the exact
case exclusion; no broader exemption or new runtime diagnosis is inferred.

Canonical status documents now name33944 and the current evidence. A read-only
precise-edge review is copied from e8c082a73ff: trusted managed fields may avoid
revalidating already fully marked targets if the new profile selects that cost.
First marks, SHALLOW upgrades, native inputs and diagnostic modes retain full
validation. This is a proposal, with no new runtime edit or benchmark launch.
Independent source/document review clears these updates; the union remains
unlanded. Next hourly checkpoint remains18:44PDT.
