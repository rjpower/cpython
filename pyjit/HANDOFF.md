# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`. Its C files
are not the measured runtime. Start with [goal](GOAL.md), [status](STATUS.md),
[active plan](issues/call-model55/plan.md), [report](issues/frame-state54/report.md),
[tables](issues/frame-state54/benchmark-tables.md) and
[next decision](issues/frame-state54/next-steps.md).

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
Current evidence E=`BASE/pyjit-link-wave1/frame-state54`.

## Current checkpoint

Round54 is closed. Local and remote `pyjit/round54-golden`, and annotated tag
`pyjit-golden-20261112-frame-state`, resolve to
`61015a9452d9069646fb5fede9ff963896833b89`. The interrupted push completed;
do not treat publication as pending. This is the cold-only correctness
checkpoint, selected with diagnostic limits and no performance lift.

Selected source `BASE/worktrees/frame54-cold-construction`, implementation
branch `pyjit/frame54-cold-construction`, builds
`BASE/frame54-cold-construction-{release,debug}` are retained. Only genobject.c's
public constructor/wrappers, a testcapi helper and three constructor regression
methods differ from golden51. No NONE/R/S/header/hot change is absorbed.

Round55 holistic call/frame/bitmap architecture research and independent
post-review are complete. Follow [plan](issues/call-model55/plan.md),
[proposal](design/proposals/register-call-model55.md) and
[resolution](issues/call-model55/review-resolution.md). No runtime implementation
has started. No native jobs or performance rescue lanes are active.

Completed candidate evidence under E:

- `cold-construction-initial-{release,debug}-build.log`:117 modules/79 shared.
- `cold-construction-focus-focus-checks.json` and role logs:808 tests per role,
  10 skips release,9 debug; matching clean source/native/magic64418.
- `cold-construction-allocation-commands.json` and role logs:48 points per
  subtype per role; generator3 failures/45 successes,coroutine5/43,asyncgen3/45;
  code/namespace owners checked after each collection.
- `cold-construction-value-api.log`, command JSON and rebased CDB:306 units,
  0 failing findings,0 parse errors; informational counts93/151/1.
- `cold-construction-{release,debug}-full-full-checks.json` and
  `cold-construction-{release,debug}-full-{release,debug}-full.log`: completed
  54,692 tests per role. Release retains nine debugger errors and an unstable
  GC-verifier failure. Debug retains nine debugger errors, LLTrace and one
  unexplained profiling failure. These are failed full diagnostics.
- `gc-verifier-repeat-commands.json` and12 logs:6 alternating targeted runs
  per release build;4 failures/2 passes on both oldgolden and coldcandidate.
  Source hypothesis is conservative-root sensitivity, not an established cause.
- `cold-diagnostic-repeat-commands.json`: same nine debugger error identities
  and debug LLTrace failure as selected51. Profiling targeted, tracing-module
  and complete-package repeats pass both debug builds; see the three
  `profiling-*-commands.json` journals. The original full-run profiling finding
  remains open, not proven preexisting or fixed.

Independent proposal/source/native/cold reports live in the canonical packet.
Both-role normalized hot call/send/yield/return/ordinary-construction sequences
match selected51; native placement and elapsed equality do not follow from that.
All qualification and final measurement jobs are complete. Final all29 versus
golden51 is 1.006381 (11/29 lower); separate hot6 is 0.996669. Upstream all29
is 1.156543 (6/29 faster), with valid hot6 in
`final54-hot6-confirm2-upstream`. Preserve excluded failed-load
`final54-hot6-upstream`. Final PMU `final54-stat-{cycles,branches}` complete
18/18 each; `final54-record` completes 6/6, no lost records. See
[final tables](issues/frame-state54/benchmark-tables.md) for ranges and scope.
No performance improvement is claimed for selection.

## Immutable prior runtime and controls

