# Review 1 of `register-ir.md`

Reviewed 2026-11-03 against `build/worktrees/pyvalue-rc-universal`. The
document cites `5e977ec8cbb`. The tree was at `0ab5bc42f61` when the review
started; the cited files were unchanged up to that commit. Two commits landed
during the review: `c66a0b29253` (copy-propagation kill, which closes G1) and
`6a7b3769d62` (PyValue ownership tags). Measurements were rerun on
`build/rc-integrated-release-20261101` and `build/upstream-release-3a440b6`.

## Verdict

**Not accepted yet.** Section 1 is accurate. Of the 20 claims checked, 16
hold, three are small wording errors, and one (G1) became stale during the
review. Five spot-checked measurements
reproduce. The comparison is broad, and most of the eleven gaps are correctly
diagnosed for the interpreter.

The document does not yet answer the owner's question, "is the IR cleanly
defined?", for its second consumer, the JIT. There is no single authoritative
contract per instruction. Operand roles are in `_PyIR_Formats`. Window extents
and effect flags are reached indirectly through the `shadow` stack opcode.
Feedback families are a hand-written list. Some slot reads and writes are
encoded as immediates. Whether an instruction may run Python code depends on
its operands. The existing baseline JIT therefore treats every word as an
opaque helper call.

The document also leaves out the state at a word boundary that a JIT exit or
OSR entry must respect, and the feedback that an optimising JIT would read.
Several proposals need correction:

- G4: `PUSH_NULL` is already redundant under the NULL invariant, and the unpack
  proposal has no field for its input.
- G5: lazy allocation conflicts with operands stored in the feedback entry.
- G6 and G7: the comparisons with JSC and V8 are wrong.
- G10: it introduces new limits measured only on Lib.

The format changes should be decided together rather than landing one at a
time.

## Claims checked against source

