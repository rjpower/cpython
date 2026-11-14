# Coordinator handoff

Canonical documentation is this checkout. Its C source is stale; do not build
it. Start with the [goal](GOAL.md), [active plan](issues/interpreter-performance56/plan.md),
[architecture](design/runtime-architecture.md) and [status](STATUS.md). The
[parity workstream](issues/interpreter-parity58/README.md) owns the next work;
its [design](design/proposals/interpreter-parity58.md),
[research](issues/interpreter-parity58/research.md) and
[peer review](issues/interpreter-parity58/review.md) are complete. No round58
runtime implementation or native campaign has begun. The completed
[comprehension plan](issues/comprehensions57/plan.md) retains round57 evidence.
Follow [process](PROCESS.md). The coordinator owns integration,
qualification, measurement and selection.

## Selected checkpoint and controls

Selected57 is `0dcba75bb3df40c32137db453b001f7e8f101de0`, branch
`pyjit/round57-golden`, tag `pyjit-golden-20261114-dictionary-readers`.
The [report](issues/comprehensions57/report.md),
[tables](issues/comprehensions57/benchmark-tables.md) and
[cold review](issues/comprehensions57/review.md) record selection and debt.
Golden56 remains immutable: commit
`883524c180e4486b47df77ee2276c9276c0ec32c`, branch `pyjit/round56-golden`, tag
`pyjit-golden-20261113-interpreter-performance`.

Current I is `0dcba75bb3df40c32137db453b001f7e8f101de0` in
`BASE/worktrees/call55-integrated`, branch `pyjit/comp57-dictionary-candidate`.
Its range from golden56 contains only the dictionary reader cleanup, mixed-table
cached-string lifetime repair and tests. Both `comp57-dictionary` builds complete
117 modules/79 extensions; 28 focused reader methods and 203 methods across
three dictionary modules pass per role (overlapping coverage). Native capture:
E55/comp57-dictionary-native, SHA256
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
All timing and PMU campaigns are complete and validated under
E57/dictionary-measurement. All29 is .995586/golden56 and 1.127137/upstream;
the upstream two-pair aggregate includes large XML outliers. Root explicitly
accepts six-pair coroutine/raytrace losses of about 2% and the generator mean
loss with its wide range. Comprehensions and DeltaBlue are near flat. No native
job remains active. Do not repeat measurements to find a favorable result.

The initial composition `9ec291998f7395fedf21f456e5eb58c2187d2e23` is preserved
on `pyjit/comp57-qualified-initial`, with retained `comp57-final2` evidence.
Full release at `3235a266f14` and debug at `9ec` each execute 55,045 tests. Release found the six baseline debugger errors plus one assertion-lint
failure; the explicit-borrow correction passes its final release focused check.
It changes no release instruction: final `.text` matches the measured `70a`
composition byte for byte. Debug's only errors are the same six debugger cases.
Final verbose checks match their names exactly in both roles. Five bootstrap
images and 27 compile probes pass. Only GDB `PrettyPrintTests` is quarantined.
Qualification retains these baseline limits; no green full-suite claim.

Evidence: E55/comp57-final-release-release-full.log,
comp57-final2-debug-debug-full.log, comp57-final2-lint-release-focus.log,
comp57-final2-freezer-commands.json and E57/final-debugger-baseline.json.
Initial `9ec` release ELF: E55/comp57-final2-native/python, SHA256
`57ffcfa3e292913294909d8a485208368bd8d37439b43db45deb8f7f1f8cabf0`.

Four reviewed changes were integrated: checked native method entry, shared
dictionary lookup layers, checked private list iteration, and inline spare
capacity append. The preceding two-lane composition had a logging regression;
the four-lane composition also loses and is rejected.
The uniform 64-byte handler alignment trial was rejected and reverted. Its
separate branch `pyjit/comp57-alignment-trial` at `cd0` is retained as evidence.
See the [workstream plan](issues/comprehensions57/plan.md) for the measured
screens and next gates. The interpreter-local append trial `0748` remains on
`pyjit/comp57-integrated`: 172 focused methods pass per role, but its screen
loses logging 21.16%, coroutines 3.30% and Richards 3.66% despite a 7.30%
comprehension gain. Its ELF is E55/comp57-local1-native. It is rejected too.

The frozen golden56 control is
`BASE/worktrees/round56-golden`, branch `pyjit/round56-golden`, with build
`BASE/round56-golden-release`; its module record is
`E57/control-retry-build.log` (117 modules, 79 extensions). Do not modify this
control. Root owns all native jobs and serialized quiet timing windows.

