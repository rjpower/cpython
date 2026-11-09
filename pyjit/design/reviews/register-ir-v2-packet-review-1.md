# Review 1: register IR v2 implementation packet

Reviewed: [register-ir-v2-packet.md](../proposals/register-ir-v2-packet.md),
revision 1, at `6afc95f78e0`. Source read: `build/worktrees/pyvalue-rc-universal`
at `48bb4d6d1d3` through `git archive`, as the proposal did. Builds: release
`build/pyvalue-rc-release-20261103`, upstream `build/upstream-release-3a440b6`.
Date: 2026-11-03.

The review's scripts and raw output are in
`build/pyjit-link-wave1/irv2-review/` (`scripts/`, `out/`, `timing*.tsv`,
`mydyn*.txt`, `inv.txt`, `suite-tccl.log`). Large intermediates (trees,
objects, the three linked interpreters) were removed. `tclink.sh`,
`scripts/cgl.sh` and `scripts/tccl.sh` rebuild the interpreters, and
`scripts/repvariants.sh` rebuilds the diagnostic objects.

## Verdict

**Revise.** Measurement supports the direction: state transport first, tail
calls as the target backend. A tail-call link of HEAD, with no other change, runs
the 12 kernels 13% faster than the computed-goto build under GCC, and 12% faster
under Clang 21. The proposal had this binary, but it measured nothing.

These findings stop acceptance:

- The packet drops a decided ownership rule (D4, IR-D3) without saying so.
- Step A2 fixes a contract that the code does not need.
- Step A3a promises no address-taken locals, but it has no plan for the
  transaction records that about 140 handlers pass by address.
- The computed-goto build stays as a second backend, although no measured limit
  forces it.
- Format steps after B2 would change the `.pyc` format without bumping the
  magic number.
- Step 7 overlaps A1/A2 in `ceval.c`, although the proposal says it does not.
- The estimate is low.

Most of the evidence the proposal gives does hold. The summary claims more than
the evidence shows.

## Evidence re-derived

| Number in the proposal | How it was re-derived | Result |
|---|---|---|
| 290 `&next_instr` sites in 185 handlers, 11 helpers; 5 in `ceval_macros.h`, 2 in `bytecodes.c` | `grep -o '&next_instr'` on `generated_ir_cases.c.h`, with an `awk` count per `TARGET` block and a call-name tally | **Holds**: 290, 185, the same 11 helpers with the same counts (165, 38, 28, 26, 10, 7, 6, 4, 2, 2, 2). See finding 20 on the macro sites. |
| 177 of 292 handlers never box | A different classifier: list every call whose top-level arguments include `value_temps` or `value_boxes`, per handler | **Holds**. In the 177, the record reaches only `finish_displaced_local` (81 handlers) and `assert`s. In the 115, it reaches `value_operation_finish`, `value_reader_complete`, `value_binary_operation`, `retire_published_values`, `_PyValueTemps_Object` and the attribute and store helpers. |
| Shortest call-free path 48.9 instructions / 12.1 stack refs per word against upstream's 22.0 / 1.3 (59% of entries); 53.0 / 12.3 on 78% | Handler addresses from DWARF `DW_TAG_label` entries of each `ceval.o`, offset to the linked binary (not the table relocations). A BFS over `objdump` (`scripts/sp.py`), my own weighting | **Holds exactly**. The DWARF labels match `labels_rc.json` and `labels_up.json` for all 292 and 231 handlers. The per-handler path lengths and `%rbp` counts agree for every handler. No path uses `%rsp`-relative memory. Finding 2 covers what the measure means. |
| Words that B5–B7 remove (§4.1) | My own `sys.monitoring` classifier (`scripts/mydyn.py`, `mydyn2.py`) on 13 kernels | **B5 holds** exactly on chaos, coroutines, deltablue, fannkuch, nbody, spectral_norm and unpack_sequence (8,805). **B6 is overstated.** It includes the loop inversion that §7 defers: fannkuch has 2,843,309 fused pairs, against the 3,103,959 counted (finding 13). **B7 holds as the stated upper bound**, but 27% of fannkuch's count feeds windows (finding 14). |
| Evaluator 407,484 B with 4,772 calls (upstream 58,419 B, 611); 471 / 257 indirect jumps; C frame 3,304 / 360 B; `ceval.o` text 595,778 / 107,446 B | `nm -S`, `size`, `objdump` counts. I also compiled `ceval.c` from `48bb4d6d1d3` with the release flags | **Holds.** The fresh object has the same text (595,778 B) and loop size (407,484 B). This also confirms the proposal's claim that the dirty release object describes HEAD. |
| `vt` loop −18%; tail-call canaries 276 of 298 (200 under `tcvt`); median frame 160 B, largest 512 B | Rebuilt `tc` with `tclink.sh` and `tcvt` with `variants.py`, `tcpatch.py` and `tcframes.py` in the review directory | **Holds**: 333,672 B; 276 and 200; 160 and 512. |
| GCC tail-call link fails the same 9 files as computed goto | Re-linked with `tclink.sh`. Also linked a Clang 21 variant (`tccl`) and ran its full suite | **Holds**, and holds for Clang too: 9 files, 33 failures, the same files (`suite-tccl.log`). |

