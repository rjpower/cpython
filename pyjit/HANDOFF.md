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

Measured early checkpoint 6a00c6a08d4 has complete all 29/upstream, hot six/golden,
18 stat and 6 record invocations at stable identities. See E56/call-ex-* and the
[current report](issues/interpreter-performance56/report.md). Transaction 12
methods pass each role. Initial COPY-web census has zero candidates across 21
corpus files/426 CFGs; frozen parser/metadata/input hashes reproduce the result.
No compiler pass is justified for that slice.

I now f6ef4a81a87 includes second replay469b→aa952, recipient0888→99f,
products31a3 and fixture maintenance d531→f6ef. Full host37 and actual-recipient
source checks pass; full regeneration and both 117/79 builds pass. The native
proof assertions now pass; the chained witness fails later because its synthetic image has f_lineno=None,
so assignment raises "lineno must be an integer" before the intended active-call
check. Actual denial is retained under E56/redirect-denial-diagnostic; IR owns a
valid target fixture. Nine completion/observer modules pass all 84 methods per
native role. Core selector omitted ReachedPhaseTests, covered by that second
completion run. E56/recipient-hot6-golden completes two balanced pairs with
stable identities/no errors: generator .949424, coroutine1.065409,
DeltaBlue1.022672, Richards .968734, comprehension1.027499, logging1.017075
(wide range). No current upstream/full qualification or promotion is claimed.

The earlier root source diagnosis was confirmed in actual code: legacy transfer
erased untouched scratch facts after a complete normal graph. The reviewed
repair makes that graph authoritative, with conservative fallback for unknown
source. E56/scalar-normal-legacy-gdb* records unsuccessful debugger attempts;
the second timed out during startup and supplies no native confirmation.

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
| call55-integrated | pyjit/call55-integrated /f6ef4a81a87 | Both 117/79 builds; proof passes, synthetic line-target fixture pending. Completion84 passes per role; hot6/golden mixed, generator .949424. |
| call55-bitmap | pyjit/call55-bitmap /0e918ddc25b | Both 117/79 builds. Release 34 focused pass; debug HomeRead and monitor/profile failures remain. No timing. |
| call55-ir | pyjit/perf56-value-replay /469b5682289 | Second replay integrated; shared tuple/list fact migration in progress. Frozen IRSC2 packet remains on pyjit/call55-ir at3cfc808. |
| call55-pins | pyjit/perf56-generator-recipient /d531d993d30 | Recipient and fixture packets integrated; exact awaitable transfer proposed. Retained-EH80b preserved on pyjit/call55-frames-retained. |
| call55-gc | pyjit/perf56-polymorphic-shape /323cb4da34a | Reviewed PIC frozen; no products/native. Prior monitor/EH branches preserved. |

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
