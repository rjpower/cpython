# Register IR v2 steps A3a and A3c: tail calls as the only backend (proposal)

Status: phase 1, revision 2, 2026-11-04. Review 1
([register-ir-v2-a3-review-1.md](../reviews/register-ir-v2-a3-review-1.md))
is folded in; see "Changes after review 1".

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
  `ceval.o` compiled out of tree on both compilers, with and without A3.2's
  helper fixes;
- `scratch.py`, `kinds.py`, `weights.py` (entry shares, from
  `remeasure-20261104b-C-kernels-boxes.json`) and the other `probes/`.

The tree was not built. Where the packet and the code disagree, the code
wins (§0).

## Summary

| Item | Decision |
|---|---|
| 1. The fork's JIT | Out of the build (decided). A3.1 deletes the loop's JIT locals and labels, the `JIT_*` macros, the three warm handlers and the generated JIT helpers. It takes the fork's JIT objects out of the build and makes `sys._jit` constant `False`. A3b is dropped. §1.2 lists what the JIT stage restores. |
| 2. Calling convention | `(frame, slots, tstate, next_instr, ir_word)`, plus `lastopcode` under `Py_STATS`, still in a register. `preserve_none` on Clang 21 (r12–r15, rdi), `no_callee_saved_registers` on GCC 15.2 (rdi, rsi, rdx, rcx, r8), plain `musttail` otherwise. `configure` checks `musttail`, picks the attribute and records it, and measurement refuses the plain arm. GCC takes 0 to 11 more instructions than Clang in a handler that makes a C call. |
| 3. Activation record | `_PyEvalActivation`. A3.2 adds the previous link and the entry frame, and gdb walks the chain. A3.4 adds the box area (20 boxes) and the handler scratch (a generated union, at most 232 B). 544 B per activation, linked from the thread state. The JIT fields go. The borrow floor stays in `_PyEval_EvalFrameDefault`. |
| 4. Handlers and labels | 298 functions: 292 handlers, 5 labels and `UNKNOWN_OPCODE` (295 after the JIT handlers go). Every exit is a macro or generator output. G9 is counted on the object: at HEAD, 200 functions carry a canary (56.7% of entries). Two helper fixes in A3.2 remove 67 of them (41.8%). A3.4 removes the other 133 (14.9%) with one generator change, then a fan-out over the remaining canary list and GCC's `-Wmaybe-musttail-local-addr` output. Ten edits are by hand. |
| 5. Debug, stats, free threading | The debug frame assert already exists, and becomes a real check once `frame` is a parameter. A3 adds executing-copy, record-range and empty-box asserts. `lastopcode` is the sixth argument, in a register on both compilers. Free threading is outside the supported ABI, and A3.3 makes `configure` reject `--disable-gil`. |
| 6. Steps | A3.1 JIT hooks out (4 agent-hours); A3.2 tail-call build with helper fixes, record skeleton and gdb (7); A3.3 = A3c, one backend (3.5); A3.4 record and G9 rule, with fan-out (13); measurement (2.5). 30 agent-hours and about 16.5 hours wall clock, against the packet's 32. A3.2 and A3.3 land together. A3.1 and A3.4 each land alone. "A3a + A3b + A3c, one landing" no longer holds. |

## Changes after review 1

Review 1 gave "accept with changes" with 15 findings. All are accepted;
finding 7 is accepted with one qualification.

| # | Finding | Change | § |
|---|---|---|---|
| 1 | must-fix: the G9 recount misses addresses taken in inlined helpers | Reproduced on the object (`tcobj.sh`): 200 functions (Clang 200, GCC 197), 56.7% of entries. Of the 67 that the source count misses, 66 inline `finish_displaced_local`; the 67th is `LOAD_GLOBAL_BUILTIN` (`_PyValue_Publish` not inlined; Clang here). A3.2 makes `finish_displaced_local` `Py_NO_INLINE` and `_PyValue_Publish` `Py_ALWAYS_INLINE`. The list then falls to 133 functions (14.9%), the source count's set exactly. The object list is the work list. §3.4, §7, the A3.4 fan-out and the hand-edit count (8 → 10) are redone | 0, 3.4, 3.5, 4, 6, 7 |
| 2 | should-fix: GCC's cost with a C call on the hot path | Shape `hotcall` added: Clang 26, GCC 37 (+11); with the attribute dropped, Clang 46 and GCC 48. Handlers with a hot-path C call take about 10% of entries. R1 and Q2 are weighted by them | 2.2, 2.5, 9 |
| 3 | should-fix: the base moved; `sys._jit` | Rebased on `36aa4a0345a`. A3.1 gains `sysmodule.c` and `test_sys`. The sentence on upstream's `jit*.c` goes. The fork's JIT objects have no `PCbuild` entries | 1.1, 6 |
| 4 | should-fix: R4's gdb fix fails for nested activations | The record's persistent half (previous link, entry frame) moves to A3.2 under a fixed local name, `activation`. `libpython.py` walks `eval_activation`. The two `frame == &entry.frame` asserts come back as `frame == &act->entry.frame` | 3.2, 4, 6, 9 |
| 5 | should-fix: the C-stack claim used stale frames | Restated: about 2.7 KB less per nested activation than computed goto, and 400–480 B more than HEAD's tail-call build. `TestRecursion`'s depth is recorded | 3.2, 7 |
| 6 | should-fix: the plain arm builds silently; MSVC falls through | `configure` reports the convention as `PY_TAIL_CALL_CC`, and the harness refuses `none`. The MSVC arm comes first | 2.4 |
| 7 | should-fix: GCC names escapes at compile time | Reproduced: 1,096 warnings in 115 functions (the review saw 1,111). Workers get the warnings with the canary list, and GCC builds with `-Werror=maybe-musttail-local-addr` after A3.4. Qualified: the stored address the review names, `method_found`, is upstream's pattern, with the pointer scoped to one call, so it is not a use-after-return bug | 3.4, 3.5 |
| 8 | should-fix: the estimates omit known items | Costed: the helper fix, gdb, the sampler, `sys._jit`, regenerating `configure`, recording the convention, and 7 hours for the generator over two agents | 6 |
| 9 | note: GCC drops the frame pointer in small handlers | The sampler counts `_TAIL_CALL_*` self time | 7, 9 |
| 10 | note: a stack switch that misses `eval_activation` corrupts memory | Debug assert: `act` lies between the stack pointer and `act->previous` | 3.6, 5 |
| 11 | note: a refused `PyValueBorrow` field has no remedy | Remedy stated | 3.3 |
| 12 | note: record the compiler and attribute | Recorded with every measurement | 7 |
| 13 | note: Q1, delete the source | Its own commit in A3.1, named in the JIT-stage notes. The user decides | 6, 9 |
| 14 | note: free threading is left unbuilt | A3.3: `configure` rejects `--disable-gil` | 5, 6 |
| 15 | note: remote debugging is unaffected | No change; `test_external_inspection` stays in A3.2 | 6 |

