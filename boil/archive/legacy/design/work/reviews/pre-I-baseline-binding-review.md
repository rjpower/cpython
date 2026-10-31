# Checkpoint-I upstream baseline binding review

## Round 1: source and comparison concept

Verdict: **SOURCE_CONCEPT_CLEAR**, 2026-10-29. Reviewed primary
`pre-I-baseline-binding.md`, SHA256
`6bd23acaf3c1959461630d50c880201fd013ea5a99990a263f8257fa1f14c601`,
committed in `bd60e64095c2173f0c3e7875aa86d866af1e853b`.
No blocking concept finding. This is not clearance of a concrete setup,
configure/build, interpreter, import witness, test or measurement command.
The reviewer performed only source/Git/configuration/artifact reads and
hash comparisons. No author, native, runner or historical result was edited.

The ancestry claims match Git. Exact upstream is
`321925c92c9e67864cafa1566840927f6f07e90a`; retained baseline revision
`15e96bf639d6609b358d69daa4992644d42f9d48` adds only `boil/WORKLOG.md`.
The user's main checkout was clean at
`3a440b65695615e9fd9912342b79cdd54fced0d2`, with only the two stated Doc
differences from upstream. Neither fact establishes historical imported
bytes or grants ownership of the main checkout.

The retained baseline executable, Makefile, pyconfig.h and result JSON
match all four published hashes. Makefile `srcdir=../..` and `abs_srcdir`
resolve into the active primary fork. `config.status` and generated Python
sysconfig data agree. **Nonblocking wording precision:** the generated
sysconfig JSON's `srcdir` is an installed `/usr/local/.../config-...` path;
its `abs_srcdir` still points at the fork. The binding-gap conclusion is
unchanged, but not every generated `srcdir` scalar is literally `../..`.
WORKLOG's October25 records already describe the incompatible current Lib
and venv behavior; no execution was needed to rediscover it.

`baseline.json` records 78 instruction, 37 RSS and 35 elapsed rows, with
runner commit `41157098106ef8fec7940db6d38f7f623ab0b0d6` and October27
timestamps. It lacks complete source/native/import identities. This proves
the current environment is unsuitable as a bound checkpoint-I baseline;
it does not prove exactly which files every old run imported. Preserve
those artifacts and describe their limits without retrospective receipts.

The current runner confirms the stated gaps. `boil/bench/run.py:84–85`
removes PYTHON variables and sets hash seed0; `:257–267` uses `-S` and a
fresh pycache prefix but inherits cwd and other environment variables.
`build_info` (`:427–436`) records a version/git/hash-seed probe, while
`--update` (`:542–548`) checks path/label and hostname rather than current
source/product/workload identities. The benchmark driver explicitly loads
workload files by path (`boil/bench/internal/driver.py:27–35`); that alone
does not bind transitive standard-library or extension imports. Future
origin controls must reach the intended wrong-origin rejection, rather
than pass through an unrelated import failure.

The separately owned detached exact-upstream worktree, its own
`build/default`, and a new `checkpoint-i-upstream-bound` result identity are
appropriate. Preserve the main checkout, old baseline products and old
results. Root has separately authorized the author to create only this
managed source checkout and its ownership/source inventory; this review
adds no setup or native execution grant. Complete source content, symlink
targets and task ownership must be bound in the forthcoming setup receipt.

The proposed matched GCC/O3, non-debug, shared-GIL, non-tailcall,
stats/JIT/PGO/LTO/BOLT-off, retained-frame-pointer cohort is coherent for
attribution. Upstream supports explicit experimental-JIT disablement;
the fork's removed option must not be passed as though still supported.
Actual eval-unit flags and linked products must establish the effective
policy. Keep allocator/model differences recorded, required extension
backends fixed and timing uninstrumented. The separate stock-production
comparison correctly permits declared deployment-policy differences and
does not attribute their combined ratio solely to interpreter changes.

## Pending concrete setup and make-only review

No exact configure/build adapter or command plan was frozen for this round.
These are prospective closure requirements, not defects in a concept note:

1. Bind the newly owned checkout's full commit/content inventory, source
   paths and tool identities; reject unexpected source dirt or replacement.
   Preserve old worktrees and artifacts. New labels must not imply old
   measurement provenance.
2. Freeze exact configure commands, controlled environment/cwd and effective
   GCC/linker/target/shared-library policy. Admit configure and fixed `make
   -j4` through the normal machine-slot/build-lock mechanism with bounded
   timeout and cleanup. Account for build-time bootstrap executions as
   build steps, not test coverage.
3. Capture generated configuration/Makefile/header inputs after configure
   and require their exact bytes through make and cleanup. Bind complete
   source/compiler/configuration/native inventories before and after each
   transition. Capture all new native products and resolved aliases under
   the lock immediately after make, before any later qualification run.
   Do not reproduce the old-before-build/new-after-test provenance gap.
4. Stop after make/product capture. Import-origin witnessing, correctness
   qualification, runner binding changes and paired measurements require
   separate reviewed plans and coordinator grants. No timing under active
   qualification/build load; quiet timing windows and per-run identities
   remain prerequisites for a later comparison.

Append later checkout/plan dispositions here without replacing this round.

## Round 2: owned checkout clear; configure runner FIX_FIRST

Reviewed source setup `pre-I-baseline-source-setup.md`
`22aed66031945a77af6b86c2f4a0f4c497446ae42979622c87ac1e17251f8fd4`,
revised parent `0366dae2d9712e9e6778ce630dcf59bf644e64cd68875fd621891b0cf9b382e7`,
and primary `build/checkpoint-i-baseline-binding/` artifacts:

| Artifact | SHA256 |
|---|---|
| `source-inputs.json` | `d5099a3c3792495ed7ada27d8941fb5447c7a0e542c9c7100dfe96ebe36fcad0` |
| `ownership.json` | `92ef7189ef296025bac3a1b34d15fb17ec14a477bf65e78b9dd4c753f78143b8` |
| `tool-inputs.json` | `305f2c27497317ec9d470c298d54a2de77af1bc988c3c9fbf3ed076d49a1ff1e` |
| `fork-release-reference.json` | `12ac893f85125df69aff68b94f30607a55f5c0e7bbfdc36b7d9f6e545cc43e3a` |
| `phase_runner.py` | `5832a7a88cbe6ab9fe8d69d488cf55fe6dfcaef918e4214d122a7a3385f90922` |
| `plan.json` | `7be6e3ae62fcd7f41dea301e0d7b79b2c8db2d25a2573251680ff9ec3130e1d0` |

**SOURCE_SETUP_CLEAR.** The detached owned checkout is clean at exact
`321925c92c9e67864cafa1566840927f6f07e90a`. All 5,806 path/mode/Git-OID
entries agree with its Git index; all 5,802 regular-file content hashes and
four symlink-text hashes agree with actual bytes. Its `.git` points to the
managed worktree registration. No build directory or configure outputs
exist. The ownership record reserves only this source/build for the author;
main and historical baseline use are excluded. The revised parent closes
round1's sysconfig wording point and retains the original document identity.

**CONFIGURE_FIX_FIRST.** The configure flags and phase split are coherent,
but the orchestration cannot yet meet its declared admission and receipt
contract. No configure launch clears this round. Blocking findings refer
to the frozen runner above:

