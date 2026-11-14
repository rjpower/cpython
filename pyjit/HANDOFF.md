# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`; its C source
is stale and must not be built. Start [goal](GOAL.md) →
[active plan](issues/interpreter-performance56/plan.md) →
[architecture](design/runtime-architecture.md) / [status](STATUS.md).
Follow [process](PROCESS.md). Do not restart completed runtime integration.

## Active integration

Astra high completed the architecture/audit in 6036b9a5ce7, integrated here as
bd32753b5a0 and refined in 8cc069dc884. Read the [proposal](design/proposals/interpreter-performance56.md)
and [review](issues/interpreter-performance56/review.md). Sol frames, IR and GC
workers own generator-recipient/call, shared-value, and polymorphic-shape lanes.
Root owns products, native execution, evidence, publication and selection.
Do not call Astra in the inner loop.

Current I source head is **ac4c7019f9b**, with logical-vacancy repair
0dccdf84069→dab1ffaed3d and eight qualification-fixture updates. Both builds
pass 117/79 under E55/perf56-vacancy-qualification. The three unchanged
admission/retirement regression methods pass per role under
E55/perf56-vacancy-smoke; five-module core verification is active. The unchanged-template
inline-list experiment remains rejected/reverted; its branch/evidence is retained.

Full release at7a21 completed55,023 tests:116 failures across13 modules,
576 modules OK. Debug was deliberately interrupted after128/675 files and5,310
tests when a real admission/proof mismatch was identified; it is not qualified.
Consumed scratch reads were accepted on three public routes, and16 full-extent
retirement checks lost EXACT. Source repair uses one logical-absence predicate
in five consumers without granting NULL/INIT/readability or weakening MAY_OWNER/
completeness. Existing semantic assertions remain unchanged. Targeted verbose
reproducer is E55/perf56-admission-diagnostic; complete source/unit diagnostics
are E55/perf56-qualification-verbose. Both runs remain sealed.

Magic/table pins, exact call-route names, unknown-producer controls and frame
sizeof formulas are corrected in ac4c7019f9b. Remaining generated-case failures
are in IR's source-review lane: three stale expectations, two mutations now
failing earlier and two minimal-analysis optional-observer lookups. Do not
paper over the source defects. Root owns generation/native reruns. Six known
sys_settrace jump errors remain explicit; no new quarantine is authorized.

Last complete timing/PMU belongs to
**82e857384f899fbf5f9d2c5f68852250de84c31e**; it is not a qualification result.
Restored7a21 source tree equals82e857, and retained .text/.data.rel.ro match.
The new cold admission fix changes source again, so final timing must use its
new native identity after qualification.
It includes the scalar replay repairs, exact-tuple call preparation, direct
completion entries, generator-recipient admission, managed SHAPE PIC, canonical
list/tuple transport and exact-coroutine owner transfer. Products are committed;
both native builds complete 117 modules/79 shared. All 154 focused methods pass
per role, plus five bootstrap and 27 compile probes. Formats are unchanged.
Actual merged-source/product reviews and an independent cold source review
found no blocker. Full lifecycle qualification remains pending.

The latest two-pair hot6/golden screen is stable and error-free:
DeltaBlue .960183, generators .934380, coroutines 1.010237, Richards .720826,
comprehensions .975365, logging .951952. Coroutine/logging ranges cross parity.
All29/upstream mean1.151069 (seven wins); direct all29/golden mean.989410
(12 wins). Six-pair confirmation includes raytrace regression1.035032, with
one faster pair. Both controls/counters/profile campaigns are complete and
stable under E56/transport-await-*. Golden54 is unchanged. The [report](issues/interpreter-performance56/report.md) owns the
exact-head tables, prior screens, native costs and failures.

Native ELF/disassembly is retained under E55/perf56-transport-await-native,
SHA256 e9d36ff85b79b03eeeb3d75e86dd59320f78965e1407f3696cee2b16e4555725.
The coroutine scratch arm bypasses its helper and reference-count pair, but
adds 238 text bytes and 16 stack bytes. Do not call this a prologue removal.

