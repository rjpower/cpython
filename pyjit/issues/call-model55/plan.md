# Register call model: architecture and migration plan

Status: implementation authorized and active. Architecture research, source
audit and independent post-review are complete. Follow the
[execution ledger](implementation.md) for worktree ownership, integration and measurements.
The [review resolution](review-resolution.md) closes all five must-fix findings;
the [peer report](peer-review.md) accepts the design contract. The current task
implements M0–M7 and integrates candidates that earn admission into a new golden.

The user asks for a holistic allocation, calling and ownership model rather
than further adjustments within the existing model. Reconsider frame bitmaps,
caller/callee responsibilities, teardown, exceptions, observation and
suspension together. The [proposed design](../../design/proposals/register-call-model55.md)
owns the target contracts and diagrams; [status](../../STATUS.md) owns
the selected implementation and its limits. Proposed behavior must remain
clearly distinct from the as-built [frame ABI](../../design/call-and-frame-lifetimes.md).

## Baseline and question

Golden54 `61015a9452d9069646fb5fede9ff963896833b89` is the fixed baseline.
It adds only the cold public-constructor repair to golden51. The
[round54 report](../frame-state54/report.md) records five rejected performance
experiments, their revisions and compositions. Their results do not reject a
different ownership representation: removing one cache word or NULL store
within the old model does not test a complete replacement model.

Exact calls already overlap arguments, reserve and initialize once, and return
inline. Warm arena reuse avoids repeated full initialization. Resume already
dispatches within the evaluator. The new design must preserve these benefits
and identify additional work that disappears across the whole lifecycle.

The [source cost audit](../frame-state54/call-model55-cost-audit.md) traces
hot work and its authority. The review must distinguish Python behavior,
selected RC policy, implementation convenience and support for rare observers.
Exact CPython destruction order and numeric register stack order are not
language requirements.

## Work and ownership

1. **Broad research — Astra medium.** Compare complete alternatives using the
   actual source and primary V8/LuaJIT material. Define the recommended frame,
   call, ownership, exception and suspension ABI, with diagrams, cost changes,
   rejected alternatives and open decisions. Research typed execution authority,
   register-oriented IR and construction/specialization-time hints or caches.
2. **Independent source audit — Sol medium.** Trace normal/exceptional call
   flow, storage and root consumers. Identify which costs are structural and
   which the current code already avoids. Supply concrete obligations for
   physical ownership maps, overlap, callback publication and escaped frames.
3. **Independent design review — Sol medium.** Review the complete proposal
   against source and adversarial lifecycle examples. Look for hidden hot
   bookkeeping, incomplete root authority, ownership duplication, unbounded
   retirement, cold-state allocations on ordinary calls and accidental stack
   semantics. Report must-fix findings and correct them before closure.
4. **Coordinator integration.** Resolve review findings, separate selected
   decisions from experiments, and publish one coherent design and staged
   migration plan. Update the goal/status/navigation and Loom mirrors. The
   final explanation must show how the proposed model differs from the actual
   baseline and why its removed work can outweigh its added bookkeeping.

Astra's broad review is complete. Component review and corrections stay with
Sol high-effort agents and the coordinator. Root serializes generation, builds,
native tests, profiles and quiet measurements; workers own disjoint source edits.

## Required design decisions

- Define initialized/readable storage, observable local boundness, ownership
  and future-read liveness separately. Compare dynamic physical owner maps,
  static PC/phase maps and hybrids. A bitmap need not mirror every inline
  assignment; count its real updates and all root consumers.
- Specify the fixed hot header, per-code immutable plans, variable homes,
  optional observer state and suspended/escaped records. Preserve independent
  code/function lifetimes without assuming they require generic value decoding.
- Define caller argument ownership, call-clobber windows, callee bindings,
  result transfer, fallback before commit and retirement after commit. Register
  allocation must obey the call ABI without reproducing global operand-stack
  numbering or stack-pop destruction order.
- Put exception recovery and required context/finally effects in explicit
  edge/phase plans. Identify what must be published before callbacks versus
  work that can run only when an exception or observer actually occurs.
- Specify generator/coroutine construction, live-home persistence, resume
  inputs, suspension, close/throw and frame escape under the same model.
- Define validation of constructed/replaced IR, instrumentation invalidation,
  GC roots during unlinked retirement and C-owned results, and cold debugger
  reconstruction. Instruction hints are verified execution facts, not debug
  position information or unchecked promises.

The implementation plan must assign file/interface boundaries, dependencies,
focused tests, representation-migration gates, native work checks and unchanged
workload comparisons at each integration point. Full suites belong at coherent
representation checkpoints. State falsifiers and fallbacks without adding
permanent dual ABIs or invoking new permission requirements.

## Completion