## 0. Where the packet and the code disagree

| Item | Packet | Code at `36aa4a0345a` |
|---|---|---|
| Box area | 37 boxes (296 B); capacities 2–37 | **20 boxes** (160 B). 96 handlers have a value record, with capacities 3–20 (`CALL_LEN` 20, `CALL_METHOD_DESCRIPTOR_O` 18) |
| Handlers that pass a local to an out-of-line function | 75 and one label | **76 and one label** (`exception_unwind`) in the source. With the 96 value records, 133 functions |
| Functions with a canary in the tail-call object | 200 (`tcvt`, packet:1120) | **200** (Clang 200, GCC 197): the 133, plus 67: 66 that inline `finish_displaced_local`, and `LOAD_GLOBAL_BUILTIN`. 56.7% of entries (§3.4) |
| `callable` (12 handlers) | in the scratch | **no handler** takes `&callable` |
| Record size | about 850 B | **544 B**: no JIT fields, 20 boxes (§3.2) |
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
computed-goto release build still compiles it all. Step A3.1 deletes it.

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
| Per-activation JIT state | six loop locals | fields of `_PyEvalActivation`, kept apart from the scratch because they persist across handlers |
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
  - The activation record comes from `tstate` on the paths that use it
    (§3.6).
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

- **The smallest handler.** GCC takes 14 instructions and Clang 17. Clang
  keeps `push %rbp; mov %rsp,%rbp; pop %rbp` under
  `-mno-omit-leaf-frame-pointer`. GCC omits the frame pointer in a function
  whose only call is the tail call. With `-fomit-frame-pointer`, Clang also
  takes 14.
- **Any C call costs GCC on the hot path.** GCC sets up a frame on entry
  and moves the state that is live after the call into callee-saved
  registers, then moves it back before the jump: in `call`, four moves in and
  three out; in `hotcall`, five in, three out and a spill of `next_instr`.
  Clang's state is already in r12–r15. Against Clang, GCC is 1 instruction
  shorter to 7 longer when the call is cold (`call` +7, `record` +5,
  `outlined` +7, `close` −1) and 11 longer when it is hot (`hotcall`).
  Nearly every handler has a cold call, if only `_Py_Dealloc`; handlers with
  a hot-path call take about 10% of entries (`CALL_BUILTIN_*`, `LOAD_ATTR`,
  `FOR_ITER`, `CALL_ISINSTANCE`, `BINARY_OP_SUBSCR_DICT` and others).
- **An address-taken local costs both compilers about 8 instructions on
  every entry.** That covers a frame and a canary, even when the address is
  taken only on a cold path: `addr` against `record` is 31 against 23 under
  Clang, and 36 against 28 under GCC. The box array (`boxes`) costs the same.
- **A tail-called slow path is the shortest form on both compilers**
  (`tailslow`, 19 and 18), because the hot path then makes no C call. This is
  open question Q5, not part of A3.
- **`Py_STATS`.** With `lastopcode` as a sixth argument, GCC's hot paths
  barely change (`gcc-ncsr-stats`: 14, 28, 36; `hotcall` 39).
- **Debug levels.** `musttail` holds at `-O0` and `-Og` on both compilers
  (`probes/optlevels.sh`).

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

In the tree the table is `static`, so Clang also uses a `lea`. GCC's
register moves around a C call (§2.2) are in `handler.sh asm`'s `h_call`.

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
- **GCC's `preserve_none`.** The GCC manual documents an x86 `preserve_none`
  that passes r12, r13, r14, r15, rdi and rsi, and calls it "subject to
  change" (PR 119628). GCC 15.2 lacks it (`probes/attrs.c`). When GCC ships
  it, the first arm picks it up with no source change, and GCC's state moves
  to Clang's registers.
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
- **`CFLAGS_CEVAL`.** `configure` adds `-fno-gcse` (GCC) and
  `-finline-max-stacksize=512` (gh-148284) only when computed gotos are on
  (`configure.ac:8115–8128`). Both were tuned for the one large loop
  function. A3c deletes the computed-goto check, so both flags fall away;
  A3c records the text size and the static path without them.

## 3. The activation record (item 3)

### 3.1 Per-activation state at HEAD, recounted

Sizes are the same on both compilers (`probes/sizes.sh`).

