# Proposal: register IR v2 packet (IR-D1, IR-D2, IR-D3/D4, G2–G6, G9, G10, G12)

Status: proposal, revision 2, 2026-11-04. Reviewed in
[review 1](../reviews/register-ir-v2-packet-review-1.md) (`d5fed44bd29`). The
coordinator's decisions on findings 3, 4, 6, 7 and 8 are built in. Every other
finding has a disposition below.

Design reference: [register-ir.md](../register-ir.md) §3 (IR-D1 to IR-D3,
G2–G12), §4 (bit budget and format list) and §5 (instruction contract).
Companion: [pyvalue-representation-packet.md](pyvalue-representation-packet.md),
whose steps 6a–9b are still to land.

**Source.** `build/worktrees/pyvalue-rc-universal` at `48bb4d6d1d3`, read
through `git show` and `git archive`, because other agents edit and build in
that working tree. The release build `build/pyvalue-rc-release-20261103`
reports `f3a20271f26`-dirty. `ceval.c` from `48bb4d6d1d3`, compiled with the
release flags, gives the same `ceval.o` text (595,778 B) and loop (407,484 B);
the review confirmed this independently. Upstream is
`build/upstream-release-3a440b6` (`3a440b65695`).

**Tools.**
- **Compilers.** GCC 15.2 and Clang 21.1.8 (`/usr/bin/clang-21`).
  - GCC has `musttail` and `no_callee_saved_registers`; it ignores
    `preserve_none`.
  - Clang has `preserve_none` and `musttail`, but not
    `no_callee_saved_registers`.
- **`perf`** is unavailable (`perf_event_paranoid` is 4).
- **Timing** comes from review 1. This revision timed nothing.
- **Scripts and raw output.** This proposal's are in
  `build/pyjit-link-wave1/irv2/`; the review's are in
  `build/pyjit-link-wave1/irv2-review/`.

**Out of scope.** Exception save and restore (representation steps 5–7),
value bridging (the value-API and native-boundary work), lazy feedback
allocation (G5 item 2), heat and OSR (G7), and the JIT-stage decisions
(IR-D4).

## Changes after review 1

| # | Severity | Finding | Disposition |
|---|---|---|---|
| 1 | major | The headline rests on static counts | **Accepted.** The summary and §1.1 lead with the review's timing: GCC tail calls alone run 0.870× the computed-goto build; computed goto runs 2.42× upstream. The static counts are now labelled as diagnosis (§1.2). The per-word costs are assigned to an owner: format, representation or structure (§1.4). §6.3's "3.2×" sentence is deleted. A re-split of the gap at HEAD with the sampler is a §8 measurement; this revision does no timing. |
| 2 | minor | The path measures favour the fork | **Accepted.** The shortest path is called the inline-scalar lower bound. `LOAD_FAST_BORROW`'s object path (about 61 instructions) is given. The per-variable fall-through counts are dropped. The handlers with no weight are listed. Object paths for the top ten handlers are a §8 measurement before A1; they were not measured here. |
| 3 | blocking | D4 has no step | **Accepted (coordinator).** New step A5 (D4) follows representation step 8 and is independent of the A4/B3 outcome. B3 keeps only the class bits. §11 reconciles IR-D3's wording with G2's rule. |
| 4 | major | A2 fixes the wrong contract | **Accepted, after checking the code (§0.2).** `_co_exec` is set once (`ir.c:312–326`) and cleared only from `code_dealloc` (`codeobject.c:2559`). Both writes through `_PyInstr **` (`ceval.c:1587`, `2175`) store the value `next_instr` already has. A2 deletes the out-parameter from all 12 helper families, keeping their `int` status. It also answers the representation packet's §3.4 hand-off: the in-helper re-instrumentation duplicates `instrument_all_executing_code_objects`, so A2 makes it a debug assert. The "may redirect the PC" column is dropped. |
| 5 | major | A3a's "no address-taken locals" lacks a plan for transaction records | **Accepted.** Box storage and the transaction records (`operation`, `call`, `pending`, `held_*`, `receiver`, `callable`, …) live in one activation record. The target is "no address-taken locals; frame at most N bytes", checked by `tcframes.py` on both compilers, not a canary count. A3a is re-estimated at 14 h. |
| 6 | major | Clang is installed | **Accepted (coordinator).** Corrected. A3a qualifies on GCC 15 and Clang 21. `Py_PRESERVE_NONE_CC` uses `preserve_none` where the compiler has it, then `no_callee_saved_registers`, then nothing (plain `musttail`). The JIT enters and leaves through C-ABI functions only. |
| 7 | major | Keeping computed goto is a second backend | **Accepted (coordinator).** A3c deletes the computed-goto and switch backends. `configure` rejects a compiler without `musttail`. Until A3 lands, computed goto stays the qualified build. |
| 8 | major | Ordering | **Changed.** A1 and A2 follow representation step 7, which edits their helpers. A3a, A3b and A3c land together after 9b. There is no measured reason to land A3a earlier: the 13% is not a gate, and landing it early would make steps 7–9b pass on two backends. The dual-CI cost is limited to the A3 branch and counted (2 h). `bytecodes.c` is listed for A3a. "A4 or B3 before B7" is dropped. "A2 before A3" is kept as natural, not forced. |
| 9 | major | Every format step needs its own magic number | **Accepted.** B2–B7 each bump `PYC_MAGIC_NUMBER`. Numbering is dense and regenerated in every step that adds or removes handlers (B2, B4, B6); there are no reserved holes. |
| 10 | major | The estimate is low | **Accepted.** Re-estimated per step at about 97 agent-hours (§10), with B3's prototype counted in every case. |
| 11 | minor | Errors in the V8/LuaJIT table | **Accepted.** §2 uses the review's corrections from the primary sources, and the "from memory" caveat is gone. |
| 12 | minor | Missed techniques: predecode, lookahead pairing, static operand roles | **Scope cut, listed as open questions 7–9.** The register budget is stated in A3a. |
| 13 | minor | B6 counted the deferred inversion; head shapes | **Accepted.** B6's column no longer counts inversion (fannkuch 5.4%). The head shapes are in §4.1, and the deferral stands under §4.3's fused-only rule. |
| 14 | minor | B7's bound includes window-fed constants | **Accepted.** The tighter bound is used: fannkuch 20.5%; geometric mean 0.66 without unpack_sequence. |
| 15 | minor | Limit errors are mostly done | **Accepted.** Corrected. B1 now changes only `regcfg.c`'s "too many temporaries" to `SyntaxError`, tested through a test-only limit knob. The "two live temporaries" check stays `SystemError`, because it is a compiler invariant, not a limit. |
| 16 | minor | Five literal masks, not four | **Accepted.** `Modules/_remote_debugging/code_objects.c:495` is added, with tests under B2 and B3. |
| 17 | minor | A4 is not one macro | **Accepted.** A4 passes `nlocalsplus` to the class test explicitly. Helpers outside the handlers read it from the executing copy's header. Risk is low to medium, estimate 3.5 h. |
| 18 | minor | Test sets | **Accepted.** `test_gdb` does not run on this host. A3a adds a debug tail-call build (frame assert) and a `Py_STATS` tail-call build. B7 adds `test_code`, copy and pickle. B2 adds the remote-debugging opcode check. A1 alone is recorded against HEAD. |
| 19 | minor | §11 edits | **Accepted.** §11 is rewritten with every item the review lists. |
| 20 | minor | `finish_value_bridges` is a twelfth escaping helper | **Accepted.** It is in C1 and A2. |

## Summary

- **Measured: tail calls alone cut the time by 13%.** These timings are from
  review 1 (§1.1): 5 interleaved rounds; each sample the best of 3 runs of
  `run(1)`; the median over rounds; load 1.1–3.7. On 12 kernels:
  - computed goto at HEAD runs **2.42× upstream**. This is the corrected
    baseline for this packet, replacing the triage's 2.6× on build F;
  - a tail-call link of HEAD with no other change runs **0.870×** computed
    goto under GCC (0.80–0.92 per kernel; repeat 0.872) and 0.877× under
    Clang 21, which is 2.10× upstream;
  - that removes about 16% of the log gap;
  - a ten-copy probe (`straight`) is 1–4% slower under GCC tail calls and 15%
    slower under Clang.
- **Diagnosis, from static counts.** The fork's handlers keep `next_instr`,
  `frame` and `slots` in stack homes, set up a `_PyValueTemps` record in
  every handler, and pass `next_instr` by address to 12 helper families.
  - On the inline-scalar lower bound (the shortest call-free path), a word is
    48.9 instructions with 12.1 stack references, against upstream's 22.0
    and 1.3.
  - In diagnostic builds, the record removal and tail calls take the stack
    references to 1.5 but leave 45.5 instructions.
  - The rest is representation work (C5: borrow tags, publish tests, D4 and
    step 8) and loop structure (bridges, transactions, the class test).
- **Who owns which cost.**
  - Structure: state transport, the record, the prologue. Phase A.
  - Format: the class test, the `NOT_TAKEN` skip. Steps A4 or B3, and B2.
  - Representation: borrow tags and publish tests. Step A5 (D4) and
    representation step 8.
  - The encoding's word savings are about 11%: 0.74 → 0.66 words per upstream
    instruction on 13 kernels.
- **Order.**
  - A1 and A2 follow representation step 7, and A5 (D4) follows step 8.
  - A3 (tail calls, the JIT entry, deletion of computed goto) is one landing
    after 9b.
  - A4, the G2 control, comes after A3.
  - The contract table (B0a) can start now. The format steps B2–B7 follow
    A3, so they change one backend.
