# Project status

The active [frame/call redesign](issues/frame-call-model-20261109/plan.md)
is integrated through engineering `488e85fa3a8`. It remains unpromoted.
Frames use initialized storage and explicit call/result ownership. Compact IR
keeps source boundaries in metadata. Observers publish committed results before
callbacks; debugger transfers use an exact stopped-state view rather than
assuming a callback stopped before an operation. Canonical local duplicates
reuse their existing PyValue bits through the shared final store.

The current bounded nonneutral committed view copies the actual site's rows,
applies its selected normal transfer, and invalidates consumed old origins.
It admits only inherited handled context, empty stopped/target saved-binding
states and a target without incoming scratch requirements. Existing PRE and
neutral-jump recipes keep their separate proofs. Tracing now uses explicit
borrow views for its argument window. Independent source/result review approved.
Release/debug each build 117 modules. Both pass 455 frame/observer tests with
one skip, including all of monitoring. After a narrow warning cleanup, both
rebuild and pass the seven targeted checks. All five core designs match `488e85fa3a8`.

The last full release diagnostic belongs to `3cde040a1afd`: 54,563 tests,
six failed modules (monitoring, peepholer, sys_settrace, pdb, gc_verify and tools).
Current focused checks repair the monitoring and API-lint failures. The GC
negative control passes separately on retained417 release, current31 release
and debug, but its full-run failure remains unresolved. Full release/debug
qualification, current stats and final performance gates remain open. The
current API check passes 306 units with zero failing findings/parse errors.
Golden remains immutable and selected.

Logging meets the timing gate at `488e85fa3a8`: two independent six-round
upstream campaigns give **0.90235** and **0.89841** elapsed ratios, with every
pair below 0.937. The unchanged 29-workload screens give geometric means
**0.96632/golden** and **1.19038/upstream**. Four original losses are flat or
faster in the screen (unpack, float, pickle, deepcopy_memo). Four-pair repeats
confirm three remaining losses against golden: generators +7.1%, coroutines +8.7%
and nbody +4.9%. The goal remains active. The
[ninth report](issues/frame-call-model-20261109/ninth-integration-report.md)
records the complete breakdown and exact evidence.

Current counters confirm generators/coroutines execute 5–6% more instructions;
nbody executes 1.4% fewer but takes 5.6% more cycles. All 18 counter captures and
six zero-loss cycle profiles pass their gates. Nbody's six further branch
captures show fewer branches and no consistent miss penalty. Next: peer-review
shared completion specialization against saved continuations and direct heap
frame binding, then admit a bounded shared-model improvement. The normal
frame/call owner model remains coherent; a new ABI needs evidence for its
per-resume work and frame-size costs. The next NULL-only debugger proposal
remains design-only. Review remaining debugger cases against documented useful jumps and explicit safe
refusals across active protocols; no blanket skips or weaker assertions. See the
[scope review](issues/frame-call-model-20261109/debugger-transfer-scope-review.md).
Full qualification, current stats and final review still remain.

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