1. **Admission lifetime and duplicate submission.** Lines 133–158 acquire
   a machine-slot file only in the outer Python process, then call
   `subprocess.run` without inheriting its descriptor. If that outer process
   dies, the slot is released while timeout/gate/phase cleanup can remain
   active. There is also no nonblocking task/build admission lock before
   machine-slot acquisition. Concurrent submissions can take both slots
   and queue on gate_run's blocking build lock, contrary to the declared
   no-queued-same-build contract. Pass a verified slot/admission lease
   through the entire bounded child/cleanup lifetime and reject duplicate
   task submissions before they consume another slot.
2. **The internal entry has no lock proof.** `--locked` at lines 113 and
   121–126 directly enters `locked_phase`; lines 68–69 verify source/tools
   but never prove actual inherited build/slot lock identities or ownership.
   A direct invocation bypasses both machine admission and gate_run. Require
   the real inherited descriptors for this exact build and permitted slot,
   and reject missing/wrong descriptors before phase/evidence mutation.
   Hiding an argparse option is not an admission guard.
3. **Incomplete claimed compiler identity.** The frozen tool manifest has
   eleven top-level tools, but no `cc1`, `collect2` or `lto-wrapper`, despite
   the setup table explicitly claiming them. GCC driver bytes do not bind
   the compiler subprocesses that configure will execute. Add their actual
   resolved paths/hashes to the enforced tool inventory; retain the v1
   manifest rather than silently changing its claimed scope. Future header/
   library dependency binding remains a separate documented qualification
   obligation and need not be invented before configure.
4. **No final wrapper outcome/cleanup receipt.** The inner result and product
   snapshot are written at lines 98–103 before gate_run's cleanup. The outer
   call at line 158 only returns an exit code: it retains no timeout/cleanup
   result or final source/tool/product recheck. An inner zero result is not
   proof the complete bounded wrapper succeeded. Freeze the exact outer
   command, fixed environment and cwd, preserve its result, and verify final
   state after cleanup while task admission remains held. Bind inner logs,
   inventories and receipt hashes in that final outcome. Preserve failures
   and partial attempts without a success claim or overwrite/retry.

The author independently identified the first finding after freezing v1;
the coordinator agrees these are launch blockers and assigned source-only
revision plus harmless host lock/process controls. Those controls should
falsify missing/wrong inner locks, duplicate admission and outer death while
a harmless child still holds the lease, and prevent a failed/partial wrapper
from yielding a final success. They must not run configure or candidate code.

The actual proposed configure argument vector is supported by pinned source:
release/shared-GIL/non-tail/non-debug/stats/JIT/PGO/LTO/BOLT-off, retained
frame pointers and static libpython policy. `CFLAGS_NODIST` carries
`-ftrivial-auto-var-init=zero` to match current fork configure's supported
flag addition (`configure.ac:2016–2018`). The fixed phase environment clears
inherited user compile/loader/Python settings and sets CONFIG_SITE=/dev/null.
Actual O3, NDEBUG, source/build paths, eval-unit flags and optional modules
must still be reviewed from generated configure output; no runtime binding
or matched fork cohort exists yet. Configure's native conftest executions
are disclosed build setup, not benchmark or correctness evidence.

**MAKE_REMAINS_PENDING_CONFIGURATION_REVIEW.** The future `make -j4 all`
proposal correctly requires separately supplied configure receipt/product
hashes and must not chain automatically from configure. Its build-internal
bootstrap/freezing/sysconfig/checksharedmods executions are disclosed; no
separate test/import-origin/timing grant is present. Review its exact
post-configure commands, enforced unchanged configuration inputs and native
product capture before granting that phase. The configure-only ceiling is
1,200 seconds and admission limit 900; the future make ceiling is 3,600.
These are setup bounds, not elapsed performance measurements.

The reviewer executed no runner, host-Python control, configure, compiler,
candidate interpreter, test or benchmark. Round1 remains unchanged.

## Round 3: CONFIGURE_ONLY_PLAN_CLEAR

Verdict: **CONFIGURE_ONLY_PLAN_CLEAR**, 2026-10-29. All four v1 launch
blockers close in the refrozen source/protocol evidence below. This clears
the exact configure-only plan for the coordinator's command review and
grant; it does not execute or qualify configure. No make, candidate
interpreter, correctness test, import-origin witness or timing is cleared.
Rounds1–2 and the retained `history/v1/` artifacts remain history.

Reviewed primary `build/checkpoint-i-baseline-binding/` identities:

| Artifact | SHA256 |
|---|---|
| `phase_runner.py` | `d8531220d85d1ec1e88d450619ecd0327ec08bf6fedb00597e4c8fd67daa81f0` |
| `plan.json` | `419a918f9a23852eec1199e9d9e41f363c890dddc7bd299560773a3cfc508eb0` |
| `tool-inputs.json` | `53ae2f5f7a77d1992f30fb1038f74cdfa4be3ffd38ed6873028826bc73152900` |
| `configure-launch.json` | `a0ca9a7c929aa92a254db0c1acb1566e49e55ad4b78421b80213219180044949` |
| `controls-v2/results.json` | `0f78d66d62a15ac82e6fdbbbbba1ef8047df953c7fc12b159b9238036c677cce` |
| `controls-v2-extra/results.json` | `3daca13baa61bb1edd1c2d185decf863f8f612c7981a4ca4e4e45e6c532084aa` |
| `v2-provenance.json` | `e55167b4ebf2113466ffd3378ae30ec02320dea24c859675540cf5ec7c9c3506` |

The updated source-setup record is
`b833adbd82e1013395c82f9b392d56b4f2a6bd6826ce3ff65d3935748021a916`.
All 184 indexed artifact hashes and all fourteen tool payload hashes match
retained bytes. The three added GCC subtools are actual `cc1`, `collect2`
and `lto-wrapper` files under `/usr/libexec/gcc/x86_64-linux-gnu/15/`;
`verify` enforces them with the other tools. Source inventory/ownership
retain round2's identities. The actual upstream tree remains clean at exact
`321925`, without a build directory or configure outputs.

The new nonblocking task-admission lock precedes machine admission.
Descriptors 7 and 9 are explicitly inherited by a separate-session
supervisor, then timeout/gate/inner processes; gate_run owns descriptor 8.
`require_lock` checks actual inode identity, the inherited open description's
`/proc/self/fdinfo` flock and an independent-open exclusivity probe. Thus
an arbitrary open descriptor for the correct inode is insufficient. Inner
entry proves all three locks before phase/evidence mutation. Gate arguments
contain no test marker, so gate_run does not acquire a second machine slot.
The supervisor retains admission through gate cleanup and final checks.

The two copied-control drivers preserve the exact runner and use harmless
host fixtures instead of upstream native commands. Their thirteen retained
cases establish: a positive actual three-lock path; outer SIGKILL with task
and selected-slot locks retained and duplicate submission rejected; actual
gate sweep terminating the fixture's host child; named input/tool/source/
plan drift rejections; missing, unlocked and wrong-inode lock rejections;
and a real cleanup-time product mutation rejected despite successful gate
and inner results. The mutation receipt has `post_cleanup_verified=false`
and the exact `products changed during gate cleanup` failure. These are
protocol witnesses, not configure/native/runtime or timeout-coverage claims.

The frozen outer launch specifies the exact UV argv, minimal environment
and cwd. The runner rejects different arguments, cwd, UV-adjusted runtime
environment or resolved host Python; internal modes use the fixed host
Python directly. Phase commands receive the declared clean environment
and build cwd, with a task-owned pycache prefix. Source/input/tool guards
run before and after the phase and after gate cleanup. Products are captured
under the build lock before cleanup and compared with the post-cleanup
inventory while task admission remains held. Final lifecycle receipts record
the gate result and cleanup verification; missing/failed/partial receipts
cannot be presented as configure success. The phase evidence directory and
launch-start/result paths reject overwrite. The configured flags, 900-second
admission and 1,200-second configure ceiling retain round2's accepted scope.