At earlier f291, Richards .712132/golden and .892020/upstream, but logging
1.206807/golden blocked promotion. Actual warmed PIC counts are DeltaBlue
1,709,700 hits/0 misses, Richards 1,253,909 hits/13 misses; other hot cases,
including logging, execute no PIC selection. E56/pic-selector-diagnostic-f291
retains the separate diagnostic ELF, patch and outcomes. All diagnostic source
paths were restored, hashes verified and ordinary rebuilding passed 117/79.
No counters remain in current source; never time the diagnostic ELF.

The early 6a00 checkpoint has complete all29/upstream and matched PMU evidence
under E56/call-ex-*. Those measurements do not transfer to current source.
The initial COPY-web census closes at zero candidates across 21 files/426 CFGs;
its frozen inputs/parser reproduce the result. No compiler pass was added.

Next: qualify the logical-vacancy fix and fixtures, resolve generated-case
failures, then repeat full coherent qualification and final measurements. Preserve failed harness/fixture attempts and
source identities in evidence; the report records their corrections. If
logging regresses again, diagnose the current compiled call/transport paths;
its zero PIC count excludes extra selector-loop execution as the cause.

Source-only next packets use existing isolated worktrees. Full static M4,
IRSC2 and bitmap qualification remain separate. Root's common native wrapper
writes tags under E55; current native builds are shared and must stay serialized.

## Controls

