# Internal register IR and feedback vector (Phase 1, item 1z)

Design note for idea 0 of the plan. Every `file:line` refers to commit
`321925c92c9`. Measurements come from `build/baseline-default/python`; step 0
turns the scripts into a harness.

## Decisions

1. 64-bit words: a 16-bit handler index and three 16-bit fields, with a second
   word for three operands plus feedback. Slots index `localsplus`.
2. Dispatch stays one table load; specialisation rewrites the handler in place.
3. One typed feedback entry per specialisable instruction, beside the IR. Seven
   kinds replace fifteen cache layouts.
4. Lazy lowering from `co_code`. Temporary d lives in slot `nlocalsplus + d`.
5. Handlers are generated from the existing DSL bodies.
6. Tier 2 and the JIT go first, in their own step.
7. `co_code`, `dis` and line numbers do not change; `f_lasti` differs in one
   case (4.5).

## 0. Facts the design rests on

- Stack depth is a static function of opcode, oparg and edge.
  `calculate_stackdepth` rejects inconsistent code (Python/flowgraph.c:819), and
  `mark_stacks` recomputes depths from `co_code` at run time
  (Objects/frameobject.c:1392).
- The compiler proves which loads may borrow (`optimize_load_fast`,
  Python/flowgraph.c:2903, criteria at :2872-2890) and records the proof in the
  opcode `LOAD_FAST_BORROW` (Python/bytecodes.c:290).
- Closing a borrowed `_PyStackRef` does nothing; stealing one creates a new
  reference (Include/internal/pycore_stackref.h:529-563).
- The DSL already separates use from release: `_BINARY_OP_ADD_INT` returns its
  inputs as `l, r` and the macro closes them (Python/bytecodes.c:679, :712).

| Measure, `compile()` of every file in `Lib/` (tests excluded / included) | Value |
|---|---|
| Code objects | 23,679 / 99,830 |
| Largest `co_nlocalsplus + co_stacksize` | 70 / 803 |
| Largest `co_consts`, `co_names` | 4,048 / 368 |
| Longest jump | 5,458 code units |
| Most specialisable sites in one code object | 510 / 1,181 |
| Specialisable share of instructions | 32% |
| Most frequent pair | `LOAD_FAST_BORROW` then `LOAD_ATTR`, 4.6% of instructions |
| Function code objects run after nine stdlib imports and a small workload | 16% (18.6% of code units) |
| Instructions removed by the 4.3 rules, function bodies, static | 14% |

## 1. Encoding

```c
typedef uint64_t _PyIRWord;
/* word 0: [0..15] handler  [16..31] A  [32..47] B  [48..63] C
   word 1: [0..15] D        [16..31] E  [32..63] F (signed)   two-word formats only */
#define IR_BORROW   0x8000      /* on an input: the slot is a local, read borrowed */
#define IR_SLOT(x)  ((x) & 0x7FFF)
```

| Field role | Width | Meaning |
|---|---|---|
| slot | 15 + 1 bits | index into `localsplus`; locals, cells, free vars and temporaries share one numbering |
| fb | 16 bits, signed | feedback entry, in 8-byte units, relative to the instruction |
| jump | 16 (C) or 32 (B:C or F), signed | target in words, relative to the next instruction |
| imm | 16 or 32 bits | constant index, small int, count |

| IR instruction | Words | A | B | C | Word 1 |
|---|---|---|---|---|---|
| `LOAD_FAST_BORROW` (materialised) | 1 | dst | local | | |
| `LOAD_CONST`, `LOAD_SMALL_INT` | 1 | dst | imm32 in B:C | | |
| `STORE_FAST` | 1 | local | src | | |
| `LOAD_ATTR*`, `TO_BOOL*` | 1 | dst | src | fb | |
| `LOAD_GLOBAL*` | 1 | dst | | fb | |
| `STORE_ATTR*` | 1 | owner | value | fb | |
| `CALL*`, `CALL_KW*` | 1 | window base | argc | fb | |
| `POP_JUMP_IF_*` | 1 | cond | fb | jump16 | |
| `BINARY_OP*`, `COMPARE_OP*`, `CONTAINS_OP*`, `STORE_SUBSCR*` | 2 | dst | lhs | rhs | D = fb |
| `FOR_ITER*` | 2 | next | iter | fb | F = jump |
| compare fused with branch (step 6) | 2 | | lhs | rhs | D = fb, E = branch fb, F = jump |

Oparg immediates (name index, `NB_*` operator, comparison bits, argument and
unpack counts) move into the feedback entry's `arg` field, so `LOAD_ATTR` and
`CALL` fit one word. Fast paths never read `arg`.

Limits: 32,767 slots, ±256 KiB to the feedback entry, ±32,767 words for a
one-word branch. Measured maxima: 803 slots, 5,458 units. An `EXT` prefix supplies the high 16 bits of A, B and C, as
`EXTENDED_ARG` does for oparg (Python/bytecodes.c:6022). Dispatch extracts the
fields into 32-bit variables, as `NEXTOPARG` does (Python/ceval_macros.h:251),
so an absent prefix costs nothing. No stdlib or test code needs it; a generated
test function with 40,000 locals and 70,000 constants keeps it exercised. Jumps
target instruction starts. The exception table is not translated (4.4).

| | CPython bytecode | This IR | Ignition | Lua 5.4 |
|---|---|---|---|---|
| Unit | 16-bit unit plus caches | 64-bit word, 1 or 2 | byte stream | 32-bit word |
| Opcode | 8 bits | 16 bits | 8 bits | 7 bits |
| Operands | one 8-bit oparg | three 16-bit fields | 8-bit, scaled to 16 or 32 by prefix | A 8, B and C 8 plus k bit, Bx 17, sJ 25 |
| Implicit operand | the stack | none | accumulator | none |
| Mutable per-site data | inline caches | feedback entry, relative offset | feedback vector, slot-index operand | none |
| Constants | `LOAD_CONST` | `LOAD_CONST` into a temporary | `LdaConstant` | `LOADK`, K variants, RK via k bit |
| Calls | stack window | slot window at A | register-list operand | register window at R[A] |

