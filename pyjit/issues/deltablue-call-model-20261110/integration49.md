# Conditional leaf activation elision: rejected performance candidate

Experimental runtime `24d97fa8c3b` integrates the complete leaf-region experiment on the
classmethod checkpoint `5559f60093e`. Its production implementation is
`656326dc375`; the follow-up admits common-constant roots and corrects the native
qualification fixtures. Magic remains 64418. Historical golden and upstream
controls remain unchanged.

## Structural change

A bounded certificate derived from complete final IR describes arguments,
constants, live globals, guarded attributes and an optional scalar conditional.
An admitted exact call computes the selected result without initializing,
pushing, executing or retiring a child activation. It uses the existing owned
call window and ordinary return recipient. There is no alternate call ABI or
per-function executable recipe.

Admission requires current ordinary providers, exact function/code/arity guards,
available stack and recursion budget, no observers or pending work, and a
callback-free complete operation. Before commit, aggregate reference counts
prove that retiring all actual aliases cannot finalize an object. A MISS keeps
the original call homes intact. Shared typed raw probes provide the same lookup
guards for ordinary and elided execution; the experiment does not duplicate the
ordinary guard prefixes.

The [proposal](C-implementation-proposal.md), [post-review](C-post-review.md),
[cold review](cold-review49.md) and canonical
[interpreter design](../../design/interpreter.md) specify the boundaries.
Debug, free-threaded, DTrace and Emscripten builds decline elision. Instrumented
code with allocated monitoring metadata also conservatively declines even after
events are disabled. General callback recovery, predicate composition and
arbitrary control flow remain outside this increment.

## Qualification

Both release and debug build 117 modules. Both pass 18 focused modules with
908 tests run; release skips 13, debug 36. Release separately passes all 19 new
leaf tests, including positive hit deltas on both comparison arms/senses, all
14 common constants, distinct live globals, alias multiplicity, final-owner
fallback and observation. Debug demonstrates fallback rather than optimized
hits. The value API check covers 306 units with zero failing findings or parse
errors.

Initial failed qualification records are retained. They exposed target fixtures
created without specialization versions, observer-test interference, and the
missing common-constant instruction. The fixes preserve strict hit assertions;
they do not relax version or observer admission. See the
[diagnosis](C-call-admission-diagnostic.md). Focused qualification passes, but the performance candidate is rejected below.
A full suite is deferred until a useful coherent candidate is selected.

## Performance decision

**Do not promote this candidate.** Two balanced pairs against frozen48 give
DeltaBlue 1.15213, logging silent 1.21281 and nbody 0.99705. Both DeltaBlue and
logging pairs are slower. The unchanged full29 upstream screen gives geometric
mean 1.19527, DeltaBlue 1.40721 and logging silent 0.94489. The previous48 screen
was 1.18014, 1.20915 and 0.75865 respectively; keep the campaigns separate.
The elision count is not a speedup.

A matched four-invocation profile has stable source/native/input identity,
correct executable build IDs and no lost samples. Flat DeltaBlue samples locate
14.38% in the completion helper, 8.50% in `_PyIR_InstrFeedback`, and 5.65% in the
path reader. These roughly 2,000 samples locate broad costs; they are not exact
causal cost shares. The generic transaction rechecks established call facts,
redecodes immutable feedback locations and builds a dynamic owner ledger.
The ordinary raw probes are inlined in the actual binary, so an imagined new
out-of-line raw lookup boundary does not explain the loss. Logging's common
callees are not certified; its regression must be separated from leaf execution.

One diagnostic ablation (`af60e5eb43e`) compiles away only the call-site leaf
summary gate and completion invocation, retaining shared ordinary probes and
certificate construction. Against frozen48 it gives DeltaBlue 1.01954, logging 1.08961 and nbody
1.00025. Most DeltaBlue loss disappears, while the logging residual remains
unexplained. It cannot be caused by a gate compiled away in that variant. No new compiler or call
ABI is introduced. The reviewed [C2 proposal](C2-proposal.md) selects eligible calls during
specialization, caches immutable feedback units and consumes checked-entry
facts. It remains future work; this round retains48. Avoid adding more generic
machinery to justify activation omission.

## Actual execution coverage

An unprofiled warm run followed by one unchanged DeltaBlue iteration records
25,504 elided activations. Of those, 25,023 are the two conditional input/output
accessors. Their probes decline 242 times and then execute ordinary Python;
there are no owner or entry misses in the scoped census. These are actual
before/after counters, not estimates from source shapes or time shares. The
semantic profiling census separately counts 51,422 calls per iteration; profiling
disables elision, so that census is not a simultaneous optimized denominator.

Evidence is under `BASE/pyjit-link-wave1/deltablue-call-model-20261110`:
`delta49*` and `delta49b*` build/check records, `coverage49-leaf.json`, and the
initial diagnostics, `measure49-call-pair48`, `measure49-all29-upstream` and
`perf49-record-affinity`. BASE and exact comparison paths are in
[HANDOFF](../../HANDOFF.md). The coordinator owns timings and final batch results.
