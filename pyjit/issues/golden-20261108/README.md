# Golden interpreter checkpoint, 2026-11-08

This freezes the current universal-PyValue register interpreter as the baseline
for subsequent experiments. Golden means reproducible and correctness-qualified.
It does not imply upstream performance parity or a completed native JIT.

## Published identities

| Ref | Contents |
| --- | --- |
| [`pyjit-golden-20261108-runtime`](https://github.com/rjpower/cpython/tree/pyjit-golden-20261108-runtime) | Exact runtime `0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae`; origin branch `boil/pyvalue-rc-universal` |
| [`pyjit-golden-20261108`](https://github.com/rjpower/cpython/tree/pyjit-golden-20261108) | The same implementation with current canonical documentation/tools under `pyjit/`, replacing the historical `boil/` documentation tree; branch `pyjit/golden-20261108` |
| `0267d428808` | Functional checkpoint used for the full suites, final timings and profiles; exact measured ELF retained locally |

Tags are immutable. The combined snapshot is for readers who need current
implementation and documentation in one checkout. Its publication checks
require all paths outside `boil/` and `pyjit/` to match the runtime commit
exactly. Publication receipts record the resolved commits and remote refs;
the canonical `weaver/python-jit` documentation branch is also pushed.

`0765e9be47f` only relocates an existing comment. The rebuilt debug `.text`
matches the measured functional checkpoint; release has four changed address
bytes pointing to equivalent `f` literals because build-identifier suffix
merging changed their locations. There is no interpreter-code change. Do not
claim identical whole ELFs or relabel old measurements as new captures.

## What is frozen

- Canonical eight-byte PyValues in interpreter homes, core containers and
  attributes; RC with cyclic collection; rebuilt bundled extensions. Direct
  value operations coexist with owning compatibility bridges where needed.
- Compiler-produced register IR with explicit slot operands and call windows,
  folded loads/stores, temporary compaction and constant homes for eligible
  loop scalars. Baseline IR is distinct from the planned optimizing SSA tier.
- Generated musttail handlers, guarded feedback/specialization, monitoring,
  frame stacking and overlapping calls, explicit ownership retirement and
  exception unwinding.
- The latest batch: typed code homes and successful-refresh cleanup
  (`0539e0c1ff9`), checked ordinary NOP continuation (`73c20472a9f`), dictionary
  first-probe identity hit (`d93b83800a5`), guarded general Python-call entry
  (`10627c339fa`), and test-interface fixes through `0267d428808`.

Read the [IR design](../../design/register-ir.md),
[interpreter design](../../design/interpreter.md),
[runtime architecture](../../design/runtime-architecture.md) and
[PyValue design](../../design/pyvalue.md). The first three include Mermaid
flow diagrams. The [historical IR reference](../../design/archive/register-ir-20261108.md)
retains earlier comparisons, proposals and decisions without presenting them
as the current specification.

## Qualification and performance

At `0267d428808`, release and debug each passed **54,394 tests, zero failures**.
Release/debug/stats check 117 modules; API lint covers 306 units with zero
failing findings. The comment-only rebuild subsequently passed the 142-test
stats smoke. Qualification covers Linux x86-64/GIL, GCC 15.2.0. The golden
documentation publication does not rerun full native suites because it changes
no implementation blobs.

The [retained report](../../bench/results/interpreter-parity-20261108.md)
is the measurement authority. JIT is explicitly disabled; compiler/build
policy is fixed. Elapsed candidate/control factors are:

| Comparison | Factor |
| --- | --- |
| Logging / upstream, two six-round campaigns | 1.387177; 1.378619 |
| 29-kernel broad screen / upstream, two rounds | 1.241158 |
| Logging / retained R5 ELF, four rounds | 0.840163 |
| 29-kernel comparison / retained R5 ELF | 0.980981 |
| Spectral / retained R5 ELF | 1.086037 |

The retained-R5 ELF comparison shares current extensions and standard library;
it is not a pristine old-source comparison. The spectral regression is accepted
for this checkpoint and must remain visible in subsequent integration screens.
Its extra counter capture shows a CPI penalty with unresolved cause. Logging's
instruction/cycle ratios against upstream are 1.434888/1.383861, suggesting
added work as the main measured distinction rather than a demonstrated cache
cause. Samples identify candidates, not additive potential savings.

All 72 stat invocations were valid; eight DWARF captures have zero lost samples.
The perf reports retain an `Invalid HEADER_EVENT_DESC` warning; stat results
are independent. Exact ELF, module, source, JIT-off and workload identities and
raw timings remain under the local evidence path in [HANDOFF](../../HANDOFF.md).
Raw build evidence is not uploaded to Git by this publication; another machine
can rebuild the source but cannot claim to possess the retained measured ELF.

## Cleanup and next work

Current navigation and design pages replace the accumulated historical boards.
The previous pages are [archived](../../archive/20261108/STATUS.md), with links
adjusted for their new locations. Sealed reviews, failed checks and measurement
records remain unchanged. Temporary instrumentation from the implementation
batch was already removed and stats rebuilt.

The [cleanup audit](cleanup-audit.md) records preserved dirty worktrees, ignored
evidence and an active build dependency. No legacy tree was established as
safe to delete. The three preexisting canonical local changes remain outside
publication. No runtime rewrite or speculative cleanup is included.

The [fresh cold review](v8-cold-review.md) assesses IR, optimization and
interpreter implementation independently. The [next plan](next-steps.md)
records its recommendations alongside accumulated evidence. Follow that plan
in separate experiments; do not move the golden tag.