Ignition encodes for size. An accumulator carries one input and the result, so
most bytecodes name a single register. Operands are one byte, widened to two or
four by `Wide` and `ExtraWide`, which select the second and third banks of a
3×256 handler table. Specialisable bytecodes name their feedback slot as an
operand. We take the feedback operand, the width prefix, and registers as frame
slots. We reject the accumulator: CPython instructions have up to three inputs
and two outputs, so it adds moves without saving fields, and tier 0 wants every
operand explicit.

Lua 5.4 packs a 7-bit opcode and 8-bit register fields into 32 bits, so a
function has at most 255 registers. Constants enter through `LOADK`, K-variant
opcodes (`ADDK`, `EQK`, `GETFIELD`) or the k bit; comparisons skip the following
`JMP`; calls use a register window. Lua keeps no feedback. We take fixed words,
call windows, and, in step 6, K and immediate variants for hot sites in place of
a general constant-operand mode. We cannot take 8-bit fields: CPython has no
local limit, and three slots plus a feedback reference do not fit 32 bits.

## 2. Dispatch

The handler index is the low 16 bits of word 0. Dispatch loads from
`opcode_targets` (computed goto, Python/ceval_macros.h:133) or
`instruction_funcptr_handler_table` (tail call, :104) and jumps. The tail-call
parameters (:75-78) lose `stack_pointer` and `oparg` and gain A, B and C. The
switch build still works because indices are dense.

We reject handler addresses in the word:

- specialize.c (`_PyOpcode_Deopt`, Python/specialize.c:381), instrumentation
  (`DE_INSTRUMENT`, `INSTRUMENTED_OPCODES`, Python/instrumentation.c:113, :135),
  `dis`, lltrace and tier 0 all need instruction identity; addresses would need
  a reverse map.
- An address takes at least 32 bits and would push three-field instructions to
  two words.
- JavaScriptCore's 2019 redesign replaced direct threading with an indexed
  table and measured no change. Its 10% interpreter slowdown came from
  out-of-line metadata reached through three loads (webkit.org/blog/9329).
- Tier 0 removes dispatch from hot code.

What we remove is secondary decoding: `EXTENDED_ARG` chains, cache skipping,
the original-opcode lookup in `INSTRUMENTED_LINE` (Python/bytecodes.c:5916-5946)
and the tracing-table swap (Python/ceval_macros.h:147-157, deleted with tier 2).

Quickening writes the handler field with a relaxed atomic store, as `set_opcode`
writes the opcode byte today, and keeps its compare-and-swap against
instrumentation in the free-threaded build (Python/specialize.c:326-345).

## 3. Feedback vector

```c
typedef enum { FB_COUNTER, FB_BRANCH, FB_UNARY, FB_BINARY,
               FB_ATTR, FB_GLOBAL, FB_CALL } _PyFbKind;
typedef enum { FB_COLD, FB_MONO, FB_POLY, FB_MEGA, FB_OFF } _PyFbState;

typedef struct {
    uint8_t  kind;                /* _PyFbKind, fixed at lowering */
    uint8_t  state;               /* _PyFbState */
    _Py_BackoffCounter counter;   /* pycore_backoff.h; warmup and cooldown as today */
    uint16_t misses;              /* guard failures since last specialisation, saturating */
    uint16_t hits;                /* FB_CALL only: specialised executions, saturating */
    uint32_t arg;                 /* static immediate; FB_BRANCH keeps its 16-bit history here */
    uint32_t poly;                /* 0, or index of a _PyFbPoly in the IR block */
} _PyFbHeader;                    /* 16 bytes */

typedef struct { _PyFbHeader h; uint32_t type_version, _pad; } _PyFbUnary;           /* 24 */
typedef struct { _PyFbHeader h; uint32_t lhs_version, rhs_version;
                 const _PyBinaryOpSpecializationDescr *descr; } _PyFbBinary;         /* 32 */
typedef struct { _PyFbHeader h; uint32_t type_version;
                 union { uint32_t keys_version, dict_version, func_version, version; };
                 union { uint32_t index, offset, hint, dictoffset; };
                 uint32_t _pad;
                 union { PyObject *descr, *fget, *getattribute; PyTypeObject *type; };
               } _PyFbAttr;                                                          /* 40 */
typedef struct { _PyFbHeader h; uint32_t globals_version, builtins_version;
                 uint32_t index, _pad; } _PyFbGlobal;                                /* 32 */
typedef struct { _PyFbHeader h; uint32_t func_version;   /* type version for CALL_ALLOC_AND_ENTER_INIT */
                 uint32_t callable_type_version; } _PyFbCall;                        /* 24 */

typedef struct {                  /* allocated on the first MONO -> POLY transition */
    uint32_t type_version[4], aux[4], index[4];
    uint16_t handler[4];          /* specialised handler that matches each entry */
    PyObject *descr[4];           /* borrowed; valid only while type_version matches */
} _PyFbPoly;
```

Union members carry the DSL cache-effect names (`type_version`,
`func_version`, `dict_version`, `index`, `offset`, `hint`, `descr`, ...), so the
generator maps a cache effect to `fb->name` by name. Versions and indexes widen
to 32 bits, which removes `LOAD_GLOBAL`'s `SPEC_FAIL_OUT_OF_RANGE` failures
(Python/specialize.c:1379-1391).

