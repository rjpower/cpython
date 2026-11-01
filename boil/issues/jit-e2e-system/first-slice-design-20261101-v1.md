# First source cut: shared VM actions to whole native methods

Read-only implementation proposal. Root has prepared clean `build/worktrees/jit-method-baseline`, branch `boil/jit-method-baseline`, at exact `aa0c166ef399973bfd1ccd4010e50b84007c73d9` (accepted RC runtime plus prose). Use only that new worktree under a future source grant. Do not merge a4b0 or the18-path prefix experiments. Their source/images/evidence stay frozen. This cut implements M0 and a useful M1 consumer of [the system design](../../design/jit-e2e-system.md); it does not implement the tracing optimizer yet.

## First useful executable

```python
def bridge(i, cb):
    return abs(cb(i))

def work(n, cb):
    total = 0
    i = 0
    while i < n:
        if i & 1:
            total = total + bridge(i, cb)
        else:
            total = total - 1
        i = i + 1
    return total
```

Compile the COMPLETE CFGs of work and bridge, then call them normally with varying n/callbacks. The callback can remain interpreted, return varying integers, return an object whose __abs__ reenters Python through the real C builtin, or raise at a selected iteration. The same compiled method has normal return and error paths. No recognition of these functions belongs in compilation. Require actual native entry AND return to the compiled continuation after the Python/C call. Compile-success/active flags alone do not prove this milestone. The companion [validation contract](first-slice-validation-20261101-v1.md) specifies four small program families and retained interpreter regressions; do not inherit the old54-scalar roster.

## M0: one action consumer in the existing evaluator

Keep `_PyEval_EvalFrameDefault` as the initial VM trampoline. It already owns `_PyEntryFrame`, tstate/current_frame setup, recursion and external return. Add a narrow Action-consumption label there; do not implement a second frame engine or recurse through EvalFrameDefault for each Python call. `Python/jit_runtime.c` owns CodeUnits/lifetime/lookup, not another interpreter.

Define `JitContext` with real tstate/frame, code/exec generation, logical operation word, next word, action phase, optional prepared frame, active CodeUnit pin and typed transfer fields. `Action` includes Continue/Branch/EnterFrame/Return/Raise/Yield/Deopt. The first baseline uses canonical homes at every helper. No new per-frame operand-stack pointer: actual frames have base/saved_top.

Freeze these consumption rules before the source commit:

| Producer result | Existing consumer and ownership rule |
|---|---|
| Continue/Branch | Native caller chooses its compiled fallthrough/known target; no interpreter dispatch |
| EnterFrame: arguments transferred, start pending | Generic CALL has prepared a frame/window and saved caller return_slot. Publish it as current_frame once, then consume existing generated start_frame: `_Py_EnterRecursivePy`, DTRACE, IR_SKIP_RESUME once. Never do those again in jit_runtime |
| EnterFrame: start completed | If a helper emits `_PUSH_FRAME`, that body already updates current_frame, decrements recursion, LOAD_IP0/IR_SKIP_RESUME and DTRACE/LLTRACE. Mark typed phase JIT_ENTER_STARTED; consumer resolves dispatch/admission only, never start_frame or another RESUME skip |
| Return: return effects completed | Generated RETURN body has unlinked/cleared dying frame, left recursion, restored caller and committed result once through IR_STORE_RETURN. Consume already incremented post-call word. Resolve a FRESH valid caller CodeUnit+offset after callee return; never retain a raw old native continuation across callbacks/invalidation |
| Raise: error logging pending | Carry original raising-word separately from frame->instr_ptr; restore next_instr=raising_word+1 and goto existing error. Its traceback/monitor raise and exception_unwind execute once |
| Unwind already in progress | Preserve existing CURRENT_OFFSET=next_instr-1 rule, including RERAISE's separate frame IP. Existing exit_unwind unlinks/pops first and propagates at caller call-word via next_instr=caller->instr_ptr+1; do not replay the callee or call |
| Deopt before / after effect | Both carry explicit phase and exact continuation. Before resumes the pending operation; after resumes its successor with committed result/error. Never infer replay from a single status bit |

Use explicit enum phases JIT_ENTER_PREPARED versus JIT_ENTER_STARTED, JIT_RETURN_COMPLETED, JIT_ERROR_LOG_PENDING versus JIT_UNWIND_PENDING, and JIT_DEOPT_BEFORE/AFTER; never infer them from a caller flag. The initial pristine generic CALL uses the prepared route. A generic-call helper's frame transfer is precisely classified: it returns a prepared frame before the existing start_frame effects. A return helper uses the existing completed-return semantics. No producer/consumer pair repeats argument stealing, result ownership, frame clearing or recursion. The generator must reject an unclassified frame-changing path.

Interpreter→compiled admission goes at existing frame-start transitions; interpreted-callee return gets a generated resolver only AFTER return output flush/IR_STORE_RETURN. Add that return hook at the generator's `inst.properties.returns` epilogue, not DISPATCH globally. Exception handling can resolve a compiled handler after its existing home/error setup. These are whole-method tier transfers, with their entry cost explicit, not revived scalar-prefix filters. A sentinel/external return remains the existing INTERPRETER_EXIT path.

## Callable semantic helpers: actual shared generation

Add `Tools/cases_generator/jit_helpers_generator.py` as a consumer of analyzer AST, IREmitter, IRStack/Storage, field formats, borrow/boxing/window logic and generated labels. Emit `Python/generated_jit_helpers.c.h`; include it in `Python/ceval.c` in the same translation unit as current private exception/runtime helpers. This avoids exporting or hand-copying get_exception_handler and other static semantics. The new header defines callable helpers/descriptor table, not a second computed-goto interpreter loop.

