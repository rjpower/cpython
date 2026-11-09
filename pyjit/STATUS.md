# Project status

The active [frame/call redesign](issues/frame-call-model-20261109/plan.md)
is integrated through engineering `3cde040a1afd` in the existing runtime
checkout. It is not fully qualified or promoted.
The frame stack, compact register IR and initialized arena now have terminal
observer output commits, activation-owned phase scopes and explicit saved-local
restoration. Genuine unbound bindings remain unbound; unsafe target reads refuse.
Committed jumps now provide an exact stopped-state view; canonical local loads
reuse their existing value format without redundant publication checks.
The [eighth report](issues/frame-call-model-20261109/eighth-integration-report.md)
owns current component evidence. The local
load change has independent source review, 117-module release/debug builds and
25 focused tests passing in each profile. Four component pairs give nbody
0.97077, logging 1.03461 and pickle 1.01118 versus retained417, using ratios of
round medians. Logging slows in all four pairs; no general win is claimed.

At `417a8bd6e2e3`, release/debug each pass 343 focused tests with one skip, five generator
methods and the selected twelve-test gate. Eventual ownership probes pass.
The compiler accepts 2,156 valid library files without new rejections.
The 999-test wide gate in each profile still fails four modules: monitoring
(one error), peepholer (three), sys_settrace (24, down from 38), pdb (two failures/two errors).
The old 21 diagnostic mismatches are repaired. The last full release diagnostic
remains 87: 54,504 tests and 13 failed modules. Final full suites, stats and
API checks remain open. Golden stays immutable and selected.

The clean `417a8bd6e2e3` two-pair logging/upstream screen is **0.86426× elapsed time**,
about 14% faster. A separate golden screen gives logging 0.61846, nbody 1.04803
and pickle 0.97892. Native31 still needs a direct upstream comparison.
Final parity campaigns and the unchanged broad screen remain open. Retained c142
warmed PMU counts show logging/upstream cycles 0.91821 and instructions 1.04497;
nbody/golden cycles 1.04088 and instructions 1.03331. Flat profiles locate method
lookup, calls/retirement and numeric/local paths; they do not assign causal
speedup budgets. See the [performance review](issues/frame-call-model-20261109/sixth-performance-review.md).

Next: complete the running full release diagnostic,
then shared committed-edge and typed NULL-requirement capabilities for the
remaining actual callback phases. All six each-LINE fixtures now pass without
changing expectations. Public line events do not independently establish PRE.
Retained protocol roots, caught exits, yield cancellation and complete observer
qualification remain work in the [repair plan](issues/frame-call-model-20261109/plan.md).
The five current design specifications are synchronized to `417a8bd6e2e3`; both previously changed
Mermaid diagrams render and have been inspected.

The previous [execution-boundary and operand-mode packet](issues/execution-boundaries-20261108/plan.md)
implements the first three bounded cold-review recommendations at runtime
`4b3d739d867`. Release and debug each passed 54,421 tests, debug on retry after
an unexplained profiling failure. **Performance promotion is withheld:**
logging is flat versus golden and several broad regressions repeat. The
[report](issues/execution-boundaries-20261108/report.md) records the candidate;
golden remains selected.
Python does not require CPython's prompt
finalizer timing; the [lifetime policy](design/lifetimes-and-language-semantics.md)
separates language requirements, compatibility choices and runtime safety.

The 2026-11-08 interpreter checkpoint is **golden**: frozen, qualified and
published at `pyjit-golden-20261108-runtime` (`0765e9be47f`). It is a baseline,
not a claim of upstream parity. The [manifest](issues/golden-20261108/README.md)
owns exact identities; the [performance report](bench/results/interpreter-parity-20261108.md)
owns the retained measurements.

## What works

- Universal eight-byte PyValues are integrated across the register interpreter,
  core containers/attributes and rebuilt bundled extensions. Owning bridges
  remain where direct-value operations have not replaced them; this is tuning
  work, not a second heap representation.
