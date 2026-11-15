# Grouped architecture review68

Recommendation: independently adopt **one** bounded retained-delegation unit
from [design68](../../design/proposals/interpreter-parity68.md), subject to its
source-contract peer. No second source lane is justified by the inspected
remaining work. This is a proposal, not source adoption, qualification or
selection. Selected57 remains unchanged; comp/Delta/coro parity is unresolved.

Reviewed canonical `10533a2b2d411a62c26c409438733c6862827333`, the active plan,
selected architecture/IR/value/call/activation contracts and the closed trial
report. Followed applicable AGENTS and read the [CPython AI policy](https://devguide.python.org/getting-started/ai-tools/).
Actual runtime reads use I, not canonical's stale C source. No runtime source edits,
generation, build, target execution, benchmark, PMU or control mutation occurred.

## Evidence that changes the architecture choice

| Frozen candidate | Qualified mechanism | Measured rejection |
| --- | --- | --- |
| A67, integrated `ba4ea9dd9a9`, ELF `ecf5d401b09d…` | Complete METH_VALUE CALL→B0 retirement/poll→APPEND, actual natural/error/defusion/OOM controls | hot8 GM 1.026137293; comp .985867441 with mixed pairs; Delta 1.022084868; logging 1.210653142; spectral 1.053212755 |
| B67, integrated `65fa10014885`, ELF `78680c36c470…` | Actual unpromoted mortal self and promotion before exact caller replacement; closure qualifies | hot8 GM 1.114220219841174; comp 1.020893, Delta 1.141350, coro 1.027999, gen 1.032565, logging 1.430373, ray 1.061317, spectral 1.149823, nq 1.100358; all selection predicates false |
| Continuation66, `64348dd5c182` | Qualified private-await lifecycle and suspension | coro .626026, gen 1.014457, logging 1.076467; rejected despite GM .936871 |

The report and `E59/{callout67,anchored-self67}-source-host/root-screen-decision.json`
own 67's stable 32-row decisions. Final B native peer4 proves 20 selected symbols
byte/VMA/size-equal to anchor3, with only eight whole-text bytes changed in cold
nomination/literal references. This supports using that peer's resource ledger;
it does not carry old hit evidence across the corrected nomination.

A's complete nominal boundary grows 228→370 instructions, stack 72→216B and
text 11,584 B. B's method stack grows 8→136B, returns 24→40B, text 149,568 B, with
new ordinary effect/home/retirement work. These are structural negative outcomes
for the proposed abstractions. They neither quantify nor establish the cause of
the logging regressions. Fewer explicit dispatches or counted owners cannot
stand in for less complete work.

Round66's finite diagnostic remains instructions .992588/cycles 1.085426, full
counter coverage, no recorded loss. It rejects equal-CPI proportional-count
explanation only. No cache/frontend/layout cause, new profiler campaign or
padding/compiler search follows. All losing packets remain rejected.

## Selected-source contrasts

I=`BASE/worktrees/call55-integrated`, `0dcba75bb3df40c32137db453b001f7e8f101de0`;
U=`BASE/worktrees/upstream-3a440b6`, `3a440b65695615e9fd9912342b79cdd54fced0d2`.
BASE and immutable build paths remain in [HANDOFF](../../HANDOFF.md).

| Inspected location in I | Finding and consequence |
| --- | --- |
| `Python/bytecodes.c:_INIT_CALL_PY_EXACT_ARGS`, `_PUSH_FRAME`; `ceval.c:call_bind_known_python` | Exact overlap, one reservation/header and RESUME skipping already exist. Direct/default/keyword binding avoids the generic transaction on a hit. A binder redesign cannot claim all GENERAL calls or exact Delta traffic. |
| `pycore_interpframe.h:_PyFrame_SetReturnValue`; `ceval.c:clear_thread_frame`; `frame.c:try_clear_except_code` | Local/discard/scratch recipients and callback-selected control are real obligations. EMPTY already skips scratch, nonfinalizing clear is inline in the clearing loop, and only the reached remainder builds roots. Another return mode is not a fresh saving. |
| `ceval_macros.h:IR_VIEW/IR_CONSUME_INPUT/IR_TAKE`; generated method and SHAPE handlers | Local views already borrow; escaping ownership is explicit. Non-owning canonical bits in a frame would still require the full B67 consumer port. An immortal tag cannot represent a mortal borrow. |
| `ceval.c:call_bind_known_python`; `specialize.c:specialize_keyword_binding`; function versions | Cold keyword proofs are repeated on hits; immutable tuple identity could discharge some loops, but same-code closures may have different defaults. Any certificate must retain current-default validation and cannot own an unguarded cached pointer. |
| `bytecodes.c:SEND_GEN/_YIELD_VALUE/_YIELD_DELEGATED`; `ir_generator.py:enter_generator` | Each ordinary descent publishes input, links exception/frame state, accounts recursion and dispatches. Each ascent restores them and relays one owning result. Existing direct generator entry removes recursive evaluator calls, not depth-proportional intermediate work. |
| `Objects/genobject.c:gen_send_value[_ex]`, yieldfrom getter, traverse/clear/finalize | Raw state currently controls running rejection, exception handling, finalization and inspection. Retaining a chain requires replacing that authority for members, not merely caching the leaf pointer. Native entry asserts detached links after suspension; those consumers need explicit ports. |

Read-only disassembly of retained I ELF
`E55/comp57-dictionary-native/python` (SHA256
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`)
rules out the tentative release-helper lane. `_PyFrame_TryClearExceptCode`
at 0x3e9ef0 loads the mortal count once, compares above 1 at +0x6f, then decrements
and stores at +0x40/+0x43; no zero/deallocator branch remains on that path.
Raw f_locals/f_funcobj clears reuse their loaded count; the extra sign test
implements immortality, not a duplicated last-owner test. `clear_thread_frame`
at 0x34cdc0 likewise reuses the code count. A renamed consuming helper would
not establish further native removal. Existing native evidence, not C-text
appearance, therefore demotes option 5 before implementation.

## Independent assessment of the five choices

Design68 ranks retained delegation first: remove repeated transparent ancestors'
execution, preserve actual owners, and pay state/reader complexity and record
churn. Writer-controlled code ownership ranks second: a real pair exists, but
retirement can repay it. Logical value/use/effect lowering ranks third: no broad
complete consumer is demonstrated after compact/callout losses. Private-await
residence ranks fourth: no distinct new broad boundary after 65/66. Conditional
release ranks fifth: its apparent removal is already native selected behavior.

The bounded `E59/parity68-source-host/code-owner-contract.md` memo confirms
selected exact CALL really increments code at 0x38b027/2a and retirement
decrements it at 0x34ce00/03; GIL upstream independently owns code too. The actual
replacement is `func_set_code`, not a nonexistent `PyFunction_SetCode` API.
Writer repair could avoid B's universal effects, but must cover parked threads,
unlinked retiring roots, copy/takeover and real GC ownership. Selected retirement
releases function before its last code use; unconditional promotion repays the
pair. A safe loan end or retained lender needs a complete priced contract first.
No spare state or net saving is established; this is no second implementation GO.
Keyword certificates also lack hot4 reach: no Delta PY_GENERAL or hot logging
keyword visits. None of these options supplies a parity sum.

Retained monitored visits are semantic words assigned pre-monitor labels, not
specialized-hit frequencies: generators has 3,137,890 SEND_GEN, 2,937,892
YIELD_DELEGATED and 2,937,892 noninterrupting backedges. The unchanged kernel's
Tree.__iter__ delegates left, yields its own value, then delegates right. Many
leaves yield once. Thus merely caching a leaf between two resumes is insufficient:
whole-prefix rebuilding at each return recreates O(depth) work. Incremental
append/pop and retained ancestors are the essential new consumer contract.
Resume62 saved one counted instruction on a 29-instruction continuation and lost;
it is neither the mechanism nor a prerequisite here.

## Hard questions settled before source work

1. **Owners remain ordinary.** Parent receiver homes own children; frame code
   pins keep real sites alive. The record's pointers are non-owning. Clear/GC
   cannot visit them as duplicate edges. Any member's teardown invalidates the
   record before owner release; cyclic GC need not clear the root first.
   One yielded owner moves to the external recipient; no hidden per-level relay.
2. **State becomes explicit only for members.** Shared RUNNING/SUSPENDED state
   resolves intermediate logical state and real PC. Frame/exception links are
   retained internally and detach externally at suspension. Every raw reader
   either resolves it or receives ordinary materialized state. Cold readers may
   bounded-scan rows; hot append/pop/resume/yield derive tails without searching.
3. **Exposure stays cold but complete.** Existing frame aliases decline entry.
   Later frame/native exposure, writes, trace/profile/evaluator changes and
   error/throw/close materialize before observable mutation. Pure state getters
   can resolve; remote readers need a versioned schema, never a stale-state
   assumption. Active materialization must preserve actual recursion debt.
4. **Growth is real work.** One new generator-prefix pointer costs 8 B on every
   subtype; record/link arrays, growth, all generator entry/yield/return guards
   and native/remote access are charged. No ordinary Python CALL or generic
   effect hook is added. Lack of logging hits does not prove logging unchanged.
5. **No state replay.** Pop only a genuinely completed child and run its parent's
   SEND completion once. Unsupported ingress materializes at the reached point;
   allocation failure cannot restart a provider. First creation, ordinary leaf
   operations and actual exception handlers continue through selected code.

The source peer must reject any missing owner/state/PC consumer, O(depth)
steady-state validation or ordinary leaf-return rebuild. Numerical witnesses
at depths 2, 8, 32 require zero intermediate bodies and zero per-member updates
per steady resume/yield. One actual tree left→parent→right transition must retain
its prefix. These are mechanism tests, not dynamic instruction or speed budgets.
Exact output/error/observer, GC, aliases, recursion and allocation controls are
specified in design68, with root-owned builds/native qualification afterward.

## Handoff and remaining gap

Root adopts first; Sol-high then implements the single complete unit with an
independent source peer, root generation/products, focused two-role tests,
actual mechanism and complete native review. Preserve all frozen controls and
rejected branches. Do not implement a release helper or a second packet while
waiting for delegation closure. One qualified hot8 screen uses design67's exact
protections, carried unchanged in 68. Only a winner advances to finite confirmation,
explicit all29, supported full diagnostics and direct upstream comparison.

Selected all29 remains .995586/golden56 and 1.127137/U under older two-pair limits.
Matched selected/U instruction/cycle ratios remain comp 1.436416/1.333179,
Delta 1.361424/1.118423 and coro 1.307831/1.267788; gen 1.372523 is older elapsed.
There is no new upstream ratio. Comp's 30.38% own-instruction deficit-to-count-
parity is a scale check, not a removable budget. The sole unit directly targets
generators; comp, Delta and immediate coroutines still need distinct complete
work removal. A broad winner is an increment followed by another grouped
review, not completion of the user's four-workload parity goal.