- **Estimate:** about 97 agent-hours (§10).

---

## 0. State of the tree

### 0.1 Recount of register-ir.md

Every number the design gives for the code was recounted at `48bb4d6d1d3`.
Lib counts compile upstream `3a440b6`'s `Lib/` with the release build (2,071
files; 11 fail by design). Where a number changed, the code wins.

| Item | register-ir.md | Code at `48bb4d6d1d3` |
|---|---|---|
| `generated_ir_cases.c.h` | 18,772 lines, 292 `TARGET`s | **20,710 lines**, 292 `TARGET`s |
| Handler numbers; first-only | 305; 267 | 305; 267 |
| Valid formats / targets | – | 293 / 292 (`BINARY_OP` is valid but has no `TARGET` of its own) |
| Window handlers; role tuples; layouts; window formats; formats | 83; 9; 10; 19; 64 | the same |
| NOFB handlers; `FBARG`; `RUNS_CODE`; with an fb field; all four fields | 7; 87; 261; 167; 144 | the same |
| Class-test macro lines | 890 (1,040 with store and move) | 1,040 lines in **211 handlers** |
| `instr_ptr` in the prologue; value record | 289; 292 of 292 | the same; 177 of the 292 never box (§6.2) |
| `ir_displaced` / `finish_displaced_local` | 174 / 165 | 174 handlers / 165 calls in 159 handlers |
| `_PyValue_Publish`; `RECORD_BRANCH_TAKEN` | 184; 20 | 184 uses in 67 handlers; 20 |
| `ceval.o` text | 223 KB (interim build), upstream 107 KB | **595,778 B**; upstream 107,446 B |
| Evaluator function | – | `_PyEval_EvalFrameLoop` **407,484 B**; upstream `_PyEval_EvalFrameDefault` 58,419 B |
| Evaluator C frame | about 5.5 KB at `ca8a9d6d827`; upstream 360 B | **3,304 B**; upstream 360 B |
| Text per target | 0.76 KB | **1.40 KB** |
| Literal 0xFFFF handler masks | about 20 functions in 13 files | **5**: `pycore_ir.h:71` (`IR_HANDLER`), `pycore_ir.h:272` (`_PyIR_SetHandler`), `specialize.c:311`, `Lib/dis.py:413`, `Modules/_remote_debugging/code_objects.c:495` (out of process). Every other reader goes through `IR_HANDLER` (about 70 sites in 17 files) |
| Dead handlers | 5 | 5, with 0 static uses |
| Lib words; `NOT_TAKEN`; `PUSH_NULL` (all into temporaries) | 4,574,431; 80,618; 117,632 | **4,584,034**; 80,618; 117,632 |
| Constant loads; feedback sites | 855,432; 1,492,371 | the same, same kinds |
| Fusable compares; distances | 17,381 of 21,564 (+3,557 `CONTAINS_OP`, 1,960 `IS_OP`); within ±2,047 | the same; 22,840 within 127, 56 more within 511, 2 more within 2,047 |
| Jumps | 138,306 | 138,306 without `FOR_ITER` (18,168) and `SEND` (3,155); 48,248 backward with them |
| `LOAD_GLOBAL` NULL bits; `LOAD_FAST_BORROW` into a temporary | – | **93,334**; 232,433 |
| Unpacks; at most 8 elements; then stores; into consecutive locals | 9,871; 99.7%; 8,894; 8,057 | 9,871; 9,846 (99.75%); **8,887; 8,050** |
| `CALL` → `POP_TOP` | 218,006 | **229,553** counting every `CALL*` family (intrinsics and `CALL_FUNCTION_EX` included); 218,006 not reproduced |
| Per-code-object maxima | slots 803, entries 1,180, 4,480 units, 7,764 words, 4,048 constants, 368 names, compare sites 70 | unchanged |
| Limit errors | `SystemError` | **Mostly `SyntaxError`**: `assemble.c:716–720` (`too_large`) for words and opargs; a function with 5,000 locals raises `SyntaxError` (needs 5000 slots, limit 4095). Only `regcfg.c`'s "too many temporaries" (`slot_error`, line 790, called at 820) is a limit that still raises `SystemError` |
| Other stale items (G10) | docstring, `_PyFb_FAMILIES`, `IR_MAX_FORWARD` | still present: `ir_generator.py` docstring lines 55–63; `_PyFb_FAMILIES` lists `RESUME` and `JUMP_BACKWARD`; `IR_MAX_FORWARD` 2047 (`pycore_ir.h:69`) without a reason |

The 29-kernel geometric mean of 0.81 words per upstream instruction is from
the checkpoint. On the 14 kernels used here it is 0.78.

### 0.2 Facts the design does not state

- **The tail-call backend does not build at HEAD.**
  - `ceval.c:1337` includes the handlers before the helpers they call (lines
    1549–2140).
  - `ceval_macros.h` stops with `#error` unless the compiler has
    `preserve_none`, which GCC 15.2 lacks.
  - Two edits make it build under GCC (`irv2/tcpatch.py`): move the include,
    and use `no_callee_saved_registers` under GCC.
  - Linked with `irv2/tclink.sh`, it fails the same 9 test files as computed
    goto over the full suite. The only difference is `test_jit_method`
    (§9.1).
  - The review linked a Clang 21 `preserve_none` variant (`tccl.sh`). It
    gives the same 9 files and 33 failures.
- **The baseline method JIT is compiled out under tail calls**
  (`pycore_jit.h:157–158`, which also excludes `Py_STATS` and free
  threading). Its entry state is loop-local:
  - the locals `jit_flow`, `jit_request`, `jit_context`, `jit_cold_request`
    and `jit_from_return` (`ceval.c:2209–2214`);
  - the labels `jit_resume`, `jit_enter`, `jit_step` and
    `jit_consume_action` (`ceval.c:2290`, 2304, 2320, 2365).
- **The executing copy is fixed for the code object's life.**
  - `_PyCode_GetExec` creates `_co_exec` once, when it is NULL, under the
    code object's critical section (`ir.c:312–326`).
  - `_PyCode_ClearExec` (`ir.c:331`) is its only clearer. Its only caller is
    `code_dealloc` (`codeobject.c:2559`), and a running frame holds a
    reference to its code object.
  - `exec->words` points into the same allocation and is never reassigned.
    Instrumentation and specialisation rewrite handler fields in place.
  - The free-threaded build shares one copy (`pycore_ir.h:140–152`).
  - **What follows.** The only two writes through the helpers' `_PyInstr **`
    parameters store `exec->words + word` (`ceval.c:1587`) and
    `exec->words + state->next_word` (`ceval.c:2175`). Both offsets were
    taken from `next_instr` against the same `exec->words` when the
    operation began (`value_operation_begin`, `ceval.c:834`). The writes
    therefore store the value `next_instr` already has.
  - The `frame->ir_words = exec->words` refreshes at lines 1584, 1816, 2125
    and 2172 are identities in the same way.
- **The instrumentation refresh in the helpers duplicates
  `sys.monitoring`.** `value_operation_refresh` and `native_call_finish` call
  `_Py_Instrument(code)` when the version changed. But setting events already
  instruments every code object with a frame on any thread's stack
  (`instrument_all_executing_code_objects`, `instrumentation.c:2031`, called
  at 2116, 2201 and 2543), as upstream does. `DISPATCH` re-reads the
  rewritten handler.
- **Step 5 left the record in every handler.** Representation step 5 says
  handlers that box nothing drop `value_temps`. At HEAD all 292 set it up.
- **Release flags.** `-fno-omit-frame-pointer`, `-fcf-protection` (`endbr64`
  at each handler entry), `-fno-gcse`; Ubuntu GCC adds
  `-fstack-protector-strong`. Upstream's build has the same.

---

## 1. The loop cost

### 1.1 Timing (review 1)

Method:
- `irv2-review/scripts/interleave.sh` and `tbench.py`;
- 5 interleaved rounds per binary; each sample is the best of 3 runs of
  `run(1)`; the table uses the median over rounds;
- load 1.1–3.7 in run 1 and 2.2–3.4 in run 2, on 16 cores (`timing.tsv`,
  `timing2.tsv`).

The binaries:
- `cg`: the release binary, copied into the same layout;
- `tc`: the GCC tail-call link of HEAD with no other change (`tclink.sh`);
- `cl`: `ceval.c` built with Clang 21 under `preserve_none`, linked with the
  GCC objects;
- upstream `3a440b6`.

| Geometric mean, 12 kernels | Run 1 | Run 2 |
|---|---:|---:|
| GCC tail call / computed goto | 0.870 (per kernel 0.80–0.92) | 0.872 |
| Clang 21 tail call / computed goto | – | 0.877 |
| Computed goto / upstream | **2.418** | – |
| GCC tail call / upstream | 2.104 | – |

| Kernel | cg / upstream | GCC tc / cg |
|---|---:|---:|
| chaos | 2.43 | 0.853 |
| coroutines | 1.97 | 0.887 |
| deltablue | 4.52 | 0.902 |
| fannkuch | 2.49 | 0.869 |
| float | 2.07 | 0.900 |
| generators | 2.60 | 0.855 |
| go | 2.86 | 0.870 |
| nbody | 1.80 | 0.796 |
| nqueens | 2.15 | 0.861 |
| raytrace | 2.31 | 0.891 |
| richards | 3.05 | 0.920 |
| spectral_norm | 1.76 | 0.845 |

- **Tail calls alone help.** With every escape and every record still in
  place, they remove about 16% of the log gap (log 0.870 / log 2.418).
