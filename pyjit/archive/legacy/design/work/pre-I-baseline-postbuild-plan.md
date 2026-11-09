# Checkpoint-I upstream: qualification and import binding

Status: V2 diagnostic, three valid wrong-origin controls and one18-module
correctness transaction passed, 2026-10-29; independent actual review
`9bac61da` clears closure058d9bfb for that scope. The first launch's host-
verifier coverage failure remains frozen as75b4999b. All584 native products
and source/config/tools remain unchanged. Complete workload traces and
timing have not run. No production runner or upstream source changed.

Use these absolute roots in the eventual frozen command manifest:

```
U=/home/power/code/cpython/.worktrees/boil-checkpoint-i-upstream
B=/home/power/code/cpython/.worktrees/boil-checkpoint-i-upstream/build/default
P=/home/power/code/cpython/.worktrees/python-boil-the-ocean
Q=/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/checkpoint-i-baseline-binding/post-build-v1
```

These are explanatory substitutions, not environment assignments to an
already approved launch. Freeze expanded argv, cwd, environment, helper
bytes, input/output inventories and timeout separately before execution.
Reuse normal machine admission, one slot, task/build locks, supervision,
bounded children and final cleanup. Recheck upstream321925/source5806,
tools14, configure8908/config14 and make closure before and after. New
bytecode, temporary tests and reports belong under Q; do not modify the
accepted product snapshot. Keep old baseline products/results intact.

## Preserved first target transaction

The coordinator chose one positive diagnostic, three valid wrong-origin
controls, then **one18-module correctness transaction**. This supersedes
the earlier source-only4+14 staging draft (`8a84114c`); no earlier target
launch occurred. Keep workload traces separate until their exact roster,
loops and variants have been reviewed.

Task-owned source and fixtures live under Q. `qualification-launch.json`
contains expanded host-uv argv, cwd and minimal environment; it runs the
adapted V3 supervisor with phase `qualify`. The one gate invokes host
qualify.py, whose children are the four diagnostic commands and upstream
regrtest. Thus gate_run does not acquire a second machine slot by matching
`-m test` in its own argv. Unchanged FD/task/slot/build-lock bodies retain
their prior control evidence; no ROOTS-specific mechanism is added.

| Frozen source/launch | SHA256 |
|---|---|
| phase_runner.py | `1fe7dc9ab2d1e385c2a6923be86b50aad6d3624fe40f880519921a0fcabb3b19` |
| plan.json | `4c172485584e78ab51f73ae0884bee2185aa7133b32976edf071d0ac64945d0a` |
| qualification-launch.json | `e907ee72bf30ee2f2c29a3313a7584b7fdd3177739b65f38f8eefc8d3480f704` |
| origin_witness.py | `0d1d1057f2e602263cf21f02aa4ceeb15d080d3ba93597e9a2dbd6f0127f120c` |
| verify_origin.py | `91fe0447ca55abfe60ebb0f61d69c21e0c47af503cacaf7ad7877bdf02ba35d1` |
| qualify.py | `fac770a5b814ca0caa3a5db6b9a8ded4a26be911af5cb5f47d141c98db42a34f` |

Outer ceiling is3000seconds; each diagnostic child is bounded120seconds,
and regrtest2400seconds. Capture all584 native products before/after and
again after gate cleanup, requiring exact equality with make closure;
source5806/tools14/config14/make/configure raw evidence must also remain
unchanged. New caches, temporary tests and reports remain under Q. Stop
on any unexpected failure. Admission/supervision remain the accepted
ordinary protocol, with no target launch until independent exact review
and a coordinator grant.

## Actual first diagnostic failure and bounded successor

Independent round8 `QUALIFICATION_LAUNCH_PLAN_CLEAR` (`1ae2ef2e`) and an
exact coordinator grant preceded the once-only V1 launch e907ee72. Its
positive target command returned0, wrote a report and complete import
trace, then the host verifier raised `UNKNOWN_MODULE_LOADER: pyexpat.errors`.
The adapter stopped immediately. Outer/gate/inner returned1. The inner
input post-check passed and final gate cleanup completed with identical
pre/after/post-cleanup inventories, but final `post_cleanup_verified` is
**false** because the qualification failed; it is not a pass receipt.

`build/checkpoint-i-baseline-binding/qualification-failure-v1.json`, SHA256
`75b4999b121ab4069292c6ec83e8602f181f8e2ea4a9694f006b7a9628baff56`,
binds367 actual artifacts, all584 unchanged native products, source5806,
tools14 and original make/configure closures. V1 source, inputs, reports,
maps, logs and receipts remain untouched. No retry occurred.