**Required stop after configure:** preserve and jointly hash the actual
launch/inner receipts, command/supervisor logs, generated configuration and
both product inventories for independent configuration review. The lifecycle
receipt itself is not a self-contained hash manifest of those files. Review
actual O3/NDEBUG/frame/eval flags, compiler/source paths, optional modules,
conftest results and final cleanup success. The future make branch currently
checks the inner configure receipt/product hashes; its later concrete grant
must also bind accepted final launch success and exact configuration inputs
through make/cleanup. This remains a make-phase closure item, not permission
to chain make after configure. New native products must be frozen before any
later qualification or import witness.

No native phase or control was executed by the reviewer, and no owner file
was changed. External dependency binding, the matching final fork cohort,
actual startup/transitive imports and wrong-origin controls, correctness
qualification, benchmarks and stock-policy comparisons remain later work.

## Round 4: actual CONFIGURATION_CLOSURE_CLEAR; make still held

Reviewed the once-only coordinator-granted configure closure, not a new
configure invocation. Joint `configure-closure.json` is
`8908a1cc959d3828d5d50cda311e827400eef2b3bf79d92803d464cd12e49b02`;
final launch receipt is
`2dd59476670c92fec03cf1332be5b7ae7143304f6361ce91206aa21579be2124`;
inner result is
`1107b06e463851b6b5ea67af590331e4d03b2ed88fa1a71b285ab7c958a28b27`;
both post-phase and post-cleanup product manifests are
`379f76037cb827ba5ba5abd1ddb7cb4592566beca8900cfd2f490c4f34da703a`.
The updated source-setup record is
`bd45f6657b68038f33c8ca045d5c51fe2bc2dd96320cd945be80defb56f5bf10`.
The runner/plan/launch identities remain the round3 identities.

**CONFIGURATION_CLOSURE_CLEAR**, 2026-10-29. All joint-closure artifact
hashes and fourteen configured product hashes match actual files. Source
regular-file hashes and all fourteen tools were rechecked unchanged; Git
remains clean at exact `321925`. The product roster contains only the build
lock and thirteen generated configure files. No target `build/default/python`
exists and make has not run. Outer/gate/inner return codes are zero, all
three actual lock proofs are recorded, and final cleanup verification is
true with no failure. The empty supervisor log and retained command log
are bound, not inferred from the success summary. The 27.660-second configure
and 30.443-second launch/cleanup intervals describe setup turnaround only.

The generated Makefile binds both source paths to the owned upstream tree
and its own build directory. It has `/usr/bin/gcc`, release
`OPT=-DNDEBUG -g -O3 -Wall`, retained frame/leaf-frame pointers and
`-ftrivial-auto-var-init=zero`. Header/configuration evidence has debug,
assertions, stats, GIL-disabled, shared-library, tailcall, JIT, PGO, LTO and
BOLT off. Required array/math/json/pickle/expat/elementtree module states
are `yes`, meaning configured to build, not compiled/import-qualified.
The missing states for decimal/dbm/gdbm/readline/tkinter/uuid/bz2/zstd/ssl/
hashlib and disabled sqlite3 match current ROOTS default configuration.
They are distinct from later custom optional-module qualification receipts.

**Open comparison-policy decision:** upstream's actual `CFLAGS_CEVAL` is
empty, whereas current ROOTS default has `-fno-gcse`. Fork
`configure.ac:8075–8080` adds it for GCC computed gotos; pinned upstream
`configure.ac:8126–8134` does not. The common O3/frame/zero-init policy
matches, but the eval-unit flags are not identical. Record an explicit
matched-attribution versus per-runtime-policy decision before timing,
preferably before make if matched flags are desired. This does not falsify
the configure closure, authorize an upstream source edit, certify a final
fork cohort or convert this setup into a stock-production comparison.

The coordinator resolves this setup choice explicitly: preserve accepted
upstream configure and its empty eval-unit flags; report the fork's deliberate
`-fno-gcse` as a source-specific optimization. Common compiler/O3/frame/zero-init
policy is matched, not every per-unit flag, so no object-model-only attribution
is claimed. Before checkpoint-I parity is claimed, retain an upstream
same-toolchain `-fno-gcse` sensitivity result, or select a fully matched eval
policy from predeclared evidence. No extra configure/build is assigned now
solely for this decision. H1 parent/changed benefit still requires exactly
the same eval policy. This closes the setup choice while preserving the
later sensitivity/attribution obligation.

Make remains held for a new concrete executor/plan review and coordinator
grant. That plan must enforce this accepted final launch success and joint
closure, exact configuration bytes through make/cleanup, and complete new
native/alias/generated-product capture before later qualification. Existing
inner-only make checks are not sufficient launch authority. No reviewer
command executed configure, a compiler, candidate Python, tests or timing;
only the review record was changed.

## Round 5: MAKE_ONLY_PLAN_CLEAR

Verdict: **MAKE_ONLY_PLAN_CLEAR**, 2026-10-29, for the exact separate V3
plan below. This clears a first unqualified product-establishment build for
the coordinator's exact-command grant, not execution or native qualification.
Reviewed `pre-I-baseline-make-plan.md`
`af2eee87c055bc1d3da61ae2fb3f5d70dce026eb92a545319e43b3395b4dbcf5`
and primary `build/checkpoint-i-baseline-binding/make-v3/`:

| Artifact | SHA256 |
|---|---|
| `phase_runner.py` | `a3f271b50940a091f03245d7b3ef4179f1ca0e7b7eec82499ce5bc0b7567d9b4` |
| `plan.json` | `d08ddfa21196e0d8e64c018530c9029419516c0aca543ba9b0b327bdaafa5096` |
| `make-launch.json` | `74e124646fbdff98c41145e1995b3829b35c87429a0774a870db4c785e740f72` |
| `host-controls-v2/results.json` | `469dc1d3e15adeb9844d3c583be156e78df50d242d056a61c61c80e0184e6176` |
| `provenance.json` | `7abc6e476db5d7a4b4e8c29e43243c4eaaaa7ca31cccd1ebee16387db2cefd5c` |

All 396 indexed artifact hashes, plan-input hashes, fourteen tool payload
hashes, source regular-file hashes and actual fourteen configured-product
hashes agree. Upstream remains clean at exact `321925`; no target Python
exists at this review. Original V2/configure artifacts remain unchanged.

The V3 delta preserves the reviewed lock/supervisor protocol and adds the
required actual-configuration closure. `accepted_configuration` enforces
accepted joint closure `8908a1cc`, every raw artifact it binds, final launch
`2dd59476` with zero gate status/true cleanup/no failure, successful inner
receipt `1107b06e`, and product manifest `379f7603`. It requires matching
source/build identities and all accepted configuration bytes. The locked
pre-make check additionally requires the exact fourteen-file build roster;
later new outputs are allowed while every accepted configuration input
must remain unchanged. Those guards run after the command and again after
actual gate cleanup. This closes round4's final-receipt/configuration item.

