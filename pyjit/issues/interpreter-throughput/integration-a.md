# Integration A: calls and retirement publication

Runtime commit `8a8fdc91368`, parent `6a0e4c12c17`, 2026-11-06.

The common positional Python-call path now transfers argument owners into
overlapping child frame homes after fallible preparation and final guards.
Missing defaults and empty varargs are supported; kwargs gets a fresh empty
dictionary before commit. Unsupported shapes and instrumented calls retain
the established provider. No fallible work follows ownership detachment.

Attribute retirement now owns input consumption and publication. Five load
producers drop their redundant `SYNC_SP`; the specialized generator override
is deleted. Analyzer retirement markers preserve synchronization metadata.
Regeneration produces exactly five duplicate store deletions, with no generated
metadata or target-table changes. There is no publication cache or new flag.

## Review and executed checks

[Call review](call-frame-review.md) and [publication review](publication-review.md)
accept the actual source. The coordinator reviewed the final diff and generated
products. A fresh cold review accepted the integrated runtime with no blocking
regression in [cold review A](cold-a-review.md). It identified a pre-existing
assertion that rejected an allowed provider-decline status, a misleading
overlap-range comment, and a possible later publication optimization. The first
two are handled in Integration B; the additional publication opportunity is
deferred pending generated-code/GCC evidence.

Release, debug and stats builds pass, each checking 117 modules with none
missing or failing import. Value API lint reports 306 units and zero failing
findings. Nine affected modules pass on both release and debug: call,
generated cases, IR, descriptors, frames, monitoring, opcache, GC and generators.
Each profile runs 939 tests (release skips 11; debug skips 10). The broad suites
are reserved for the coherent calls/frame-entry batch.

Early checks caught and corrected a borrowed-versus-owned C type assignment
for the immortal empty tuple, and a generator test calling a helper from a
different test class. Failed logs and corrected passes are retained. The
candidate's actual two new generator regressions pass; the worker's host-stdlib
import failure is not used as execution evidence.

The basic timing sample is mixed and does not support a broad speedup claim.
On the selected kernels, logging_silent candidate-A/parent was 0.8819 and the
current candidate/upstream ratio was 1.736. Generators took 1.0456 times
the parent time and 1.5061 times upstream; the candidate/upstream ratio worsened
by 11.75% versus the baseline comparison. Controls and baseline drift
limit interpretation. Retain these as screening measurements only; the full
matched kernel set and declared pyperformance subset remain for the final
coherent batch report.

Evidence root:
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/interpreter-throughput-20261106/`.
It contains component/unit logs, regeneration, three build logs, lint, focused
test commands and raw outputs. The clean baseline has three alternating rounds
of seven timing values for seven kernels under upstream and candidate.
Integration A timing follows a clean source and build-info refresh. The basic
sample is complete and recorded above; it supports no broad speedup claim.

The initial automatic approval rejection concerned unreviewed external runtime
writes. Normal coordinator integration was approved after actual source review;
the separate typed correction was independently reviewed and approved too.
No rejected action was performed through a workaround.
