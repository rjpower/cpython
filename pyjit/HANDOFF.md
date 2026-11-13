# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`; its C source
is stale and must not be built. Start [goal](GOAL.md) →
[active plan](issues/interpreter-performance56/plan.md) →
[architecture](design/runtime-architecture.md) / [status](STATUS.md).
Follow [process](PROCESS.md). Do not restart completed runtime integration.

## Active integration

Astra high completed the combined design/audit in 6036b9a5ce7, integrated here
as bd32753b5a0. Read [proposal](design/proposals/interpreter-performance56.md)
and [review](issues/interpreter-performance56/review.md). No native evidence was
produced by that source review. Sol frames, IR and GC workers now peer the
call/borrowing, value/action and specialization contracts against actual sources
before bounded implementation. Do not call Astra in the inner loop.

Root admitted isolated empty-family fix 3cfc808 as I1413a9fe218. Its new host
gate E55/scalar-results-host-empty-family passes 33 methods and full cases,
metadata and contracts emission. The reused driver's final message incorrectly
says thirty-two; unittest's actual count is 33. Full generation passes under
scalar-results-qualified; product review precedes commit and native build.

The launch brief/manifest remains under E56. The active plan keeps full static
M4 and bitmap qualification separate from ordinary value/call improvements.
Root owns full generation, native evidence, publication and selection.

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
| call55-integrated | pyjit/call55-integrated / 1413a9fe218 | Shared builds still 8232dd8adde. VALUE/error-copy and isolated empty-family repair pass 33 narrow methods and full cases/metadata/contracts; full generation passes; product review pending. |
| call55-bitmap | pyjit/call55-bitmap /0e918ddc25b | Both 117/79 builds. Release34 focused cases pass; debug HomeRead aborts in a synthetic image. Monitoring/profile failures remain. No bitmap timing. |
| call55-ir | pyjit/call55-ir /3cfc808d8eb | Clean; includes frozen IRSC2 compiler/readers/runtime9c297 plus separate empty-family fix. No native/full generation. |
| call55-pins | pyjit/call55-frames-retained /80b3de991c6 | Clean source-reviewed presence-provider retained runtime; no native. Earlier separate fixes are already integrated into B. |
| call55-gc | pyjit/call55-gc-monitor-read-roles /0b447e8235b | Clean source-only monitoring diagnosis. Compiler bf2f44a2973 and reader6ca122f3135 preserved on pyjit/call55-gc-retained-eh. |

Latest measured I8232: E55/scalar-storage-hot-screen-{golden,upstream}; stable
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
