# Coordinator handoff

Canonical documentation is this checkout on `weaver/python-jit`; its C source
is stale and must not be built. Start [goal](GOAL.md) →
[active plan](issues/interpreter-performance56/plan.md) →
[architecture](design/runtime-architecture.md) / [status](STATUS.md).
Follow [process](PROCESS.md). Do not restart completed runtime integration.

## Selected checkpoint

Selected56 is **883524c180e4486b47df77ee2276c9276c0ec32c**, branch
`pyjit/round56-golden`, tag `pyjit-golden-20261113-interpreter-performance`.
I is BASE/worktrees/call55-integrated, branch `pyjit/call55-integrated`, clean
at that head. Builds are BASE/call55-integrated-{release,debug}. Do not build C.
Astra's architecture/audit6036 was integrated as bd32753 and refined8cc069;
no new Astra inner-loop review is needed. Sol component peers and an independent
cold reviewer completed source/selection review.

Read the [report](issues/interpreter-performance56/report.md),
[all tables](issues/interpreter-performance56/benchmark-tables.md) and
[plan](issues/interpreter-performance56/plan.md). Root explicitly accepts five
broad regressions for the mostly-better incremental checkpoint; golden54 is
preserved unchanged as rollback/comparison. No parity or universally faster claim.

### Qualification and native identity

Runtime composition includes scalar graph/owner receipts, exact-tuple CALL_EX,
direct native completion entries, generator recipient admission, SHAPE PIC,
canonical container transport and exact-coroutine transfer. The logical-vacancy
repair0dcc→dab1 changes five consumers without granting NULL/read authority.
Fixture/generator repairs end at cb3; final 883 changes only LLTrace expectations.

Coherent full cb3 release/debug each run 55,023 tests, no crash. Release 588 modules
OK with only six sys_settrace errors; debug 587 with the same six plus stale
LLTrace. Final 883 debug LLTrace 3 pass; release 3 skip. Final 484-method debugger
checks per role reproduce exactly the six baseline failures. Five bootstrap
images, 27 compile probes/freezer build and both 117/79 module builds pass.
Full regeneration is product-byte-identical. No new quarantine; GIL-only.

Evidence: E56/qualification56/{check.py,commands.json,
final-fixture-identity.json,final-debugger-baseline.json}; full logs
E55/perf56-closure56-{release,debug}-{release,debug}-full.log. Final native:
E55/perf56-final56-native/python, SHA256
`2f46bcc126b8857095bd8b25a9256ad0f76fe65667e023e96edd4d7acd21f4cc`.
Final build/trace/jump/freezer commands use E55/perf56-final56-*.
Preserve the earlier failed qualification attempts; report explains repairs.

### Measurement

All final campaigns use883 with stable source/native identities:
E56/closure56-all29-{golden,upstream} (116 commands each),
closure56-confirm7-golden (84), closure56-perf-stat (24),
closure56-perf-record (8). E56/final-measurement/{check.py,commands.json,
analyze.py,numbers.json} owns execution and derived tables.
Geomean .984655/golden,1.141077/upstream. Six focus cases win every confirmation
pair; Raytrace loses every pair. Ratios/ranges are not confidence intervals.

E56/regression-diagnostic records 18 golden/candidate stat commands and 6 profiles;
its isolated capture.py is peer-approved, source hash85751251…195b2cac.
Raytrace instructions/cycles1.020394/1.039664; spectral_norm.972709/1.024014;
nqueens1.015206/1.039182. More work does not explain every regression; exclusive
cause remains open. Do not reuse modified inputs for sealed campaign validation.

Historical82e libc-copy attribution is under
E56/qualification56/libc-profile-analysis; its isolated symfs uses retained ELF,
not current native. All109 samples at libc0x19846d unwind to zero-filled created-
generator scratch. Do not transfer its 4.33% attribution to current profiles.
Current and historical ELFs, failed experiments and source branches are retained.

The rejected inline-list accessor is reverted; no diagnostic counters remain.
The closed COPY-web census is E56/multi-use-borrow-census. Conditional as-built
patches were applied on selection; canonical designs now describe selected56.
Full M4, bitmap readers and IRSC2 remain separate unfinished qualification gates.

Next work follows the five prioritized hypotheses in the active plan, with a
parallel accepted-regression lane. No native job remains active at handoff.
Root owns merged qualification/selection; workers propose/peer/implement small
increments and use focused tests before a coherent full gate.

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
| call55-integrated | pyjit/call55-integrated /883524c180e | Selected56; full diagnostics and final direct timings complete, six baseline debugger errors remain. |
| call55-bitmap | pyjit/call55-bitmap /0e918ddc25b | Both 117/79 builds. Release 34 focused pass; debug HomeRead and monitor/profile failures remain. No timing. |
| call55-ir | pyjit/perf56-absence-facts /0dccdf84069 | Vacancy fix source reviewed/integrated; generated-case closure complete in I. Prior value-replay e61a and IRSC2 branch3cfc retained. |
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
The selected/full-diagnostic limits are in status and the current56 report;
round54's report retains the baseline limits.

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