Retain the earlier comparison controls as well:

| Role | Source worktree | Build | Modules / extensions |
| --- | --- | --- | --- |
| Golden54 | `R/frame54-cold-construction` | `BASE/frame54-cold-construction-{release,debug}` | 117 / 79 |
| Prior51 | `R/round51-golden` | `BASE/round51-golden-{release,debug}` | See round-56 report |
| Upstream | `R/upstream-3a440b6` | `BASE/upstream-release-devlibs-3a440b6` | 116 / 78 |

Golden54 remains immutable at `61015a9452d9069646fb5fede9ff963896833b89`;
upstream is `3a440b65695615e9fd9912342b79cdd54fced0d2`. Their comparison logs
remain under `BASE/pyjit-link-wave1/frame-state54/cold-construction-initial-release-build.log`
and `BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log`.

## Work and evidence locations

`BASE` is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`;
`R` is `BASE/worktrees`. Evidence roots are `E55=BASE/pyjit-link-wave1/call-model55`,
`E56=BASE/pyjit-link-wave1/interpreter-performance56`, and
`E57=BASE/pyjit-link-wave1/comprehensions57`.

The initial `9ec` campaigns are complete: E57/final-measurement/{numbers.json,
benchmark-tables.md} validates all 29/control (two pairs each), hot-nine/golden
(six pairs), and upstream/golden PMU. All-29 elapsed is 1.001784/golden56 and
1.144639/upstream; do not promote it. Silent logging and coroutines regress in
every confirmation pair despite unchanged instruction counts versus golden.
The six-event frontend diagnostic failed with all counters not counted; retain
its failure and make no cache-specific claim. The local-append trial also loses;
do not repeat full qualification or run arbitrary padding/alignment sweeps.
The minimal correctness-only `72ee` branch is preserved at
`pyjit/comp57-golden-candidate`, with E55/comp57-correctness-native. Its all29
is flat, but DeltaBlue loses 6.94% in confirmation, reproduced by four diagnostic
pairs. E57/correctness-measurement retains validated timing and diagnostic
numbers/tables. The selected dictionary-only subset removes nested lookup/pin
layers as well; no further candidate or layout sweep follows this closure.
Its matched upstream instruction ratios are DeltaBlue 1.361424, comprehensions
1.436416 and coroutines 1.307831. Full tables and hashes remain in
dictionary-measurement.

Existing component worktrees and branches are retained:

| Worktree | Branch | Head |
| --- | --- | --- |
| `R/call55-ir` | `pyjit/comp57-local-append` | `20d149b1163` |
| `R/call55-pins` | `pyjit/comp57-checked-method` | `6987a396f83` |
| `R/call55-gc` | `pyjit/comp57-dict-only` | `0dcba75bb3d` |

Use I for integration; worker branches are component packets without every root
compile correction. Preserve them, old golden controls and all retained outputs.
Prior `pyjit/comp57-list-paths` at `c04e0503353` and `pyjit/comp57-dict-read`
at `015e2e3bcc9` remain intact. Do not treat them as selected runtime code.
The bitmap `0e918`, IRSC2 and M4 experiments are independent and remain
experimental; they are not requirements for this workstream. The
[call-model55 report](issues/call-model55/report.md) and
[round-56 report and tables](issues/interpreter-performance56/report.md) own
their historical details and qualification evidence.

## Native work rules

Use the existing helpers in `issues/call-model55` for native commands,
builds and qualification. Host Python commands use
`UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`. Keep build
and generation jobs serialized. During a quiet timing window, workers may read
or edit source but must stop host tests and heavy jobs; never mutate the measured
tree. Require exact source and native identities, stable workloads, matched
configuration and balanced campaign pairs.

Do not reuse a runner with an embedded old source identity. Retain native ELF
and disassembly before advancing builds. Startup RSS is not a throughput gate.
The existing six debugger-jump errors are baseline limits; the GDB `PrettyPrintTests`
class remains the sole quarantine. Do not describe either as a green full suite.

## Documentation and preservation

Keep the active chain clear: goal → workstream plan → architecture/status.
Historical reviews and experiment reports remain linked references, not active
plans. Update the plan and status when qualification or selection changes.
Publish canonical docs before guarded Loom mirrors and verify exact readback;
the artifact root is `http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.

Preserve unrelated pre-existing files:
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`, and
`pyjit/issues/checkpoint-closure/w5-release.lock`.
