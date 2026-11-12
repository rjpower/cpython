# Project status

Golden51 `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254` remains selected.
All three activation53 candidates are rejected after six-pair confirmation:
direct final-home construction, shared completion and native-only construction.
Native-only improves generators 5.9%, but repeatedly loses 2.4% on DeltaBlue
and 2.7% on Richards. No experimental code is absorbed.

Start with the [goal](GOAL.md), [round plan](issues/activation53/plan.md),
[current report](issues/activation53/report.md), [tables](issues/activation53/benchmark-tables.md)
and [next decision](issues/activation53/next-steps.md).
[HANDOFF](HANDOFF.md) owns exact paths and recovery. Canonical documentation is
in this checkout; its C files are not the measured runtime.

## Selected performance

Unchanged workloads, GCC15.2, CPU15, hash0, JIT disabled. Ratios are elapsed
golden51/upstream; lower is faster. The fresh all29 two-pair geometric mean is
**1.156831**, with 6/29 cases faster. Separate six-pair hot confirmation:

| Case | Geometric mean | Pair range |
| --- | ---: | ---: |
| Generators | 1.450583 | 1.441005–1.462817 |
| Coroutines | 1.257081 | 1.240689–1.271521 |
| DeltaBlue | 1.174507 | 1.170100–1.186879 |
| Richards | 1.245939 | 1.208283–1.271766 |
| Comprehensions | 1.376346 | 1.355705–1.402835 |
| Logging silent | 0.730916 | 0.662089–0.771119 |

Logging has a wide spread. The all29 generator screen also has a wide pair
range, 1.437–1.784; retain it separately from the longer confirmation. These
campaigns measure unchanged selected source and are not evidence of a new
implementation gain. Do not multiply independent comparison ratios.

The [checkpoint report](issues/interpreter-round51/report.md) retains direct
prior50 and golden39 comparisons, selected PMU data and incremental regressions.
The [round52 report](issues/generator-delegation52/report.md) records two rejected
delegation variants. Round53's full construction candidate reduces coroutine
instructions to 0.985784 of golden51 but takes 1.027712 times elapsed time.
Neither that result nor flat samples establishes a hardware cause.

## Selected design and gaps

Universal eight-byte PyValues span the runtime and rebuilt extensions.
The register interpreter is implemented; native method/tracing JIT tiers remain
unfinished. Exact calls reserve and initialize once, overlap thread arguments
and publish ordinary returns inline. Function and executing code remain separate
owners. Warm thread storage avoids repeated NULL initialization.

The remaining activation costs include a 112-byte header versus upstream's 80,
value classification, recipient/continuation state, construction-time physical
home movement and callback-safe retirement. Generator resume already dispatches
inside the evaluator; hidden constants are initialized at construction, not
each resume. The workload's frequent SEND/YIELD transitions and recursive
coroutine creation/completion need different treatment. These are concrete
mechanisms, not a complete causal accounting of the elapsed gap.

Current contracts: [IR](design/register-ir.md),
[interpreter](design/interpreter.md), [calls/frames](design/call-and-frame-lifetimes.md),
[activation transfers](design/activation-transfers.md) and [values](design/pyvalue.md).
Source positions do not authorize ordinary dispatch or call admission.

## Qualification and next decision

Selected release/debug builds each check 117 modules. Prior focused checks pass
821 tests with nine skips plus five generator tests per role; the value API gate
passes 306 units. Full diagnostics run 54689 filtered tests in each role:
release retains nine debugger-jump errors; debug retains those nine plus one
LLTrace failure. These are documented failed diagnostics, not green full suites.
Only the GDB pretty-printer class is quarantined; other GDB groups remain active.

Round53 followed thesis, one broad Astra review, independent implementation/
post-review, focused release/debug checks, native route probes and balanced
timing. Full suites were omitted once its candidates failed performance.
All three branches are pushed. Verified archives retain patches, six native
ELFs/extensions/configurations and hashes; only the nine task source/build
directories were retired. Selected builds and controls remain intact.

The [next decision](issues/activation53/next-steps.md) prioritizes reducing eager
activation state using existing authority, with alternatives for carrying resume
proofs and bounded compiled regions. Require named removed work, peer review and
cross-benchmark gates before implementation. Astra reviews broad designs rather
than the inner loop.
