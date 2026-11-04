# Proposal: register IR v2 packet (IR-D1, IR-D2, G2–G6, G9, G10, G12)

Status: proposal, revision 1, 2026-11-03. Not yet reviewed.
Design reference: [register-ir.md](../register-ir.md) §3 (IR-D1 to IR-D3,
G2–G12), §4 (bit budget and format list) and §5 (instruction contract).
Companion: [pyvalue-representation-packet.md](pyvalue-representation-packet.md),
whose steps 6a–9b are still to land.

**Source.** `build/worktrees/pyvalue-rc-universal` at `48bb4d6d1d3`, read
through `git show` and `git archive`, because another agent had uncommitted
edits in the working tree. The release build `build/pyvalue-rc-release-20261103`
reports `f3a20271f26`-dirty. Its `ceval.o` has the same text size (595,778
bytes) as `ceval.c` from `48bb4d6d1d3` compiled with the same flags, so its
disassembly describes HEAD. The three commits after `f3a20271f26` rename
value API functions. Upstream is `build/upstream-release-3a440b6`
(`3a440b65695`).

**Tools.** GCC 15.2. Clang is not installed. `perf` is unavailable
(`perf_event_paranoid` is 4). The evidence is therefore static (disassembly,
DWARF, generated source) plus `sys.monitoring` counts. Nothing was timed. The
scripts and their raw output are in `build/pyjit-link-wave1/irv2/` (§8.1).

**Out of scope.**
- Exception save and restore: 25% of the gap, removed by representation
  steps 5–7.
- Value bridging: 17%, which belongs to the value-API and native-boundary work.
- Lazy feedback allocation (G5 item 2), heat and OSR (G7), and the JIT-stage
  decisions (IR-D4).

## Summary

- **The 39% is per-word cost, and the encoding does not cause it.**
  - Weighted over 29 kernels, a word's predicted path in the eval loop is
    77.3 instructions with 15.3 stack references.
  - The shortest call-free path is 48.9 instructions and 12.1 stack
    references, against upstream's 22.0 and 1.3 on the same handlers.
  - Most of the stack traffic comes from the state the C compiler cannot keep
    in registers in one 407 KB function. `next_instr` escapes to helpers at
    290 sites, so it has a fixed stack home. `frame`, `slots` and `tstate` are
    reloaded in every handler. And every handler sets up a `_PyValueTemps`
    record in memory, although 177 of the 292 never box.
- **The encoding revision is worth about 12% of executed words.** On 13
  kernels (unpack_sequence excluded), words per upstream instruction go from
  0.74 to 0.71 after G4, 0.69 after G3 and at most 0.65 after G6.
  unpack_sequence goes from 1.70 to 0.15.
- **Hence two phases.**
  - **Phase A** changes the interpreter's structure, not the format. Its
    steps are A1 (the record only where a handler boxes), A2 (helpers return
    the PC instead of writing it through `&next_instr`), A3 (tail-call
    dispatch as the qualified build) and A4 (the G2 control).
    - In diagnostic builds, A1–A3 cut stack references on the shortest path
      from 12.1 to 1.5 per word. Upstream has 1.3.
    - A linked GCC tail-call interpreter fails the same test files as the
      computed-goto build over the full suite. It differs only in the JIT
      tests, because the JIT is compiled out under tail calls today.
    - Phase A can start after representation step 6c. A3's JIT part waits
      for step 9b.
  - **Phase B** is the encoding (IR-D1). Its first format step comes after
    representation step 9b, as IR-D1 says.
    - The contract table (IR-D2) can start before int51. Each later format
      step updates it, and the verifier checks every step against it.
- **Design changes proposed** (§11):
  - Phase A is G9 work. It moves before baseline, because it is the measured
    39%.
  - G6's constant table must not persist `PyValue` bits in `.pyc` files.
  - Loop inversion with backward fused jumps is deferred, because it removes
    0.5% of words on one kernel and nearly nothing elsewhere.
  - The 0xFFF-mask reader list shrinks to four literal masks.

---

## 0. State of the tree

### 0.1 Recount of register-ir.md

Every number the design gives for the code was recounted at `48bb4d6d1d3`.
Lib counts compile upstream `3a440b6`'s `Lib/` with the release build, as
§2.3 did (2,071 files; 11 fail by design). Where a number changed, the code
wins.

| Item | register-ir.md | Code at `48bb4d6d1d3` |
|---|---|---|
| `generated_ir_cases.c.h` | 18,772 lines, 292 `TARGET`s | **20,710 lines**, 292 `TARGET`s |
| Handler numbers | 305; first-only 267 | 305; 267 |
| Valid formats / targets | – | 293 / 292 (`BINARY_OP` is valid but has no `TARGET` of its own) |
| Window handlers; role tuples; layouts | 83; 9; 10 | 83; 9; 10 |
| Window formats; formats | 19; 64 | 19; 64 |
| NOFB handlers | 7 | 7 (`COMPARE_OP`, `CONTAINS_OP`, `LOAD_ATTR`, `LOAD_GLOBAL`, `LOAD_SUPER_ATTR`, its instrumented twin, `STORE_ATTR`) |
| `FBARG`; `RUNS_CODE`; formats with an fb field; using all four fields | 87; 261; 167; 144 | 87; 261; 167; 144 |
| Class-test macro lines | 890 (1,040 with store and move) | 1,040 lines in **211 handlers** |
| `instr_ptr` saved in the prologue | 289 of 292 | 289 of 292 |
| Value record set up | 292 of 292 | 292 of 292 (177 never box, §6.2) |
| `ir_displaced` / `finish_displaced_local` | 174 / 165 | 174 handlers / 165 calls in 159 handlers |
| `_PyValue_Publish`; `RECORD_BRANCH_TAKEN` | 184; 20 | 184 uses in 67 handlers; 20 |
| `ceval.o` text | 223 KB (interim build), upstream 107 KB | **595,778 B**; upstream 107,446 B |
| Evaluator function | – | `_PyEval_EvalFrameLoop` **407,484 B**; upstream `_PyEval_EvalFrameDefault` 58,419 B |
| Evaluator C frame | about 5.5 KB at `ca8a9d6d827`; upstream 360 B | **3,304 B** (`sub $0xce8,%rsp`); upstream 360 B (`0x168`) |
| Text per target | 0.76 KB | **1.40 KB** (407,484 B / 292) |
| Readers that mask the handler with 0xFFFF | about 20 functions in 13 files | **4 literal masks**: `pycore_ir.h:71` (`IR_HANDLER`), `pycore_ir.h:272` (`_PyIR_SetHandler`), `specialize.c:311`, `Lib/dis.py:413`; every other reader goes through `IR_HANDLER` (about 70 sites in 17 files) |
| Dead handlers | 5 | 5, with 0 static uses |
| Lib words | 4,574,431 | **4,584,034** |
| `NOT_TAKEN`; `PUSH_NULL` (all into temporaries) | 80,618; 117,632 | 80,618; 117,632 |
| Constant loads; feedback sites | 855,432; 1,492,371 | 855,432; 1,492,371, same kinds |
| Fusable compares | 17,381 of 21,564; `CONTAINS_OP` 3,557; `IS_OP` 1,960 | the same (of 21,564, 4,431 and 2,697) |
| Fused jump distance | all within ±2,047 | 22,840 within 127, 56 more within 511, 2 more within 2,047 |
| Jumps | 138,306 | 138,306 without `FOR_ITER` (18,168) and `SEND` (3,155); 48,248 backward with them |
| `LOAD_GLOBAL` NULL bits | – | **93,334** |
| `LOAD_FAST_BORROW` into a temporary | – | 232,433 (every one) |
| Unpacks; 2 elements; at most 8 | 9,871; –; 99.7% | 9,871; 7,250; 9,846 (99.75%) |
| Unpack then stores; into consecutive locals | 8,894; 8,057 | **8,887; 8,050** |
| `CALL` → `POP_TOP` | 218,006 | **229,553** counting every `CALL*` family (`CALL_INTRINSIC_*` and `CALL_FUNCTION_EX` included); 218,006 was not reproduced, and the design does not say which families it counted |
| Per-code-object maxima | slots 803, entries 1,180, 4,480 units, 7,764 words, 4,048 constants, 368 names, compare sites 70 | unchanged |
| unpack_sequence after G4.4 | about 870 words | 862 (9,667 − 8,805) |
| Stale items (G10) | docstring, `_PyFb_FAMILIES`, `IR_MAX_FORWARD`, `SystemError` | all still present: `ir_generator.py` docstring lines 55–63 ("folds no load yet", "stack_pointer"); `_PyFb_FAMILIES` lists `RESUME` and `JUMP_BACKWARD`; `IR_MAX_FORWARD` 2047 (`pycore_ir.h:69`) with no stated reason; limit errors raise `SystemError` |

Not recounted: note 06's frame-pointer reference counts (2,021 against 3,736)
cited in G2. The 29-kernel geometric mean of 0.81 words per upstream
instruction is taken from the checkpoint ([checkpoint-20261103.md](../../bench/results/checkpoint-20261103.md)).

### 0.2 Facts the design does not state

