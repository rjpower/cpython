# Register IR v2 steps A3a and A3c: tail calls as the only backend (proposal)

Status: phase 1, revision 3, 2026-11-04. It folds in review 1
([register-ir-v2-a3-review-1.md](../reviews/register-ir-v2-a3-review-1.md))
and the user's review of revision 2; see the two change tables.

This is phase 1 of steps A3a and A3c of the
[register IR v2 packet](register-ir-v2-packet.md). After them the
interpreter has one backend: each handler is a C function, and dispatch is a
guaranteed tail call (`musttail`) through a constant table, on GCC 15.2 and
Clang 21. The fork's JIT is out of the build (user decision, runtime commit
`faddfd1454d`), so step A3b is dropped, and the interpreter's JIT state and
labels are deleted rather than converted.

All counts are at runtime HEAD `36aa4a0345a`
(`build/worktrees/pyvalue-rc-universal`). Against `faddfd1454d` it removes
upstream's JIT remnants and makes `sys._jit` report the fork's JIT. It
touches no handler, generator or `ceval.c` line, and the recount gives the
same rows at both. Counts come from the scripts in `build/pyjit-link-wave1/a3/`:

- `recount.py`: handlers, labels, exits, and address-taken locals visible in the source;
- `probes/tcobj.sh` with `tccanary.py`: canaries and frames in a tail-call
  `ceval.o` compiled out of tree on both compilers, with and without step 1's
  helper fixes;
- `scratch.py`, `kinds.py`, `weights.py` (entry shares, from
  `remeasure-20261104b-C-kernels-boxes.json`) and the other `probes/`.

The tree was not built. Where the packet and the code disagree, the code
wins (§0).

## Summary

| Item | Decision |
|---|---|
| 1. The fork's JIT | Out of the build, and its source deleted in its own commit (user decisions). Step 1 deletes the loop's JIT locals and labels, the `JIT_*` macros, the three warm handlers and the generated JIT helpers, and makes `sys._jit` constant `False`. A3b is dropped. §1.2 lists what the JIT stage rebuilds. |
| 2. Calling convention | `(frame, slots, tstate, next_instr, ir_word)`, plus `lastopcode` under `Py_STATS`, still in a register. `preserve_none` on Clang 21 (r12–r15, rdi), `no_callee_saved_registers` on GCC 15.2 (rdi, rsi, rdx, rcx, r8), plain `musttail` otherwise. `configure` checks `musttail` and records the attribute as `PY_TAIL_CALL_CC`, and measurement refuses the plain arm. GCC takes 0 to 11 more instructions than Clang in a handler that makes a C call. |
| 3. Activation record and G9 | The record is only the persistent half: previous link and entry frame, 128 B, linked from the thread state, walked by gdb. Handlers have no addressable locals by construction: values in and out of helpers, no cached transaction, memory work out of line, persistent state in slots, and a generator lint. The box area stays until R27 deletes it. The borrow floor stays in `_PyEval_EvalFrameDefault`. |
| 4. Handlers and labels | 298 functions (292 handlers, 5 labels, `UNKNOWN_OPCODE`), 295 after the JIT handlers go. Every exit is a macro or generator output. G9 outside the box area: 77 functions, 462 sites, 12.7% of entries. The generator clears 27 alone; 50 have one of 35 `bytecodes.c` lines for the fan-out. Two helper fixes in step 1 first remove 67 inlining canaries (41.8% of entries). |
| 5. Debug, stats, free threading | The debug frame assert exists and becomes a real check once `frame` is a parameter. Step 1 adds executing-copy and record-range asserts. `lastopcode` is the sixth argument, in a register on both compilers. `configure` rejects `--disable-gil`. |
| 6. Steps | Two landings. Step 1, mechanical: 6 agent-hours, 3 h wall. Step 2, G9: 9 agent-hours, 3 h. Measurement once, after step 2: 1.5 and 1.5. 16.5 agent-hours; about 6 h wall clock to the second landing with step 2 started when step 1 first compiles, 7.5 h with measurement. A3a's tail-call half lands with A3c; its G9 half lands second. |

## Changes after the user's review

| # | Decision or objection | Change | § |
|---|---|---|---|
| U1 | Q1 decided: delete the fork's JIT source; history keeps it | Step 1, in its own commit | 1.1, 6 |
| U2 | 30 agent-hours and 16.5 h are excessive. The plan keeps upstream's handler shape (every uop inlined, bridging and slow work inline) and patches the result | Replaced by handlers with no addressable locals by construction, as in LuaJIT and V8 (§3.5, rules R-a to R-d, and a generator lint). The 232 B scratch union, the renaming pass, the hot copies and `IR_SCRATCH` go. The record shrinks from 544 B to 128 B | 3 |
| U3 | Leave the box area: boxes in the core are R27 debt | The 96 handlers keep their C local box arrays. The canary check takes them from a list that may only shrink, tied to R27. The warning check ignores `value_owners` only | 3.3, 3.4 |
| U4 | Two landings in bulk style, one suite run each, one measurement | §6: scripts plus a lead and two workers, then a generator lead, four workers and a builder. GCC debug and Clang release suites run in parallel. About 6 h wall clock rather than 5; §6 says why | 6 |
| U5 | Recount G9 without the box area | §3.3: 77 functions, 462 sites, 12.7% of entries; 37 (11.0%) lose their canary | 3.3 |

## Changes after review 1

Verdict "accept with changes"; all 15 findings accepted, finding 7 with a qualification.

| # | Finding | Change | § |
|---|---|---|---|
| 1 | must-fix: the G9 recount misses addresses in inlined helpers | Reproduced on the object (`tcobj.sh`). Step 1 makes `finish_displaced_local` `Py_NO_INLINE` and `_PyValue_Publish` `Py_ALWAYS_INLINE`, and the object list becomes the work list | 3.4 |
| 2 | should-fix: GCC's cost with a hot-path C call | Shape `hotcall`: Clang 26, GCC 37; R1 and Q2 weighted by the hot-call handlers' 10% of entries | 2.2, 2.5, 9 |
| 3 | should-fix: base moved; `sys._jit` | Rebased on `36aa4a0345a`; `sysmodule.c` and `test_sys` in step 1 | 1.1, 6 |
| 4 | should-fix: gdb fails for nested activations | The record (`activation`) is walked from `eval_activation`; the entry asserts use it | 3.2, 9 |
| 5 | should-fix: stale C-stack claim | Restated against the computed-goto frame and HEAD's tail-call frames | 3.2 |
| 6 | should-fix: the plain arm builds silently; MSVC order | `PY_TAIL_CALL_CC`, refused when `none`; the MSVC arm first | 2.4 |
| 7 | should-fix: GCC's `-Wmaybe-musttail-local-addr` | Reproduced: 1,096 warnings in 115 functions. Qualified: `method_found` is upstream's pattern, scoped to one call, not a use-after-return. Used as a check that ignores only `value_owners` | 3.4 |
| 8 | should-fix: estimates omit known items | All are in §6 | 6 |
| 9–15 | notes | Sampler counts `_TAIL_CALL_*` self time; record-range assert; remedies stated; compiler and convention recorded; source deletion in its own commit; `--disable-gil` rejected; remote debugging unchanged | 3, 5, 7 |

## 0. Where the packet and the code disagree