- **Tiny handlers get slower.** The `straight` probe (ten `b = a` per
  iteration) is 1.04× and 1.01× slower under GCC tail calls, and 1.15× slower
  under Clang. The likely cause is a function prologue in very small
  handlers.
- **The 39% is a share of build F, before step 5.** It is loop *self* time,
  so it includes the representation work inlined into the handlers (C5).
  This packet does not claim phase A removes all of it. A re-split of the
  gap at HEAD with the sampler is a §8 measurement.

### 1.2 Diagnosis: what the static counts show

These are counts of code shape, not time. They say where the time can come
from, not how much.

- **Handler addresses** come from the `opcode_targets` relocations
  (`labels.py`). The review reproduced them from DWARF labels for all 292
  and 231 handlers.
- **The inline-scalar lower bound** (`hmin.py`) is the shortest call-free
  path from handler entry to the next dispatch. In a handler that branches
  on the value, it is the inline-scalar case. An object value takes more: on
  `LOAD_FAST_BORROW`, the borrow-tag block, the `ob_type` load and two type
  compares, about 61 instructions against 44. Upstream's handler is 14
  either way.
- The fall-through measure of revision 1 is dropped. Its
  `LOAD_FAST_BORROW` path stored to a local, which no `LOAD_FAST_BORROW`
  does, so the per-variable reference counts taken from it (3.3, 3.5, 1.8,
  0.4, 4.1) are withdrawn.
- **Weights.** Handler entries per kernel come from the checkpoint's stats
  build (`checkpoint-20261103-C-kernels-boxes.json`, `dbce7b1260d`).
  `INSTRUCTION_STATS` counts only handlers below 256 (`ceval_macros.h:70–80`),
  so the IR-only handlers 267–304 carry no weight. They include:
  - `IR_JUMP`;
  - the per-operation `BINARY_OP_*` generics, which count as `BINARY_OP`, a
    name with no label (up to 1.7% of entries on raytrace);
  - the NOFB forms;
  - the `*_JIT_WARM` forms.
- **Variants** of `ceval.c`, compiled with the release flags:
  - `ni`: a short-lived copy replaces `&next_instr`;
  - `vt`: `ni`, plus no record in the 177 handlers that never box;
  - `tc`, `tcni`, `tcvt`: the GCC tail-call build, alone and with those two;
  - the review's `rep`: no publish canonicalisation, and `PyValue_BorrowAt`
    returns the slot bits. This approximates step 8 and D4. It does not
    preserve behaviour and was only compiled.

Weighted inline-scalar lower bound, per word (`weighmin*.md`; review
`out/sp-*.json`):

| Build | 78% of entries | 59% of entries (upstream has the handler) | Loop text |
|---|---:|---:|---:|
| Computed goto, HEAD | 53.0 / 12.3 | 48.9 / 12.1 | 407,484 B |
| cg + `rep` (step 8, D4) | – | 42.6 / 10.8 | 365,548 B |
| cg + `ni` + `vt` (A1, A2) | 50.0 / 9.3 | 45.4 / 8.6 | 333,672 B |
| cg + `vt` + `rep` | – | 40.8 / 8.3 | 304,791 B |
| Tail call | 57.4 / 6.8 | – | – |
| Tail call + `ni` | 55.9 / 5.4 | – | – |
| Tail call + `ni` + `vt` | 50.0 / 2.2 | 45.5 / 1.5 | – |
| Upstream | – | 22.0 / 1.3 | 58,419 B |

Each cell gives instructions / stack references per word.

What follows from the table:
- **The escape alone does not free the register under computed goto.** In
  `ni`, GCC still keeps `next_instr` in its stack home.
- **Under tail calls, the state is in argument registers.** There the escape
  and record removals take stack traffic to upstream's level.
- **The remaining instruction gap is representation and loop structure.**
  `rep` removes 16 of `LOAD_FAST_BORROW`'s 30 excess instructions (44 → 28).
  `STORE_FAST` stays at 45–46 in every variant.

### 1.3 `LOAD_FAST_BORROW` on the inline-scalar path

| Construct (`hcat.py`, computed goto) | Instructions |
|---|---:|
| `IR_STORE_DST`: class test against `nlocalsplus` (two dependent loads), write | 11 |
| `PyValue_BorrowAt`: borrow tag of an in-place read | 10 |
| `DISPATCH`: record count test, fetch, decode, jump | 7 |
| `_PyValueTemps` set-up | 4 |
| `_PyValue_Publish`: `PyLong`/`PyFloat` tests | 4 |
| `instr_ptr` store; `next_instr += 1` through its home | 2 + 2 |
| Other | 4 |
| **Total** (upstream: 14) | **44** |

Under `tcvt` it is 43 instructions with 2 stack references: `DISPATCH` 10,
prologue and canary 9, `IR_STORE_DST` 8, `PyValue_BorrowAt` 7, publish 4,
frame load 1, `instr_ptr` 1, other 3.

### 1.4 Named costs and their owners

| # | Cost | Owner | Handlers affected | Evidence | Removed by |
|---|---|---|---|---|---|
| C1 | `next_instr` passed by address | structure | 290 generated sites in 185 handlers, to 11 helpers (`finish_displaced_local` 165, `value_operation_finish` 38, `value_reader_complete` 28, `call_transaction_invoke` 26, `retire_published_values` 10, `call_transaction_expand_method` 7, `call_transaction_refresh` 6, `call_transaction_prepare_tuple` 4, `value_attribute_complete` 2, `native_call_finish` 2, `call_transaction_monitor` 2). The twelfth helper, `finish_value_bridges`, is reached from `DISPATCH` (`ceval_macros.h:272–281`), the three `JUMP_TO_LABEL`s and `DISPATCH_INLINED`, so it is in all 292 | One fixed DWARF home (`-0xc90(%rbp)`); tail call 6.8 → 5.4 stack refs with `ni` | A2 (delete the parameter), effective under A3 |
| C2 | One 407 KB function | structure | 292 | 4,772 calls against 611; 471 indirect jumps against 257; C frame 3,304 B against 360 B; under tail calls, stack refs fall to 1.5 with A1 and A2 | A3 |
| C3 | The value record in every handler | structure | 292, of which 177 never box | `vt`: 48.9/12.1 → 45.4/8.6; loop −18% | A1 |
| C4 | The slot-class test reads the code header | **format** | 1,040 lines in 211 handlers | 1.2 header loads per word, each behind `frame->ir_words` | A4 (`nlocalsplus` as state) or B3 (class bits); open question 9 (static roles) |
| C5 | Borrow tags and publish canonicalisation on the fast path | **representation** | Publish: 67 handlers; borrow macros: 590 lines generated | `rep`: 48.9 → 42.6 instructions; loop −10%; `LOAD_FAST_BORROW` 44 → 28 | A5 (D4) and representation step 8 |
| C6 | Fixed prologue: `instr_ptr`, frame load (tail call), canary, `endbr64`, frame pointer | structure | 289 (`instr_ptr`); 276 of 298 tail-call functions have a canary | 9 prologue instructions in `tcvt` `LOAD_FAST_BORROW` | A3a (frame parameter; no address-taken locals, so no canary); `instr_ptr` at escapes only is G9 item 1, after baseline |
| C7 | The `NOT_TAKEN` skip on every conditional fall-through | **format** | conditional jumps | 0–8.9% of words | B2 |
| C8 | Code size | all | all | mean 1,358 B per handler against 243 B; loop 7.0× upstream; 5.2× with A1, step 8 and D4 | A1, A3, A5, step 8 |

---

## 2. How V8 Ignition and LuaJIT keep state in registers

Review 1 checked these against LuaJIT v2.1 (`src/vm_x64.dasc`, `lj_bc.h`) and
V8 main (`src/codegen/x64/register-x64.h`,
`src/interpreter/interpreter-assembler.cc`, `bytecodes.h`).

