# Grouped architecture review73

Date: 2026-11-16. Recommendation: implement the complete generator
producer/effect/consumer slice in
[execution-boundary73](../../design/proposals/execution-boundary73.md), starting
from selected57. Do not adopt rejected72 or call its remaining transactions a
completed implementation of the effect ABI. This is one grouped architecture
review; source owners and root own the subsequent implementation loop.

Root adoption amendment: both source peers subsequently approved the explicit
HOME completion consumer. Actual selected57 coroutine IR showed that the
RETURN/local/discard grammar excluded both `fibonacci` awaits: one result stays
live across another call, and the other feeds ADD. HOME publishes the necessary
live result through the same new retirement/effect engine. It adds no carried
arithmetic mode or old-helper fallback and receives no home-removal credit.
The design's source-adoption section records the evidence and contract; a new
two-await native control will measure the entire consuming expression.

## Evidence reviewed and verdict

The runtime source anchors are actual immutable trees:

* I: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`, `0dcba75bb3df40c32137db453b001f7e8f101de0`.
* G72: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-gc`, `c6d54fd3b5d53b89d47d1dbda30080e4ebdae529` during this review.
* E59: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59`.

Read canonical README/GOAL/STATUS/HANDOFF/PROCESS, design72, execution72-abi,
the report, retained-delegation68's contract, the final72 cold review and
source/evidence for the mechanisms below. Source inputs for73 are
`E59/execution73-compiler-proposal.md` and
`E59/execution73-runtime-proposal.md`; root's bounded diagnostic is
`E59/execution73-diagnostic`, with natural specialization coverage in its
`specialization-coverage.md`. Canonical runtime C source was not used as an
implementation anchor. This reviewer ran no target, generation, build, timing
or subagent work and changed only the two new73 documents.

Final source-contract GO applies to runtime memo SHA256
`d251c9cd06385fb384414d13ea47a191f9736dc50424cb39ffeb5db8794f872c`
and compiler memo
`be5c46408fc5da8b37ded7e2adb02a0da9a8152657172d657638307d56c99deb`.
The compiler peer independently confirms the eight-byte cold schema and
existing physical reader views. Root received the grouped implementation GO;
this does not select an executable.

The user's objection is supported by source. Design72 called for effects to
own publication, roots and continuation. Its executable instead added carried
placement to existing provider/retirement transactions, retained reserved
homes and initialization, and added universal continuation metadata and
ordinary kind work. Those compromises were qualified for behavior but were
not the intended complete simplification. The next implementation must deliver
the missing boundary rather than shrinking the requirement to the fragments
that already passed.

This is not evidence that reference counting, generator lifetime or Python
observation can be removed. It is evidence that their real work must be
separated from redundant transport and charged through the final consumer.

## What72 establishes, and what it does not

Both final builds pass117 modules/79 extensions. Cumulative focused coverage
is768 methods per role across19 modules, plus8 discarded-completion methods;
the final coroutine module passes100 per role. Six native controls, six owner
controls and native-entry fault handling pass. This is meaningful bounded
behavioral evidence, not one fresh full-suite run or complete dynamic branch
coverage. The accepted physical-sink and all-ingress corrections should be
preserved when reusing source concepts.

The protected screen nevertheless rejects72: hot8 GM1.067106/selected57,
Delta1.068427, logging1.210843, raytrace1.289980. Selected57 remains golden.
The historical STOP text correctly rejects that executable; the user's later
authorization permits this bounded diagnosis and design correction.

Whole-path native controls show the remaining architecture problem directly:

| Producer through actual consumer | Selected57 /72 instructions | Selected57 /72 maximum RSP decrease |
| --- | ---: | ---: |
| Arithmetic |223 /202|80 /80 B|
| Exact Python call |479 /474|80 /80 B|
| Generic native call |1764 /1821|1328 /1408 B|
| Delegated yield |245 /253|24 /40 B|
| Generator completion |769 /848|272 /368 B|
| Coroutine completion |721 /832|272 /368 B|

These are single reached paths from the frozen final native receipts, excluding
red-zone storage; they neither supply natural frequencies nor apportion timing.
The112 B frame header and all reserved homes remain. Executable headers grow
40→48 B and every word acquires a2 B continuation row. The final cold review
also records225 added native targets,592 mode rows at8 B and102 monitor rows
at6 B. Final release text grows236,982 B. Smaller stores and two improving
controls did not offset the complete transport costs.

## New diagnosis: separate actual work from cycle cost

Root's finite diagnostic has16 successful invocations, stable pre/post
identities and zero lost samples. Three matched counter pairs per workload
give these rejected72/selected57 geometric means:

| Workload | Cycles | Retired instructions | Cycles per instruction |
| --- | ---: | ---: | ---: |
| logging_silent |1.223496|0.975541|1.254171|
| raytrace |1.293510|1.202562|1.075629|

These counters belong to the pinned diagnostic protocol in
`execution73-diagnostic/{summary,results,identity-*}.json`. They are not a new
selection screen or a kernel-frequency census. Logging's loss is not an
increase in retired instructions. Its hardware mechanism remains unresolved;
neither code size, layout, dependency chains nor cache behavior is established
as the cause. The single user-cycle profiles have modest sample counts and
cannot convert relative symbol shares into a causal budget.

The untimed, unchanged-kernel adaptive snapshots provide a different concrete
finding. Across58 matched raytrace code objects, nine warmed ordinary handler
heads differ: six constructor-init sites in Point/Vector methods, one builtin
sqrt call in Vector.magnitude, a float ADD in Vector.dot and a float SUBTRACT
in Sphere.intersectionTime. The candidate's result placement lacks those
compatible specialized targets. Seven differences appear during the actual
run; Vector.__sub__ and scale are already warmed by module-import assertions.
These are reached specialization selections, not execution counts or proof
that they explain the entire20.3% instruction increase.

Across159 logging code objects and3946 sites, the same probe finds **zero**
ordinary-handler differences and no constructor-head loss. Do not transfer
the raytrace explanation to logging. Likewise, preserving CALL source in73
does not guarantee unchanged cycles after relinking; the sentinel screen
remains necessary.

## Source findings that determine the correction

**Specialization is a semantic execution capability, not a casualty of
placement.** G72 `Python/specialize.c:310` declines a requested target if no
placement counterpart exists. Its CALL output registry at
`Tools/cases_generator/ir_generator.py:210` covers generic/exact/general forms,
while `Python/bytecodes.c:4179` declares a much broader family. Constructor-init,
builtin and descriptor forms are missing, and arithmetic placement also omits
float/Unicode targets. The backoff is safe but imposes real Python-provider
work. A new placement must close over the complete source family before
activation, including deopt and observation. A runtime missing-body refusal
is not acceptable specialization preservation.

**Completed delegation still takes the old transaction.** G72
`Python/ceval.c:3800` decodes immutable parent words and calls
`complete_step_impl`. That helper at`:3616` adds a result root on the carried
route before any reached callback, then retains carried/materialized state
through release and observation. Its compiled frame is paid even when the
conditional callback body does not execute. The emitted RETURN and completion
paths at `ir_generator.py:5990–6058` return through that helper, status handling
and typed completion. An additional accumulator does not remove this boundary.

**Selected already supplies a usable retirement primitive.** I
`Python/frame.c:451` stops before a close that may reenter, leaving that owner
in its home. `:499` initializes the remaining-root description; `:527` finishes
that suffix, including frame-object takeover; `:565–624` applies it to
generators and exception state.73 should reuse these ownership semantics while
moving callback execution into a native effect entry that does not return
through the ordinary transfer. It must not add a second speculative lifetime
analysis or replay already completed closes.

**The existing semantic site is sufficient for outcome selection.** While a
child runs, the parent `instr_ptr` identifies SEND. Its `return_pc` already
means actual successor/redirect landing. I `Objects/frameobject.c:1490–1555`
compares those values with observer context; `:3714` captures them for generation
validation; `:3839` publishes debugger landing state. Repurposing `return_pc`
would add a control-state problem without removing a needed field.73 keeps
it unchanged and selects known yielded/completed native entries from the
current source handler before effects. After effects, original-site identity
alone grants no right to dispatch or retake a home.

**The three generator edges cannot collapse.** SEND yielded output is B+2,
completion is its actual A/exit, and the suspended generator's future input
belongs to its YIELD/RESUME edge. I `ir_generator.py:5621` and
`bytecodes.c:1788` already distinguish the real transitions.72's generic SEND
vacancy, CLEANUP_THROW join and physical-sink defects show why validation must
describe those actual physical outcomes. Suspended input does not exist yet.

**Finalization is not an exceptional benchmark category.** A freshly completed
child often has a unique receiver reference. Closing that reference reaches
generator deallocation. The corrected completed path must include this native
effect entry, its roots, actual deallocation and final consumer in its normal
budget. Winning a nonfinal yielding relay while leaving this path expensive
would miss the coroutine workload's synchronous completion behavior.

## Fixed recommendation and implementation checks

Adopt the concrete interface and order in design73. No further option ranking
is needed. The initial executable must contain the complete SEND_GEN → yielded
consumer → delegated suspension → real resume → SEND cycle **and completed
return**, including child and input retirement. Its source-generated target
tables provide outcome-specific consumers using the existing five-argument
ABI. They add neither a frame pointer nor a per-word continuation map.

The effect cut has exact ownership: result in the canonical payload; every
other still-live owner in a declared home until the private effect receives
it. Sequential close progress is encoded in its private target/cursor. The
effect immediately roots the result, detaches the remaining inputs before an
overlapping output publication, preserves PRE/POST and release order, and
dispatches the fresh actual materialized continuation after callbacks. It
never restarts the old completion transaction after partial progress.

This recommendation requires deletion of the old completion/helper nesting
from every admitted path. Calling that old helper on every completed return,
importing72's universal map, or declining a specialized body because its
consumer expects a carried value would fail the source contract before
performance selection. Ordinary, unadmitted public shapes and genuinely
different providers keep their complete semantics; their existence cannot
substitute for delivering the admitted path.

The new path stays inside the existing generator-return decision; ordinary
thread RETURN gains no boundary query or new kind predicate. No ACC_LIVE phase
is needed because an owner crosses only private transfer entries and its actual
consumer, never ordinary dispatch. Existing WAIT/suspended/uncommitted views
remain physical-home views; they do not claim the native result. Every effect
roots or materializes before reentry. Callback-driven completed chains pop
their current roots before tail transfer to the next retirement, so the new
mechanism does not accumulate a C effect frame per completed Python frame.

Source-ready implementation ownership:

1. Backend owns the shared inert payload/outcome interface, generated consumer
   and retirement entries, source effect/physical-owner facts and runtime
   readers. Compiler owner owns the shared cold source-family closure,
   all-ingress nomination and public-image admission. Shared headers/IDs get
   one sequential writer; no activation until both halves close.
2. Backend replaces all phases of the initial cycle, including finalizing
   completion, generic-family outcome delivery, native entry, throw joins and
   observer continuations. Compiler enables only the complete shape. CALL and
   arithmetic remain selected57; do not carry72's rejected specialization tax
   into this unit.
3. Root integrates, generates, builds both roles and verifies reached dynamic
   owners and effects. Run complete native controls, not handler-size samples.
   Include common unique receiver, multiple/aliased owners, local/discard,
   teardown callbacks, pending/applied/same-PC redirects, live/suspended
   instrumentation, native-entry failure, public malformed shapes and remote
   readers. Test claims must be matched to actual reached branches.
4. Use native results to correct a defective lowering while preserving the
   complete requirement. A predictor is not permission to abandon the effect
   ABI after a yield-only implementation. A losing integrated screen rejects
   that executable and permits bounded causal correction under the continuing
   user authorization; all protection and final selection gates remain.

The first native accounting must show all target/table bytes, target and word
loads, partial-release state, phase writes, tail transitions, actual stack and
red-zone use, and every callback/helper through the final consumer. Preserve
ordinary CALL/arithmetic mechanism controls and natural specialization heads.
Do not require each single path count to predict throughput perfectly, but do
require a concrete removed-work account for the complete initial slice.

Frame packing stays deferred because72's isolated allocator produced identical
extents. Reopen it only with a placement that reduces actual extent or
initialization and with all cold readers consuming that chosen layout. That
does not block logical/physical separation or this effect ABI.

The largest unresolved design risk is replacement of home traffic by table,
branch and native spill traffic. Source-generated entries and separate effect
frames make that risk measurable; they do not guarantee a speedup. Root's
unchanged broad guards decide selection. This review recommends completing the
architecture and measuring its real paths, without a parity claim or a promise
that it explains logging's cycle regression.