- **The tail-call backend does not build at HEAD.** Two things stop it:
  - `ceval.c` includes `generated_ir_cases.c.h` under `_Py_TAIL_CALL_INTERP`
    at line 1337, before the bridge helpers it calls (lines 1549–2140), so
    `call_transaction_begin` and others are undeclared.
  - `ceval_macros.h` stops with `#error` unless the compiler has
    `preserve_none`. GCC 15.2 does not: it warns and ignores the attribute.
- **GCC 15.2 has what a tail-call interpreter needs.**
  - It has `musttail` and the x86 attribute `no_callee_saved_registers`.
  - `irv2/tcpatch.py` makes two edits: it moves the include after the
    helpers, and under GCC it defines `Py_PRESERVE_NONE_CC` as
    `no_callee_saved_registers`. With them, the backend compiles.
  - `irv2/tclink.sh` rebuilds the ten core files that depend on
    `_Py_TAIL_CALL_INTERP`, directly or through `pycore_jit.h`, and links an
    interpreter from the release objects.
  - The full suite (§9.1) fails the same nine test files under that
    interpreter and under the copied computed-goto binary, both on HEAD's
    `Lib`. The only difference is `test_jit_method`, because the JIT is
    compiled out under tail calls.
  - The handlers save no callee-saved registers; each one pushes only
    `%rbp`, because the build keeps frame pointers.
- **The baseline method JIT is compiled out under tail calls.**
  `pycore_jit.h:157` defines `_PyJIT_BASELINE_SUPPORTED` only when
  `!_Py_TAIL_CALL_INTERP`. Its entry code lives in locals and labels of the
  loop:
  - the locals `jit_flow`, `jit_request`, `jit_context`, `jit_cold_request`
    and `jit_from_return`;
  - the labels `jit_resume`, `jit_enter` and `jit_step` (`ceval.c:2290`
    onwards).
- **Step 5 left the record in every handler.** Representation step 5 says
  "Handlers that box nothing drop `value_temps`". At HEAD, all 292 handlers
  still set up the record.
- **The tail-call handlers have a stack canary.** Under
  `_Py_TAIL_CALL_INTERP`, each handler declares its own `value_boxes` array
  and `_PyValueTemps`. With Ubuntu's default `-fstack-protector-strong`, any
  handler with an array or an address-taken local gets a canary check.
- **The release flags.** They include `-fno-omit-frame-pointer`,
  `-fcf-protection` (`endbr64` at every handler entry) and `-fno-gcse`.
  Upstream's release build has the same.

---

## 1. The 39%: what the eval loop spends per word

### 1.1 Method

- **Handler addresses.** They come from the relocations of the
  `opcode_targets` table in the release `python` and in upstream's
  (`labels.py`).
- **Two path measures per handler.** Each ends at the indirect jump that
  dispatches the next word.
  - *Fall-through* (`hpath.py`) follows the not-taken side of every
    conditional branch, which is the path GCC laid out as likely. Along it,
    the script counts instructions, calls, conditional branches, stack
    references by variable, loads of `nlocalsplus` (`-0x14(%r…)` off
    `ir_words`) and the `PyLong`/`PyFloat` tests of `_PyValue_Publish`.
  - *Shortest call-free path* (`hmin.py`) is the cheapest path with no call,
    which is the path a handler's fast case takes. Unlike fall-through, it
    compares across backends, because tail-call handlers place cold code
    differently.
- **Stack homes.** DWARF gives `next_instr` one fixed home for the whole
  function, `-0xc90(%rbp)` (`DW_OP_fbreg -3232`, `homes.py`). GCC's location
  lists for `frame` and `slots` are almost all empty, so their homes
  (`-0xcc8` and `-0xcd8`) were found from the loads that feed `frame->` and
  `slots[…]` accesses.
- **Weights.** Each handler is weighted by its entries per kernel, from the
  checkpoint's stats build (`checkpoint-20261103-C-kernels-boxes.json`, 29
  kernels, `dbce7b1260d`). Handler names are stable across those commits.
  Means are taken per kernel, then over kernels.
- **Diagnostic variants of `ceval.c`.** All are compiled with the release
  flags (`variants.py`, `tcpatch.py`, `cc.sh`):
  - `ni` wraps each helper call that takes `&next_instr` as
    `({ _PyInstr *ir_ni = next_instr; int r = F(…, &ir_ni, …); next_instr = ir_ni; r; })`,
    which removes the escape without changing the helpers;
  - `vt` is `ni` plus `value_temps = NULL` in the 177 handlers that never box;
  - `tc` is the tail-call backend under GCC (`tcpatch.py`); `tcni` and `tcvt`
    add `ni` and `ni`+`vt`.

  Only `tc` was linked into an interpreter.

### 1.2 One word: `LOAD_FAST_BORROW`

`LOAD_FAST_BORROW` is 13.8% of handler entries. Its shortest path is 44
instructions with 12 stack references. Upstream's is 14 instructions with
one stack load, the dispatch table base. Upstream keeps `next_instr` in
`%r12`, `frame` in `%rbx`, `stack_pointer` in `%r13` and `oparg` in `%r11d`.

`hcat.py` charges each instruction on the fork's path to the innermost
source construct, using the DWARF inline chain:

| Construct | Instructions |
|---|---:|
| `IR_STORE_DST`: the class test against `nlocalsplus` (two dependent loads), the write | 11 |
| `PyValue_BorrowAt`: the borrow tag of an in-place read | 10 |
| `DISPATCH`: the record's count test, fetch, decode, jump | 7 |
| `_PyValueTemps` set-up | 4 |
| `_PyValue_Publish`: `PyLong` and `PyFloat` tests | 4 |
| `frame->instr_ptr = next_instr` (a load of the stack home, then the store) | 2 |
| `next_instr += 1` (a load and a store of the stack home) | 2 |
| Other: `this_instr`, the field decode, a NULL test, `ir_bytes_at` | 4 |
| **Total** | **44** |

Under `tcvt`, the same handler is 43 instructions with 2 stack references:

| Construct | Instructions |
|---|---:|
| `DISPATCH` | 10 |
| Function prologue and canary | 9 |
| `IR_STORE_DST` | 8 |
| `PyValue_BorrowAt` | 7 |
| `_PyValue_Publish` | 4 |
| The frame load from `tstate` | 1 |
| `instr_ptr` | 1 |
| Other | 3 |

The stack references go. The instruction count stays, because the value
representation's work and the prologue remain.

### 1.3 Weighted over the kernels

**Fall-through path, computed goto, today** (`weigh-rc.md`; per executed
word, mean of 29 kernels):

| Instructions | Stack refs | `next_instr` | `frame` | `slots` | `tstate` | Value record | `nlocalsplus` loads | Publish tests | Calls | Conditional branches |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 77.3 | 15.3 | 3.3 | 3.5 | 1.8 | 0.4 | 4.1 | 1.2 | 0.6 | 0.7 | 9.9 |

The `ni` variant gives 79.7 instructions and 16.3 stack references. The `vt`
variant (`ni` plus `vt`) gives 75.1 and 11.9, and the value record's
references fall from 4.1 to 0.9.

**Shortest call-free path, per variant** (`weighmin.md`, `weighmin-up.md`;
per word):

| Build | 78% of entries: instructions / stack refs | 59% of entries (upstream has the handler): instructions / stack refs |
|---|---:|---:|
| Computed goto (today) | 53.0 / 12.3 | 48.9 / 12.1 |
| Computed goto + `ni` + `vt` | 50.0 / 9.3 | 45.4 / 8.6 |
| Tail call | 57.4 / 6.8 | – |
| Tail call + `ni` | 55.9 / 5.4 | – |
| Tail call + `ni` + `vt` | 50.0 / 2.2 | 45.5 / 1.5 |
| Upstream | – | 22.0 / 1.3 |

The hottest handlers on that path (instructions / stack references):

| Handler | Share of entries | Computed goto | cg + `ni` + `vt` | tc + `ni` + `vt` | Upstream |
|---|---:|---:|---:|---:|---:|
| `LOAD_FAST_BORROW` | 13.8% | 44 / 12 | 38 / 8 | 43 / 2 | 14 / 1 |
| `STORE_FAST` | 4.7% | 46 / 11 | 46 / 8 | 36 / 0 | 18 / 1 |
| `POP_JUMP_IF_FALSE_BOOL` | 4.3% | 59 / 11 | 56 / 7 | 56 / 0 | – |
| `POP_TOP` | 3.8% | 28 / 9 | 25 / 5 | 26 / 0 | 14 / 1 |
| `LOAD_GLOBAL_MODULE` | 3.6% | 60 / 11 | 58 / 8 | 54 / 0 | 40 / 1 |
| `PUSH_NULL` | 3.0% | 25 / 9 | 23 / 6 | 32 / 2 | 11 / 1 |
| `LOAD_SMALL_INT_INLINE` | 2.9% | 31 / 10 | 29 / 7 | 37 / 2 | 17 / 1 |
| `JUMP_BACKWARD` | 2.8% | 27 / 12 | 20 / 5 | 20 / 0 | 13 / 2 |
| `SEND_GEN` | 1.2% | 118 / 39 | 115 / 36 | 62 / 0 | – |

Three things follow.