The build-only parser rejects configure and wrong configure-identity CLI
arguments. The frozen UV outer command/environment/cwd, host identity,
nonblocking task admission, one normal machine slot, inherited lock proofs,
separate-session supervisor and no-overwrite evidence paths retain their
reviewed meaning. The native command is exactly `/usr/bin/make -j4 all` in
the owned build directory, bounded at 3,600 seconds after normal admission.
The make scope explicitly includes normal bootstrap/freezing, generated
sysconfig/platform/build-details and checksharedmods actions. These may
execute new build programs and import new extensions as build steps; they
are not standalone tests or import-origin qualification.

The eight retained copied-host cases use the exact V3 runner with harmless
host commands. The positive captures an additional text product. Failed
final configure status, joint closure drift, raw configure-log drift,
configuration-byte drift and wrong CLI identity reject at their intended
guards before the host command. Deliberate mutation after a successful host
command leaves inner return0 but `post_verified=false`; cleanup mutation
leaves inner success/gate0 but final `post_cleanup_verified=false` with the
named configuration failure. Both deny final success. The earlier scratch/
phase-directory collision is retained as a harness failure with no semantic
negative credit. Original V2's thirteen controls remain evidence of its
unchanged protocol bodies, not relabeled V3 runs.

Capture complete new product/alias bytes under lock after make, compare
post-cleanup state and jointly freeze final/inner receipts, logs and all
inventories before any later qualification. Stop on failure or partial
evidence. No smoke, separately invoked target probe/import witness, test,
benchmark, final cohort certification or parity follows from make success.

The coordinator explicitly rules that incomplete consumed SDK/header byte
closure does not block this first unqualified build. Bounded supplemental
pre-make hashes may be retained separately without rewriting V3 or copying
the SDK. Configure detection is not full consumed-header identity; remaining
dependency gaps must stay explicit and close before final timing/parity
cohort claims. No retrospective provenance is invented. The accepted empty
upstream eval-unit flags and the fork's disclosed `-fno-gcse` difference,
later sensitivity obligation and H1 matched-policy requirement remain as
ruled in round4; no sensitivity build is added now.

The reviewer executed no runner, host control, configure, compiler, make,
candidate interpreter, test or benchmark and changed only this review.

## Round 6: actual MAKE_CLOSURE_CLEAR

Verdict: **MAKE_CLOSURE_CLEAR**, 2026-10-29, for the completed once-only
coordinator-granted V3 build. This verifies product establishment and its
retained closure; it is not standalone correctness, origin binding, final
benchmark-cohort qualification or parity. Reviewed joint
`build/checkpoint-i-baseline-binding/make-closure.json`
`dddcf24dbea8b0eead4b4bec8eb09f6801837a62b1b696bdd4de8a25b5e54a42`.
The updated make work record is
`9eb17bf90669f07a77684b2a0f4ede60bcafdf06a9a3a660cec466cd02e3794c`.

The actual build uses unchanged runner `a3f271b5`, plan `d08ddfa2` and
launch `74e12464`. Its final receipt is
`241d86940daa38d0466281dcccb2dec90155c0277fe0b75986eb655240246940`,
inner receipt
`8539798e5b2d51476c7593cb2c877d0f2d0545d31ec5672e7f233c5dbfb3baa0`,
post-build/post-cleanup manifest
`ab41a2dfedb83d7976c4560209d3cb925770cd150d75bafafc0acc81e48f87db`,
and raw make log
`29b712caac5aaf8dc09d8208f7d28ad72ac5122112b980d74cf10ac39644e321`.
Outer/gate/inner statuses are zero, inner post-verification is true, final
cleanup verification is true and no failure is recorded. Actual command
and cwd remain the approved `/usr/bin/make -j4 all` and owned upstream build.

All 518 indexed V3 evidence payloads and all 584 actual build-product
payloads match the closure. Both retained product manifests equal the joint
product map, and an independent file roster agrees, including symlinks.
The 67 build-library alias texts and resolved targets were checked separately.
The 134 `.so` paths represent 67 module implementations and their aliases,
not 134 implementations. There are 570 new paths beyond configure's fourteen;
objects, bootstrap/testembed executables, libpython archive, generated frozen
headers/sysconfig/build details and extension products are retained.

All fourteen accepted configuration files and original configure closure
artifacts remain unchanged. Source hashes and fourteen actual tool payloads
also agree; upstream Git is clean at exact `321925`. New Python is
38,581,192 bytes, SHA256
`4565c7f92f126a1ad6dab0ae756ec57fec612103682062c25bf9cfdc7da576e7`.
The compile log retains O3/NDEBUG/frame/zero-init flags and the accepted
empty upstream eval-unit policy. Generated sysconfig source paths point
at the owned upstream checkout; that metadata is not a substitute for the
later actual workload import-origin witness.

The raw make log reports 116 checked modules: 37 builtin, 67 shared, one
not applicable, one disabled, ten missing and zero failed imports. Missing
SSL and the other configured optional backends remain explicit. These
are normal build-internal checks; no separately invoked correctness suite,
origin witness or timing is reported or credited. The retained command
scope stops after make, inventory capture and final cleanup. This is not
a claim of complete system-wide process lineage. Make took 61.092 seconds
and launch/cleanup 66.120 seconds, both setup durations.

The separately frozen pre-make supplement
`b960cb2e5dee062d456cd54eeb12b1c84db633c2be2364a122f390041f67a13f`
binds 341 SDK candidate paths/metadata with its stated missing candidates.
It remains candidate evidence, not consumed-header or loaded-library proof;
the accepted later dependency/cohort boundary is unchanged. So are the
recorded eval-flag difference, sensitivity obligation and absence of a
matching final fork cohort. Subsequent runtime qualification, transitive
startup/workload origins and wrong-origin controls need separate reviewed
plans and grants. No current parity or performance result follows.

Only this review was edited. The reviewer ran no build, compiler, target
interpreter, test, import witness or benchmark.

## Round 7: post-build launch FIX_FIRST, narrow JUnit guard

Reviewed `pre-I-baseline-postbuild-plan.md`
`5211e709f1e77226939dfd91677914ef18cebf491adce15cd85e44de06421b88`
and `build/checkpoint-i-baseline-binding/post-build-v1/` runner
`1fe7dc9ab2d1e385c2a6923be86b50aad6d3624fe40f880519921a0fcabb3b19`,
plan `86afd0905050fb5ea031c6ca3971dee427d44a8488870b176340ae796a12c6de`,
launch `3dfdab7c0a47eceeebee438f8bd51250774054c72f03c0a8a0330267aa2b8661`,
verifier `91fe0447ca55abfe60ebb0f61d69c21e0c47af503cacaf7ad7877bdf02ba35d1`,
108-artifact provenance
`9faba737f23613345de85ff6908b95b2d68cb75f3bfdb21c818a04c910ec7394`
and seventeen synthetic host controls
`24dca02a1966916a091ac397e100810de604c618ae2c10c9c37cfad91dc469dd`.
All indexed hashes and explicit plan inputs match. No target command has
run, and no reviewer helper/compiler/interpreter execution occurred.

The fixed order, scope and provenance guards are coherent: one positive
diagnostic, three valid copied-file wrong-origin controls, then one fixed
18-module upstream regrtest transaction. The supervisor retains normal
one-slot admission and all three actual locks without a second test-marker
slot. Before/after/cleanup guards enforce accepted make/config/source/tool
evidence and all 584 native products. Clean target environments exclude
both PYTHON and _PYTHON injection except each intentional negative. Copy,
math and generated sysconfig fixtures match their actual upstream originals;
the adapter requires valid import/functional results and exact wrong-origin
errors, rather than accepting an unrelated ImportError.