| Technique | V8 Ignition | LuaJIT 2.1 | Fits a C interpreter? | Here |
|---|---|---|---|---|
| Fixed registers for dispatch state | accumulator `rax`, bytecode offset `r9`, bytecode array `r12`, dispatch table `r15` | x64 POSIX: `BASE` `rdx` ("Not C callee-save, refetched anyway"), `KBASE` `r15`, `PC` `rbx`, `DISPATCH` `r14` (each "Must be C callee-save"); x86 and Windows x64: `edx`, `edi`, `esi`, `ebx` | **Yes, through tail calls**: a handler's parameters are its fixed registers. Clang's `preserve_none` uses r12–r15, rdi, rsi, rdx, rcx, r8, r9, r11 and rax. GCC's `no_callee_saved_registers` uses the six SysV argument registers. | A3a |
| State across calls into C | registers are frame slots off the interpreted frame pointer; it reloads the bytecode array after every call "in case the debugger has been activated" (`BytecodeArrayTaggedPointer`) | `PC`, `KBASE`, `DISPATCH` in C callee-saved registers | **Yes.** Either attribute leaves `rbx` and `r12`–`r15` free for the compiler to carry state across calls. Here the executing copy never moves, so nothing needs reloading (§0.2). | A2, A3a |
| Operand decode during dispatch | operands loaded from the bytecode array by offset | `ins_NEXT` decodes A, D and OP before `jmp [DISPATCH+OP*8]` | **Yes.** Upstream predecodes `oparg` into `%r11d` under computed goto and passes it as a tail-call parameter. A3a passes `ir_word` and decodes after the jump; whether to predecode a field is a register-budget question (open question 7). | A3a |
| PC saved only when calling out | `SaveBytecodeOffset` in `CallPrologue`, unless `MakesCallAlongCriticalPath` | handlers store `mov SAVE_PC, PC` to a C-stack slot (`aword [rsp+aword*3]`) before calls that need it | **Yes**; the generator knows which uops escape | G9 item 1, after baseline |
| Dispatch-table register | `r15` | `DISPATCH`: one dynamic table and a static copy, with hooks re-dispatching through `[DISPATCH+OP*8+GG_DISP2STATIC]`; also the base for `global_State` and `jit_State` (`DISPATCH_GL`, `DISPATCH_J`) | **A choice.** Upstream CPython passes the table as a tail-call parameter (`instruction_funcptr_table`, `ceval_macros.h:75–79`) and switches it for trace recording (`ENTER_TRACING`, lines 146–157). Its computed-goto loop loads the table from the stack, which is upstream's one stack reference per word. The fork switches no table, so the table can be a constant; the sixth GCC register goes to A4 or to a predecoded field instead (open question 7). | A3a, A4 |
| Slow paths out of line | deferred labels | shared `->vmeta_*` stubs | **Yes**: cold helpers; under tail calls a slow path is its own function | A3a |
| Compare and jump | no fused bytecode (accumulator compare, then a jump) | ISLT consumes the following JMP (`add PC, 4`, then `PC_RD`) | **Yes**: a fixed fused form | B6 |
| Lookahead pairing | inlines a following short `Star` (`StarDispatchLookahead`) | ISLT with JMP, as above | open question 8 | – |
| Accumulator | implicit operand | – | **No**: a second operand model | §7 |
| Handlers in a macro assembler | CodeStubAssembler / TurboFan | DynASM | **No**: a second handler language | §7 |
| Short forms, operand-kind opcodes | `Star0`–`Star15`, `LdaSmi`, `AddSmi` | `ADDVN`, `KSHORT` | **No**: they multiply handlers (G6's experiment) | §7 |
| Variable-width operands | `Wide`/`ExtraWide` prefixes, with separate handler-table ranges (256–511, 512–767), tripling handlers | – | **No**: two encodings of one operation | §7 |

Calling conventions:
- **GCC.** The current manual documents an x86 `preserve_none` that passes
  integer parameters in r12, r13, r14, r15, rdi and rsi and "is subject to
  change". GCC 15.2 lacks it, and the work on it (PR 119628) cites CPython.
- **Clang.** It documents `preserve_none`'s ABI as unstable.

Hence A3a's rule: handlers use an internal convention, and everything else,
the JIT included, enters and leaves through C-ABI functions.

---

## 3. Ordering against the representation packet

| Representation step | Files that matter here |
|---|---|
| 6a–6c | sweep sites; 6b rewrites the release helpers in `ceval.c`'s bridge region |
| 7 | deletes `_PyValue_RestoreException` and its 33 uses in `ceval.c`, inside the bridge helpers (`drop_failed_value` 1546; `call_transaction_close_value` and `_object` 1683, 1693; line 1840); `DISPATCH_INLINED` in `ceval_macros.h`; 2 uses in `bytecodes.c` |
| 8 | constructors; `generators_common.py:POINTER_CONSTRUCTORS`; regenerated files |
| 9a | `pycore_value.h`, the numeric handlers in `bytecodes.c`, `specialize.c`, the compiler's inline-int users |
| 9b | `jit_*.c`, `jit_x86.h`, `pycore_jit*.h` |

| IR v2 step | Starts after | Why |
|---|---|---|
| A1, A2 | rep. 7 | Step 7 edits the bridge helpers whose signatures A2 changes, and `ceval_macros.h`'s dispatch macros, which A1 changes |
| A5 (D4) | rep. 8 | D4 needs canonical construction: a value stored in a home must be canonical before its tag means only "immortal or not". It shares `pycore_value.h` with 9a, in different functions; any order with 9a and 9b |
| A3a + A3b + A3c, one landing | rep. 9b (and A2) | A3a's slot-addressed rewrites edit `bytecodes.c` handler bodies that 9a rewrites; A3b edits `jit_runtime.c`, which 9b rewrites. Landing them together means no representation step has to pass on two backends (coordinator decision on finding 8) |
| A4 | A3 | It needs the parameter list |
| B0a | rep. 6c | A new generated table and consumers in `assemble.c`, `regcfg*.c`, `ir.c`, `specialize.c` and `dis`; no format change |
| B0b | A3 | The JIT front end, after 9b's rewrite and A3b's entry change |
| B1 | any time | Independent |
| B2–B7 | A3 | IR-D1 puts them after 9b. After A3c there is one backend, so each format step edits one dispatch form. B6's `_INT` forms compare int51 values, and B7's slots hold inline values, whose range int51 sets |

Forced orders inside the packet:
- **B2 first among the format steps:** `NOT_TAKEN` and the dead handlers.
- **B4 before B6:** the fused entry is G5's 32-byte layout.
- **A4 before B3:** G2's rule.
- **A2 before A3** is natural, not forced. A3a builds without it, but its
  target (no address-taken state) includes `next_instr`.
- **B7 needs no class step.** Today's run-time test already treats hidden
  slots below `nlocalsplus` as frame variables.

---

## 4. Steps

Landing order: A1, A2 → A5 → A3 (a, b, c together) → A4 → B2 → B3 (if
needed) → B4 → B5 → B6 → B7, with B0a in parallel from now, B0b after A3, and
B1 at any time.

| Step | Content | Gaps | Words executed | Per-word or code effect | Agent-h |
|---|---|---|---|---|---:|
| A1 | The record only in handlers that box | G9.3 | 0 | `vt` against `ni`: 45.4/8.6 from 48.9/12.1 on the lower bound; loop −18% | 2 |
| A2 | Delete the PC out-parameter (12 families); instrumentation refresh becomes an assert | C1 | 0 | none alone under computed goto; under tail calls 6.8 → 5.4 stack refs | 2.5 |
| A5 | D4: no home holds a borrow | IR-D3, D4 | 0 | with step 8: `LOAD_FAST_BORROW` 44 → 28, loop −10% (`rep`); plus one count pair per `LOAD_FAST_BORROW` into a temporary (3.8–8.5% of words) | 6 |
| A3a | Tail-call build with no stack state, GCC 15 and Clang 21 | §1.13, G9.6 | 0 | measured 0.870× with nothing else changed; stack refs 12.1 → 1.5 with A1 and A2 | 14 |
| A3b | JIT entry and exit as C-ABI functions | §1.13 | 0 | – | 9 |
| A3c | Delete the computed-goto and switch backends | finding 7 | 0 | one decode, one dispatch form | 2 |
| – | Both suites on the A3 branch until A3c | finding 8 | – | – | 2 |
| A4 | G2 control: `nlocalsplus` in dispatch state | G2 | 0 | −1.2 header loads | 3.5 |
| B0a | Contract table; C consumers; `dis` | IR-D2, G1 | 0 | 0 | 11 |
| B0b | JIT front end reads the table | IR-D2 | 0 | 0 | 3 |
| B1 | Last limit `SyntaxError`; docstring; `IR_MAX_FORWARD` | G10 | 0 | 0 | 1.5 |
| B2 | No `NOT_TAKEN`; dead handlers out; dense numbering; magic | G12 | 0 | a load and a compare less on 0–8.9% of words | 3 |
| B3 | Class bits (prototype always; lands only if A4 loses); magic | G2 | 0 | −1.2 header loads | 5 |
| B4 | Feedback layout and index; NOFB out; renumber; magic | G5.1, G10 | 0 | feedback memory −22%; −6 handlers | 9 |
| B5 | Call and unpack forms; magic | G4 | −0.7% to −8.1% (unpack_sequence −91%) | unpack store loop | 7 |
| B6 | Fused compare-and-jump, forward only; jump rule; renumber; magic | G3, §4.3 | 0 to −7.4% | one dispatch, slot write, read and NULL store less per pair | 7 |
| B7 | Inline scalar constants as hidden slots; magic | G6 | 0 to −20.5% | entry stores in looping functions | 6.5 |
| M | Measurements (§8) | – | – | – | 3 |

### 4.1 Expected words executed

The source is `irv2/steps.py` over `irv2/dyn-v2/`, with the window-fed
constants from review `mydyn2.txt` (`irv2/steps-r2.md`). A step's column is
the words of the current IR that the new form removes, if the compiler
applies it at every counted site.

| Kernel | U instructions | C words | B5 | B6 | B7 | C/U now | after B5 | after B6 | after B7 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| chaos | 10,532,435 | 7,321,871 | 0.7% | 2.3% | 4.2% | 0.70 | 0.69 | 0.67 | 0.65 |
| comprehensions | 3,223 | 2,417 | 2.4% | 0.8% | 0.8% | 0.75 | 0.73 | 0.73 | 0.72 |
| coroutines | 3,884,596 | 3,277,628 | 7.4% | 7.4% | 7.4% | 0.84 | 0.78 | 0.72 | 0.66 |
| deltablue | 726,098 | 545,927 | 4.0% | 6.0% | 0.1% | 0.75 | 0.72 | 0.68 | 0.68 |
| fannkuch | 70,923,145 | 53,069,157 | 3.5% | 5.4% | 20.5% | 0.75 | 0.72 | 0.68 | 0.53 |
| float | 12,199,935 | 7,899,942 | 1.3% | 3.8% | 0.0% | 0.65 | 0.64 | 0.61 | 0.61 |
| generators | 6,806,866 | 6,106,863 | 0.0% | 0.0% | 2.2% | 0.90 | 0.90 | 0.90 | 0.88 |
| go | 28,667,983 | 18,762,468 | 2.2% | 7.1% | 2.5% | 0.65 | 0.64 | 0.59 | 0.58 |
| nbody | 28,581,241 | 19,300,999 | 7.8% | 0.0% | 8.8% | 0.68 | 0.62 | 0.62 | 0.56 |
| nqueens | 14,592,232 | 11,850,310 | 0.7% | 2.0% | 4.5% | 0.81 | 0.81 | 0.79 | 0.75 |
| raytrace | 41,877,624 | 29,606,956 | 3.7% | 0.9% | 0.5% | 0.71 | 0.68 | 0.67 | 0.67 |
| richards | 9,784,097 | 5,789,061 | 0.7% | 1.5% | 0.5% | 0.59 | 0.59 | 0.58 | 0.58 |
| spectral_norm | 19,045,221 | 17,011,597 | 8.1% | 0.0% | 0.0% | 0.89 | 0.82 | 0.82 | 0.82 |
| unpack_sequence | 5,679 | 9,667 | 91.1% | 0.0% | 0.0% | 1.70 | 0.15 | 0.15 | 0.15 |
| **Geometric mean** | | | | | | **0.78** | **0.64** | **0.62** | **0.59** |
| Without unpack_sequence | | | | | | 0.74 | 0.71 | 0.69 | 0.66 |

- **B5** counts:
  - `POP_TOP` of the result of the `CALL*` just before it (not the
    intrinsics);
  - `PUSH_NULL`;
  - the `STORE_FAST`s of an unpack into consecutive locals;
  - the `LOAD_FAST_BORROW` of the unpacked sequence or appended value.

  Review 1's classifier matches on seven kernels. On go it counts 393,631
  against 409,593 here, a difference of 4%.
- **B6** counts compares whose result is the condition of the next
  conditional jump. It no longer counts loop inversion.
- **Loop inversion is deferred.** The loop heads that it could remove a back
  edge from are:
  - compare heads: fannkuch 260,650, chaos 5,030, go 133;
  - fannkuch: 260,650 more after a constant load (`while r != 1:`);
  - truth-test heads: fannkuch 1,731,245 (3.3%), richards 106,950 (1.8%),
    nqueens 40,319.

  §4.3 allows backward conditional jumps only in the fused form, so
  truth-test heads would need a plain backward jump, which the design
  excludes. The deferral stands.
- **B7** counts inline scalar constants loaded inside a loop, less those a
  call or build window reads, which keep their load. For fannkuch that is
  4,086,019 `BUILD_SLICE` inputs, for go 45,854. It is still an upper bound
  on the compiler's coverage.

### Step A1. The value record only where a handler boxes

- **The change.** `ir_generator.py` emits `value_owners`, `value_temps` and
  the `DISPATCH` count test only in the 115 handlers whose body boxes
  (`vtemps.py`; the review confirms the split with a different classifier).
  - In the 177 others, the record reaches only `finish_displaced_local` (81
    handlers) and `assert`s.
  - `finish_displaced_local` takes no record there.
  - `finish_value_bridges` folds away, because `value_temps` is NULL.
- **Check the attribute handlers.** The ten specialised attribute handlers
  (`LOAD_ATTR_INSTANCE_VALUE`, `_MODULE`, `_SHAPE`, `_SLOT`, `_WITH_HINT` and
  the five `STORE_ATTR_*`) guard an object receiver. Check whether they box
  only through the shared completion helper (open question 5).
- **Debug check.** `IR_KEEP_BOX` asserts that a record exists.
- **Test.**
  - The step-0 set.
  - `test_pyvalue_call_bridges`, `test_pyvalue_repr_cleanup`.
  - A generator test that no record is emitted where the classifier says
    none.
  - The debug suite.
  - Record A1 alone against HEAD on the lower bound when it lands.
- **Risk.** Low.

### Step A2. Delete the PC out-parameter

- **The change.** All 12 helper families take `next_instr` by value, or not
  at all, and keep their `int` status:
  - `finish_displaced_local`, `value_operation_finish`,
    `value_reader_complete`, `retire_published_values`,
    `value_attribute_complete` and `native_call_finish`;
  - the five `call_transaction_*` helpers;
  - `finish_value_bridges`.
- **The writes go.** The two writes through `_PyInstr **` (`ceval.c:1587`,
  `2175`) and the four identity refreshes of `frame->ir_words` and
  `ir_feedback` go (§0.2).
- **Sites.**
  - 290 generated sites from 10 emitters in `ir_generator.py` (lines
    1288–2113).
  - 5 macro sites in `ceval_macros.h` (`DISPATCH`, three `JUMP_TO_LABEL`s,
    `DISPATCH_INLINED`).
  - 2 in `bytecodes.c`.
- **Kept.** Each family's validation (frame, code and range checks that raise
  `SystemError`) and its return status stay. Representation §3.4 decided to
  keep them.
