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
||||||| parent of 9641b189f38 (boil: worklog entries for step 4 milestones 1 and 2 and milestone 3 so far)

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
