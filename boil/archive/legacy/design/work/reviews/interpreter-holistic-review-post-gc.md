# Interpreter review after GC source composition

Read-only source review, 2026-10-30. Inspected the composed native-cleanup
tree at `4d8e690f48334b0a5af26f50c3a41c32d75f1372`; its qualification is
pending. A bounded Git comparison finds the cited bytecode/compiler/value/
call/cases-generator paths byte-identical to integrated `75e8a1e0c01`.
The frame header additionally has the scanner's GC-range view; the frame
handoff helpers reviewed here retain their previous bodies.
The source review was committed on the selective topic as `39c1a956d43` and
copied into integration documentation without importing runtime changes.
It changes no serving or union source. No build, target, benchmark, generator
or profiler was run for this review.

The current register/value architecture is a useful foundation. Keep it and
remove demonstrated adaptation at its boundaries before introducing another
IR, type system or dispatch backend. The best small source candidate is
sharing an attribute guard's matched row with its load/store. Larger likely
opportunities are native value interfaces and producer facts, but their
current costs and benefit remain unmeasured. Roughly 2× for simple arithmetic
is guidance, not an opcode quota or a claimed result.

The existing matched29 result, upstream/fork `0.824299`, predates this union.
Selective paired-four's coroutine ratio `1.308649` and overlapping ranges for
the other three rows describe native `10d8773815c`, not this union or its
interpreter changes. The final selective debug-stress failure and warning
repair remain recorded in [the topic work record](../gc-selective-death.md).
Fresh post-landing measurements must supply updated performance conclusions.

## What these paths actually do

| Example | Current path | Work that is still present |
| --- | --- | --- |
| `i += 1` | The stack CFG becomes register CFG; eligible borrowed local loads and the final store fold into operands/destination. The assembler emits an inline small-int/constant load. Warm integer addition checks compact-exact operands, decodes, adds with overflow handling, and writes a canonical `PyValue`. | The constant still takes an instruction and the arithmetic operand still has a general compact-int guard. An int32 result allocates nothing; overflow boxes. Generated local variables are not evidence of machine-level moves. |
| `o.x + o.y` | The existing test specifies two attribute words, add and return. Shape admission checks owner shape/version and loads an existing `PyValue`. | The load does not box an int/float. Its filled-slot proof is distinct from the field's I32/F64 promise. The consumer checks its own operands. A generic fallback or property can return another kind. |
| `if i < n` | Flowgraph folding removes `TO_BOOL`; the specialized comparison produces a True/False singleton value, then a conditional branch consumes it and records branch history. | A bool guard and another dispatch remain. There is no boolean allocation to remove. Comparison results used as values or supplied by user `__lt__` need their full semantics. |
| `f(x)` | Exact Python calls guard function version/arity/space/recursion and enter a frame directly. Thread-owned callers overlap the callee's arguments with their call window; generator-owned callers copy to a separate frame. | Frame setup and publication remain. Ordinary exact Python calls already avoid an argument tuple and, on the overlapping path, argument copying. Native NOARGS still has object self/result; O/FAST conventions already use value arguments/results but object self. |
| `send` / `yield` | SEND_GEN admits exact gen/coro and execution state, links exception/frame state and enters it. YIELD publishes the value in the caller before unlinking the suspended frame. Entry skips an ordinary RESUME when breaker/instrumentation state allows. | Frame ownership, return slot, exception links, IP and initialized roots remain necessary. A sample on dispatch does not establish that these stores are redundant. |

Source anchors in the inspected tree:

- [Register pass driver](../../../../Python/regcfg.c), `run_passes:1417`;
  [passes](../../../../Python/regcfg_passes.c), copy propagation:237 and store
  elimination:369; [assembler](../../../../Python/assemble.c), inline loads:638.
- [Operation DSL](../../../../Python/bytecodes.c), integer guards/add:627–700,
  shape load:2631–2665, compare:3030–3100, branches:3304–3405,
  exact calls:4180–4270, SEND:1520–1607 and YIELD:1674–1718.
- [IR behavior tests](../../../../Lib/test/test_ir.py), attribute sum:770,
  folded destination:1037, compare/branch and monitoring:262–333, mixed
  resumers:234, exceptions from an entered property frame:152.