| State | Bytes | Where at HEAD | Lifetime | A3 |
|---|---:|---|---|---|
| Entry frame `_PyEntryFrame` | 120 | local of `_PyEval_EvalFrameLoop` (`ceval.c:2255`), linked as `frame->previous` | activation | record |
| JIT locals | `_PyJitContext` 136, 5 words; `_PyJitFlow` 1,560 on the heap | loop locals, computed goto only | activation | deleted (A3.1) |
| Borrow floor: `outer_floor`, `outer_request`, `floor` | 20 | locals of `_PyEval_EvalFrameDefault` (`ceval.c:2517–2524`) | activation | **stay**. No handler reads them; `_Py_HandlePending` reads `borrow->eval_floor` |
| Box area | 160 (20 × 8) + `_PyValueTemps` 24 | computed goto: one shared array. Tail calls: an array in each of 96 handlers | one handler | record |
| Handler scratch | ≤ 232 (`CALL_BUILTIN_O`: `_PyNativeScope` 152, `_PyNativeCallPublication` 56, three values); median 80 | C locals of 76 handlers and 1 label | one handler | record, as a generated union |
| Previous link | 8 | – | activation | record |

Review 2 asked for one structure that also holds the floor (R2-2). With the
JIT out, only the entry frame persists across handlers. The floor is three
locals of a function that is not a handler, so moving it buys nothing, and
merging the two functions would move gdb's anchor (`EVALFRAME =
'_PyEval_EvalFrameLoop'` in `libpython.py:106`).

### 3.2 The record

```c
typedef struct _PyEvalActivation {
    struct _PyEvalActivation *previous;   /* A3.2: restored when this activation exits */
    _PyEntryFrame entry;                   /* A3.2: persists across handlers */
    _PyValueTemps owners;                  /* A3.4: dead at every dispatch */
    PyObject *boxes[_PyIR_MAX_BOXES];      /* generated constant: 20 at HEAD */
    _PyIRScratch scratch;                  /* generated union of per-handler layouts */
} _PyEvalActivation;                       /* 8 + 120 + 24 + 160 + 232 = 544 B */
```

- `_PyEval_EvalFrameLoop` declares it as the local `activation` and links it
  from `_PyThreadStateImpl.eval_activation`, beside `borrow`. It restores the
  previous link on both of its exits: the return from the handler chain, and
  `early_exit`.
- **Two halves.** A3.2 adds `previous` and `entry`, the persistent half that
  gdb and the entry asserts need (§9, R4). A3.4 adds `owners`, `boxes` and
  `scratch`.
- **C stack per nested evaluation.** HEAD's tail-call handler frames are at
  most 320 B (GCC) and 304 B (Clang), with medians of 96 B and 64 B
  (`tcobj.sh`). The handler that calls out keeps a small frame and the
  activation adds the record's 424 B beyond the entry frame, so each nested
  activation costs 400–480 B more than HEAD's tail-call build. It costs about
  2.7 KB less than the qualified computed-goto build, whose loop frame was
  3,304 B at `48bb4d6d1d3`. A3.4 records `sizeof(_PyEvalActivation)` and
  the depth that `test_call`'s `TestRecursion` reaches before and after.
- `_PyIR_MAX_BOXES` and `_PyIRScratch` are generated into `ir_targets.h`,
  which `ceval.c` includes before it defines the record.

### 3.3 How each field obeys L1

The record outlives handlers, so it is a home and holds owned `PyValue` words
only (A5b §8.2, L1).

| Field | Holds | L1 |
|---|---|---|
| `entry` | a frame header; `entry.stack[0]` receives the returned or yielded value, owned | home, owned |
| `owners`, `boxes` | `PyObject *` boxes from `PyValue_Box`, owned (`PyValue_MakeHeapSafe`, `PyValue_NewRef` when boxing) | owned object references; no `PyValue` word |
| Scratch: `_PyValueOperation`, `_PyCallTransaction`, `_PyNativeCallPublication` | thread, frame and code pointers, word offsets, ints; `publication.result` points at a slot | no `PyValue` word. A slot pointer is not a borrow (G9.6) |
| Scratch: `_PyNativeScope` | after A5b-3, object views that `End` releases in the same handler, and an owned `argument_box`; no `PyValueBorrow` | views are allowed (R2-S2) |
| Scratch: `PyValue` locals (`pending`, `pending_self`, `held_*`, `receiver`, `attr`, `old_value`, `item`, `v`, `c`, `s`, `release_value`, `retained_dict`, `watched_dict`) | owned after A5b (`TAKE`, `OWNER`) | owned |
| Scratch: `PyObject *` and `int` out-parameters (`temp`, `dupkey`, `match_o`, `rest_o`, `retval_o`, `err`, `invoked`, `exc_slot`, `handler`, `lasti`, `method_found`) | API results | no `PyValue` word |
| `null_or_index` (`FOR_ITER`, `INSTRUMENTED_FOR_ITER`, `SEND`) | an in-out stack item read in place, which the callee updates | **not moved**: slot-addressed (`&IR_SLOT(F)`, G9.6), by hand, since after A5b an in-place read is a view |
| `index_or_null` (`GET_ITER`) | an output, owned | scratch |

The generator enforces the rule: it refuses a scratch field of type
`PyValueBorrow` and names the handler. The remedy is to `TAKE` the value
before the escape, or to pass the slot (`const PyValue *`, G9.6) instead.

### 3.4 Where G9 fails at HEAD

**The work list is the object, not the source.** `tcobj.sh` compiles HEAD's
`ceval.c` as the tail-call build on both compilers, with
`-fstack-protector-strong`, so a canary marks a function with a local array
or an escaping address after inlining.

| Set | Functions | Entries | What puts the canary there |
|---|---:|---:|---|
| HEAD | **200** (Clang 200, GCC 197) | **56.7%** | the 133 below, and 67 more |
| the 67 the source misses | 67 | 41.8% | 66 inline `finish_displaced_local` (`ceval.c:2128–2137`), which passes `&displaced` to `finish_value_bridges_ex`; 159 handlers call it. `LOAD_GLOBAL_BUILTIN` calls an out-of-line `_PyValue_Publish(&V)` (Clang here, GCC in the review). The largest: `LOAD_FAST_BORROW` 13.8%, `PUSH_NULL` 3.0%, `LOAD_SMALL_INT_INLINE` 2.9%, `LOAD_COMMON_CONSTANT` 2.7%, `LOAD_GLOBAL_BUILTIN` 2.2%, `BINARY_OP_ADD_INT` 1.5% |
| after A3.2's two fixes | **133** (Clang 133, GCC 131) | **14.9%** | exactly the source-visible set below |

A3.2's fixes, both one line: `finish_displaced_local` becomes `Py_NO_INLINE`,
so `&displaced` is taken in its own frame on the cold path, and
`_PyValue_Publish` becomes `Py_ALWAYS_INLINE`. `tcobj.sh` with `FIX=1`
measures that result.

GCC's `-Wmaybe-musttail-local-addr` names source locals: 1,096 warnings in
115 functions, for `value_owners` (955), `call` (96), `err` (28),
`pending_self` (8), `method_found`, `publication`, `dupkey`, `retval_o` (2
each) and `temp` (1). It names neither `displaced` nor `held_*` nor
`operation`, so it supplements the object list. The fixes do not change it.

The 133, by kind: `kinds.py` over `recount.py`'s rows, that is, locals
declared in a handler whose address reaches a function that is not
`static inline`, or is stored. "Sites" are in the generated file, where one
uop line appears in every handler that uses the uop.

| Kind | Functions | Sites | Locals | Address taken in |
|---|---:|---:|---|---|
| Box area (value record) | 96 | 96 | `value_owners` (+ the `value_boxes` array) | generator 96 |
| Transaction struct | 54 | 154 | `operation` (38), `call` (15), `publication`, `scope` | generator (`value_operation_begin`, `call_transaction_begin`, `value_reader_complete` emission); `bytecodes.c` for `scope` and `publication` |
| Value or int to a static helper | 45 | 260 | `pending` (31), `held_left`, `held_right` (27 each), `receiver` (14), `attr` (5), `old_value`, `invoked` (4 each), `held_value`, `held_sub`, `pending_self` (2 each), `held_dict`, `held_container`, `release_value`, `retained_dict`, `watched_dict`, `c`, `s` | `bytecodes.c` uop bodies; the generator's `retire_published_values` and `value_reader_complete` calls (`ir_generator.py:1946`, `:2165`) |
| Out-pointer written by a helper | 15 | 26 | `call_child` | generator 26 |
| Runtime API out-parameter (upstream has the same) | 14 | 21 | `err` (4), `item`, `null_or_index` (3 each), `temp`, `dupkey`, `match_o`, `rest_o`, `index_or_null`, `v`, `retval_o`, `pending`; `exc_slot`, `handler`, `lasti` (`exception_unwind`, as the packet says) | `bytecodes.c` |
| Pointer stored for later use | 1 | 1 | `method_found` (`LOAD_SUPER_ATTR_METHOD`) | `bytecodes.c` 1 |
| **Any of these** | **133** (132 handlers, `exception_unwind`) | **558** | | **35 source lines** in `bytecodes.c`; the rest is generator output |

Also counted:

- **Inline-only.** 286 sites, in 74 more handlers, pass an address only to
  `static inline` code: the publish macros (`IR_STORE_DST` and three others,
  275), `PyValue_FloatOperandValues` (8) and `__builtin_*_overflow` (3).
  After A3.2's fixes, none leaves a canary.
- **Local arrays.** `value_boxes` in the 96, and no other.
- **Entry shares of the 133** (29 kernels; `INSTRUCTION_STATS` counts only
  handlers below 256): 3.9% of entries reach a handler with a record and
  12.7% one with an escaping local; 14.9% reach one or both. The largest is
  `LOAD_ATTR_SHAPE` (4.49%, `receiver` and `attr` to
  `retire_published_values`). Next come `LOAD_ATTR` 0.99%, `CALL_BUILTIN_O`
  0.92%, `FOR_ITER` 0.76%, `LOAD_ATTR_INSTANCE_VALUE` 0.67% and
  `LOAD_ATTR_SLOT` 0.64%.

### 3.5 The rewrite: a generator change, then a fan-out over the object list

The rule: **no handler function takes the address of its own local.** A
callee that needs a pointer gets one into the record, or into a frame slot.
The generator does steps 1–3 in one pass over each handler's tokens. Step 4
is by hand, step 5 is the check and step 6 the fan-out:

1. **Box area.** The record replaces the per-handler `value_boxes[N]` and
   `value_owners`. The handler resets `owners` and sets
   `value_temps = &act->owners`. The generator emits `_PyIR_MAX_BOXES` as the
   largest `value_bridge_bound`. The `DISPATCH` and `JUMP_TO_LABEL` tests on
   `value_temps->count` do not change.
2. **Escaping locals.** The generator finds every `&name` whose `name` is
   declared in the handler, outside a short allowlist of inline callees
   (`__builtin_*_overflow`, `PyValue_FloatOperandValues`). It collects the
   type from the declaration, emits `struct _PyIRScratch_<HANDLER>` with
   those fields and the union `_PyIRScratch`, and rewrites the declaration
   and every use of the name to `IR_SCRATCH(name)`. Locals the generator
   declares itself (`operation`, `call`, `call_child`) it declares in the
   scratch directly. A name declared twice with one type in disjoint scopes
   (`call_child` in 12 call handlers) gets one field. A declaration of
   several names (`PyValue pending, pending_self;` in `LOAD_ATTR`,
   `int exc_slot, handler, lasti;` in `exception_unwind`) is split. These
   locals already live
   in memory today, on the handler's stack, so the rewrite moves their home
   without adding memory traffic. It refuses a name declared twice with two
   types, and a `PyValueBorrow` field.
3. **Hot copies.** For the generator's own `retire_published_values` call in
   the ten attribute handlers (`receiver`, `attr`, `old_value`;
   `ir_generator.py:1946`), it copies the values into the scratch before the
   call and back after it. Their fast path then keeps them in registers.
   These handlers take 7.8% of entries.
4. **Slot-addressed.** By hand: `FOR_ITER`, `INSTRUMENTED_FOR_ITER` and
   `SEND` pass `&IR_SLOT(F)` for `null_or_index`
   (`bytecodes.c:1630, 3764, 3807`), using A5b's parameter types
   (`const PyValue *` for slots, `PyValueBorrow` for values only read).
5. **Check on the object.** `tcobj.sh`'s list (`tccanary.py`, from
   `irv2/tcframes.py`) becomes a test tool. For every `_TAIL_CALL_*` function
   on both compilers, it reports the canary, the stack adjustment and the
   pushes. The target: **no function has a canary**, and every frame is at
   most N bytes. N is the value measured at landing and may only fall.
   Frames for register spills are allowed. GCC then builds with
   `-Werror=maybe-musttail-local-addr`.
6. **Fan-out over what remains.** After steps 1–4, the builder regenerates,
   compiles on both compilers, and takes the union of the canary list and
   GCC's warnings. If 10 functions or more remain, it splits them by handler
   family into up to four disjoint batches of uop definitions in
   `bytecodes.c` (attribute; call; operators and containers; iteration,
   generators, labels and `ceval.c` helpers). Each Sonnet worker gets its
   batch, the rule, the remedies (scratch, `TAKE`, slot-addressed parameter)
   and the one-object check, and returns a patch from its own worktree. The
   batches touch disjoint lines, so the builder applies them in turn,
   reruns the check and fixes the remainder. The expected remainder after
   step 2 is small, because the rewrite covers every `&name` declared in a
   handler, but the plan does not depend on that.

### 3.6 Rules for the record

- **Explicit access.** A handler reads `tstate->eval_activation` on the path
  that uses it, through `IR_SCRATCH` and `value_temps`. It passes pointers
  into the record to its helpers, as it passes `value_temps` today. No helper
  reads the thread state's record: a helper does not start an activation, so
  if it boxed into the record it found there, it would overwrite the live
  handler's boxes.
- **One per activation.** Every path that runs Python code (`__del__`, trace
  and monitoring callbacks, `gen.send` from C, PEP 523 hooks, `throw`)
  enters `_PyEval_EvalFrameDefault` and so pushes its own record. A
  suspending generator crosses a dispatch, so it carries no handler state.
- **Lifetime.** `owners`, `boxes` and `scratch` are dead at every dispatch,
  and debug builds assert an empty box count there. `entry` persists.
- **Never scanned.** Stale copies stay in the scratch. Counts keep the
  values alive, as they keep C locals alive today.
- **Stack switching.** The record is on the C stack, like the entry frame
  that `tstate->current_frame` reaches. Greenlet and its kin must save and
  restore `eval_activation` with `current_frame` and the borrow stack. The
  ports notes say so. A miss would corrupt another stack's record silently,
  so debug builds assert at record use that `act` lies between the stack
  pointer and `act->previous`, in the stack's growth order.
- **Aliasing.** Scratch fields have the same type as slots, so after a slot
  store the compiler reloads them. The hot copies (step 3) keep fast-path
  values out of the record.

### 3.7 Alternatives considered

Hot-path instructions are for the `LOAD_ATTR_SHAPE` shape (§2.2), Clang /
GCC.

| Home for an escaping local | Hot path | For | Against | Verdict |
|---|---:|---|---|---|
| A C local in the handler (HEAD's tail-call build) | 31 / 36 | no new structure; the frame dies at dispatch | a frame and a canary on every entry of 133 functions after A3.2 | rejected |
| Record via `tstate` | 23 / 28 | one mechanical generator change; no handler signature grows | one load on the paths that use it; greenlet must save a pointer | **chosen** |
| Record as a sixth parameter (`-DRECPARAM`) | 23 / 28 | no load from `tstate` | no shorter on the record path; Clang handlers with a C call get 2 longer (`close` 26, `outlined` 23); it takes GCC's last free register and would push `lastopcode` onto the stack | rejected |
| `noinline` helpers that take and return values | 21 / 28 | no record | hand restructuring of the 63 handlers whose transaction records span several helper calls (the packet's count holds) | rejected for A3; used where the generator already emits the call (step 3) |
| Tail-called slow functions | 19 / 18 | shortest on both; GCC moves nothing | the generator must split handler bodies at cold paths, which the DSL does not mark | open question Q5 |

## 4. Handlers, labels and exits (item 4)

| Kind | At HEAD (sites / functions) | Under tail calls | Rewrite |
|---|---|---|---|
| Handlers | 292 `TARGET`s; 289 after A3.1 | functions | generator |
| Labels | 5: `error`, `exception_unwind`, `exit_unwind`, `exit_unwind_notrace`, `start_frame`; and `UNKNOWN_OPCODE` from `ir_targets.h` | functions | generator; `TAIL_CALL_LOAD_FRAME` deleted |
| JIT labels | 4 in `ceval.c` (`jit_resume`, `jit_enter`, `jit_step`, `jit_consume_action`), computed goto only | – | deleted (A3.1) |
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
| JIT | 3 handlers; 9 sites in `bytecodes.c`; 3 emission points | – | deleted (A3.1) |
| `#if _Py_TAIL_CALL_INTERP` and `USE_COMPUTED_GOTOS` | 295 generated blocks; 27 lines in `ceval.c` (10), `ceval_macros.h` (11) and `bytecodes.c` (6), and 3 in the generated `ir_targets.h`; 11 in the generator | – | script and generator (A3c) |
| `IR_FIELD_*` | 1,550 generated lines; 2 emission points | `IR_*(ir_word)` | generator (A3c) |
| G9: canaries on the object | 200 functions; 133 after A3.2's helper fixes (§3.4) | record | 2 helpers by hand (A3.2); generator, then fan-out (A3.4) |