| # | Claim (§) | Where checked | Result |
|---|---|---|---|
| 1 | Word layout: 16-bit handler, then A, B, C, D in 12 bits; CD has 24 bits (§1.2) | `pycore_ir.h`: `IR_A`…`IR_CD`, `IR_WORD` | holds |
| 2 | `IR_FIRST_ONLY_HANDLER` = 267, `IR_NUM_HANDLERS` = 305, 293 valid formats, 292 `TARGET`s, 7 NOFB handlers (§1.2, §1.10) | `pycore_ir_metadata.h`, `generated_ir_cases.c.h` | holds |
| 3 | "`BINARY_OP_<op>` × 26" (§1.6) | `pycore_ir_metadata.h` | **wrong**: there are 27 handlers (`NB_ADD`…`NB_SUBSCR` = 0–26) |
| 4 | "83 formats are window style" (§1.3) | metadata rows | **wrong unit**: 83 *handlers* are window style; they have 11 distinct window formats (64 distinct formats in all) |
| 5 | The format rows in the §1.3 table (`LOAD_GLOBAL`, `STORE_SUBSCR`, `STORE_ATTR`, `FOR_ITER`, `CALL`, `UNPACK_SEQUENCE`, `POP_JUMP_IF_FALSE`, `JUMP_BACKWARD`, `LOAD_CONST`, `IMPORT_NAME`) | `_PyIR_Formats` | hold |
| 6 | `GET_ITER` is window style (§1.6, Iteration row) | `_PyIR_Formats[GET_ITER]` | **wrong**: it is register style, `{DST, IN0, FB, IMM}` |
| 7 | Slot limit 4,095 (fields 0–4,094); `SystemError` from `assemble_words` (§1.14) | `assemble.c:assemble_words`; interim build with 5,000 locals | holds, message identical |
| 8 | `IR_IS_LOCAL` loads `nlocalsplus` from the header before the words; the listed macros appear on 890 generated lines (§1.4) | `ceval_macros.h:IR_NLOCALSPLUS`; grep | holds (890; 1,040 if `IR_STORE_INPUT`, `IR_STORE_TEMP_DST`, `IR_MOVE_TO_DST` and `IR_BORROW_KNOWN` are included) |
| 9 | Entry 0 is shared past `IR_FIELD_MAX`; FBARG handlers switch to NOFB (§1.5.5) | `assemble.c:assign_handlers` | holds; NOFB forms keep the oparg in CD (24 bits) or D |
| 10 | Relaxation: 4 words for `FOR_ITER`, 3 for `SEND`; `IR_MAX_FORWARD` = 2047; `SystemError` past 2^24 words (§1.5.5, §1.8) | `assemble.c:layout`, `RELAXED_*_WORDS` | holds |
| 11 | Entry stride is `sizeof(_PyFbAttr)` = 40 B; header is 16 B; the kind sizes in the table (§1.10) | `pycore_feedback.h` | holds; the header also has an unused `misses` field (2 B) |
| 12 | Entry 0 is a COUNTER entry in state OFF; branch history starts at 0x5555; the free-threaded build turns the counters off (§1.10) | `ir.c:make_exec` | holds |
| 13 | Exception entry holds `exc_slot << 1 \| lasti`; binary search above 40 bytes; clearing starts at `exc_slot − lasti` (§1.9) | `assemble.c`, `ceval.h:get_exception_handler` (`MAX_LINEAR_SEARCH` 40), `bytecodes.c` label `exception_unwind` | holds |
| 14 | Frame header is 13 words; two stacks share one chunk (§1.12) | `FRAME_SPECIALS_SIZE`, `_PyThreadState_PushFrameUnchecked` | holds (`return_slot` is `uint16_t`) |
| 15 | `gi_yieldfrom` reads `return_slot − 2` (§1.12) | `genobject.c:269` | holds |
| 16 | Dispatch: `NEXTOPARG`; 16-bit memory re-reads of the fields in the GCC computed-goto GIL build; `instr_ptr` saved in 289 of 292 handlers; `value_boxes` in all 292 (§1.13) | `ceval_macros.h`, grep | holds (CD is read with a 32-bit load) |
| 17 | JIT heat is a per-code `loop_heat` that saturates at 256 (§1.13) | `jit_runtime.c:1085` | holds; `make_exec` seeds the back edges |
| 18 | G1: the kill was removed in `1440f8c6fd1` and no `RUNS_CODE` symbol remains | `git show 1440f8c6fd1` | held at `0ab5bc42f61`; **stale** since `c66a0b29253` |
| 19 | The `ir_generator.py` docstring is stale (G10) | docstring | holds ("handlers keep stack_pointer", "folds no load yet") |
| 20 | Worked example: listing, raw words 1/4/6/7/9/10/16, 21 words, `co_stacksize` 5 (§1.15) | interim build | holds, word for word |

## Measurements spot-checked

| Measurement | Document | Rerun | Verdict |
|---|---|---|---|
| G1 test (`outer()`) | 100 / 1 | 100 / 1 | reproduces |
| Lib: files, code objects, words | 2,071 compiled (11 fail), 99,831, 4,574,431 | 2,082 files, 11 fail, 99,831, 4,574,431 | reproduces |
| Constant loads | 855,432 (18.7%) | 448,208 + 234,865 + 138,431 + 26,957 + 6,971 = 855,432 | reproduces |
| Slots per frame: max / p50 / p99 / p99.9 / frames > 256 | 803 / 7 / 25 / 39 / 1 | same | reproduces |
| `COMPARE_OP` that feeds the next conditional jump | 17,381 of 21,564 (80.6%) | same | reproduces |
| Static shares of `POP_TOP`, `PUSH_NULL`, `LOAD_FAST_BORROW`, `STORE_FAST`, `NOT_TAKEN`, `STORE_NAME` | 7.5 / 2.6 / 4.9 / 0.9 / 1.8 / 4.2% | same | reproduces |
| unpack_sequence dispatches, IR / upstream | 9,669 / 5,679 | 9,658 / 5,668 | same ratio (1.70); 11 extra events in both, so the harness boundary differs |
| `ceval.o` text | 223,198 / 107,446 | same | reproduces; see finding 7 |

## Findings

**Accuracy and provenance**