The verifier binds explicit diagnostic files, verbose startup-to-exit traces,
bytecode/source correspondence and the live process's mapped ELF files.
The synthetic controls reach intended early/transient origin, loader,
configuration, trace, native-inventory and lock errors. This diagnostic has
its own script-directory context; no full benchmark-workload import binding,
consumed-header proof or timing follows. The requested correctness roster
is fixed in argv and uses unchanged upstream loader behavior. Its raw XML/
transcript provide observed coverage, not a new per-ID success journal.

**FIX_FIRST:** `qualify.py` calls its JUnit guard failed-or-empty, but it
only checks that at least one testsuite element exists and that errors/
failures are zero. A nonempty suite list with zero tests, or all skipped
cases, can receive PASS. Require nonzero actual executed cases and reject
empty/all-skipped synthetic JUnit at the intended guard. Preserve actual
counts/skips and selection inference; do not invent expected totals or a
new module-roster framework. The coordinator agrees this narrow correction
and host controls must precede a launch grant. No other launch blocker was
identified in this frozen package. Preserve this version and append the
refrozen disposition; no new native gate or repeat is required.

## Round 8: QUALIFICATION_LAUNCH_PLAN_CLEAR

Verdict: **QUALIFICATION_LAUNCH_PLAN_CLEAR**, 2026-10-29, for the refrozen
successor package only. The sole round7 blocker closes. The coordinator
must grant the exact launch; this review executes no target command and
credits no target diagnostic, wrong-origin control or correctness result.

The runner remains `1fe7dc9ab2d1e385c2a6923be86b50aad6d3624fe40f880519921a0fcabb3b19`,
origin witness `0d1d1057` and verifier `91fe0447`. New identities are:

| Artifact in `post-build-v1/` | SHA256 |
|---|---|
| `qualify.py` | `fac770a5b814ca0caa3a5db6b9a8ded4a26be911af5cb5f47d141c98db42a34f` |
| `plan.json` | `4c172485584e78ab51f73ae0884bee2185aa7133b32976edf071d0ac64945d0a` |
| `qualification-launch.json` | `e907ee72bf30ee2f2c29a3313a7584b7fdd3177739b65f38f8eefc8d3480f704` |
| `host-controls-v4/results.json` | `3e95782f3e6dfc146237cc450bc97cfaf7fe548385efb63eb006397ea8749558` |
| `provenance.json` | `339737472d48498b3db1019730f766815db40b8242d03e08f23c09c3c25fceaa` |

The updated work record is
`9883526dd43a8d07a45af4ab748e1ff8dd23beb28d2deaaa5c3d72edd2353f21`.
All 152 indexed payload hashes and explicit plan-input hashes agree. The
earlier reviewed package remains under `history-review-v1/`, with round7
findings intact. No `run/` target evidence directory exists at review.

The narrow `summarize_junit` addition requires a positive observed
non-skipped testcase count supported by the suite's declared tests minus
skips. Empty suite lists, zero-test suites and all-skipped suites reject;
errors/failures still reject. Four new synthetic JUnit cases prove those
three negatives and one valid executed positive, alongside the seventeen
retained parser/guard cases in the new 21-case host batch. These are host
controls, not target execution. Summary wording now distinguishes fixed
requested argv and unchanged upstream-loader selection inference from
observed JUnit names/counts; it makes no new per-module success-journal claim.

All previously reviewed target commands, valid copied wrong-origin fixtures,
exact expected errors, environment/cwd, supervisor/locks, native-product
guards and time bounds remain unchanged. The granted scope can be only:
one positive diagnostic, valid copy/math/sysconfig wrong-origin executions,
then one 18-module correctness transaction. Unexpected failures, wrong
control outcomes, native drift or partial/failing final cleanup stop it.
Actual receipts/counts/skips/maps and original source/config/tool/native
identities require subsequent independent review before any further phase.

The positive diagnostic's own startup/transitive imports and mapped ELF
files are its provenance scope. Full benchmark-driver setup/run traces,
consumed-header closure, final fork cohort, eval-policy sensitivity and
timing remain separate obligations. No new repetition, production-runner
edit, parity or performance claim is introduced. The reviewer changed only
this review and ran no runner, target interpreter, suite or host control.

## Round 9: first diagnostic stopped; retained failure verified

Reviewed `qualification-failure-v1.json`
`75b4999b121ab4069292c6ec83e8602f181f8e2ea4a9694f006b7a9628baff56`.
All 367 indexed evidence payloads and 584 current native-product hashes
match. The positive diagnostic target returned zero and wrote its report;
the host verifier rejected `UNKNOWN_MODULE_LOADER: pyexpat.errors`.
Outer/gate/inner statuses are one. Inner input postchecks passed, and the
retained post-cleanup inventory matches the accepted make inventory.
The final success flag correctly remains false; it is not qualification.
No wrong-origin control, correctness transaction, workload or timing ran.
Source/configuration/tools and original make/config closures are unchanged.

The actual report records `pyexpat.errors` and `pyexpat.model` with no file
or spec origin and a None loader. Pinned upstream
`Modules/pyexpat.c:2212` creates these specific children with `PyModule_New`,
registers them in sys.modules and adds them to the parent extension. The
parent itself has the expected built-extension origin. The failure exposes
an incomplete diagnostic classification, not an upstream runtime failure
or a valid wrong-origin rejection. Preserve the failure and give it no
positive/control/correctness credit.

The coordinator has authorized only a source-side successor classification
for these two exact children, bound to the verified parent extension bytes
and observed parent-attribute identity with the corresponding sys.modules
child. A blanket None-loader/origin allowance would weaken the verifier.
Review that successor and its falsifying host controls before any retry or
target grant. The reviewer executed no target, verifier or control here.

## Round 10: PYEXPAT_SUCCESSOR_LAUNCH_PLAN_CLEAR

Verdict: **PYEXPAT_SUCCESSOR_LAUNCH_PLAN_CLEAR**, 2026-10-29. This clears
the separately frozen `post-build-v2/` source successor for the coordinator's
exact retry grant. It does not turn the first failed attempt into a pass or
execute any new target command. Reviewed updated work record
`f71ac6e7cc0fff25c021b7a67377f8dc5f4813faa6d51b45f0cc4b5c0bcd9396`.

| Artifact | SHA256 |
|---|---|
| `phase_runner.py` | `35b8d365bcf80b70e8d4d384a2e086983aa626e0cd0937af537ffa3329183b1b` |
| `origin_witness.py` | `2381e2b747dc3d1e1209acff9822cc8f5342771326c3868b81a696b9b3035ec8` |
| `verify_origin.py` | `a7435dcc2505b66e96110a17691064746e3bfe7308e6bab010f3c3604cc814da` |
| `plan.json` | `7f201ec8a0949c60f937091e9510ca2277fc302529861ef34e0be94f71cb0686` |
| `qualification-launch.json` | `77b51ce01a76adbabef55deae3d2fce8b11c1fa22ccea7d7a94a0425cc871883` |
| `host-controls/results.json` | `9fce98d9ca47da729224e77ab4095a4a9daf9ad7d7fa55c3cffd5fd5af137da0` |
| `provenance.json` | `59f888285e80193055dfc8d4f9f38c3adda45aa0c2a5ed35d8ff72ddb0b08457` |

The qualifier remains `fac770a5`. All 63 new indexed payloads and explicit
plan inputs match. All 367 first-failure artifacts and 584 actual native
product hashes were rechecked unchanged. Source/config/tool identities and
the original make/config closures are retained. No V2 target `run/` exists.
The runner additionally requires the preserved failure closure's hash.