Ten edits are done by hand:

- in A3.2, the five `_Py_TAIL_CALL_INTERP` sites in `bytecodes.c`
  (`bytecodes.c:1476, 1866, 5670, 5694, 5729`). These are
  `assert(frame == &entry.frame)` twice, which becomes
  `frame == &act->entry.frame`, `assert(throwflag)` once, and two
  `int opcode` declarations;
- in A3.2, the two helper fixes (§3.4);
- in A3.4, the three slot-addressed handlers (§3.5).

Every other change is in a macro or in the generator, or comes from the
fan-out's work list.

## 5. Debug, stats and free threading (item 5)

- **Debug tail-call build** (`--with-pydebug`). At every dispatch
  `PRE_DISPATCH_GOTO` asserts:
  - `frame == tstate->current_frame`, which already exists and becomes a
    real check once `frame` is a parameter (A3.2);
  - A2's executing-copy invariant, `frame->ir_words ==
    _PyFrame_GetCode(frame)->_co_exec->words` (A3.2);
  - an empty box count, `act->owners.count == 0` (A3.4);
  - at record use, that `act` lies between the stack pointer and
    `act->previous` (A3.2, §3.6).

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
  and in A3.3 makes `configure` reject `--disable-gil` with a message, so no
  path is left that is neither built nor tested.

## 6. Steps (item 6)

Wall clock assumes this host (16 cores), where a full suite takes 3–11
minutes at `-j8` (the A5b-1 and box-site logs). It does not allow for
waiting on another agent's timing window.

| Step | Change | Done by | Test | Agent-h | Wall |
|---|---|---|---|---:|---:|
| A3.1 JIT hooks out | §1.1, on computed goto, with `sys._jit`. If the user agrees (Q1), a second commit deletes the fork's JIT sources | a deletion script for the marked blocks, then the compiler and linker: every remaining `_PyJit_*` reference is the work list. One Sonnet agent | release full suite: HEAD's failure set; debug targeted; `test_generated_cases` and `test_ir_contract`, which list handler numbers (302–304 gone); `test_sys` | 4 | 2 |
| A3.2 Tail-call build | include the handlers after the helpers (`irv2/tcpatch.py`'s edit); the §2.4 arms, MSVC first; `frame` added to `TAIL_CALL_PARAMS`/`ARGS`; the generator's frame load and `TAIL_CALL_LOAD_FRAME` deleted; the two entry calls in `_PyEval_EvalFrameLoop`; the record's persistent half, linked from `tstate`; the two helper fixes (§3.4); the debug asserts; the `configure` check and `PY_TAIL_CALL_CC`, regenerated; `libpython.py` walking `eval_activation` | the lead (Opus, 5 h) by hand: about 15 macro lines, 2 generator emission points, 5 `bytecodes.c` sites and 2 helpers, with compile errors on both compilers driving the rest. Beside it, one Sonnet agent does `libpython.py` and `configure` (2 h) | tail-call release on GCC 15.2 and Clang 21 (§2.6 flags), full suite each: the computed-goto failure set; debug tail-call (GCC) full suite; stats tail-call build and `test_sys`; `test_call` recursion, `test_perf_profiler`, `test_external_inspection`; `tcobj.sh`: at most the 133 canary functions on both compilers | 7 | 4 |
| A3.3 = A3c One backend | delete computed goto and the switch (`ceval_macros.h:152–188`, the `opcode_targets` table, the `ceval.c` paths); the `IR_FIELD_*` memory decode (`:204–240`); `IR_PREDICTED` labels; `--with-computed-gotos` and its `CFLAGS_CEVAL` flags; `--with-tail-call-interp` and the `_Py_TAIL_CALL_INTERP` macro, with every reader (`test_generated_cases`, `Doc/using/configure.rst`); `--disable-gil` rejected | a 40-line script that resolves the `#if`s with known values (`unifdef` is not installed), then the generator by hand (11 conditional emissions, `IR_FIELD_*` → `IR_*`). One Sonnet agent | release, debug and stats on both compilers (6 builds); full suites for both releases and the GCC debug build, targeted tests for the rest; the docs build | 3.5 | 2 |
| A3.4 Record and G9 rule | §3.2–§3.5 | the generator, in two parts on disjoint files: the lead (Opus, 4 h) changes `ir_generator.py`'s emission (record, generator-declared locals, union, hot copies); a second Opus agent (3 h) writes the declaration rewrite as its own module, with `test_generated_cases` cases for each rule. Then the object check and `-Werror` (0.5 h); the fan-out to up to four Sonnet workers (1 h each, §3.5 step 6); the builder's remainder and both suites (1.5 h) | full suite on both compilers; debug with the empty-box and range asserts; zero canaries, the frame bound and zero GCC `musttail` warnings on both compilers; recursion depth, profiler and inspection tests | 13 | 7 |
| Measurement (§7) | after A3.3, beside A3.4; and after A3.4. The sampler's classifier is widened beside A3.2 | one Sonnet agent | – | 2.5 | 1.5 |
| **Total** | | | | **30** | **16.5** |