- Register lowering, explicit operands, constant homes, folded loads/stores,
  compacted slots, guarded specialization, monitoring and tail-call dispatch
  execute the normal Python workload. Calls use a frame stack and can overlap
  argument windows. See the [IR](design/register-ir.md) and
  [interpreter](design/interpreter.md) specifications.
- The selected batch integrates typed code homes, successful-refresh cleanup,
  a dictionary first-probe hit and guarded general
  Python-call entry. Full qualification at functional `0267d428808` passed
  **54,394 tests each in release and debug, zero failures**. All three builds
  check 117 modules; API lint covers 306 units with zero failing findings.
  `0765e9be47f` moves an existing comment; the manifest records its binary check.

## Candidate measurements

At `4b3d739d867`, six-round logging/golden is **1.00142**, logging/upstream
**1.39308**; Richards/golden is **0.98296**, spectral/golden **0.84032**.
The two-round 29-workload geomean is **1.00428 / golden**, **1.24457 / upstream**.
A four-round repeat confirms unpack +9.8%, pickle +8.0%, float +7.3%,
deepcopy_memo +6.4% and nbody +4.6%. Spectral's 16% gain repeats; the initial
generator regression does not repeat consistently. These are combined-image
results without component attribution. Next: bounded ablation, then a call/owner
cost ledger. See [follow-up](issues/execution-boundaries-20261108/next-steps.md).

Logging counters at the candidate show 1.4028× upstream instructions and
1.3988× cycles per workload iteration, with almost identical cycles/instruction.
Samples point to call preparation, frame clear, method lookup and return.
Profiles report zero lost samples but retain a header metadata warning.

## Golden performance

Elapsed ratios below are candidate/control; lower is better. Protocols differ,
so compare within each row. All use GCC and explicitly disable the JIT.

| Comparison | Result | Limit |
| --- | --- | --- |
| Logging / upstream | 1.3872 and 1.3786 | Two six-round campaigns; parity failed |
| Broad 29 workloads / upstream | 1.2412 | Two-round screen |
| Logging / retained R5 ELF | 0.8402 | Four rounds; both use current shared modules |
| Broad 29 workloads / retained R5 ELF | 0.9810 | Same shared-module limitation |
| Spectral / retained R5 ELF | 1.0860 | Repeatable regression, accepted in this batch |

Logging retires about 1.435× upstream's instructions and consumes 1.384× its
cycles. Added work is the main observed distinction. Samples point to argument
binding, frame retirement, method lookup and call/return handling; they do not
apportion a causal speedup budget. Spectral's regression has a measured CPI
penalty whose cause remains unresolved. Profiles have zero lost samples but a
retained perf metadata warning. See the report before interpreting samples.

## What remains

The [current frame/call plan](issues/frame-call-model-20261109/plan.md)
owns the next work; the previous packet retains the earlier nine cold-review
ideas and evidence. Keep the golden runtime immutable; changes
pass design → peer review → implementation → focused checks → post-review →
integration. Broad performance screens and full suites belong at coherent
integration points.

The current source implements compact executable positions, source-edge metadata,
semantic loop/delegation completion and initialized frame storage. The next cold
transfer model separates cancellable expression owners from saved-local/with
roots and exception restoration. General caught exits, suspended phases and
source-aware remote/GDB display remain open. Semantic owner retirement still
needs explicit transitions; ordinary thread clear does not clear live scratch.

Borrowed frame views are a reviewed investigation, not an approved implementation.
Ownership anchors, GC traversal, promotion, ordinary fallback and a complete
consumer census remain open. The fresh review must weigh this against other
structural changes. Native JIT recovery/backend completion and external
extension adoption remain later stages in the [roadmap](ROADMAP.md).

Supported qualification is Linux x86-64/GIL with GCC 15.2.0. Free-threaded and
32-bit builds are unqualified. Startup RSS and a zero-box census are not
performance admission gates. The [cleanup audit](issues/golden-20261108/cleanup-audit.md)
records why legacy dirty/evidence-bearing worktrees remain. Prior boards are
[archived](archive/20261108/STATUS.md), not active assignments.
