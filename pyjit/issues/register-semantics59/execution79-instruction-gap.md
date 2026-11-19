# Execution79: where checkpoint78 executes more instructions than upstream

Analysis of checkpoint78 (`99a04733128`) against upstream `3a440b65695`,
2026-11-19. It changes no source. Evidence: `E59/execution79-call-ledger`.
The method is execution76's ([call ledger](execution76-call-ledger.md)):
exact per-call counts by perf-stat differencing, gdb single-step traces mapped
to handlers and source lines, and IBS op profiles for whole kernels. Kernel
figures convert IBS shares to instructions and are estimates (±10–25%).

## Whole kernels

Excess instructions per loop by category, as a percentage of upstream's total
instruction count:

| Category | DeltaBlue | raytrace | coroutines | deepcopy_memo | richards |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ours / upstream | 1.296 | 1.156 | 1.303 | 1.295 | 1.086 |
| Call, return, frame | +15.4% | +13.1% | +13.3% | +19.5% | +9.2% |
| Attribute and method lookup | +13.2% | +5.3% | −0.1% | 0.0% | −1.0% |
| Generators and coroutines | −0.4% | −0.4% | +12.2% | −0.4% | −0.5% |
| Locals, constants, moves | −5.0% | −4.5% | −1.5% | +3.2% | −5.7% |
| Everything else | +6.3% | +2.1% | +6.3% | +7.2% | +6.4% |
| **Total** | **+29.6%** | **+15.6%** | **+30.3%** | **+29.5%** | **+8.6%** |

Calls lead in every kernel. Locals and moves are the only category in which
the register IR saves work.

## Cause 1: the handler calling convention

GCC's `no_callee_saved_registers` passes the interpreter state (frame, slots,
thread state, instruction pointer, word) in argument registers, which every C
call clobbers. A handler that contains any call, even on a cold path, copies
the state into call-preserved registers on entry and back before dispatch.
Across 92 traced handler entries this costs 6.7 register shuffles and 1.9
stack adjustments per handler; upstream's evaluator keeps its state in
call-preserved registers for the whole function and executes 1.2 register
moves of any kind per bytecode instruction.

Dispatches per loop (from the share of IBS samples at handler `jmp`s,
calibrated against a trace to within 2%) times 6.7 shuffles:

| Kernel | Dispatches per loop | Shuffles, % of upstream's total |
| --- | ---: | ---: |
| DeltaBlue | 545K | 9.4% |
| raytrace | 31.8M | 7.3% |
| coroutines | 3.33M | 7.6% |
| deepcopy_memo | 4.2K | 8.1% |
| richards | 5.91M | 7.3% |

This one convention accounts for a quarter to a third of the gap in
DeltaBlue, coroutines and deepcopy_memo, half in raytrace and most of it in
richards. Clang's
`preserve_none` passes the state in call-preserved registers instead.
Compiled with clang-21, `LOAD_FAST_BORROW`'s fast path is 25 instructions
against GCC's 36, with no shuffles.

## Cause 2: the operand and result protocol

Specialized handlers execute the same guards and loads as upstream, but the
register form decides at run time what a stack machine fixes by position:

- whether each input is a borrowed local or a temporary to take (`IR_CONSUME_INPUT`);
- whether the destination is a local, a temporary or none, decided twice in
  some handlers (`IR_STORE_CANONICAL_TEMP_DST`, then `IR_STORE_DST`);
- whether a released input is a last owner that may re-enter;
- the `phase_state` store, the feedback side-table address (`_PyIR_FbEntry`,
  4) and the pointer-space test and untag of each object operand (about 5).

The micro costs explain DeltaBlue's attribute excess per execution:

| Handler | Ours | Upstream | Executions per loop | DeltaBlue excess |
| --- | ---: | ---: | ---: | ---: |
| Instance attribute (`LOAD_ATTR_SHAPE*` / `LOAD_ATTR_INSTANCE_VALUE`) | 69–83 | 39 | 95K | +2.9M |
| Class attribute (`LOAD_ATTR_CLASS`) | 89 | 37 | 27K | +1.5M |
| Method (`LOAD_ATTR_METHOD_WITH_VALUES`) | 51 | 30 | 46K | +1.1M |

