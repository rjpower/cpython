# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`. Its C files
are not the measured runtime. Start with [goal](GOAL.md),
[status](STATUS.md), [active plan](issues/frame-state54/plan.md),
[current report](issues/activation53/report.md), [tables](issues/activation53/benchmark-tables.md)
and [next decision](issues/activation53/next-steps.md).

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Current evidence E=`BASE/pyjit-link-wave1/activation53`.

## Round54: research active

Packet `pyjit/issues/frame-state54`; new evidence
`BASE/pyjit-link-wave1/frame-state54`. Astra researches five options covering
register validity, header layout and transfer specialization. Independent Sol
inventories cover fields and ownership validity. Implementation follows reviewed
contracts in isolated golden51-derived source trees. Root owns native execution
and integration; no round54 build or timing is running yet.

## Selected runtime and controls

Selected source is `BASE/worktrees/round51-golden`, branch
`pyjit/round51-golden`, commit
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`.
Release/debug builds are `BASE/round51-golden-{release,debug}`.
The branch and annotated tag `pyjit-golden-20261111-runtime` are published.
Source is clean. Universal PyValues and the register interpreter are integrated;
native method/tracing JIT tiers remain unfinished.

All paths below are relative to BASE. Controls remain immutable.

| Role | Source under worktrees/ | Release build | Commit |
| --- | --- | --- | --- |
| Prior checkpoint50 | execution-boundaries-20261108 | execution-boundaries-release | bf810ec1a33 |
| Golden39 | golden-20261109-runtime | golden-20261109-release | 4d666e1b482 |
| Upstream | upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695 |

One GCC15.2 release configuration, CPU15, hash0, JIT off; magic64418,
ATTR/polymorphic geometry 48/120 bytes. Selected builds check 117 modules,
including 79 shared extensions; upstream 116/78. Matching selected module log:
`BASE/pyjit-link-wave1/interpreter-round51/combined34-release-build.log`.
Upstream module log:
`BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log`.

## Activation53: closed, all candidates rejected

One broad Astra review corrected the thesis before implementation. Independent
Sol implementation/review lanes and Terra documentation work are complete.
Root owned generation, native checks, focused release/debug tests, GDB route
probes and quiet timings. All candidates remove the intended native boundary,
but fail confirmed breadth. None is absorbed.

| Retained pushed branch | Commit | Rejection |
| --- | --- | --- |
| pyjit/activation53-completion | 4d40da15b6e2bb9299fa56ef5d08d884a927e7d1 | Coroutines +3.1%, Richards +1.1% |
| pyjit/activation53-final-homes | 4b339980d020ae4f6026474e8942b7174ce57d16 | Coroutines +2.8%, logging +24.4%, comprehensions +1.5% |
| pyjit/activation53-native-construction | 779a1568a41652736dbd4c93b968d3381a9693b5 | DeltaBlue +2.4%, Richards +2.7% despite generators −5.9% |

Every listed regression appears in all six confirmation pairs.
Component reports own qualification and limits. No rejected candidate ran full
suites or all29. The full-construction instruction diagnostic has coroutine
instructions 0.985784 but elapsed 1.027712 versus golden51; it does not establish
a hardware cause. Native-only CALL/RETURN normalized instruction sequences
match selected51, but that does not prove unchanged placement or zero native
provider cost.

Fresh selected/upstream campaigns are E/`selected51-refresh-all29-upstream`
and `selected51-refresh-hot-upstream`. All29 two-pair geometric mean is 1.156831
with 6/29 wins. Separate six-pair ratios: generators 1.450583,
coroutines 1.257081, DeltaBlue 1.174507, Richards 1.245939,
comprehensions 1.376346, logging_silent 0.730916. Logging has a wide spread;
the all29 generator screen has a noisy 1.437–1.784 range. Retain both campaigns.

All nine round53 source/build directories are retired. E/`retired-experiments`
retains three source patches/commit logs and six verified tar archives containing
1014 native/configuration files, with SHA256 manifest and `retirement.json`.
No round53 native job remains. Recreate sources from the pushed branch heads;
`issues/activation53/build.py` and archived configurations provide build inputs.
Archived experimental ELFs are not selected binaries. Selected builds, all
controls, preexisting worktrees and raw evidence remain intact.

## Qualification and prior evidence

Selected51 focused checks pass 821 tests with nine skips plus five generator
tests per role; the strict value API gate passes 306 units. Full diagnostics
run 54689 filtered tests per role: release retains nine debugger-jump errors,
debug those nine plus one LLTrace failure. These are failed diagnostics with
documented limits. Only `test.test_gdb.test_pretty_print.PrettyPrintTests.*`
is quarantined; other GDB groups remain enabled.

The [selected checkpoint report](issues/interpreter-round51/report.md) retains
prior50/golden39 comparisons, prior diagnostics and PMU evidence.
The [round52 report](issues/generator-delegation52/report.md) records two rejected
delegation variants. Their branches, raw evidence and verified retirement
archives remain available under `BASE/pyjit-link-wave1/generator-delegation52`.
Do not revive those variants or subtract independent campaign ratios.

## Next work and operating rules

The [next decision](issues/activation53/next-steps.md) prioritizes eager activation
state contraction, then existing-proof SEND/YIELD specialization or bounded
compiled regions. Require a weighted native work-removal case before code.
Constants need a weighted hidden-home census; feedback already uses direct
scaled offsets rather than an entry-pointer table. Astra reviews broad designs,
not implementation revisions. Round54 owns the new implementation decision.

Never overlap quiet timing/profiles with builds, tests, GDB or bulk cleanup.
Root owns integration, native execution, timing, commits and publication.
Host scripts use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Loom mirrors canonical documents under
`http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.
Authenticated GitHub publication works through gh's HTTPS credential helper
without changing origin.

Never stage/reset/delete the preexisting report/log/lock:
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`,
`pyjit/issues/checkpoint-closure/w5-release.lock`.
Preserve unrelated controls, caches, sources and artifacts.