| Today (Include/internal/pycore_code.h) | Fields | Instructions | Kind |
|---|---|---|---|
| `_PyLoadGlobalCache` (:67) | counter, module and builtin keys versions (16-bit), index | `LOAD_GLOBAL` | GLOBAL |
| `_PyBinaryOpCache` (:76) | counter, descr pointer | `BINARY_OP`, including subscripts | BINARY |
| `_PyUnpackSequenceCache` (:83) | counter | `UNPACK_SEQUENCE` | UNARY |
| `_PyCompareOpCache` (:90) | counter | `COMPARE_OP` | BINARY |
| `_PySuperAttrCache` (:96) | counter | `LOAD_SUPER_ATTR` | ATTR |
| `_PyAttrCache` (:102) | counter, type version, index | `STORE_ATTR` | ATTR |
| `_PyLoadMethodCache` (:108) | counter, type version, keys version or dict offset, descr | `LOAD_ATTR` | ATTR |
| `_PyCallCache` (:124) | counter, function version | `CALL`, `CALL_KW` | CALL |
| `_PyStoreSubscrCache` (:132) | counter | `STORE_SUBSCR` | BINARY |
| `_PyForIterCache` (:138) | counter | `FOR_ITER` | UNARY |
| `_PyGetIterCache` (:144) | counter | `GET_ITER` | UNARY |
| `_PySendCache` (:150) | counter | `SEND` | UNARY |
| `_PyToBoolCache` (:156) | counter, type version | `TO_BOOL` | UNARY |
| `_PyContainsOpCache` (:163) | counter | `CONTAINS_OP` | BINARY |
| `_PyCallFunctionExCache` (:169) | counter | `CALL_FUNCTION_EX` | CALL |
| one unit, no struct (Python/specialize.c:83-97) | backoff counter | `RESUME`, `JUMP_BACKWARD` | COUNTER |
| one unit, no struct (Python/specialize.c:83-97) | branch history | `POP_JUMP_IF_*` | BRANCH |

Rules:

- Types and functions are recorded as versions. Tier 1 resolves them through
  `_PyType_LookupByVersion` (Include/internal/pycore_typeobject.h:154) and the
  function version cache (Objects/funcobject.c:311). `descr` and its aliases are
  the only pointers, valid while the version matches, as today.
- UNARY and BINARY entries record operand type versions, which today's
  counter-only caches discard. Only the generic path writes them.
- Polymorphism: up to four entries, V8's limit. In 1z the interpreter stays
  monomorphic and re-specialises to the newest type, as today. The slow path
  appends each new version to `_PyFbPoly`, sets POLY, and sets MEGA on the
  fifth. Tier 1 reads the table.
- Fast paths write the entry only for branch history (the 16-bit shift register
  of `RECORD_BRANCH_TAKEN`, Python/ceval_macros.h:393) and for call `hits`,
  which tier 1 needs for inlining. The deoptimising path writes `misses`.

**Placement.** One allocation per code object and IR form,
`[header][IR words][feedback][poly tables][pcmap]`, held by a new field `_co_ir`
that replaces `co_executors` (Include/cpython/code.h:104). `co_extra` (:113)
belongs to PEP 523 tools. The default build has one block per code object. The
free-threaded build has one per thread-local bytecode index, mirroring `co_tlbc`
(Include/internal/pycore_code.h:556-577) and `copy_code`
(Objects/codeobject.c:3358). The relative `fb` field needs no base register and
keeps entries near their instructions, which avoids the load chain that cost
JavaScriptCore.

**Reset.** Nothing is reset eagerly. Each fast path re-checks the version it
depends on (`_GUARD_TYPE_VERSION`, Python/bytecodes.c:2833; `_LOAD_ATTR_MODULE`,
:2889; `_CHECK_FUNCTION_VERSION`, :4443). A failed check falls into the generic
path, which counts a miss and backs off (cooldown 52,
Include/internal/pycore_code.h:463), as today. Three events reset an entry:
instrumenting an instruction warms its counter (Python/instrumentation.c:700-705,
unchanged); a site that runs out of versions goes to OFF; a new code object,
including the result of `code.replace()`, starts cold. Watcher events do not
touch feedback. They will invalidate tier-1 code, following
Python/optimizer_analysis.c:142-178.

## 4. Lowering from stack bytecode

### 4.1 When

| | In `_PyCode_New` (Objects/codeobject.c:712) | At first execution |
|---|---|---|
| Code that never runs | lowered anyway: 84% of loaded functions in the measured workload | never lowered |
| Import of a `.pyc` (Python/marshal.c:1928) | pays for every function | pays nothing |
| Hand-built bytecode from `code.replace()` | must lower, may fail | lowered only if run |
| Instrumentation state | unknown | known, so the right form is chosen |
| Free-threaded build | no locking | lowers under the code object's critical section |

Decision: lazy. `co->_co_ir` starts at a static one-word stub whose handler,
`LOWER`, lowers the code, publishes `_co_ir`, rebases `frame->instr_ptr` and
dispatches, in the manner of the trampoline at Python/ceval.c:1056.
`_PyFrame_Initialize` (Include/internal/pycore_interpframe.h:189-218) loads
`co->_co_ir->words` in place of `_PyCode_CODE(code)`. Malformed bytecode raises
`SystemError` at first call. The compiler is not a candidate: code objects also
come from marshal and the constructor (Objects/codeobject.c:908, :1000), and the
compiler's facts survive in the bytecode anyway.

### 4.2 Slots

The temporary at depth d lives in slot `nlocalsplus + d`, and dead temporaries
hold NULL. Frame push clears them (extending the loop at
Include/internal/pycore_interpframe.h:215) and handlers clear each temporary
they consume. Consequences:

- `PUSH_NULL` and the NULL outputs of `LOAD_GLOBAL` and method `LOAD_ATTR` cost
  nothing.
- GC traversal, clearing, copying and unwinding scan the whole temporary area
  (Python/frame.c:11, :100; pycore_interpframe.h:134; Python/bytecodes.c:6454)
  without a stack pointer.
