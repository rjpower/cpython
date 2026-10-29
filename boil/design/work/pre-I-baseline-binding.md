# Checkpoint I: bind the upstream baseline environment

Status: source/configuration audit and proposed repair, 2026-10-29.
**No interpreter, build, test, probe or benchmark was run. No worktree,
branch, configuration, native source or existing build was changed.**
This document requests a later bounded source/build/runner assignment;
it is not execution approval or a current parity result.

## What is established, and what is missing

The retained baseline's C ancestry is plausible, but its standard-library
binding is wrong for checkpoint I. Its Makefile, config.status and generated
`_sysconfigdata` Python file specify `srcdir=../..`, resolving from
`build/baseline-default` into the active fork worktree. Moving that
executable or supplying PYTHONPATH would not repair its complete build
and extension provenance. WORKLOG's October25 entries already record
that this old interpreter cannot run the fork's current Lib/create a venv.
Do not rediscover that failure by running it during this audit.

| Retained input | Source/configuration evidence and limit |
|---|---|
| `boil/bench/results/baseline.json` | Records version/git `15e96bf639d`, GCC15.2.0, executable path, hash seed0 and benchmark runner commit `41157098106`. It contains78 instruction,37 RSS and35 time rows; time was recorded2026-10-27. It records no complete source/native/import closure. |
| Git `15e96bf639d6609b358d69daa4992644d42f9d48` | Diff against upstream `321925c92c9` is only `boil/WORKLOG.md`. That proves source-tree ancestry, not which Lib/extension files the old measurement imported. |
| `build/baseline-default/{Makefile,config.status,config.log,pyconfig.h}` | GCC release `-DNDEBUG -g -O3 -Wall`, configure `--with-lto=no CC=gcc`; config.log reports PGO, LTO, BOLT and Py_STATS off, frame pointers on. Header has debug/GIL-disabled/tailcall off; JIT objects/defines are absent. These are retained configuration facts, not a fresh build qualification. |
| Generated `build/lib.linux-x86_64-3.16/{_sysconfigdata__linux_x86_64-linux-gnu.py,_sysconfig_vars__linux_x86_64-linux-gnu.json,build-details.json}` and `pybuilddir.txt` | Bind build metadata and extension-directory selection. build-details' `/usr/local` install paths do not demonstrate runtime sys.path or installed-library provenance. Sysconfig's source path points at the fork. |
| Existing main checkout `/home/power/code/cpython` | Clean at `3a440b65695615e9fd9912342b79cdd54fced0d2`; differs from upstream321925 only in `Doc/c-api/pyexpat.rst` and `Doc/library/re.rst`. Native sources and Lib match upstream at this inspected snapshot. It has no build directory. Its future ownership/freeze is not established here. |
| Other managed worktrees | ROOTS, queue, compiler/H3, primary and existing review worktrees are fork states, not immutable upstream environments. No existing exact-upstream bound build was identified. Historical `baseline-jit` and `baseline-ft` are separate configurations, not substitutes for this JIT-off/shared-GIL comparison. |

Read-only current hashes, useful for preserving artifacts rather than
retroactively certifying them:

- baseline executable: `da19e76303708159109efa5c649a8e21c07525b06437275c23bd52fabc0696f2`;
- baseline Makefile: `244f2f21cca6244b0e0d9fb8221d53124d0212b7f315f6883cc250bb2190710e`;
- baseline pyconfig.h: `770fb14f5fb1bc81c4a61fa525846bf009ce365a4797b96649b97caecb31d63e`;
- historical baseline.json: `0134b37263883b9ef90ec4c5157d3936cf1e294b020d5a98a0f0d11274feb873`.

No located receipt binds the current executable hash and all actual
extension target bytes to the historical run. This audit did not reconstruct
a historical source snapshot or run imports. Preserve results as historical
measurements with their recorded limitations; do not overwrite or silently
upgrade them into current upstream evidence.

## Smallest proposed repair

Use a repository-managed **detached upstream checkout at exact321925**
with exclusive task ownership. Proposed reserved source path:
`/home/power/code/cpython/.worktrees/boil-checkpoint-i-upstream`;
proposed build: its own `build/default`; result label:
`checkpoint-i-upstream-bound`. No such reservation/creation is performed
here. The main checkout's two-document delta is comparison evidence,
not permission to borrow or freeze the user's main checkout.

