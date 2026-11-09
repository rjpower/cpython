# Project status

The active [frame/call redesign](issues/frame-call-model-20261109/plan.md)
is integrated through engineering `bfd67cbe4b4`. It remains unpromoted.
Frames use initialized storage and explicit call/result ownership; source
boundaries live in metadata. Proved VM completion directly decodes immutable
SEND/FOR fields and specializes one shared owner/observer body. Native/THROW
providers retain publication. There is no new frame ABI or per-resume work.
Release/debug build 117 modules and pass575 focused tests/11modules each.
The [eleventh report](issues/frame-call-model-20261109/eleventh-integration-report.md)
records compiled tradeoffs, current counters and validation limits.

Logging preserves parity in its latest four-pair check: **0.80151/upstream**,
all pairs below0.820. The full two-campaign six-round gate belongs to32; the
latest change is not assigned logging's observed gain. Current four-pair golden
losses persist: generators+7.0%, coroutines+2.6%, nbody+4.5%. Source32's full29
screen remains **0.96632/golden**, **1.19038/upstream**, not new34 evidence.

Current34 counters pass18 capture/identity gates with100% event running:
generators1.0330 instructions and1.0842 cycles/golden; coroutines0.9908/1.0293;
nbody0.9860/1.0366. No single extra-work explanation fits all three. Compiled
SEND_GEN already coalesces the apparently duplicate state loads. Next: preserve
proved canonical values through plain yield and list-child transfers, using
existing transfer/store machinery. In a separate source worktree, the reviewed
NULL-only PRE query and consumer are being implemented. File ownership is
explicit; native integration and timing remain serialized.

Full release/debug qualification and current stats remain open. The last full
release diagnostic at31 ran 54,563 tests with six failed modules. Focused32
checks repair monitoring/tools; its API check passes306 units with zero failing
findings/parse errors. Peepholer, sys_settrace and pdb still have open debugger
cases. The GC negative-control full-run failure is unresolved despite isolated
method/module passes. No blanket skips or weaker assertions. Golden remains
immutable and selected; the overall goal stays active.

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