- **The answer to representation §3.4's hand-off.**
  - The `_Py_Instrument` call in `value_operation_refresh` and
    `native_call_finish` becomes a debug assert that the code's
    instrumentation version is current. `instrument_all_executing_code_objects`
    already re-instruments every executing code object when events change.
  - If the debug suite fires the assert, the refresh stays and the case is
    recorded. No dispatch-time check is added.
- **New invariant**, asserted after each reentrant call in debug builds:
  `frame->ir_words == _PyFrame_GetCode(frame)->_co_exec->words`. register-ir.md
  §1.10 states it (§11).
- **Test.**
  - The step-0 set.
  - The step-5 tests: a `__del__` or native call that calls
    `sys.monitoring.set_events` or `sys.settrace` takes effect at the next
    instruction.
  - `test_monitoring`, `test_sys_settrace`, `test_bdb`.
  - A generator test that no emitted line takes `&next_instr`.
- **Risk.** Low.

### Step A5. D4: no home holds a borrow

- **Decided, not optional.** IR-D3 and pyvalue.md D4 decided this. It lands
  whichever of A4 or B3 wins: D4 concerns value tags, not slot classes.
- **The change.**
  - A frame slot never holds a borrow. `LOAD_FAST_BORROW` into a temporary
    takes a count for a mortal object; inline values and immortals take
    none.
  - A consumer of a temporary owns what it consumes, and releases it by the
    immortal test alone.
  - An in-place read of a local produces a borrow only in the handler's C
    local: `PyValue_BorrowAt` returns the slot bits, and `IR_BORROW_KNOWN`,
    `IR_LOAD_BORROWED` and `IR_DROP_BORROW` lose their class-dependent tag.
  - The value tag means only "immortal or not".
- **Files.** `ceval_macros.h` (the five borrow macros), `pycore_value.h` (23
  borrow-tag uses), `ir_generator.py` (4), `bytecodes.c` (3), and
  `stack.py` and `analyzer.py` (1 each). The generated files are
  regenerated: 590 and 91 lines.
- **Cost and benefit.**
  - One count pair per `LOAD_FAST_BORROW` into a temporary, on 3.8–8.5% of
    words (`load_fast_borrow_to_temp`, register-ir.md G2).
  - With step 8's publish change, the review's `rep` approximation takes
    `LOAD_FAST_BORROW` from 44 to 28 instructions and the loop down 10%.
- **Test.**
  - The step-0 set; `test_tagged_values`.
  - `test_sys` (`getrefcount` around calls), `test_gc`, `test_weakref`,
    `test_finalization` (lifetimes of values held by temporaries).
  - `test_generators` and `test_frame` (frame copies), `test_peepholer`.
- **Risk.** Medium. It changes when references are taken on the hottest
  path.

### Step A3a. Tail-call build with no stack state

- **Build.** `Py_PRESERVE_NONE_CC` is chosen in this order:
  - `preserve_none` when `__has_attribute(preserve_none)` (Clang, and GCC
    once it ships the attribute);
  - otherwise `no_callee_saved_registers` (GCC on x86);
  - otherwise nothing, which leaves plain `musttail`: one source, slower.

  `configure` rejects a compiler without `musttail`. The handlers are
  included after the helpers, or the helpers are declared in a header.
- **Parameters: the register budget.**
  - `frame, slots, tstate, next_instr, ir_word` take five of GCC's six
    argument registers.
  - Under GCC the sixth is held for the G2 decision (A4). The alternatives
    are a predecoded field or the dispatch table (open question 7).
  - Under Clang twelve are available.
  - `TAIL_CALL_LOAD_FRAME` goes. Handlers that change frames pass the new
    frame.
- **No address-taken locals in any handler.**
  - One activation record holds the box area (three boxes) and every
    transaction record that handlers now pass by address:
    | Record | Handlers |
    |---|---:|
    | `operation` (`_PyValueOperation`) | 38 |
    | `pending` | 29 |
    | `held_left`, `held_right` | 27 each |
    | `call`, `call_child` | 15 each |
    | `receiver` | 15 |
    | `callable` | 12 |
    | `publication`, `scope` | 1 each |
  - Handlers never nest within one activation, so one record per activation
    serves them. The record lives in the entry function's frame and is
    reached through `tstate` (pushed and popped at eval entry and exit), so
    nested evaluations get their own. A per-thread buffer would not survive
    nesting.
  - Values held there sit in one known place per activation. G11's open risk
    is that the free-threaded collector cannot see values held only in C
    variables. A collector could scan this record, but A3a does not add that
    scan; it stays with free-threaded qualification.
  - The remaining `&res` and `&left` become slot-addressed helpers
    (`&slots[F]`), as G9.6 says. This edits handler bodies in `bytecodes.c`.
- **One field decode.** Every handler decodes from `ir_word`. The `IR_FIELD_*`
  memory re-read goes with A3c.
