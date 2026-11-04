# Project status

Status as of 2026-11-03 17:40 PST, from the coordinator fact sheet. Update this
page at each landing.

## Outcome and next milestone

The universal-value runtime now links and runs, in debug and release builds.
There are no performance results for it yet. Any performance claim about it is
premature.

The next milestone is a near-green test suite, then a checkpoint measurement
(time and RSS) against upstream `3a440b6` and the interim runtime on a quiet
machine, then a qualified baseline.

## Position

- Runtime branch `boil/pyvalue-rc-universal`, HEAD `6d39221996d`.
- All 67 shared modules whose third-party headers exist build. The decimal,
  tkinter, ssl, sqlite and zstd headers are not installed on this host.
- Full debug suite at 17:31, before the last fixes: 611 of 619 files run, 484
  OK, 30 failed, 3 env changed; 48,500 tests, 52 failures.
- Failure causes: zero-filled slots (NULL = 0), per-call malloc in
  `native_scope.c` (P4), IR and tooling expectations. The eval-loop C frame fell
  from 35.6 KB to 5.5 KB (upstream about 360 B).

## Active work packets, in order

1. Test-suite triage: two agents fix the remaining failures.
2. Native boundary P4: argument vectors on the borrow stack (P1 to P3 landed;
   post-review is running).
3. [PyValue representation packet](design/proposals/pyvalue-representation-packet.md):
   accepted plan, 11 steps, about 42 agent-hours; starts after triage and P4.
4. Register IR v2 (IR-D1), then the baseline.

## Design decisions

- [PyValue](design/pyvalue.md), [review 1](design/reviews/pyvalue-review-1.md):
  D1 G1' fixes (done); D2 canonical construction and release contract; D3 int51
  inline ints (chosen from the [int-width histogram](bench/results/pyvalue-int-width-20261103.md));
  D4 static borrowing (joint with IR-D3); D5 NULL = 0; D6 ABI policy (done);
  D7 type-slot gap after baseline; D8 G7 non-goal, G8 deleted.
- [Native boundary](design/native-boundary.md),
  [review 1](design/reviews/native-boundary-review-1.md): accepted with changes.
- [Register IR](design/register-ir.md), [review 1](design/reviews/register-ir-review-1.md),
  [review 2](design/reviews/register-ir-review-2.md): accepted after review 2.
  IR-D1 one encoding revision (IR v2) with a bit budget; IR-D2 generated
  instruction contract; IR-D3 ownership = static slot class plus value tag;
  IR-D4 JIT boundary state is open. One limits rule; names past 4,095 lower to
  an intrinsic; 8-byte feedback offsets.
- [Representation packet](design/proposals/pyvalue-representation-packet.md),
  [review](design/reviews/pyvalue-representation-packet-review-1.md): accepted
  plan for D2, D3, D5, G1', G3, G10 and G11.

## Measurement setup

- Debug builds: `build/pyvalue-rc-debug-20261102-v1`,
  `build/pyvalue-rc-bringup-debug`, `build/pyvalue-rc-triage-rt-debug`.
  Release build: `build/pyvalue-rc-release-20261103` (mimalloc, computed gotos,
  shapes, -O3).
- Upstream control: `build/upstream-release-3a440b6` (`3a440b65695`, -O3). The
  harness time metric works. The instruction metric is blocked by
  `kernel.perf_event_paranoid=4`; the user was asked to lower it and has not.

## Stages

Stage 1 ([runtime integration](stages/01-runtime/README.md)) is the critical
path. Stage 2 ([baseline](stages/02-baseline/README.md)) is blocked by it.
Stages [3](stages/03-method-jit/README.md), [4](stages/04-tracing-jit/README.md)
and [5](stages/05-extensions/README.md) are deferred or waiting on the
native-boundary work. See [HANDOFF](HANDOFF.md), [PROCESS](PROCESS.md) and
[WORKLOG](WORKLOG.md).
