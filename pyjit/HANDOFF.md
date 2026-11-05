# Coordinator handoff

Operational snapshot: 2026-11-03 20:40 PST. Read [STATUS](STATUS.md) for the
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
- Model policy (from 2026-11-04): every agent launch sets its model and
  effort. Opus runs at high effort, only for critical design work and subtle
  failure debugging, and is never fanned out. Sonnet at medium effort, or
  Haiku, does porting, finishing, reviews and doc syncs. In the W1 pilot,
  Haiku's mechanical batch matched Sonnet's site for site.
- Step agents run no full suite. A landing runs the step's test list plus a
  smoke set on both builds (`tools/integrate.sh <tag> <tests>`); the full
  suites run only at integration points (`tools/full.sh`, see
  [PROCESS](PROCESS.md) step 6). Each step agent release-compiles its own
  tree, because release builds compile `#ifndef Py_DEBUG` code that debug
  builds skip; two landings broke on this.

## Locations

| Role | Location / identity |
| --- | --- |
| Documentation root (canonical `pyjit/`) | `/home/power/code/cpython/.worktrees/python-jit`, branch `weaver/python-jit` |
| Runtime tree (nested worktree) | `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/pyvalue-rc-universal` |
| Runtime branch / HEAD | `boil/pyvalue-rc-universal`, `b9d3e3a3c9b` (see STATUS.md, Position) |
| Shared builds at HEAD | `build/pyvalue-rc-repr-debug`, `build/pyvalue-rc-repr-release` (under the `python-boil-the-ocean` worktree) |
| Older debug build | `build/pyvalue-rc-debug-20261102-v1` |
| Triage debug builds | `build/pyvalue-rc-bringup-debug`, `build/pyvalue-rc-triage-rt-debug` |
| Release build | `build/pyvalue-rc-release-20261103` (`--with-mimalloc --with-computed-gotos --with-shapes`, -O3) |
| Upstream control | `build/upstream-release-3a440b6` (`3a440b65695`, `CC=gcc --with-mimalloc --with-computed-gotos`, -O3); passes `test_int` |
| Coordinator working files | `build/pyjit-link-wave1/` (briefs, `FOLLOWUPS.md`, reviews, progress logs; bring-up ladder in `BRINGUP-BRIEF.md`) |

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
tkinter, ssl, sqlite and zstd headers are not installed on this host. Runtime
HEAD is `d686623e3fb` (I5). Full suites last ran at the integration point
`8eee8081ca0`, gcc debug and release: 52,178 tests each; the two failures were
stale tests, fixed in `a1ecc0eb4f3`. I5 ran its step tests and a smoke set on
both builds; full suites run at integration points only. Magic number: 64410
(B5b), set by the coordinator with
`tools/bump_magic.py`. The
fork's JIT source is deleted (A3 step 1; history keeps it) until the JIT
stage. Tail calls are the only interpreter backend; `--with-computed-gotos`
no longer exists, and `PY_TAIL_CALL_CC` in sysconfig names the convention.

Checkpoint measurement
([checkpoint](bench/results/checkpoint-20261103.md),
[triage](bench/results/checkpoint-20261103-triage.md)): kernels take 2.6x
upstream `3a440b6` (geomean, interleaved) with natural method-JIT admission off;
4.6x at `dbce7b1260d` with it on; the interim build takes 1.21x. Remaining gap:
interpreter-loop self cost 39%, exception save/restore around releases 25%,
value bridging 17%, other 14%. The IR executes 0.81 words per upstream
instruction. Peak RSS is 34% above upstream (startup 12.4 vs 9.5 MiB).

Measurement lessons: `sys._jit` covers only upstream's JIT, so the harness must
assert the fork's method-JIT state too. The pystats build compiles that JIT out,
so pystats dispatch counts do not see it. A `pybuilddir.txt` of `none` makes a
release build 5 to 12x slower; check it. `perf` is blocked
(`perf_event_paranoid=4`); `tools/sampler/` has an LD_PRELOAD SIGPROF sampler.

In flight, briefs in `build/pyjit-link-wave1/`:

- W2, the core wave, is running. W2a is Clinic regeneration and slots; W2b is
  callers, fields and the deferred `ceval.c` sites. W4 starts from W2a.
- G2 has A4 and B3 prototypes off-tree. Open question 9 is answered (a
  fixed-dst rule removes 9 to 14% of class tests at no word cost); the
  decision stays between A4 and B3.

Landed since `b9d3e3a3c9b`: the table in [STATUS](STATUS.md) lists all 27.
The newest, first:

- `d686623e3fb`: I5, calls, monitoring and tracing on values.
- `a1ecc0eb4f3`: two stale tests fixed after the integration point.
- `8eee8081ca0`: B5b, unpack into locals and append from fields (magic 64410).
- `1a9b96b1000`: W4-0, module value homes.
- `5c94f78a582`, `b904ab8f014`, `1996cbbb080`: the interpreter returns a value;
  I3 and I4 handlers on values.
- `56906c53125`: B7 fix, the self slot of a pushed frame.
- `1fc13423e46`, `4eeb6ea2ff2`: B5 (magic 64409) and B6 (magic 64408).

Landed since `f7d374b1f92`, newest first:

- `b9d3e3a3c9b`: M3, value vectorcall, `METH_VALUE`, Clinic value emission.
- `c66a6339ccf`: W1b, object forms of containers compatibility-only.
- `763c8ececd5`: A5b-4, final ownership names.
- `c6fb9beb79f`: the two test_capi failures fixed.
- `0bf1b0486a5`: A3 step 2, handlers without addressable locals.

Landed earlier on the runtime branch, newest first:

- `a6002e5854b`: A5b-1, `PyValueBorrow` and the reader macros
  (`review-irv2-a5b-1.md`).
- `2f572ea0ea8`: value-form container calls in 11 box sites (contains,
  compare, set and dict adds, global and name stores, dict delete, list and
  tuple indexing); post-review `review-boxsites.md`.

- `5de7bd62ef9`, `80bb9c84701`, `bf0639df819`: release-order fixes and stale
  fork tests (9 of 11 long-standing failures); int51 in the trace JIT (9b)
  and in the runtime and interpreter (9a).

- `3e28e1755bd`, `d60d3be46d6`, `3c7ac067f7f`: register IR v2 A5a
  (post-review generator rules; no frame slot holds a borrow; the checker).

- `c8637fe7c6b`, `335b86b71e9`, `44eb4e716dd`: register IR v2 A2 fixes
  (failed instrumentation walk; generator refuses a value use of a boxed
  input), A2 (`next_instr` by value), A1 (the value record only where a
  handler boxes).

- `f6508007b94`, `308a86c3914`, `1381d5b49c0`, `5640c46361c`: representation
  step 8 (object-level value identity; canonical construction and bit-compare
  `is`; store-counting home checker; step 7 review fixes).
- `54825c08a1a`: register IR v2 step B0a, the instruction contract table.
- `1a9d0a6fbbe`, `78908a19694`, `13ae4cfddda`, `1a5600cc9f5`, `ab07912b7cb`:
  representation step 7 (perf watcher fix, restore helper deleted, defences,
  residue, status APIs).
- `c8a0b4dc54c`: `PyDict_Pop` takes its status from the operation.
- `fbe866fae0a`, `f9bd0364fd9`, `cedd72de803`: representation steps 6c, 6b,
  6a (save/restore sites 766 to 117).
- `c4b01b23a54`: step 5 review fixes.
- `48bb4d6d1d3`: value API V1, `PyValue_ERROR` removed; `PyValue_AsObject` is
  the inline boxing converter.
- `946f622e1d1`: value API V1, `pyvalue.h` moved to `Include/internal`.
- `1a71a5cc2cf`: value API V1, `PyValue_AsPyObjectBorrow`/`Steal` renamed to
  `PyValue_AsKnownObjectBorrow`/`Steal`, checked in every build.
- `f3a20271f26`: value API V1 additive part (box counters by site,
  `Py_VALUE_CORE`/`Py_VALUE_COMPAT`, `make check-value-api`, value format
  conversions, entry primitives).
- `0bf3ad80ecf`: representation step 5, release contract and retire path.
- `1f19952778d`: natural method-JIT admission off by default.
- `7171541fa49`: representation step 4, `PyValue_NULL` is all zero bits.
- `fdd7fed3216`: P4 post-review fixes (borrow release bit re-armed only when it
  was already set).
- `379e92cded9`, `dbce7b1260d`: step 3 (debug check of canonical values in homes;
  3b checks homes in the IR store macros).
- `2558376b321`: native boundary P4, call arguments through the borrow stack.
- `f7dfc5b961f`: step 2, 48-bit address limit checked where memory comes from
  the OS.
- `f129780a9a7`: step 1, one PyValue build profile and one set of constants.

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
The designs below and the value API page are accepted.

- [pyvalue.md](design/pyvalue.md), [review 1](design/reviews/pyvalue-review-1.md):
  decisions D1 G1' fixes (done); D2 canonical construction and release contract;
  D3 int51 (measured, `bench/results/pyvalue-int-width-20261103.md`); D4 static
  borrowing (joint with register-ir IR-D3); D5 NULL = 0 (done, step 4); D6 ABI policy (done);
  D7 type-slot gap after baseline (rows updated); D8 G7 non-goal, G8 deleted.
- [native-boundary.md](design/native-boundary.md),
  [review 1](design/reviews/native-boundary-review-1.md): accepted with changes.
  A per-thread borrow stack replaces `_PyArg_OwnerScope`. P1 to P4 landed
  (P4: argument vectors on the borrow stack, with post-review fixes); the page
  describes P4 as built.
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
  agent-hours. Steps 1 to 7 landed; 8 is running.
- [value-api.md](design/value-api.md): values are the internal API and
  `PyObject *` is the compatibility layer. Reviews 1 and 2 and revisions 2 and 3
  (`74236263ef1`) done; accepted for packet V1, which landed (`f3a20271f26` to
  `48bb4d6d1d3`).

**Clinic means CPython's Argument Clinic wrapper generator.** Producer changes
must be made in `Tools/clinic/libclinic/`; bulk edits to generated C are
overwritten.

## Next steps

1. W2a, then W2b. W4 starts from W2a.
2. W5 and the R27 gate.
3. G2's timed decision between A4 and B3, on a quiet host.
4. Follow-ups in `build/pyjit-link-wave1/FOLLOWUPS.md`.
5. Then the checkpoint measurement.
6. After the checkpoint: return `co_code` and `.pyc` to stack
   bytecode with the IR as a hidden cache
   ([proposal](design/proposals/stack-bytecode-recovery.md), user decision
   2026-11-05).

Use the time metric; the instruction-count metric needs `perf_event_paranoid`
lowered, which the user has not done. Do not present host controls, object files
or compiler-error reductions as runtime performance.

## Coordination

Update the relevant workstream and STATUS at each landing, keep one WORKLOG line
per decision or concluded experiment, and mirror the project pages to local
Loom. Give the user a status report about every two hours while work is active
and at material milestones. A monitor due time is not a notification service.
The rename to `pyjit/` preserves historical `boil` branch, ABI and build
identities and `Tools/boil`. Do not touch `pyjit/archive/`.