1. Header, §1.2, §1.5.3, G1, G10. Weak.
   - Finding: `c66a0b29253` restored `_PyIR_RUNS_CODE` (flag 128, which fills
     the `uint8_t` flags field), the kill and the regression tests
     (`test_peepholer`). "No `RUNS_CODE` symbol remains" and the G1 "When" are
     stale. `6a7b3769d62` changed the PyValue tag rules that §1.4 relies on.
   - Required change: pin to the new HEAD. Mark G1 landed and cite the commit.
     Add `RUNS_CODE` to the flags list. Re-read §1.4 against `6a7b3769d62`.
2. §1.6, Arithmetic row. Wrong (minor).
   - Finding: there are 27 per-operation `BINARY_OP` handlers, not 26.
   - Required change: fix the count.
3. §1.3. Wrong (unit).
   - Finding: "83 formats" counts handlers.
   - Required change: write "83 handlers (11 distinct window formats)".
4. §1.6, Iteration row. Wrong (minor).
   - Finding: `GET_ITER` is register style.
   - Required change: fix the style column.
5. §1.4, "Ownership is decided at run time". Wrong (precision).
   - Finding: a temporary can hold a *borrow-tagged* value. `LOAD_FAST_BORROW`
     into a temporary sets the tag (`PyValue_BorrowAt`). `IR_IS_LOCAL` decides
     the **slot transfer**: whether the reader NULLs the slot, and whether an
     in-place read sets the borrow tag. The reference's ownership is the
     **value tag**. These are two separate facts, and the document merges them.
     G2's "owned temporary" bit inherits the confusion.
   - Required change: state two rules, slot class (frame variable or transfer
     temporary) and value tag (owned or borrowed reference), and say which one
     a JIT must preserve where. Rename G2's bit "temporary" or "transfer".
6. §1.14, §2.3, G2, G8, G10. Weak.
   - Finding: every limit is justified by Lib alone. Lib has no
     machine-generated code (protobuf `_pb2` modules, generated parsers, large
     literal tables), which is where the slot, feedback-site and name limits
     would be reached.
   - Required change: re-measure the maximum and p99.99 of slots, feedback
     sites, names and constants per code object on a corpus with generated
     code, such as the `.py` files of the top PyPI wheels. State the policy for
     code past each limit.
7. §2.3, interpreter size. Vague.
   - Finding: "The two configure lines were not compared." They differ only in
     `--with-shapes`: interim `CC=gcc --with-mimalloc --with-computed-gotos
     --with-shapes`, upstream the same without it. Neither uses PGO or LTO.
   - Required change: say so.
8. §2.4, method. Weak.
   - Finding: the scripts "were not kept". The unpack_sequence rerun differs
     by 11 events on both builds. INSTRUCTION monitoring replaces handlers with
     `INSTRUMENTED_INSTRUCTION`, so the counts are of words executed, not of
     the uninstrumented dispatch path (`RESUME` skipping, JIT admission
     handlers).
   - Required change: check the static and dynamic scripts into
     `pyjit/bench/`, state the harness boundary, and state what the counts
     exclude.
9. G9. Vague.
   - Finding: "0.871× upstream over 29 workloads" gives no direction (speed or
     time), build or date. This is the most important performance fact: the IR
     dispatches 10–41% less yet is not faster. It is buried in a gap entry.
   - Required change: state it in §2 with its direction and provenance.

**Is the IR cleanly defined?**