Pinned upstream Modules/pyexpat.c's add_submodule (2212–2239) creates a
PyModule_New object, installs that exact object in sys.modules and adds
it as the parent's attribute. Its two callers (2368,2419) create exactly
pyexpat.errors and pyexpat.model. These are constant namespaces with no
spec/loader/file, not evidence that an external module was imported.
The actual first report contains only those two None-loader modules.

The separate source package is at
`build/checkpoint-i-baseline-binding/post-build-v2/`; use that as Q for a
prospective fresh attempt. The reporting helper now records the declared
child name and the actual identity test
`pyexpat.__dict__[attribute] is sys.modules[child_name]`. The verifier
accepts absent metadata **only for those two exact children**, with both
identity records true, the exact verified pyexpat extension origin/loader
and its accepted product hash. Wrong child identity, external or wrong
parent, unknown child and unknown loader reject. No general None-loader
or synthetic-module allowance is introduced. The18-module command and
three target negatives retain their scope and arguments under the fresh
Q; no extra build or broad gate rerun is needed to repair this verifier.

| V2 source/launch | SHA256 |
|---|---|
| phase_runner.py | `35b8d365bcf80b70e8d4d384a2e086983aa626e0cd0937af537ffa3329183b1b` |
| origin_witness.py | `2381e2b747dc3d1e1209acff9822cc8f5342771326c3868b81a696b9b3035ec8` |
| verify_origin.py | `a7435dcc2505b66e96110a17691064746e3bfe7308e6bab010f3c3604cc814da` |
| Unchanged qualify.py | `fac770a5b814ca0caa3a5db6b9a8ded4a26be911af5cb5f47d141c98db42a34f` |
| plan.json | `7f201ec8a0949c60f937091e9510ca2277fc302529861ef34e0be94f71cb0686` |
| qualification-launch.json | `77b51ce01a76adbabef55deae3d2fce8b11c1fa22ccea7d7a94a0425cc871883` |

V2 host-only controls pass28 cases, including valid parent-owned synthetic
namespaces, wrong child/parent identity, external parent, unknown loader,
unknown child and unlisted synthetic module. `host-controls/results.json`
SHA256 is `9fce98d9ca47da729224e77ab4095a4a9daf9ad7d7fa55c3cffd5fd5af137da0`.
These use synthetic host fixtures, not new target executions. Exact V2
independent review and coordinator grant remain required.

## Actual V2 qualification and independent review

Independent round10 `PYEXPAT_SUCCESSOR_LAUNCH_PLAN_CLEAR` (`b1e17586`) and
the coordinator's exact V2 grant preceded its once-only launch77b51ce0.
Positive diagnostic and all three valid wrong-origin controls succeeded;
each negative imported its specified valid copied module and was rejected
at the exact expected origin. Upstream's18-file correctness transaction
reported SUCCESS. Outer/gate/inner returned0, and final cleanup verification
passed. No workload or time-mode command followed it.

The joint closure is
`build/checkpoint-i-baseline-binding/qualification-closure-v2.json`, SHA256
`058d9bfb9d46723f2cb7f1f064d857cc9c643b858532fa106982e74b3ace4b3e`.
It binds1101 evidence files/aliases, all584 unchanged native products,
source5806/tools14, original make/configure closures and all367 preserved
V1-failure artifacts. The positive report contains111 modules and100
trace-observed imports; its native map includes this build's executable,
ten own extension products and the host loader/libc/libm, with resolved
file hashes. This remains the explicit diagnostic's scope, not complete
workload import coverage or consumed-header provenance.

The raw transcript reports5,145 tests,109 skips and18/18 files. JUnit
contains18 suites,5,145 declared tests,101 skipped testcase records and
5,044 non-skipped testcase records. Preserve these different reporting
counts; do not invent equivalence or independently mapped per-module
execution counts. The JUnit acceptance guard has meaningful nonzero
executed-case evidence. The transcript's18.9seconds is correctness setup
time, not a benchmark result.

Two test-created cache files were owner-write-only0200. The first evidence
inventory could not read them. Only their task-owned cache permissions
temporarily gained owner-read and were restored, including on errors;
no native/source/old-failure permissions changed. The explicit capture
receipt `qualification-cache-access-v2.json`, SHA256
`df8dbb798d47b93a29e85c0f6ac74fa6276d1fb0c25b20f32201b83bd53cbe19`,
records before/after device, inode, exact0200 mode and byte hashes, and
binds the joint closure. Its scope prose says mode000; the per-file0200
records are authoritative. This wording discrepancy is retained rather
than rewriting the frozen receipt. Independent round11 accepts the
closure with this bounded owner-capture-only cache evidence; it checked
the other1099 readable indexed files and unchanged native/source/tools.
Complete actual-driver origins and timing remain separately reviewed
and authorized phases.