- **Target.** No handler has an address-taken local, and every handler's
  frame is at most N bytes. `tcframes.py` checks this on both compilers. N
  is the value measured when A3a lands and may only fall.
  - Today the median frame is 160 B and the largest 512 B under GCC; under
    Clang the median is 144 B and the largest 464 B.
  - Meeting the target also removes GCC's canaries, because
    `-fstack-protector-strong` adds one only to a function with a local array
    or an address-taken local.
- **Test.**
  - The full suite on GCC 15 and Clang 21, with the same failure set as
    computed goto, `test_jit_method` aside until A3b.
  - A debug tail-call build: `PRE_DISPATCH_GOTO` asserts
    `frame == tstate->current_frame`.
  - A `Py_STATS` tail-call build: `lastopcode` is a seventh argument, on the
    stack under GCC.
  - `test_sys` and `test_call` recursion tests, `test_perf_profiler`,
    `test_external_inspection`.
  - `test_gdb` does not run on this host: gdb is not installed, and the
    tests "only work on source builds". The stack-walking check needs a
    source build on a host with gdb.
- **Measured so far.** The unchanged tail-call link runs 0.870× (GCC) and
  0.877× (Clang) of computed goto. That is a measurement, not a gate.
- **Risk.** Medium. The calling conventions are documented as unstable, so
  they stay internal (A3b).

### Step A3b. JIT entry and exit as C-ABI functions

- **The change.**
  - The loop-local JIT state (§0.2) moves into the activation record.
  - `jit_enter`, `jit_resume`, `jit_step` and `jit_consume_action` become
    ordinary C functions that the `JIT_SITE_ENTRY` handlers call.
  - Compiled code enters and leaves only through C-ABI functions, because
    the two handler conventions put arguments in different registers.
  - `_PyJIT_BASELINE_SUPPORTED` no longer depends on `_Py_TAIL_CALL_INTERP`.