The reporter records each module's declared name and observes the actual
identity of each parent's `errors`/`model` attribute with its sys.modules
child. The verifier accepts absent file/spec-origin/loader metadata only
for the two exact names and exact declared names, with the exact two-entry
namespace roster and true observed identity. Their parent must have the
expected `pyexpat` extension path, ExtensionFileLoader and accepted bytes;
the normal manifest/path/hash checks still apply. Missing/unknown children,
mismatched parent/child identity and foreign parents/loaders remain errors.
This reflects the pinned C constructor, not a general namespace or None-
loader exemption. The retained failed report's only unknown-loader entries
are these two children; that read is diagnostic history, not a V2 positive.

The 28 synthetic host controls include a parent-owned positive and named
rejections for wrong identity, parent name/origin/loader, unknown child and
unlisted None-loader module, plus retained origin/trace/native/JUnit/lock
controls. They test parser/guard behavior without loading the target or an
extension. The intended target scope remains one positive diagnostic,
three valid wrong-origin controls and one fixed 18-module correctness run,
under the same reviewed environment/locks/timeouts/native pre/post guards.
All workload traces, timing and further qualification remain separately
held. Only this review was edited; no reviewer execution occurred.

## Round 11: ACTUAL_DIAGNOSTIC_AND_CORRECTNESS_SCOPE_CLEAR

Verdict: **ACTUAL_DIAGNOSTIC_AND_CORRECTNESS_SCOPE_CLEAR**, 2026-10-29.
Reviewed the granted V2 launch's actual joint closure
`qualification-closure-v2.json`
`058d9bfb9d46723f2cb7f1f064d857cc9c643b858532fa106982e74b3ace4b3e`.
The unchanged plan is `7f201ec8`; the exact launch is `77b51ce0`.
Outer/gate/inner return codes are zero, final cleanup verification is true,
and the inner input postcheck passed. Final launch receipt is
`a476ae69bbd9811130bfdbb4b8eb7affda76a7e8caae623b648f94bc75358269`;
inner receipt is
`b81f6cec70496854e1878e58c62952de789747403c8c4dedb13efc9fa405ebc0`.

The positive diagnostic accepted 111 recorded modules and 100 traced imports,
including the two narrowly classified pyexpat children. All three target
wrong-origin commands returned zero, imported their intended valid foreign
copy/math/sysconfig payloads, and received their exact expected WRONG_ORIGIN
rejection. Recorded deepcopy and sqrt results are valid. These are actual
target controls, distinct from the earlier synthetic host controls.

The single requested 18-module transaction returned zero. Its transcript
reports SUCCESS, 18/18 files, 5,145 tests and 109 skips. The retained JUnit
has 18 suites, 5,145 declared tests, 101 skipped testcase records and 5,044
non-skipped testcase records, with zero errors/failures. These are separate
reporting domains; no equality between transcript skips and XML testcase
skips is inferred. The fixed argv plus unchanged upstream loader supports
selection inference; this is not an independently instrumented per-module
success journal. Correctness summary hash is
`534fc796524ce46c5686028a431dca0d1b1cb34fff28ec11f31efa9d51ec0f99`.

The 584 current product hashes match the accepted make inventory, and the
before/after inventories agree. Source remains clean at exact upstream
`321925c92c9e67864cafa1566840927f6f07e90a`; all regular source entries and
14 tool hashes were independently rechecked. The closure retains the
original configure/make closures and 367 first-failure artifacts. Of 1,101
indexed V2 evidence files, 1,099 were independently reread and matched.
Two task-created test bytecode files are restored to owner-write-only 0200,
so this reviewer did not change their permissions or independently reread
their bytes. Supplemental access receipt
`df8dbb798d47b93a29e85c0f6ac74fa6276d1fb0c25b20f32201b83bd53cbe19`
records the owner's temporary read permission, hashes, unchanged inodes and
restored modes. Its generic scope string says mode000; the actual per-file
0200 records are authoritative. This bounded evidence limitation does not
affect the reviewed logs, receipts, native products or source identities.

The earlier failure remains a failure. This completes only the granted
diagnostic, three origin controls and initial correctness transaction.
Actual benchmark-driver setup/run imports, consumed SDK/header identity,
eval-policy sensitivity, final fork cohort, quiet timing and parity claims
remain separate. No target command, verifier, test or control was executed
by the reviewer; only this review was edited.

## Round 12: DIRECT_DRIVER_ORIGIN_LAUNCH_PLAN_CLEAR

Verdict: **DIRECT_DRIVER_ORIGIN_LAUNCH_PLAN_CLEAR**, 2026-10-29. This is
source/host-evidence clearance for the coordinator's exact new grant, not
actual workload-origin or timing evidence. No target `run/` exists at review.

| Artifact in `workload-origins-v1/` | SHA256 |
|---|---|
| `phase_runner.py` | `bff785d1a898d28a435cbcac2b82cfc379873b7d7294360069c37a40b5a00117` |
| `campaign.py` | `389a541739e25ca836088a595432c843ca4dd0525207253829af8e75fd340e26` |
| `origin_trace.py` | `c398218c5ef1ca10081e991c2864cc6f2e319fc1d0d725c8384e2748286c0e99` |
| `plan.json` | `b5b38e63937f971e90648e5874048d06533cb28a6eff12ab34c74eb4b30af0f6` |
| `origin-launch.json` | `23bf93c55c6dea16ddac6431cf95ccc3031170b30691bd546fbe4b11cb428099` |
| `provenance.json` | `421e679f749bb26c8f9a2dbed574f4eb2450acaef69292dc79276d389900138c` |
| `host-controls-v3/results.json` | `d786c5cfc0e39a3827dbf0fd53ccf34cbb000ba862c22ab8e760d856b1352025` |

Initial parser `457f9f94`, plan `d87d6846`, launch `52e1c4d1` and 119-artifact
package `f0aa73cc` omitted generated build Python/data files from the accepted
Python file table. That contradicted the explicit sysconfig allowance and
could cause an avoidable verifier failure on valid generated imports. The
whole predecessor is preserved at `workload-origins-reviewed-v1`. The exact
runtime successor adds all 584 accepted manifest-bound products to that table;
the executable/.so subset and observed ELF-load requirements remain separate.
Three added host controls accept generated sysconfig code and reject changed
or unlisted generated files. No target execution discovered this gap.

All 151 successor indexed artifacts, 44 shared inputs, 15 tools, nine allowed
resolved external ELF files and 584 native-product hashes match. Upstream
remains clean exact `321925`. The accepted configure/make/qualification
closures, 38 readable critical qualification artifacts and cache-access
receipt remain launch guards. Old 0200 test caches are historical outputs;
the fresh phase neither rereads nor changes them. Seven existing supervisor
function bodies are unchanged, preserving actual inherited lock proof,
single-slot admission, bounded cleanup and final input/product verification.

The source plan fixes 50 registry-derived commands: all 29 application kernels,
eight memory rows and 13 existing call/string micro rows. The latter groups
are origin diagnostics, not additions to the performance score. The actual
unmodified driver loads command index7's exact workload, calls its setup and
run once in run mode, then returns. Existing loop counts/sizes remain intact.
Each child has its own fresh cache/temp directory and exact clean environment,
Python verbose tracing and `LD_DEBUG=libs,files`. Per-row bounds are 180 seconds
or 600 for memory rows; campaign bound is 3600. No automatic retry/overwrite or
production driver/registry edit is introduced. Failure retains the row command
receipt and partial traces and stops the campaign.