| Item | Packet | Code at `36aa4a0345a` |
|---|---|---|
| Box area | 37 boxes (296 B); capacities 2–37 | **20 boxes** (160 B). 96 handlers have a value record, with capacities 3–20 (`CALL_LEN` 20, `CALL_METHOD_DESCRIPTOR_O` 18) |
| Handlers that pass a local to an out-of-line function | 75 and one label | **76 and one label** (`exception_unwind`) in the source. With the 96 value records, 133 functions |
| Functions with a canary in the tail-call object | 200 (`tcvt`, packet:1120) | **200** (Clang 200, GCC 197): the 133, plus 67: 66 that inline `finish_displaced_local`, and `LOAD_GLOBAL_BUILTIN`. 56.7% of entries (§3.4) |
| `callable` (12 handlers) | in the scratch | **no handler** takes `&callable` |
| Record size | about 850 B | **128 B**: the previous link and the entry frame only (§3.2) |
| `Py_STATS` `lastopcode` | seventh argument, on the stack under GCC | **sixth**: r9 under GCC, rsi under Clang. It moves to the stack only if a later step takes GCC's sixth register (A4) |
| Debug frame check | added by A3a | **exists**: `PRE_DISPATCH_GOTO` asserts `frame == tstate->current_frame` (`ceval_macros.h:247`). It is vacuous while each handler reloads `frame` from the thread state |
| Frame load per handler | `TAIL_CALL_LOAD_FRAME` | handlers get it from the generator (`ir_generator.py:2783`); labels and `UNKNOWN_OPCODE` from `TAIL_CALL_LOAD_FRAME` (`ceval_macros.h:119`) |
| Plain `return` out of the loop | – | 3 sites in 2 functions: `INTERPRETER_EXIT` (2), `exit_unwind_notrace` (1). The recount's "5 / 4" counts two string literals |
| Attribute handlers that pass `attr`, `old_value` or `receiver` to `retire_published_values` | nine | **ten**: five `LOAD_ATTR_*` and five `STORE_ATTR_*` (`STORE_ATTR_SHAPE_ADD` included), 7.8% of entries |
| Weighted lower bound per word | 48.9 / 12.1 at `48bb4d6d1d3` | **40.7 / 7.6** at `5de7bd62ef9` (59% set; remeasure-20261104b). Upstream 22.0 / 1.3 |
| Hardening in the Clang link | – | Ubuntu Clang 21 enables neither `-fstack-protector-strong` nor `-fcf-protection`; Ubuntu GCC 15.2 enables both (`probes/defaults.sh`). The review's `tccl.sh` passed neither, so its 0.877× ran with no canaries and no `endbr64` |

## 1. The fork's JIT (item 1, decided)

### 1.1 What A3 deletes

`_PyJIT_BASELINE_SUPPORTED` is 0 in every build at HEAD. The loop's JIT code
has its own guard,
`!_Py_TAIL_CALL_INTERP && !Py_STATS && !Py_GIL_DISABLED`, so the
computed-goto release build still compiles it all. Step 1 deletes it.

| Item | Where at HEAD | Size |
|---|---|---|
| Loop-local JIT state: `jit_request`, `jit_context`, `jit_result`, `jit_from_return`, `jit_flow`, `jit_cold_request`; the `_PyJit_TestClaim` call | `ceval.c:2260–2267`, `:2306–2310` | 13 lines |
| Labels `jit_resume`, `jit_enter`, `jit_step`, `jit_consume_action` | `ceval.c:2339–2474` | 136 lines |
| `jit_helpers_have_monitoring`, `_PyJit_ObserversAllow`; `JIT_RETIRE_COLD` and `_PyJit_FreeFlow` at `early_exit` | `ceval.c:1500–1546`, `:2486`, `:2491–2493` | 46 lines |
| `JIT_*` macros | `ceval_macros.h:843–896` | 54 lines |
| Macro sites in handler bodies | `bytecodes.c:1481, 1484, 1507, 5319, 5628, 5689, 5697, 5732`; the `CALL_JIT_WARM` test at `:4188` | 9 sites |
| Generator emission: `JIT_PREPARE_CALL`, `JIT_MAYBE_RESUME`, `JIT_SITE_ENTRY`, the JIT-helper mode, `skipped_part` | `ir_generator.py:2761–2776`, `:2884–2893`, `:2935–2938`, `:2706–2710` | – |
| Handlers `CALL_JIT_WARM`, `JUMP_BACKWARD_JIT_WARM`, `RESUME_JIT` (302–304, the last three, so no other handler is renumbered) and their writers | `specialize.c:2229–2240`; `_PyJit_SeedBackedges` at `ir.c:275` and `instrumentation.c:1986`; `codeobject.c:465`; `pycore_ir_metadata.h`, `pycore_feedback.h` | 3 handlers |
| Generated helpers and their generator | `generated_jit_helpers.c.h` (5,997 lines), `jit_helpers_generator.py` (216), `Makefile.pre.in:2206–2208`, `#include` at `ceval.c:2228` | – |
| The fork's JIT objects | `Makefile.pre.in:491–496`: `jit_runtime`, `jit_baseline`, `jit_backend_x86`, `jit_record`, `jit_ir`, `jit_regalloc` (2,616 lines), with `jit_x86.h` and `pycore_jit_ir.h`; the `exec->jit` hooks `_PyJit_DetachExec`, `_PyJit_ClearExec`, `_PyJit_ExecSizeOf` (`instrumentation.c:1872`, `ir.c:342`, `:357`); `_testinternalcapi`'s JIT entry points (`JIT_BASELINE_SUPPORTED`, `jit_method_natural` and the rest). None has a `PCbuild` entry | – |
| `sys._jit`: `is_enabled()` calls `_PyJit_NaturalAdmissionEnabled` (in `jit_runtime.c`). It becomes constant `False`, as `is_available()` is, and `test_sys.py`'s `test_available` and `test_enabled`, which use `_testinternalcapi`'s JIT entry points, assert `False` | `sysmodule.c:25`, `:4250`, `:4263`; `test_sys.py` | 2 functions, 2 tests |
| Tests | `test_jit_method.py` (37 tests, all skipped at HEAD); `TestJitHelpers` in `test_generated_cases.py` (20 tests); the JIT-helper half of `test_ir_contract`'s generated fixture | 57+ tests |

### 1.2 What the JIT stage must restore