## Correctness: one bounded baseline qualification

The unmodified upstream interpreter needs its own correctness evidence,
not repeated fork ROOTS/GC-verifier gates. Propose this single transaction,
cwd B, child ceiling2400seconds:

```
B/python -S -X pycache_prefix=Q/run/pycache-correctness -m test
  -q -j4 --timeout 600 --fail-env-changed --tempdir Q/run/tmp
  --junit-xml Q/run/tests.xml
  test_grammar test_opcodes test_builtin test_dict test_list test_tuple
  test_class test_generators test_importlib test_sysconfig test_json
  test_pickle test_copy test_math test_array test_re test_logging
  test_xml_etree
```

The first eight cover core execution and containers; the remaining ten
cover import/configuration and backends exercised by the existing suite.
They exist in the pinned upstream source. Use upstream regrtest's normal
worker dispatch, not a fork-specific worker adapter. Record actual test
counts, skips, environment changes and failures; optional-module absence
is neither a promised skip nor permission to suppress failures. Retain the
exact requested argv roster, full transcript, observed JUnit case/suite
names, counts, skips and resource exclusions. Selection of those18 modules
is an inference from fixed argv and upstream's unmodified loader; this
adapter does not independently map every JUnit class to a requested module
or claim per-module execution counts. The host adapter rejects a failed
process, failed/empty JUnit results, zero tests or an all-skipped report.
It requires a positive count of non-skipped testcase records supported
by each suite's declared test/skip counts. Stop on failure and diagnose before timing.
Build-internal checksharedmods' zero
failed imports is separate evidence. Regrtest deliberately creates foreign
temporary modules; the benchmark origin allowlist below must not reject
those correctness fixtures.

## Origins: actual startup and complete workload execution

Use an explicit environment allowlist: PATH=/usr/bin:/bin,
HOME=/home/power, LANG=LC_ALL=C.UTF-8, TZ=UTC and PYTHONHASHSEED=0.
Exclude PYTHONHOME, PYTHONPATH, virtual-environment injections, all other
PYTHON* and **_PYTHON*** variables, LD_PRELOAD and LD_LIBRARY_PATH. `-S`
avoids site startup; it does not isolate all import/configuration inputs.
Current `boil/bench/run.py` strips keys beginning PYTHON but retains
`_PYTHON_SYSCONFIGDATA_NAME/PATH`, which upstream sysconfig reads. A clean
outer allowlist closes that input without editing the production runner.

The prepared task-owned reporting helper emits sys.executable/
_base_executable, prefixes, sys.path, builtin names, sysconfig source/
build/compiler/configuration values, and each loaded module's name,
spec.origin, file and loader. It imports the required json/pickle/copy/
math/array/re/logging/ElementTree modules and explicit accelerators.
It records raw process maps while alive; the host verifier resolves and
hashes mapped ELF files after exit. This is file provenance, not a dump
of loaded pages. The positive command is:

```
B/python -S -v -X pycache_prefix=Q/run/positive/pycache
  Q/origin_witness.py Q/run/positive/report.json
```

The helper and host-side verifier are implemented for this diagnostic
scope only; exact independent review is pending. A final sys.modules snapshot alone misses
early/transient imports. Preserve startup-to-exit `-v` stderr as well;
builtin/frozen origins require binding to this executable and the frozen
source/build inventories, not treating the strings `built-in`/`frozen` as
filesystem paths. Unknown loaders, unresolved origins or incomplete traces
must leave the affected claim unqualified. Host verification checks every
reported file and trace-observed source/extension against exact source/
product/helper inventories, binds any cached code to its traced source,
requires startup imports and final cleanup markers, and rejects unknown
loaders. The three target negatives use unchanged copy.py, this build's
own math extension and unchanged generated sysconfig data under Q/fixtures.
They must actually import that valid wrong file and compute valid copy/
math results before the verifier's exact named WRONG_ORIGIN rejection.
Their expanded argv/environment/expected path are frozen in plan.json.

Twenty-one synthetic host-only controls pass at the frozen runner/verifier,
`host-controls-v4/results.json` SHA256
`3e95782f3e6dfc146237cc450bc97cfaf7fe548385efb63eb006397ea8749558`.
They cover positive source/main/maps, wrong origins/config/path/loader,
early transient source and extension traces, orphan bytecode, incomplete
trace/backend, deleted mappings, make-closure/native-product drift and
missing inherited locks. The four JUnit controls accept a non-skipped
executed case and reject a zero-test suite, all-skipped suite and absent
suites. Earlier host-control cohorts are retained;
they are not target wrong-origin runs. Host controls never execute the
new interpreter, tests or workload. Original make/config closures remain
unchanged.