An existing managed detached upstream tree may replace that proposed
path only after its ownership, exact commit/content, Lib, extensions and
configuration identities are proven. The inspected worktree list exposes
no qualified one. Do not reuse another task's review tree merely because
it is detached. Freeze all source bytes before/after every build/run;
commit identity alone does not exclude a dirty or replaced Lib.

Build a newly labeled upstream environment there, using its own source
Lib and own extension products. This is one necessary new build against
the correct source; leave historical primary `build/baseline-*`, main and
all existing results/logs intact. Do not copy old extensions, relink the
old executable or redirect it with PYTHONPATH. Preserve configured source/
build paths and complete compile/link logs from the start. Shared caches
remain configured as usual; no build tree or environment belongs under
`/tmp`. Checkout/build execution needs a later bounded assignment.

The fork side similarly uses the eventual checkpoint-I **frozen integrated
head**, after its correctness and pre-JIT dependencies. Reserve an existing
managed release build only at quiescence; reuse a qualifying build if its
actual flags and complete source/product closure match the comparison
policy. Current primary/dev build directory names are not that evidence.
Never rebuild an active gate's directory to obtain a timing cohort.

## Match the comparison policy, not just the label

For the first **matched attribution cohort**, propose both sides use the same
GCC executable/version and release O3 policy, shared GIL, non-tailcall,
non-debug, Py_STATS off, all JITs off, retained frame pointers, PGO/LTO/BOLT
off, and the same target-ISA/link/shared-library policy. Upstream can
explicitly configure experimental-JIT=no; fork configure no longer has
that option, so establish its absence from actual defines/objects instead
of passing an unsupported flag. Inspect expanded compile/link commands,
including eval-unit flags: equal CONFIG_ARGS alone is insufficient.

This common baseline does not forbid a later separately labeled matched
Clang/tailcall or PGO/LTO cohort. If checkpoint I chooses H1's tailcall
configuration as its primary release policy, rebuild **both** appropriate
sides with the same supported compiler/tailcall/frame-pointer policy and
state the decision before measuring. Do not attribute a compiler, JIT,
frame-pointer or profiling-policy change to the interpreter design.
PGO-on comparisons require equivalent declared training workloads and
retained profile identities, not recycling old profile data.

Keep that frame-pointer-matched attribution result distinct from any
**stock-policy production comparison**. A separately declared deployment
cohort may compare each runtime's intended supported production policy,
including its PGO/LTO/tailcall/frame-pointer choices, with all differences
reported. It cannot attribute the resulting ratio solely to object model
or dispatch. The coordinator chooses which declared cohort answers
checkpoint I; a controlled O3/no-PGO result is not automatically a claim
against upstream's best production build. H1's own benefit uses matched
non-stats release parent/changed calling conventions with the same
supported compiler/configuration; Py_STATS supplies mechanism evidence,
not its elapsed-time result.

Keep each interpreter's intended native object/collector model; record
allocator configuration and effective runtime options rather than trying
to erase that architectural difference. Match available benchmark-required
extensions and external-library versions. The historical baseline lacks
several optional modules; determine actual new availability and fail on
a missing required backend instead of quietly changing benchmark semantics.
Box/root/pause attribution builds remain separate from uninstrumented
release timing, with source/configuration links and explicit coverage.

## Prove each runtime's actual environment before timing

The later approved binding check must run under the **same command,
environment and working directory policy** as measurements, including
startup and the actual benchmark driver. A standalone `sys.version`
print does not close this boundary.

1. Freeze source content, configuration/Makefile/sysconfig/header inputs,
   compiler/linker identities and compile commands; then hash executable,
   library and every extension alias **and resolved target**, generated
   frozen/import products and external native dependencies. Check these
   before and after build qualification and measurement. Record paths,
   sizes and symlink resolution; metadata timestamps alone are insufficient.
2. Capture actual `sys.executable`, `_base_executable`, prefixes/base
   prefixes, `sys.path`, sysconfig's source/build paths and config-module
   origin. Assert upstream's source/Lib/build-extension roots are its
   reserved upstream checkout/build, and fork's are its frozen fork roots.
   No active opposite tree, old primary baseline extension path, venv,
   user site, installed `/usr/local` stdlib or accidental cwd import is
   allowed. Prefix `/usr/local` in an uninstalled build is not by itself
   an import failure; check actual paths/origins rather than prefix alone.
