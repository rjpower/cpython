# Coordinator handoff

Operational snapshot: 2026-11-03 16:20 PST. Read [STATUS](STATUS.md) for the
project summary, [ROADMAP](ROADMAP.md) for stage outcomes, [GOAL](GOAL.md) for
standing direction, and [PROCESS](PROCESS.md) before assigning work. This is a
restart guide, not a claim that workers remain active. Recheck agents and
processes before assigning work.

## Mission and selected design

Complete a safe universal-value register interpreter, qualify it, then measure
it with JIT disabled against matched interim RC and upstream controls. Build
the cohesive method/tracing JIT afterward using shared optimizing IR, recovery
and backend. The stage pages distinguish selected designs from working code.

- Canonical language values are eight-byte NaN-tagged PyValues in frames,
  containers and attributes. Correct owning object views at C boundaries are
  acceptable for the first baseline; eliminating their boxing is later tuning.
- Reference counting plus cyclic collection is selected. The tracing collector
  and nursery timebox is closed. Do not restart it from historical plans.
- Rebuild and port extensions. Hard backward source/binary compatibility is not
  an admission gate and needs no further approval. Preserve Python semantics,
  ownership and useful errors; evolve toward an opaque extension API.
- Ports use idiomatic upstream CPython ownership code.
- Model roles, review and commit rules are in [PROCESS](PROCESS.md). The
  coordinator owns integration and shared builds.

## Locations

