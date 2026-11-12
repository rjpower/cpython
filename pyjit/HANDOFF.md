# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`. Its C files
are not the measured runtime. Start with [goal](GOAL.md),
[active plan](issues/activation53/plan.md), [status](STATUS.md),
[experiment report](issues/generator-delegation52/report.md) and
[designs](design/runtime-architecture.md).

## Activation 53: reviewed experiments

The [plan](issues/activation53/plan.md), [thesis](issues/activation53/thesis.md)
and [phase evidence](issues/activation53/phase-evidence.md) describe the active
investigation. Source/native audits and one broad Astra review are complete; the
[resolution](issues/activation53/review-resolution.md) chooses two bounded lanes.
Worktrees `BASE/worktrees/activation53-{final-homes,completion}` and matching
release build directories are configured from golden51. Sol lanes cross-review
concrete proposals before source implementation. Root owns regeneration and
native build/test/timing. No measured win or qualified candidate exists yet.
Golden51 and the control paths below remain unchanged.

## Round 52: closed, both candidates rejected

Packet `pyjit/issues/generator-delegation52`; evidence
`BASE/pyjit-link-wave1/generator-delegation52`. One broad Astra review,
cross-component implementation/review and a fresh cold review are complete.
The terminal emitter is trusted compiler code: producer/predicate gates and
selected generated/native checks exist, while Astra finding 7's stronger emitted
body mutation obligation remains partial. The design now states that boundary.

First candidate `58f9ea6cfee47536893ec59780a0f719b4c856ef`, branch
`pyjit/generator-delegation52`, source `BASE/worktrees/delegation52`, builds
`BASE/delegation52-{release,debug,stats}`. All three import 117 modules. Focused
runtime/protocol/observer checks pass after integration repairs; 306 value API
units have zero failing findings/parse errors. Native GDB demonstrates two
successive forwarding hits with one C-held owner and empty intermediate homes.
Six golden51 pairs lose 4.4%/4.7%/2.7% on generators/coroutines/DeltaBlue;
all29/upstream is 1.167121. Fresh PMU stat has 24 valid 100%-running intervals;
four sampled profiles have zero losses and stable identities.

One bounded followup `e85171923c08b0d68ee169119e761d7583bbcb26`, branch
`pyjit/generator-delegation52-entry`, source `BASE/worktrees/delegation52-entry`,
release build `BASE/delegation52-entry-release` (now retired). It imports 117
modules and passes 462 focused tests. Debug/stats are configured but deliberately unbuilt
after its failed release performance screen. Static native inspection confirms
generation metadata moved behind wrappers. Six golden51 pairs nevertheless
lose 1.0%/11.3%/11.9% on the three targets; all29/golden51 is 1.020473, only
8/29 cases win. Both candidates are rejected; no further implementation is
queued. Followup all29/upstream is 1.172901; fresh PMU has 24 valid intervals,
all 100% running. Instructions/upstream are 1.593718 generators, 1.411233
coroutines and 1.356641 DeltaBlue. Reporting and independent review are complete.
The [report](issues/generator-delegation52/report.md),
[tables](issues/generator-delegation52/benchmark-tables.md) and
[next decision plan](issues/generator-delegation52/next-plan.md) own closure.
No round 52 native job remains. Never overlap future quiet campaigns with builds/tests.

The two task-created source trees and six role build directories above are
retired. Both branches are pushed. E/`retired-experiments` retains 349 files:
source patches, four built-role ELFs/extensions, configurations and SHA256
manifest; `retirement.json` lists the eight removed paths. Debug/stats followup
configuration copies do not imply built binaries. Raw evidence and all controls
remain intact. Recreate a source/build if needed with the retained branch head
and packet `build.py`; its matching configurations are retained. Do not confuse
an archived experimental binary with the selected runtime.

Golden51 remains immutable and selected. No new full suite was run after
the performance gate failed. Canonical closure documentation is on
`weaver/python-jit`; Loom mirrors the final report, tables, design and next plan.
GitHub CLI's authenticated credential helper publishes over HTTPS without
changing origin.

## Selected runtime and evidence

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
D=`pyjit/issues/interpreter-round51`.
E=`BASE/pyjit-link-wave1/interpreter-round51`.

