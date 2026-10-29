# Checkpoint-I upstream: owned source and proposed build phases

Status: owned source setup, reviewed configure and exact **make-only**
execution completed, 2026-10-29. Products are frozen and independent
`MAKE_CLOSURE_CLEAR` accepts the closure. **Standalone qualification, origin witnesses and
benchmarks remain unapproved and unperformed.** Make included its disclosed
bootstrap/freezing/sysconfig/checksharedmods interpreter actions.
The coordinator authorized this detached comparison-source exception;
it is not an optimization step branch, native change or performance claim.

Current make closure is `dddcf24dbea8b0eead4b4bec8eb09f6801837a62b1b696bdd4de8a25b5e54a42`.
The [make work record](pre-I-baseline-make-plan.md#exact-granted-execution-and-frozen-products)
records exact grants, receipts, product counts and remaining provenance
limits. Earlier phase descriptions below retain their historical scope.

## Completed setup and preserved inputs

Before creation, the proposed path did not exist, Git registered no
upstream source tree there, and the source filesystem had222GiB free.
Created only the repository-managed detached checkout
`/home/power/code/cpython/.worktrees/boil-checkpoint-i-upstream` at
`321925c92c9e67864cafa1566840927f6f07e90a`. Its5806 tracked entries match
Git and have independent SHA256 content/link inventories. At that initial
source-setup checkpoint status was clean, with no build directory, configure
outputs, interpreter or extension. The later approved configure created
the14 recorded configuration files; the separately approved make produced
570 additional files/aliases. Tracked source stays clean.
Owner is `/root/string_design`, coordinator `/root`; reuse by another task
requires a handoff. Main and all historical baseline builds/results remain.

Evidence lives outside tracked upstream sources, at primary
`build/checkpoint-i-baseline-binding/`. The proposed new build is inside
the owned upstream tree at `build/default`, label
`checkpoint-i-upstream-bound`. No old executable/extension is copied.

| Evidence | SHA256 |
|---|---|
| `source-inputs.json`: exact commit,5806 tracked paths/Git modes/OIDs/content hashes and link targets | `d5099a3c3792495ed7ada27d8941fb5447c7a0e542c9c7100dfe96ebe36fcad0` |
| `ownership.json` | `92ef7189ef296025bac3a1b34d15fb17ec14a477bf65e78b9dd4c753f78143b8` |
| V2 `tool-inputs.json`:14 actual entries including resolved GCC/make/binutils/shell/timeout/uv/pkg-config/host-Python and cc1/collect2/lto-wrapper | `53ae2f5f7a77d1992f30fb1038f74cdfa4be3ffd38ed6873028826bc73152900` |
| `fork-release-reference.json`: ROOTS1bfe default Makefile/config.log/pyconfig hashes and inspected policy | `12ac893f85125df69aff68b94f30607a55f5c0e7bbfdc36b7d9f6e545cc43e3a` |
| Proposed V2 `phase_runner.py` | `d8531220d85d1ec1e88d450619ecd0327ec08bf6fedb00597e4c8fd67daa81f0` |
| Proposed V2 `plan.json` | `419a918f9a23852eec1199e9d9e41f363c890dddc7bd299560773a3cfc508eb0` |

The selected `/usr/bin/gcc` resolves to
`/usr/bin/x86_64-linux-gnu-gcc-15`, SHA256
`b5f1b773a7c733738352000c92a077dc5852a1a2fc6d836b1e411be1e9ec5f88`.
Current fork release's config.log identifies GCC15.2.0 Ubuntu15.2.0-16ubuntu1,
and Makefile uses gcc. Current compiler bytes are a prospective shared
identity, not a recovered historical compiler-product receipt. The future
fork timing cohort must bind to the same actual toolchain.

## Concrete proposed configure phase

`plan.json` stores the exact argument vector and a fixed environment;
its final hash is reported with this draft. Configure command:

```
/home/power/code/cpython/.worktrees/boil-checkpoint-i-upstream/configure
  --with-lto=no --disable-optimizations --disable-bolt --enable-gil
  --without-pydebug --disable-pystats --without-assertions
  --enable-experimental-jit=no --without-tail-call-interp
  --with-frame-pointers --disable-shared
  CC=/usr/bin/gcc AR=/usr/bin/ar RANLIB=/usr/bin/ranlib
```

These options are present in pinned configure.ac. Default GCC OPT supplies
`-DNDEBUG -g -O3 -Wall`; actual generated Makefile/configuration must confirm
that after configure. Environment fixes PATH to `/usr/bin:/bin`, locale to
C.UTF-8, timezone UTC, CONFIG_SITE=/dev/null, empty user CFLAGS/CPPFLAGS/
LDFLAGS/MAKEFLAGS/PKG_CONFIG_PATH, host PYTHON_FOR_REGEN=/usr/bin/python3,
shared uv cache and no uv Python downloads. It sets
`CFLAGS_NODIST=-ftrivial-auto-var-init=zero`: current fork configure adds
this unconditionally, so a matched attribution cohort must carry it on
upstream too. Removing/changing that policy requires a separate matched
cohort decision; this setup does not edit fork configure. Keep retained
frame pointers and source-specific eval-unit policies explicit.

Configure gets1200seconds, under the owned build lock and one of the two
normal machine slots. It may run its normal compiler feature checks and
host regeneration-Python version detection; those are part of a future
configure grant, not already executed evidence. Freeze all produced config
files, including Makefile/pyconfig/config.log/config.status/Modules setup,
and stop. Independently review generated actual flags, source paths,
compiler/config manifests and optional-module availability **before any
make grant**. No interpreter smoke or binding witness is added here.

## Concrete proposed make-only phase

Only after that separate review/grant, run `/usr/bin/make -j4 all` in
the owned build directory, ceiling3600seconds, using the same frozen
source/tools/environment. The wrapper requires exact configure result
and product-manifest hashes as additional arguments; it rejects altered
configured products or any prior build-phase evidence directory.

Important execution scope: upstream `all`/`build_all` necessarily invokes
build-internal `_bootstrap_python`/freezing, generated sysconfig/platform/
build-details and `checksharedmods` extension imports. A make-only grant
must explicitly include these normal build actions. It is not literally
zero execution of newly built code. It includes **no separately invoked**
smoke, test suite, post-build import-origin witness, native probe or timing.
Those remain later assignments even if make succeeds.

The runner borrows unmodified primary `boil/tools/gate_run.sh`, SHA256
`44b689c0492544aab075dcbbabb4dfed168309d8e7818ca1039e2c5f74b777c5`.
V2 first takes a nonblocking task-admission lock, rejecting duplicate
same-build submission **before** taking a machine slot. Exactly one
standard machine slot is acquired; the unmodified gate wrapper does not
take another for these non-test commands. A separate supervisor inherits
the task/slot open descriptions and survives an outer/uv interruption.
It holds them through timeout, gate cleanup and final outcome capture.
gate_run holds the build lock through command and sweep. The locked phase
requires actual inherited task/slot/build FDs, matching device/inode,
Linux fdinfo's exclusive FLOCK and independent-descriptor exclusion;
direct internal-mode invocation cannot substitute an unlocked FD.
Timeout bounds the actual gate/phase group. It records pre/post build-file inventories, links and
resolved target bytes, source/tool rechecks, raw command log and result;
failures/partial products are preserved, never overwritten or advertised
as success. Stage evidence/pycache stay in project build storage.
The supervisor writes a final launch receipt **after** gate_run's sweep,
rechecks source/tools/admission and compares post-cleanup products with
the locked phase's snapshot. A changed product rejects even if the command
and inner result returned0. Missing/failing final receipts do not qualify.

Prospective host invocation uses shared uv cache and
`uv run --no-project --python /usr/bin/python3 -- python phase_runner.py`.
Configure requires `configure --plan-sha REVIEWED_SHA`; make requires
`build --plan-sha REVIEWED_SHA --configure-receipt-sha REVIEWED_SHA
--configure-products-sha REVIEWED_SHA`. Configure subsequently ran once
under the separate reviewed grant below. **Make has not been invoked.**
The concrete `configure-launch.json` freezes the outer argv, minimal
environment and cwd; V2 additionally verifies the actual UV-adjusted
runtime environment, exact arguments and host interpreter. It launches
the inner host Python directly under the initial host-uv ownership,
preserving inherited locks without a second uv/admission layer.

## V1 findings and V2 host-only controls

Preserve original setup draft22aed660, runner5832a7a8, plan7be6e3ae and
tool manifest305f2c27 under `history/v1/`. Independent V1 review returned
SOURCE_SETUP_CLEAR / CONFIGURE_FIX_FIRST. V1 had no inner inherited-lock
proof, no duplicate admission guard, and could release its outer-owned
slot before surviving child cleanup; it also lacked the claimed compiler
subtools and a final post-cleanup receipt. No native phase used V1.
Its11-entry tool manifest omitted cc1/collect2/lto-wrapper; V2 binds the
three actual `/usr/libexec/gcc/x86_64-linux-gnu/15/` files explicitly.

Two preserved batches of **harmless host-only copied-plan** controls pass
13 named cases; neither executes upstream configure/make/target Python.
The positives use the actual frozen gate wrapper with an ordinary host
Python fixture, not the native phase commands. At the time of those controls,
the owned upstream source stayed unchanged and had no build directory.

- `controls-v2/results.json`, SHA256
  `0f78d66d62a15ac82e6fdbbbbba1ef8047df953c7fc12b159b9238036c677cce`:
  positive actual three-FD proof; abrupt outer SIGKILL with task/selected-
  slot ownership retained through real gate sweep/final receipt; duplicate
  submission rejected; input, compiler-tool, source-manifest and plan
  drift rejected; direct missing-lock and unlocked-inode calls rejected.
- `controls-v2-extra/results.json`, SHA256
  `3daca13baa61bb1edd1c2d185decf863f8f612c7981a4ca4e4e45e6c532084aa`:
  wrong task/slot inode, absent build FD8 and unlocked build FD8 each
  reject at their named lock oracle; a host fixture deliberately changing
  a product during actual gate cleanup rejects at the final guard despite
  gate return0 and the successful inner receipt. All negatives stop before
  native commands. The outer-death fixture's cleanup target is a symlink
  to the existing host interpreter in a copied control directory, never
  an upstream build product.

Exact copied fixtures, drivers, plans, logs and receipts are hash-indexed
in `v2-provenance.json`; that source-scoped evidence is not baseline
configure/build qualification. Independent review and the coordinator's
exact-command configure grant subsequently closed that boundary below. Make remains held
for a later review of actual generated configuration and its hashes.

## Remaining binding and comparison boundary

This creates immutable upstream source ownership, not an upstream runtime
receipt. Generated configuration now has its separate receipt below;
missing evidence includes compiled native products, external-header/library
dependency binding, build success,
correctness qualification, real startup/import origins, transitive/early
module witnesses and wrong-origin controls, plus the matching final fork
cohort. The parent baseline plan retains those obligations and historical
receipt limits. It also distinguishes this matched O3/no-PGO/frame-pointer
attribution cohort from any separately declared stock-production policy.
Py_STATS stays mechanism evidence; H1 elapsed benefit uses matched nonstats
release configurations. No current parity or speedup follows from setup.

## Approved configure-only execution

The coordinator granted the exact configure launch after independent
CONFIGURE_ONLY_PLAN_CLEAR review
`8fbce96b229b9d506168019d30c685aaf293e33e943bc87a864572a49978682a`.
The once-only launch used unchanged V2 runnerd853, plan419a and concrete
launch manifesta0ca9a7c. Outer, gate and inner return codes are0;
inner/post-cleanup guards pass. Configure took27.660seconds and the
launch-through-cleanup interval30.443seconds; these are setup turnaround,
not interpreter performance results.

| Frozen configure evidence | SHA256 |
|---|---|
| Joint `configure-closure.json` | `8908a1cc959d3828d5d50cda311e827400eef2b3bf79d92803d464cd12e49b02` |
| Accepted-for-review final `configure-launch-result.json` | `2dd59476670c92fec03cf1332be5b7ae7143304f6361ce91206aa21579be2124` |
| Inner `configure/result.json` | `1107b06e463851b6b5ea67af590331e4d03b2ed88fa1a71b285ab7c958a28b27` |
| `configure/products-after.json` | `379f76037cb827ba5ba5abd1ddb7cb4592566beca8900cfd2f490c4f34da703a` |

The joint closure binds final/inner receipts, all raw launch/phase logs,
plans/source/tool inputs and14 configured product paths. An independent
post-cleanup read agrees with both product snapshots; all5806 tracked
source hashes and14 tool identities remain unchanged. Git status is clean.
There is no target `build/default/python` or compiled extension product.

Actual Makefile source/build paths are the owned upstream checkout and
its own build/default. CC is `/usr/bin/gcc`; OPT is release O3/NDEBUG;
BASECFLAGS retains frame and leaf-frame pointers. CONFIGURE_CFLAGS_NODIST
includes the deliberately matched trivial-auto-var-init=zero and C11.
Config.log confirms Py_STATS, PGO, LTO and BOLT off, frame pointers on;
the header confirms debug/shared-library/GIL-disabled off and tailcall0,
and JIT flags/objects are absent. Host regeneration-Python detection
reports3.14.4, not execution of a new upstream interpreter.

Actual-config reviewde24b7f8 also records the source-specific eval flag:
upstream CFLAGS_CEVAL is empty; fork release deliberately adds -fno-gcse.
Retain that accepted upstream config unchanged. Common compiler/O3/FP/
zero-init match does not establish all-per-unit-flag matching or isolate
object-model cost. Before parity, require an upstream sensitivity or a
predeclared fully matched eval cohort; no extra build is authorized now.

Configured required json/pickle/math/array/expat/elementtree backends are
available to build; that is not proof of compiled/imported availability.
The optional missing/disabled modules remain decimal, dbm, gdbm, readline,
sqlite3, tkinter, uuid, bz2, zstd, ssl and hashlib; preserve these facts
for actual-config review rather than claiming extension qualification.

**Stopped after configure.** Before a separate make grant, its executor
must bind the accepted **final launch success and joint closure**, not
just V2's current inner-only build check, and freeze accepted configuration
through make/cleanup. This remains a bounded make-phase design/review item;
no build or post-build witness is authorized by configure success.
The separate [make-only plan](pre-I-baseline-make-plan.md) now proposes
that final-receipt/closure/configuration guard without altering original
V2 or any configure input. Its copied-host controls are not a make grant.
