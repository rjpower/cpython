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