- **Landing.** A3.1 lands on its own, on computed goto, after A5b. A3.2 and
  A3.3 land together, so `main` never carries two qualified backends; this
  is why the packet joined the steps. A3.4 lands on its own, after them, on
  one backend, so its effect is measured on its own.
- **Order against the packet.** The packet put A3c last. Here A3c comes
  before the record, so the record rewrite never has to fit the
  computed-goto shared box area or the `#if` arms in every handler.
- **Against the packet's 32 hours:** A3b's 9.5 hours go with the JIT, and
  the dual-CI cost (2 h) goes because no landing carries two backends. The
  record loses its JIT fields and its dual-backend form. Review 1's items
  (helper fixes, gdb, `sys._jit`, `configure`, sampler) and the fan-out add
  about 8 hours.

## 7. Expected effect and measurement (item 7)

These are hypotheses. Performance is recorded, not a gate. The baseline is
HEAD's computed-goto build: 40.7 instructions and 7.6 stack references per
word (59% set), against upstream's 22.0 and 1.3.

| Change | What it removes from the per-word path (static model) | Hypothesis |
|---|---|---|
| A3.2 tail calls, `frame` a parameter | Loop state leaves the computed-goto loop's C frame (3,304 B at `48bb4d6d1d3`) for argument registers. At `48bb4d6d1d3` (`tcvt` against `vt`, 59% set) this cut stack references from 8.6 to 1.5 per word at the same instruction count (45.5 against 45.4), with all 200 canaries in place. It removes the per-handler frame load and adds `endbr64` (1) and, under Clang, the frame-pointer triple (3) | stack references 7.6 → 1.5–2.5; instructions 40.7 ± 2 before the helper fixes |
| A3.2 helper fixes | a frame and a canary, about 8 instructions (§2.2), on the 67 functions with 41.8% of entries: about 3.3 per word | instructions about −3.3 per word. Time for A3.2 as a whole: 0.78–0.88× of HEAD's computed goto (the first link, with every canary in place, ran 0.870×) |
| A3c, one backend | the `IR_FIELD_*` re-reads (computed goto only); `CFLAGS_CEVAL`'s `-fno-gcse` and `-finline-max-stacksize` | small; size recorded |
| A3.4 record | the same cost on the other 133 functions, 14.9% of entries: about 1.2 per word; the box-array set-up | instructions −1 to −2 per word; time 0.97–0.99× of A3.3 |
| Compiler | GCC moves state around C calls: −1 to +7 per handler with a cold call, +11 with a hot call (about 10% of entries, so about +1.1 per word). Clang keeps a frame pointer (3) | Clang 21 at or below GCC 15.2 in instructions per word, and faster overall; to be measured |

