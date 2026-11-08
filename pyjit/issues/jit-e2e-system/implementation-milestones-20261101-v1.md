# Linked implementation milestones

One maintained system: interpreter → method baseline → optimizing traces, with
one owner, semantic helper/effect ledger, state-map format and tiering policy.
These are implementation-sized outcomes, not promises of completion in a fixed
number of minutes. Every future source/native grant remains explicit.

| Milestone / owner boundary | Concrete deliverable and hypothesis | Cheap evidence first; later evidence |
|---|---|---|
| M0: shared execution ABI / A+B | Specify tagged values, owned/borrowed/NULL homes, native activation, helper argument/result ownership, logical PC, exit reasons and code owner. One materializer handles pre-effect, post-effect and error resumes. Baseline and trace plans compile against this same contract. | Host contract/decoder checks and source review. Then a small native round-trip fixture with varying values, owned objects, error state and cache invalidation. Include destructor reentry; never fake native activity with an inspection API. |
| M1: usable method baseline / A, B reviews transition compatibility | Compile an ordinary method's CFG with integer/boxed operations, conditional/backedge and generic call paths. A counted loop calls a Python function, which can call C or invoke a dunder; normal return and an exception both resume correctly. Cold unsupported bytecode tiers down with exact state. | Differential results/side-effect order/traceback/ownership at every exit and forced helper/allocation failures. Inspect actual emitted calls, stack alignment, saved registers, PC-by-value and source maps. Measure cold compilation+startup and warm method elapsed on calls/loops/mixed work; no all29 gate per opcode. |
| M2: integrated method workload slice / A+B | Naturally compile and execute representative methods from existing call-heavy, loop-heavy and mixed-object programs. Record actual compiled coverage, helper frequency, exits, compile bytes/time and cache use outside scored paths. Optimize the largest baseline boundary costs first. | A fixed small matrix includes Richards-like object calls, a loop/call recurrence, unpack/list or dictionary work, and generator/coroutine fallback/resume. Inspect admission cost on cold/noncompiled functions. Repeat only for a changed hypothesis or invalid run, not score shopping. Broader regression sampling after meaningful coverage. |
| M3: first trace optimization / B, A supplies shared backend/exit owner | Record one hot loop with nonconstant inputs, a conditional and an exit. Build SSA with loop phis, types/guards, ordered effects, liveness, snapshots and hardware allocation. Exit into the same baseline frame. Hypothesis: removing repeated guards/boxing/home traffic over many iterations amortizes recording and compilation. | Differential loop/overflow/type-change/zero-iteration/error/side-exit checks; snapshot round trips and forced exit at each guard. Emission proves values stay in registers where planned. Compare whole mixed-loop elapsed, cold/hot breakeven, compile/memory cost and side-exit behavior; retain slow paths. |
| M4: calls and layered optimization / B+A | Specialize call targets and inline selected Python calls with virtual-frame snapshots. Add constant propagation, CSE, guard elimination, DCE rooted in snapshots/effects, range analysis and selective allocation/RC elimination. Reuse validated method helpers for unsupported cases. | Changing function/type/dict versions, recursion, exception traceback, finalizers, aliasing and post-call exits. Add one pass at a time with its own mechanism/cost claim; evaluate combinations on calls+loops+mixed programs, not spectral-only functions. Then run one broad distribution comparison for this integrated increment. |
| M5: production closure / shared runtime owner | Code-cache budget/eviction, active-code retirement, observer invalidation, OSR at canonical loop headers, publication, native unwind rules and ordinary RC support are complete for the declared platform. Generator suspension may first tier down; later native suspension must materialize the same state. | Early correctness already exercises all ownership/exit boundaries. Expensive full release/debug, selected ASan, extension, lifecycle, publication/unwind and broad workload/RSS checks run once the integrated path merits landing. Do not re-enable the withdrawn global StackRef checker or reopen tracing GC through this plan. |

OSR is an entry into a *validated state*, not an arbitrary branch to native
bytes. Start at canonical loop headers with an explicit entry map. Do not
require arbitrary-frame OSR or virtual-frame inlining to obtain M1, but reserve
their representation in the common exit/state-map contract so M3/M4 do not
replace the runtime again.

The minimum deterministic semantic matrix spans: varying integer recurrences;
branch direction changes; calls with actual default/keyword/exception paths;
object ownership and a destructor that calls Python; replaced call targets and
type/dict mutation; mixed int/float/subclass fallback; periodic checks and
signals; tracing/profile/instruction and line monitoring transitions; frame
inspection; yield/send/throw/close through exact fallback. These may grow within
shared existing test classes. They are not all workloads to run at every edit.

Maintain three separate ledgers:

1. **Coverage/correctness:** each operation and exit's actual behavior, supported
   helper/native/tier-down status, and platform limits. A test of a direct leaf
   does not prove normal method dispatch or trace recording.
2. **Mechanism/cost:** genuine automatic entries, coverage, exits and compile/cache
   cost in diagnostic runs; actual generated bytes/register use. Keep probes out
   of scored paths. Include entry, helper, materialization and invalidation cost.
3. **Program outcomes:** fixed relevant programs first; broad application
   distribution and RSS at integration/landing checkpoints. Explain repeated
   losses and regressions. No arbitrary text cap, hardware-counter prerequisite,
   adaptive extra samples, or per-family all29 referendum.

A can implement baseline/helper lowering while B builds recorder/SSA against a
small versioned contract fixture. Neither should fork ownership, state maps,
code cache or feedback. Resolve changes to those common interfaces together;
review effectful operations before adding optimizer transformations over them.
