**Verdict: amend, not accept.** The mechanism is sound and small enough, but two contracts in the frozen design are unsound or wasteful as written, and the stop rule throws away half of the only consumer. Fix those, then run the finite attempt after RC critical qualification.

## 1. Blockers, ranked

**Facts** are cited to the RC tree. Everything about speed is unknown.

1. **Observer guard is unsound (correctness, blocks start).** The design builds descriptors from immutable words and guards entry with version equality. In this tree, `_PyCode_HAS_INSTRUMENTATION` is defined as version greater than zero (`Include/internal/pycore_code.h:27`), and `_Py_Instrument` rewrites handlers in the executing copy only (`Python/instrumentation.c:88-91`). So a code object with line or call events installed passes the equality check while its executing copy runs instrumented handlers that the native prefix would skip. Smallest fix: invalidate or disable the descriptor from `set_word_opcode` or `_Py_Instrument` (`instrumentation.c:1996`), refuse to compile when the macro is true, and require `tstate->tracing == 0` as RESUME does (`Python/generated_ir_cases.c.h:16235`). Also refuse when `inline_values_off()` is true (`Python/ir.c:213-219`): the executing copy then runs boxing handlers and no int is inline.

2. **NULL live-in crashes the guard (correctness).** `PyStackRef_IsCompactInt` treats NULL as pointer space and dereferences it (`pycore_stackref.h:1115-1122`). Only `LOAD_FAST_BORROW` folds into a word (`Python/regcfg_passes.c:165`), so a folded local input is bound by construction. Admission must require that every local input is folded through `borrow_mask` (`ir.c:111-129`) and never a separate `LOAD_FAST_CHECK` word. State that as an admission rule.

3. **Per-word IP stores inside the leaf are dead work (design error).** The design keeps "existing per-word IP publication" and passes the frame address for it. Nothing can observe `frame->instr_ptr` during the leaf: no call, no error, no signal-driven Python runs. On rollback the interpreter dispatches at the unchanged `next_instr`. On commit the next handler publishes its own pointer, as every handler does (`generated_ir_cases.c.h:173`). Make the leaf a pure scalar function over an input and output scratch block with no frame argument. This is not "lazy publication"; it is the absence of any publication requirement, and it simplifies CFI and testing.

4. **The stop rule halves the consumer (scope).** The divisor of `//` is `LOAD_SMALL_INT 2` inside the same prefix (`Python/bytecodes.c:296-306`), so the zero and minus-one exits of `_BINARY_OP_FLOOR_DIVIDE_INT` (`bytecodes.c:746-767`) are statically excluded, and `+ i + 1` are two more checked adds. Seven int operations with one int live-out fit the same mechanism with one extra lowering rule. See question 4.

5. **Infrastructure facts the design understates.** `_Py_JIT` is never defined; `configure.ac` defines only the perf trampoline (`configure.ac:4171`), so `jit_publish.c` compiles to nothing. Both CFI flavours in `jit_unwind.c` describe other frames: the perf rows walk a push/mov/call/pop trampoline (`jit_unwind.c:549-565`), the GDB rule pins RBP (`jit_unwind.c:663-698`). `jit_x86.h` has no `ret`, no `pop`, no 32-bit store, and its register-immediate arithmetic forces REX.W (`jit_x86.h:319-332`). These are small additions, but list them in the 60-minute checkpoint.

**Assumptions to flag:** the eval_A word layout is inferred from the generator rules, not decoded. The design already requires a decoded witness first. Keep that.

## 2. IR boundary and traps

Local SSA over finalized homes is adequate here. In a straight line it is per-slot versioning; there are no joins and nothing to persist from the RegCFG. Do not keep the pre-compaction graph.

Avoidable work the design should not emit:

- **NULL stores for consumed temporaries.** Frame temporaries start NULL (`pycore_interpframe_structs.h:33-36`), and the prefix sits at function entry, so the shadow map should list live-outs only. Commit for the four-op prefix is two stores, the float constant bits and the product.
- **The duplicated `i+j`.** Trivially CSE-able in a straight line. Leave it on the table per the no-micro-optimisation rule, but record it.

Traps that are real:

- **Destination aliasing an input slot** is legal: handlers consume the input then store (`ceval_macros.h:625-664`). Versions handle it.
- **Owned temporaries cannot arise inside the prefix.** Constants are inline or borrowed immortal pointers, results are inline, and overflow exits. So "no DECREF in the interval" is a consequence, not just a rule.
- **A folded store to a local** releases the old value through `IR_STORE_DST` (`ceval_macros.h:652-664`). Declining unless the slot is NULL or inline is correct, and then a plain overwrite is equivalent.
- **Boxed compact live-ins** stay in their homes; extraction happens in the C wrapper. Correct as designed.

The next useful boundary is not loops and not global SSA. It is the same contract applied to any straight-line run of pure, inline-capable words that begins at a word with a feedback entry, keyed by word index rather than function entry. Loops need genuine joins and a decision about where int32 facts live; defer that.

## 3. Contract completeness

- **Entry.** Hook after `IR_SKIP_RESUME` in `_PUSH_FRAME` (`bytecodes.c:4386-4400`) and `start_frame` (`bytecodes.c:5929-5937`). Condition on `next_instr == words + prefix_start`. That single compare proves the RESUME was skipped, so the breaker is clear and the version current, and that the frame is at its first word. Reach the descriptor through the exec header as `IR_NLOCALSPLUS` does (`ceval_macros.h:556-557`), one load and no code-object touch. Exclude generator and coroutine flags at build time, not entry time.
- **Rollback.** Complete once blocker 3 is applied: nothing is written before success, so rollback is doing nothing.
- **Commit.** Write live-outs, set `next_instr` to the first unsupported word, dispatch. Destination temporaries are NULL at function entry, so the "writable home" check reduces to any folded local store.
- **Arithmetic.** 32-bit add, sub, imul with a jump-on-overflow exit reproduce the builtin overflow paths exactly (`bytecodes.c:676-731`). Encode results as `Py_INT_BASE | (uint32_t)` (`pycore_stackref.h:153-156`). Small ints come from the B field; inline constants read `co_consts` at compile time and carry `_PyStackRef_DoubleBits` (`bytecodes.c:277-290`).
- **Lifetime.** `_PyCode_ClearExec` is the only free site (`ir.c:366-377`); free the code owner there. Append fields to `_PyCodeExec` (`pycore_ir.h:152-159`) so `words = exec + 1` holds (`ir.c:296`).
- **Unwind.** Defer registration and CFI. A frameless leaf that never moves RSP is skipped by frame-pointer walkers and never hosts Python; faulthandler walks `tstate->current_frame`, not the C stack. CFI is a profiler nicety and the publication path is compiled out anyway. Cut it from the 90 minutes and add it after a measured win.

## 4. Throughput and the one alternative

No forecast. The structure is: the four-op prefix replaces six interpreter dispatches with one descriptor load and compare, two input guards, a direct call and return, and two stores. The extended prefix replaces ten dispatches for the same fixed cost. The universal tax is one load and compare on every Python call, which the Richards and coroutine controls will measure.

**Alternative:** extend the menu by one rule, floor division by a positive constant loaded in the same prefix, so the prefix covers all seven int operations of eval_A and leaves only the true division and return to the interpreter. Same consumer, same mechanism, same ownership contract. Stop rule: if the decoded words do not match that shape, fall back to the four-op prefix; do not add any other opcode.

## 5. Gates

| Gate | Content | Stop rule |
|---|---|---|
| G0, 10 min | Decode eval_A words with the `_PyCode_Instr*` readers; retain the dump | Shape mismatch stops the attempt |
| G1, 30 min | Admission, descriptor, both entry hooks, invalidation on instrument, with the leaf as a plain C function of the same scalar semantics | All correctness tests pass under the C leaf |
| G2, 60 min | Emitter additions, W to RX owner, C leaf replaced; retained disassembly | Same tests plus int32 extrema and every overflow exit |
| G3 | Two-round, five-value spectral_norm against the RC parent, with Richards, coroutines and unpack_sequence as controls; raw vectors, RSS, compile counts | Any primary loss rejects |
| G4 | Debug, ASan, refleak, broad suites; then CFI and registration | Only after a G3 win |

G1 isolates contract bugs from encoding bugs, which is where the time went in earlier attempts. Tests at G1 and G2: the operator-call oracle with compilation prohibited, the varying `step(x, y)` recurrence with a consumed checksum, settrace and monitoring toggled before and after compilation, generator exclusion, boxed and inline inputs, aliasing destinations, and compile OOM. A constant-result loop proves nothing and should not appear.