10. §1.2–§1.3. Missing (major).
    - Finding: there is no single contract per instruction. A JIT front end
      must combine five sources:
      - `_PyIR_Formats`: roles and flags;
      - the `shadow` opcode's stack effect: `_PyOpcode_num_popped/pushed`,
        which give window extents;
      - the `shadow` opcode's `HAS_ERROR`, `HAS_ESCAPES`, `HAS_DEOPT`,
        `HAS_EVAL_BREAK`, `HAS_CONST`, `HAS_NAME` and `HAS_LOCAL` flags;
      - the hand-written `_PyFb_FAMILIES`, already inconsistent (`RESUME`,
        `JUMP_BACKWARD`);
      - `choose_handler` and the handler C code.

      `_PyJitHomeEffect` copies only roles, `ninputs`, `borrow_mask` and
      flags, and the baseline JIT treats every word as an opaque helper.
      V8's `bytecodes.h` gives each operand a type with a direction (`kReg`,
      `kRegOut`, `kRegList` + `kRegCount`, `kRegOutPair`, `kIdx`, `kImm`,
      `kUImm`, `kFlag8`) and states the implicit accumulator use. LuaJIT's
      `BCDEF` gives a mode per field (dst, base, var, rbase, uv, lit, lits,
      pri, num, str, func, jump) and a metamethod column, which means "may
      call Lua".
    - Required change: add a section "Instruction contract", backed by a
      generated table with one row per handler. Each row gives:
      - slots read and written, with window extents as a function of the
        oparg;
      - which inputs are consumed and which are read in place;
      - may raise, may run Python, may deoptimise (and to which handler), may
        jump (and the target encoding), may suspend, may push a frame, polls
        the eval breaker, writes feedback.

      Export the table through `_opcode`. State plainly that an instruction
      means "its shadow stack instruction, with operands bound by the
      format".
11. §1.3 table. Missing.
    - Finding: some slot operands are encoded as `IMM`:
      - `LOAD_FAST*` and `LOAD_FAST_AND_CLEAR` read (and clear) slot B;
      - `STORE_FAST` and `DELETE_FAST` write slot B and release its old value;
      - `LOAD_DEREF`, `STORE_DEREF` and `MAKE_CELL` use the cell held in
        slot B.

      Def-use built from roles misses all of these. The generator has
      `imm_kind`, but `_PyIRFormat` does not export it.
    - Required change: give these fields slot roles (for example `LOCAL_IN`,
      `LOCAL_OUT`, `CELL`), or export the kind of each `IMM`.
12. §1.4 and the new `RUNS_CODE`. Missing.
    - Finding: "may run Python" depends on the operands. A `STORE_DST` format
      whose dst names a local releases the old value (`IR_STORE_DST` calls
      `_PyValueTemps_RetireLocal`), and that can run `__del__`. At
      `c66a0b29253`, `LOAD_CONST`, `LOAD_SMALL_INT_INLINE`, `LOAD_FAST_BORROW`,
      `LOAD_DEREF` and `PUSH_NULL` are `STORE_DST` formats without `RUNS_CODE`.
      Copy propagation is sound only because it runs before store elimination.
      That pass order is load-bearing and undocumented.
    - Required change: define effects per word (the format plus the kinds of
      its operands). Document the pass-order dependency, and make the verifier
      reject a folded load whose reader comes after a write to another local
      through dst.
13. §1.4. Missing.
    - Finding: the interpreter state at a word boundary is not stated as a
      contract: live slots, the NULL invariant, the tag rules, the meaning of
      `instr_ptr`, `return_slot` for a pending call, and the exception slot
      and lasti. Liveness from `_PyRegCfg_Liveness` is discarded after
      compaction. JIT exits, OSR entry (`jit_enter` at "the current word") and
      debuggers all depend on this state.
    - Required change: add "State at a word boundary". Decide whether liveness
      is emitted (per block entry and exception edge) or recomputed from the
      contract table (finding 10).