- [Value contract](../../../../Include/pyvalue.h), canonicalization:207;
  [compact ints](../../../../Include/internal/pycore_value.h):178–202;
  [shape promises](../../../../Include/internal/pycore_shape.h):48–58.

## Ranked prospective cuts

Ranking weighs proof cost and reuse as well as possible scope. It does not
rank measured savings; no new measurements were made.

### 1. Reuse admission results within one attribute handler

`_GUARD_SHAPE_POLY` calls `_PyFb_ShapeRow`, then `_LOAD_ATTR_SHAPE_POLY` and
`_STORE_ATTR_SHAPE_POLY` find it again (`bytecodes.c:2672–2707,2933–2963`).
The generated LOAD_ATTR_SHAPE_POLY body at
`generated_ir_cases.c.h:13193` really contains both calls. The lookup compares
up to four shape/version rows (`pycore_interpframe.h:60`). This is definite
source duplication, not yet definite duplicate machine work: the supported
GIL build's FT atomic wrappers are ordinary reads, so compiler commoning may
already remove it. Historical ce2ac samples at that helper are motivation,
not current cost attribution.

Smallest cut after qualification: inspect the matched release handler's
emitted lookup first. If both searches survive, have one shared admission
operation produce a temporary C row for the same word's load/store/method
body. Keep the exact version check, generic miss path and statistics. Retain
no row or borrowed feedback pointer across words or callbacks, and add no
registry. Existing shape polymorphism, class mutation,
generalization and method/data tests provide behavior controls. A bounded
existing Richards/attribute workload can decide whether the simplification
is material; reject the optimization if native code already shares the work.

### 2. Close one measured native value boundary coherently

The conversion is explicit, not conjectural: specialized descriptor NOARGS
calls `_PyValue_ObjectRooted` for the receiver and an object-result trampoline
(`bytecodes.c:4817`); generic descriptor and escaped PyCFunction routes have
the same convention (`Objects/descrobject.c:method_vectorcall_NOARGS`,
`Objects/methodobject.c:517`). `Include/methodobject.h:18–32` shows that O/FAST
already migrated their argument/result interface while self and NOARGS did
not. `PyValue_FromResult` then canonicalizes small numeric results. Raw box
allocation depends on the receiver, result and small-integer caches; a
conversion site alone is not an allocation count or a runtime share.

Smallest cut: use the existing box-site/CPU attribution after union landing
to select the already-audited finite numeric NOARGS receiver family. If it
matters, declare value-self/value-result in the existing native convention
and update generic, specialized, escaped method, Clinic and checker usages
together. Preserve wrong-receiver/keyword/error ordering, collecting-call
roots and raw roots when a callback genuinely needs a box. Keep bound-method
`m_self` storage and numeric getsets explicit separate boundaries. One
`bit_length` intrinsic would leave the architecture and other entry paths
inconsistent. See [the receiver audit](../pre-I-inline-receiver-audit.md).

### 3. Reuse producer facts, beginning with constant operands

The assembler already proves `LOAD_SMALL_INT_INLINE` and exact inline constant
loads (`assemble.c:638–647`). Integer arithmetic still runs both compact-int
guards and decoders (`generated_ir_cases.c.h:134`). That is a real check in
the generated path, but its native cost is unmeasured. A shape kind promises
the same representation until generalization renews its version; it is not
currently a consumer proof after arbitrary attribute fallback.

Smallest cut: first assess a local constant-input mode for the existing
arithmetic family, using register CFG definitions and the same DSL/effects
authority. Keep overflow, bool/subclass exclusion and generic semantics.
Only extend to attribute-produced facts after defining what happens when a
producer falls back. A specialized producer that merely deoptimizes to an
unconstrained result cannot license an unchecked consumer.

Do not introduce a second type lattice or assume fields are interchangeable:
the 64-bit word has four 12-bit fields, with assigned slot/immediate/feedback
roles, and current family fallbacks read those fields in their existing form.
An immediate-operand mode needs a compatible generic miss path. Same-line
regions, exceptional definitions, physical slot reuse, monitoring and legal
debugger entries bound the proof. Start with constant facts, not speculative
loop-wide int invariants or an int-specific LOAD_ATTR spelling. Compare the
existing integer/float/control workloads only after code size and generated
fallback correctness establish a viable cut.

### 4. Give compare/branch a shared result contract before considering fusion

