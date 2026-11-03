# Next interpreter increment: value/effect-aware bounded regions

2026-11-01. **Execution proposal for review, not implementation authority.**
Recommend one complete comparison/branch region built around a reusable
compiler-to-generator materialization contract. Hold I1. Its global nine-bit
decode, initialization and validation change is substantial for two cleanup
consumers; the region increment instead tests whether logical values can cross
an internal operation boundary without a frame-home round trip or dispatch.
This begins a compiler/interpreter architecture, rather than making a global
encoding change the next step.

The detailed [region design](ir-region-hypothesis.md) and
[I1 design](operand-locality-design-20261101.md) remain frozen and held. This
proposal supplies the architectural implementation boundary and sequencing;
their unresolved workload-evidence and correctness obligations still apply.
No new survey, source/helper mutation, target, compiler or census was performed.

## One reusable boundary

Keep the current conversion, copy/store/truth passes, register names and frame
assignment. Add one internal **RegionPlan** boundary after those rewrites and
before `_PyRegCfg_Compact`, with a physical verification after compaction
(`regcfg.c:_PyRegCfg_Build`, immediately after `run_passes`):

```text
existing register rewrites
  → RegionPlan: original operations + internal values + effects + exits
  → existing compaction and _PyRegCfg_Verify
  → assembler selects a region entry; original words remain
  → generator carries admitted internal values; materializes every exit
```

A plan references the two original `_PyRegOp` indices, their input/output
virtual registers, internal single-use values, original word/exception/source
states and exit obligations. Reuse `_PyRegCfg_Liveness`, `nimplicit`, block
entry flags and location/handler metadata; do not infer exception liveness
from the explicit inputs alone. The plan is compile-time data owned/freed by
`_PyRegCfg`, not a new runtime per-PC map or frame representation.

`assemble.c:assign_handlers` consumes the verified nomination when choosing
the head form. Planning allocation failure propagates the ordinary compiler
error; no half-plan is published. The first planner admits exactly two
adjacent operations in one block, one
fresh internal boolean, the same source line and exception context, and a
forward conditional exit. Reject additional uses, canonical/live-out values,
stored-local results, windows, frame switches and intervening observation or
service boundaries. After compaction, reuse `_PyRegCfg_OpSlots` to verify the
actual pair and every slot alias. Retain a physical home for the boolean:
ordinary execution and legal direct branch entry still need it. This increment
claims fewer accesses, not smaller frames.

The generator adds a small region composition entry alongside
`generate_handler`. Reuse `write_uop`, `IREmitter`, `IRStack`, `SlotLayout` and
the existing local/temporary ownership operations. The new contract lets a
designated internal value remain a C value across the logical operation
boundary; it explicitly suppresses that value's intermediate home flush and
feeds it to the next operation. All other inputs keep their existing homes.
An exit flush uses the original logical operation's layout and current
ownership state, including NULLing consumed temporaries.

This is more than concatenating handlers: ordinary IRStack finalization
materializes each operation. Conversely it is much less than SSA or a JIT:
there are no phi nodes, new global allocator, native code cache or general
register residency policy. The reusable part is the proof and materialization
boundary. Another proven short predicate/branch or guarded arithmetic/test
sequence could use it without another frame ABI or encoding. Such consumers
require their own evidence and admission review; none is silently included now.

Effects must distinguish the generic operation from its admitted fast path.
Generic comparison can allocate and run Python. The first fast recipe admits
only the existing exact compact-integer guards and nonallocating comparison
and boolean branch. Reuse generator properties and call-effect checking;
unknown calls, allocation, escapes, eval-breaker service, frame switching or
observers inside the fast interval reject composition. Never mark the generic
family pure merely to make the region pass an audit.

## First consumer, conditional on actual workload evidence

Select **boolean COMPARE_OP followed by forward POP_JUMP_IF_FALSE**, provisionally
at the registered coroutines `fibonacci` site. The shared counter baseline must
first support relevant current comparison/branch activity, and bound current
get_ir/source attribution must identify the eligible site. Aggregate pairs
are an upper bound when multiple PCs contribute; specialized-entry misses and
same-word adaptive redispatch are not successful region executions. The saved
three synthetic pair occurrences and zero-call disassembly establish neither
registered frequency nor a performance primary. If attribution is insufficient,
hold this consumer rather than replace it with another after inspecting scores.

Reuse the existing two words and their feedback/targets. Assembly selects a
generic region head and one integer region specialization in the existing
sixteen-bit domain. Preserve the ordinary second word and all word offsets.
The existing comparison selector's uint8_t specialized-handler temporary must
be widened for a new full-domain ID; metadata, specialization and monitoring
restoration must retain the region form. This is a local form addition, not
I1's global bit reinterpretation. The new code validator must bound and verify
the actual pair; no crafted/truncated word may cause unchecked lookahead.