| What | At `36aa4a0345a` | Restore as, on the final IR |
|---|---|---|
| Entry at back edges and calls | three warm handlers, which jump to loop labels through `JIT_SITE_ENTRY` | contract-table rows. A handler calls a C-ABI entry and dispatches on the action it returns. Compiled code never uses the handler convention, which both compilers document as unstable |
| Per-activation JIT state | six loop locals | fields of `_PyEvalActivation`, which persist across handlers |
| Frame-exit hooks | `JIT_LEAVE_FRAME` (3), `JIT_RETIRE_PENDING`, `JIT_FREE_FLOW(_RESULT)` (3), `JIT_MAYBE_RESUME` (3), `JIT_RETIRE_COLD` | calls at the same sites |
| Semantic helpers | `generated_jit_helpers.c.h` | regenerated from the contract table |
| Seeding and admission | `_PyJit_SeedBackedges`, the `CALL_JIT_WARM` rewrite in `specialize.c` | the same |
| Executing-copy re-checks | `jit_runtime.c:358–403`, `jit_record.c:84` | not restored: A2's invariant makes them dead |
| Tests and measurements | `test_jit_method` (method helpers, native and natural methods, observed loops with 9b's int51 import checks), `TestJitHelpers`, the M0 fixture (`_PyJit_TestCall`) | rewritten against the final IR |

Until then, no JIT test runs. That is already true at HEAD, where they skip,
and nothing in the interpreter's suite depends on them.

## 2. The calling convention (item 2)

### 2.1 Signature and pinned state

```c
#define TAIL_CALL_PARAMS _PyInterpreterFrame *frame, PyValue *slots, \
    PyThreadState *tstate, _PyInstr *next_instr, _PyIRWord ir_word  /* , int lastopcode */
#define TARGET(op) Py_NO_INLINE PyObject *Py_PRESERVE_NONE_CC _TAIL_CALL_##op(TAIL_CALL_PARAMS)
#define DISPATCH_GOTO() \
    Py_MUSTTAIL return instruction_funcptr_handler_table[opcode](TAIL_CALL_ARGS)
```

| # | Parameter | Clang 21 `preserve_none` | GCC 15.2 `no_callee_saved_registers` | Why |
|---|---|---|---|---|
| 1 | `frame` | r12 | rdi | 289 of 292 handlers store `instr_ptr` through it. Today each handler loads it from `tstate->current_frame`: one dependent load per word. Handlers that switch frames assign `frame` before they dispatch, and the new value passes on |
| 2 | `slots` | r13 | rsi | every slot access |
| 3 | `tstate` | r14 | rdx | eval breaker, record, helpers |
| 4 | `next_instr` | r15 | rcx | fetch. A2 passes it by value |
| 5 | `ir_word` | rdi | r8 | decoded after the jump. It is dead in most handlers once decoded |
| 6 | `lastopcode` (`Py_STATS` only) | rsi | r9 | pair statistics |

- **Return.** `PyObject *` in rax. A plain `return` (`INTERPRETER_EXIT`,
  `exit_unwind_notrace`) goes straight back to `_PyEval_EvalFrameLoop`,
  because every handler in between has been replaced.
- **Clang's first four registers survive C calls.** r12–r15 are callee-saved
  in the C ABI, so the state crosses a call into ordinary C with no moves.
  GCC's are the SysV argument registers, which C clobbers (§2.2).
- **Not parameters:**
  - The handler table is a static constant. Upstream passes it because it
    switches tables for tracing; the fork switches none.
  - The activation record: only gdb and the debug asserts read it (§3.2).
  - GCC's sixth register is left to the G2 decision (A4's `nlocalsplus`) or
    to a predecoded field (open question Q4).

### 2.2 What the attributes give: probes

`probes/handler.c` has eleven handler shapes, nine of them shown here.
`probes/handler.sh` compiles them with the release flags, `-O3
-fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -fno-strict-overflow`
(and `-fno-gcse` under GCC), plus `-fstack-protector-strong
-fcf-protection=full` on both compilers. Each cell
is the number of instructions from entry to the dispatch `jmp *` on the hot
path. The letters show what else is on that path: p is a push, s a stack
adjustment, c the canary.

| Shape | What it models | Clang `preserve_none` | Clang plain `musttail` | GCC `no_callee_saved_registers` | GCC plain `musttail` |
|---|---|---:|---:|---:|---:|
| `copy` | slot to slot, dispatch | 17 p | 17 p | 14 | 14 |
| `call` | a cold C call with a value live across it | 21 ps | 31 p×7 | 28 ps | 28 ps |
| `close` | release a value; `dealloc` is the only C call (`POP_TOP`) | 24 p | 40 p×5 | 23 ps | 23 ps |
| `addr` | two locals passed by address on a cold path (`LOAD_ATTR_SHAPE` at HEAD) | 31 psc | 41 p×6 sc | 36 psc | 36 psc |
| `record` | the same two in a record reached from `tstate` | 23 ps | 33 p×7 | 28 ps | 33 pps |
| `outlined` | the same cold path in a `noinline` function that takes values | 21 p | 33 p×7 | 28 ps | 28 ps |
| `boxes` | a 20-box array, as the 96 record handlers declare today | 31 psc | 41 p×6 sc | 35 psc | 35 psc |
| `tailslow` | the cold path in a tail-called function of the same signature | 19 p | 19 p | 18 | 18 |
| `hotcall` | a C call on the hot path whose result is stored (generic `BINARY_OP`, `CALL_BUILTIN_*`, `LOAD_ATTR`; review 1) | 26 p | 46 p×7 | 37 ps | 48 p×6 s |

What the table shows:

- **Smallest handler:** GCC 14, Clang 17. Clang keeps
  `push %rbp; mov %rsp,%rbp; pop %rbp` under `-mno-omit-leaf-frame-pointer`.
  GCC omits the frame pointer in a function whose only call is the tail call.
- **Any C call costs GCC on the hot path.** GCC moves the state that is live
  after the call into callee-saved registers and back: four moves in and
  three out in `call`; five in, three out and a spill in `hotcall`. Clang's
  state is already in r12–r15. GCC runs 1 shorter to 7 longer with a cold
  call, and 11 longer with a hot one. Handlers with a hot-path call take
  about 10% of entries (`CALL_BUILTIN_*`, `LOAD_ATTR`, `FOR_ITER`,
  `CALL_ISINSTANCE`, `BINARY_OP_SUBSCR_DICT` and others).
- **An address-taken local costs about 8 instructions on every entry**, even
  when the address is taken only on a cold path: a frame and a canary
  (`addr` 31/36 against `record` 23/28). The box array costs the same.
- **A tail-called slow path is the shortest form** (`tailslow`, 19 and 18;
  Q5).
- `Py_STATS` barely changes GCC's hot paths (`hotcall` 39). `musttail` holds
  at `-O0` and `-Og` on both compilers (`probes/optlevels.sh`).

### 2.3 One handler entry and dispatch on each compiler

`copy` (`slots[B] = slots[A]`), from `objdump -d`:

```text
Clang 21, preserve_none                    GCC 15.2, no_callee_saved_registers
endbr64                                    endbr64
push   %rbp                                mov    %r8,%rax           ; ir_word
mov    %rsp,%rbp                           shr    $0x10,%r8
mov    %r15,(%r12)        ; instr_ptr      mov    %rcx,(%rdi)        ; instr_ptr
mov    %rdi,%rax          ; ir_word        add    $0x8,%rcx          ; next_instr
shr    $0x1c,%rax                          and    $0xfff,%r8d
and    $0xfff,%eax                         shr    $0x1c,%rax
shr    $0xd,%edi                           mov    (%rsi,%r8,8),%r8   ; slots[A]
and    $0x7ff8,%edi                        and    $0xfff,%eax
mov    0x0(%r13,%rdi,1),%rcx  ; slots[A]   mov    %r8,(%rsi,%rax,8)  ; slots[B]
mov    %rcx,0x0(%r13,%rax,8)  ; slots[B]   mov    (%rcx),%r8         ; fetch
mov    0x8(%r15),%rdi     ; fetch          lea    table(%rip),%rax
add    $0x8,%r15                           movzwl %r8w,%r9d
movzwl %di,%eax           ; handler        jmp    *(%rax,%r9,8)
mov    table@GOTPCREL(%rip),%rcx
pop    %rbp
jmp    *(%rcx,%rax,8)
```

