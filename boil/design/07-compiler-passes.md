# 07: The compiler's register passes, and what Falcon's design contributes

Status: design, 2026-10-25. Extends note 06 sections 2.3 and 6 (step 3).
Falcon is rjpower/falcon (2013), a register-based Python 2 VM whose compiler
turned stack bytecode into register ops and ran a pass pipeline over them.

## 1. What the branch has today

Three layers, two of which are CPython's:

| Layer | Where | Form | Passes |
|---|---|---|---|
| CFG optimiser | `Python/flowgraph.c`, 4,200 lines | basic blocks of stack instructions, `basicblock` with predecessors, exception targets, line numbers | CPython's own: constant folding, jump threading, NOP and jump removal, cold-block layout, `optimize_load_fast` (a dataflow over an abstract stack of references that decides which loads may borrow), stack depth, line resolution |
| Slot assignment and folding | `Python/regalloc.c`, 290 lines (step 2a and 2b) | the flat instruction sequence after the CFG optimiser, with jump targets as indices | depth per instruction; `fold_loads` and `fold_store`, which rewrite the sequence in place: a `LOAD_FAST` that feeds the next word becomes that word's operand, a `STORE_FAST` after a producer becomes its dst |
| Encoder | `Python/assemble.c` | one 64-bit word per surviving instruction | none |

The CFG layer is a real pass framework: a `cfg_builder` holds blocks, each pass
is a function over it, and `optimize_cfg` and `_PyCfg_OptimizeCodeUnit` are
the drivers. It is the right place for anything that needs predecessors,
exception ranges or dataflow. `optimize_load_fast` shows it already supports a
dataflow analysis over the abstract stack.

The folding layer is not a framework. It is two peepholes over a flat
sequence, limited by design to a load that feeds the very next word, because
while the interpreter keeps a stack pointer every temporary must occupy its
canonical slot at any point where the stack can be observed (unwinding, frame
clear, generator traversal, `f_lineno` assignment). A load that stays pending
under another word would leave a stale canonical slot below the stack
pointer. Step 2b's own measurement puts the unfolded cases (a local under a
constant load, under a call or under `LOAD_GLOBAL`, and window consumers) at
6 percent of richards' dispatches and 11 percent of go's.

## 2. What Falcon did

Falcon's compiler (`src/falcon/optimizations.h`, `register_stack.h`,
`compiler_pass.h`) converted stack bytecode to register ops once, then ran
passes over a CFG of register ops:

- **Conversion.** A `RegisterStack` holds the register that each stack
  position currently names and a stack of `Frame {target, stack_pos,
  is_exc_handler}` for pending control-flow targets. Pushing a `LOAD_FAST`
  pushes the local's register; a producer pushes a fresh temporary. Every op
  ends up as `code, regs[inputs..., dest], has_dest`. Registers 0 to
  `num_consts + num_locals` are fixed; temporaries are numbered above them.
- **Pass framework.** `CompilerPass` with `visit_fn`, `visit_bb`, `visit_op`
  hooks and `dead` flags on blocks and ops; `BackwardPass` and a
  predecessor-ordered `SortedPass`; a `UseCounts` mixin. The driver
  `optimize()` runs a fixed list: `MarkEntries`, `FuseBasicBlocks`,
  `CopyPropagation`, `StoreElim`, `DeadCodeElim`, `LocalTypeSpecialization`,
  `DeadCodeElim`, `CompactRegisters`, `RenameRegisters`.
- **The two passes that matter here.** `CopyPropagation` keeps a per-block
  `env: reg -> reg` and rewrites every input through it, so a `LOAD_FAST` into
  a temporary becomes a rename. `StoreElim` keeps `env: reg -> defining op`;
  when a move's source has one use and a defining op in the block, the def's
  destination is retargeted to the move's target and the move dies. Together
  they are the general form of step 2b's two peepholes, and they are not
  limited to adjacent words because registers are not positional.
- **Compaction.** `CompactRegisters` (left commented out in Falcon) reuses a
  temporary's register when its use count reaches zero, so frame size is the
  maximum number of simultaneously live temporaries, not the stack depth.

Reading it also turns up the bugs to avoid, which are the standard ones:
`CopyPropagation` never kills an entry when its source register is
redefined (`y = x; x = x + 1; return y` reads the new `x`); `StoreElim`
records the dead move as the target's definition and does not check for an
intervening read or write of the target; the liveness analysis has no kill
set and no fixpoint; the backward walks skip the last op and the last block;
`is_pure` includes ops that can raise, so dead-code removal changes exception
behaviour.

## 3. What to take, and when

Falcon's architecture is the right one for this branch, and its precondition
is step 3. Once the frame has no stack pointer and a static size, a temporary
is just a slot; nothing is positional, nothing below a pointer can be stale,
and the unwinder, frame clear and generator traversal visit `co_nslots` slots
regardless of what the compiler put where. At that point:

1. **Convert once, at the end of the CFG optimiser.** A new pass turns each
   `basicblock` of stack instructions into a block of register ops
   (`handler, dst, in[3], imm, fb`) over virtual registers: locals, cells and
   frees keep their numbers; each push allocates a fresh virtual temporary.
   The abstract stack at a block entry is canonical (temporary k at depth k),
   as Falcon's `Frame` snapshots and the current depth pass both do, so
   predecessors agree without phi nodes. Exception handlers start from the
   depth the table records. This replaces `regalloc.c`'s depth pass.
