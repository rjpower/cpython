# H1 tail dispatch

- State: measured experiment closed; retain GCC computed, reject Clang computed/tail
- Updated: 2026-11-01
- Owner: restart_audit prepares helpers; coordinator reviews and integrates
- Dependencies: exact compiler/backend plan and independent review before native execution
- Current grant: none; native and fixed elapsed campaigns complete
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
are fresh; GCC release is reused unchanged. No runtime/frame ABI source change
was made. The fixed elapsed result rejects both Clang packages.

The fixed [revision3 plan](compiler-backend-plan-20261101-v3.md) governs the
four-workload G/C/T pilot:24 scored processes/120 values, twelve origin warmups,
two fixed rounds, all three contrasts and a30-minute inclusive quiet window.
The campaign completed06:02 UTC, session69441 exit0. Independent root review
recomputed all120 values, reparsed twelve origins and verified265 bindings:
Clang computed/GCC coroutines1.061794/1.056042; Clang tail/GCC1.082834/1.073053.
Every primary/adequacy gate fails. Both tail contrasts also have repeated
comprehension regressions. [Independent decision](reviews/elapsed-outcome-20261101-v1-root.md).
No extra samples or broad backend qualification follow.

The independently reviewed [pre-score profile rule](../gc-parity-deadline/compiler-profile-addendum-20261101.json)
selects T only if T/C and T/G clear every gate; otherwise C if C/G does; otherwise
G. [G is now selected](../gc-parity-deadline/compiler-selection-20261101.json)
and stays fixed for the GC timebox. Original matched GCC upstream, budget
bytes, deadline and numerical target remain unchanged. No Clang upstream build
is needed. This pilot establishes no all29 parity.

## Prior work

Revision2, revision3 and their original reviews remain frozen. The separate
[process-count clarification](compiler-backend-v3-count-clarification-20261101.md)
corrects physical children without changing exposure. Earlier source94ab98e8fd3
and the unmerged passed-frame convention62456cc59bd remain retained, with no
current measured benefit. The slot-base-only increment and compare/branch
region are rejected on elapsed. The exact strrchr effect declaration is already
in qualified0b15b4bbc9b; that does not land a new dispatch convention.

## Next

Closed on measured loss. Preserve sources, failed audits and raw evidence;
do not retime or carry either rejected backend into the GC candidate.

## Completion

Land a useful convention with preserved call/return/yield/unwind behavior, or
close with a reported negative/inconclusive result. Deferred is not abandoned.
A source patch or bounded correctness result is not a speedup claim.

## References

- [Handler and observer contract](../../design/handler-and-observer-contract.md).
- [Compiler and frame interfaces](../../design/compiler-and-ir.md).
- [Measurement contract](../../design/measurement.md).
