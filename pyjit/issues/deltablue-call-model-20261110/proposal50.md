# DeltaBlue: preserve checked facts through execution

Status: grouped design approved with constraints in the
[Astra review](astra-proposal50-review.md); runtime implementation starts only
after this review. Selected control is runtime48
`5559f60093e8216d9399bea74d0f10c8cdfb9a6b`. This round reopens the broader
execution design at the user's request. Obtain one grouped Astra review before
implementation, then use independent component reviews and measured landings.

## Evidence and problem

Fresh matched FIFO captures are in `BASE/pyjit-link-wave1/deltablue-call-model-20261110/`:
`baseline50-perf-record` and `baseline50-perf-stat`. Both campaigns passed their
native/source/input identity checks; all four cycle captures have zero lost
samples, and all twelve counter intervals are valid. JIT off, GCC15.2, CPU15,
hash0, unchanged workloads; collection excludes setup/warmup.

Three normalized counter intervals per role give the following means per run
iteration (profiling is diagnostic, not the final balanced timing admission):

| DeltaBlue | Upstream | Selected48 | Ratio |
| --- | ---: | ---: | ---: |
| Native instructions | 38,936,924 | 55,343,778 | 1.42137 |
| User cycles | 15,868,451 | 18,680,197 | 1.17719 |
| Elapsed ms | 3.74456 | 4.42345 | 1.18130 |

Logging's corresponding ratios are 1.03636 instructions, 0.74143 cycles and
0.75037 elapsed. Historical six-pair DeltaBlue time was 1.225425/upstream;
do not attribute the between-campaign difference to a source change.

Selected48 DeltaBlue cycle self shares: exact CALL13.98%, shape-local10.70%,
method lookup7.83%, shape5.50%, RETURN4.32%, frame clear4.21%, frame pop2.30%.
These are sampled locations, not independent removable fractions. Upstream
uses one large evaluator symbol, so its symbol shares are not equivalent
operation attribution. Native annotations retain the actual instruction paths.

The [cache audit](cache-path-audit50.md) and [call audit](static-path-audit50.md)
establish that:

- Caches already belong to instructions. Hits index feedback directly and
  compare exact type version plus shape/validity; there is no hashed lookup.
- Method handlers repeatedly canonicalize a guarded descriptor and canonical
  receiver during conversion/publication. These checks survive compilation.
- Inline-values guards reload `tp_basicsize` through the receiver type even
  though a successful type-version guard already establishes its layout.
- Exact calls already overlap arguments. Repeated source code getters and
  several generic-looking retirement branches optimize away.
- Real frame initialization, code ownership and ordinary retirement remain.
  Two native retirement helper boundaries survive; upstream also has them.
- Rejected49's generic feedback decoding is absent from selected48. No new
  leaf evaluator, generic ownership ledger or per-call optional gate is proposed.

## General design principle

Specialization should produce a checked lookup plan whose outputs retain their
representation and ownership facts through the generator and into their next
consumer. Re-proving immutable layout or re-canonicalizing a canonical home is
unnecessary. Mutable type/shape/function/observer guards remain authoritative.
Keep the existing frame stack, call window, result recipient and language
semantics. Do not introduce a second cache, hash token or calling convention.

### A. Canonical guarded method capture and publication

Use the existing guarded canonical feedback word for method payloads after
nomination establishes descriptor semantics. Acquire it only under the existing
live type/layout/absence proof; preserve raw-pointer/guarded-value separation.
Teach the generator the narrow positive fact that these method outputs are
canonical. Publish function and receiver without numeric recanonicalization.
Apply the same contract to with-values/no-dict/lazy-dict method variants.

No cache growth or activation change. Audit every snapshot, polymorphic-row,
reset and debug consumer. Preserve acquire-before-retire and callback refresh.
Tests cover descriptor/type replacement, shadowing, dict materialization,
local/temporary receivers, mortal/immortal values and reentry. Native checks
must actually disappear before claiming an optimization.

### B. Cache addresses selected by the layout proof