Python verification requires actual startup/workload code records, normal
cleanup and process zero, checks file/alias/resolved bytes, and rejects unknown
loaders or unbound bytecode without inventing a diagnostic-only completion
marker. Regex corpus helpers are among the 44 bound shared files. ELF parsing
distinguishes a search candidate from selected link-map/init/fini/open events,
requires the exact program and one process, hashes actual resolved loads and
requires observed ELF loads for Python extension traces. Six additional allowed
libraries have pre-make candidate hashes plus actual configured DT_NEEDED;
alongside the diagnostic's three, these are nine predeclared admissible files,
not nine assumed loads or a general /usr/lib allowance. The 68 retained host
readelf outputs corroborate dependency scope and do not substitute for runtime
loader traces. The 28 host controls cover the direct/shared-corpus trace,
search-versus-load, foreign origins/aliases, cache, loader, process, row/input,
qualification and missing-lock guards; synthetic ELF fixtures are not run.

Only actual receipts can establish the 50 rows' origins. Time-mode's later
json import and warmup/timed context remain separate, as do consumed headers,
eval-policy sensitivity, fork cohort, quiet elapsed results and the 2x target.
The reviewer executed no target, helper, parser, host control, compiler or test;
only this review was edited.

## Round 13: ACTUAL_50_RUN_MODE_ORIGIN_SCOPE_CLEAR

Verdict: **ACTUAL_50_RUN_MODE_ORIGIN_SCOPE_CLEAR**, 2026-10-29. Actual
joint closure `workload-origin-closure-v1.json`
`9e725b7ff8aa5b06ede10e82c101015ef6b11d497727e782f0eed1bdce56f0ac`
binds the granted plan `b5b38e63` and launch `23bf93c5`. Final receipt
`fbe642b1d00d7680f29929d78fd1947180fde8cacfe0b9cdcfd352c0e77323ea`
and inner receipt
`86bf78fb74342c9dfef006e7024f5080a9ca634944f3cb2720ce8caced699294`
report zero status, successful postchecks and final cleanup verification.
Before/after/post-cleanup native inventories agree. All 1,169 indexed evidence
files and 584 current native-product hashes independently reread and match.
Shared 44 inputs and 15 host tools match; exact upstream remains clean.
The accepted 38 critical qualification artifacts remain guards, and historical
0200 test caches were neither inputs to this campaign nor permission changes.

All 50 unique row receipts have zero return, no timeout and exact frozen
command/cwd/environment. They preserve the 29 application, eight memory and
13 micro diagnostic selection and sizes. All 50 origin results report
DIRECT_DRIVER_ORIGINS_CLEAR and all raw traces have normal audit cleanup.
Independent aggregation gives 117 trace-bound file entries and 118 imported
names. Raw spot checks confirm both regex corpus imports during setup, owned
_pickle for pickle/unpickle and owned _elementtree during XML setup, alongside
the actual libz/liblzma loader events. These are the real run-mode setup/run
paths, not a load-only diagnostic or a replacement benchmark driver.

The actual external resolved ELF union is precisely loader, libc, libm,
liblzma and libz with the predeclared bytes. The additionally admissible
libffi/ncursesw/panelw/tinfo files are not claimed observed. Search attempts
remain distinct from selected loads. The original failure, accepted diagnostic,
correctness and make/configure evidence remain separately labelled.

This closes origins only for these exact run-mode commands. It supplies no
time-mode json-tail/warmup/timed-context evidence, elapsed/RSS/box measurement,
consumed-header identity, eval-policy sensitivity, final fork cohort, parity
or 2x speedup claim. Timing still needs its own reviewed quiet-run plan and
coordinator grant. No target, verifier, parser or host control was executed
by the reviewer; only this review was edited.

## Round 14: QUIET_TIMING_TEMPLATE_SOURCE_CLEAR, no launch

Verdict: **QUIET_TIMING_TEMPLATE_SOURCE_CLEAR**, 2026-10-29. This clears
source design only. Fork runtime and quiet reservation remain null; binding
validation rejects before any target execution. An actual stable qualified
fork, its own source/products/ELF and reviewed qualification/origin receipts,
disclosed flags, quiet reservation and final exact launch need independent
review and a coordinator grant. No witness or timing is authorized here.

| Artifact in `quiet-timing-v1/` | SHA256 |
|---|---|
| `paired.py` | `bc4ee13e9b4ecc4fd7f43191ff355a1326bf9ac4821ced1824003e67c2e51cc8` |
| `phase_runner.py` | `89a1c73fe8f224dcfb49e09013220f3ec68160a897f68f89aaa8fe389e7c8d86` |
| `plan.json` | `01fcf21ca3cf9d4334d5887bb704593b5a5524c65e6824dd00bdb9bd916f27a0` |
| `launch-template.json` | `7a4f13521f572947aa2d9628cd76da00a2b07e3e307b1c778150470b0a48027a` |
| `provenance.json` | `c76823a2bfb29674d4f2147f1d8b2d3bec4a041719c14107e17366018f5165fa` |
| `host-controls/results.json` | `9e32a5b91287c0123d563c8e4e8a97794ad05990e799bf842cb175bd87d6dc36` |

Initial runner `6d90090a`/plan `f6acbf29` checked source HEAD and listed bytes
without checking clean Git status. An added untracked import source could
change a later measured path while that check passed. The original package
is preserved at `quiet-timing-reviewed-v1`, provenance `85ee627e`. The narrow
successor factors the same byte/HEAD checks into `verify_source` and adds
actual clean porcelain status, exercised before targets, afterward and after
cleanup. New copied-repository controls accept clean source and reject
untracked Python additions and tracked drift. All 88 indexed artifacts match;
the 26 host-only controls run no target interpreter or benchmark module.

Pair collection reuses unchanged Context.command/driver and matching time
normalization. All 29 application workloads keep their loops and variants;
each runtime gets one fresh process, one warmup and five serial timed calls.
First runtime alternates with registry order. All raw values, order, command,
stdout/stderr and quietness observations are retained; errors/timeouts or
post-run load rejection stop without retry or a partial-score substitute.
Five within-process samples are an inexpensive estimate, not independent
process trials or a calibrated reliability threshold. Complete per-workload
medians/raw vectors and upstream-over-fork geometric mean require all 29 rows.
The score is explicitly a source-policy diagnostic, not parity, object-model
attribution or target attainment with unmatched eval flags or incomplete SDK
identity. Further repetitions need a question and bounded budget.

One named regex_compile RUNS0 time-mode witness per runtime executes setup
and one untimed warmup, then the shared driver's json tail and `[]`, using
that runtime's own source/native/ELF binding. It requires own json and _json.
The source argument uses the unchanged shared driver and its only declared
sys.path-changing workload; it is not per-workload time-mode tracing. Measured
children omit verbose/LD_DEBUG. CPU0 affinity, sibling8 condition, common
temporary filesystem, readable CPU-policy data and max-load guard are declared;
pinning alone does not prove quietness. The coordinator must reserve the window
and require no competing gates/builds before actual binding or collection.

The supervisor acquires both ordinary machine slots together, releases partial
admission before waiting and retains task/slot descriptors through cleanup.
The unchanged upstream gate's build FD8 is duplicated to FD11 in the host
bridge while the nested fork gate owns FD8. Actual inherited descriptions are
proved separately; neither nested command requests another test slot. The
host-only nested-gate control checks both build locks using harmless children.
Complete native maps and source/tool/shared inputs are checked before/after
and after both gate sweeps, with final failure/success receipts. This does not
claim immunity to unrelated host interference or supply missing fork evidence.
No native command or host control was executed by this reviewer.

