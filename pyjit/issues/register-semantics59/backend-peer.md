# Round60 step 1: independent backend source review

Direction GO with the implementation constraints below. This is a source review
of [interpreter-parity60.md](../../design/proposals/interpreter-parity60.md),
not implementation authorization or native qualification. The reference source
is selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0`, retained in the
immutable integrated worktree. No runtime source changed and no interpreter,
host test, generator, build or native analysis ran for this review.

## Closed conversion

One normal-ABI `run_handlers` function can replace the separate tail-called
functions on an experiment branch. Keep the outer `_PyEval_EvalFrameLoop` as
activation owner and debugger symbol; the frozen selected57 checkout/builds
remain the control. A permanent two-backend abstraction or fallback layer is
unnecessary.

The complete source boundary is:

- `Python/ceval_macros.h:45–174`: handler ABI, TARGET/LABEL, ordinary/error/global
  and predicted transfers; `:196–227`: dispatch, same-word redispatch and child
  entry; `:249`: next-word load.
- `Python/ceval.c:2404–2419`: once-per-activation normal-ABI entry shim;
  `:2529–2640`: outer activation, START/THROW entry, sentinel and cleanup.
- `Tools/cases_generator/ir_generator.py:7105` onward: target wrapper/local
  declarations; `:7381` onward: global labels; `:9001–9042`: prototypes,
  UNKNOWN_OPCODE and function-pointer table.
- Selected `Python/bytecodes.c:1445` onward: INTERPRETER_EXIT;
  `:5562–5613`: error/exit/throw/start labels and start-frame dispatch.
- Configure/Makefile backend identity and native/object preflight consumers;
  generated cases/target products and symbol-based diagnostics.

These line anchors identify selected57, not later experimental worktrees.

## Required control-flow and lifetime contracts

1. The label-address table and UNKNOWN_OPCODE pseudo-target must both live in
   the engine function. Each handler/global label enters a fresh braced scope;
   no target jumps past its declarations. Keep persistent engine state at
   function scope, but initialize each reached handler's `this_instr`, opcode,
   boxes, owner record and `value_temps` anew. Audit VLAs, alloca and any escaped
   local addresses before moving bodies. Do not hoist handler owner records
   into one implicitly shared lifetime.
2. Ordinary DISPATCH finishes bridges before loading the next word.
   JUMP_TO_LABEL retains bridge-failure processing and its exact error origin.
   PROCESS_ERROR_AT passes the current lexical `value_temps` before leaving
   its scope, then reloads frame/slots/next only for HANDLED. Global labels keep
   their existing NULL value record. Child entry must still clear a staged
   child if finishing parent bridges fails before the frame switch.
3. Predicted transfer sets `next_instr = this_instr` and preserves the operand
   word. DISPATCH_SAME_OPARG rereads the handler ID while preserving that word's
   operands. Neither can become an ordinary NEXTOPARG reload. Preserve the
   existing PRE_DISPATCH/LLTrace checks, instruction statistics and Py_STATS
   previous-opcode state, including its extra state parameter's replacement.
4. Replace `enter_handlers` with one normal-ABI call choosing START or THROW.
   Preserve activation installation/unlink, native sentinel, recursion checks
   and the separate early-exit path. INTERPRETER_EXIT and terminal error returns
   leave the engine once, then outer activation cleanup runs once. A goto must
   not bypass bridge/root cleanup or return directly past the outer owner.

## Identity and acceptance gates

Semantic words, formats, source contracts, ownership metadata and provider APIs
stay fixed. Generated wrapper/table products intentionally change; semantic
metadata should be byte-identical. Source proof analysis must not learn new
facts merely because two bodies now share a C function.

Record `computed_goto` truthfully in configure/build/native preflight. Retire
musttail/ncsr-only acceptance rules for this candidate rather than claiming
they passed; preserve sealed control checks and evidence. Use the same GCC
version, optimization/hardening flags, build mode, source/ELF/Lib identity,
affinity, hash seed and JIT-off controls. No alignment search or unreviewed flag
change is part of this experiment. Keep the outer GDB symbol, and update any
`_TAIL_CALL_*` symbol probes to a truthful new target map.

Native acceptance must inspect the whole engine's frame reservation/spills,
state moves, common dispatch tails, cold-path liveness, computed-goto tail
merging/locality and CET targets. The old entry shim is not per dispatch;
callee-save pushes/pops are already absent under the selected handler ABI;
ordinary dispatch does not have an established duplicate word load. Do not
credit those nonexistent savings. Cheap SHAPE/JUMP handlers remain negative
controls against uniform per-word savings.

Root owns full generation, both builds, owner/error/observer/bridge/entry and
remote-reader gates, and matched timing/PMU capture. Charge newly introduced
spills and sender/receiver work before interpreting native reductions. A losing
closed backend experiment is a valid result. Implementation awaits final Astra
review and root GO; the Invoke-only P branch/packet remains preserved.