- `frame->stackpointer` keeps one job: at a call, send or yield it holds the
  caller's result slot, so `_RETURN_VALUE` (Python/bytecodes.c:1574) and
  `_YIELD_VALUE` (:1852) push exactly as today.

About forty users of `stackpointer` (Python/frame.c, gc.c, gc_free_threading.c,
Objects/frameobject.c, object.c, Modules/_remote_debugging) need an audit.

### 4.3 Algorithm

1. Decode `co_code_adaptive`, which is no longer rewritten.
2. Mark block starts: jump targets, successors of terminators, handler targets,
   and every line-event offset (computed by `initialize_lines`,
   Python/instrumentation.c:1553-1675, factored out).
3. Compute block entry depths from `_PyOpcode_num_popped` and
   `_PyOpcode_num_pushed` (Tools/cases_generator/opcode_metadata_generator.py:98),
   as `calculate_stackdepth` does. A handler enters at the table depth plus
   lasti and the exception.
4. Walk each block with an abstract stack of TEMP and BORROWED(local) entries,
   applying the table below.
5. Record per IR word `start` (first bytecode unit consumed) and `anchor` (the
   unit of the instruction that does the work).
6. Allocate feedback entries for anchors with caches; fill `arg`.
7. Resolve jumps.

| Bytecode | Lowering |
|---|---|
| `LOAD_FAST_BORROW`, `LOAD_FAST_BORROW_LOAD_FAST_BORROW` | push BORROWED; emit nothing |
| `LOAD_FAST`, `LOAD_FAST_CHECK`, `LOAD_FAST_AND_CLEAR`, `LOAD_CONST`, `LOAD_SMALL_INT` | load into the canonical temporary |
| `COPY n` | BORROWED: push again; TEMP: emit DUP |
| `SWAP n` | both BORROWED: swap in the abstract stack; otherwise emit XCHG |
| `POP_TOP` | BORROWED: nothing; TEMP: emit CLOSE |
| `PUSH_NULL`, `NOP`, `NOT_TAKEN` | nothing; the instrumented branch twin reports `NOT_TAKEN`'s offset for BRANCH_LEFT |
| `STORE_FAST i` | materialise every BORROWED(i). If the previous IR instruction produced the top TEMP and both are on one line, set its A to i; else emit `STORE_FAST` |
| register-style X | BORROWED inputs become `IR_BORROW` operands. If X escapes or can raise (`HAS_ESCAPES_FLAG`, `HAS_ERROR_FLAG`), materialise BORROWED entries below its inputs |
| window-style X | materialise from the window base up, and below it if X escapes |
| block end | materialise everything |

Folding preserves semantics. Only compiler-proven borrows fold, so the local is
not stored before its consumer runs; nothing that escapes or raises sits between
them; and fusion never crosses a line start, so a trace function writing
`f_locals` sees what it sees today.

### 4.4 Depths and exceptions

No instruction has a dynamic depth. `UNPACK_SEQUENCE`, `UNPACK_EX`
(Python/bytecodes.c:2125), `BUILD_*` (:2473), `CALL*` and `CALL_FUNCTION_EX`
take oparg-sized arrays and become window-style. Four cases need care:
`FOR_ITER` and `SEND` have a different depth on the jump edge (step 3); handlers
take their depth from the table; a resumed generator receives the sent value in
the slot saved at the yield; `RERAISE` restores the location from lasti.

The IR keeps `co_exceptiontable`. The unwinder (Python/bytecodes.c:6454) looks
up `pcmap[pc].anchor` with `get_exception_handler` (Python/ceval.h:461), closes
and clears temporaries above the handler depth, writes lasti (still a bytecode
offset) and the exception into canonical slots, and finds the handler's IR pc by
binary search over `start`. Handlers are block starts, so the search always
hits. `RERAISE` maps lasti back the same way.

### 4.5 Line numbers and `f_lasti`

`_PyInterpreterFrame_LASTI` (Include/internal/pycore_interpframe.h:18) becomes
`pcmap[pc].anchor`. Its callers (Python/traceback.c, Objects/frameobject.c and
four others) do not change, nor do `f_lineno`, `tb_lineno`, caret positions and
`co_lines()`. LINE events fire at
block starts and report `start`. Assigning `frame.f_lineno`
(Objects/frameobject.c:1729) targets line starts, which are block starts, and
takes depths from `mark_stacks` as now.

One divergence remains. When `STORE_FAST` is folded and the overwritten value's
finaliser inspects the frame, `f_lasti` names the producer, not the store. Both
lie on one line, so `f_lineno` agrees. The pcmap costs 4 bytes per word, 8 above
65,535 code units.

## 5. Generator changes

The analyzer chooses a binding style from each instruction's stack effect.

- **Register style**: scalar inputs and outputs, at most three items plus
  feedback. An input binds as
  `(_PyStackRef){ .bits = localsplus[IR_SLOT(x)].bits | (x >> 15) }`. Bit 15
  lands on `Py_TAG_REFCNT`, so bodies are unchanged and closing a borrowed local
  does nothing. `Py_STACKREF_DEBUG` builds call `PyStackRef_Borrow` instead.
  Consumed temporaries are cleared. Outputs are stored by swap and `XCLOSE`.
- **Window style**: instructions with array items (`args[oparg]`,
  `values[oparg]`) or the `window` annotation. The handler sets
  `stack_pointer = &localsplus[A + popped]` and the existing code is emitted
  unchanged.