`flowgraph.c:2674` already removes a redundant truth-conversion instruction.
The remaining comparison-result store, BOOL admission and branch dispatch
are visible in the DSL. Simplifying them may help, but neither their cost nor
safe elimination follows from the current source. Generic rich comparison
may run arbitrary Python; the condition form's truth conversion, branch
events and exception location are observable.

Smallest cut: check the post-union native profile and existing compare/branch
IR/event tests. If material, express guaranteed-bool output and a compatible
condition consumer in the existing family model, with all incoming paths
verified. Keep word boundaries initially. Fusing two words additionally
requires instruction/line events, f_lineno control transfers, branch history,
exception locations and H3 physical-versus-semantic counting to agree; it is
a separate, higher-risk cut. Do not fuse one benchmark's compare opcode just
to meet an elapsed target.

### 5. Audit frame transitions through the existing shared protocol

Python exact calls already use `_PyFrame_PushOverlapping`
(`pycore_interpframe.h:506`). SEND/return/yield already use return-slot helpers
and publish outgoing values before unlinking/clearing. Reusing these
contracts is simpler than adding another frame ABI. The current
`ceval_macros.h:127` reload of `tstate->current_frame` is tail-handler-only;
H1's isolated passed-frame experiment cannot explain ordinary GCC non-tail
frame work. RESUME skipping is also already implemented at `:624`.

Smallest cut: attribute allocation/initialization, transfer, clear and
dispatch separately in the post-union existing generator/coroutine profiles.
Then select one redundant adaptation, if any, across the existing exact
call/resume helper boundary. Preserve generator-owned versus thread-owned
storage, PEP523, recursion, exception links, escaped frames, instrumentation
and the early return-value root. No source evidence here licenses dropping
clear stores, live slots or IP publication. H2's inert partial decoder is
not a runtime liveness map; reopen it only for a measured reader/clearing
consumer, and reopen H1 only for an actual matched tail-backend decision.

## One operation model, with separate dimensions

The existing DSL, HandlerDomain and generated formats are the right authority
(`ir_generator.py:400–428,491–580`, `pycore_ir.h:12–144`). Parameterize only
admitted combinations and derive concrete handlers/monitoring/metadata from
them. Do not emit the Cartesian product or maintain a parallel effect table.

| Dimension | Concrete meaning and boundary |
| --- | --- |
| Semantics | Add/subtract/compare/load/store; in-place and descriptor semantics remain explicit. |
| Receiver admission | Exact numeric, exact container, shape/version, descriptor/property or generic. An observed receiver is not a static guarantee. |
| Stored representation | Frame/container/shape `PyValue` versus an object-only/native field. I32/F64/any-value shape kinds have one layout; string is a semantic kind without a current shape-storage promise. |
| Result guarantee | General value, exact inline I32/F64, bool or object result; state whether fallback preserves it. |
| Call interface | Value arguments/results, object self, NOARGS/O/FAST/METHOD/VARARGS, direct Python frame; escaped binding/storage is separate. |
| Observation/effects | Monitoring, tracing, feedback/NOFB, reentry, collection, exceptions, frame/slot reads and initialized outputs. “Pure” arithmetic can still allocate an overflow result. |

Existing `runs_code` and borrowed-input analysis already prevent folding a
local past reentrant code or reading it after a callback. Extend that shared
analysis when a concrete optimization needs a fact. Complete physical
liveness is more than handler membership: exception and frame transitions,
windows, aliases and final assembled words still matter. H3 already supplies
full-domain handler relationships; its domain size is not an optimization
budget. H1 and H2 remain isolated, not hidden parts of this source.

Two apparent complexities have simple explanations. Immutable code words
plus one mutable execution/feedback allocation (`ir.c:245`) separate marshal
identity from specialization/monitoring. Uniform feedback stride avoids an
extra address lookup; changing it needs actual memory/cache evidence. Keep
those choices until a measured consumer warrants their replacement. Also
check generated output before “fixing” comments: the stale list-store comment
at `bytecodes.c:1290` says it boxes the stored value, while the generated
STORE_SUBSCR_LIST_INT at `:17961` stores that `PyValue` without such a box.

The post-qualification sequence should choose one of these cuts from the
refreshed profiles, implement the shared contract, and run its finite behavior
controls plus affected coverage once. A later matched timing refresh supplies
the result. This review introduces no runtime change or measurement campaign.