Prior selected source `BASE/worktrees/round51-golden`, branch `pyjit/round51-golden`,
head `0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`; builds
`BASE/round51-golden-{release,debug}`. Published annotated tag
`pyjit-golden-20261111-runtime` remains immutable. Universal PyValues and the
register interpreter are integrated; native method/tracing tiers are unfinished.

| Role | Source under BASE/worktrees/ | Release build under BASE/ | Commit |
| --- | --- | --- | --- |
| Prior checkpoint50 |execution-boundaries-20261108 |execution-boundaries-release |bf810ec1a33 |
| Golden39 |golden-20261109-runtime |golden-20261109-release |4d666e1b482 |
| Upstream |upstream-3a440b6 |upstream-release-devlibs-3a440b6 |3a440b65695 |

One GCC15.2 release configuration,CPU15,hash0,JIT off; magic64418,
ATTR/polymorphic geometry48/120 bytes. Selected54 and golden51 each117 modules/79 shared; upstream
116/78. Golden51 module log:
`BASE/pyjit-link-wave1/interpreter-round51/combined34-release-build.log`.
Upstream module log:
`BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log`.
Golden51 prior full limits:54,689 tests per role,9 debugger-jump errors release,
those9 plus1 LLTrace debug. Only the GDB pretty-print class is quarantined.

## Closed performance experiments

One broad Astra review and independent Sol inventories/implementation/peer and
post-reviews covered five options, two revisions and two compositions.
All performance candidates are rejected; no component result is selected.

| Published branch under pyjit/ | Commit | Decision |
| --- | --- | --- |
| frame54-recipient |612fb4e9d17 |All29.989479,22/29lower; hot coroutine1.068741 |
| frame54-feedback |7ea40d5d67c |All29.993748; hot coroutine1.040777 |
| frame54-globals |c2cc6777e4d |Header96; hot mixed/worse |
| frame54-locals |b48fe2d3989 |Header104; exec1.222247,class1.042134 |
| frame54-clears |4fbd16a1462 |Initial four repeated geometry checks; screen rejected |
| frame54-clears-validated |e95328ba1d6 |Shared admission proof; all29.992128; hot coroutine1.024050 |
| frame54-compact |82afe7aef2d |Actual header80; screen rejected |
| frame54-recipient-default |fcc82220ddd |All29.999754; hot coroutine1.063816 |
| frame54-combined |f3c464a54c8 |All29.993149; hot generator.956948,coroutine1.098464 |

Ratios are candidate/golden51 elapsed. All29 screens have2pairs; hot6where
specified. Do not infer one candidate's result from another or multiply ratios.
The report records why broad modest wins do not compensate material target loss.

All27 rejected source/build directories are retired. E/`retired-experiments`
contains9 source patches/commit logs,18 verified native/configuration archives,
3,042 archived files, SHA256 manifest and retirement.json. Recover source from
published branch heads; `issues/frame-state54/build.py` and archived configs
provide inputs. Archived ELFs are experimental, not selected. Source citations
use pinned published commits. Raw timing/native/build/test evidence is unchanged.

Rounds52/53 are also closed and rejected. Their reports and archives remain
under their issue/evidence directories. Selected/upstream round53 refresh lives
under `BASE/pyjit-link-wave1/activation53/selected51-refresh-{all29,hot}-upstream`.
That historical all29 mean is1.156831; current selected54 results are in STATUS.
The selected checkpoint51 report retains direct prior50/golden39 comparisons.

## Operating rules

Never overlap quiet timing/profiles with builds,tests,running GDB or bulkcleanup.
Root owns generation, integration, native execution, commits and publication.
Use `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B` for host
scripts. Current final build/source must remain clean and stable through evidence.
Loom mirrors canonical docs under
`http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.
Authenticated GitHub publication uses gh's HTTPS credential helper without
changing origin. Preserve old golden tags and unrelated controls/caches/worktrees.

Never stage/reset/delete the preexisting report/log/lock:
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log`,
`pyjit/issues/checkpoint-closure/w5-release.lock`.