A `SlotStack` beside `Stack` (Tools/cases_generator/stack.py:218) emits slot
references where `Stack.pop` emits `stack_pointer[...]` (:242-279).
`ir_generator.py` reuses `Emitter` (generators_common.py:104) and replaces
`DEOPT_IF`, `EXIT_IF`, `INSTRUCTION_SIZE`, `JUMPBY`, `LOAD_IP` and cache
effects. `ir_metadata_generator.py` emits per handler: format, field roles,
feedback kind, flags, generic handler, instrumented twin, stack opcode for
`dis`, and the uop expansion now in `_PyOpcode_macro_expansion`
(opcode_metadata_generator.py:232), which tier 1 needs to emit one guard per
guard uop.

DSL changes:

- `window`, for frame transitions and instructions that reach below their
  inputs: the `CALL` family, `SEND`, `YIELD_VALUE`, `RETURN_GENERATOR`,
  `PUSH_EXC_INFO`, `WITH_EXCEPT_START`, `CLEANUP_THROW`, `END_ASYNC_FOR`,
  `RERAISE`.
- `no_fold`, for outputs that may alias a borrowed input. The analyzer detects
  `out = in` itself; the annotation covers the rest.
- `FB_COUNTER()` and `FB_BRANCH()` replace the 29 direct cache-unit references.
  `JUMP_TARGET()` and `RETURN_TO_CALLER()` replace instruction-pointer
  arithmetic such as `LOAD_IP(1 + INLINE_CACHE_ENTRIES_SEND)`. Both generators
  implement them, so bytecodes.c stays the single source.
- `_Py_Specialize_*` (17 call sites) take the instruction and its feedback entry.
- `tier2` ops, `_RECORD_*` ops and `replicate(N)` are deleted.

**Coexistence.** `--with-ir-interp` defines `Py_IR_INTERP` and selects
generated_ir_cases.c.h over generated_cases.c.h. Both come from bytecodes.c and
share specialize.c, which writes feedback entries in both builds from step 2.
Frame accessors (`_PyFrame_GetBytecode`, `_PyInterpreterFrame_LASTI`,
`_PyFrame_IsIncomplete`) get one implementation per mode. The test interpreter
Modules/_testinternalcapi/test_cases.c.h (Makefile.pre.in:2186) moves in step 7.

**Tier 0.** `ir_generator.py` will also emit each handler as a function
`(frame, tstate, A, B, C, fb) -> status`, which tier 0 calls for instructions it
does not inline, as Sparkplug calls Ignition's builtins.

## 6. Salvage and deletion

| Fate | Items |
|---|---|
| Keep and extend | Python/bytecodes.c; Python/specialize.c (the feedback writer); Tools/cases_generator/analyzer.py, parser.py, parsing.py, lexer.py, plexer.py, stack.py, cwriter.py, generators_common.py, tier1_generator.py, target_generator.py, opcode_metadata_generator.py, py_metadata_generator.py, opcode_id_generator.py, uop_id_generator.py; Include/internal/pycore_backoff.h |
| Keep for tiers 0 and 1 | Python/perf_trampoline.c, perf_jit_trampoline.c, jit_unwind.c, jit_publish.c; the type, dict and function watcher APIs; the type and function version caches |
| Delete in step 1 | Python/optimizer.c, optimizer_analysis.c, optimizer_bytecodes.c, optimizer_symbols.c, optimizer_cases.c.h, executor_cases.c.h, record_functions.c.h, jit.c; Tools/jit/; Include/internal/pycore_optimizer.h, pycore_optimizer_types.h, pycore_uop.h, pycore_uop_metadata.h, pycore_jit.h; Tools/cases_generator/tier2_generator.py, optimizer_generator.py, record_function_generator.py, uop_metadata_generator.py; `--enable-experimental-jit` (configure.ac:3183); `ENTER_EXECUTOR`, `RESUME_CHECK_JIT`, `JUMP_BACKWARD_JIT`, `JUMP_BACKWARD_NO_JIT`, `TRACE_RECORD`, `_JIT`; tier-2 macros in Python/ceval_macros.h (:147-165, :455 onward) and `_PyTier2Interpreter` (Python/ceval.c:1410); `co_executors`; Lib/test/test_capi/test_opt.py |
| Reference only, from git history at the deletion commit | the type rules in optimizer_bytecodes.c and the lattice in optimizer_symbols.c, for tier-1 speculation; the bloom-filter dependency tracking in optimizer.c, for tier-1 invalidation |

`sys._jit` stays as a stub that reports no JIT, so tests and tools still import
it.

## 7. Compatibility

| Surface | Result | Mechanism |
|---|---|---|
| `co_code` | unchanged | `co_code_adaptive` is never rewritten; `_PyCode_GetCode` (Objects/codeobject.c:2215) becomes a copy |
| `dis` | unchanged | reads `co_code` |
| `dis(adaptive=True)`, `show_caches` | same names; counters and versions read from feedback | `_co_code_adaptive` is synthesised: each anchor's handler maps to its stack opcode, and its entry is written into the legacy cache layout. Lib/test/test_dis.py:1320-1510 and Lib/test/test_opcache.py:2238 read these |
| `sys.monitoring`, `sys.settrace` | same events | below |
| `f_lasti`, `f_lineno`, tracebacks | unchanged except 4.5 | pcmap |
| Generators and coroutines | unchanged | `frame->instr_ptr` holds an IR pc; yield and resume offsets become IR sizes; `gen_close` (Objects/genobject.c:453-507) decodes the IR word |
| `__code__.replace()`, `types.CodeType` | unchanged | new object, lowered lazily |
| marshal, `.pyc`, magic number | unchanged | the IR is never marshalled |
| `co_extra`, PEP 523 | unchanged | |
| `_remote_debugging`, py-spy, austin | must change | they derive lines from `instr_ptr - co_code_adaptive` (Modules/_remote_debugging/frames.c:210); `_Py_DebugOffsets` (Include/internal/pycore_debug_offsets.h:128) gains `_co_ir`, the pcmap and its entry width |

