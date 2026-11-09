# Closing 38: cold architectural assessment

This is a fresh engine-design review of final39 (`4d666e1b482`, production
behavior from38). H40 is deferred: its minimal release gate passed but the
debug run asserted while instrumenting RESUME in the structural EH0 negative;
root reversed the H40 patch. It is not part of the final image. This review is
not independent source approval of code I authored,
and it does not qualify the unfinished owner draft. No native execution or
runtime changes were made for this review. The coordinator owns final results.

The central model is worth keeping: canonical owning homes, separate code and
function owners, explicit recipient/continuation, initialized reusable storage,
and commit-before-close transactions. They give the interpreter a credible
reentry and GC story without a second borrowed-frame ABI. The main architectural
debt is that different subsystems repeatedly rediscover the physical effects
of those same operations. More opcode-specific facts alone will not retire it.

V8's own [Sparkplug description](https://v8.dev/blog/sparkplug) distinguishes
interpreter virtual registers from machine registers and discusses retaining
interpreter-compatible frames. LuaJIT's primary
[bytecode definitions](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_bc.h)
and [frame definitions](https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lj_frame.h)
make operands, saved PCs and continuation frame types explicit. These are
comparative design examples, not evidence that V8 or LuaJIT endorses this
Python ABI or that their throughput transfers to a reference-counted runtime.
Reducing a word count or C branch count does not establish
less executed work or lower latency. Conversely, a valid ownership model need
not reproduce CPython's internal stack projections, formatting, or prompt
destructor timing. Tooling ports and exact disassembly/Pdb transcript matching
are not Python semantic gates or performance gates. Real debugger behavior,
exception identity, live bindings and valid source positions remain behavior.

## Five structural priorities

### 1. Make one emitted ownership-effect contract authoritative

Today `_PyIR_Outcome`, DefinesHome, the physical NULL query, observer outputs,
origin transfer and the generated bodies overlap without being interchangeable.
The CALL temporary-recipient restriction and sequential POP_TOP cleanup are
concrete examples: a format alone did not describe the actual final stores.

Derive a small normal/error transfer contract from the same ownership objects
used by the emitter. Keep physical NULL/value/unknown distinct from old-origin
availability and from future-read liveness. Opaque native/VM providers need an
explicit checked contract, not a guessed effect from their names. Consumers
then share the transfer rather than independently growing handler lists.

Benefit: fewer proof holes and fewer false refusals; constructor, debugger,
GC/retirement audits and future optimizers agree about actual homes. Risk:
creating a second semantic language that drifts from the emitter. Avoid that
by deriving facts at actual publication/consumption sites and failing closed
when a provider cannot supply one.

Gate: mutation tests must invalidate changed output/input mappings and aliases;
small encoded ownership-vector tests compare generated transfer with the cold
decoder across success/error edges. Retain mortal-owner/finalizer/GC controls.
Ordinary generated bodies and hot metadata footprint must stay unchanged unless
an individually measured change is intended.

### 2. Reduce continuation interpretation, keeping one return transaction

Normal call, generator completion, native return and pending redirect currently
arrive through several adapters. Their differences are real, but repeated
decoding and refreshing should not become the frame ABI. In particular, adding
an exit-kind field to every frame merely to save a completion decode is not
automatically a win.

Prototype one derived recipient/continuation description for the return that
is actually executing, based on the existing owned code and real saved PC.
Keep canonical publication, displaced-owner retirement, error ownership and
newer-redirect precedence in one implementation. Specialize proven ordinary
and SEND/FOR callers without copying that transaction or broadening its aliases.

Benefit: a smaller auditable completion path and possible call/resume savings.
Risk: C-held results, overlapping child storage and pending redirects are easy
to misclassify; code cloning can increase spills and text size. A new frame
field has an every-frame cost, while decode savings occur only on some exits.

Gate: ordinary and generator/native/local/temp/sink routes retain identical
exception and owner behavior. Compare summed compiled text and spills, then
matched elapsed and current counters on generator, coroutine, call and logging
kernels. Lower instructions without lower cycles is not a successful result.

### 3. Avoid constructing a temporary activation solely to copy a coroutine

RETURN_GENERATOR currently creates the heap object and moves an initialized
executing frame into it. That provides clear ownership, but creation-heavy
workloads may pay frame reservation/initialization and a full ownership copy
before any user body executes. This deserves a larger potential win experiment,
not endless tuning of the completion decoder.