14. §1.8. Weak.
    - Finding: there are two jump-target encodings (absolute in CD; a forward
      distance in D for `FOR_ITER` and `SEND`), and G3 adds a third, a signed
      relative D. Backward conditional jumps and their eval-breaker check are
      not addressed.
    - Required change: adopt one rule ("CD is absolute; a 12-bit D is a signed
      distance"), name `_PyCode_InstrTarget` as the only decoder, and decide
      whether conditional back edges exist.
15. §1.5.1, §1.6. Weak.
    - Finding: `LOAD_FAST_LOAD_FAST`, `LOAD_FAST_BORROW_LOAD_FAST_BORROW`,
      `STORE_FAST_LOAD_FAST` and `STORE_FAST_STORE_FAST` have formats and
      `TARGET`s, but Lib emits none of them. Lib also has no `EXTENDED_ARG`.
      They are dead handlers that a JIT must support or reject, and they add to
      the G9 text size.
    - Required change: remove them from the IR handler set, so that it equals
      what the compiler, the executing copy and instrumentation can produce.
      Recount 292 and 305.

**Comparison completeness**

16. §2.2, accumulator row. Weak.
    - Finding: "Memory-homed slots are also the deopt state" is true of V8 as
      well, whose registers are frame slots. The real trade-off: V8 keeps the
      latest result in a machine register across dispatch, while this IR
      stores every intermediate to a slot, reloads it, and NULLs the slot on
      consume.
    - Required change: measure slot loads and stores per dispatch on the
      kernels, and argue the choice against dispatch count (0.59–0.90×).
17. §1.4, §2.2. Missing.
    - Finding: the NULL invariant costs a store per consumed temporary
      (`IR_CONSUME_INPUT`). Neither the stack machine nor V8 nor LuaJIT pays
      it. The reason is not stated: there is no stack pointer, so the GC,
      frame walkers and the unwinder must see dead slots as NULL.
    - Required change: state the reason and the cost, and add it to G9 as a
      measured item.
18. §2.2. Missing. Each item below is marked I (matters for the interpreter)
    or J (matters only for the JIT).
    - Finding: the feature table has no rows for:
      - deopt and frame-state metadata, and liveness maps (J);
      - OSR entry points (J);
      - generator suspension and resume points (J; the baseline JIT rejects
        `CO_GENERATOR`);
      - value and result-type profiles, and call counts (J; JSC value
        profiles, V8 call counts and binary/compare hints);
      - feedback in the free-threaded build (J; none, because specialisation
        is off);
      - upvalue and cell access: LuaJIT `UGET`/`UCLO` and V8 context slots
        against cells in slots and `COPY_FREE_VARS` on every call
        (`LOAD_DEREF` 0.7% and `COPY_FREE_VARS` 0.2% of static words) (I);
      - global caching (V8 property cells) (I);
      - the size of the line and exception tables against upstream (I);
      - `NOT_TAKEN` (I).
    - Required change: add these rows, each marked I or J.
19. §1.8. Missing (new gap).
    - Finding: `NOT_TAKEN` is 80,618 static words (1.8%), 8 B each, plus a load
      and a compare on every fall-through, kept only as a monitoring location.
      The instrumented branch twin can fire `BRANCH_LEFT` itself; G3 already
      proposes this for fused sites.
    - Required change: add a gap to drop `NOT_TAKEN` everywhere.
20. §1.15. Missing.
    - Finding: in the example, the path where the `if` is not taken spends
      `POP_JUMP_IF_TRUE` and `JUMP_BACKWARD`, two dispatches, because
      conditional jumps only go forward. A backward fused compare-and-branch
      would make that path 3 dispatches, not 4.
    - Required change: discuss loop inversion and conditional back edges
      (which need a periodic check), or say why they are excluded.
21. §2.1, G7. Wrong.
    - Finding: G7 implies that the other VMs count heat per loop. V8's
      interrupt budget is per function (the `FeedbackCell`), and `JumpLoop`'s
      loop depth arms OSR against an urgency kept per function. JSC's
      `loop_hint` increments the `CodeBlock`'s counter. Only LuaJIT counts per
      loop, with hashed per-PC hot counters.
    - Required change: correct the comparison. Split G7: per-code heat with OSR
      at the hot back edge is enough for a method JIT; per-loop counters are a
      tracing-JIT need (trace anchors).
22. G6. Wrong.
    - Finding: "As JSC does": JSC reads constants from the `CodeBlock`'s
      constant registers by an index-range test and copies nothing into the
      frame. G6 proposes hidden frame slots initialised at entry or at a "loop
      preheader", which the IR does not have (G7). The 18.7% (static) and
      3.3–31.9% (dynamic) figures include strings, tuples and code objects,
      which G6 does not cover.
    - Required change: correct the comparison. Measure the covered subset
      (inline scalars used in loops) statically and dynamically. State the
      preheader dependency and the entry cost for short functions. Compare
      with Lua's constant base (`KBASE`).

**Gaps and recommendations**

23. G1. Done.
    - Finding: landed in `c66a0b29253`. Finding 12 is the remaining caveat.
    - Required change: mark G1 done.
24. G2. Weak.
    - Finding: the evidence is a static count of frame-pointer references
      (note 06 §12.3), not time. A control with no format change exists: keep
      `nlocalsplus` in dispatch state, or load it once per handler. The kind
      bit also adds an AND to every slot access (`slots[F & 0x7FF]`).
    - Required change: add the control experiment and state the decode cost.
25. G3. Weak.
    - Finding: "About 16 handlers" omits the instrumented twins that fire the
      branch events, and the NOFB forms needed unless G10 lands first.
      Backward targets are unaddressed (finding 20).
    - Required change: list the handlers exactly, with an estimate of the
      added evaluator text.
26. G4.2. Wrong approach.
    - Finding: `PUSH_NULL` writes NULL into a temporary that the NULL invariant
      already guarantees is NULL; `IR_STORE_DST` asserts NULL-or-equal on
      temporaries. It is a dispatched no-op (2.6% of static words, up to 7.4%
      of dispatches) that still saves `instr_ptr` and sets up `value_boxes`. A
      "push NULL" bit would be a second form.
    - Required change: the compiler omits `PUSH_NULL` into a temporary, and the
      verifier proves the slot dead there. Treat `LOAD_GLOBAL`'s NULL bit the
      same way. Then `PUSH_NULL` keeps only its local-dst case, or goes.
27. G4.4. Missing.
    - Finding: the proposal does not say where the sequence lives. Today it
      sits at the window base, and the outputs overwrite it. If the base names
      a local, the input needs its own field; D is free in `UNPACK_SEQUENCE`'s
      format. That field also removes the `LOAD_FAST_BORROW` of the sequence
      (800 of 9,658 dispatches, 8.4%, on unpack_sequence). The proposal also
      omits `UNPACK_EX`, and the generic path's scratch buffer when n is large.
    - Required change: specify `{A: base, B: count, C: fb, D: source}` and the
      generic path.
28. G4.3. Weak.
    - Finding: "Keep the argument copies" is right for calls. Single-input
      window formats with a free field could read a source slot instead.
    - Required change: list each window format and its free fields, and decide
      for each.
29. G5. Weak / wrong.
    - Finding:
      - (a) Per-kind arrays need one base per kind per code object, which is a
        table or an extra load, so "no table" is wrong. A fixed-form
        alternative is an fb field that holds an offset, in 8- or 16-byte units,
        into one packed vector: one shift, no table, and a measured limit of
        4,095 units.
      - (b) Lazy allocation conflicts with operands kept in the entry
        (`FBARG`): before allocation, those handlers have nothing to read. G5.2
        therefore depends on G10.
      - (c) "About a fifth" is given without per-kind counts.
    - Required change: fix the dependency order, give the per-kind counts, and
      evaluate the offset encoding.
30. G7. Weak.
    - Finding: see finding 21. A feedback entry per loop also adds a counter
      read-modify-write on every back edge for good.
    - Required change: compare per-loop entries with LuaJIT's hashed counters.
      Decide by consumer.
31. G8. Weak.
    - Finding:
      - The bit budget after G2 (kind bit), G3 (signed D), G4 (sentinel), G7
        (fb in A) and G10 (oparg in D) is nowhere in one place.
      - Decode cost is not discussed. Fields of 12 bits need a shift and a
        mask, which is why GCC spills. LuaJIT's byte-aligned fields need one
        `movzx`.
    - Required change: add a table of the target word, every format after
      the proposed changes, and a paragraph on decode cost.
32. G10, NOFB. Weak.
    - Finding:
      - Moving `LOAD_ATTR`'s name into D sets a 2,047-name limit, where names
        reach 2^24 today. The limit is measured on Lib alone (finding 6).
      - Making `COMPARE_OP` and `CONTAINS_OP` one handler per operation adds
        about 14 generic handlers (6 operations × the bool bit, plus 2). These
        multiply with G3, against the lesson of G2 and G6.
      - An alternative with one rule: every site gets its own entry, with a
        measured hard limit (`SyntaxError` past 4,095 sites; Lib's maximum is
        1,181). That needs no NOFB forms and no new handlers.
    - Required change: compare the two options on the wider corpus, with
      handler counts.
33. Summary, order. Weak.
    - Finding: G2, G3, G4.1, G4.4, G7 and G10 each re-encode the words. Each
      bumps the pyc magic number and changes `dis`, `mark_stacks` and the JIT
      decoders, and they are scheduled one at a time. The owner wants little
      churn, and wants the JIT to build on a settled IR.
    - Required change: decide the format changes together as one encoding
      revision ("IR v2") before the method JIT consumes words. Measure the
      interpreter effect as one step after baseline.

**What implementation needs**

34. Missing.
    - Finding: these decisions are open and not listed:
      1. where the contract table lives (finding 10);
      2. the state at a word boundary and whether liveness is exported;
      3. one jump-target rule, and conditional back edges;
      4. the feedback-site limit policy: NOFB or a hard limit;
      5. the naming of slot class and value tag;
      6. whether constants become slots, which changes the frame layout the
         JIT sees;
      7. feedback in the free-threaded build;
      8. generator resume in compiled code;
      9. per-loop or per-code heat.
    - Required change: add a section "Open decisions", each with an owner and
      the stage it blocks.
35. Missing.
    - Finding: the interfaces the JIT needs are not listed:
      - the generated contract table;
      - per-word decoders for slots read and written (`_PyCode_InstrReads` and
        `_PyCode_InstrWrites`, or an equivalent);
      - liveness at block entries and exception edges;
      - an OSR entry predicate;
      - an exit and resume API that writes back slots, NULLs dead
        temporaries, sets `instr_ptr` and returns to the interpreter;
      - a feedback reader with its version and borrow rules (entries hold
        borrowed pointers that are valid only while versions match);
      - invalidation on instrumentation and version changes.
    - Required change: add a section "Interfaces for the JIT".

## Changes required before acceptance

1. Re-pin to the current HEAD. Mark G1 landed (`c66a0b29253`). Fix findings 2–4
   and 7. Re-read §1.4 against `6a7b3769d62` (findings 1–4, 7).
2. Rewrite the ownership rule as slot class plus value tag, and rename G2's bit
   (finding 5).
3. Add "Instruction contract", backed by a generated, exported table: slot
   reads and writes including window extents, consumed or read in place, may
   raise, may run Python, may deoptimise, may jump, may suspend, may push a
   frame, polls the eval breaker, writes feedback (findings 10–12).
4. Add "State at a word boundary" (live slots, NULL invariant, tags,
   `instr_ptr`, `return_slot`, exception slot) and decide on liveness export
   (finding 13).
5. Fix one jump-target rule and decide on conditional back edges (findings 14,
   20).
6. Remove dead handlers from the IR set and recount (finding 15).
7. Correct the V8, JSC and LuaJIT comparisons in G6 and G7, and split G7 by
   consumer (findings 21, 22, 30).
8. Replace G4.2 with omitting `PUSH_NULL` under the NULL invariant. Give G4.4 a
   source field and a generic-path design (findings 26–28).
9. Fix G5's dependency on G10 and evaluate the offset encoding (finding 29).
10. Re-measure every limit on a corpus with generated code. Compare NOFB
    removal with a hard limit on feedback sites (findings 6, 32).
11. Add the accumulator trade-off, the cost of the NULL store, a `NOT_TAKEN`
    gap and the missing rows in §2.2, each marked I or J (findings 16–19).
12. Add the bit-budget table for the target word and batch the format changes
    into one encoding revision in the summary order (findings 31, 33).
13. State the direction and provenance of the 0.871× result, and check the
    measurement scripts into `pyjit/bench/` (findings 8, 9).
14. Add "Open decisions" and "Interfaces for the JIT" (findings 34, 35).

## Not verified

- The tail-call backend: the handler signature and the frame load from
  `tstate`. Only the computed-goto build was read.
- Free-threaded behaviour beyond `make_exec`.
- Note 06 §12.3 (2,021 against 3,736 frame-pointer references) and the
  results of the constant-operands and slot-base experiments.
- Dynamic counts other than unpack_sequence. The memory figures in §2.3 were
  checked for arithmetic only.
- Facts about V8, LuaJIT, JSC and Lua, which are stated from the reviewer's
  knowledge of their public sources and were not re-read for this review.