| Role | Location / identity |
| --- | --- |
| Documentation root (canonical `pyjit/`) | `/home/power/code/cpython/.worktrees/python-jit`, branch `weaver/python-jit` |
| Runtime tree (nested worktree) | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/pyvalue-rc-universal` |
| Runtime branch / HEAD | `boil/pyvalue-rc-universal`, `0ab5bc42f61` (checkpoint56 was `6bf198d1ef5`) |
| Debug build | `build/pyvalue-rc-debug-20261102-v1` (under the `python-boil-the-ocean` worktree) |
| Release build | `build/pyvalue-rc-release-20261103` (`--with-mimalloc --with-computed-gotos --with-shapes`, -O3) |
| Upstream control | `build/upstream-release-3a440b6` (`3a440b65695`, `CC=gcc --with-mimalloc --with-computed-gotos`, -O3); passes `test_int` |
| Wave 1 working directory | `build/pyjit-link-wave1/` (reviews in `review-*.md`, bring-up ladder in `BRINGUP-BRIEF.md`) |

The outer `python-boil-the-ocean` worktree's git link is broken. Run Git in the
nested runtime worktree. Older worktrees hold historical copies of these pages;
do not edit them.

`build` is shared storage. Reuse the existing checkout and builds. Use `uv` for
Python commands, with the shared cache at `/home/power/.cache/uv`.
`/usr/bin/python3.14` has `test.support` for reference controls; it is not the
restored candidate.

The qualified interim source is `eba351fee7d`, with executables under
`build/rc-integrated-{release,debug}-20261101/python`. It has inline frame values
but boxed heap fields. Its ABI is `boilrc1`; restoration uses
`boilvalrc1-316d-x86_64-linux-gnu`. Neither is a new native JIT tier.

Restoration configuration is Linux x86-64, GIL, GCC, mimalloc, computed gotos
and shapes. Read the build directories for the exact commands.

## Source and build state

The runtime tree has no held modifications. The type graph/MRO/cache projection
(`Objects/typeobject.c`, `Lib/test/test_pyvalue_typecache.py`) is committed as
checkpoint57, `5e977ec8cbb`.

Commits since checkpoint56 (each reviewed independently before commit):

- `5e977ec8cbb`: held type projection admitted.
- `337dd518abb`, `5c250397c43`, `494e53f657a`, `bd1520443fd`, `abc8631ce60`:
  wave 1, about 50 core files ported off deleted borrowed getters. New internal
  helpers: `_PyTuple_ReplaceTakeRef`, `_PyDict_NextBorrowed`,
  `_PySet_NextBorrowed`.
- `271f4e435e5`: `PyValue_NewRef` keeps the immortal no-count tag.
- `6ea1e52db4a`: marshal set writer stops on iteration error.
- `62fee2c1656`, `c7194ad146e`, `570b81e0e7a`: wave 2 extension modules
  (stdlib accelerators, pickle, ctypes).
- `0b7f39711d0`: system modules (remote debugging, select, socket, curses,
  termios, posixsubprocess, resource, multibytecodec).
- `0ab5bc42f61`: test and C API modules; test wrappers for deleted borrowed
  APIs removed, with the owning variants keeping their coverage.

Build: debug `make -k python` (merged v23) fails only in `Python/getargs.c`
(11 errors plus one `pymacro.h` expansion). A scratch link test showed the only
undefined symbols are getargs.c's deleted-getter calls. Shared modules fail only
in Argument Clinic generated headers and `Include/internal/pycore_modsupport.h:168`.
The release build shows the same core errors. All of these belong to the
native-boundary redesign. There is no executable, runtime test or timing.
Compiler-error counts include cascades; they are not a completion ratio.

## Designs

All under `design/`. Each is short markdown. Design reviews live in
`design/reviews/`; code reviews of ports live in `build/pyjit-link-wave1/`.

- [pyvalue.md](design/pyvalue.md): as-built spec, comparison with LuaJIT, JSC,
  V8, SpiderMonkey and PyPy, 12 prioritised gaps. G1 is fixed. Pre-baseline
  packet: G2 canonical-form checker, G3 owned test excludes ERROR, G4 boxing
  counters and removal of exception save/restore around `Py_DECREF` (912 sites in
  about 40 files from earlier ports), G6.1 heap-store asserts, G10 48-bit check,
  G11 dead-encoding cleanup. Under independent review.
- [native-boundary.md](design/native-boundary.md): an implicit per-thread borrow
  stack, scoped at native calls and eval entries, replaces explicit
  `_PyArg_OwnerScope`. The four frozen FORMAT templates are withdrawn.
  Handwritten `PyArg_*` callers are unchanged. `PySequence_Fast_ITEMS` and
  `PySequence_Fast_GET_ITEM` are deleted. Packets: P1 stack and getargs; P2 entry
  points; P3 delete OwnerScope from Clinic and nine files; P4 move FASTCALL
  boxing onto the stack, after link. Under independent review; P1 and P2
  implementation is running in parallel and nothing commits before findings are
  folded in.
- [register-ir.md](design/register-ir.md): as-built spec plus V8 Ignition and
  LuaJIT comparison, being written. Known deviation on the interim build:
  copy propagation changes `f_locals` (`x + g()` returns 100, upstream 1).

**Clinic means CPython's Argument Clinic wrapper generator.** Producer changes
must be made in `Tools/clinic/libclinic/`; bulk edits to generated C are
overwritten.

## Next steps

1. Native-boundary P1 and P2, with review findings folded in.
2. First link, then first runtime execution (`_freeze_module`).
3. Bring-up ladder (`build/pyjit-link-wave1/BRINGUP-BRIEF.md`).
4. PyValue hardening packet.
5. Clinic P3 and shared modules.
6. Test-suite triage.
7. Release build.
8. Measurement against upstream and the interim runtime.

The instruction-count metric needs `kernel.perf_event_paranoid` lowered; the
user was asked. Until then use the time metric. Do not present host controls,
object files or compiler-error reductions as runtime performance.

## Coordination

Update the relevant workstream and STATUS at each landing, keep one WORKLOG line
per decision or concluded experiment, and mirror the project pages to local
Loom. Give the user a status report about every two hours while work is active
and at material milestones. A monitor due time is not a notification service.
The rename to `pyjit/` preserves historical `boil` branch, ABI and build
identities and `Tools/boil`. Do not touch `pyjit/archive/`.