In the tree the table is `static`, so Clang also uses a `lea`.

### 2.4 The conventions in the build

```c
#if defined(_MSC_VER) && !defined(__clang__)        /* upstream's arm, first */
#  define Py_PRESERVE_NONE_CC __preserve_none
#elif _Py__has_attribute(preserve_none)             /* Clang; GCC when it ships it */
#  define Py_PRESERVE_NONE_CC __attribute__((preserve_none))
#elif _Py__has_attribute(no_callee_saved_registers) /* GCC on x86 */
#  define Py_PRESERVE_NONE_CC __attribute__((no_callee_saved_registers))
#else                                               /* plain musttail */
#  define Py_PRESERVE_NONE_CC
#endif
```

- **One source.** Only the attribute differs, so the three arms add no code
  path. The packet's "three conventions in `configure`" become one
  `configure` check. A two-function `musttail` program must compile, or
  `configure` stops. The check then compiles the same test under each
  attribute, prints the one the header will pick, and records it as
  `PY_TAIL_CALL_CC` (`preserve_none`, `no_callee_saved_registers` or `none`)
  in `sysconfig`. The `#error` at `ceval_macros.h:100–102` goes.
- **Two qualified conventions:** GCC 15.2 with `no_callee_saved_registers`,
  and Clang 21 with `preserve_none`. The plain arm is not qualified: it costs
  10 to 20 instructions in a handler with a C call (§2.5). The measurement
  harness refuses a build whose `PY_TAIL_CALL_CC` is `none`.
- **GCC's `preserve_none`** (r12–r15, rdi, rsi; "subject to change", PR
  119628) is not in GCC 15.2 (`probes/attrs.c`). When it ships, the first
  arm picks it up with no source change.
- **The MSVC arm** (`[[msvc::musttail]]`, `__preserve_none`;
  `ceval_macros.h:107–110`) comes first, because `_Py__has_attribute` is 0
  on MSVC. It is untested: the project has no Windows configuration.

### 2.5 What a build without `preserve_none` costs

| Compiler | Smallest handler | Handler with a cold C call | Why |
|---|---|---|---|
| GCC 15.2, attribute dropped | unchanged (14) | cold call: unchanged on most shapes, `record` +5; hot call: +11 (48, 6 pushes) | GCC shrink-wraps the saves into the cold path, which a hot call does not allow |
| Clang 21, attribute dropped | unchanged (17) | +10 (`call`) to +20 (`hotcall`): 5–7 pushes and pops on the hot path | it must save the callee-saved registers it uses to carry state across the call |
| GCC 15.2 with `no_callee_saved_registers`, against Clang `preserve_none` | −3 | −1 to +7 cold; +11 hot | GCC's argument registers do not survive calls (§2.2) |

### 2.6 Flags

- **Hardening.** Ubuntu GCC enables `-fstack-protector-strong` and
  `-fcf-protection` by default, and Ubuntu Clang does not. A3 builds Clang
  with both in `CFLAGS`, so the two compilers are compared with the same
  hardening. The build recipe records this.
- **Frame pointers stay**, as `configure`'s default. They cost Clang three
  instructions in every handler (open question Q6).
- **`CFLAGS_CEVAL`.** `-fno-gcse` (GCC) and `-finline-max-stacksize=512`
  are added only with computed gotos (`configure.ac:8115–8128`), so they
  fall away in step 1. The single measurement records text size without them.

## 3. The activation record and G9 (item 3)

### 3.1 Per-activation state at HEAD, recounted

Sizes are the same on both compilers (`probes/sizes.sh`).

| State | Bytes | Where at HEAD | Lifetime | After A3 |
|---|---:|---|---|---|
| Entry frame `_PyEntryFrame` | 120 | local of `_PyEval_EvalFrameLoop` (`ceval.c:2255`), linked as `frame->previous` | activation | record |
| Previous link | 8 | – | activation | record |
| JIT locals | `_PyJitContext` 136, 5 words; `_PyJitFlow` 1,560 on the heap | loop locals, computed goto only | activation | deleted (step 1) |
| Borrow floor: `outer_floor`, `outer_request`, `floor` | 20 | locals of `_PyEval_EvalFrameDefault` (`ceval.c:2517–2524`) | activation | **stay**. No handler reads them |
| Box area: `value_boxes[N]`, `value_owners` | ≤ 160 + 24 | computed goto: one shared array. Tail calls: a C local array in each of 96 handlers | one handler | **stays** in the 96 handlers, as R27 debt (§3.4) |
| Escaping handler locals | ≤ 232 (`CALL_BUILTIN_O`); median 80 | C locals of 76 handlers and 1 label | one handler | **none**: values pass by value, persistent state goes to slots, and work that needs memory runs out of line (§3.5) |

### 3.2 The record: the persistent half only

```c
typedef struct _PyEvalActivation {
    struct _PyEvalActivation *previous;   /* restored when this activation exits */
    _PyEntryFrame entry;                   /* persists across handlers */
} _PyEvalActivation;                       /* 128 B */
```

- `_PyEval_EvalFrameLoop` declares it as the local `activation` and links it
  from `_PyThreadStateImpl.eval_activation`, beside `borrow`. It restores
  the previous link on both exits (the return from the handler chain, and
  `early_exit`).
- **Users.** gdb walks the chain (§9, R4): Python frame k is
  `act[k−1].entry.frame.previous`, and the innermost is
  `tstate->current_frame`. The two entry asserts become
  `frame == &act->entry.frame`. No handler keeps per-handler state in it.
- **L1.** `entry.stack[0]` receives the returned or yielded value, owned;
  the rest is a frame header and a pointer. The record holds no borrow.
- **C stack per nested evaluation.** The record is the entry frame plus
  8 B. HEAD's tail-call handler frames are at most 320 B (GCC) and 304 B
  (Clang), medians 96 B and 64 B (`tcobj.sh`), and they shrink where G9's
  escapes go. Against the computed-goto loop frame, 3,304 B at
  `48bb4d6d1d3`, a nested activation costs about 3 KB less. Step 2 records
  `TestRecursion`'s depth before and after.
- **Stack switching.** Greenlet and its kin save and restore
  `eval_activation` with `current_frame` and the borrow stack (ports notes).
  Debug builds assert at each loop entry that the record lies between the
  stack pointer and `previous`, in the stack's growth order.
- Review 2's request for one structure that also holds the floor (R2-2) is
  declined: the floor is three locals of a function that is not a handler,
  and merging the two eval functions would move gdb's anchor (Q7).

### 3.3 Where G9 fails at HEAD, under the new rule

The rule: **no handler takes the address of one of its own locals, except
the box area.** The box area (`value_owners`, 96 functions) is R27 debt.
The value-API waves delete boxes from the core
([value-api.md](../value-api.md) R27, [value-api-v2a0.md](value-api-v2a0.md)),
so A3 leaves it alone. Moving it now would be work that R27 then throws away.

`kinds3.py` over `recount.py`'s rows at `36aa4a0345a`, with the box area
excluded. Each site is classed by who removes it: **the generator**, when
the callee is emitted by `ir_generator.py`, or **the fan-out**, when the
address is taken in a `bytecodes.c` uop body.