The selected round-51 source is `BASE/worktrees/round51-golden`, branch
`pyjit/round51-golden`, commit
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`.
It is pushed to origin with tag `pyjit-golden-20261111-runtime`.
Builds are `BASE/round51-golden-{release,debug}`. Both check 117 modules,
regeneration leaves source clean, focused checks pass 821 tests with nine skips
plus five generator tests in each role, and value API checks pass 306 units.
No stats-layout change is claimed. The value checker uses a reference release
compilation database with paths rebased to the identically configured new
checkout; both native candidate builds are real, separate compilations.

Astra gave one grouped review of five options. Independent Sol proposals,
reviews and subtree implementations are complete. Only options 3 (shared exact
call setup) and 4 (canonical ADD/SUB publication) are selected. Options 1 and 5
failed measured breadth or regression gates; adding option 2 to 3+4 regressed
multiple cases despite a comprehensions gain. Do not revive rejected variants
or call Astra in the inner loop.

## Controls

All paths below are relative to BASE. Controls are immutable. One GCC 15.2
release configuration, CPU15, hash0, JIT disabled; magic64418,
ATTR/polymorphic geometry48/120 bytes.

| Role | Source under worktrees/ | Release build | Commit |
| --- | --- | --- | --- |
| Prior checkpoint50 | execution-boundaries-20261108 | execution-boundaries-release | bf810ec1a33 |
| Golden39 | golden-20261109-runtime | golden-20261109-release | 4d666e1b482 |
| Upstream | upstream-3a440b6 | upstream-release-devlibs-3a440b6 | 3a440b65695 |

The earlier golden, frozen46/47/48 and all preexisting worktrees remain intact.
Task-created option/control/trial worktrees and builds were removed. Local
branch commits and `E/retired-experiments` retain patches, native binaries,
configurations and the removal manifest. Do not confuse a removed experiment directory with selected source.

## Completed qualification and next work

All29 two-pair campaigns against upstream, actual prior50 and golden39 are
complete with stable source/native identities. Their elapsed geometric means
are 1.156535, 0.985700 and 0.982829 respectively. Six-pair direct target
comparisons and the 2+3+4 increment are complete. Keep DeltaBlue/generator
regressions and logging pair spread visible; do not multiply campaign ratios.
Fresh post-warmup PMU records 24 valid counter intervals and four zero-loss
sampled profiles. `final51-perf-stat` is an excluded pre-interval failure from
an old hardcoded extension count; `final51-perf-stat-clean` is the accepted
campaign. New clean builds have 79 current-ABI shared extensions; upstream has
78. The independently reviewed wrapper checks exact counts and hashes every
extension before/after, retaining all prior module/config/native gates.

Full diagnostics and deepcopy_memo confirmation are complete. Both builds run
54,689 filtered tests; release retains nine debugger-jump errors, debug those
nine plus one LLTrace failure. No additional module fails. The other four GDB
groups pass. Exact current case IDs are retained in verbose logs and independent
review; prior50 quiet logs establish only groups/counts. Independent final
review approves this incremental checkpoint with its explicit performance losses
and diagnostic limits.
Timing/profiling finished before full tests; never overlap quiet campaigns with
builds, tests or bulk cleanup.

`D/run-checks.py` quarantines only
`test.test_gdb.test_pretty_print.PrettyPrintTests.*`; the explicit opt-in restores
it. Other four GDB groups passed the filtered control check and remain enabled.
Debugger-jump and LLTrace retain their established failures; report actual
remaining failures rather than calling filtered full diagnostics green.

Activation 53 has weighted selected-image creation, resume, yield and completion
evidence for the active thesis. Constant operands need a weighted hidden-home
cost census before proposal. Feedback already uses a directly addressed scaled
byte offset, not an entry-pointer table. The source assessment records rejected premises.

Host scripts use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Root owns builds, tests, timing, integration, commits and publication. Workers
leave changes for review. Loom mirrors canonical docs under
`http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.

## Preserve unrelated work

Never stage/reset/delete the preexisting checkpoint report/log/lock:
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`,
`pyjit/issues/checkpoint-closure/w5-release.lock`.
Preserve other tasks' source, controls, caches and artifacts. Universal PyValues
are integrated; native method/tracing JIT tiers remain unfinished.