In `LOAD_ATTR_CLASS` the guards cost 16 against 9 and the result 41 against
12: take the receiver from its temporary, store `phase_state`, decode the
destination twice, test the receiver for a re-entrant release, release it and
store the result. One register handler costs about what upstream's stack
sequence costs: `o.v` is 69 instructions against 67 for `LOAD_FAST_BORROW`,
`LOAD_ATTR_INSTANCE_VALUE` and `POP_TOP`. The IR executes about a third fewer
dispatches (92 against 141 in the traces), but each remaining handler spends
that saving on operand and destination decoding.

## Cause 3: the call path

Instructions per call, net of the loop:

| Case | Round 2 | Checkpoint78 | Upstream | Ratio |
| --- | ---: | ---: | ---: | ---: |
| `f(x, y)` | 455 | 430 | 343 | 1.25 |
| `o.m(x)` | 423 | 396 | 334 | 1.19 |
| `o.store(x)`, returns None | 540 | 495 | 403 | 1.23 |
| Default argument | 523 | 501 | 479 | 1.05 |
| Keyword argument | 760 | 733 | 558 | 1.31 |
| Class with a Python `__init__` | 1,633 | 1,589 | 1,202 | 1.32 |
| Callee whose local dies at return | 1,004 | 963 | 702 | 1.37 |
| Polymorphic call site | 860 | 794 | 541 | 1.47 |
| Native method on a list subclass | 837 | 765 | 463 | 1.65 |

Exact calls take fewer cycles than upstream's (`f(x, y)` 92 against 113);
the slow and generic shapes take more (polymorphic 285 against 194, native
424 against 305 for two calls).

**Exact calls.** For `f(x, y)` the call and return themselves are now close:
the call handler costs 156 against 139, and return with retirement 143
against 143 since round 3. The remaining 86 are the handlers around the call:
moving the two arguments into the call window costs 72 against upstream's 20
(each `LOAD_FAST_BORROW` is 36, a third of it the convention above), and
loading `f` costs 58 against 41.

**Generic and native calls.** A native method call that misses its
specialization runs `CALL` (81), method expansion (21),
`call_transaction_run_vector` (71), `call_transaction_native` (249) and four
`call_transaction_refresh` calls (92): 514 instructions of call machinery
around a callee of about 100. DeltaBlue makes 3,568 such calls per loop on
`OrderedCollection`, a list subclass that misses the exact-receiver guard in
both builds. Generic Python and native calls together cost DeltaBlue about
1.9M more per loop than upstream's (4.9% of upstream's total).

**Retirement when a local dies.** A callee whose local dies at return takes
the slow retirement path: 268 instructions against upstream's 87, excluding
the deallocation. Raytrace's `Vector` temporaries make this its largest call
item: return and retirement are about 183M per loop more than upstream's
(6.2%).

**Exact calls from generator frames.** A generator frame cannot overlap its
callee, so `call_push_moved_exact_args` and `call_transfer_exact_args` copy
the arguments. Coroutines spends about 24M per loop more than upstream on exact
calls (8.2%).

DeltaBlue's call excess (+15.4%) divides into the exact call handler (+4.8%),
return and retirement (+4.6%) and generic and native calls (+4.9%).

## Order of work

1. **Remove the shuffles.** Either make every handler's fast path call-free,
   with slow paths as tail calls to out-of-line continuations (GCC then keeps
   the state in the argument registers), or adopt Clang's `preserve_none`,
   which changes the standing single-GCC measurement configuration. A
   clang-21 attribution build of checkpoint78 would price the convention
   exactly before choosing.
2. **Tighten the operand and result protocol in the generator.** Decode each
   destination once with the temporary case straight-line, fold the receiver
   release, and keep the feedback address off the fast path. This applies to
   every handler, so it needs no per-handler work.
3. **Call-path items:** argument coalescing into the call window, a direct
   native-call arm with one refresh after the callee, list-subclass
   receivers, a leaner retirement for dying locals, and exact calls from
   generator frames.
4. **Hot-code layout last**, on the smaller and more uniform code.

Instructions per loop are exact and independent of code placement, so each
step can be attributed by instruction count without controlling layout; the
geomean of timed runs remains the throughput measure.