Instrumentation is the hazard. `instrument()`, `instrument_line()` and
`instrument_per_instruction()` (Python/instrumentation.c:755-823) rewrite
opcodes in place and save the originals in `_co_monitoring` by bytecode offset.
Running frames point into the instruction stream, and a fused instruction
covers several offsets.

- `_co_monitoring` stays indexed by bytecode offset. The instrumenter finds the
  IR word whose `start` (LINE) or `anchor` (other events) matches, rewrites its
  handler field in place and saves the original in a side array. Frame pointers
  stay valid.
- Every line-event offset starts an IR instruction (4.3, step 2).
- CALL, RETURN, YIELD, RESUME, JUMP, BRANCH_LEFT, BRANCH_RIGHT and
  STOP_ITERATION anchor on the instruction that raises them. Fused instructions
  have instrumented twins that report the right offsets.
- INSTRUCTION events need one IR instruction per bytecode instruction. Enabling
  them lowers the code again in 1:1 form, kept beside the fused form. Frames on
  thread stacks are remapped in `instrument_all_executing_code_objects`
  (Python/instrumentation.c:1985), which stops the world. The remap is exact:
  frames stop only at escape points, where both forms hold the same values in
  the same slots. Suspended generators remap on resume, in `RESUME`'s version
  check (`_MAYBE_INSTRUMENT`, Python/bytecodes.c:180). A handler returning from
  an escaping call reloads its pc when the eval breaker reports a version
  change.

## 8. Risks and sequence

| Risk | Mitigation |
|---|---|
| Memory: IR, pcmap and feedback add about five times the bytecode size of executed code | Lazy lowering; 16-bit pcmap; RSS recorded in steps 0, 4 and 5 |
| Out-of-line feedback slows the interpreter, as at JavaScriptCore | Relative offsets in one block; step 2 measures this alone |
| Folding changes semantics | Fold only proven borrows; materialise before escapes, raises and line starts; differential runs of fused and 1:1 forms |
| Instrumentation remap bugs | In-place rewriting for every event except INSTRUCTION; both forms kept; monitoring tests in both forms |
| A stale temporary causes a use-after-free | Debug builds assert at each dispatch that temporaries above the static depth are NULL |
| External profilers break | `_remote_debugging` and debug offsets updated in step 4 |
| The two interpreters drift apart | One DSL source; CI builds both; step 7 deletes one |
| Free-threaded build | One block per thread-local bytecode index; free-threaded CI from step 2 |

Every step passes the full suite on the default, debug and free-threaded
builds, with pyperformance compared against `boil/bench/run-default.log`.

0. **Harness.** Scripts in `boil/bench/` for static code-object statistics
   (section 0), dynamic pair counts from a `--enable-pystats` build run through
   a model of the 4.3 rules, the executed fraction, code-object memory, and
   `perf stat` counters on five loop benchmarks. Test: they reproduce section 0
   on the baseline and emit JSON.
1. **Delete tier 2 and the JIT** (section 6). Test: pyperformance unchanged,
   since tier 2 is off by default.
2. **Typed feedback under the stack interpreter.** `_PyCode_Quicken` allocates
   entries; cache unit 0 holds the entry index; specialize.c writes typed
   fields; `_co_code_adaptive` is synthesised. Test: test_opcache, test_dis,
   test_monitoring; pyperformance within 2%, else move entries closer first.
3. **Generator backend.** IR files compile under `--with-ir-interp`; nothing
   runs them. Test: `make regen-cases` is reproducible;
   Lib/test/test_generated_cases.py covers both binding styles, `EXT` and
   feedback mapping.
4. **1:1 IR interpreter behind `--with-ir-interp`**: lazy lowering, slots,
   pcmap, unwinding, instrumentation, generators, frame accessors,
   `_remote_debugging`. Test: suite in both configurations; a debug check of
   pcmap invariants; the huge-function test; pyperformance within 3%.
5. **Folding** and the INSTRUCTION-event form switch. Test: a differential
   runner comparing results, exceptions, traceback lines and LINE events
   between forms; a speedup in line with the step 0 model.
6. **Fused compare-and-branch and constant variants**, chosen from step 0 pair
   counts. Test: test_opcache cases for each specialisation.
7. **IR only.** Flip the default; delete the stack interpreter, the flag and
   run-time use of inline caches; move the test interpreter.
8. **Out-of-line handlers and a call-threaded mode** that runs code by calling
   handler functions in sequence: tier 0 without an assembler. Test: the suite
   in that mode.

## 9. Open questions for the coordinator

1. Is the `f_lasti` divergence of 4.5 acceptable, or should `STORE_FAST`
   folding wait until a test shows nothing observes it?
2. What memory budget applies? The estimate is about five times the bytecode of
   executed code; less requires compressing the pcmap and feedback.
3. May step 1 remove `--enable-experimental-jit` now? Its baseline is in
   `boil/bench/run-jit.log`.
4. Is the free-threaded build in scope for 1z, or may it trail by a step?
5. Should instrumented code ever run in tier 0? If not, instrumented twins stay
   interpreter-only.

## 10. Coordinator decisions (2026-10-23)

1. **`f_lasti` during a folded `STORE_FAST`.** Acceptable. A finaliser that
   runs inside a fused store observes the producing instruction's offset.
   When line or instruction events are enabled the code runs in the 1:1 form,
   so debuggers and tracers see exact offsets.
2. **Memory budget.** About five times the bytecode size of executed code is
   accepted. Record RSS on the pyperformance set at steps 0, 4 and 5. If RSS
   grows more than 5 percent at step 4, narrow the 1:1 form before step 5.
3. **`--enable-experimental-jit`.** Remove it in step 1, after the baseline
   JIT numbers are recorded in `boil/bench/`.
4. **Free-threaded build.** In scope only for compiling and passing the test
   suite, with no performance work, as decided for the tagged-values note.