- **Under computed goto, removing the escape alone does not help.** In the
  `ni` variant, GCC still stores `next_instr` to its home at each dispatch,
  and the copies add instructions. The single huge function, not the
  escapes alone, is what decides where GCC keeps its state.
- **Under tail calls, the state is in argument registers.** Each handler is
  its own allocation problem, and the state arrives in argument registers.
  The escape removal and the record removal then take stack traffic down to
  upstream's level.
- **The instruction count is what remains.** It is 45.5 against 22.0 per
  word. It is the value representation's fast-path work (§1.4, C5) and the
  per-handler prologue (C6), not state transport.

### 1.4 Named causes

| # | Cause | Handlers affected | Evidence | Removed by |
|---|---|---|---|---|
| C1 | **`next_instr` escapes.** Helpers that may redirect the PC after reentrant code write it through `_PyInstr **`. | 290 `&next_instr` sites in 185 handlers, passed to 11 out-of-line helpers: `finish_displaced_local` 165, `value_operation_finish` 38, `value_reader_complete` 28, `call_transaction_invoke` 26, `retire_published_values` 10, `call_transaction_expand_method` 7, `call_transaction_refresh` 6, `call_transaction_prepare_tuple` 4, `value_attribute_complete` 2, `native_call_finish` 2, `call_transaction_monitor` 2. Also 5 in `ceval_macros.h` and 2 in `bytecodes.c`. The generated sites come from 10 emit sites in `ir_generator.py`. | One stack home for the whole function; 3.3 references per word | A2 (return the PC), effective with A3 |
| C2 | **One 407 KB function.** GCC allocates registers across 292 handlers with 4,772 call instructions (upstream: 611) and 471 indirect jumps (upstream: 257). `frame`, `slots` and `tstate` live in stack homes and are reloaded in each handler. | All 292 | 3.5 + 1.8 + 0.4 references per word; C frame 3,304 B against 360 B | A3 (each handler a function; the state as arguments) |
| C3 | **The value record.** `value_owners = (_PyValueTemps){value_boxes, 0, 3}` is three stores in each handler, and `DISPATCH` tests the count. | 292, of which 177 never box | 4.1 references per word; the `vt` variant cuts loop text by 18% (407,484 → 333,672 B) | A1 |
| C4 | **The slot-class test reads memory.** `IR_IS_LOCAL` compares a field with `nlocalsplus` from the code header at `ir_words − 0x14`, behind a load of `frame->ir_words`. | 1,040 lines in 211 handlers | 1.2 header loads per word, each on a dependent chain | A4 (`nlocalsplus` as dispatch state) or B3 (class bits), chosen by G2's rule |
| C5 | **Representation work on the fast path.** It covers the publish tests (`_PyValue_Publish`), the borrow-tag computation (`PyValue_BorrowAt`, `IR_LOAD_BORROWED`) and the displaced-local protocol (`ir_displaced`, `finish_displaced_local`). | Publish: 67 handlers; `ir_displaced`: 174; `finish_displaced_local`: 159 | 0.6 publish tests per word; 19 of `LOAD_FAST_BORROW`'s 43 `tcvt` instructions; 15 of `STORE_FAST`'s 46 are `IR_LOAD_BORROWED` | Representation step 8 (canonical construction makes publish-time canonicalisation redundant) and D4 with G2 (no borrow tags in homes); not the format |
| C6 | **Fixed prologue per handler.** It covers the `instr_ptr` store, the frame load from `tstate` (tail call only), the canary, `endbr64` and the frame pointer. | 289 (`instr_ptr`); 276 of 298 tail-call functions have a canary (200 under `tcvt`) | 1–2 instructions for `instr_ptr`; 9 for prologue and canary in `tcvt` `LOAD_FAST_BORROW` | A3 (frame as a parameter; no handler arrays, so no canary); `instr_ptr` at escapes only is G9 item 1, after baseline; `endbr64` and the frame pointer stay, as upstream pays them |
| C7 | **Code size.** | All | Loop 407,484 B against 58,419 B (7.0×); mean 1,358 B per handler against 243 B (§6.1) | A1 (−18%), A3 (cold paths out of line per function), representation steps 7–9a (bridge code) |

The branch-history read-modify-write (G9 item 2, 20 handlers) and the NULL
store per consumed temporary (G9 item 4) are real but smaller. They stay
after baseline, as G9 says.

---

## 2. How V8 Ignition and LuaJIT keep the state in registers

Facts about the two VMs come from their public sources: V8's
`src/interpreter/` and `src/codegen/x64/register-x64.h`, and LuaJIT's
`src/vm_x86.dasc`, `vm_x64.dasc` and `lj_bc.h`. Clang's `preserve_none`
argument order is from LLVM's documentation. All of these are from memory
and were not re-verified for this note.

