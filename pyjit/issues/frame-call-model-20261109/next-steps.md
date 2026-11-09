# Next five priorities

This is the coordinator's evidence-weighted plan after frozen runtime
`4d666e1b482`. Two independent engine reviews supplied structural alternatives;
the complete benchmark set, current counters and source inspection changed their
initial frame-focused ranking. These are proposals for the next round. No new
runtime feature is required to close this round.

Keep canonical owning homes, initialized reusable storage, overlapping arguments,
explicit recipients/continuations and commit-before-close transactions. The
[closing report](closing-report.md) owns measurements and qualification limits.
The [complete tables](closing-benchmark-tables.md) distinguish elapsed time from
raw and skeleton-subtracted instruction counts.

| Rank | Fix | Evidence and first scope | Admission gate |
| --- | --- | --- | --- |
| 1 | Finish guarded lookup for canonical values | An instance reading an unshadowed inline class scalar explicitly declines specialization. The existing cache assumes an object-form descriptor. Its microcase uses 4.54× upstream raw instructions (8.83× after skeleton subtraction). First represent a guarded canonical class value without boxing solely to fit the cache. Inspect existing dictionary first-probe hit rates separately before changing that path. | Preserve instance shadowing, type/MRO versions, descriptors, custom hooks and cached-owner lifetime. Require actual warm handler/hit evidence, mutation controls, lower instruction counts and paired application timing. |
| 2 | Reduce work across the full generator resume/yield/delegation cycle | The generator kernel uses 1.416× upstream instructions and 1.442× cycles. The resume microcase creates one generator per 1,000 yields, so creation alone cannot explain its gap. First ledger SEND/FOR, YIELD, recipient installation, completion and retirement as one cycle. | Keep one return transaction; count classification updates as well as deliveries. Preserve thrown errors, close, recursion, observation, owner reentry and newer redirects. Inspect total text/spills, then measure generators and coroutines. |
| 3 | Reuse a fixed keyword binding plan | The fixed three-keyword microcase uses 1.985× upstream raw instructions. The current known binding fast path covers positional shapes; keyword calls still use the generic provider. First cache a versioned name-to-local permutation and reuse the same consuming binder. | Cover defaults, rebinding, duplicate/unexpected keywords, arbitrary mappings, partial-child cleanup and callbacks. No second binding semantics or per-frame flag. Measure move counts, keyword calls and call-heavy kernels. |
| 4 | Complete value-native getset and native-result boundaries | The cell microcase uses 2.028× upstream raw instructions. Source inspection finds an inline 3000 boxed by cell_get_contents and canonicalized again later; an existing value-getset record offers a direct route. Start there, then measure other native bridges. The str_join microcase mostly executes 1,000 append calls per amortized join, so it does not locate a Unicode-join defect. | Keep public object getters' owned-box contract and native error/lifetime rules. Use typed value callbacks, not casts or a borrowed box cache. Test empty cells, subclasses, descriptors and reentry; measure actual bridge traffic and unchanged workloads. |
| 5 | Derive ownership effects once and attach retirement to semantic outcomes | Lowering, generated handlers, publication proofs and cold validation repeat facts about the same transfers. Current whole-C-body proof templates are deliberately conservative but costly to extend. First migrate one producer proof to structured emitter facts with identical output; use that contract for a bounded semantic-retirement optimization. | Mutation/alias/error tests must reject changed transfers. Preserve GC roots, initialized reuse, local visibility, unwind and reentry; do not assume normal frame clear closes arbitrary scratch. Require dynamic dispatch counts before removing an operation, and compare added decoding with saved work. |

Raw micro instructions include the loop; net counts subtract a different
skeleton in each runtime. Neither ratio is an elapsed speedup or a whole-program
cost fraction. The lookup source defect is concrete, but its share of DeltaBlue
has not been isolated. Current profiles locate both calls and attribute paths.

Direct owning heap construction remains a useful separate experiment if creation
counts justify it. Hot-code layout and feedback placement remain measurement
variables, but current DeltaBlue CPI is better than upstream: removing executed
work is the stronger first lead. Cold debugger proof caching, additional safe
jumps and exact developer-tool display ports are outside this throughput list.

## Execution order and checks

Use separate bounded packets: propose → independent review → implement → focused
tests → post-review → integrate → basic performance. The coordinator owns
integration and serialized native runs. Full suites and the full benchmark set
belong at coherent checkpoints, not every opcode or call-site edit.

Before declaring a new memory regression, repeat the collector rows in balanced
order: the single serial sweep is roughly 4% slower than golden and 27–34% slower
than upstream. That observation does not authorize collector redesign. Also
measure actual dictionary fast-path hits and call/resume shape frequencies;
benchmark names alone do not identify a failing mechanism.

Independent references:

- [Architectural alternatives](closing38-cold-architecture-review.md)
- [Implementation review](final-engine-architecture-review38.md)
- [Measured priority review](closing39-measured-priority-review.md)
- [Source-backed micro addendum](final-micro65-priority-addendum39.md)
- [Independent raw/counter/profile audit](final-performance-audit.md)

## Preserved drafts

Coroutine ingress, retained-owner bundles and the partial GDB port are excluded
from the measured image. Complete source and identities are retained under
`E11/deferred-after39-source/`; the authoritative coroutine packet is
`E11/coroutine40-frozen-source.patch`. The task-created worktree and branch were
removed after verifying the archive.

The coroutine packet passed its minimal release run but debug asserted while
instrumenting its synthetic handler-to-RESUME negative. The exact packet was
reversed. The owner draft has no native admission. Nine ordinary frame-jump
cases remain documented capability gaps. Revisit them through a shared
owner/context contract if prioritized; do not add per-test setter rules.