3. Record loaded modules' `__spec__.origin`, `__file__`, package search
   locations, loader and resolved hashes after actual workload setup and
   execution, **outside timed intervals**. Include transitive imports,
   not only sentinels such as encodings, sysconfig, json/_json,
   pickle/_pickle, re/_sre, math, array, copy/dataclasses,
   logging and xml.etree/_elementtree. Builtin/frozen modules need their
   own source/generated/executable binding, not an invented file path.
   Capture early imports too; a final sys.modules snapshot alone can miss
   a transient import or a replaced module. Design the corresponding
   untimed import-origin witness before running it.
4. For extensions, bind actual loaded bytes and external dependencies,
   not only extension suffix or sys.version. Freeze required backend
   availability and the benchmark workload/data source hashes. The common
   benchmark driver/kernel files are the sole deliberate shared Python
   sources; load them explicitly by path without making their directory
   a source of unrelated stdlib replacements.
5. Preserve controlled environment values, not an unrestricted environment
   dump. The current runner drops PYTHON* variables and sets hash seed0,
   uses `-S` and a fresh pycache prefix. These do not prove stdlib identity
   and do not clear inherited LD_PRELOAD/LD_LIBRARY_PATH. The approved
   runner policy must prevent loader injection/opposite-tree imports,
   record allowed loader/library paths, working directory, locale/timezone,
   affinity and measurement settings, and isolate pycache per interpreter
   campaign. Keep hash seed0 effective; adding `-E`/`-I` blindly would
   ignore that environment setting and change current behavior.

These are future checks, not executed positives. Retain a failing binding
control: a copied environment with an opposite-tree stdlib or extension
origin must be rejected before timing. A wrong config-module origin must
also reject even when the executable's version/git string is correct.
The check must fail on the wrong origin, not merely on an unrelated import
exception. Restore the clean environment before qualification.

## Runner and evidence work still needed

`boil/bench/run.py` currently records version/git/hash seed and its own
benchmark commit. It does not record interpreter-source or native closure,
sys.path/import origins, effective config/flags or loaded extensions.
Its `--update` checks only executable path/label and hostname before
merging metrics, so changed bytes at one path could combine incomparable
runs. Before checkpoint I, the coordinator-owned runner needs a bounded
binding receipt/interface and must require matching source/product/policy/
workload identities for any metric update. Do not rewrite historical
boil-bench/1 results to invent missing fields; new bound result identities
must be separate.

The minimum later assignment is: reserve source/build owners; freeze the
comparison policy; build and qualify the upstream environment and matching
fork release environment; review the origin/binding witness and its wrong-
origin controls; then collect fresh paired elapsed results in quiet windows
with common workloads, warm-up and repetitions, preserving per-run values,
uncertainty and regressions. Record startup separately from warm execution.
Current load guards and instruction differencing can be reused, but do not
replace that evidence or explain away slower rows. Existing historical
instruction ratios and micro improvements establish no current parity.

The coordinator must approve the exact source reservation, build/runner
hunks, correctness qualification and timing window before execution. This
audit supplies a repair plan, not a new build or performance claim.

## Owned source setup follow-up

The original source-only plan is retained at primary `bd60e64095c`, SHA256
`6bd23acaf3c1959461630d50c880201fd013ea5a99990a263f8257fa1f14c601`.
Independent concept review clears that plan only, with no build approval.
For precise generated-metadata wording: the `_sysconfigdata` Python
srcdir is `../..`; `_sysconfig_vars` JSON instead records an installed-
prefix srcdir while its **abs_srcdir still points into the active fork**.
The current source-binding gap remains; neither value proves historical
imports.

The coordinator first authorized owned-source setup. The
[source-setup record](pre-I-baseline-source-setup.md) records the clean
detached exact321925 checkout,5806 source hashes, ownership and concrete
separate configure/make-only plans and independent reviews. A later exact
configure-only grant completed successfully with unchanged source/tools
and a jointly frozen14-product configuration closure. A separate reviewed
make-only grant then completed successfully and froze584 products,
including its disclosed internal interpreter actions. A separately granted
diagnostic, three valid wrong-origin controls and18-module correctness
transaction now pass and clear independent actual review; complete workload
origins and benchmarks have not run. Main and
all historical baseline products/results remain unchanged. Configuration
success does not close runtime import binding or qualify a comparison.
Actual configuration preserves upstream's empty CFLAGS_CEVAL while fork
release deliberately uses -fno-gcse. Treat the common O3/compiler/FP/
zero-init policy as partly matched; require the separately planned
upstream sensitivity or a predeclared fully matched eval cohort before
parity. The make-only work doc records this boundary; no extra build or
performance measurement is implied.