| Technique | V8 Ignition | LuaJIT 2 | Fits a C interpreter under GCC or Clang? | In this packet |
|---|---|---|---|---|
| Fixed registers for dispatch state | The accumulator, bytecode offset, bytecode array and dispatch table each have a fixed register (x64: `rax`, `r9`, `r12`, `r15`), set by the bytecode-handler calling convention | `BASE`, `KBASE`, `PC` and `DISPATCH` in fixed registers (x86 VM: `edx`, `edi`, `esi`, `ebx`) | **Yes, through tail calls.** A handler's parameters are the fixed registers: Clang's `preserve_none` passes the first arguments in `r12`–`r15`, then `rdi`, `rsi`, …; GCC's `no_callee_saved_registers` passes them in the SysV argument registers. Under computed goto, the compiler allocates registers for the whole function and nothing can be pinned. | A3 |
| State that must survive calls into C | Saved to the frame around calls | `PC`, `KBASE` and `DISPATCH` are in C callee-saved registers, so C helpers preserve them; `BASE` is caller-saved and refetched | **Yes.** With either attribute, a handler may use `rbx` and `r12`–`r15` without saving them, and the compiler moves live state there around calls. Under Clang the arguments arrive in `r12`–`r15` already. Under GCC they arrive in caller-saved registers, and a handler with a call path pays a few moves. | A3 |
| Operand decode during dispatch | Operands read from the bytecode array by offset | `ins_NEXT` loads the word, decodes `A` and `D` into registers and jumps; handlers receive them in registers | **Yes.** Pass the word as a parameter (`ir_word` already is one in the tail-call build) and decode fields with shifts. This retires the computed-goto build's memory re-read of fields (`IR_FIELD_*`), a second decode form that exists only because of GCC's spill. | A3 |
| PC saved only when calling out | `SaveBytecodeOffset` before calls that may need it | `savePC` before calls into C | **Yes.** The generator knows which uops escape. | G9 item 1, after baseline; A2 makes the PC a value, which is the precondition |
| Slow paths out of line | Deferred labels in the handler builder | Shared `->vmeta_*` stubs | **Yes.** Cold `noinline` helpers. Under tail calls, a slow path can be its own function reached by `musttail`, which keeps the fast handler small. | A3 (cold-path placement); G9 item 5 |
| Register file in the machine frame | Interpreter registers are frame slots off `rbp` | Lua slots off `BASE` | Already: `slots` is the base, and it is a parameter in the tail-call build | – |
| Dispatch-table register | `r15` | `DISPATCH` (also used to swap hook tables) | **Not needed.** The table is a RIP-relative constant (one `lea` or an indexed jump), and instrumentation rewrites handlers instead of swapping tables. | – |
| Accumulator | Implicit operand of most bytecodes | – | **No.** A second operand model next to the slot fields. Its gain is encoding density, which a three-address register IR already has. | rejected (§7) |
| Handlers written in a macro assembler | CodeStubAssembler compiled by TurboFan | DynASM | **No.** CPython generates handlers from `bytecodes.c` for the C compiler; tail calls get the register discipline without a second handler language. | rejected (§7) |
| Short forms and operand-kind opcodes | `Star0`–`Star15`; `AddSmi`, `LdaSmi` | `ADDVN`, `ADDNV`, `KSHORT` | **No.** They multiply handlers (G6's rejected experiment). | rejected; G6 uses hidden slots |
| Variable-width operands | `Wide` and `ExtraWide` prefixes | – | **No.** Two encodings of one operation. | rejected (§7) |
| Compare and jump | Accumulator compare, then `JumpIfTrue` | `ISLT` and the following `JMP` read as a pair | Yes: a fused fixed form | B6 (G3) |

Upstream CPython 3.14 ships the tail-call interpreter for Clang 19. Its gain
there is small, because upstream's computed-goto loop already keeps its state
in registers (one stack load per word, §1.3). Here, the computed-goto loop
does not keep its state in registers, which is why the same change matters
more.

---

## 3. Ordering against the representation packet

The representation packet's remaining steps touch these files:

| Step | Files and generated output |
|---|---|
| 6a–6c | `Objects`, `Modules`, `Python` sweep sites (running now) |
| 7 | judgment sites in `Objects`; comments in `ceval.c`; `_PyValue_RestoreException` deleted |
| 8 | constructors; `generators_common.py:POINTER_CONSTRUCTORS`; generated files regenerated |
| 9a | `pycore_value.h`, `bytecodes.c` numeric handlers, `specialize.c`, the compiler's inline-int users |
| 9b | `jit_x86.h`, `jit_backend_x86.c`, `jit_record.c`, `jit_ir.c`, `jit_baseline.c`, `jit_runtime.c` |

| IR v2 step | Can start | Why |
|---|---|---|
| A1, A2 | after 6c | They change `ir_generator.py`'s prologue, epilogue and helper calls, plus the helper signatures in `ceval.c` (lines 1549–2140). Steps 7–9a edit handler bodies and other files; the generated files are regenerated, not merged. Step 7 keeps every check in `ceval.c` (packet §3.4), so it does not edit these helpers. |
| A3a | after A2 | It edits `ceval.c`'s include order, `ceval_macros.h`, the tail-call prologue in the generator and `configure`. Its JIT part (A3b) edits `jit_runtime.c`, which 9b rewrites. |
| A3b | after 9b | Same file as 9b (`jit_runtime.c`, entry and resume), and the JIT suite is 9b's validation point. |
| A4 | after A3a | It needs the tail-call parameter list. |
| B0a (contract table, C consumers) | after 6c | A new generated table and the consumers in `assemble.c`, `regcfg*.c`, `ir.c`, `specialize.c`, `dis`. No format change. 9a's handler rewrites only regenerate rows. |
| B0b (JIT front end on the table) | after 9b | The consumers are the JIT files 9b rewrites. |
| B1 | any time | `SyntaxError` limits, the docstring and the `IR_MAX_FORWARD` reason are independent. |
| B2–B7 (format) | after 9b | IR-D1. Each changes the word stream, so `dis`, `mark_stacks`, the verifier and the JIT decoders, which 9b also edits. And: B6's `_INT` fused forms compare scaled int51 values (written once, after 9a); B7's constant slots hold inline values, and int51 decides which ints are inline; B3's D4 consequence (counted temporaries) needs step 8's canonical constructors. |

**Forced orders inside the packet.**
- A2 before A3a: without A2 the parameter escapes and stays in memory (§1.3,
  tail call 6.8 against 5.4).
- A3a before A4: the control is a parameter.
- A4 before B3: G2's decision rule.
- B2 first among the format steps: it carries the magic-number bump, dense
  numbering and the dead-handler removal, so later steps renumber nothing.
- B4 before B6: the fused entry is G5's 32-byte BINARY layout with the taken
  history in `misses`.
- A4 or B3 before B7: constant slots need a static class. Either outcome
  works, because hidden slots count as frame variables (below
  `nlocalsplus`).

---

## 4. Steps

| Step | Content | Gaps | Kind | Words executed | Per-word cost | Agent-h |
|---|---|---|---|---|---|---:|
| A1 | The record only in handlers that box | G9.3 | generator | 0 | −4.4 stack refs and −4.6 instructions per word (fall-through, against `ni`); loop text −18% | 2 |
| A2 | Helpers take and return the PC | C1 | generator + `ceval.c` | 0 | none under computed goto alone; −1.4 stack refs under tail calls | 4 |
| A3a | Tail-call backend builds and passes, GCC and Clang; frame as a parameter; no handler arrays; one field decode | §1.13, G9.6 | judgment | 0 | shortest path 12.1 → 1.5 stack refs per word (with A1, A2); canaries 276 → 0 | 5 |
| A3b | JIT entry as functions; tail calls become the qualified build | §1.13 | judgment | 0 | – | 6 |
| A4 | G2 control: `nlocalsplus` as the sixth parameter | G2 | experiment | 0 | −1.2 header loads per word | 2 |
| B0a | Contract table; verifier, passes, assembler, specialiser, `dis` | IR-D2, G1 | generator | 0 | 0 | 6 |
| B0b | JIT front end reads the table | IR-D2 | judgment | 0 | 0 | 3 |
| B1 | `SyntaxError` limits; docstring; `IR_MAX_FORWARD` | G10 | small | 0 | 0 | 1 |
| B2 | First format step: no `NOT_TAKEN`; dead handlers out; dense numbering | G12, §1.6 | format | 0 (already excluded from counts) | a load and a compare less per conditional fall-through: 0–8.9% of words | 3 |
| B3 | Class bits and D4, only if A4 does not recover the cost | G2, IR-D3 | format | 0 | −1.2 header loads per word; one count pair more per `LOAD_FAST_BORROW` into a temporary (3.8–8.5% of words) | 5 |
| B4 | Feedback layout and index; NOFB removed | G5.1, G10 | format | 0 | none; feedback memory −22% | 6 |
| B5 | No-dst calls; `PUSH_NULL` omitted; append and unpack source fields; unpack into locals | G4 | format | −0.7% to −8.1% (unpack_sequence −91%) | unpack handlers grow a store loop | 7 |
| B6 | Fused compare-and-jump; one jump-target rule; signed `FOR_ITER`/`SEND` | G3, §4.3 | format | −0% to −7.4% | one dispatch, a slot write, read and NULL store less per pair | 7 |
| B7 | Inline scalar constants as hidden slots | G6 | format | −0% to −28.2% (upper bound) | one 8-byte store per constant slot at entry of looping functions | 6 |
| M | Measurements (§8) | – | measurement | – | – | 2 |

### 4.1 Expected words executed

The columns are `sys.monitoring` counts of one `run(1)` after a warm-up, with
`NOT_TAKEN` excluded on both sides, from `steps.py` over `dyn_v2.py` and
`dyn_v3.py` output. A step's column is the words of the *current* IR that
the new form removes, assuming the compiler applies it at every counted site.

| Kernel | U instructions | C words | B5 removes | B6 removes | B7 removes (upper bound) | C/U now | after B5 | after B6 | after B7 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| chaos | 10,532,435 | 7,321,871 | 0.7% | 2.3% | 4.2% | 0.70 | 0.69 | 0.67 | 0.64 |
| comprehensions | 3,223 | 2,417 | 2.4% | 0.8% | 0.8% | 0.75 | 0.73 | 0.73 | 0.72 |
| coroutines | 3,884,596 | 3,277,628 | 7.4% | 7.4% | 7.4% | 0.84 | 0.78 | 0.72 | 0.66 |
| deltablue | 726,098 | 545,927 | 4.0% | 6.0% | 0.3% | 0.75 | 0.72 | 0.68 | 0.67 |
| fannkuch | 70,923,145 | 53,069,157 | 3.5% | 5.8% | 28.2% | 0.75 | 0.72 | 0.68 | 0.47 |
| float | 12,199,935 | 7,899,942 | 1.3% | 3.8% | 0.0% | 0.65 | 0.64 | 0.61 | 0.61 |
| generators | 6,806,866 | 6,106,863 | 0.0% | 0.0% | 2.2% | 0.90 | 0.90 | 0.90 | 0.88 |
| go | 28,667,983 | 18,762,468 | 2.2% | 7.1% | 2.8% | 0.65 | 0.64 | 0.59 | 0.58 |
| nbody | 28,581,241 | 19,300,999 | 7.8% | 0.0% | 8.8% | 0.68 | 0.62 | 0.62 | 0.56 |
| nqueens | 14,592,232 | 11,850,310 | 0.7% | 2.0% | 4.5% | 0.81 | 0.81 | 0.79 | 0.75 |
| raytrace | 41,877,624 | 29,606,956 | 3.7% | 0.9% | 0.5% | 0.71 | 0.68 | 0.67 | 0.67 |
| richards | 9,784,097 | 5,789,061 | 0.7% | 1.5% | 0.5% | 0.59 | 0.59 | 0.58 | 0.58 |
| spectral_norm | 19,045,221 | 17,011,597 | 8.1% | 0.0% | 0.0% | 0.89 | 0.82 | 0.82 | 0.82 |
| unpack_sequence | 5,679 | 9,667 | 91.1% | 0.0% | 0.0% | 1.70 | 0.15 | 0.15 | 0.15 |
| **Geometric mean** | | | | | | **0.78** | **0.64** | **0.62** | **0.59** |
| Without unpack_sequence | | | | | | 0.74 | 0.71 | 0.69 | 0.65 |

- **B5** counts `POP_TOP` of the result of the `CALL*` just before it (no
  intrinsics), `PUSH_NULL`, the `STORE_FAST`s of an unpack into consecutive
  locals in order, the `LOAD_FAST_BORROW` of the unpacked sequence, and the
  `LOAD_FAST_BORROW` before `LIST_APPEND`/`SET_ADD`/`MAP_ADD`.
- **B6** counts compares whose result is the condition of the next
  conditional jump. Inverted `while` loops add only 260,650 words on
  fannkuch (0.5%), 5,030 on chaos and 133 on go (`invertible_backedge`). So
  conditional back edges are deferred (§7).
- **B7** counts loads of small ints, inline ints and floats, `None`, `True`
  and `False` between a back edge's target and the back edge. It is an upper
  bound, because a constant that feeds a call or build window keeps its load
  (the window is copied).
- **Over the 17 kernels** in `checkpoint-20261103-dispatch.json`, the counters
  recorded for every kernel (call results popped, `PUSH_NULL`, fusable
  compares) give 0.78 → 0.77 → 0.74.

### Step A1. The value record only where a handler boxes

- **Generator.** `ir_generator.py` emits `value_owners`, `value_temps` and
  the `DISPATCH` count test only in handlers whose body boxes: `IR_KEEP_BOX`,
  `_PyValueTemps_Box` or `_PyValueTemps_Object`, or `value_temps` passed to a
  bridge helper that may box. `vtemps.py` classifies 177 as never boxing and
  115 as boxing at HEAD.
- **Other handlers.** They pass no record to `finish_displaced_local`, so its
  record parameter becomes optional, or the function splits by whether a
  record exists. `DISPATCH` loses its test there.
- **Close the 115 at the source.** Ten of the 115 are specialised attribute
  handlers: `LOAD_ATTR_INSTANCE_VALUE`, `_MODULE`, `_SHAPE`, `_SLOT`,
  `_WITH_HINT`, and the five `STORE_ATTR_*`. Their guards prove an object
  receiver, so they box only if the shared completion helper does.
  `LOAD_ATTR_SHAPE` alone is 4.5% of entries. If the stored value or the
  result is the only reason they box, pass the record only on that path.
- **Debug check.** `IR_KEEP_BOX` asserts a record exists, so a
  misclassification fails in debug builds.
- **Test.**
  - The step-0 suite set.
  - `test_pyvalue_call_bridges`, `test_pyvalue_repr_cleanup`.
  - A generator test that checks no record is emitted where the classifier
    says "never boxes".
  - The debug suite (assert above).
- **Expected effect.** Words 0. Per word, fall-through 79.7 → 75.1
  instructions and 16.3 → 11.9 stack references (`vt` against `ni`); loop
  text −18%.
- **Risk.** Low.

### Step A2. Helpers take and return the PC

- **The rule.** No handler or macro takes the address of `next_instr`. A
  helper that can redirect the PC after reentrant code takes `next_instr` by
  value and returns the PC to continue at, or NULL with an exception set:

  ```c
  next_instr = value_operation_finish(&state, next_instr, value_temps);
  if (next_instr == NULL) { goto error; }
  ```

  Each of these helpers returns an int status today. At each site, A2 checks
  that the status means only success or failure. Where it carries more, the
  helper returns a two-word struct of status and PC, which SysV returns in
  `rax:rdx`.
- **Sites.**
  - 290 in the generated file, from 10 emit sites in `ir_generator.py` (lines
    1288–2113).
  - 5 in `ceval_macros.h` and 2 in `bytecodes.c`.
  - The 11 helper families of C1 in `ceval.c`.
  - `generated_jit_helpers.c.h` is regenerated from the same emitters.
- **Also.** `frame` and `slots` stay values. A helper that needs the frame
  takes it, and none takes `&frame`.
- **Test.**
  - The step-0 set.
  - The step-5 tests: a `__del__` from rebinding a local, and a native call,
    that call `sys.monitoring.set_events` or `sys.settrace` take effect at
    the next instruction.
  - `test_monitoring`, `test_sys_settrace`.
  - A static check (the generator's own test) that no emitted line contains
    `&next_instr`.
- **Expected effect.** Words 0. No per-word gain under computed goto alone
  (§1.3). Under tail calls, shortest-path stack references go from 6.8 to
  5.4. It is the precondition for A3.
- **Risk.** Low to medium. The redirect logic itself does not change, only
  how its result returns.

### Step A3a. The tail-call backend builds, passes and has no stack state

- **Build.**
  - Move the tail-call include in `ceval.c` after
    `generated_jit_helpers.c.h`, or declare the helpers in a header; the
    second needs no include-order rule.
  - `Py_PRESERVE_NONE_CC` is `preserve_none` under Clang and
    `no_callee_saved_registers` under GCC ≥ 15 on x86-64, with `musttail`
    under both.
  - `configure` tests for both attributes, and the `#error` goes.
- **Parameters.** `TAIL_CALL_PARAMS` becomes `frame, slots, tstate,
  next_instr, ir_word`, five of the six SysV integer argument registers.
  - Handlers that change frames (calls, returns, generator entry and exit)
    pass the new frame.
  - `TAIL_CALL_LOAD_FRAME` goes, along with its dependent load per handler.
- **No arrays or address-taken locals in handlers (G9.6).**
  - **Box storage.** The 115 boxing handlers need box storage that lives
    until the handler finishes. It is owned by the eval activation: an area
    of three boxes (today's capacity) in the entry function's frame, reached
    through the entry frame record. Nested evaluations get their own area.
    A per-thread buffer would not work, because reentrant code nests
    evaluations on one thread.
  - **Helper-owned boxes.** Where a box is not read after the helper
    returns, the helper owns and releases it instead, as G4.4's unpack
    collector does.
  - The per-handler `value_boxes[N]` and `_PyValueTemps` declarations under
    `_Py_TAIL_CALL_INTERP` go.
  - Each remaining `&res` or `&left` becomes a slot-addressed helper
    (`&slots[F]`), as G9.6 says.
- **One field decode.** Every build decodes fields from `ir_word`. The
  `IR_FIELD_*` memory re-read under computed goto with the GIL goes.
- **Build roles.** The tail-call build joins CI as a second configuration
  with the JIT compiled out, as it is today. The qualified build stays
  computed goto until A3b.
- **Test.**
  - The full suite on the tail-call build, under GCC and, where available,
    Clang 19+, with the same failure set as the computed-goto build.
  - `test_sys` and `test_call` recursion tests: the C stack per Python call
    level changes from one 3.3 KB frame to small handler frames.
  - `test_gdb`, `test_perf_profiler`, `test_external_inspection`: tools that
    look for `_PyEval_EvalFrameDefault` frames.
  - `tcframes.py` as a build check: no `_TAIL_CALL_*` function reads
    `%fs:0x28`, and none has a frame above a stated bound.
- **Expected effect.** Words 0.
  - Shortest-path stack references go from 12.1 to 1.5 per word on the
    upstream-covered subset, measured with A1 and A2 (`tcvt`). The frame
    parameter and the canary removal are not in that measurement; they
    remove one more load and up to 9 prologue instructions per handler.
  - Tail-call text today is 450,721 B in 298 functions, and 387,461 B under
    `tcvt`.
- **Risk.** Medium.
  - `no_callee_saved_registers` with `musttail` is a rarer combination than
    Clang's `preserve_none`.
  - Stack-walking tools change.
  - The diagnostic link matched the computed-goto suite result outside the
    JIT tests (§9.1), but that is not a qualification.

### Step A3b. JIT entry under tail calls; tail calls become the qualified build

- **JIT entry.** The baseline JIT's loop-local state (`jit_flow`,
  `jit_request`, `jit_context`, `jit_cold_request`, `jit_from_return`) moves
  into a per-activation record reached from the entry frame. The labels
  `jit_enter`, `jit_resume`, `jit_step` and `jit_consume_action` become
  functions called (or tail-called) from the handlers that use
  `JIT_SITE_ENTRY`.
  - The computed-goto build calls the same functions, so the JIT's entry
    exists in one form.
  - `_PyJIT_BASELINE_SUPPORTED` no longer depends on
    `_Py_TAIL_CALL_INTERP`.
- **Qualified build.** The tail-call build becomes the qualified and
  measured build.
  - Computed goto stays generated for compilers without `musttail`, as
    upstream keeps it. It is compiled and smoke-tested in CI, but not
    measured.
  - Its loop no longer holds the JIT state as locals.
- **Test.** The JIT modules (`test_jit_method` and the others in the
  representation packet's step-0 survey), with admission forced on, and the
  full suite on both builds.
- **Expected effect.** None on words. The JIT state leaves the loop's locals,
  which also helps the computed-goto build.
- **Risk.** Medium to high. It is the one part that changes JIT control
  flow.

### Step A4. G2 control: `nlocalsplus` in dispatch state (experiment)

- **Change.** `IR_NLOCALSPLUS()` reads a sixth tail-call parameter, set
  wherever the frame changes. This uses the last SysV argument register;
  under Clang's `preserve_none`, more remain.
- **Decision.** Run the matched elapsed comparison of A4 against B3 (G2's
  decision rule), on a quiet machine.
  - If A4 recovers the cost, A4 lands, the class bits are dropped, and §4.1
    keeps a 16-bit handler.
  - Otherwise B3 lands, and A4 is discarded.
- **Expected effect.** Words 0. 1.2 header loads per word fewer, each behind
  a load of `frame->ir_words`.
- **Risk.** Low. It is one macro and a parameter.

### Step B0a. The instruction contract table

See §5 for the table's form. This step generates it, replaces
`_PyFb_FAMILIES`, `_PyIR_RUNS_CODE`'s separate use in the passes and the
flag tests in `regcfg_passes.c`, and makes the verifier check folded loads
against "may run Python" with its operand dependence (G1's remaining caveat).

- **Test.**
  - A generated-table test (`test_ir_contract`): every handler has a row; the
    rows agree with `_PyOpcode_num_popped/pushed` for window extents, with
    the analyzer's `escapes`, `error_*`, `deopts` and `jumps`, and with
    `pycore_feedback.h`'s kinds.
  - `test_peepholer`, `test_ir`, `test_dis`.
  - `_PyRegCfg_Verify` over the stdlib in a release test job (G11).
- **Risk.** Medium. It is generator work, and it replaces three hand-written
  consumers.

### Step B0b. The JIT front end reads the table

`_PyJitHomeEffect` (`pycore_jit.h`, `pycore_jit_ir.h`) goes. `jit_ir.c`,
`jit_record.c` and `jit_baseline.c` read roles, window extents and effects
from the table.

- **Test.** The JIT modules.
- **Risk.** Medium.

### Step B1. Small G10 items

- Limit errors raise `SyntaxError` at the code object's first line, naming
  the limit (`regcfg.c:occ_take`, `assemble.c`).
- Fix the `ir_generator.py` docstring (lines 55–63).
- Either state why `IR_MAX_FORWARD` is 2,047 (`pycore_ir.h:69`), or set it to
  the signed rule's 2,047 in B6 and say so.
- **Test.** A function with 5,000 locals raises `SyntaxError`.
- **Risk.** Low.

### Step B2. First format step: `NOT_TAKEN` dropped, dense numbering

- **The change.**
  - Remove `NOT_TAKEN` (G12).
  - The instrumented twins of the conditional jumps fire `BRANCH_LEFT` and
    `BRANCH_RIGHT` with the jump's own location.
  - Remove the five dead handlers and renumber densely.
  - Bump the magic number.
- **Test.**
  - `test_monitoring` (branch events now at the jump's offset: a visible
    change; the tests state what they accept), `test_dis`, `test_compile`.
  - `test_coverage`-style users of branch events in `Lib/test`.
- **Expected effect.** 80,618 static words fewer (1.8%). Executed words are
  unchanged, since `NOT_TAKEN` is skipped, not dispatched.
  - Per word, a load and a compare less on each conditional fall-through. On
    the kernels the fall-throughs are 0–8.9% of words (richards 513,653 of
    5,789,061; fannkuch 6.8%; go 5.5%).
- **Risk.** Low to medium, because of the monitoring change.

### Step B3. The class bits (only if A4 loses)

- **The change.** As G2 and §4.1 specify. The handler is in bits 0–11 and the
  class bits in 12–15.
  - The four literal 0xFFFF masks become 0xFFF.
  - `IR_WORD` sets the bits, and `_PyIR_SetHandler` keeps them.
  - The verifier checks each bit against `nlocalsplus`.
- **D4.** No slot holds a borrow, so `LOAD_FAST_BORROW` into a temporary
  takes a count for a mortal object.
- **Test.**
  - `test_ir`, `test_dis`, `test_peepholer`.
  - The verifier over the stdlib.
  - `test_tagged_values`.
- **Expected effect.** Words 0. 1.2 header loads per word fewer. A count pair
  more per `LOAD_FAST_BORROW` into a temporary, on 3.8–8.5% of words
  (`load_fast_borrow_to_temp`).
- **Risk.** Moderate.

### Step B4. Feedback layout and index; NOFB removed

- **Precondition.** The generated-corpus maximum of compare and contains
  sites per code object (G10). It has not been measured, because the corpus
  listing was not kept.
- **The change.** As G5 item 1 and G10 option (c):
  - offsets in 8-byte units, per-kind sizes, priority-ordered assignment, a
    header-only entry 0;
  - names in D (12 bits) or CD (`LOAD_GLOBAL`);
  - a second generic head for `LOAD_ATTR`'s method bit;
  - the 7 NOFB handlers go, and 14 intrinsics are added for compare sites
    past the limit.
- **Test.**
  - `test_ir`, `test_dis`, `test_opcache`.
  - The JIT feedback readers.
  - A generated code object past 4,095 units that still specialises its
    early sites.
- **Expected effect.** Words 0. Feedback memory −22%. Handlers −6.
- **Risk.** Moderate.

### Step B5. Call and unpack forms (G4)

- **The change.** As G4 items 1, 2 and 4, and item 3 for `LIST_APPEND`,
  `SET_ADD` and `MAP_ADD`.
  - The `LOAD_GLOBAL` NULL bit (93,334 static) is dropped with `PUSH_NULL`
    (117,632 static).
  - Unpacks past 8 elements collect into a temporary list in an out-of-line
    helper.
- **Test.**
  - `test_unpack`, `test_unpack_ex`, `test_listcomps`, `test_setcomps`,
    `test_dictcomps`, `test_extcall`, `test_call`.
  - A failure in `__next__` part-way through an unpack into locals leaves the
    locals unchanged.
  - The verifier proves the omitted `PUSH_NULL` slot NULL.
- **Expected effect.** §4.1 B5 column. spectral_norm −8.1%, nbody −7.8%,
  coroutines −7.4%; unpack_sequence 9,667 → 862.
- **Risk.** Small to moderate.

### Step B6. Fused compare-and-jump and the one jump-target rule

- **The change.** As G3 and §4.3, with two changes:
  - Fused forms are forward only.
  - Conditional back edges and loop inversion are deferred (§4.1: 0.5% of
    fannkuch's words, almost nothing elsewhere), so the fused handlers never
    poll the eval breaker, and the contract needs no sign-dependent "may run
    Python" for them.
- **Jump encoding.** `FOR_ITER` and `SEND` use the signed 12-bit distance.
- **Handlers.** 11 new: 3 generic, 5 specialised and 3 instrumented. The
  compiler declines fusion when the compare and the jump have different line
  numbers, so line events do not change.
- **Test.**
  - `test_compile`, `test_peepholer`, `test_dis`, `test_monitoring`
    (`BRANCH_*` from the fused twin), `test_sys_settrace`.
  - The 2 static pairs at distance 512–2,047.
- **Expected effect.** §4.1 B6 column. go −7.1%, coroutines −7.4%, deltablue
  −6.0%, fannkuch −5.8%.
  - Text grows by about 11 × 1.6 KB = 18 KB at today's handler sizes, or less
    after phase A.
- **Risk.** Moderate.

### Step B7. Inline scalar constants as hidden slots (G6)

- **The change.** As G6, with one change (§11, item 9).
  - The constant-slot table stores `(slot, co_consts index)` pairs.
  - The executing copy (`make_exec`) writes the values when it is built, and
    the frame push copies them.
  - Marshal then stores no `PyValue` bits, and the representation packet's
    "no persisted encodings" still holds.
- **Test.**
  - `test_marshal`, `test_py_compile`, `test_compileall` (`.pyc`
    determinism).
  - `test_frame` and `test_inspect` (`f_locals` hides the slots), `test_dis`,
    and generators (`_PyFrame_Copy`).
- **Expected effect.** §4.1 B7 column, an upper bound. fannkuch −28.2%,
  nbody −8.8%, coroutines −7.4%.
  - The cost is entry stores per call of a looping function.
- **Risk.** Moderate.

---

## 5. The instruction contract table (IR-D2)

- **Form.** One table, `_PyIR_Formats` grown into the contract, not a second
  table next to it.
  - Today's row already has `flags`, `roles`, `imm`, `fb`, `ninputs`, `dst`,
    `borrow_mask`, `shadow` and `nofb`.
  - It gains the §5 columns:
    - per-field operand roles, including the slot roles hidden in `IMM`;
    - window extent as (base field, a fixed count or "oparg field + k");
    - consumed or in place, per input;
    - an effect bitset: raises, runs Python, deoptimises, jumps, suspends,
      pushes a frame, polls, allocates, releases the old dst;
    - "runs Python" operand rules: releases a local through `DST`,
      `LOCAL_OUT` or a window output;
    - feedback kind and layout;
    - target fields and their encoding;
    - the monitoring twin and its events;
    - pass-through inputs.
  - Two columns are new against §5:
    - **boxes inputs**: the handler needs the value record (A1);
    - **may redirect the PC**: it calls a helper that returns a new PC (A2).
  - Heap reads and writes, guards with the feedback fields they read, and
    result facts are added with B0b. They need annotations in `bytecodes.c`
    and are read only by the JIT.
- **Where.** The table is generated by `Tools/cases_generator/ir_generator.py`
  into `Include/internal/pycore_ir_metadata.h`, where `_PyIR_Formats` is
  generated today.
  - Python reads it through `_opcode.get_ir_contract()`, which grows from
    `get_ir_formats()`. There is no generated Python copy, so there is one
    source.
  - This settles O1.
- **Consumers today, which move to the table** (counts at HEAD):

  | Consumer | Uses today | Reads |
  |---|---|---|
  | `Python/ir.c` (decoders `_PyIR_FieldOparg`, `_PyCode_Instr*`) | `_PyIR_FormatOf` 10, flags and roles 6 | roles, targets, fb location |
  | `Python/assemble.c` (handler choice, feedback assignment) | `_PyIR_FormatOf` 3, flags and roles 12, `_PyFb_FAMILIES` 2 | roles, feedback kind and layout (B4's assignment order) |
  | `Python/regcfg.c` (conversion, compaction, verifier) | `_PyIR_FormatOf` 3, flags and roles 10 | roles, extents, consumed or in place |
  | `Python/regcfg_passes.c` (copy propagation, store elimination) | flags 7 | runs Python with operand rules, slots read and written |
  | `Python/specialize.c` | `_PyIR_FormatOf` 1 | feedback kind and layout |
  | `Include/internal/pycore_ir.h` | flags and roles 12 | – (accessors) |
  | `Lib/opcode.py` → `Lib/dis.py` | `_opcode.get_ir_formats()` | roles, hidden slot operands, folded inputs (G11) |
  | `Python/jit_ir.c`, `jit_record.c`, `jit_baseline.c` | `_PyJitHomeEffect`, `_PyIR_FormatOf` 1, flags 2 | everything, including heap effects, guards and result facts (B0b) |
  | `ir_generator.py` itself | – | boxes inputs (A1), slot-addressed helpers (G9.6) |
  | Verifier (`_PyRegCfg_Verify`, `_PyCode_Validate`) | format flags | every row; checks each format step from B2 on |

- **Step.** B0a builds the table with v1 rows, after 6c and before int51. B0b
  switches the JIT front end after 9b. Each format step B2–B7 updates the
  rows it changes, and the verifier checks the step against the table.

---

## 6. G9: handler count, code size and dispatch

### 6.1 Today against upstream (release builds)

| Measure | Fork | Upstream |
|---|---:|---:|
| Handlers with a `TARGET` | 292 | 231 |
| Handler numbers | 305 | – |
| `ceval.o` text | 595,778 B | 107,446 B |
| Evaluator function | 407,484 B | 58,419 B |
| Bytes per handler region (`hsize.py`, labels included): mean / median | 1,358 / 1,362 (300 regions) | 243 / 196 (240 regions) |
| C frame of the evaluator | 3,304 B | 360 B |
| Call instructions in the evaluator | 4,772 | 611 |
| Indirect jumps in the evaluator | 471 | 257 |

| Handler | Fork bytes | Upstream bytes |
|---|---:|---:|
| `LOAD_FAST_BORROW` | 866 | 56 |
| `STORE_FAST` | 496 | 88 |
| `BINARY_OP_ADD_INT` | 1,948 | 239 |
| `LOAD_ATTR_INSTANCE_VALUE` | 2,365 | 206 |
| `COMPARE_OP_INT` | 1,616 | 294 |
| `RETURN_VALUE` | 1,040 | 160 |
| `CALL_PY_EXACT_ARGS` | 961 | 378 |

The tail-call object (`tc`) has 298 `_TAIL_CALL_*` functions totalling
450,721 B. The median frame is 160 B, the largest 512 B, and 276 have a
canary. Under `tcvt` the functions total 387,461 B, with a median frame of
80 B, the largest 304 B, and 200 canaries. `LOAD_FAST_BORROW` is 852 B under
`tc` and 571 B under `tcvt`; `STORE_FAST` 462 and 210; `JUMP_BACKWARD` 462
and 116.

### 6.2 Which handlers can drop `value_temps`

- **Now: 177 of 292.** They never box; the list is in
  `irv2/value-temps-handlers.txt`. Step A1.
- **The other 115 box.** They are:
  - generic `BINARY_OP` operations, 28;
  - the attribute families: `LOAD_ATTR` 7, `STORE_ATTR` 7, `LOAD_SUPER_ATTR` 6;
  - `CONTAINS_OP` 4, `CALL_METHOD_DESCRIPTOR_*` 4, `STORE_SUBSCR` 2,
    `LOAD_GLOBAL` 2;
  - the generic and instrumented heads of other families.

  They pass an inline input to an object API. The representation packet does
  not change that:
  - step 8 canonicalises constructors;
  - step 9a makes numeric guards inline-only, which removes box paths from
    specialised numeric handlers, but those are already in the 177.

  They keep the record until value-form object entry points exist. That is
  the value-API work, not this packet. The ten specialised attribute
  handlers are the exception to check in A1 (§4, A1).

### 6.3 Dispatch overhead

- **Per word, computed goto.**
  - `DISPATCH` is 7 instructions on `LOAD_FAST_BORROW`'s path: the record
    count test, fetch, decode and indirect jump.
  - The prologue adds 4 for the record, 2 for `instr_ptr` and 2 for
    `next_instr += 1` through its home.
- **Per word, tail call (`tcvt`).** `DISPATCH` is 10 instructions: the
  fetch, the decode into parameters and the tail jump through the table. The
  prologue adds 9 for the frame setup and the canary.
  - A3a removes the canary.
  - The frame-pointer push stays while the build keeps frame pointers, as
    upstream's tail-call build does.
- **Words per upstream instruction.** 0.78 on the 14 kernels today, 0.59
  after B7 at most (§4.1).
  - With method-JIT admission off, the kernels take 2.6× upstream's time
    (triage) while executing 0.81 words per upstream instruction. Each word
    therefore costs about 3.2 times an upstream instruction.
  - A 12% cut in words is small against that. That is why phase A comes
    first.

---

## 7. What we would not do

- **An accumulator register (V8).** It is a second operand model next to the
  slot fields, and it buys density a register IR already has.
- **Handlers in a macro assembler (TurboFan, DynASM).** It is a second handler
  language; tail calls give the register discipline from C.
- **GCC global register variables to pin state under computed goto.** They
  are GCC-only, reserve the register for the whole program and every library
  call, and Clang does not support them for this.
- **Fixing the computed-goto loop with compiler flags or hot/cold splitting.**
  Tuning one 407 KB function's register allocator is fragile; tail calls make
  each handler its own problem.
- **Keeping the `IR_FIELD_*` memory decode.** It is a second decode form whose
  only reason is GCC's computed-goto spill. It goes in A3a.
- **Variable-width operands (V8 `Wide`/`ExtraWide`).** They are two encodings
  of one operation.
- **Short forms and operand-kind opcodes (V8 `Star0`–`Star15`, LuaJIT
  `ADDVN`).** They multiply handlers; G6's constant-operands experiment
  regressed.
- **A dispatch-table register.** The table is a RIP-relative constant, and
  instrumentation rewrites handlers.
- **Conditional back edges and loop inversion in v2.** They remove 0.5% of
  words on fannkuch and almost nothing elsewhere, and they cost a
  sign-dependent eval-breaker poll and contract rule in the fused handlers.
  Revisit with the JIT, where loop shape matters more.
- **The copy-around-call idiom (`ni`) as the landed form.** It hides the
  escape behind a second variable. Returning the PC states the contract.
- **A handler per slot-class combination (G2).** That multiplies handlers.
- **`PyValue` bits in `.pyc` files for constant slots.** They would make
  every representation change a bytecode format change.
- **Measuring elapsed time per step as a gate.** No intermediate performance
  gates. Counts are recorded per step, and elapsed time once after A3a and
  once after B7.

---

## 8. Measurements (not gates)

Recorded on the release build and upstream `3a440b65695`, at the points
named:

| What | Script | When |
|---|---|---|
| Words executed per kernel and per counted class | `pyjit/bench/ir/dynamic_counts.py`, `irv2/dyn_v2.py`, `irv2/dyn_v3.py`, `irv2/steps.py` | before A1; after B2, B5, B6, B7 |
| Shortest and fall-through paths: instructions, stack refs per word, per state variable | `irv2/labels.py`, `hpath.py`, `hmin.py`, `weigh.py`, `weighmin.py`, `hcat.py` | before A1; after A1, A2, A3a; after B7 |
| Handler bytes, loop text, C frame, tail-call frames and canaries | `irv2/hsize.py`, `tcframes.py`, `size`, `-fstack-usage` | the same points |
| Static Lib counts | `pyjit/bench/ir/static_counts.py`, `irv2/static_pairs.py` | after each format step |
| Generated-corpus `compare_sites` maximum | `static_counts.py` | before B4 (required) |
| Matched elapsed comparison, interleaved, with the load stated | `pyjit/bench/run.py` | after A3a; A4 against B3; after B7 |

### 8.1 Files

In `build/pyjit-link-wave1/irv2/`:

- **Scripts.**
  - `labels.py`: handler addresses from table relocations.
  - `hpath.py`, `hmin.py`, `homes.py`, `hcat.py`: the path measures, the
    DWARF homes and the per-construct charges.
  - `weigh.py`, `weighmin.py`: entry weighting.
  - `hsize.py`, `tcframes.py`: sizes, frames and canaries.
  - `formats.py`: format recounts.
  - `vtemps.py`: the record classifier.
  - `static_pairs.py`, `dyn_v2.py`, `dyn_v3.py`, `steps.py`: the static and
    dynamic counts.
  - `variants.py`, `tcpatch.py`, `cc.sh`: the diagnostic variants.
  - `tclink.sh`, `runtests.sh`, `fullsuite.sh`: the linked tail-call
    interpreter and its tests.
- **Raw output.**
  - `hpath-*.tsv`, `hmin-*.tsv`, `hsize-*.tsv`, `labels_*.json`.
  - `weigh-*.md`, `weighmin.md`, `weighmin-up.md`.
  - `static-lib.*`, `static-pairs-lib.*`.
  - `dyn-v2/`, `dyn-v3/`, `steps.md`.
  - `value-temps-handlers.txt`.
  - `tests-*.log`, `suite-*.log`.
- **Removed.** Large intermediates (disassembly, DWARF dumps, object files,
  source trees). The scripts regenerate them.

---

## 9. What could go wrong

### 9.1 Validation done

Full suite, `-j6`, both binaries on HEAD's `Lib` (`fullsuite.sh`):

| Build | Test files | Tests run | Failed files | Failures and errors |
|---|---:|---:|---|---:|
| Computed goto (copied release binary) | 622 | 50,935 | 9 | 20 |
| Tail call, GCC (`tclink.sh`) | 622 | 50,935 | the same 9 | 33 |

- **The failed files.** Both runs fail the same nine: `test_pydoc`,
  `test_build_details`, `test_capi`, `test_jit_method`,
  `test_pyvalue_dict_vm`, `test_pyvalue_native_scope`, `test_subprocess`,
  `test_sysconfig` and `test_unittest`.
- **The only difference is `test_jit_method`.** It has 25 errors and 13
  failures under tail calls, against 1 error under computed goto. The
  baseline JIT is compiled out under tail calls, which is A3b's work.
- **Every other file has the same counts.**
  - `test_capi` (3), `test_pyvalue_dict_vm` (4) and
    `test_pyvalue_native_scope` (5) are the tree's 12 known failures.
  - The rest come from the copied directory layout: those tests read source
    or build paths. That includes `test_jit_method`'s one computed-goto
    error, a `runpy` path.

### 9.2 Risks

| Risk | Step | How it is detected |
|---|---|---|
| A handler classified "never boxes" boxes | A1 | `IR_KEEP_BOX` asserts a record; debug suite |
| A PC redirect lost when a helper returns | A2 | step-5 monitoring and settrace tests; `test_monitoring` |
| GCC miscompiles `no_callee_saved_registers` with `musttail` | A3a | full suite on the GCC tail-call build; a Clang build where available; `tcframes.py` checks |
| Deep recursion or stack-walking tools assume one evaluator frame | A3a | `test_sys`, `test_call`, `test_gdb`, `test_perf_profiler`, `test_external_inspection` |
| JIT entry or resume broken by moving its state out of the loop | A3b | JIT modules with admission forced on |
| The unmeasured computed-goto build decays | A3b | CI compile and smoke job |
| The G2 decision needs timing on a loaded machine | A4, B3 | interleaved runs; state the load; repeat when quiet |
| Branch events move to the jump's offset | B2 | `test_monitoring`; documented as a visible change |
| Generated code past the compare-entry limit | B4 | the required corpus measurement; the intrinsic lowering's tests |
| A partly failed unpack into locals changes locals | B5 | the new test in B5 |
| Line events change with fusion | B6 | decline fusion across lines; `test_sys_settrace` |
| Frame introspection shows constant slots | B7 | `test_frame`, `test_inspect`, `test_dis` |
| Conflicts with representation steps 7–9b | all | §3's ordering; recount at the start of each step |
| Handler mix drift: the weights come from `dbce7b1260d` | all | re-weigh with a new stats build before A1 and after B7 |

---

## 10. Estimate

| Step | Agent-hours | Model |
|---|---:|---|
| A1 | 2 | Opus |
| A2 | 4 | Opus (generator), Sonnet (helper signatures) |
| A3a | 5 | Opus |
| A3b | 6 | Opus |
| A4 | 2 | Sonnet |
| B0a | 6 | Opus |
| B0b | 3 | Opus |
| B1 | 1 | Sonnet |
| B2 | 3 | Opus |
| B3 | 5 (only if A4 loses) | Opus |
| B4 | 6 (+1 corpus measurement) | Opus |
| B5 | 7 | Opus |
| B6 | 7 | Opus |
| B7 | 6 | Opus |
| M | 2 | Sonnet |
| **Total** | **about 66** (61 if A4 wins) | |

Each step is one commit, reviewed with a release-mode compile, and keeps the
suite's pass set.

- **Phase A** (19 agent-hours) can run while representation steps 7–9b land.
  A3b waits for 9b.
- **Phase B** (44 agent-hours) begins with B0a in parallel with phase A, and
  the format steps follow 9b.

---

## 11. Proposed edits to register-ir.md

1. **Source header.** Recount at `48bb4d6d1d3`, and replace the numbers in
   §0.1's table:
   - generated file 20,710 lines;
   - `ceval.o` 595,778 B; evaluator 407,484 B against 58,419 B;
   - C frame 3,304 B;
   - 1.40 KB per target;
   - Lib words 4,584,034; unpack stores 8,887 and 8,050;
   - `CALL` → `POP_TOP` 229,553, with its family set stated;
   - `LOAD_GLOBAL` NULL bits 93,334.
2. **§1.13 Dispatch.**
   - The tail-call backend does not build at HEAD (include order; `#error`
     without `preserve_none`).
   - GCC 15 has `musttail` and `no_callee_saved_registers`, and with them the
     backend builds and passes (§0.2).
   - Handlers load the frame from `tstate`; v2 passes it.
   - Replace "decoding from `ir_word` made GCC spill `next_instr`": the spill
     comes from `&next_instr` escaping to 11 helpers in one function, and the
     memory decode is a second form that goes with A3a.
3. **§1.13 JIT admission.** The baseline JIT is compiled out under tail calls
   because its entry state is loop-local (`pycore_jit.h:157`).
4. **G9 problem list.** Add:
   - `next_instr` escapes (290 sites in 185 handlers, 11 helpers);
   - `frame`, `slots` and `tstate` reloaded from stack homes in every handler
     (3.5, 1.8 and 0.4 references per word);
   - the per-handler record set-up (4.1 references per word);
   - the stack canary that `-fstack-protector-strong` adds to any handler
     with an array or address-taken local (276 of 298 tail-call functions).

   Replace "5.5 KB" with 3,304 B.
5. **G9 timing.** State transport and the record (this packet's A1–A3) move
   before baseline, because they are the measured 39%. Items 1, 2, 4 and 5
   stay after baseline.
6. **G9 item 6.** Add the canary rule: no handler-local arrays, and the
   tail-call build is checked by a script for canaries and frame size.
7. **G2.**
   - The control's concrete form is `nlocalsplus` as the sixth tail-call
     argument.
   - Replace the reader list with the four literal masks and `IR_HANDLER`'s
     about 70 uses.
   - State that G6 works under either outcome, because hidden slots are
     frame variables.
8. **G3.**
   - Fused forms are forward only in v2.
   - Conditional back edges and loop inversion are deferred, with the count
     (fannkuch 0.5%, chaos 0.07%, others 0).
   - Text estimate: 11 handlers at about 1.6 KB each.
9. **G6.**
   - The constant-slot table stores `(slot, co_consts index)`, and the
     executing copy builds the values. Marshal stores no `PyValue` bits.
   - "With G2" becomes "after representation step 9a and after G2's decision
     (A4 or B3)".
   - Add the window caveat: a constant that feeds a call or build window
     keeps its load.
10. **§4.3.**
    - The rule stands for field encodings.
    - "Conditional back edges: allowed in the fused form" becomes "deferred"
      (item 8).
    - The contract's sign-dependent poll rule goes with it.
11. **§4 introduction and IR-D1.**
    - The format steps land as a sequence (B2–B7) after representation step
      9b. Counts are recorded per step, and elapsed time is measured once
      after B7.
    - Phase A precedes them.
    - The contract table is built first (B0a) and checks each format step.
12. **§5 and O1.**
    - The contract is `_PyIR_Formats` extended, one table, exported through
      `_opcode.get_ir_contract()` with no generated Python copy. That decides
      O1.
    - Add the columns "boxes inputs" and "may redirect the PC".
    - Heap effects, guards and result facts are built with the JIT front-end
      switch (B0b).
13. **G10.** The stale docstring, `_PyFb_FAMILIES`' `RESUME`/`JUMP_BACKWARD`,
    `IR_MAX_FORWARD` and the `SystemError` limits are all still present at
    `48bb4d6d1d3`.
14. **§2.4.** Record the per-kernel removed-word counts of §4.1 and the
    `invertible_backedge` counter (`dyn_v3.py`).

---

## 12. Open questions

1. **GCC tail calls.** Is `no_callee_saved_registers` with `musttail` sound
   enough to be the qualified GCC build? The diagnostic link matches the
   computed-goto suite result outside the JIT tests (§9.1), but no Clang
   build was possible here to compare with `preserve_none`. Qualification needs Clang 19+ on the
   build host.
2. **Computed goto after A3b.** Should it stay generated? Upstream keeps it,
   and the proposal keeps it unmeasured. The alternative is to drop it and
   require `musttail`, which GCC ≥ 15 and Clang ≥ 19 both have.
3. **The G2 decision.** It needs a matched elapsed comparison of A4 and B3 on
   a quiet machine. The load during this work was 7–20 on 16 cores.
4. **Generated-corpus `compare_sites`.** The maximum is a precondition of B4.
   The corpus listing must be rebuilt.
5. **The attribute handlers in A1.** Can the ten specialised attribute
   handlers leave the boxing set without a value-form attribute API? This is
   decided by reading `value_attribute_complete` in A1.
6. **Elapsed gain of phase A.** It is unmeasured. The counts say stack
   traffic falls to upstream's level and instructions by about 7%. The time
   effect on a 2.6× gap is for the first matched run after A3a.