By this model A3 removes about 4.5 of the 19-instruction excess over
upstream: 3.3 from the helper fixes and 1.2 from the record. Its larger
effect is on stack traffic, which the instruction count does not show, and
time is the measure of that. The rest of the excess has other owners:

- the slot-class test, 1.2 header loads per word (A4 or B3);
- borrow tags and publish canonicalisation (A5, representation step 8);
- `STORE_FAST` at 24 instructions and `LOAD_ATTR_SHAPE` at 85;
- `NOT_TAKEN` (B2).

Measurement plan, after A3.3 and after A3.4:

| Measurement | Tool | Note |
|---|---|---|
| Lower bound per word, 59% and 78% sets | `irv2/hmin.py` and `weighmin.py` on the tail-call `ceval.o`, GCC and Clang, with the remeasure-20261104b weights | under tail calls a handler's entry is its `_TAIL_CALL_*` symbol, not an `opcode_targets` relocation, so `labels.py` needs a symbol mode; the packet's `tc` figures (`labels_tc.json`, `hmin-tc.tsv`) show the path works |
| Canaries, frames, pushes per function | `tcobj.sh`, `tccanary.py`; GCC's `musttail` warnings | target in §3.5 |
| 29 kernels, interleaved | remeasure method: HEAD computed goto, tail call GCC, tail call Clang, upstream | U/C and against HEAD |
| Loop share of the gap | the LD_PRELOAD sampler | **its classifier keys on `_PyEval_EvalFrameLoop`. Under tail calls the loop is the `_TAIL_CALL_*` symbols, so the bucket must first be widened to count their self time.** Inclusive time differs between compilers, because GCC drops the frame pointer in small handlers |
| Entries that use the record, per kernel | stats build | from the generator's list of record users |
| C stack per nested evaluation | `sizeof(_PyEvalActivation)`, DWARF frames of the two eval functions, and `TestRecursion`'s depth | against the computed-goto loop frame and HEAD's tail-call build |
| `ceval.o` text | `size` | against 473,589 B (computed goto at `5de7bd62ef9`) |