At specialization retain the inline-values displacement in the existing
four-byte feedback padding. After matching the current nonzero type version,
use receiver plus that displacement to check shape/validity. Keep exact
shape/validity checks, field offsets and all mutation invalidation.

Prove that supported layout changes invalidate the type proof, reject offsets
that do not fit, preserve atomic field access, and update every payload copier.
Keep 48-byte ATTR/120-byte polymorphic geometry. This shortens a dependency
chain; its expected gain is modest, and it can be rejected independently.

### C. Carry method facts directly into an ordinary activation

The larger option is an adjacent, zero-explicit-argument method lookup followed
by exact CALL. A guarded pair can use one ordinary activation commit without
publishing/reloading a split function/self pair or rediscovering function kind.
The lookup and CALL still require their own mutable guards. No body inlining or
activation omission is necessary.

Choose compiler fusion versus quickened two-word execution only after reviewing
source/exception/control-flow targets, live handler changes and observation.
Every retained word must have meaningful semantics; no filler or executed
no-op. A miss before capture may execute ordinary lookup. A miss after target
capture must retain that target and use ordinary CALL, never redo a potentially
effectful lookup. Caller call-PC, error source, return-PC, tracing/monitoring and
observer-enabled entry must retain the two original semantic positions. An
intermediate control-flow entry must remain valid or prevent fusion. Entry
still owns function/code, uses overlap, recursion/stack admission and ordinary
return. First establish actual site/dynamic coverage; implement only if its
proof and code footprint are bounded and cleaner than existing composition.

### D. Ordinary frame retirement (alternative, not automatic extra scope)

A direct common thread-frame cleanup could move escaped-frame/generator/chunk
transitions cold and reduce two native boundaries. Preserve profile-cache
updates, code ownership, NULL-before-close, parent-current-before-callback and
callback-selected continuation. Upstream also pays these boundaries. Prefer C
if it removes more redundant work; do not rewrite frame layout merely because
the helper is visible in a profile.

## Review and execution decisions

The review selects **A then C** as the principal work. B remains an optional
separate measurement; D is deferred. C must preserve two ordinary live words,
start with thread-owned GIL callers and exact Python functions, and share the
ordinary exact activation implementation. Nomination proves adjacency, window
geometry, compatible source/exception regions and no unsupported interior
entry. Live neighbor and instrumentation checks precede capture. After capture,
any failed call admission publishes the two canonical owners and resumes CALL
at its own PC; lookup is never replayed. Temporary/local receiver ownership and
overlap must be explicit. Both word origins remain authoritative for observation
and errors. A supplies independent per-output canonical provenance, validated
from guarded-home duplication and unchanged canonical-input transfer; it is not
coupled to receiver retirement. These are implementation requirements, not
optional follow-ups. See the review for the complete guard/owner/test contract.

Component review removes source-position equality as an admission condition.
Line/column maps provide error and observation attribution; equality proves
neither execution safety nor ownership. Both real PCs and their complete source
positions survive. Use explicit control-flow, exception and observer conditions
instead of an identical-column or same-line proxy. The bounded first scope may
decline exception tables directly. The [component proposal](C50-implementation-proposal.md)
and [peer review](C50-peer-review.md) own this refinement of the grouped design.

Astra should challenge the ranking and identify missing broader opportunities,
not merely approve A/B. Specifically: is C the cleanest cross-instruction proof
contract, or would it duplicate machinery? Is B justified versus keeping layout
addressing uniform? Do A and C compose without ad hoc generator exemptions?
Which single larger lane has the best cost/complexity ratio after A?

After incorporating review, independent workers implement disjoint approved
components. The coordinator owns regeneration, real release compilation,
focused semantic/API checks, post-review and quiet matched benchmarks. Compare
each increment with frozen48 on DeltaBlue/logging/nbody (two balanced pairs),
then confirm a useful candidate with six-pair targets and a full29 upstream
screen. Run full release/debug diagnostics only at a useful batch checkpoint.
Promote/push only an effective reviewed candidate; retain rejected experimental
commits as evidence. Preserve golden/upstream and unrelated work. Update design,
plan/status/report and Loom with actual outcomes, including remaining losses.