## Findings

### 1. Major. The headline claim rests on static counts, and a direct measurement was available

- **Where.** Summary lines 32–41 and 62–64. §1.3 lines 288–290. §6.3 lines
  875–881. §11 item 5.
- **Problem.**
  - "The 39% is per-word cost, and the encoding does not cause it" is a claim
    about time. The packet supports it with instruction and stack-reference
    counts on a static path. HANDOFF says: "Do not present host controls,
    object files or compiler-error reductions as runtime performance."
  - The 39% was measured on build F (`dbce7b1260d` plus the fix), before
    step 5. It is loop *self time*, so it includes the inlined representation
    work. The packet itself attributes that work to step 8 and D4, not to
    phase A (C5).
  - By the packet's own counts, phase A leaves 45.5 instructions per word
    against upstream's 22.0, which is 7% fewer than today. So phase A cannot
    be "the measured 39%".
  - "The encoding does not cause it" is too broad.
    - C4, the slot-class test, is an encoding question: B3 is its format fix.
    - B2's `NOT_TAKEN` test is a format cost on every conditional
      fall-through.
  - §6.3's "about 3.2 times an upstream instruction" divides whole-program
    time (2.6×) by words. 61% of that time is outside the loop (triage §6).
- **Evidence (this review).** The `tc` link (`tclink.sh`) and a copy of the
  release binary in the same layout (`scripts/cgl.sh`) were run with
  `scripts/interleave.sh`:
  - 5 interleaved rounds; each sample is the best of 3 runs of `run(1)`, and
    the table uses the median over rounds;
  - load 1.1 to 3.7, and 2.2 to 3.4 for the repeat (`timing.tsv`,
    `timing2.tsv`).

  | Measure (geometric mean, 12 kernels) | Run 1 | Run 2 |
  |---|---:|---:|
  | GCC tail call / computed goto | 0.870 (per kernel 0.80–0.92) | 0.872 |
  | Clang 21 tail call / computed goto | – | 0.877 |
  | Computed goto / upstream | 2.418 | – |
  | GCC tail call / upstream | 2.104 | – |

  - So tail calls alone, with every escape and every record still in place,
    remove about 16% of the log gap: log 0.870 / log 2.418.
  - The `straight` probe (ten `b = a` per iteration) is 1.04× and 1.01×
    *slower* under GCC tail calls, and 1.15× slower under Clang, presumably
    because the tiny handlers pay a per-function prologue.
  - At HEAD the computed-goto build runs these kernels at 2.42× upstream. The
    triage's F/U on the same 12 kernels was 2.51, with a different harness.
- **Change.**
  - Replace the summary claim. State the timing above as the evidence for
    phase A, and the static counts as diagnosis.
  - Re-split the gap at HEAD with the triage's sampling profiler, which needs
    no `perf`, before quoting shares.
  - Say which per-word costs belong to the format (the class test, the
    `NOT_TAKEN` test) and which to representation work (finding 3).
  - Delete §6.3's 3.2× sentence.
  - Rewrite §11 item 5's justification.

### 2. Minor. The path measures favour the fork, and the fall-through measure follows a path that never runs

- **Where.** §1.1. §1.3 (77.3 / 15.3 and the per-variable columns). §1.4 C1–C3
  evidence.
- **Problem.**
  - The shortest call-free path of a handler that branches on the value is its
    inline-scalar case. `LOAD_FAST_BORROW` reaches 44 instructions by:
    - taking the non-pointer exit at `jb 308b2c`;
    - skipping the borrow-tag block at `303ba0` and the publish tests at
      `308b2e`–`308b59`.

    An object value goes through both: 5 instructions at `303ba0`, then the
    `ob_type` load and two type compares. That is about 61 instructions, with
    one more dependent load. Upstream's handler is 14 either way.
  - The fall-through path of `LOAD_FAST_BORROW` is the not-taken side of
    `jge 308b10` (`2f067b`), the store to a *local*. Every one of the 232,433
    static `LOAD_FAST_BORROW`s writes a temporary (§0.1). The handler with
    13.8% of entries is therefore measured on a path it never executes. The
    77.3 / 15.3 row and C1–C3's per-word reference counts (3.3, 3.5, 1.8, 0.4,
    4.1) come from this measure.
  - The weights come from `Py_STATS`. `INSTRUCTION_STATS` counts only handlers
    below 256 (`ceval_macros.h:70–80`). Handlers 267–304 carry no weight:
    - `IR_JUMP`;
    - the per-operation generic `BINARY_OP_*`, which count as `BINARY_OP`, a
      name with no label, up to 1.7% of entries on raytrace;
    - NOFB and `*_JIT_WARM`.