The first reviewed package's runner/qualifier/helper/plan/launch/provenance
bytes are preserved under Q/history-review-v1. Its plan86afd090,
launch3dfdab7c, provenance9faba737 and document5211e709 remain historical.
The focused successor changes only JUnit acceptance, host controls and
the corresponding manifest/document identities; the supervisor, target
diagnostic/verifier, target argv roster and make/config inputs are unchanged.

For each declared benchmark/variant, also run the **existing unmodified
driver directly**, using its real module-loading, setup and run path:

```
B/python -S -v -X pycache_prefix=Q/pycache-WORKLOAD
  P/boil/bench/internal/driver.py run BENCHPATH LOOPS 1 [VARIANT]
```

Freeze the actual workload list, loops, variants, cwd P and absolute paths
before grant. This is the command emitted by Context.command with `-v`
added. Retain the full import trace through cleanup. An import-only pass
is insufficient: xml_etree imports ElementTree and selects its accelerator
inside setup. The driver loads workloads as `_boil_bench_module`, calls
setup once, then run once. Existing semantic checks remain useful but are
not a universal benchmark oracle. Origin executions produce no timing
claims; large memory workloads need their normal resource bounds.

Accept benchmark/driver/reporting files only as named, frozen shared
inputs. Every other filesystem-backed stdlib origin must be in U/Lib;
every extension and generated sysconfig origin must be in B and match
the accepted product inventory. Resolve aliases and compare bytes, not
just path prefixes or version strings. Bind imported bytecode to source
and use task-owned caches; reject unexplained external modules. The
startup witness has its own script-directory context; direct driver
traces establish the actual benchmark script-directory context separately.

## Native dependencies and negative controls

Read ELF DT_NEEDED/RPATH/RUNPATH with host readelf for the executable and
the actual imported extension products. Preserve tool identity and raw
output. A separately declared `LD_DEBUG=libs,files` trace of direct driver
commands can show resolved loader paths from startup; the reporting
helper's process maps supplement it. Hash resolved mapped library bytes,
loader and aliases. Do not substitute ldd output or config detection for
actual runtime binding; unresolved/deleted mappings remain gaps, and
kernel/vDSO identity is host metadata rather than a normal file hash.

The pre-make SDK supplementb960 contains341 configured candidate paths,
GCC15 include roots, selected headers/library aliases and package/loader
metadata. It is not proof of every consumed header. The build command
log identifies selected flags/libraries but not a complete transitive
header closure. Review dependency extraction or a final cohort rebuild
if that stronger claim is required before parity. Preserve this gap;
do not retrospectively label candidate hashes as consumed bytes.

Before accepting an origin verifier, falsify it with valid wrong origins:

- Copy unchanged upstream copy.py outside U, intentionally inject that
  directory for a deepcopy workload, and require a named origin rejection
  after a valid import rather than accepting an unrelated ImportError.
- Copy this build's own compatible math extension outside B. Require a
  path/manifest rejection even when its bytes and ABI match; do not load
  an unrelated fork extension to demonstrate the guard.
- Inject valid copied sysconfig data outside B using the two _PYTHON
  sysconfig variables. Require config-origin rejection despite a correct
  interpreter version. Restore the clean allowlist for positive cases.

These controls and their fixtures are proposed qualification executions,
not host-only controls already run. Freeze each control's expected named
failure and retain raw evidence. No production benchmark-runner change is
needed to make the initial clean-environment/origin distinction explicit.

## Timing remains a separate decision

Correctness success and accepted import/native provenance qualify this
baseline environment; they do not establish fork parity. Bind the final
fork source/build/tool/config/import cohort independently. Upstream's
accepted empty CFLAGS_CEVAL differs from fork's deliberate -fno-gcse;
common compiler/O3/FP/zero-init matches do not erase that difference.
Before parity require an upstream sensitivity or a predeclared fully
matched eval cohort selected from evidence. No extra build is granted.

Later elapsed comparisons reuse the established runner and workloads in
a quiet, declared window, preserving per-run values, warmup, startup vs
warm execution, uncertainty and regressions. Current build_info records
version/Git/hashseed only, and --update checks path/label/host; a bound
external launch/result receipt is required rather than treating either
as a full environment identity. Py_STATS is mechanism evidence only.
Historical instruction ratios and this build's setup duration establish
no current elapsed parity.