For positively known generator/coroutine providers, explore binding directly
into a fully clearable heap frame, with the generic path as the unchanged
fallback. Publish the object only after its code/function/argument owners and
initialized homes are complete. Do not add a parallel binding semantics or
assume PEP523, defaults, keywords and monitoring are absent.

Benefit: remove redundant work at the source of many short-lived frames.
Risk: partial allocation/GC visibility, mutable function code, binder failures,
trampoline recipients and observation during construction. This is higher risk
than item2 and needs a separate source packet, not a hidden fast path.

Gate: creation-only and creation-plus-first-send kernels distinguish the gain
from suspension. Measure allocations, copied/initialized cells, elapsed and
counter totals; test every failure boundary and frame escape. Retain generic
provider, keyword/default, monitoring and recursion behavior. Reject a speedup
that moves incorrect work into the first send or leaks failed construction.

### 4. Separate reusable code proofs from stopped-frame validation

The debugger analysis repeatedly builds a sparse graph, origin/context rows,
NULL dataflow and backward demand lanes. H40 correctly makes the first-send
ingress explicit and excludes pc0 ordinary/source error recipes. That precision
should stay, but repeated request-time reconstruction is not a scalable long-term
substitute for a code-level proof object.

Keep the stopped view small and dynamic: exact phase/PC/generation, actual
handler capabilities, current binding/owner contents and callback revalidation.
Make immutable geometry, sparse adjacency and producer/dominance facts reusable,
derived lazily under a bounded code-owned cache or equivalent cold interface.
Do not cache runtime homes, exception objects or old monitoring authority.

Benefit: predictable interactive latency and less repeated proof code. Risk:
cache memory/lifetime, stale quickening capabilities and confusing a static
producer with a dynamic generation. An eager large per-code matrix would simply
move the cost to compilation and inflate every program.

The analogy is explicit reconstruction metadata: V8's primary
[deoptimization document](https://chromium.googlesource.com/v8/v8/+/HEAD/docs/runtime/deoptimization.md)
describes translating optimized state into interpreter frames. That does not
solve Python f_lineno, RC ownership or caught-context restoration. The useful
lesson is to describe the required state once and validate its concrete
availability, rather than infer it repeatedly from incidental instruction shape.

Gate: bounded-size scaling for first and repeated setters, large NameLimit-style
code, mutation/code.replace, instrumentation changes, allocation failures and
retained memory. Keep aggregate budgets and graceful refusal. Verify H40's
created-send, later-send and throw ingress separately; no all-resumes-fresh seed.

### 5. Spend proven destination knowledge at the lowering boundary

Lowering often knows local, temporary or sink destination and canonical source
provenance, but a generic final store reinterprets some of that information.
Use that knowledge only through one shared final-store implementation and
explicit derived facts. Do not grow handwritten copies of ownership bodies or
introduce raw-result canonical claims from handler names.

Benefit: possibly remove repeated normalization/destination decisions in hot
numeric and local paths. Risk: multiplying specialized handler bodies, deopt
and instrumentation mappings, instruction-cache pressure and alias mistakes.
Current measurements do not establish this as the cause of any remaining CPI
gap; this is a conditional source experiment, not an alignment prescription.

Gate: first demonstrate a material dynamic cost in current hot paths. A packet
must preserve canonical publication, local displacement, sink retirement,
specialization/deopt and observer contracts; account for total generated text.
Then use matched whole-workload timing and current counters. Abandon it if
extra variants merely trade branches for footprint without a repeatable win.

## What not to carry into this closing round

The unreviewed owner draft in N is follow-up material only. Its payload-token
and common-anchor direction needs actual-source review and native witnesses;
it is not part of the final39 conclusion. In particular, preserving a
scratch exception is not authority to rewrite a callback-owned handled link,
reuse a different manager, or synthesize a missing iterator/binding.

Broad qualification should report ordinary behavior separately from tooling
ports and optional safe-jump capabilities. Keep original positive cases or
explicit state-preserving refusals with protocol checks; do not use a greener
suite to justify a per-test policy. Performance evidence must name the exact
image and matched inputs. Old PMU observations are historical, not explanations
of a later image.