- **Change.**
  - Call the shortest path what it is: the inline-scalar lower bound.
  - Add the object path for the top ten handlers.
  - Derive the per-variable counts from the shortest path, or drop them.
  - List the handlers that have no weight.

### 3. Blocking. D4 has no step: the packet makes a decided ownership rule conditional without saying so

- **Where.** Step B3 (lines 659–675). The step table, row B3. §10 ("61 if A4
  wins"). register-ir.md IR-D3 (line 1176) and G2's decision rule (lines
  1299–1305). pyvalue-representation-packet.md line 15.
- **Problem.**
  - IR-D3 decides that no home ever holds a borrow (D4), jointly with
    pyvalue.md. The representation packet puts D4 out of scope, "decided with
    the register IR's G2".
  - G2's rule states that "D4 concerns value tags, not slot classes". So D4
    must land whichever of A4 or B3 wins.
  - This packet puts D4 inside B3, which runs "only if A4 does not recover the
    cost". If A4 wins, no step removes the borrow tags from homes.
  - Yet C5 counts on "D4 with G2" to remove `PyValue_BorrowAt`, the largest
    item on the hottest handler's path.
- **Evidence (this review).** Diagnostic variant `rep`
  (`scripts/reppatch.py`, `scripts/repvariants.sh`). In it, `_PyValue_Publish`
  stops canonicalising, which step 8 makes redundant, and `PyValue_BorrowAt`
  returns the slot bits, which approximates D4. It is not behaviour-preserving
  and was only compiled. Per-word figures are weighted shortest paths on the
  same 59% subset (`out/sp-*.json`).

  | Variant | Loop text | Instructions / stack refs per word | `LOAD_FAST_BORROW` |
  |---|---:|---:|---:|
  | base (HEAD) | 407,484 B | 48.9 / 12.1 | 44 |
  | `rep` | 365,548 B (−10%) | 42.6 / 10.8 | 28 |
  | `vt` (A1) | 333,672 B (−18%) | 45.4 / 8.6 | 38 |
  | `vt` + `rep` | 304,791 B (−25%) | 40.8 / 8.3 | 28 |
  | upstream | 58,419 B | 22.0 / 1.3 | 14 |

  - Representation work in two macros is 16 of the 30 excess instructions of
    `LOAD_FAST_BORROW`, 13% of the weighted path, and 10% of loop text. On
    object values (finding 2) it is more.
  - Phase A does not touch it. Even with A1 and both representation changes,
    the loop is 5.2 times upstream's. The rest is loop structure: bridges,
    transactions, class tests and the displaced-local protocol.
    `STORE_FAST` is 45 or 46 in every variant.
- **Change.**
  - Add a D4 step after representation step 8, independent of the A4/B3
    outcome. It carries the count cost (`load_fast_borrow_to_temp`, 3.8–8.5%
    of words) and its tests.
  - B3 keeps only the class bits.
  - Reconcile IR-D3's wording ("the slot class … lives in the instruction")
    with G2's rule in §11.

### 4. Major. A2 fixes the wrong contract: the PC never changes, so the out-parameter can go

- **Where.** Step A2 (lines 470–505). C1. §5's new column "may redirect the
  PC".
- **Problem.**
  - Only two places write through the `_PyInstr **next` parameters. Both
    rebase the PC on `code->_co_exec` (`grep -n '\*next = ' Python/ceval.c`):
    - `ceval.c:1587` (`value_operation_refresh`):
      `*next = exec->words + word`;
    - `ceval.c:2175` (`native_call_finish`):
      `*next = exec->words + state->next_word`.
  - The executing copy cannot move while a frame runs:
    - it is created once, when `_co_exec` is NULL (`ir.c:312–326`);
    - it is freed only in `code_dealloc` (`codeobject.c:2559`,
      `_PyCode_ClearExec`);
    - instrumentation rewrites handler fields in place
      (`instrumentation.c:set_word_opcode`);
    - `value_operation_begin` asserts `frame->ir_words == exec->words`
      (`ceval.c:832`).
  - So both writes store the value `next_instr` already has. Instrumentation
    that reentrant code installs takes effect anyway, because `DISPATCH`
    re-reads `*next_instr`.
  - The proposed return convention (a PC, or NULL with an exception set) also
    needs a second form, the "two-word struct", which the packet already
    anticipates. The current sites show why:
    - 30 sites call `(void)value_operation_finish(...)` and ignore the status;
    - `JUMP_TO_LABEL` treats −1 with an exception set before the call as
      success (`&& !value_had_error`, `ceval_macros.h:131`);
    - `call_transaction_invoke` has a third output, `&call_child`.
  - The representation packet (§3.4) hands IR v2 the question of a
    dispatch-time instrumentation check. This packet does not take it up.