5. **Instrumented code in tier 0.** Never. Enabling instrumentation on a code
   object drops its tier-0 code and runs the interpreter in the 1:1 form.

Dispatch stays a table load by handler index rather than an address in the
word, as the note recommends; the plan artifact's wording is superseded.

## 11. Implementation decisions

Step 3 departs from sections 1 and 5 as follows. The coordinator approved
items 1 to 4 on 2026-10-24; the others settle points the note left open.

1. **`IR_BORROW` is bit 0 of a slot field**, and the slot index is the field
   shifted right by one (`IR_SLOT_FIELD`, `IR_SLOT_INDEX` in
   Include/internal/pycore_ir.h). Bit 0 is `Py_TAG_REFCNT`, so a handler ORs
   the field's low bit into the value without a shift. It does so only once it
   knows the value to be a pointer (`IR_BORROW_KNOWN`), because the bit means
   something else in an inline int or float. The limit stays 32,767 slots.
2. **Word 1 holds two 32-bit fields**: D in bits 0-31 is the oparg or jump
   distance, F in bits 32-63 the feedback distance, both in words. Field E is
   gone. `IR_EXT` widens A, B and C only.
3. **Step 4 keeps the stack pointer.** Temporary d stays canonical in slot
   `nlocalsplus + d`, so `stack_pointer` remains valid, window-style handlers
   need no setup, and outputs are written through `stack_pointer` as before.
   Register-style handlers read through their fields only the inputs that may
   be borrowed locals; debug builds assert that every slot field names the
   canonical slot of its input or output (`IR_CHECK_SLOT`, `IR_CHECK_INPUT`).
   Step 5 keeps the stack pointer too; see item 21.
4. **Absorbed loads keep their word.** The lowering replaces a
   `LOAD_FAST_BORROW` whose consumer reads the local through `IR_BORROW` with
   `IR_ABSORBED_LOAD_FAST_BORROW`, which pushes NULL. Depths, offsets, jump
   targets and instrumentation stay one word per instruction. Step 5 deletes
   these words; see item 20.
5. **Inline values in borrowed inputs.** An input used as an object that holds
   an inline int or float is boxed, and the box is written back to its slot,
   local or temporary, so that the slot owns it and the borrow stays valid.
   Guards deoptimise instead of boxing.
6. **Formats are computed per family.** A family's head, its specialisations,
   and its `INSTRUMENTED_` twin share one format. Inputs are aligned at the top of the stack and cover the
   member that reads deepest, since `INSTRUMENTED_END_FOR` reads two inputs
   that `END_FOR` leaves alone. Dst is the lowest first new output of any
   member. A format is window style if a member is annotated `window`, takes
   an array, or needs more than three slots; this covers `WITH_EXCEPT_START`,
   `CLEANUP_THROW` and `RERAISE` without annotations. A register-style member
   that switches frames and pushes outputs is an error. Fields go to slots,
   then feedback, then oparg; if they do not fit, the oparg moves to the
   entry's `arg` field (never for jumps), and otherwise to word 1.
7. **Borrowing is decided per input.** Input k may carry `IR_BORROW` only if
   every member first uses it as an object and no member leaves it on the
   stack. An `unused` output passes through the input at its position.
8. **Transparent handlers are found by structure**: no stack effect, oparg or
   feedback, and a `DISPATCH_GOTO`. These are `IR_EXT`, `INSTRUMENTED_LINE`
   and `INSTRUMENTED_INSTRUCTION`; the generator names no opcode.
9. **Handler indices.** Handlers annotated `ir_only` follow the 256 opcodes in
   name order. `IR_LOWER` is one of them.
10. **The configure flag in step 3.** `--with-ir-interp` compiles
    Python/ceval.c a second time with `Py_IR_INTERP` into Python/ceval_ir.o,
    which is not linked, so the IR interpreter keeps compiling. Step 4 makes
    the flag define `Py_IR_INTERP` instead.

Step 4 adds the following.

11. **Lowering** lives in Python/ir_lower.c. A code object starts at the
    shared two-word block `_PyIR_LowerStub`, whose `IR_LOWER` lowers it on the
    first call; word 1 of the stub marks a frame as past the first `RESUME`,
    for frames that never run. Stack depths come from the opcode metadata; a
    depth mismatch, underflow or unknown opcode raises SystemError.
12. **Fields are decoded in the handler** in the computed-goto build.
    Dispatch loads word 0 into `ir_word` and jumps; `IR_ENTRY` decodes A, B
    and C into locals, which the compiler drops where a handler does not read
    them. `IR_EXT`, the transparent handlers and `DISPATCH_SAME_OPARG` run
    another handler on the fields at hand: they pass them in `ir_fields` to
    its `IR_BODY_` label, after the decoding, and set `ir_word` to
    `IR_FIELDS_PASSED`, so that a family head's `PREDICTED_` label knows
    where to take the fields from. Decoding all three fields at dispatch cost
    six instructions per instruction, 27 percent on the micro loops. The
    tail-call and switch builds still decode at dispatch.
13. **Instrumentation shadows `co_code_adaptive`.** Instrumentation keeps
    rewriting the bytecode as in the stack build. After each rewrite
    `_PyIR_SyncInstrumentation` sets the word's handler to the instrumented
    opcode or, once instrumentation is removed, to the handler the lowering
    chose, which `real[]` holds per word. Sync warms the counters of
    specialisable instructions, so `warm_up` does nothing in the IR build.
14. **A line that starts on `EXTENDED_ARG` starts at its instruction**, since
    the IR has no word for `EXTENDED_ARG`; `initialize_lines` moves the start.
    Per-instruction events on `EXTENDED_ARG` units are not raised. `IR_EXT`
    does not save the instruction pointer, so an `INSTRUMENTED_LINE` on the
    next word sees the previous instruction in `frame->instr_ptr`.