The design checkpoint is closed: the proposal and independent review define
the full lifecycle and migration gates. The active implementation checkpoint
closes only when M0–M7 have evidenced dispositions, the complete static and
bitmap alternatives have been evaluated under the same ABI, and qualified
winners have been selected or rejected with retained evidence. Canonical
documents and Loom must agree. A partial source port or reviewed design does
not establish speedup, implemented bitmap authority or completed migration.

## Reviewed recommendation and implementation sequence

Use static scratch ownership maps with initialized named locals as the first
complete candidate. Compare a physical non-NULL presence bitmap under the same call
ABI and initialized-value policy before choosing the final representation.
Keep exact-call overlap, inline return, direct hot caches and the current
112-byte GIL header initially. Replace generic code/function values with typed
independent pins; move exception, observation and retirement descriptions into
verified per-code plans. This is a whole-lifecycle change, not a smaller-header
experiment. The proposal's M0–M7 table owns the exact schemas and admission gates.

| Integration | Deliverable | Gate before advancing |
| --- | --- | --- |
| M0 | Freeze transfer, site/phase, root and reader contracts; census selected54; compare shared storage, separate locals/scratch, clean ranges and explicit local validity before fixing placement interfaces | Count entry and exit work, cold initialization, argument packing, extra bases, local validity, publication and record costs; shared placement is a reference, not a register-machine requirement |
| M1 | Typed pins and common semantic-image validation | Focused code replacement, construction, marshal, hooks and failure tests; retain NULL homes |
| M2–M3 | Shadow ownership plans, nested collector roots and all lifecycle readers | Compare logical values and unique owners; callback, partial failure, suspension and escape coverage; retain NULL homes |
| M4 | One coherent switch to map-authoritative scratch and non-stack allocation | Complete root/reader review, focused plus full integration checks; remove scratch clearing and arena-value scanning together |
| M5 | Equivalent physical-bitmap branch | Same lowering/ABI/locals policy; measure full bit traffic including locals, copying, GC and teardown |
| M6 | Bounded optional lowering and locality improvements | One reviewed change at a time: result forms, constants, return moves, heap capture and cache layout |
| M7 | Select and publish a combined checkpoint | Native-path review, both-role qualification, retention checks, hot6/all 29 versus selected54 and upstream; preserve known/open diagnostic limits |

### Bitmap experiment refinement

The counted-only preparation builds and passes focused checks, but still pays
NULL maintenance and cannot authorize inline/control/no-count copy readers.
Its first hot screen loses every case. Root selects a same-byte single
**non-NULL occupancy** plane as the next complete rival direction, with exact
interface/source peer review before implementation. This replaces the bit's
meaning; it adds no second plane. Preserve the counted checkpoint/evidence.

A positive bit authorizes an initialized canonical non-NULL value; consumers
classify its tag. Zero never authorizes a load. Keep named bindings initialized
and clip physical bits to the actual parent/child/staged/retiring owner
partition. Presence alone cannot assign ownership between overlapping frames
or prove a protocol read. A typed HomeView variant and all copy/traverse/error/
diagnostic readers must compose before removing scratch NULL stores. The
[authority assessment](bitmap-authority-assessment.md) owns the concrete source
scope and costs. Added inline/control bit traffic is part of the comparison,
not a presumed improvement. Static M4 remains an independent full candidate.

### Delegation when implementation starts

Freeze shared interfaces before parallel edits. Each lane follows propose →
peer review → implement → focused test → post-review → integrate. Sol high
owns architecture-sensitive implementation and debugging; Terra/Luna medium
can port bounded readers and keep documentation current once interfaces are
fixed. Astra's broad review is complete and stays out of the inner loop.

- **IR and execution image:** regcfg allocation/verification, assembly, ir.c,
  code construction/replacement, marshal, metadata/case generation and
  disassembly. Own definite assignment, explicit transfer effects, call-clobber
  constraints and validation of every admitted image.
- **Frames and calls:** interpframe headers, ceval, Python/frame.c, genobject.c
  and pystate.c. Own overlap commit, typed pins, local initialization, caller
  handoff, retirement cursor/reservation and suspended storage.
- **GC and observation:** collector files, frameobject.c, debugger/remote
  readers and exposure/mutation adapters. Own the root census and nested
  transition interface. Request Python/frame.c changes through the frames
  owner; do not edit shared files concurrently.
- **Coordinator:** own merges, shared-header and generated-file integration,
  merged checks, native inspection and serialized measurements. Use a cold
  Sol review after each representation integration. Full suites run at coherent
  representation checkpoints, not for each mechanical reader port.

The hot set remains DeltaBlue, generators, coroutines, Richards,
comprehensions and silent logging. Use unchanged workloads and immutable
controls; preserve all 29 breadth at checkpoint selection. Static maps lose
if publication/decoding/code growth exceeds saved traffic; the bitmap loses
if bit maintenance/cache traffic exceeds its sparse teardown benefit. Retain
selected54 if neither complete implementation earns admission. Summarize what
changed, what was learned and current hot-set metrics at each integration point;
publish a final composition report when all stages have an evidenced disposition.