| Boundary | Required execution state |
|---|---|
| Region entry | Original operand homes, exception state and IP at comparison. All runtime guards precede consumption. |
| Successful internal edge | Exact comparison produces an immortal boolean in C. No allocation, call, collection or observer occurs; no boolean home write/read is needed. |
| Logical branch and exits | Retain branch IP publication and feedback/history. Consume original temporary inputs and leave the internal boolean home NULL, preserving aliases and borrowed locals. Dispatch to the original target/fallthrough. |
| Type/pair/epoch/observer/breaker rejection | Run the original materializing comparison and ordinary branch with untouched inputs. Enter a real ordinary body, not a loop redispatching the same region head. |
| Direct second-word entry | Ordinary branch reads its actual home; no assumption that the region ran. |

Fast admission also requires the executing second word to be exactly
POP_JUMP_IF_FALSE_BOOL; cold/adaptive or instrumented branches use ordinary
execution so their warmup and counters remain real. Reuse branch history and
NOT_TAKEN handling, rather than recreating those algorithms.

Keep both IP publications. Deny fast entry for pending breaker work, active
tracing/monitoring or instrumentation wrappers; use existing authorities,
not a new event protocol. Preserve ordinary callbacks and errors on rejection.
Do not move a RESUME/backedge/service check. Work arriving during the two
nonreentrant steps reaches the same next existing service point. Racy IP/frame
readers retain the existing contract; there is no new atomic-frame promise.
GC still sees canonical frame homes whenever collection/reentry is possible.
The C boolean does not become a generally unrooted object temporary.

These ordinary entries are semantic exits and debugger entry points within
one maintained interpreter, not permanent alternative runtime modes.

## Concrete implementation and decision sequence

1. **Independent design review, then one bounded host prototype.** Grant at
   most 45 minutes with 15/30-minute checkpoints to implement RegionPlan,
   physical validation and generator composition for this pair together.
   Preserve original logical ops, effects and observer states. Host fixtures
   must prove rejection of extra uses, exception-live values, local results,
   alternate entries, changed context and malformed pairs. If this needs a
   general optimizer, duplicated compare semantics or handwritten unwind,
   stop and report the missing work. Do not implement I1 alongside it.
2. **Exact-source independent review.** Freeze one source/capsule. Review the
   producer-to-consumer mapping, all exits and metadata restoration, including
   the runtime guards and handler-width boundary. Demand unchanged generation
   outside admitted forms and strict root/call-effect/linked audits. The shared
   census may select the consumer; it cannot accept its mechanism.
3. **Fresh finite native feedback.** Use existing build/gate/origin/cleanup
   protocols. Execute actual comparison relations, both branch outcomes,
   compact boundaries, equal/aliased inputs, subclass/reflected/generic truth
   fallback, allocation/error paths, callback local writes, monitoring toggles,
   f_lineno/direct-branch entry, branch history and malformed pairs. Retain
   stale-root, exception, generator/frame and affected-backend coverage. Native
   branch-following must prove the boolean round trip and second dispatch are
   absent on fast success, with both IP stores and every slow exit intact.
   Changed common-state traffic remains a real package cost.
4. **One elapsed decision, then qualification.** Compare unchanged parent,
   region candidate and the temporary source-only materializing-region control
   already specified in the frozen design. The control forces the same boolean
   home write/read/clear while retaining region admission and dispatch; native
   proof must show the intervention survived optimization. Its separate binary
   can change layout, so it is not a cycle-perfect causal subtraction. Freeze
   all 29 registered workloads, original setup/loops, two rounds/raw vectors,
   quiet resources, RSS and compile-cost scope before scores. Actual elapsed,
   variability and controls decide; instruction/text counts neither rescue a
   losing primary nor veto a winning one by themselves. Qualification follows
   the existing validation contract on one selected source; retire controls
   with the decision.

The last step reuses the frozen region proposal's finite 174 score
processes/870 serial durations plus 87 origin processes and 45-minute quiet cap;
these are proposals, not activation. Compilation and first execution need
separate resource accounting because the planner and materialization contract
also cost time and memory. Do not fold those costs into an unmeasured warmup.

The coordinator must schedule source review, fresh builds/behavior, elapsed
and full qualification explicitly. They cannot be assumed to fit an old
04:18:11 adoption deadline, and this proposal extends no grant. Source-level
planning can progress while the shared baseline is prepared; consumer
installation and timing remain gated on the actual evidence and review.

**Recommendation:** this region boundary plus one justified consumer is the
next interpreter implementation. Keep native-boundary conversion and GC death
work independently owned, but do not make regex or another tiny conversion
site stand in for the requested interpreter/GC architectural work.