Every result records the compiler version and `PY_TAIL_CALL_CC`.

## 8. Comparison (item 8, scope cut: brief)

| System | What it does | Taken | Rejected |
|---|---|---|---|
| Upstream CPython tail-calling interpreter (3.14; `3a440b6`) | Clang only: `#error` without `preserve_none` and `musttail`. Handlers are `Py_NO_INLINE` functions; at `3a440b6` the parameters are `frame, stack_pointer, tstate, next_instr, instruction_funcptr_table, oparg` (+ `lastopcode`). The table is a parameter so tracing can switch it. Labels are functions | handler and label as functions; `frame` as a parameter; `musttail` dispatch through a table | Clang only (A3 qualifies GCC); the table as a parameter (the fork switches none); `oparg` predecoded (A3 passes `ir_word`; Q4) |
| LuaJIT 2.1 interpreter (`vm_x64.dasc`) | handlers in DynASM assembly; `KBASE`, `PC` and `DISPATCH` in fixed callee-saved registers, so they survive C calls (`BASE` is refetched); `ins_NEXT` decodes A, D and OP before the jump; slow paths in shared `->vmeta_*` stubs | state in registers that survive C calls (Clang's r12–r15); slow paths out of line, where the generator already emits the call (§3.5 step 3; Q5) | hand assembly: a second handler language |
| V8 Ignition | handlers built with the CodeStubAssembler under a fixed interpreter calling convention (accumulator, bytecode offset, bytecode array, dispatch table); dispatch is a tail call into the next handler; the bytecode offset is saved only when calling out | the generator owns the convention; save the PC only at escapes (G9 item 1, after A3) | the accumulator; a second handler language |
| upb's tail-call protobuf parser (`musttail`; J. Haberman, "Parsing Protobuf at 2+GB/s", 2021) | as its author describes it: each field handler is a function with six register parameters that ends in `musttail`, and fallback paths are separate functions, so a fast path calls nothing and saves nothing | the probe's finding that a hot path with no C call is the cheapest form on both compilers (`tailslow`) | – (Q5). wasm3, by its design notes, relies on the optimiser to turn its calls into jumps; A3 requires `musttail`, which also holds at `-O0` |

## 9. Risks and open questions (item 9)

### 9.1 Risks

| # | Risk | Recommendation |
|---|---|---|
| R1 | GCC moves state on hot paths around C calls (§2.2): up to 7 instructions with a cold call, 11 with a hot one; hot-call handlers take about 10% of entries | Measure both compilers after A3.3 and A3.4 (Q2), and report the hot-call handlers separately. GCC's coming `preserve_none` removes it, with no source change |
| R2 | Both calling conventions are documented as unstable | Handlers stay `static`, and nothing exported uses the convention. The JIT stage enters through C-ABI functions (§1.2) |
| R3 | A miscompile under `musttail`, which is new in GCC 15's C front end | Full suites on both compilers; the debug frame and executing-copy asserts; `tcobj.sh` on both |
| R4 | gdb: `libpython.py` finds `EVALFRAME = '_PyEval_EvalFrameLoop'` and reads its `frame` variable (`get_pyop`, `:1885–1894`). Under tail calls that variable is the entry frame, stale after any inlined call. Reading `tstate->current_frame` would give every outer activation the innermost frame, and GCC builds most small handlers with no frame to read. `test_gdb` cannot run on this host | In A3.2, `libpython.py` reads each loop frame's `activation` local and walks `eval_activation`: Python frame k is `act[k−1].entry.frame.previous`, and the innermost is `tstate->current_frame`. Record gdb as unverified until it runs on a host with gdb |
| R5 | Profilers and the sampler see `_TAIL_CALL_*` symbols instead of one loop. A sample in a GCC handler without a frame pointer skips `_PyEval_EvalFrameLoop` | `test_perf_profiler`, `test_samply_profiler`; widen the sampler's classifier to `_TAIL_CALL_*` self time before measuring (§7) |
| R6 | Stack-switching extensions lose `eval_activation`, and a handler then writes another stack's record | ports notes; greenlet's port saves it with `current_frame`; the debug range assert (§3.6) |
| R7 | C stack per nested evaluation changes | `test_sys` and `test_call` recursion tests; record the size (§7) |
| R8 | A helper boxes into a live handler's record | the explicit-access rule; the empty-box assert at dispatch |
| R9 | Regenerating `configure` needs the project's autoconf | regenerate with the tree's documented script; if it cannot run here, the coordinator regenerates |
| R10 | The tail-call figures are old (`48bb4d6d1d3`) | A3.2's first commit records the static model and time at HEAD before anything else changes |
| R11 | The token rewrite misreads a declaration (a shadowed name, an unusual declarator) | the generator refuses ambiguous cases and names the handler; `test_generated_cases` covers each rule; `tcobj.sh` and GCC's warning catch what it misses |

### 9.2 Open questions

| # | Question | Recommendation |
|---|---|---|
| Q1 | Delete the fork's JIT source files, or keep them in the tree out of the build? HEAD keeps them. **A user decision**; review 1 agrees with the recommendation | **Delete them in A3.1**, in their own commit after the hook removal, named in the JIT-stage notes; history keeps them at `faddfd1454d`. Unbuilt files rot unseen, and greps and lints (`recount.py`, `check_value_api`) would keep reading them. If the source must stay, take it out of `Makefile.pre.in`, leave it frozen, and have no IR step edit it |
| Q2 | Which compiler builds the measured release after A3c? | Measure both after A3.3 and A3.4, weighting GCC's cost by the hot-call handlers' entries (R1). The faster becomes the measured build, and both stay qualified. The probes favour Clang 21 |
| Q3 | Keep the plain-`musttail` arm? | Keep it: one empty macro, no code path, not qualified, its cost stated (§2.5) |
| Q4 | Predecode a field (`oparg`, `dst`) into a parameter (packet open question 7)? | After A3.4, from `hmin.py` evidence. GCC's sixth register is free except under `Py_STATS` |
| Q5 | Tail-called slow paths (`tailslow`: 19 / 18 against 23 / 28)? | A G9 follow-up after A3.4. It needs the DSL or the contract to mark cold sections (error, deopt, release), and it is the natural successor to the record |
| Q6 | Frame pointers in handlers (3 instructions per handler under Clang)? | Keep them: `configure`'s default, needed by native profilers. Record the cost |
| Q7 | Merge `_PyEval_EvalFrameDefault` and `_PyEval_EvalFrameLoop`, putting the floor in the record? | No: no handler reads the floor, and gdb anchors on the loop's name |

## Files

Under `build/pyjit-link-wave1/a3/`:

| File | What |
|---|---|
| `recount.py` | handlers, labels, records, address-taken locals and exits, from the git blobs of a commit |
| `addr-36aa4a0345a.tsv` (= `addr-faddfd1454d.tsv`, `addr-a6002e5854b.tsv`) | one row per address-taken site in the source |
| `scratch.py` | per-handler scratch bytes |
| `kinds.py` | §3.4's kind table, from the TSV |
| `declcheck.py` | declaration shapes the A3.4 rewrite must handle (repeated names, several names per declaration) |
| `weights.py` | entry shares of record and escaping handlers, from the TSV and `pyjit/bench/results/remeasure-20261104b-C-kernels-boxes.json` |
| `probes/attrs.c` | attribute support per compiler |
| `probes/defaults.sh` | default hardening per compiler (canary, `endbr64`) |
| `probes/handler.c`, `handler.sh`, `fnstat.py`, `pivot.py` | §2.2's table (with `hotcall`), §2.3's listings and §3.7's `-DRECPARAM` column |
| `probes/tcobj.sh`, `tccanary.py` | §3.4's object list: HEAD's `ceval.c` as the tail-call build on both compilers, with `FIX=1` for A3.2's helper fixes; GCC's `musttail` warnings |
| `probes/optlevels.sh` | `musttail` at `-O0`, `-Og`, `-O2` |
| `probes/sizes.c`, `sizes.sh` | §3.1's sizes, from a `git archive` of the headers |