- **Change.**
  - Make A2 "delete the PC out-parameter". Each of the 12 helper families,
    including `finish_value_bridges` (finding 20), keeps its `int` status and
    its refresh and validation. `next_instr` stays a value.
  - Add a debug assert that `frame->ir_words == code->_co_exec->words` after
    each reentrant call, and state the invariant (the executing copy is fixed
    for the code object's life) in register-ir.md §1.10.
  - Drop the "may redirect the PC" column.
  - Answer §3.4's hand-off: either the refresh stays in the helpers, or it
    moves to dispatch.
  - A2 becomes smaller (about 2–3 h).

### 5. Major. A3a's "no address-taken locals" and "canaries 276 → 0" do not follow from the steps it lists

- **Where.** Step A3a (lines 522–535 and 548–556). C6. The step table ("canaries
  276 → 0").
- **Problem.**
  - A3a removes the box arrays, the record and `&res`/`&left`. About 140
    handlers also take the addresses of other locals. A regex count over
    `generated_ir_cases.c.h`:

    | Local | Handlers |
    |---|---:|
    | `&operation` (`_PyValueOperation`) | 38 |
    | `&pending` | 29 |
    | `&held_left`, `&held_right` | 27 each |
    | `&call` (`_PyCallTransaction`) | 15 |
    | `&call_child` | 15 |
    | `&receiver` | 15 |
    | `&callable` | 12 |
    | `&publication`, `&scope` (native calls) | 1 |

  - These are the transaction records the bridge helpers mutate. A3a says
    nothing about where they live.
  - Under `tcvt`, 86 of the 177 never-box handlers still have a canary,
    `LOAD_FAST_BORROW` (48-byte frame) among them. Part of that is the `ni`
    wrapper's `&ir_ni`, which finding 4 removes. The rest is not measured.
  - The canary itself comes from the toolchain's defaults. Ubuntu's GCC
    enables `-fstack-protector-strong`. Clang 21 builds the same source with
    no canaries but still with frames: `tcframes.py` on the Clang object
    reports 0 canaries, median frame 144 B, largest 464 B.
- **Change.**
  - Choose where transaction state lives: in the activation record next to
    the box area, or passed and returned by value.
  - Measure the canary and frame counts after finding 4's change.
  - State the target as "no address-taken locals; frame at most N bytes",
    checked by `tcframes.py`, rather than as a canary count.
  - Re-estimate A3a at about 12–16 h. It now rewrites handler bodies in
    `bytecodes.c` (finding 8).

### 6. Major. Clang is installed; qualify on both compilers now

- **Where.** Line 18 ("Clang is not installed"). §0.2. A3a's build bullet. §9.2.
  §12 item 1.
- **Evidence.**
  - `/usr/bin/clang-21` exists: `Ubuntu clang version 21.1.8`, installed in
    March 2026. `__has_attribute(preserve_none)` is 1;
    `no_callee_saved_registers` is 0.
  - GCC 15.2 has `musttail` and `no_callee_saved_registers` but not
    `preserve_none`: it prints `'preserve_none' attribute directive ignored`.
  - `scripts/tccl.sh` builds `ceval.c` with Clang 21 under `preserve_none`
    (8 s) and links it with the GCC tail-call objects. The full suite fails the
    same 9 files and 33 tests as the GCC link (`suite-tccl.log`). Its timing is
    in finding 1.
  - GCC's `no_callee_saved_registers` is an x86 attribute. The current GCC
    manual ([x86 attributes](https://gcc.gnu.org/onlinedocs/gcc/x86-Attributes.html))
    documents an x86 `preserve_none` that passes integer parameters in r12,
    r13, r14, r15, rdi and rsi and "is subject to change". GCC 15.2 lacks it.
    A web search shows H.J. Lu's GCC work on it, which cites CPython's
    tail-call interpreter
    ([PR 119628](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119628); a
    [2026 follow-up patch](https://patchwork.sourceware.org/project/gcc/patch/CAMe9rOrUyoDVm2_vpSA=G0ZNW-K_eYZ8vAEgd3qqphMEjzaTmg@mail.gmail.com/)).
    The bug page refused automated access, so its release target is
    unverified.
  - Clang documents that `preserve_none`'s "ABI is still unstable"
    ([attribute reference](https://clang.llvm.org/docs/AttributeReference.html)).
  - Upstream:
    - 3.14 ships the backend as opt-in for Clang 19+ only; it notes that "a
      future release of GCC is expected to support this", and recommends PGO
      (`Doc/whatsnew/3.14.rst:438–459`, `Doc/using/configure.rst:809–815`);
    - 3.15 adds MSVC 2026 (`Doc/whatsnew/3.15.rst:2849–2862`);
    - `ceval_macros.h` at `3a440b6` requires both `preserve_none` and
      `musttail` (lines 82–95).
- **Judgement.** The GCC route is not a bet on one compiler. Two compilers
  build the backend, give the same suite result and time within 1% of each
  other (only `ceval.c` was compiled with Clang). But its GCC
  attribute is undocumented for this use and x86-only.
- **Change.**
  - Correct line 18 and §0.2.
  - Under GCC, use `preserve_none` when `__has_attribute(preserve_none)`, and
    `no_callee_saved_registers` otherwise.
  - Qualify A3a on GCC 15 and Clang 21 on this host.
  - Keep handler calling conventions internal: the JIT must enter and leave
    through C-ABI functions only (A3b), because the two conventions put
    arguments in different registers.

### 7. Major. Keeping computed goto after A3b is a second backend with no measured limit

- **Where.** Step A3b, "Computed goto stays generated … not measured" (lines
  577–580). §9.2 ("The unmeasured computed-goto build decays"). §12 item 2.
- **Problem.**
  - After A3b, nothing in the packet needs computed goto. The JIT works under
    tail calls, and both available compilers build tail calls.
  - AGENTS.md makes hard source and binary compatibility a non-gate. "Upstream
    keeps it" is not a measured limit.
  - The packet already finds computed goto behind two second forms: the
    `IR_FIELD_*` memory decode and the JIT labels.
  - A backend kept "compiled and smoke-tested but not measured" is the
    expedient dual form the project excludes. The switch fallback
    (`ceval_macros.h:170–188`) is a third.
- **Change.** Decide in the packet.
  - After A3b, the generator emits only tail-call handlers.
  - A compiler without the attribute uses plain `musttail`: still one source,
    only slower.
  - Delete the computed-goto and switch backends.
  - If a platform needs another backend, name it and the measurement.
  - Until A3b, computed goto stays the qualified build, and the tail-call build
    is a CI configuration (finding 8).

### 8. Major. Ordering: step 7 does edit A1/A2's files; other overlaps and orders are missing

- **Where.** §3, A1/A2 row ("Step 7 keeps every check in `ceval.c` … so it
  does not edit these helpers"), and the forced-order list.
- **Problem.**
  - Step 7 deletes `_PyValue_RestoreException` and its 33 uses in `ceval.c`,
    including those inside the bridge helpers A2 rewrites:
    - `drop_failed_value`, line 1546;
    - `call_transaction_close_value` and `_object`, lines 1683 and 1693;
    - line 1840.
  - Step 7 also changes `DISPATCH_INLINED` in `ceval_macros.h` (one use) and
    the two uses in `bytecodes.c`.
  - It takes the class C, E and F save/restore sites. The representation
    packet counts 53 such sites in `ceval.c` (F 22, B 10, A 9, C 7; packet
    line 650). Step 6b rewrites the release helpers in the same region.
  - A3a's slot-addressed rewrites and box storage edit handler bodies in
    `bytecodes.c`, which 9a also rewrites (the int and float handlers). The
    proposal lists neither file for A3a.
  - Missing cost: once A3a joins CI, every representation step from 7 to 9b
    must pass on two builds. Neither estimate counts it.
  - Unnecessary orders:
    - "A4 or B3 before B7": the run-time test already classes hidden slots
      below `nlocalsplus` as frame variables, so B7 works under today's test.
    - "A2 before A3a" is not forced: A3a builds and passes without A2 and
      gains 13% without it. It is needed only for the frame and canary goal.
- **Change.**
  - Put A1/A2 after step 7, or land step 7's `ceval.c` and `ceval_macros.h`
    part first.
  - List `bytecodes.c` for A3a, and order its handler rewrites after 9a or
    split them by file region.
  - Add the dual-CI cost.
  - Drop the two orders above, or give their reason.

### 9. Major. Every format step needs its own magic number, and B4 and B6 renumber

- **Where.** Forced orders, "B2 first … it carries the magic-number bump …
  so later steps renumber nothing" (lines 369–370). Steps B4–B7.
- **Problem.**
  - `.pyc` files hold the IR words: `marshal.c:999–1018` writes
    `_PyCode_GetCode`, which copies `co->co_words`.
  - B4 (feedback index and NOFB removal), B5 (calls and unpack), B6 (fused
    forms) and B7 (the constant-slot table) each change the marshalled
    format.
  - With one bump in B2, a build at B5 loads `.pyc` files that B4 wrote under
    the same magic. A stale `__pycache__` in a build tree or CI cache is enough
    to trigger this.
  - B4 removes the 7 NOFB handlers and B6 adds 11, so the numbering changes
    again.
- **Change.**
  - Bump `PYC_MAGIC_NUMBER` (64402 today) in every format step.
  - Renumber densely in B2, and accept renumbering in B4 and B6, or reserve
    their numbers in B2.

### 10. Major. The estimate is low

- **Where.** §10 (about 66, or 61).
- **Problem.**
  - A3a: transaction state (finding 5), configure support for two compilers, a
    CI configuration and frame checks. 5 h → about 12–16 h.
  - A3b: moving JIT entry out of loop labels, under both builds. 6 h → about
    8–10 h.
  - D4 (finding 3) has no hours: about 5–8 h.
  - G2's decision rule compares A4 *against* B3. Either B3 or an oracle build
    must exist even if A4 wins, so "61 if A4 wins" does not follow.
  - A4 does not compile as one macro (finding 17). 2 h → about 3–4 h.
  - B0a: about 10 new columns, three hand-written consumers replaced, the
    verifier and `get_ir_contract`. 6 h → about 10–12 h.
  - B4: per-kind sizes, priority assignment, 14 intrinsics, a second
    `LOAD_ATTR` head and the corpus measurement. 6 h → about 8–10 h.
  - Dual-CI work during steps 7–9b: about 2–4 h.
  - A2 shrinks (finding 4) to about 2–3 h. B1 is mostly done (finding 15).
- **Change.** Re-estimate at about 90–110 agent-hours, with the per-step basis
  stated. This is a judgement, not a measurement.

### 11. Minor. Errors in the V8 and LuaJIT table (§2)

Checked against `LuaJIT/src/vm_x64.dasc` and `lj_bc.h` (v2.1), and V8's
`src/codegen/x64/register-x64.h`, `src/interpreter/interpreter-assembler.cc` and
`bytecodes.h` (main).

- **LuaJIT registers.** `edx, edi, esi, ebx` is the x86 (and Windows x64)
  mapping. On x64 POSIX it is BASE `rdx`, KBASE `r15`, PC `rbx` and DISPATCH
  `r14`, each marked "Must be C callee-save" except BASE, which is "Not C
  callee-save, refetched anyway".
- **`savePC`.** There is no `savePC` macro. Handlers store `mov SAVE_PC, PC` to
  a C-stack slot (`SAVE_PC` is `aword [rsp+aword*3]` on POSIX).
- **DISPATCH "also used to swap hook tables".** LuaJIT keeps one dynamic table
  and a static copy. The hook path re-dispatches through
  `[DISPATCH+OP*8+GG_DISP2STATIC]`. DISPATCH's other use is as the base for
  `global_State` and `jit_State` (`DISPATCH_GL`, `DISPATCH_J`), which saves a
  register.
- **"Dispatch-table register: not needed".** Upstream CPython at `3a440b6`
  passes the table as a tail-call parameter (`instruction_funcptr_table`,
  `ceval_macros.h:75–79`). It switches the table for JIT trace recording
  (`DISPATCH_TABLE_VAR`, `ENTER_TRACING`, lines 99–101 and 146–157). Its
  computed-goto loop loads the table variable from the stack. That is the one
  stack reference per word that §1.2 counts (`-0x150(%rbp)` in upstream's
  `LOAD_FAST_BORROW`). The row should weigh this against A4's use of the sixth
  register.
- **V8 `Wide`/`ExtraWide`.** V8 implements them with separate handler-table
  ranges (`DispatchWide`: indices 256–511 and 512–767), so each also triples
  the handlers.
- **V8 around calls.** V8 also reloads the bytecode array after every call "in
  case the debugger has been activated" (`BytecodeArrayTaggedPointer`). That is
  V8's answer to code changing under a reentrant call. It reloads state from the
  frame; it does not return a PC (finding 4).
- **Correct.**
  - V8: `rax`, `r9`, `r12` and `r15`; registers as frame slots off the
    interpreted frame pointer; operands loaded from the bytecode array by
    offset; `SaveBytecodeOffset` in `CallPrologue` unless
    `MakesCallAlongCriticalPath`; `Star0`–`Star15`, `LdaSmi` and `AddSmi`; no
    fused compare-and-jump bytecode.
  - LuaJIT: `ins_NEXT` decodes A, D and OP before `jmp [DISPATCH+OP*8]`; the
    ISLT handler consumes the following JMP (`add PC, 4`, then `PC_RD`);
    `ADDVN` and `KSHORT`.
  - Clang's `preserve_none` registers are r12–r15, rdi, rsi, rdx, rcx, r8, r9,
    r11 and rax.
- **Change.** Fix the six items above, and remove "from memory and not
  re-verified".

### 12. Minor. Techniques the comparison misses that fit a C interpreter

- **Operand predecode in dispatch.**
  - LuaJIT decodes A and D into registers before the indirect jump.
  - Upstream's computed-goto dispatch predecodes `oparg` into `%r11d`
    (`movzbl %ah,%ecx; mov %ecx,%r11d` before `jmp *(%rsi,%rax,8)`), and its
    tail-call build passes `oparg` as a parameter.
  - A3a passes `ir_word` and decodes in each handler, after the jump.
  - The register budget is the design question: six SysV registers under GCC,
    twelve under Clang. The candidates are `frame`, `slots`, `tstate`,
    `next_instr`, `ir_word`, and one of `nlocalsplus`, the table or a
    predecoded field.
  - Also note that `slots` is `frame->base` (a pointer), not an offset from
    `frame`.
- **Lookahead pairing.**
  - V8 inlines a following short `Star` in the current handler
    (`IsStarLookahead`, `StarDispatchLookahead`, `InlineShortStar`), and
    LuaJIT's compare reads its JMP.
  - Both take one dispatch per pair with no new format. That is an
    alternative to weigh against B6's 11 handlers and B5's no-destination
    form, even if the fused fixed form wins.
- **Static operand roles per handler.**
  - In Lib, `LOAD_FAST_BORROW` (50,332 of 51,198 dsts), `LOAD_CONST`,
    `LOAD_SMALL_INT`, `LOAD_GLOBAL` and `PUSH_NULL` write temporaries at
    least 98% of the time, and `LOAD_ATTR` 95% (`scripts/dstclass.py`; my local
    count slightly overcounts cells).
  - If the contract fixes such a dst as a temporary, and the compiler emits a
    `STORE_FAST` for the rare local, the class test leaves the hottest
    handler. No format bit and no parameter are needed. Count this before
    the A4/B3 decision.

### 13. Minor. B6's column counts the deferred inversion, and the inversion count misses loop shapes

- **Evidence.**
  - `steps.py` lines 15–16 add `invertible_backedge` to B6: 260,650 words on
    fannkuch, 5,030 on chaos and 133 on go. §7 defers exactly that.
  - `invertible_backedge` requires the back edge to target the compare itself.
    `scripts/inv.py` also counts heads that start with constant loads, and
    truth-test heads:
    - fannkuch: 260,650 more after a constant load (`while r != 1:` begins
      `LOAD_SMALL_INT 1`), which B7 would turn into compare heads, and
      1,731,245 truth-test heads (`while k:`, 3.3% of words);
    - richards: 106,950 truth-test heads (1.8%);
    - nqueens: 40,319.
  - §4.3 allows backward conditional jumps only in the fused form, so
    truth-test heads need a plain backward conditional jump, which the design
    excludes.
- **Change.**
  - Remove inversion from B6's column. fannkuch becomes 5.4%, not 5.8%.
  - State the head shapes, and say that the deferral stands under §4.3's
    fused-only rule.

### 14. Minor. B7's upper bound includes constants that feed windows

- **Evidence.** `scripts/mydyn2.py` counts, per kernel, the loads that B7
  removes and how many of them a window reads. With the same definition as
  `dyn_v3.py`, the B7 totals match exactly.

  | Kernel | B7 count | Read by a window |
  |---|---:|---:|
  | fannkuch | 14,963,857 | 4,086,019 (27%): `BUILD_SLICE` inputs for `perm[k::-1]` |
  | deltablue | 1,719 | 1,222 |
  | go | 516,598 | 45,854 |
  | others | – | none or nearly none |

- **Change.** Report the tighter bound: fannkuch 20.5% rather than 28.2%, and
  geometric mean after B7 about 0.66 without unpack_sequence.

### 15. Minor. G10's limit errors are mostly done, and B1's test already passes

- **Evidence.**
  - `assemble.c:716–720` (`too_large`) raises `SyntaxError` for word count and
    opargs.
  - On the release build, a function with 5,000 locals raises
    `SyntaxError too many local variables and temporaries in function (needs
    5000 slots, limit 4095)`.
  - Only `regcfg.c:slot_error` (line 792, "too many temporaries") and the
    "two live temporaries" check (line 823) raise `SystemError`.
  - WORKLOG records "register IR limits raise `SyntaxError`" (`7f6501d9669`).
- **Change.**
  - Correct §0.1's row and §11 item 13.
  - Give B1 a test that reaches `slot_error`. Find one: a call or display of
    5,000 elements compiles today without reaching it.

### 16. Minor. There are five literal 0xFFFF handler masks, not four

- **Evidence.** `Modules/_remote_debugging/code_objects.c:495`:
  `PyLong_FromLong((long)(word & 0xFFFF))`. It is an out-of-process reader, so
  it cannot use `IR_HANDLER`. Dense renumbering (B2) also changes the numbers it
  reports.
- **Change.** Add it to §0.1, B3 and §11 item 7, and give it a test under B2
  and B3.

### 17. Minor. A4 is not "one macro and a parameter"

- **Evidence.**
  - The proposal's own `nlp` variant (`variants.py`, `obj/cc-nlp.log`) failed
    to compile with 65 errors. Its docstring gives the reason: code outside the
    loop also tests the class. `IR_IS_LOCAL` (`ceval_macros.h:603`) is used 4
    times in `generated_jit_helpers.c.h` and once in `ceval.c` outside the
    handlers, and `value_operation_restore2` (`ceval.c:841`) reads
    `co_nlocalsplus` itself.
  - A4 takes the last SysV register. Upstream uses that register for the
    dispatch table (finding 11), and the `Py_STATS` build's `lastopcode`
    becomes a stack argument under GCC.
- **Change.** State both. Raise the risk to low-to-medium. Say how helpers
  outside the handlers get `nlocalsplus`.

### 18. Minor. Test sets

- **A3a.**
  - The `test_gdb` modules are skipped on this host ("test_gdb only works on
    source builds", and `gdb` is not installed), so the stack-walking check
    does not run here.
  - Add a debug tail-call build: `PRE_DISPATCH_GOTO` asserts
    `frame == tstate->current_frame`.
  - Add a `Py_STATS` tail-call build (seventh argument).
- **B7.** Add `test_code` (`code.replace(co_consts=…)` must rebuild the
  constant-slot table) and code-object copy and pickle.
- **B2.** Add a check of the `_remote_debugging` opcode output (finding 16).
- **A1.** It measures `vt` against `ni`. A1 alone, against HEAD, is not
  measured. Record it once A1 lands.

### 19. Minor. §11 edits

- **Item 2.**
  - The replacement text ("the spill comes from `&next_instr` escaping")
    contradicts §1.3. Under computed goto, the `ni` variant removes every
    escape and GCC still keeps `next_instr` in its stack home. Write: "under
    computed goto, `next_instr` stays in memory with or without the escapes;
    under tail calls, the escapes are what keep it there".
  - "builds and passes" should read "builds and fails the same 9 files in one
    diagnostic run, JIT compiled out".
- **Item 5.** The justification should be the measured tail-call gain
  (finding 1), not "the measured 39%".
- **Item 7.** Five masks (finding 16). The control form does not compile as
  stated (finding 17).
- **Item 8.** Give the head shapes (finding 13).
- **Item 13.** Limit errors (finding 15).
- **Missing edits.**
  - IR-D3's wording against G2's rule, and D4 as its own step (finding 3).
  - §1.10: the executing copy is fixed for the code object's life
    (finding 4).
  - §1.13: Clang 21 is present (finding 6).
  - §4's churn argument for one encoding revision ("would churn all of those
    four or five times"): say that the contract table (B0a) answers it, since
    the decoders read the table. Otherwise IR-D1's rationale stands against
    six format steps.

### 20. Minor. `finish_value_bridges` is a twelfth escaping helper, reached from every handler

- **Evidence.** `DISPATCH` (`ceval_macros.h:272–281`), the three
  `JUMP_TO_LABEL`s (lines 127–135, 155–161 and 173–180) and
  `DISPATCH_INLINED` (lines 291–303) pass `&next_instr` to
  `finish_value_bridges`. Every handler dispatches, so the escape is in all
  292 handlers. A1 removes it from the 177 by constant folding
  (`value_temps == NULL`).
- **Change.** List it in C1 and A2. "185 handlers" is the count of generated
  sites only.

## Checked and found sound

- Every number in the evidence table, apart from the B6 and B7 caveats.
- The `.pyc` rule for B7 (no `PyValue` bits in marshal) is necessary, because
  marshal writes `co_words`.
- `pycore_jit.h:157–158` compiles the baseline JIT out under
  `_Py_TAIL_CALL_INTERP` (and under `Py_STATS` and free threading).
- The JIT locals and labels that A3b lists exist (`ceval.c:2209–2214`; labels
  at 2290, 2304, 2320 and 2365).
- Upstream's register assignment in §1.2: `next_instr` `%r12`, `frame` `%rbx`,
  `stack_pointer` `%r13`, `oparg` `%r11d`.
- `tcpatch.py`'s two edits are the minimum that builds the backend under
  GCC 15.2.
- The tail-call handlers push only `%rbp`.
- A per-thread box buffer would not survive nested evaluations. An
  activation-owned area is right.
- B4 before B6, and A4 before B3, follow from G5 and G2.
- B2 before the other format steps is sensible for `NOT_TAKEN` and the dead
  handlers.
- B5's word counts, and the `unpack_sequence` result of 9,667 → 862.
- Deferring loop inversion stands under §4.3's fused-only rule, on corrected
  counts.