15. **No feedback entry without a reader.** `RESUME` and `JUMP_BACKWARD` have
    counter caches that no IR handler reads, so they get no entry.
16. **Offsets.** `f_lasti` is the pcmap anchor of `frame->instr_ptr`. The
    word before a block is at offset -1, as the code unit before the bytecode
    is: the unwinding of a shim frame that has not started an instruction
    reads it.
17. **Free-threaded build.** There is no thread-local bytecode: threads share
    one block, which is not specialised, and handler rewrites are 64-bit
    atomic stores.
18. **The test interpreter** of `_testinternalcapi` is generated from the
    stack cases, so in the IR build it defers to `_PyEval_EvalFrameDefault`
    and test_allow_specialization_call is skipped. Step 7 moves it.
19. **Out-of-process readers.** The debug offsets gain `code_object.co_ir`
    and an `ir_block` section (words, nwords, flags and pcmap); a reader maps
    `instr_ptr` to its anchor and reads the opcode from `co_code_adaptive`.
    `_remote_debugging` and Tools/gdb/libpython.py do so.

Step 5 adds the following.

20. **Absorbed loads have no word.** The lowering deletes a
    `LOAD_FAST_BORROW` or `LOAD_FAST_BORROW_LOAD_FAST_BORROW` whose consumer
    reads its locals through `IR_BORROW` fields, so the consumer is the only
    dispatch. Only NOPs and other absorbed loads may lie between them, and no
    line, jump target, exception handler or edge of an exception range may
    start after the load. The consumer's pcmap start is the first absorbed
    load, so `_PyIR_InstrAt` maps the load's offset to the consumer, and the
    anchor stays the consumer's offset. Absorbed loads are 5.0 percent of the
    standard library's instructions; against step 4, words fall 4.7 percent
    and lowered bytes 2.6 percent.
21. **Handlers find the stack pointer through dst.** An absorbed load leaves
    its canonical slot alone, so `stack_pointer` lags behind at dispatch. A
    register-style handler with a borrowable input sets `stack_pointer =
    IR_STACK_POINTER(dst, dst_offset)` on entry. The assignment is
    idempotent, so entries through `PREDICTED_`, `IR_BODY_` and
    `DISPATCH_SAME_OPARG` stay correct. A format without a dst borrows
    nothing, so `STORE_ATTR*` and `STORE_SUBSCR*` no longer absorb their
    loads.
22. **Dead temporaries are not written back.** The canonical slot of an
    absorbed input holds whatever the frame left there, and is written only
    where code that reads the stack can run: once the input is known to be a
    pointer, a flush before an escaping call stores the borrowed reference,
    which is harmless to close; an exit before that, such as a failed box,
    clears the slot with `IR_CLEAR_IF_BORROWED`.
23. **Unwinding into a consumer.** When a frame that a consumer pushed raises
    (`LOAD_ATTR_PROPERTY`, `LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN`),
    `exit_unwind` looks up the code unit before the call, as the stack
    interpreter does. If the call's word starts with absorbed loads or an
    `EXTENDED_ARG`, that unit belongs to the word itself, not to the previous
    word, so `exit_unwind` looks up the word itself. Item 20 keeps the
    consumer in the range of its loads.
24. **Handlers read fields from memory.** In the default build a handler
    reads A, B and C with one 16-bit load each from its word, to which
    `next_instr` still points, instead of shifting them out of `ir_word`.
    Fields never change once a block is lowered. The free-threaded build and
    `IR_PREDICTED` decode `ir_word`. Profiling richards against the stack
    build found this the largest cost per instruction.
25. **Instrumentation stays on the bytecode shadow.** The lowering absorbs
    nothing while the code object has monitoring data or any tool has global
    events, so code lowered under a tracer runs one word per instruction. For
    code lowered before, `initialize_lines` moves a line start from an
    absorbed load to its consumer (`_PyIR_WordAnchor`), and no `INSTRUCTION`
    event is raised for an absorbed load: `f_trace_opcodes` misses those
    opcodes, which test_ir records as an expected failure. This falls short
    of decision 10.5; step 6 re-lowers such code one word per instruction.
26. **Lowering cost.** Line starts are marked only once a load could be
    absorbed, layout takes one pass when every field fits in 16 bits, and
    handler choice, absorption and feedback allocation share one loop. The
    lowering still costs about 300 instructions per bytecode instruction:
    about 66 million in a richards run, against 13 million for quickening in
    the stack build. That is all of richards' gap, since its loop costs less
    than the stack build's.
27. **Free-threaded hang, a step 4 bug.**
    `test_free_threading.test_monitoring` (SetPreTraceMultiThreaded,
    test_instrumentation) hangs in the free-threaded IR build and passes in
    63 ms in the free-threaded stack build. It must be fixed before the IR
    becomes the default, in its own commit after step 5.

Left for step 6, in order of measured cost:

- Lowering within twice the cost of quickening, in a single pass.
- `COMPARE_OP` fused with the following `POP_JUMP_IF`, with an int and float
  fast path guarded by feedback.
- Oparg folding: the specialised `LOAD_ATTR` handlers are still 7 to 8
  instructions longer than the stack build's, from three field loads against
  one oparg load, the `lea` for `stack_pointer`, the owner read through its
  field and the borrow OR.
- Re-lowering one word per instruction when opcode events are enabled,
  which removes the expected failure in test_ir.
- Absorption into formats without a dst (`STORE_ATTR*`, `STORE_SUBSCR*`), and
  borrowable inputs for `BINARY_OP`, `COMPARE_OP` and `TO_BOOL` with tagged
  values, where they take none today.
- Smaller items: the dependent load of `co->_co_ir->words` at frame push,
  `CALL_PY_EXACT_ARGS` (8 instructions over the stack build),
  `STORE_ATTR_INSTANCE_VALUE` and `TO_BOOL`.