- **Test.** The JIT modules (`test_jit_method` and the representation
  packet's step-0 survey), with admission forced on, on both compilers.
- **Risk.** Medium to high. This is the one part that changes JIT control
  flow.

### Step A3c. One backend

- **The change.**
  - The generator emits only tail-call handlers.
  - Delete the computed-goto backend (`opcode_targets`, `DISPATCH_GOTO`'s
    goto form, the loop's labels) and the switch fallback
    (`ceval_macros.h:170–188`).
  - Delete the `IR_FIELD_*` memory decode.
  - The tail-call build becomes the qualified and measured build.
- **Why delete it.** A platform that needs another backend must be named,
  with the measurement that forces it. None is known.
- **Test.** The full suite on both compilers, and the docs build.
- **Risk.** Low, once A3a and A3b pass.

### Step A4. G2 control: `nlocalsplus` in dispatch state (experiment)

- **The change.** The class test takes `nlocalsplus` explicitly:
  `IR_IS_LOCAL(F)` becomes a test against a value the caller supplies.
  - Handlers get it as a parameter, the sixth under GCC, set wherever the
    frame changes.
  - Out-of-line code reads it from the executing copy's header, as today.
    This covers the 4 uses in `generated_jit_helpers.c.h`, the one in
    `ceval.c` outside the handlers, and `value_operation_restore2`
    (`ceval.c:841`), which reads `co_nlocalsplus` itself.
  - The revision-1 variant (`nlp`) failed with 65 errors because it did not
    separate these.
- **Decision.** A matched elapsed comparison against B3's prototype, on a
  quiet machine (G2's rule).
  - If A4 recovers the cost, A4 stays and the bits are not built.
  - Otherwise B3 lands and A4 is reverted.
  - Open question 9 (static operand roles) should be counted first.
- **Risk.** Low to medium. Under GCC it costs the last argument register.

### Step B0a. The instruction contract table

See §5. This step generates the table, replaces `_PyFb_FAMILIES` and the
passes' own flag tests, and makes the verifier check folded loads against
"may run Python" with its operand rules (G1's caveat).

- **Test.**
  - `test_ir_contract`: rows agree with the stack effects, the analyzer's
    properties and the feedback kinds.
  - `test_peepholer`, `test_ir`, `test_dis`.
  - `_PyRegCfg_Verify` over the stdlib in a release test job.
- **Risk.** Medium.

### Step B0b. The JIT front end reads the table

`_PyJitHomeEffect` goes. `jit_ir.c`, `jit_record.c` and `jit_baseline.c` read
the table. Heap effects, guards and result facts are added here, as
`bytecodes.c` annotations.

- **Test.** The JIT modules.
- **Risk.** Medium.

### Step B1. The last limit error, the docstring, `IR_MAX_FORWARD`

- **The limit.** "Too many temporaries" (`regcfg.c:820` through
  `slot_error`) raises `SyntaxError` naming the limit.
  - No Lib source reaches it, so it is tested through a test-only knob,
    `_PyIR_MaxSlots`, as `_PyIR_MaxForward` is for relaxation.
  - The other `slot_error` calls (`regcfg.c:841`, 883, 887, 962) and the
    verifier's "two live temporaries in one slot" (`regcfg.c:1279`) stay
    `SystemError`: they are compiler invariants, not limits.
- **The docstring.** Correct the `ir_generator.py` docstring (lines 55–63).
- **`IR_MAX_FORWARD`.** State its reason, or set it with B6's signed rule.
- **Risk.** Low.

### Step B2. First format step: no `NOT_TAKEN`, dense numbering

- **The change.**
  - Remove `NOT_TAKEN` (G12). The instrumented twins of the conditional
    jumps fire `BRANCH_LEFT` and `BRANCH_RIGHT` with the jump's location.
  - Remove the five dead handlers and renumber densely.
  - Bump `PYC_MAGIC_NUMBER` (64402 today).
- **Test.**
  - `test_monitoring` (branch events at the jump's offset: a visible
    change), `test_dis`, `test_compile`.
  - `test_external_inspection`: `_remote_debugging`'s opcode output, which
    masks words itself, reports the new numbers.
- **Expected effect.** 80,618 static words fewer; a load and a compare less
  on each conditional fall-through, which is 0–8.9% of words (richards 8.9%,
  fannkuch 6.8%, go 5.5%).
- **Risk.** Low to medium.

### Step B3. Class bits (prototype always; lands only if A4 loses)

- **The prototype.** G2's rule needs it for the comparison whichever way the
  decision goes.
- **The change.**
  - The handler uses bits 0–11 and the class bits 12–15.
  - The five literal masks become 0xFFF, the remote-debugging reader
    included.
  - `IR_WORD` sets the bits, and `_PyIR_SetHandler` keeps them.
  - The verifier checks them against `nlocalsplus`.
  - Bump the magic number.
- **Test.** `test_ir`, `test_dis`, `test_peepholer`, the verifier over the
  stdlib, and `test_external_inspection`.
- **Risk.** Moderate.

### Step B4. Feedback layout and index; NOFB removed

- **Precondition.** The generated-corpus maximum of compare and contains
  sites per code object.
- **The change.** As G5 item 1 and G10 option (c):
  - 8-byte units, per-kind sizes, priority-ordered assignment, a header-only
    entry 0;
  - names in D or CD;
  - a second `LOAD_ATTR` head;
  - 14 intrinsics for compare sites past the limit;
  - NOFB goes (−7 handlers, +1);
  - renumber and bump the magic number.
- **Test.** `test_ir`, `test_dis`, `test_opcache`, the JIT feedback readers,
  and a code object past 4,095 units.
- **Risk.** Moderate.

### Step B5. Call and unpack forms (G4)

- **The change.** As G4 items 1, 2 and 4, and item 3 for `LIST_APPEND`,
  `SET_ADD` and `MAP_ADD`.
  - The `LOAD_GLOBAL` NULL bit goes with `PUSH_NULL`.
  - Unpacks past 8 elements collect in an out-of-line helper.
  - Bump the magic number.
- **Test.**
  - `test_unpack`, `test_unpack_ex`, the comprehension tests,
    `test_extcall`, `test_call`.
  - A failure in `__next__` part-way through leaves the locals unchanged.
  - The verifier proves the omitted `PUSH_NULL` slot NULL.
- **Expected effect.** §4.1 B5.
- **Risk.** Small to moderate.

### Step B6. Fused compare-and-jump and the one jump-target rule

- **The change.** As G3 and §4.3, but forward only. Conditional back edges
  are deferred (§4.1), so the fused handlers never poll the eval breaker.
  - `FOR_ITER` and `SEND` use the signed 12-bit distance.
  - 11 new handlers. Renumber and bump the magic number.
  - Fusion is declined when the compare and the jump are on different lines.
- **Test.** `test_compile`, `test_peepholer`, `test_dis`, `test_monitoring`,
  `test_sys_settrace`.
- **Expected effect.** §4.1 B6.
- **Risk.** Moderate.

### Step B7. Inline scalar constants as hidden slots (G6)

- **The change.** As G6, except that the table stores `(slot, co_consts
  index)`. The executing copy (`make_exec`) writes the values, so marshal
  stores no `PyValue` bits. Bump the magic number.
- **Test.**
  - `test_marshal`, `test_py_compile`, `test_compileall`.
  - `test_code`: `code.replace(co_consts=…)` rebuilds the table.
  - Copying and pickling code objects.
  - `test_frame` and `test_inspect` (`f_locals` hides the slots),
    `test_dis`, generators.
- **Expected effect.** §4.1 B7.
- **Risk.** Moderate.

---

## 5. The instruction contract table (IR-D2)

- **Form.** One table: `_PyIR_Formats` grown into the contract. Today's rows
  have `flags`, `roles`, `imm`, `fb`, `ninputs`, `dst`, `borrow_mask`,
  `shadow` and `nofb`. They gain the §5 columns:
  - per-field roles, including the slot roles hidden in `IMM`;
  - window extents (base field plus a fixed count, or an oparg field plus k);
  - consumed or in place, per input;
  - effect bits: raises, runs Python, deoptimises, jumps, suspends, pushes a
    frame, polls, allocates, releases the old dst;
  - "runs Python" operand rules: releases a local through `DST`, `LOCAL_OUT`
    or a window output;
  - feedback kind and layout;
  - target fields and their encodings;
  - the monitoring twin and its events;
  - pass-through inputs;
  - one new column, **boxes inputs**: the handler needs the value record
    (A1).

  Heap effects, guards and result facts come with B0b.
- **Where.** Generated by `ir_generator.py` into `pycore_ir_metadata.h`.
  Python reads it through `_opcode.get_ir_contract()`, grown from
  `get_ir_formats()`. There is no generated Python copy. This settles O1.
- **Consumers** (counts at HEAD):

  | Consumer | Uses today |
  |---|---|
  | `Python/ir.c` decoders | `_PyIR_FormatOf` 10, flags and roles 6 |
  | `Python/assemble.c` | `_PyIR_FormatOf` 3, flags and roles 12, `_PyFb_FAMILIES` 2 |
  | `Python/regcfg.c` (conversion, compaction, verifier) | `_PyIR_FormatOf` 3, flags and roles 10 |
  | `Python/regcfg_passes.c` | flags 7 |
  | `Python/specialize.c` | `_PyIR_FormatOf` 1 |
  | `Include/internal/pycore_ir.h` | flags and roles 12 |
  | `Lib/opcode.py` → `Lib/dis.py` | `_opcode.get_ir_formats()` |
  | JIT (`jit_ir.c`, `jit_record.c`, `jit_baseline.c`) | `_PyJitHomeEffect`, `_PyIR_FormatOf` 1, flags 2 |
  | `ir_generator.py` | boxes inputs (A1), slot-addressed helpers (G9.6) |
  | Verifier | every row, from B2 on |

- **Step.** B0a builds it with the v1 rows. Each format step updates its rows,
  and the verifier checks the step against them. This also answers §4's
  churn argument: the decoders read the table, so a format step changes the
  table and the handlers, not every decoder by hand.

---

## 6. G9: handler count, code size and dispatch

### 6.1 Today against upstream

| Measure | Fork | Upstream |
|---|---:|---:|
| Handlers with a `TARGET` | 292 | 231 |
| `ceval.o` text | 595,778 B | 107,446 B |
| Evaluator function | 407,484 B | 58,419 B |
| Mean / median bytes per handler region (`hsize.py`) | 1,358 / 1,362 | 243 / 196 |
| Evaluator C frame | 3,304 B | 360 B |
| Calls / indirect jumps in the evaluator | 4,772 / 471 | 611 / 257 |

| Handler | Fork bytes | Upstream bytes |
|---|---:|---:|
| `LOAD_FAST_BORROW` | 866 | 56 |
| `STORE_FAST` | 496 | 88 |
| `BINARY_OP_ADD_INT` | 1,948 | 239 |
| `LOAD_ATTR_INSTANCE_VALUE` | 2,365 | 206 |
| `COMPARE_OP_INT` | 1,616 | 294 |
| `RETURN_VALUE` | 1,040 | 160 |
| `CALL_PY_EXACT_ARGS` | 961 | 378 |

The tail-call functions:
- **GCC `tc`.** 298 functions, 450,721 B. The median frame is 160 B, the
  largest 512 B, and 276 have a canary.
- **GCC `tcvt`.** 387,461 B; median frame 80 B; 200 canaries. 86 of the 177
  never-box handlers still have one, from transaction records and the `ni`
  copy.
- **Clang 21.** No canaries; median frame 144 B, largest 464 B.

### 6.2 Which handlers can drop `value_temps`

- **Now: 177 of 292.** This is A1.
- **The 115 others** pass an inline input to an object API:
  - generic `BINARY_OP` operations, 28;
  - `LOAD_ATTR` 7, `STORE_ATTR` 7, `LOAD_SUPER_ATTR` 6;
  - `CONTAINS_OP` 4, `CALL_METHOD_DESCRIPTOR_*` 4, `STORE_SUBSCR` 2,
    `LOAD_GLOBAL` 2;
  - generic and instrumented heads.
- **The representation packet does not change this.** Step 9a's inline-only
  numeric guards remove box paths from handlers already in the 177. The 115
  keep the record until value-form object entry points exist.

### 6.3 Dispatch overhead

- **Computed goto.** `DISPATCH` is 7 instructions on `LOAD_FAST_BORROW`'s
  path. The prologue adds 4 for the record and 4 for `instr_ptr` and
  `next_instr` through their homes.
- **Tail calls (`tcvt`).** `DISPATCH` is 10 instructions; the prologue adds
  9 for the frame and the canary. A3a removes the canary and the frame load.
  The frame-pointer push stays while the build keeps frame pointers, which
  may be why the `straight` probe is slower under tail calls (§1.1).
- **Words.** Words per upstream instruction go from 0.78 to 0.59 on 14
  kernels, or 0.74 to 0.66 without unpack_sequence (§4.1).

---

## 7. What we would not do

- **An accumulator register (V8).** A second operand model next to the slot
  fields.
- **Handlers in a macro assembler (TurboFan, DynASM).** A second handler
  language; tail calls give the register discipline from C.
- **GCC global register variables under computed goto.** GCC-only; they
  reserve the register for the whole program.
- **Keeping computed goto or the switch after A3.** A second and third
  backend with no measured limit (A3c).
- **Keeping the `IR_FIELD_*` memory decode.** It exists only for GCC's
  computed-goto spill.
- **Returning the PC from helpers.** The PC never moves (§0.2); the
  out-parameter goes instead.
- **Variable-width operands (V8 `Wide`/`ExtraWide`).** Two encodings of one
  operation, and they triple the handler table.
- **Short forms and operand-kind opcodes (`Star0`–`Star15`, `ADDVN`).** They
  multiply handlers.
- **Conditional back edges and loop inversion in v2.** Compare heads remove
  0.5% of fannkuch's words (1.0% once B7 turns its constant-then-compare
  heads into compare heads) and almost nothing elsewhere. Truth-test heads
  need a plain backward jump, which §4.3 excludes.
- **A handler per slot-class combination (G2).** It multiplies handlers.
- **`PyValue` bits in `.pyc` files for constant slots.** Every
  representation change would become a bytecode format change.
- **One magic-number bump for the whole revision.** Stale `.pyc` files would
  load across steps.
- **Elapsed time as a per-step gate.** There are no intermediate performance
  gates. Counts are recorded per step.

---

## 8. Measurements (not gates)

| What | Script | When |
|---|---|---|
| Re-split of the loop gap at HEAD by self-symbol category | `pyjit/tools/sampler/` | before A1; after A3c; after B7 |
| Object-value paths for the ten hottest handlers | `hmin.py` with the object branch forced | before A1; after A5; after A3c |
| Words executed per class | `dynamic_counts.py`, `irv2/dyn_v2.py`, `irv2/steps.py` | before A1; after B2, B5, B6, B7 |
| Inline-scalar lower bound per word | `labels.py`, `hmin.py`, `weighmin.py`, `hcat.py` | after A1, A2, A5, A3c, B7 |
| Handler bytes, loop text, frames, canaries (both compilers) | `hsize.py`, `tcframes.py`, `size` | the same points |
| Static Lib counts | `static_counts.py`, `irv2/static_pairs.py` | after each format step |
| Generated-corpus `compare_sites` maximum | `static_counts.py` | before B4 (required) |
| Matched elapsed comparison, interleaved, load stated | review `interleave.sh` or `pyjit/bench/run.py` | after A5; after A3c; A4 against B3; after B7 |

Weights must come from a stats build that counts handlers above 255. The
stats counter skips them today (§1.2), so the first stats build after A3a
should count them.

---

## 9. What could go wrong

### 9.1 Validation done

| Build | Tests run | Failed files | Failures and errors |
|---|---:|---|---:|
| Computed goto (copied release binary, HEAD `Lib`) | 50,935 | 9 | 20 |
| GCC tail call (`tclink.sh`) | 50,935 | the same 9 | 33 |
| Clang 21 tail call (review `tccl.sh`) | 50,935 | the same 9 | 33 |

- **The one difference.** `test_jit_method` has 25 errors and 13 failures
  under tail calls, against 1 error under computed goto, because the JIT is
  compiled out.
- **The tree's 12 known failures** are `test_capi` (3),
  `test_pyvalue_dict_vm` (4) and `test_pyvalue_native_scope` (5).
- **The rest come from the copied directory layout:** tests that read
  source or build paths.

### 9.2 Risks

| Risk | Step | Detection |
|---|---|---|
| A handler classified "never boxes" boxes | A1 | `IR_KEEP_BOX` assert; debug suite |
| The executing copy is replaced after all (a missed writer) | A2 | the `ir_words` assert after each reentrant call |
| An executing code object misses re-instrumentation | A2 | the version assert; step-5 monitoring tests |
| A reference leak or early release from D4 | A5 | debug refcount checks; `test_gc`, `test_weakref`, `test_finalization` |
| A miscompile under an unstable calling convention | A3a | full suite on two compilers; debug frame assert; `tcframes.py` |
| Stack-walking tools assume one evaluator frame | A3a | `test_perf_profiler`, `test_external_inspection`; `test_gdb` on a host with gdb |
| JIT entry broken by moving its state | A3b | JIT modules, admission forced |
| Small handlers get slower under tail calls (`straight`) | A3 | the matched comparison after A3c; frame-pointer and prologue counts |
| A stale `.pyc` loaded across format steps | B2–B7 | a magic bump in every step |
| Branch events move to the jump's offset | B2 | `test_monitoring` |
| Generated code past the compare-entry limit | B4 | the corpus measurement |
| A partly failed unpack changes locals | B5 | the B5 test |
| Line events change with fusion | B6 | fusion declined across lines |
| Constant slots visible or stale after `code.replace` | B7 | `test_code`, `test_frame` |
| Conflicts with representation steps 7–9b | all | §3's order; recount at each step |

---

## 10. Estimate

This is a judgement, not a measurement.

| Step | Agent-h | Basis | Model |
|---|---:|---|---|
| A1 | 2 | the classifier exists; one emitter | Opus |
| A2 | 2.5 | parameter deletion in 12 families, 10 emitters, 7 macro and body sites; two asserts | Sonnet, Opus review |
| A5 (D4) | 6 | five macros, 23 helper uses, count cost, lifetime tests | Opus |
| A3a | 14 | activation record for boxes and transactions (about 140 handlers); frame parameter; one decode; three conventions in `configure`; two compilers; debug and stats tail-call builds; frame check | Opus |
| A3b | 9 | JIT entry, resume, step and consume as C-ABI functions; JIT suite on both compilers | Opus |
| A3c | 2 | delete two backends and the memory decode | Sonnet |
| Dual CI on the A3 branch | 2 | both suites per A3 commit until A3c | Sonnet |
| A4 | 3.5 | explicit `nlocalsplus` in the class test, helpers outside handlers, stats build | Sonnet |
| B0a | 11 | about 10 columns, three consumers replaced, verifier, `get_ir_contract` | Opus |
| B0b | 3 | JIT front end | Opus |
| B1 | 1.5 | one `SyntaxError`, a test knob, the docstring | Sonnet |
| B2 | 3 | monitoring twins, renumbering, magic | Opus |
| B3 | 5 | prototype needed for G2's comparison in every case; lands only if A4 loses | Opus |
| B4 | 9 | per-kind sizes, priority assignment, 14 intrinsics, a second head, corpus measurement | Opus |
| B5 | 7 | four forms, unpack collector | Opus |
| B6 | 7 | 11 handlers, jump rule | Opus |
| B7 | 6.5 | constant table, frame push, `code.replace`, copy, pickle | Opus |
| M | 3 | sampler re-split, object paths, three matched comparisons | Sonnet |
| **Total** | **about 97** | phase A 41, phase B 53, measurements 3 | |

Each step is one commit, except A3, which is three commits landed together.
Each is reviewed with a release-mode compile and keeps the suite's pass set.
Phase A's A1, A2 and A5 can run while representation steps 8–9b land; A3
follows 9b.

---

## 11. Proposed edits to register-ir.md

1. **Source header and numbers.** Recount at `48bb4d6d1d3`:
   - generated file 20,710 lines;
   - `ceval.o` 595,778 B; evaluator 407,484 B against 58,419 B;
   - C frame 3,304 B; 1.40 KB per target;
   - Lib words 4,584,034;
   - unpack stores 8,887 and 8,050;
   - `CALL` → `POP_TOP` 229,553, with its family set stated;
   - `LOAD_GLOBAL` NULL bits 93,334.
2. **§1.13 Dispatch.**
   - The tail-call backend does not build at HEAD (include order; `#error`
     without `preserve_none`). GCC 15.2 has `musttail` and
     `no_callee_saved_registers`; Clang 21 has `preserve_none`.
   - With two edits, each builds and fails the same 9 files as computed goto
     in one diagnostic run, with the JIT compiled out.
   - Replace "decoding from `ir_word` made GCC spill `next_instr`" with:
     "under computed goto, `next_instr` stays in memory with or without the
     escapes; under tail calls, the escapes are what keep it there".
   - The baseline JIT is compiled out under tail calls because its entry
     state is loop-local (`pycore_jit.h:157`).
   - After IR v2's A3, the interpreter has one backend: tail-call handlers.
     A compiler without `preserve_none` or `no_callee_saved_registers` uses
     plain `musttail`, and a compiler without `musttail` is unsupported.
3. **§1.10.** Add the invariant: the executing copy is created once and
   freed only with the code object, so `frame->ir_words` never changes while
   the frame runs. Helpers that run reentrant code do not rebase the PC.
4. **IR-D3 and G2.**
   - IR-D3's "the slot class … lives in the instruction" becomes "the slot
     class is static; whether it is encoded in the instruction (class bits)
     or kept in dispatch state is G2's decision".
   - Add: D4 (no home holds a borrow) is its own step after representation
     step 8, whichever way G2 goes.
5. **G9.**
   - Problem list: add `next_instr` passed by address to 12 helper families
     (290 generated sites in 185 handlers, plus `finish_value_bridges` in
     every handler); `frame`, `slots` and `tstate` in stack homes; the record
     in every handler; and the canary that `-fstack-protector-strong` adds
     to a handler with an address-taken local.
   - Replace "5.5 KB" with 3,304 B.
   - Timing: state transport and the record move before baseline, justified
     by the measured tail-call gain (0.870× on 12 kernels, review 1), not by
     the 39% share.
6. **G9 item 6.** Address-taken transaction state lives in an activation
   record. The target is "no address-taken locals; frame at most N bytes",
   checked by a script on every supported compiler.
7. **G2.**
   - Five literal masks, `_remote_debugging` included, plus `IR_HANDLER`'s
     about 70 uses.
   - The control passes `nlocalsplus` to the class test explicitly; helpers
     outside the handlers read it from the executing copy.
   - The decision rule needs a B3 prototype in every case.
   - Count static operand roles before deciding.
8. **G3 and §4.3.**
   - Fused forms are forward only in v2.
   - Conditional back edges and loop inversion are deferred. Compare heads:
     fannkuch 260,650 (0.5%), chaos 5,030, go 133. Constant-then-compare
     heads: fannkuch 260,650. Truth-test heads, which would need a plain
     backward jump: fannkuch 1,731,245, richards 106,950, nqueens 40,319.
   - The contract's sign-dependent poll rule goes.
9. **G6.**
   - The constant-slot table stores `(slot, co_consts index)`, and the
     executing copy builds the values.
   - Schedule: after representation step 9a.
   - Window-fed constants keep their loads: 27% of fannkuch's loop constants.
10. **§4 and IR-D1.**
    - The format steps land as a sequence (B2–B7) after A3, each with its own
      magic number and dense renumbering where handlers change.
    - The churn argument for one revision is answered by the contract table:
      the decoders read it.
    - Counts are recorded per step; elapsed time is measured after A5, A3c
      and B7.
11. **§5 and O1.** The contract is `_PyIR_Formats` extended, exported through
    `_opcode.get_ir_contract()`, with no generated Python copy (O1 decided).
    Add the column "boxes inputs".
12. **§1.14 and G10.**
    - Limit errors already raise `SyntaxError`, except `regcfg.c`'s "too many
      temporaries", which v2 changes and tests through a limit knob.
    - "Two live temporaries" stays `SystemError`, as an invariant.
    - The docstring, `_PyFb_FAMILIES`' `RESUME`/`JUMP_BACKWARD` and
      `IR_MAX_FORWARD` are still stale at `48bb4d6d1d3`.
13. **§2.4.** Record §4.1's removed-word counts, the loop-head shapes and the
    window-fed constants.
14. **§2.1.** Replace the V8 and LuaJIT register and save-PC facts with §2's
    corrected rows, and drop "not re-verified" for them.

---

## 12. Open questions

1. **The loop share at HEAD.** What share of the gap is loop self time at
   HEAD? Step 5 has landed and steps 6–7 are landing. The 39% is from build
   F. The sampler re-split (§8) answers it before A1.
2. **Object paths.** How much longer are the object-value paths of the
   hottest handlers than the inline-scalar bound? It is about 61 against 44
   on `LOAD_FAST_BORROW` (§8).
3. **The G2 decision.** It needs a quiet machine. Load during review 1 was
   1.1–3.7.
4. **Generated-corpus `compare_sites`.** It is a precondition of B4; the
   corpus listing must be rebuilt.
5. **The attribute handlers in A1.** Can the ten specialised attribute
   handlers leave the boxing set without a value-form attribute API?
6. **Small handlers under tail calls.** Why is `straight` slower, and do
   A3a's frame target and the frame-pointer policy fix it?
7. **Register budget (finding 12).** What takes GCC's sixth register:
   `nlocalsplus` (A4), the dispatch table, or a predecoded field (as
   upstream predecodes `oparg`)? Under Clang all fit.
8. **Lookahead pairing (finding 12).** Is a handler that consumes a following
   conditional jump (LuaJIT's ISLT/JMP, V8's `Star` lookahead) a better fit
   than B6's 11 fused handlers and B5's no-destination calls? It needs no
   format change, but it executes one pair in two ways.
9. **Static operand roles (finding 12).** In Lib, `LOAD_FAST_BORROW`,
   `LOAD_CONST`, `LOAD_SMALL_INT`, `LOAD_GLOBAL` and `PUSH_NULL` write
   temporaries at least 98% of the time, and `LOAD_ATTR` 95% (review
   `dstclass.py`).
   - If the contract fixes those dsts as temporaries, and the compiler emits
     a `STORE_FAST` for the rare local, the class test leaves the hottest
     handlers without a bit or a parameter.
   - Count this before the A4/B3 decision. The review's count (51,198
     `LOAD_FAST_BORROW` destinations) skips `test` directories and
     `idlelib`; this proposal's 232,433 covers all of Lib. The decision
     needs one file set, and dynamic counts on the kernels beside it.