Each adapter has `Action helper(JitContext*, const DecodedOp*)`. Reuse semantic uops; supply predecoded field operands and canonical frame homes. Replace dispatch/branch/frame-transfer/error macros through generator callbacks, not regex rewriting emitted C. Before a terminal action flush existing Storage exactly as IREmitter does. Error-no-pop preserves/drop-borrow behavior; local result publication precedes any last DECREF.

Adaptive `_SPECIALIZE_*` scaffolding needs an explicit helper-consumer rule: collect/update feedback through existing cold specializers, then continue the generic semantic body without entering interpreter dispatch or performing the Python operation twice. Keep this distinct from operation effects. If that rule or a raw goto/return/frame-switch cannot be lowered, classify it as a before-operation tier-down rather than guess. Tests must show one actual callback/effect through each route.

Initial adapters cover generic local/constant load-store/POP, BINARY_OP operations (ADD/SUBTRACT/AND at minimum), COMPARE_OP/TO_BOOL and conditional jumps, IR_JUMP/JUMP_FORWARD/JUMP_BACKWARD with periodic service, LOAD_GLOBAL and its NOFB form, positional CALL/window preparation, and RETURN_VALUE. Include the ordinary word0 RESUME entry convention. These are initial implementation coverage, not compiler eligibility names or benchmark filters. Every valid pristine handler gets a generated classification: direct lowering, callable helper, or precise tier-down. Missing adapters exit BEFORE that operation and remain interpreted for the rest of that activation; no one-instruction bouncing presented as native coverage. Subsequent generated adapter work closes the remaining families, exception-body operations, suspension, wide/window/NOFB cases. Whole-method generic coverage remains the target.

## M1 backend and ownership

`Python/jit_baseline.c` decodes validated pristine words into basic blocks/exception edges and a compact shared LIR: CallHelper, Branch, Jump, ServicePoint, Exit. It emits every reachable block and all normal branch/backedge destinations. Unavailable operations get a real before-op exit. No arithmetic-chain decoder or prefix Plan is imported.

`Python/jit_backend_x86.c` uses existing PRIMARY `jit_x86.h` labels/relocations, loads/moves, calls/jumps and pushes. Extend only missing ret/pop/indirect-call forms with encoding tests. Native ABI is SysV `Action entry(ctx)`; keep ctx in a callee-saved register, preserve all used callee-saved registers and16-byte call alignment. Each helper call embeds a stable DecodedOp pointer and returns to an actual native continuation; use absolute target loads/indirect calls if mmap is outside rel32 range. Branch helper results select statically compiled targets, not an opcode dispatch table. This fixed-home LIR is a real shared backend scaffold; general GP/FP interval allocation/SSA comes later.

CodeUnit owns RW→RX mapping, descriptors, word→native table, statepoint/owner metadata and active-use pin. No embedded untracked PyObject pointers: constants use owning-code indices. Use one optional exec-owned JitState; add clear/sizeof and native retirement in Python/ir.c. Detach/invalidate before reentrant release; do not unmap an active native caller while its callee or C callback runs. On every helper return, validate active generation before native continuation; invalidation exits AFTER the completed effect. Reacquire the caller unit at return. CodeUnit allocation failure leaves interpreter execution valid and does not consume a pending operation.

## Exact proposed source files and bounded commits

1. **M0 commit, proposed60-minute source cap:** `Include/internal/pycore_jit.h`; `Python/{ceval.c,ceval_macros.h,jit_runtime.c,bytecodes.c}`; `Tools/cases_generator/{jit_helpers_generator.py,ir_generator.py}`; generated `Python/{generated_jit_helpers.c.h,generated_ir_cases.c.h}`; `Include/internal/pycore_ir.h`, `Python/ir.c`; `Makefile.pre.in`; `Modules/_testinternalcapi.c`; `Lib/test/test_generated_cases.py` and new `Lib/test/test_jit_method.py`. Deliver real generated arithmetic/branch/CALL/RETURN helpers plus Action consumption through existing evaluator labels. Host generator/body/ownership tests and then a separately granted debug native semantic step test; not headers-only mocks. If CALL/RETURN/error phase closure is incomplete at cap, freeze partial source and report it.
2. **M1 commit, proposed90-minute source cap:** add `Python/{jit_baseline.c,jit_backend_x86.c}` and `Python/jit_x86.h` deltas, same owner/generator/test/build files. Deliver whole work/bridge execution and compiled continuation through the real evaluator. Cold internal compile/install fixture calls the SAME method compiler and normal admission path; it is not an alternate semantic runner. Automatic tier-budget policy follows on this substrate and is not claimed proven by forced compilation. Future source/native permissions and exact caps are root decisions.

First native exposure is the four-family validation matrix plus the selected existing ownership/frame tests, on one source/profile. Record emitted entry, native branch loop, actual helper return and exception PCs, result/effect ledgers and owner retirement. Cold/first/warm timing is a diagnostic of the component; no29-workload architecture-existence gate or inherited score threshold.

Reuse the earlier encoder **test ideas/artifacts**, not its18-file source merge: complete byte disassembly, checked labels/capacity, SysV callee-save/alignment tests, W→RX/failure-prefix/retirement checks and the pressure graph when the shared allocator exists. The old94/234 bytes, six-register Plan and twelve methods are historical; they do not validate this call-aware backend. Review any small encoding function port separately.

Real first-commit unknowns are generated raw control-flow lowering, exact prepared/start-completed phases, return output flush, exception current-word handling and conditional `_Py_JIT` publication/build wiring. Resolve them in M0's actual shared consumer before adding optimization passes. This is implementation work to schedule, not a reason for another architecture survey.
