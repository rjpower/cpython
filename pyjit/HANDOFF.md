# Coordinator handoff

Operational snapshot: 2026-11-03 17:40 PST. Read [STATUS](STATUS.md) for the
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
| Runtime branch / HEAD | `boil/pyvalue-rc-universal`, `6d39221996d` |
| Debug build | `build/pyvalue-rc-debug-20261102-v1` (under the `python-boil-the-ocean` worktree) |
| Triage debug builds | `build/pyvalue-rc-bringup-debug`, `build/pyvalue-rc-triage-rt-debug` |
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

The runtime tree has no held modifications. Debug and release builds link and
run. All 67 shared modules whose third-party headers exist build; the decimal,
tkinter, ssl, sqlite and zstd headers are not installed on this host. Full
debug suite at 17:31 (before the last fixes): 611 of 619 files run, 484 OK, 30
failed, 3 env changed; 48,500 tests, 52 failures. Two triage agents are fixing
the rest. Known causes: zero-filled slots (NULL = 0 change), per-call malloc in
`native_scope.c` (P4), IR and tooling expectations. There are no performance
results for the universal-value runtime yet.

Commits since checkpoint56 (`6bf198d1ef5`), each reviewed independently before
commit:

- `5e977ec8cbb`: held type projection admitted (checkpoint57).
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
- `0ab5bc42f61`: test and C API modules; wave 2 done.
- `c66a0b29253`: copy propagation no longer folds a load across code that may
  run Python (register-ir G1 fixed; pyc magic 64402).
- `6a7b3769d62`: PyValue tag fixes (ERROR/wrap unowned, one immortality test,
  `__slots__` traversal), PyValue outside the limited API, stable ABI manifest.
- `acbdf45627d`: native boundary P1 and P2 (per-thread borrow stack, getargs,
  scopes).
- `192694b1523`: first runtime fixes (AST arena owns strong refs; CALL
  specialization with inline self; managed-dict slot).
- `ca8a9d6d827`: eval-loop C frame 35.6 KB to 5.5 KB (upstream about 360 B).
- `e704ab282d9`: remaining modules ported (decimal, ssl, sqlite, zstd, ...).
- `7f6501d9669`: bring-up fixes; `_PyType_Lookup` and `PySys_GetObject`
  deleted; type cache admission; register IR limits raise `SyntaxError`.
- `56ff66a596b`, `643c69c7244`, `6d39221996d`: native boundary P3 (Clinic on the
  borrow stack, `_PyArg_OwnerScope` deleted), review-1 follow-ups and borrow
  scope tests. `56ff66a596b` alone does not build; the two regeneration commits
  complete it.

## Designs

All under `design/`. Each is short markdown. Design reviews live in
`design/reviews/`; code reviews of ports live in `build/pyjit-link-wave1/`.
All three designs are accepted.

- [pyvalue.md](design/pyvalue.md), [review 1](design/reviews/pyvalue-review-1.md):
  decisions D1 G1' fixes (done); D2 canonical construction and release contract;
  D3 int51 (measured, `bench/results/pyvalue-int-width-20261103.md`); D4 static
  borrowing (joint with register-ir IR-D3); D5 NULL = 0; D6 ABI policy (done);
  D7 type-slot gap after baseline; D8 G7 non-goal, G8 deleted.
- [native-boundary.md](design/native-boundary.md),
  [review 1](design/reviews/native-boundary-review-1.md): accepted with changes.
  A per-thread borrow stack replaces `_PyArg_OwnerScope`. P1 to P3 landed; P4
  (argument vectors on the borrow stack) is in progress; post-review is running.
- [register-ir.md](design/register-ir.md),
  [review 1](design/reviews/register-ir-review-1.md),
  [review 2](design/reviews/register-ir-review-2.md): accepted after review 2.
  IR-D1 one encoding revision (IR v2) with a bit budget; IR-D2 generated
  instruction contract; IR-D3 ownership = static slot class plus value tag;
  IR-D4 JIT boundary state open. One limits rule; names past 4,095 lower to an
  intrinsic; 8-byte feedback offsets.
- Proposal: [pyvalue-representation-packet.md](design/proposals/pyvalue-representation-packet.md)
  and its [review](design/reviews/pyvalue-representation-packet-review-1.md).
  Accepted plan for D2, D3, D5, G1', G3, G10 and G11: 11 steps, about 42
  agent-hours, starting after triage and P4.

**Clinic means CPython's Argument Clinic wrapper generator.** Producer changes
must be made in `Tools/clinic/libclinic/`; bulk edits to generated C are
overwritten.

## Next steps

1. Test-suite triage (two agents running) and native-boundary P4; post-review
   of P1 to P3 is running.
2. Representation packet (11 steps).
3. IR v2.
4. Baseline.
5. Checkpoint measurement: time and RSS against upstream `3a440b6` and the
   interim runtime, on a quiet machine once the suite is near green.

The instruction-count metric needs `kernel.perf_event_paranoid` lowered; the
user was asked and it is not done. Until then use the time metric. Do not
present host controls, object files or compiler-error reductions as runtime
performance.

## Coordination

Update the relevant workstream and STATUS at each landing, keep one WORKLOG line
per decision or concluded experiment, and mirror the project pages to local
Loom. Give the user a status report about every two hours while work is active
and at material milestones. A monitor due time is not a notification service.
The rename to `pyjit/` preserves historical `boil` branch, ABI and build
identities and `Tools/boil`. Do not touch `pyjit/archive/`.