| Kind | Generator: functions / sites | Fan-out: functions / sites | All: functions, entries |
|---|---:|---:|---|
| Transaction struct (`operation`, `call`; `scope`, `publication` in `CALL_BUILTIN_O`) | 53 / 146 | 1 / 8 | 54, 3.9% |
| Value or int to a static helper (`pending`, `held_*`, `receiver`, `attr`, `old_value`, `invoked`, …) | 42 / 173 | 36 / 87 | 45, 10.5% |
| Out-pointer written by a helper (`call_child`) | 15 / 26 | – | 15, 1.0% |
| Runtime API out-parameter (`err`, `item`, `null_or_index`, `temp`, `dupkey`, `match_o`, `rest_o`, `index_or_null`, `v`, `retval_o`; `exc_slot`, `handler`, `lasti`) | – | 14 / 21 | 14, 1.9% |
| Pointer stored for later use (`method_found`) | – | 1 / 1 | 1, 0.0% |
| **Any of these** | **27 functions need only the generator** | **50 functions have a source site** | **77 functions, 462 sites, 12.7%** |

- **Overlap with the box area.** 40 of the 77 (1.6% of entries) also have a
  box area, so they keep a canary after G9. The other 37 (11.0%) lose it.
  The largest are `LOAD_ATTR_SHAPE` 4.49%, `LOAD_ATTR` 0.99%,
  `CALL_BUILTIN_O` 0.92%, `FOR_ITER` 0.76%, `LOAD_ATTR_INSTANCE_VALUE`
  0.67% and `LOAD_ATTR_SLOT` 0.64%.
- **Source lines.** The fan-out's 87 sites in 50 functions come from the 35
  `bytecodes.c` lines. R-b also touches the 9 `bytecodes.c` lines that
  declare `operation`. 54 of them are one line,
  `value_operation_close3(&held_left, &held_right, …)`, in the generic
  `BINARY_OP` uop that 27 handlers share.
- **On the object.** After step 1's helper fixes, the canary list is 133
  functions (14.9%), the source set exactly (§3.4). After step 2 it should
  be the 96 box-area functions (3.9%). That list may only shrink, and R27's
  waves shrink it.

### 3.4 Step 1's two helper fixes

`tcobj.sh` compiles HEAD's `ceval.c` as the tail-call build on both
compilers with `-fstack-protector-strong`, so a canary marks a local array
or an escaping address after inlining.

| Set | Functions | Entries | Cause |
|---|---:|---:|---|
| HEAD | 200 (Clang 200, GCC 197) | 56.7% | the 133 of §3.3 (the 77 and the box area), and 67 more |
| the 67 | 67 | 41.8% | 66 inline `finish_displaced_local` (`ceval.c:2128–2137`), which passes `&displaced` to `finish_value_bridges_ex`. `LOAD_GLOBAL_BUILTIN` calls `_PyValue_Publish(&V)` out of line (Clang here, GCC in review 1). The largest: `LOAD_FAST_BORROW` 13.8%, `PUSH_NULL` 3.0%, `LOAD_SMALL_INT_INLINE` 2.9%, `LOAD_COMMON_CONSTANT` 2.7% |
| after the fixes (`FIX=1`) | 133 (Clang 133, GCC 131) | 14.9% | the source-visible set |

The fixes are one line each: `finish_displaced_local` becomes `Py_NO_INLINE`
(the address is then taken in its own frame, on the cold path), and
`_PyValue_Publish` becomes `Py_ALWAYS_INLINE`.

GCC's `-Wmaybe-musttail-local-addr` names source locals: 1,096 warnings in
115 functions, 955 of them for `value_owners`, then `call` (96), `err`
(28), `pending_self` (8) and six more. Because the box area stays,
`-Werror` cannot be used. The check script instead fails on any warning for
a local other than `value_owners`.

### 3.5 The rewrite: no addressable locals by construction

Handlers keep only register-sized locals. Four rules, after LuaJIT and V8,
where a handler holds its state in registers and calls out for anything
that needs memory:

| Rule | Applies to | How |
|---|---|---|
| **R-a. Values in, values out.** A helper takes `PyValue` and `int` arguments by value and returns a value, or a two-word struct in rax:rdx (`{status, value}`, `{value, value}`, `{status, child}`). A null result means an error where a third word would not fit | `retire_published_values`, `value_reader_complete`, `value_attribute_complete`, `value_operation_close3`, `value_load_attribute`, `value_store_attribute`, `drop_failed_value`; `call_transaction_invoke`'s `call_child` | generator (its emitted calls); fan-out (calls in uop bodies) |
| **R-b. No cached transaction.** `_PyValueOperation` caches `tstate`, `frame` and the code object, which are parameters or one load away, plus two word offsets (which survive a change of executing copy) and three slot numbers. The handler keeps the ints in registers, the helpers take them and `(tstate, frame, next_instr)` by value, and the struct goes | `operation` in 53 functions | generator |
| **R-c. Memory work out of line.** Work that needs addressable state across several calls runs in one `Py_NO_INLINE` function with its own C frame, called from the handler. `_PyCallTransaction` has mutable fields (`effects`, `monitored`, `prepared`) that live across five emitted helper calls, so the generator emits one out-of-line `call_transaction_run` per call kind, returning `{status, child}` | `call` (call handlers); `scope` and `publication` (`CALL_BUILTIN_O`); runtime API out-parameters (14 functions), by small wrappers returning two words | generator (`call`); fan-out (the rest) |
| **R-d. State that outlives a call lives in a slot.** As `FOR_ITER`, `INSTRUMENTED_FOR_ITER` and `SEND` already do for `null_or_index` (`&IR_SLOT(F)`), with A5b's parameter types (`const PyValue *`, `PyValueBorrow`) | in-out stack items | fan-out |

- **Enforcement.** The generator gains a lint: a uop body that takes `&name`
  of a local it declares is an error, unless `name` is the box area or the
  callee is on a short list of inline calls (`__builtin_*_overflow`,
  `PyValue_FloatOperandValues`). The lint makes the rule hold by
  construction for new code. The object check (canaries outside the
  box-area list, GCC warnings other than `value_owners`) catches what the
  lint cannot see, such as a helper that stops inlining.
- **Hot paths.** R-c moves work to a C call, and under GCC each C call on a
  hot path costs 7 to 11 instructions of register moves (§2.2). R-c is
  therefore used only where the handler already calls out (calls, native
  calls, runtime API). R-a and R-b add no call.
- **What goes.** Revision 2's 232 B scratch union, the generator pass that
  renamed locals into it, the hot copies, the `IR_SCRATCH` accessor and the
  record's `owners`, `boxes` and `scratch` fields.

## 4. Handlers, labels and exits (item 4)

