# Execution boundaries and operand modes

Started 2026-11-08 from golden runtime `0765e9be47f` and documentation
`3d18fd74f01`. The user authorizes the first two or three cold-review items,
with Sol 6.1 medium workers and coordinator-owned integration and performance.
The golden tags and their existing control builds remain unchanged.

Integrated runtime is `4b3d739d867b06ea4c9ed936982cfeb3503e93be`.
Release and debug full qualification passed 54,421 tests each, debug on retry
after an unexplained profiling failure. Measurements are complete: logging is
flat and broad regressions repeat. This is a qualified candidate checkpoint;
performance promotion is withheld and golden remains selected. The [report](report.md) and
[next steps](next-steps.md) record the current result and remaining gates.

## Outcome

Reduce unnecessary register-result disposal, empty source-boundary execution,
and repeated static operand-class decisions. `POP_TOP` in current IR is a
register-owner release with an explicit slot operand, not a stack operation.
Eligible call completion should discard directly under a coherent ownership and
observation protocol. Exact CPython finalizer timing is not a Python language
requirement; evaluate compatibility and memory policy separately, as specified
in [language and lifetime policy](../../design/lifetimes-and-language-semantics.md).
Ordinary execution should not decode
or publish an empty NOP boundary solely to pass through it.

Independent logical positions need not imply immediate physical compaction of
all canonical words. A bounded first implementation may retain dormant anchor
storage for observers if ordinary execution has no corresponding operation,
word check or PC update. Record that limit explicitly. Global remapping and
other excluded forms remain separate work, not silently completed features.

## Workstreams

| Packet | Worker | First gate |
| --- | --- | --- |
| Source and result boundaries | Sol 6.1 medium | Consumer census and separate post-return/NOP designs, then independent peer review before code |
| Static attribute operand mode | Sol 6.1 medium | One local-receiver/temp-result shape reader, compiled-work budget, then independent peer review |
| Integration and measurement | Coordinator | Review and admit each design; merge reviewed patches in dependency order, build all three profiles, focused checks, post-review and basic timing |

The completed component worktrees were retired after preserving complete patches
and identities in build evidence `component-source-archive.json`. The integration source is
`build/worktrees/execution-boundaries-20261108`, branch
`pyjit/execution-boundaries-20261108`. Canonical designs, reviews and progress
are in this packet. Workers never commit or run shared native builds.

The workers peer-review each other's proposal before implementation and inspect
the other's actual diff and executed checks before integration. The coordinator
reviews both. A fresh cold integrated review follows the combined checkpoint.
User model direction for this packet supersedes the older provider/role table.

## Validation and comparison

Reuse the configured GCC flags and shared development libraries in separate
`build/execution-boundaries-{release,debug,stats}` builds. The existing
`pyvalue-rc-repr-*` golden controls retain their source, binaries and modules;
candidate comparisons need not share changed extension modules with golden.
`prepare-builds.py` reuses existing products and reconfigures against the new
source. Native identity checks must prove the resulting module/stdlib paths.

Use cheapest meaningful generator and runtime tests first. Boundaries require
dynamic tracing/monitoring enabled inside callees or callbacks, legal/illegal
debugger jumps, exceptions/lasti, safe finalizer re-entry, frame escape and
generators. Label tests of chosen CPython timing compatibility separately from
language requirements; do not assume every historical decref point is mandatory.
Operand variants require guard/deopt, local/temp aliasing, mutation, canonical
values and instrumented-family mapping. Every C post-review includes a real
release compile. Whole release/debug suites and API lint run at the coherent
kept batch, not after each isolated opcode edit.

At each integration, run unchanged logging plus call/attribute/scalar controls
including spectral against golden and upstream, pinned to the same CPU with
native JIT-off assertions. Inspect generated GCC work before claiming a saved
runtime decision. Final broad comparison uses the unchanged 29-kernel roster.
Use counters/profiles to investigate unresolved differences, not as a substitute
for elapsed time. The existing two six-round parity protocol remains unchanged.
Record improvements, regressions, warnings, failures and excluded cases.

No numerical speedup is promised. Preserve failed or rejected increments and
all unrelated canonical changes. Update the goal, current specs, status,
handoff, report and Loom mirrors after measured integration.
