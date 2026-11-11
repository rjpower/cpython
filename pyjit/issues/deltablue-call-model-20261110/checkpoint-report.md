# DeltaBlue call-model checkpoint

The selected checkpoint is **runtime 48, `5559f60093e`**: canonical class-value reads plus
owned classmethod forwarding. Both increments are committed and pushed. The
conditional leaf experiment is retained separately and rejected on performance.
DeltaBlue has improved substantially, but upstream parity remains open.

Six balanced pairs show DeltaBlue takes **26.1% less time than starting
checkpoint 46**, but **22.5% more than upstream**. Logging takes 12.8% less time than starting checkpoint 46
and 24.2% less than upstream. Every target pair agrees in direction. The unchanged
29-kernel screen gives geometric mean **1.179430/upstream**: the broad gap remains
about 18%, essentially flat against the earlier checkpoint 46 screen. These are separate
matched campaigns, not a causal decomposition or a broad parity claim.

| Elapsed selected runtime 48 / control | / starting checkpoint 46, six pairs | / upstream, six pairs |
| --- | ---: | ---: |
| DeltaBlue | 0.739181 | 1.225425 |
| Logging silent | 0.872129 | 0.758002 |

DeltaBlue's upstream pairs range 1.20560–1.23426; logging's 0.73262–0.77387.
[Full tables](benchmark-tables.md) retain paired values, screen results and
regressions. GCC 15.2, CPU 15, hash 0, JIT off and unchanged inputs are fixed;
identities remain stable throughout each campaign.

## What landed

1. **Canonical class reads.** Existing guarded class/MRO/metaclass lookup caches
   publish canonical inline or object values through one ownership contract.
   Scalar class attributes no longer depend on an object-only cache. Provider,
   descriptor precedence, version and mutation behavior remain explicit.
2. **Owned classmethod capture.** Eligible class receivers capture the descriptor's
   current exact Python function and actual accessed class directly into the
   existing `(callable, receiver-or-NULL, arguments)` window. Lookup precedes
   argument evaluation. Attribute retrieval still creates its ordinary bound
   value; custom metaclasses and unsupported providers use ordinary lookup.
3. **Strict effect facts.** Two audited raw readers are correctly described as
   nonescaping. Generator negatives still reject unknown readers and removal of
   either fact. There is no per-handler checker waiver.

The call/frame representation stays coherent: overlapping argument homes,
initialized reusable storage, one owned physical call window, and an explicit
return recipient/continuation. Ordinary exact calls already avoid argument
copies and full temporary sweeps. The next improvements must remove work that
actually exists. See the [grouped design](design-overview.md),
[IR](../../design/register-ir.md), [interpreter](../../design/interpreter.md)
and [frame/call contract](../../design/call-and-frame-lifetimes.md).

## What the broad review changed

Astra performed one [grouped architecture review](astra-architecture-review.md).
Its seven recommendations led to canonical class values, direct classmethod
pairs, a complete-operation leaf experiment, shared completion rules and
authoritative final-IR facts. A hot/cold frame-layout migration and property/native
adapter expansion were deferred. Sol handled component proposals, peer reviews,
debugging and cold reviews; Luna synchronized canonical designs. No repeated
Astra inner-loop review or second compiler was used.

The [48 cold review](cold-review48.md) found no source blocker or required
abstraction cleanup. Reviewers challenged the leaf experiment's duplicated
guards and incomplete source accounting before qualification. The corrected
source passed focused qualification, but timing rejected it. Source correctness
and removal of activations are insufficient performance criteria.

## Why the leaf experiment was rejected

Candidate 49 (`24d97fa8c3b`) elides 25,504 activations per unchanged DeltaBlue
iteration. Against frozen 48, however, it is 15.2% slower on DeltaBlue and
21.3% slower on logging. The full 29-kernel upstream screen is 1.19527; DeltaBlue is
1.40721 and logging 0.94489. Both controlled target pairs regress.