2. **Passes over the register CFG**, each a function over the blocks in the
   style of `flowgraph.c`, run once in a fixed order by one driver:
   - copy propagation with a kill on redefinition (loads of locals become
     operands wherever the local is not written in between, including under
     constant loads and calls);
   - store elimination with the check that the target is neither read nor
     written between the producer and the store (a producer's dst becomes the
     local);
   - `COPY` and `SWAP` by renaming; `POP_TOP` and other dead pure results
     removed, where pure means cannot raise and has no side effect, a list
     the generator produces from the DSL rather than a hand-written one;
   - `TO_BOOL` and `UNARY_NOT` into the following conditional jump;
   - window consumers (`CALL`, `BUILD_*`, `UNPACK_SEQUENCE`) get their
     operands materialised into contiguous temporaries only when they are
     not already contiguous;
   - liveness, backward with kill sets and a fixpoint over the CFG;
   - register compaction by live ranges, so the frame holds the maximum
     number of live temporaries rather than the maximum stack depth; this
     also shrinks the words' slot numbers and the RSS that step 2a reported;
   - optional `CLEAR` of temporaries that die holding the last reference to
     a large value, where liveness shows it (note 06 section 2.3).
3. **Encode** from the register CFG. The encoder stops knowing anything
   about stack depth.

Not taken from Falcon: `LocalTypeSpecialization` and `TypeInference`, which
specialise statically from constant types; this branch specialises from
runtime feedback. `FuseBasicBlocks` is already done by `flowgraph.c`.

## 4. Effect on the step plan

- **Step 2b** stays as it is: the handler-side changes (fields read as
  borrowed, the `TOP` role, `RETURN_VALUE` as a register consumer) are
  permanent, and the two peepholes are the first version of the two passes.
  Its items 2 and 3 must carry the two checks Falcon lacked.
- **Step 3** grows from "frames and calls" to "frames, calls and the register
  CFG": the static frame first, then the conversion and the pass driver,
  then the general folding and compaction. Budget 5 days instead of 3. Its
  gate adds: the loads step 2b could not fold are gone from the Py_STATS
  mix; frame slots per code object fall (report the stdlib distribution
  against the 803 maximum); RSS against step 2b.
- **Tier 1** later consumes the same register CFG for SSA construction, which
  is the second reason to build it as a data structure rather than a
  rewrite of a flat sequence.

## 5. As built (step 3b)

Status: built, 2026-10-25, at 691c92537c6. Note 06 section 13 records
the decisions and the measurements.

**Layers.** Python/regalloc.c is gone. After `optimize_load_fast`, the
flowgraph converts its blocks into a `_PyRegCfg`:
- ops over virtual registers: locals keep their numbers, and each push
  makes a fresh register;
- blocks with their entry depths;
- regions: chains of blocks reached only by fallthrough, within which
  fresh registers may live.

At a region boundary every live value is canonical (temporary k at
depth k), so predecessors agree without phi nodes, as section 3
proposed. The driver (`run_passes` in Python/regcfg.c) runs the passes
once each, in a fixed order, with the verifier after each in debug
builds. Compaction then assigns slots, and the encoder in
Python/assemble.c reads the slots. `PYTHONREGCFGDUMP` prints the CFG
after conversion and after compaction in a debug build.

**The passes against section 3's list:**

| Proposed | Built |
|---|---|
| copy propagation with a kill on redefinition | `_PyRegPass_CopyProp`: borrowed loads only, same line, killed by a write or a line change |
| store elimination with the read and write check | `_PyRegPass_StoreElim`: also barred by any op that raises, escapes, jumps, has a dst or writes a local; calls included since 691c92537c6 |
| COPY and SWAP by renaming; dead pure results removed | `_PyRegPass_SwapRename`, `_PyRegPass_PopTop`; a COPY of a local becomes a second load in the flowgraph instead |
| TO_BOOL and UNARY_NOT into the jump | done by the flowgraph since step 2b; the value-context form is `_PyRegPass_TruthFold` (JUMP_IF_FALSE_OR_POP, JUMP_IF_TRUE_OR_POP), and UNARY_NOT takes any value |
| window operands materialised only when not contiguous | not needed: conversion leaves window operands where the stack put them, and loads into windows do not fold |
| liveness, backward with kill sets, to a fixpoint | `_PyRegCfg_Liveness`, used by the verifier |
| compaction by live ranges | `_PyRegCfg_Compact`, LIFO by holding rather than by liveness (note 06 section 13.3) |
| optional CLEAR | cut: consumers take their inputs, so no dead temporary holds a reference |

**What compaction bought.** Less than section 3 expected. Liveness
cannot drive slot reuse, because a value that no op takes, such as one
below a loop that never exits, must keep its slot. Windows also need
their operands consecutive at the top. Together these leave the
temporaries within 0.6% of the stack depth. The gains came from fewer
dispatches instead:
- richards: -12.5% against step 3a;
- go: -10.2%;
- per-loop instructions: -1.6% to -6.8%.

**Falcon's bugs, avoided:**
- copy propagation kills a pending load on any write to its local, and
  on a line change;
- store elimination checks reads and writes of the target, and every
  local write, between producer and store;
- liveness has kill sets and iterates to a fixpoint over all blocks,
  with exception edges;
- "pure" comes from the DSL annotation and excludes ops that can raise
  or escape;
- the backward walks include the first and last op of each region.

**For tier 1.** The register CFG is a data structure that tier 1 can
build SSA from, as section 4 intended. It costs compile time:
compiling every file of Lib takes 5.62 s against 4.86 s in step 3a
(+16%, gcc, best of three). The conversion, the passes and the slot
assignment each walk the ops, and debug builds also run the verifier,
with liveness, after every pass. Nothing has been tuned yet.
