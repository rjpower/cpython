# Project status

Status as of 2026-11-03 20:40 PST, from the coordinator fact sheet. Update this
page at each landing.

## Outcome and next milestone

The universal-value runtime links and runs, in debug and release builds. The
first checkpoint measurement (time and RSS, quiet-ish host) is in.
[Checkpoint](bench/results/checkpoint-20261103.md) and
[triage](bench/results/checkpoint-20261103-triage.md):

- `dbce7b1260d` ran the 29 kernels at 4.6x upstream `3a440b6`. Cause: natural
  method-JIT admission ran hot code through generic helpers. With admission off
  (`1f19952778d`), kernels take 2.6x upstream (geomean, interleaved runs). The
  interim build (`eba351fee7d`) takes 1.21x.
- Where the remaining gap goes: interpreter-loop self cost 39% (state spilled to
  the stack, a per-word `_PyValueTemps` record), exception save/restore around
  releases 25%, value bridging 17%, other 14%.
- The register IR executes 0.81 words per upstream instruction (geomean). Peak
  RSS is 34% above upstream, mostly startup (12.4 MiB against 9.5 MiB).

These numbers are a checkpoint, not a baseline. The next milestone is a
near-green suite, the 25% and 39% costs addressed (steps below), and an
official re-measure of kernels and the pyperformance subset on a quiet machine
after representation step 5.

## Position

- Runtime branch `boil/pyvalue-rc-universal`, HEAD `1f19952778d`.
- Landed today: native boundary P3 (`56ff66a596b`, `643c69c7244`,
  `6d39221996d`) and P4 (`2558376b321`, post-review fixes `fdd7fed3216`);
  representation steps 1 (`f129780a9a7`), 2 (`f7dfc5b961f`), 3 (`dbce7b1260d`,
  with 3b `379e92cded9`) and 4 (`7171541fa49`, `PyValue_NULL` is all zero bits);
  natural method-JIT admission off by default (`1f19952778d`).
- Debug suite after step 4: run=51,496, failures=13. Failing files: test_capi
  (3: test_getitem_with_error, test_is_unique_temporary, test_unicode.test_new),
  test_pyvalue_dict_vm (4), test_pyvalue_native_scope (6). Release also fails
  test_pyvalue: a release-only abort at `PyValue_Wrap` in `_testinternalcapi.c`.
- All 67 shared modules whose third-party headers exist build. The decimal,
  tkinter, ssl, sqlite and zstd headers are not installed on this host.

## Active work packets

Running now:

1. Representation step 5 (D2 core: release contract and retire path). Targets
   the 25% exception save/restore cost.
2. Value API V1, additive part: box counters, enforcement macros, a clang-based
   check, format conversions, entry primitives. The rename and header move are
   deferred scripts.

Next, in order:

3. Representation steps 6a to 6c, 7 and 8
   ([packet](design/proposals/pyvalue-representation-packet.md)).
4. Register IR v2 (IR-D1, IR-D2, G9 in [register-ir.md](design/register-ir.md)).
   Targets the 39% interpreter-loop cost.
5. Value API V2a-0 onwards.
6. Official re-measure after step 5, then the qualified baseline.

## Design decisions

- [PyValue](design/pyvalue.md), [review 1](design/reviews/pyvalue-review-1.md):
  D1 G1' fixes (done); D2 canonical construction and release contract; D3 int51
  inline ints (chosen from the [int-width histogram](bench/results/pyvalue-int-width-20261103.md));
  D4 static borrowing (joint with IR-D3); D5 NULL = 0 (done, step 4); D6 ABI policy (done);
  D7 type-slot gap after baseline (rows updated); D8 G7 non-goal, G8 deleted.
- [Native boundary](design/native-boundary.md),
  [review 1](design/reviews/native-boundary-review-1.md): accepted with changes;
  the page describes P4 as built.
- [Value API](design/value-api.md): values are the internal API and
  `PyObject *` is the compatibility layer. Review 1, revision 2, review 2 and
  revision 3 (`74236263ef1`) are done; accepted for packet V1.
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
- Upstream control: `build/upstream-release-3a440b6` (`3a440b65695`, -O3).
  `kernel.perf_event_paranoid=4` blocks `perf`, so the instruction metric is
  unavailable. `tools/sampler/` has an LD_PRELOAD SIGPROF sampler for profiles.
- Lessons from the checkpoint:
  - `sys._jit` covers only upstream's JIT. The harness must also assert the
    fork's method-JIT state (natural admission).
  - The pystats build compiles that JIT out, so pystats dispatch counts do not
    see it.
  - A `pybuilddir.txt` of `none` makes a release build 5 to 12x slower. Check it.

## Stages

Stage 1 ([runtime integration](stages/01-runtime/README.md)) is the critical
path. Stage 2 ([baseline](stages/02-baseline/README.md)) is blocked by it.
Stages [3](stages/03-method-jit/README.md), [4](stages/04-tracing-jit/README.md)
and [5](stages/05-extensions/README.md) are deferred or waiting on the
native-boundary work. See [HANDOFF](HANDOFF.md), [PROCESS](PROCESS.md) and
[WORKLOG](WORKLOG.md).