The replacement path is too general. It repeats checked call facts, decodes
immutable feedback locations on every read, evaluates three paths through a
generic helper, scans observer state and constructs a dynamic release ledger.
Matched DeltaBlue profiles locate 14.38% of flat samples in completion, 8.50% in
feedback lookup and 5.65% in path reading. Those roughly 2,000 samples establish
locations, not independent recoverable savings. The ordinary raw probes were
inlined in the measured binary.

Compiling away only the call-site leaf gate and invocation gives ratios
1.01954/1.08961/1.00025 against runtime 48 for DeltaBlue/logging/nbody. Much of the target
loss disappears; the ordinary logging residual remains unexplained. Do not
attribute that residual to helper execution or promise to recover it from an
instruction-count argument. See the [experiment report](integration49.md),
[performance audit](C-performance-audit49.md) and [cold review](cold-review49.md).

## Qualification and limits

Runtime 48 builds release/debug with 117 checked modules. Component tests and
the 306-unit value API gate pass; the [integration reports](integration47.md)
record exact coverage and the [classmethod report](integration48.md) retains
initial failures and corrected import fixtures. Both full diagnostics on
restored runtime 48 run 54,697 tests. Release/debug report
16/21 GDB display failures, nine debugger-jump errors per role and one debug
trace-format failure. Module names and counts match the established limits;
quiet full logs do not identify individual failure signatures. The eight stale
disassembly failures are absent. Stats builds 117 modules and passes 150 smoke
tests. These are failed diagnostic records with known limits, not all-green
suites. Display formatting remains separate from Python semantics and throughput.

The rejected 49 experiment separately passes 18 focused modules, 908 tests per
build and the 306-unit API gate. Debug conservatively declines elision, so its
success is fallback coverage. It received no full-suite promotion run after
its performance screen failed.

## Current profile and next work

The selected checkpoint 48 matched capture completes all four upstream/candidate invocations
with stable identity, correct build IDs and zero lost samples. Flat DeltaBlue
samples locate 13.53% in the exact Python CALL handler, 15.23% in the two shape-read
handlers, 9.61% in method lookup and 10.30% in RETURN plus frame clear/pop.
Logging places 25.36% in exact/general Python CALL handlers and 17.52% in RETURN
plus frame clear/pop. These are sample locations, not a causal decomposition or
a promise that all those costs can be removed. Attribute execution remains
substantial alongside setup/teardown; the profile supports whole-operation and
shared completion work rather than a dispatch-only explanation.

The reviewed [C2 design](C2-proposal.md) moves selection into CALL specialization,
caches immutable feedback units once and consumes checked-prefix facts.
Ordinary exact calls should carry no optional leaf gate. Its gain is unproved;
implement one bounded step, peer-review actual code, run focused checks and time
it directly against checkpoint 48 before expanding coverage.

The [completion review](C2-completion-review.md) and its
[independent peer review](C2-completion-peer-review.md) challenge the release
ledger itself. It is not required by Python semantics. A clean alternative must
publish a coherent completed caller before retiring detached owners through
callbacks. Existing observer logic can mistake a finalizer descendant for an
unfinished source CALL; current completion/refresh helpers do not establish that
boundary. Define one explicit completed-state authority and redirect/refresh
contract before removing the ledger. Do not fabricate a callee or preserve exact
CPython finalizer timing. Scalar unary refcount checks remain a bounded fallback,
not an assumed final architecture.

The [plan](plan.md) assigns Sol design/implementation and independent ownership
review, Luna/Terra mechanical synchronization, and coordinator integration and
measurement. Do not expand grammar, create function-specific recipes, migrate
the whole frame ABI or chase instruction layout to rescue the rejected model.

Evidence lives in `BASE/pyjit-link-wave1/deltablue-call-model-20261110`;
[HANDOFF](../../HANDOFF.md) gives source/build/control paths. Final records are
`delta48-final*`, `final48-all29-upstream`, `final48-targets-{baseline46,upstream}`,
`final48-perf-record` and `final48-qualification.json`. Historical golden 39 and
upstream controls remain unchanged. Repository documentation is authoritative;
Loom mirrors the goal, plan, status, report and designs.