| Kind | At HEAD (sites / functions) | Under tail calls | Rewrite |
|---|---|---|---|
| Handlers | 292 `TARGET`s; 289 after step 1 | functions | generator |
| Labels | 5: `error`, `exception_unwind`, `exit_unwind`, `exit_unwind_notrace`, `start_frame`; and `UNKNOWN_OPCODE` from `ir_targets.h` | functions | generator; `TAIL_CALL_LOAD_FRAME` deleted |
| JIT labels | 4 in `ceval.c` (`jit_resume`, `jit_enter`, `jit_step`, `jit_consume_action`), computed goto only | – | deleted (step 1) |
| `DISPATCH` | 312 / 289 | `musttail` through the table | macro |
| `DISPATCH_SAME_OPARG`; bare `DISPATCH_GOTO` (`EXTENDED_ARG`, `INSTRUMENTED_LINE`, `INSTRUMENTED_INSTRUCTION`) | 50 / 50; 3 / 3 | the same, with the same `ir_word` | macro |
| Errors: `JUMP_TO_LABEL(error)` | 570 / 239 | tail call to `_TAIL_CALL_error`, after `finish_value_bridges` when a record is live | macro |
| Exceptions: `JUMP_TO_LABEL(exception_unwind)`, `exit_unwind*` | 7 / 7; 4 / 3 | tail calls | macro |
| Deopt: `JUMP_TO_PREDICTED` | 289 / 86, into 19 family heads | tail call to the head, with `next_instr = this_instr`; the head re-runs its prologue. The 19 `IR_PREDICTED` labels are dead | macro; A3c stops emitting the labels |
| Calls: `DISPATCH_INLINED` | 27 / 16 | `frame = tstate->current_frame = NEW`, then tail call to `start_frame` with the new `frame` | macro |
| Frame switches outside `DISPATCH_INLINED` | 14 / 14: returns and yields (`RETURN_VALUE`, `INSTRUMENTED_RETURN_VALUE`, `YIELD_VALUE`, `INSTRUMENTED_YIELD_VALUE`, `RETURN_GENERATOR`); generator sends (`SEND_GEN`, `FOR_ITER_GEN`); pushed Python frames (`CALL_PY_EXACT_ARGS`, `CALL_BOUND_METHOD_EXACT_ARGS`, `CALL_ALLOC_AND_ENTER_INIT`, `BINARY_OP_SUBSCR_GETITEM`, `LOAD_ATTR_PROPERTY`, `LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN`); `exit_unwind_notrace` | the assigned `frame` passes on at dispatch | none |
| Handler-local `goto` (call fallbacks) | 41 / 11 | within one function | none |
| Plain `return` | 3 / 2 | returns to `_PyEval_EvalFrameLoop` | none |
| Instrumentation | 24 `INSTRUMENTED_*` handlers | unchanged | none |
| Periodic checks: `check_periodics` | 16 / 16 | a C call on the cold path | none |
| Generator suspension and resumption | 6: `YIELD_VALUE`, `INSTRUMENTED_YIELD_VALUE`, `RETURN_GENERATOR`, `SEND`, `SEND_GEN`, `FOR_ITER_GEN` | the frame switch passes on | none |
| JIT | 3 handlers; 9 sites in `bytecodes.c`; 3 emission points | – | deleted (step 1) |
| `#if _Py_TAIL_CALL_INTERP` and `USE_COMPUTED_GOTOS` | 295 generated blocks; 27 lines in `ceval.c` (10), `ceval_macros.h` (11) and `bytecodes.c` (6), and 3 in the generated `ir_targets.h`; 11 in the generator | – | script and generator (A3c) |
| `IR_FIELD_*` | 1,550 generated lines; 2 emission points | `IR_*(ir_word)` | generator (A3c) |
| G9 | 200 canary functions; 133 after step 1's helper fixes; 77 outside the box area (§3.3) | values in registers, memory work out of line | 2 helpers by hand (step 1); generator and fan-out (step 2) |

Step 1's hand edits are the five `_Py_TAIL_CALL_INTERP` sites in
`bytecodes.c` (`:1476, 1866, 5670, 5694, 5729`: two entry asserts, which
become `frame == &act->entry.frame`, `assert(throwflag)`, and two
`int opcode` declarations) and the two helper fixes. Step 2's source edits
are the fan-out's (§3.3, §6). Every other change is a macro, a script or
the generator.

## 5. Debug, stats and free threading (item 5)

