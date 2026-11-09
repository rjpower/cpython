# Second engineering integration

Runtime `3e5cbdb11236ec91f79d8fabc0ceb7ad2ee674cb` is an engineering
integration, not a qualified replacement for golden. Its first matched
logging/upstream screen is **0.97449× elapsed time**, about 2.6% faster.
The two balanced pairs are 0.97178 and 0.97721. This does not replace the
required full qualification and two independent six-round parity campaigns.

## Implemented model

Loop and delegation completion now own their actual branch-specific window,
result recipient and exception phase. Generator resume separates outgoing
values from incoming recipients. Thread chunks retain an initialized-storage
frontier, filling only uncertain value storage instead of every frame tail.
Remote readers use a versioned target-described layout.

Source errors belong to evaluator activations, so nested callback errors during
retirement cannot overwrite the outer handler recipe. Cold LINE handoff has an
explicit landing and monitoring epoch. RESUME owns entry synchronization and
event ordering; compiler bookkeeping carries no pre-entry source boundary.
Static line inspection merges sorted candidates and real ranges with forward
cursors. The [canonical designs](../../design/register-ir.md) describe these
contracts and their remaining work, with six rendered and inspected diagrams.

## Functional evidence

Release and debug each build and import-check 117 modules. The final
`observer20` focused check passes **207 tests per build**, including arena reuse,
completion, nested source errors, observed generator entry/reconfiguration,
discard, frame calls and imports. The compile-only library check accepts all
**2,152 valid files**, with zero constructor rejections and 11 intentional
syntax fixtures. Cache format is 64415.

Earlier wider focused checks pass generators, coroutines, GC, weakrefs,
finalization, frame escape, generated handlers and remote inspection. Their
failed observation runs and the subsequent targeted repairs remain recorded
in the [diagnostic report](observer18-integration-check.md). Useful debugger
owner reconstruction is still incomplete. No full suite has run this packet;
these focused results do not establish whole-runtime qualification.

## Matched elapsed results

Ratios are candidate/control; lower is better. GCC 15.2, JIT off, CPU 15,
fixed hash seed, original workloads and seven inner durations per process.
Source and executable identities remain stable before and after each campaign.
The upstream and golden comparisons are separate campaigns; do not derive a
ratio between controls from this table.

| Workload | Two-round ratio / golden | Four-round repeat / golden |
| --- | --- | --- |
| logging_silent | 0.60171 | 0.61458 |
| unpack_sequence | 0.83725 | — |
| spectral_norm | 0.89667 | — |
| pickle | 1.00618 | 1.00679 |
| float | 0.97664 | — |
| deepcopy_memo | 0.94694 | — |
| nbody | 1.07491 | 1.08037 |

Unpack, float and deepcopy no longer show their earlier material regressions
in this screen. Pickle retains a small loss. Nbody remains materially slower:
all four repeat pairs lie between 1.07282 and 1.08766. The unchanged 29-workload
screen and final repeat requirements remain outstanding.

## Nbody cost evidence

Twelve warmed FIFO-controlled PMU captures pass with stable identities and
100% counter running. Median candidate/golden ratios are:

| Counter | Ratio |
| --- | --- |
| Instructions | 1.03194 |
| Cycles | 1.08001 |
| Branches | 1.05608 |
| Branch misses | 1.00010 |

The excess is about 40.1 million instructions and 28.9 million cycles per
unchanged outer kernel iteration, which includes 20,000 advance steps and
energy bookends. These are not counts per VM dispatch. Both additional work
and about 4.7% higher cycles per instruction contribute; branch-miss counts
are essentially unchanged.

Matched five-second profiles have zero lost samples. Candidate samples are
concentrated in specialized float arithmetic, list access/store, borrowed local
loads and unpacking. These locations guide experiments; their shares are not
independent speedup budgets. Perf retains its HEADER_EVENT_DESC warning and
zero sample timestamps in the header, so the profiles support flat location
analysis rather than a validated timeline.

A warmed physical inventory finds six local and 33 temporary arithmetic
destinations in `advance`; these are static sites, not execution frequencies.
The [independent numeric review](numeric-hotpath-independent-review.md) proposes
local-first result classification, checked canonical numeric producer facts,
and elimination of proven duplicate publications. The first experiment changes
only destination-category order. The larger producer proof needs independent
review before implementation. No component speedup has been established.

## Next integration

Implement the reviewed NULL and active-loop debugger recipes, preserving actual
owners and committing a redirect before retiring detached values. Measure the
numeric publication experiments against nbody, pickle and logging. Continue the
reviewed owner-retirement design: ordinary thread returns assert empty scratch
and clear locals, so deleting terminal releases requires an explicit semantic
ownership transition. The proposed free whole-frame cleanup was rejected.

After these coherent changes, run full release/debug qualification, API checks,
the broad screen and the two parity campaigns. Golden remains immutable and
selected until those gates pass.

## Evidence

Build root `pyjit-link-wave1/frame-call-model-20261109/` retains
`observer20-*`, `second-engineering-{upstream,golden,repeat}/`,
`second-engineering-release-identity.log`, `second-nbody-{stat,record}/`,
`second-nbody-stat-summary.json`, `second-nbody-warmed-destinations.*` and
`second-owner-inventory.*`. Measured release identifies the committed source;
the precommit release/debug checks exercise its functional source. No previous
failed receipt or unqualified engineering screen is called a final pass.
