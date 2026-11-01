# H1 tail dispatch

- State: focused native feedback accepted; fixed elapsed helpers in preparation
- Updated: 2026-11-01
- Owner: restart_audit prepares helpers; coordinator reviews and integrates
- Dependencies: exact compiler/backend plan and independent review before native execution
- Current grant: host-only elapsed helper preparation through06:04 UTC; checkpoint05:55
- Current managed helper source: `boil/vm-compiler-backend` at `417a9443a59`
- Worktree: `build/worktrees/vm-compiler-backend`; no runtime source changes

## Now

The unchanged-runtime GCC-computed/Clang-computed/Clang-tail comparison has
passed independently reviewed focused correctness. All14 native rows completed
05:40:26 UTC; root session66715 returned zero. There are105 selected invocations:
102 passes and three declared computed-arm tail-only skips. Both tail builds
execute the tail-specific case; five stale children passed30 control executions/
3,000 internal iterations. [Actual outcome](reviews/native-outcome-20261101-v3.md).

Actual source remains417, runtime-equivalent to qualifiede9. Four Clang images
are fresh; GCC release is reused unchanged. No runtime/frame ABI source change,
performance gain, backend selection or full qualification is established.

The fixed [revision3 plan](compiler-backend-plan-20261101-v3.md) governs the
four-workload G/C/T pilot:24 scored processes/120 values, twelve origin warmups,
two fixed rounds, all three contrasts and a30-minute inclusive quiet window.
Host helpers are being prepared through06:04, checkpoint05:55. Root must review
exact bindings before activation. No timing has run.

The independently reviewed [pre-score profile rule](../gc-parity-deadline/compiler-profile-addendum-20261101.json)
selects T only if T/C and T/G clear every gate; otherwise C if C/G does; otherwise
G. The selected profile stays fixed for the GC timebox and requires fresh
same-compiler/backend upstream before any all29 parity credit. Original budget
bytes, deadline and numerical target remain unchanged.

## Prior work

Revision2, revision3 and their original reviews remain frozen. The separate
[process-count clarification](compiler-backend-v3-count-clarification-20261101.md)
corrects physical children without changing exposure. Earlier source94ab98e8fd3
and the unmerged passed-frame convention62456cc59bd remain retained, with no
current measured benefit. The slot-base-only increment and compare/branch
region are rejected on elapsed. The exact strrchr effect declaration is already
in qualified0b15b4bbc9b; that does not land a new dispatch convention.

## Next

Freeze and independently review actual three-arm elapsed bindings, run one
finite quiet campaign, then apply the predeclared selector. Inspect actual
emitted carrier reload/spill paths to explain a surviving result; do not assume
tail superiority. Only a measured survivor advances to broad qualification or
a separately scoped frame-argument ABI experiment.

## Completion

Land a useful convention with preserved call/return/yield/unwind behavior, or
close with a reported negative/inconclusive result. Deferred is not abandoned.
A source patch or bounded correctness result is not a speedup claim.

## References

- [Handler and observer contract](../../design/handler-and-observer-contract.md).
- [Compiler and frame interfaces](../../design/compiler-and-ir.md).
- [Measurement contract](../../design/measurement.md).