- **Debug tail-call build** (`--with-pydebug`). At every dispatch
  `PRE_DISPATCH_GOTO` asserts:
  - `frame == tstate->current_frame`, which already exists and becomes a
    real check once `frame` is a parameter (step 1);
  - A2's executing-copy invariant, `frame->ir_words ==
    _PyFrame_GetCode(frame)->_co_exec->words` (step 1);
  - at each loop entry, that the record lies between the stack pointer and
    `previous` (step 1, §3.2).

  `lltrace` stays as it is.
- **`Py_STATS` tail-call build** (`--enable-pystats`). `lastopcode` is the
  sixth parameter: r9 under GCC and rsi under Clang, with no stack argument.
  `INSTRUCTION_STATS` still counts only handlers below 256, so the IR-only
  handlers (267–304, including the per-operation `BINARY_OP_*` forms) carry
  no weight. That limit is older than A3 and is recorded as a follow-up.
- **Free threading.** `pycore_jit.h:157–158` excluded it from the JIT, which
  is moot now. `pycore_value.h:172–173` puts free-threaded builds outside the
  supported private ABI, and pyvalue.md D6 makes them a non-goal. A3 keeps
  the atomic fetch in `NEXTOPARG`, does not build or test free threading,
  and in step 1 makes `configure` reject `--disable-gil` with a message, so no
  path is left that is neither built nor tested.

## 6. Steps (item 6)

Two landings, in bulk style. Each landing has one suite run: GCC 15.2
debug and Clang 21 release in parallel, 3–11 minutes each at `-j8` on this
16-core host, so one slot covers both compilers and both build types. The
GCC release and `Py_STATS` builds compile and run targeted tests.
Measurement runs once, after step 2, and is not a gate.

| Step | What | Script | Agents | Test | Agent-h | Wall |
|---|---|---|---|---|---:|---:|
| **1. Mechanical** (A3a's tail-call half, A3c) | §1.1 and the fork's JIT source (its own commit); computed goto and the switch (`ceval_macros.h:152–188`, `opcode_targets`), the `IR_FIELD_*` memory decode, the `IR_PREDICTED` labels, `--with-computed-gotos`, `--with-tail-call-interp` and their `CFLAGS_CEVAL` flags; `frame` as a parameter; the §2.4 arms, MSVC first; the `configure` check and `PY_TAIL_CALL_CC`; `--disable-gil` rejected; the two helper fixes (§3.4); the record (§3.2); the gdb walk; the debug asserts | (a) delete the JIT-marked blocks and `git rm` the JIT files; (b) a 40-line resolver for `#if _Py_TAIL_CALL_INTERP` (1) and `USE_COMPUTED_GOTOS` (0) over `ceval.c`, `ceval_macros.h`, `bytecodes.c` and the generator's emitted text (`unifdef` is not installed) | **lead** (Opus, 3 h): runs the scripts, then by hand about 15 macro lines, the generator's frame load and `IR_*` decode, the 5 `bytecodes.c` sites, the 2 helpers, the record and the asserts; merges, builds, triages. **Sonnet A** (1.5 h), on disjoint files: the linker's `_PyJit_*` list (`specialize.c`, `ir.c`, `instrumentation.c`, `codeobject.c`, `_testinternalcapi.c`, `sysmodule.c`) and the tests that list handler numbers or `sys._jit`. **Sonnet B** (1.5 h): `configure.ac` and its regeneration, `Doc/using/configure.rst`, `libpython.py` | suites: GCC debug, Clang release. Targeted on GCC release and stats: `test_sys`, `test_generated_cases`, `test_ir_contract`, `test_call` recursion, `test_perf_profiler`, `test_external_inspection`. `tcobj.sh`: canaries in at most the 133 | 6 | 3 |
| **2. G9** (A3a's other half) | §3.5 | – | **lead** (Opus, 3 h): first a short interface note with the new helper signatures (R-a to R-c, 0.5 h); then the generator's emission patterns, every changed helper in `ceval.c`, and the lint with its `test_generated_cases` cases. The lead owns `ir_generator.py` and `ceval.c`. **Four Sonnet workers** (1–1.5 h each), from the interface note on, each in its own worktree, own disjoint uop definitions in `bytecodes.c`, including the 9 lines that declare `operation`: (1) the generic `BINARY_OP`, subscript and store-subscript uops; (2) the attribute uops (`LOAD_ATTR`, `STORE_ATTR`, `*_NOFB`, `STORE_ATTR_WITH_HINT`, `LOAD_SUPER_ATTR_METHOD`); (3) `CALL_BUILTIN_O`'s native scope and publication, and the rest of the canary and warning list; (4) the runtime-API out-parameters, the three slot-addressed handlers and `exception_unwind`. Each returns a patch. **Builder** (Sonnet, 1 h): merges, regenerates, builds both compilers, runs the object check, fixes the remainder | suites: GCC debug, Clang release. Canaries only in the box-area list; no GCC warning outside `value_owners`; the lint on | 9 | 3 |
| **Measurement** | §7, once, after step 2; the sampler is widened during step 2 | – | one Sonnet agent | – | 1.5 | 1.5 |
| **Total** | | | | | **16.5** | **6 to landing 2; 7.5 with measurement** |

**Against the expected 5 hours.** The critical path, in hours:

| Phase | Step 1 | Step 2 |
|---|---:|---:|
| scripts, or the interface note | 0.5 | 0.5 |
| hand edits, or workers beside the lead's generator change | 1.5 | 1.5 |
| merge, build both compilers, object check | 0.25 | 0.5 |
| suites (debug and release in parallel) | 0.2 | 0.2 |
| one triage round | 0.5 | 0.5 |
| **total** | **about 3** | **about 3** |

Step 1's hand edits do not split further: the macros, the generator and
`ceval.c` change together. Starting step 2's interface note and workers
when step 1 first compiles saves 0.5–1 h. That gives **5.5–6 h to the
second landing**. Five hours holds only if neither landing needs a triage
round. I plan on 6.

- **Landing.** Step 1 lands after A5b. Step 2 lands on one backend, so its effect is visible on its own in the single measurement's per-handler data.
- **Against the packet's 32 hours:**
  - A3b's 9.5 h go with the JIT;
  - the dual-backend work and the dual-CI cost (2 h) go, because no landing carries two backends;
  - the record shrinks to its persistent half;
  - G9 becomes emission patterns, not a scratch rewrite.

## 7. Expected effect and measurement (item 7)

These are hypotheses. Performance is recorded, not a gate. The baseline is
HEAD's computed-goto build: 40.7 instructions and 7.6 stack references per
word (59% set), against upstream's 22.0 and 1.3.

| Change | What it removes from the per-word path (static model) | Hypothesis |
|---|---|---|
| Step 1: tail calls, `frame` a parameter | Loop state leaves the computed-goto loop's C frame (3,304 B at `48bb4d6d1d3`) for argument registers. At `48bb4d6d1d3` (`tcvt` against `vt`, 59% set) this cut stack references from 8.6 to 1.5 per word at the same instruction count (45.5 against 45.4), with all 200 canaries in place. It removes the per-handler frame load and adds `endbr64` (1) and, under Clang, the frame-pointer triple (3) | stack references 7.6 → 1.5–2.5; instructions 40.7 ± 2 before the helper fixes |
| Step 1: helper fixes | a frame and a canary, about 8 instructions (§2.2), on 67 functions with 41.8% of entries: about 3.3 per word | about −3.3 per word. Step 1 as a whole: time 0.78–0.88× of HEAD's computed goto (the first link, with every canary, ran 0.870×) |
| Step 1: one backend | the `IR_FIELD_*` re-reads; `CFLAGS_CEVAL`'s `-fno-gcse` and `-finline-max-stacksize` | small; size recorded |
| Step 2: G9 | the same cost on the 37 functions without a box area, 11.0% of entries: about 0.9 per word. The 40 with a box area keep their canary. The box area itself, 3.9% of entries and about 0.3 per word, is R27's | about −0.9 per word; time 0.98–0.99× of step 1 |
| Compiler | GCC moves state around C calls: −1 to +7 per handler with a cold call, +11 with a hot call (about 10% of entries, so about +1.1 per word). Clang keeps a frame pointer (3) | Clang 21 at or below GCC 15.2 in instructions per word, and faster overall |

By this model A3 removes about 4.2 of the 19-instruction excess over
upstream (3.3 + 0.9), and R27 later about 0.3 more. The larger effect is on
stack traffic, which the instruction count does not show; time measures it.
The rest of the excess has other owners: the slot-class test (A4 or B3),
borrow tags and publish canonicalisation (A5), `STORE_FAST` at 24
instructions and `LOAD_ATTR_SHAPE` at 85, and `NOT_TAKEN` (B2).

Measurement plan, once, after step 2 (HEAD computed goto, step 2 on GCC and on Clang, upstream):

| Measurement | Tool | Note |
|---|---|---|
| Lower bound per word, 59% and 78% sets | `irv2/hmin.py` and `weighmin.py` on the tail-call `ceval.o`, GCC and Clang, with the remeasure-20261104b weights | under tail calls a handler's entry is its `_TAIL_CALL_*` symbol, not an `opcode_targets` relocation, so `labels.py` needs a symbol mode; the packet's `tc` figures (`labels_tc.json`, `hmin-tc.tsv`) show the path works |
| Canaries, frames, pushes per function | `tcobj.sh`, `tccanary.py`; GCC's `musttail` warnings | target in §3.5 |
| 29 kernels, interleaved | remeasure method: HEAD computed goto, tail call GCC, tail call Clang, upstream | U/C and against HEAD |
| Loop share of the gap | the LD_PRELOAD sampler | **its classifier keys on `_PyEval_EvalFrameLoop`. Under tail calls the loop is the `_TAIL_CALL_*` symbols, so the bucket must first be widened to count their self time.** Inclusive time differs between compilers, because GCC drops the frame pointer in small handlers |
| Entries to the box-area handlers, per kernel | stats build | R27's exemption list |
| C stack per nested evaluation | `sizeof(_PyEvalActivation)`, DWARF frames of the two eval functions, and `TestRecursion`'s depth | against the computed-goto loop frame and HEAD's tail-call build |
| `ceval.o` text | `size` | against 473,589 B (computed goto at `5de7bd62ef9`) |

Every result records the compiler version and `PY_TAIL_CALL_CC`.

## 8. Comparison (item 8, scope cut: brief)

| System | What it does | Taken | Rejected |
|---|---|---|---|
| Upstream CPython tail-calling interpreter (3.14; `3a440b6`) | Clang only: `#error` without `preserve_none` and `musttail`. Handlers are `Py_NO_INLINE` functions; at `3a440b6` the parameters are `frame, stack_pointer, tstate, next_instr, instruction_funcptr_table, oparg` (+ `lastopcode`). The table is a parameter so tracing can switch it. Labels are functions | handler and label as functions; `frame` as a parameter; `musttail` dispatch through a table | Clang only (A3 qualifies GCC); the table as a parameter (the fork switches none); `oparg` predecoded (A3 passes `ir_word`; Q4) |
| LuaJIT 2.1 interpreter (`vm_x64.dasc`) | handlers in DynASM assembly; `KBASE`, `PC` and `DISPATCH` in fixed callee-saved registers, so they survive C calls (`BASE` is refetched); `ins_NEXT` decodes A, D and OP before the jump; slow paths in shared `->vmeta_*` stubs | state in registers that survive C calls (Clang's r12–r15); handlers with no addressable locals, and memory work out of line (§3.5) | hand assembly: a second handler language |
| V8 Ignition | handlers built with the CodeStubAssembler under a fixed interpreter calling convention (accumulator, bytecode offset, bytecode array, dispatch table); dispatch is a tail call into the next handler; the bytecode offset is saved only when calling out | the generator owns the convention; a handler's state is in registers and helpers take and return values (§3.5); save the PC only at escapes (G9 item 1, after A3) | the accumulator; a second handler language |
| upb's tail-call protobuf parser (`musttail`; J. Haberman, "Parsing Protobuf at 2+GB/s", 2021) | as its author describes it: each field handler is a function with six register parameters that ends in `musttail`, and fallback paths are separate functions, so a fast path calls nothing and saves nothing | the probe's finding that a hot path with no C call is the cheapest form on both compilers (`tailslow`) | – (Q5). wasm3, by its design notes, relies on the optimiser to turn its calls into jumps; A3 requires `musttail`, which also holds at `-O0` |

## 9. Risks and open questions (item 9)

### 9.1 Risks

| # | Risk | Recommendation |
|---|---|---|
| R1 | GCC moves state on hot paths around C calls (§2.2): up to 7 instructions with a cold call, 11 with a hot one; hot-call handlers take about 10% of entries | Measure both compilers after step 2 (Q2), and report the hot-call handlers separately. GCC's coming `preserve_none` removes it, with no source change |
| R2 | Both calling conventions are documented as unstable | Handlers stay `static`, and nothing exported uses the convention. The JIT stage enters through C-ABI functions (§1.2) |
| R3 | A miscompile under `musttail`, which is new in GCC 15's C front end | Full suites on both compilers; the debug frame and executing-copy asserts; `tcobj.sh` on both |
| R4 | gdb: `libpython.py` finds `EVALFRAME = '_PyEval_EvalFrameLoop'` and reads its `frame` variable (`get_pyop`, `:1885–1894`). Under tail calls that variable is the entry frame, stale after any inlined call. Reading `tstate->current_frame` would give every outer activation the innermost frame, and GCC builds most small handlers with no frame to read. `test_gdb` cannot run on this host | In A3.2, `libpython.py` reads each loop frame's `activation` local and walks `eval_activation`: Python frame k is `act[k−1].entry.frame.previous`, and the innermost is `tstate->current_frame`. Record gdb as unverified until it runs on a host with gdb |
| R5 | Profilers and the sampler see `_TAIL_CALL_*` symbols instead of one loop. A sample in a GCC handler without a frame pointer skips `_PyEval_EvalFrameLoop` | `test_perf_profiler`, `test_samply_profiler`; widen the sampler's classifier to `_TAIL_CALL_*` self time before measuring (§7) |
| R6 | Stack-switching extensions lose `eval_activation`, and gdb or an assert reads another stack's record | ports notes; greenlet's port saves it with `current_frame`; the debug range assert (§3.2) |
| R7 | C stack per nested evaluation changes | `test_sys` and `test_call` recursion tests; record the size (§7) |
| R8 | A by-value helper no longer nulls the caller's copy (as `retire_published_values` does through its pointers), so a stale copy could be used after the call | the generator emits no use of a value after the call that consumes it; `test_generated_cases` checks each pattern; the debug build's value checks |
| R9 | Regenerating `configure` needs the project's autoconf | regenerate with the tree's documented script; if it cannot run here, the coordinator regenerates |
| R10 | The tail-call figures are old (`48bb4d6d1d3`) | the single measurement includes HEAD's computed goto, so the comparison is at one base |
| R11 | R-c adds a C call; under GCC each one on a hot path costs 7–11 instructions (§2.2) | use R-c only where the handler already calls out (calls, native calls, runtime API); R-a and R-b add no call |
| R12 | The box area keeps a frame and a canary in 96 functions (3.9% of entries) until R27 | the exemption list may only shrink; R27's waves empty it |

### 9.2 Open questions

| # | Question | Recommendation |
|---|---|---|
| Q1 | Delete the fork's JIT source files? | **Decided** (user): delete them in step 1, in their own commit, named in the JIT-stage notes |
| Q2 | Which compiler builds the measured release after A3c? | Measure both after step 2, weighting GCC's cost by the hot-call handlers' entries (R1). The faster becomes the measured build, and both stay qualified. The probes favour Clang 21 |
| Q3 | Keep the plain-`musttail` arm? | Keep it: one empty macro, no code path, not qualified, its cost stated (§2.5) |
| Q4 | Predecode a field (`oparg`, `dst`) into a parameter (packet open question 7)? | After step 2, from `hmin.py` evidence. GCC's sixth register is free except under `Py_STATS` |
| Q5 | Tail-called slow paths (`tailslow`: 19 / 18 against 23 / 28)? | A follow-up after step 2: R-c's out-of-line functions are the candidates, tail-called instead of called where they end in a dispatch. It needs the DSL or the contract to mark cold sections (error, deopt, release), and it is the natural successor to the record |
| Q6 | Frame pointers in handlers (3 instructions per handler under Clang)? | Keep them: `configure`'s default, needed by native profilers. Record the cost |
| Q7 | Merge `_PyEval_EvalFrameDefault` and `_PyEval_EvalFrameLoop`, putting the floor in the record? | No: no handler reads the floor, and gdb anchors on the loop's name |

## Files

Under `build/pyjit-link-wave1/a3/`:

| File | What |
|---|---|
| `recount.py` | handlers, labels, records, address-taken locals and exits, from the git blobs of a commit |
| `addr-36aa4a0345a.tsv` (= `addr-faddfd1454d.tsv`, `addr-a6002e5854b.tsv`) | one row per address-taken site in the source |
| `scratch.py` | per-handler escaping-local bytes (§3.1) |
| `kinds.py`, `kinds3.py` | G9 by kind; `kinds3.py` gives §3.3's table (box area excluded, generator or fan-out), from the TSV and the entry weights |
| `weights.py` | entry shares of record and escaping handlers, from the TSV and `pyjit/bench/results/remeasure-20261104b-C-kernels-boxes.json` |
| `probes/attrs.c` | attribute support per compiler |
| `probes/defaults.sh` | default hardening per compiler (canary, `endbr64`) |
| `probes/handler.c`, `handler.sh`, `fnstat.py`, `pivot.py` | §2.2's table (with `hotcall`), §2.3's listings; `-DRECPARAM` (the record as a parameter, rejected in revision 1) |
| `probes/tcobj.sh`, `tccanary.py` | §3.4's object list: HEAD's `ceval.c` as the tail-call build on both compilers, with `FIX=1` for step 1's helper fixes; GCC's `musttail` warnings |
| `probes/optlevels.sh` | `musttail` at `-O0`, `-Og`, `-O2` |
| `probes/sizes.c`, `sizes.sh` | §3.1's sizes, from a `git archive` of the headers |