## Round 15: early unlanded ce2ac binding only

Verdict: **BINDING_ONLY_SOURCE_CLEAR**, not an execution grant. The packet in
`build/checkpoint-i-baseline-binding/quiet-ce2ac-binding-v1` binds frozen
unlanded ROOTS `ce2ac8991eebb1bbb10698ff2d07a79bd4cba452`; it does not repair
or disposition the actual stress100k interpreter failure and HTTP unraisables.

| Artifact | SHA256 |
|---|---|
| `plan.json` | `1503a952b9502a899a9cb158e688ab223f9a40ef58c605c6e38ad6e4a8ebb815` |
| unchanged `phase_runner.py` | `89a1c73fe8f224dcfb49e09013220f3ec68160a897f68f89aaa8fe389e7c8d86` |
| unchanged `paired.py` | `bc4ee13e9b4ecc4fd7f43191ff355a1326bf9ac4821ced1824003e67c2e51cc8` |
| `launch-template.json` | `07a91e1641d724e2c68a8b6f353e29fe810a5b279a764613138620870d6d47fb` |
| `provenance.json` | `cbbdaef157b90dcdb6d9e508a99f2d50af35d3249a7fdff22366ef615d0b45a5` |

The reviewer checked all 158 indexed artifact hashes, all 727 current fork
product bytes, 6,320 regular source files and two source-link identities,
nine admitted external ELF byte identities, and the original expanded map's
286 resolved-target/link identities. ROOTS remains clean at ce2ac. The bound
release-normal result `7b57d7ec` and its three raw streams match their hashes:
564 modules, 51,132 tests, 3,458 skips and zero fallback are that receipt's
historical scope. Its reported 2,880 unpinned words remain visible. The metadata
explicitly distinguishes those runtime/CAPI journals from actual-driver
workload-origin evidence. Upstream's accepted binding remains unchanged.

The host capture's preserved descriptor-versus-SHA preflight failure is a
metadata error before any target execution. The corrected capture checks the
descriptor's SHA and explicit aliases; it does not describe a target retry.
Static DT_NEEDED/interpreter closure admits exact external files, not observed
loads. The fork's unresolved failures, unlanded state, eval-unit flag mismatch
and incomplete consumed-SDK identity remain explicit limitations. Any future
numbers would be an early source-policy diagnostic, without correctness,
LAND, parity, object-model-only attribution or target-attainment credit.

`quiet_reservation` remains null. The unchanged runner rejects it before
targets; this template cannot run as frozen. The coordinator must first
inspect terminal stress evidence and probe quiescence, reserve a quiet window,
and review the resulting exact launch. A ready GC repair takes priority. No
target command, witness, benchmark or host control was executed by this
reviewer.

## Round 16: terminal-failure binding, reservation still held

Verdict: **BINDING_ONLY_SOURCE_CLEAR** for successor plan
`6da2229505f171814b93e3f8fb8cbe42ce3add9e8a2e982cb629ab0d7a1e0a73`,
launch template `95ad26e2fab31392ecd1089e0d7646926d2d2a8dc74d079dfbe11567e698d2e8`,
provenance `be19d5c2c57e944d6f75a24fee10877bd2469284cac638ca015fa7f90a15e354`.
The preceding packet remains in `quiet-ce2ac-binding-reviewed-v1`.

All 161 indexed hashes match. Post-terminal product capture canonical-equals
the same frozen 727-path map. The terminal record `f5336434` binds actual
stress100k FAIL receipt `64448666` and its three raw streams; these are also
guarded by the unchanged runner through plan.inputs. The metadata records
native2, the 50,722 transcript-reported tests, interpreter assertion, twenty
minute trashcan timeout and HTTP ENV_CHANGED. It explicitly rejects a
qualified root aggregate because the killed GC worker lacks an end witness.
This is preserved failure evidence, not newly qualified coverage.

The runner and pair collector are unchanged. Null quiet reservation still
prevents targets; focused failure diagnostics take priority. Exact eventual
quiet admission and launch require a separate coordinator ruling. No native
command, target, host control, witness or timing was executed by this reviewer.

## Round 17: bounded quiet reservation proposal

Proposal `619b74b7bc238851b24a055fa2631e4bf499c894fc46bce41a5e7a2bb434ce8f`
is **PROPOSAL_SOURCE_CLEAR**, not launchable. The unchanged runner/collector
accept plan-only admission30s/native ceiling900s and explicit child hashseed0;
all 60 declared commands carry it (two untimed witnesses and 58 measured
processes). The earlier omission ran no timing. The proposal clearly assigns
reservation start/expiry to an immediate manual coordinator check, while
existing locks remain held through cleanup. It requires binding original
nativeFAIL2/harnessINVALID plus separate offline interpretation, the remaining
three terminal diagnostic receipts and a concrete quiescence reservation
through plan.inputs. Null reservation/final-hash placeholders remain; those
actual inputs and exact launch need coordinator review/grant. No template
redesign, new guard or reviewer execution is required or authorized.

## Round 18: final bounded quiet binding clear

Verdict: **FINAL_QUIET_BINDING_CLEAR** for plan
`e9a46e758bf9bba180f2c5c141278d3087ac5f0ba4ad74cfa3e1e046e8e4cec4`,
launch `939e1a59da03ac0557a165aaf4c7ac1c0c6b54115a9de0c724c453f1d30ee9ad`,
reservation `7f647dab643a6bb9e901b2fc6aee360df5c0b21f1e3dadc6b41514ad268b0f0a`
and provenance `7672ac2dd32670a8d184f1e82bfe59f79b1a9fed5e8567ab07996782f017e7c7`.
Indexed and input hashes match. Only reviewed reservation/inputs/child seed0/
admission30s/ceiling900s/status differ from the accepted terminal-bound plan;
all other plan fields canonical-equal. The four actual diagnostic receipts,
separate offline supplement, summary and raw streams are guarded inputs.
Runner/collector and exact launch environment remain unchanged. The root
manual start/quiescence check must occur before 22:32:41 UTC; the reservation
does not add runner expiry enforcement. All earlier failure and interpretation
limits remain. No target or host control was executed by this reviewer.

## Round 19: actual early elapsed diagnostic

Verdict: **ACTUAL_EARLY_TIMING_SCOPE_CLEAR** for closure
`68da9ae2865d95c69bb00ef0d90dc8bf46bbbb42a17ffb34149a87444fb13553`.
All 2,385 indexed hashes match. Final `fadaac6d` and inner `ade5ec63` report
zero return codes and verified post-command/cleanup guards; native maps agree.
The exact 29 planned workload names match the score; 58 fresh measured
processes retain all 290 positive values, five serial samples each, with
alternating first runtime. Both named RUNS0 shared-tail witnesses report
DIRECT_DRIVER_ORIGINS_CLEAR. Recorded measurement affinity is CPU0 with
sibling8 declared; governor/boost/frequency observations remain available.
Recomputing the upstream-over-fork geometric mean gives 0.82824309837826,
matching score `a4e2ff92`. Every workload/regression remains in the denominator.

This is the predeclared inexpensive source-policy diagnostic. It does not
qualify correctness, landing, parity, object-model attribution or the ~2×
target; the unlanded fork's full FAIL, incomplete per-workload time-mode origin
evidence, fork-only eval flag and consumed-SDK limits remain explicit. No
automatic repetition or tuning decision follows. No reviewer native command
or benchmark was run.