BASE=`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
E55=`BASE/pyjit-link-wave1/call-model55`.
E56=`BASE/pyjit-link-wave1/interpreter-performance56`.
One GCC15.2 configuration, CPU15, hash0, JIT disabled. Preserve these controls:

| Role | Source under BASE/worktrees | Build under BASE | Head |
| --- | --- | --- | --- |
| Golden54 | frame54-cold-construction | frame54-cold-construction-{release,debug} |61015a9452d9069646fb5fede9ff963896833b89 |
| Prior51 | round51-golden | round51-golden-{release,debug} |0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254 |
| Upstream | upstream-3a440b6 | upstream-release-devlibs-3a440b6 |3a440b65695615e9fd9912342b79cdd54fced0d2 |

Golden54 modules117/79, upstream116/78. Control module logs:
`BASE/pyjit-link-wave1/frame-state54/cold-construction-initial-release-build.log`
and `BASE/pyjit-link-wave1/checkpoint-closure/checkpoint-6a0e4c12c17-final/upstream-build.log`.

## Frozen implementation and evidence

| Source under BASE/worktrees | Branch / head | Native state and next gate |
| --- | --- | --- |
| call55-integrated | pyjit/call55-integrated /ac4c7019f9b | Vacancy/fixture fixes integrated; both builds117/79 and three regression methods pass per role. Final full gate and new timing pending. |
| call55-bitmap | pyjit/call55-bitmap /0e918ddc25b | Both 117/79 builds. Release 34 focused pass; debug HomeRead and monitor/profile failures remain. No timing. |
| call55-ir | pyjit/perf56-absence-facts /0dccdf84069 | Vacancy fix source reviewed/integrated; generated-case closure active. Prior value-replay e61a and IRSC2 branch3cfc retained. |
| call55-pins | pyjit/perf56-coro-await-transfer /a852d11faf6 | Recipient, fixture and await packets integrated; focused native checks pass in I. Retained-EH80b preserved on pyjit/call55-frames-retained. |
| call55-gc | pyjit/perf56-inline-list-borrow /4a591c4b168 | Getter experiment rejected/reverted in I after repeated coroutine regression; prior PIC5976 and monitor/EH branches preserved. |

Historical measured I8232: E55/scalar-storage-hot-screen-{golden,upstream}; stable
identities/no errors. E55/scalar-storage-{release,debug}-build.log has117/79;
runtime checks23 and admission233 per role; freezer5+27 passes. Retained ELF
E55/scalar-storage-native SHA256
2aba9d15fc2092083917d8998083a59cf4e3d038c131801084b2c9b49827223a.
The [report](issues/call-model55/report.md) owns pair ranges and earlier evidence.

VALUE source007069 → I0df398, repair5e623 → Iccb96. Root full host gates:
E55/scalar-results-host (Storage.copy KeyError val), retry (full cases PASS,
metadata empty-PUSH_NULL family IndexError). Source peer3cfc808 fixes only
that empty family; new I1413a9 full-host gate passes.
Do not include IRSC2 just to pick up this isolated fix.

IRSC2 static packet order:071d1ef3c09,4c646c88ae4,9c297510ac3. It contains
compiler identities, ID-only recovery, explicit retained/protocol homes,
32-byte cold HomeView and static runtime/readers/tooling/fixtures. Magic64423;
IRSC2 header24B, recovery rows16B; no old nonempty fallback. Static handler
entry remains honest CONTROL3; the bitmap adaptation uses actual presence2.
Hot static frame112B and roots40B do not grow. Full maps and arbitrary
allocation remain incomplete. P80b is the separate presence-provider runtime;
never replace whole shared headers across those providers. Source reviews and
Clinic/AST checks passed, not full generation or native qualification.

B stale-reader integration corrected native-entry capability, delete protocols,
reused frame-base initialization, missing bitmap-header build dependency,
LIST/TUPLE cursor deoptimization and a debug-only raw absence assertion.
E55/bitmap-assert-closure-* builds both roles at 0e918. Current focused evidence:
E55/bitmap-reader-fixtures-corrected-*; release34PASS, debug other four modules
pass but HomeRead aborts at LOAD_COMMON_CONSTANT in a constructed template.
Earlier E55/bitmap-stale-lifecycle-release-focus.log reports 480 tests, seven
modules pass, monitoring/profile fail. Do not call it617PASS. Source diagnosis
suggests optional CALL self roles are lost by renamed C inputs; actual first
GDB diagnostic is still needed. GC's proposed reconstruction uses the current
uop's declared logical input position, not a nullable waiver.

## Native execution

Host scripts: `UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B`.
Root alone runs full generation, native builds/tests, GDB, profiles and timing.
Use E55 [run-checks.py](issues/call-model55/run-checks.py),
[check-freezer.py](issues/call-model55/check-freezer.py) and
[capture-native.py](issues/call-model55/capture-native.py) for durable manifests.
Full generation includes global labels. Candidate build preflights must match
clean source, native identity, magic, source Lib and strict module counts.
Header changes must invalidate all consumers; PYTHON_HEADERS now includes
pycore_home_bitmap.h in B.

Serialize native/generation jobs. During quiet timing, workers may read/edit
source but must stop host tests and heavy jobs; never mutate the measured tree.
Use immutable controls, unchanged workloads and stable before/after identities.
Do not multiply independent campaign ratios. Full suites belong at coherent
representation gates. Only existing GDB PrettyPrintTests is quarantined.
The selected/full-diagnostic limits are in status and round54's report.

The new PMU wrapper pyjit/issues/call-model55/capture-checkpoint-perf.py requires a complete all 29
repeat campaign and explicit exact candidate head. Do not reuse an old wrapper
with an embedded source identity. Retain ELF/disassembly before advancing builds.
Startup RSS is not a throughput gate. Reuse established storage/caches; do not
remove controls or another task's worktrees and outputs.

## Publication and preservation

Push committed canonical docs before guarded Loom mirrors and verify exact
readback. Artifact root is `http://127.0.0.1:7878/s/xzzlxx17/artifacts/`.
The active chain is goal → plan → architecture/status; historical reviews and
progress pages are not separate active plans. Cleanup keeps evidence in git
history and retained experiment outputs. Do not mark experimental source golden.

Authenticated publication uses gh's HTTPS credential helper without changing
origin. Preserve unrelated preexisting files
`pyjit/bench/results/checkpoint-6a0e4c12c17.md`,
`pyjit/issues/checkpoint-closure/ctypes-rawpin-build.log` and
`pyjit/issues/checkpoint-closure/w5-release.lock`.
