# Coordinator handoff

Runtime HEAD is `6a0e4c12c17` (constructor and format work), following
`05286694774` (native child accounting). All five build-info objects match
their respective source HEADs: upstream `3a440b65695`, interim `eba351fee7d`,
and candidate `6a0e4c12c17`. Module rosters are 116/116/117. The full release
suite and debug suite both passed 54,351 tests with no failures (release: 2,531
skips; debug: 2,469). The strict whole-source R27 census failed: 6,512 dumps
contain 11,028,818 core/core-pending/unresolved boxes and process
identity/accounting is incomplete. The raw kernel capture completed with 87
records across 29 kernels: 18/29 are startup-inclusive raw-zero and all loop
deltas are zero, but the strict kernel gate failed. See the [kernel receipt](issues/checkpoint-closure/kernels-6a0e4c12c17.md)
and [full census receipt](issues/checkpoint-closure/full-census-6a0e4c12c17.md).
The matched-build preflight passed but is ineligible as a measurement. A
diagnostic timing run is active, with no result published; it cannot qualify
performance while R27 fails.

Keep the failed full-v2 results visible: both profiles ran 54,347 tests with
two AST failures and one opcache failure, and `test_tools` changed `sys.path`.
The targeted AST/call fixes and latest environment check passed. The earlier
15/29 raw-zero capture at `66981c963be` remains historical, not a new
qualification. Root owns the frozen source/builds. Audit static-gate and
process-accounting gaps before changing marks or source. A diagnostic timing
run is active and has no result published; it cannot qualify performance
while R27 fails. Ctypes and LRU implementation remain on explicit approval
hold; G2 is deferred.

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
- Current model policy (2026-11-05): Terra/Luna at medium effort handle
  porting and documentation. Sol handles important design, review or specific
  debugging. The coordinator owns integration. See [PROCESS](PROCESS.md).
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
| Runtime branch / source | `boil/pyvalue-rc-universal`, clean HEAD `6a0e4c12c17` (2026-11-06; coordinator-owned) |
| Candidate builds | `build/pyvalue-rc-repr-debug`, `build/pyvalue-rc-repr-release`, `build/pyvalue-rc-repr-stats`; retained IDs and control builds are linked in the current census receipts |
| Matched controls | Upstream `3a440b65695` and interim `eba351fee7d`; exact build IDs are in the full-census and kernel receipts |
| Historical builds | `build/pyvalue-rc-debug-20261102-v1`, `build/pyvalue-rc-bringup-debug`, `build/pyvalue-rc-triage-rt-debug`, `build/pyvalue-rc-release-20261103`, `build/upstream-release-3a440b6` (not the current matched-build set) |
| Coordinator working files | `build/pyjit-link-wave1/` (briefs, `FOLLOWUPS.md`, reviews, progress logs; bring-up ladder in `BRINGUP-BRIEF.md`) |

The outer `python-boil-the-ocean` worktree's git link is broken. Run Git in the
nested runtime worktree. Older worktrees hold historical copies of these pages;
do not edit them.

`build` is shared storage. Reuse the existing checkout and builds. Use `uv` for
Python commands, with the shared cache at `/home/power/.cache/uv`.
`/usr/bin/python3.14` has `test.support` for reference controls; it is not the
restored candidate.

The current candidate SOABI is `boilvalrc3`. The qualified interim source is
`eba351fee7d`, with executables under
`build/rc-integrated-{release,debug}-20261101/python`. It has inline frame values
but boxed heap fields. Its ABI is `boilrc1`; restoration uses
`boilvalrc1-316d-x86_64-linux-gnu`. Neither is a new native JIT tier.

Restoration configuration is Linux x86-64, GIL, GCC, mimalloc, computed gotos
and shapes. Read the build directories for the exact commands.

## Source and build state

Runtime HEAD `6a0e4c12c17` is clean. Build-info objects match their respective
upstream, interim and candidate source HEADs; rosters are 116/116/117. Release
and debug full suites each passed 54,351 tests with no failures (2,531 and
2,469 skips). The strict full census failed with 11,028,818
core/core-pending/unresolved boxes and incomplete process identity/accounting;
the strict kernel gate also failed. Exact counts and artifacts are in the
[full census receipt](issues/checkpoint-closure/full-census-6a0e4c12c17.md) and
[kernel receipt](issues/checkpoint-closure/kernels-6a0e4c12c17.md). The earlier
full-v2 failures remain historical evidence, superseded by the final full-suite
pass. The paired `.txt`/`.done` producer, strict reader and benchmark consumer
are integrated; the strict reader's current census result is a failure, not a
pending check.

The raw kernel evidence at `66981c963be` is historical and keeps its own ELF.
The current `6a0e4c12c17` kernel and full-census receipts identify the exact
retained executables. A diagnostic timing run is active, with no result yet;
it cannot qualify performance while the R27 gate is open.

The final AST type-call guard admits only the selected type-call pair when the
class object's actual object and value vectorcall fields are both inactive.
Ordinary Python classes and `object` have null fields; active built-ins retain
the normal vectorcall path. See the [v3 source review](issues/checkpoint-closure/ast-original-kwargs-peer-review-v3.md)
and [merged differential](issues/checkpoint-closure/ast-original-kwargs-v3-merged-validation.md).

The prior isolated bulk tree and W5 copies are historical worker artifacts, not
current source baselines. Base future runtime work on a fresh copy of clean
HEAD `6a0e4c12c17`; preserve the sealed reviews and build evidence.

The matched controls are upstream `3a440b65695` and interim `eba351fee7d`.
G2 remains deferred until qualification and measurement. Ctypes/LRU source
implementation remains on the recorded explicit-approval hold. Canonical
status and plan own current assignments; older records below are history.

### Historical checkpoint measurement (not the current runtime)

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

Historical bring-up records below are retained for provenance; they do not
describe active packets or current performance. Landed since `b9d3e3a3c9b`:

- `d63c129b95f`: `METH_VALUE` splits on `METH_KEYWORDS`; keyword check in the
  trampoline; dict unhashable-key messages.
- `28453e14972`: W4a post-review fixes; lint covers 203 units.
- `1e19778c783`: `_PyValue_IndexExact`.
- `71566e96043`: stats-build fix; every check now compiles a stats build.
- `229065a3c2c`: W4a, accelerator modules.
- `62bfae99844`: arity checks in the trampolines.
- `f30173f3de3`: W2b marks.
- `61872dc3152`: specializer reaches `METH_VALUE` callees again.
- `ce9289e2ddb`: W2a, core Clinic with value emission.
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

1. Keep the completed full release/debug pass and exact source/build IDs as the
   functional baseline.
2. Audit the 11 static R27 findings and actual call sites before changing any
   declaration marks. Resolve the failed census: debt boxes, unfinished and
   orphan markers, incomplete identity, accounting errors, and the recorded
   `test_subprocess` error. Do not treat predicted-uncounted events as child
   counts.
3. Resolve the 11 raw-nonzero kernel rows in the current capture and rerun the
   strict startup-inclusive gate after accounting fixes.
4. The diagnostic timing run is active; publish its results only as diagnostic
   while R27 fails. Qualified kernel/pyperformance/RSS results require a clean
   strict R27 pass and the exact matched builds. Use the one-minute load guard
   (reject at load >= 4) and retain the full 1/5/15-minute tuple.
5. Write the checkpoint recap from qualified evidence. Ctypes and LRU
   implementation remain approval-held; G2 stays deferred. Serialization
   recovery and the cohesive method/tracing JIT follow the checkpoint.
## Coordination

Update the relevant workstream and STATUS at each landing, keep one WORKLOG line
per decision or concluded experiment, and mirror the project pages to local
Loom. Give the user a status report about every two hours while work is active
and at material milestones. A monitor due time is not a notification service.
The rename to `pyjit/` preserves historical `boil` branch, ABI and build
identities and `Tools/boil`. Do not touch `pyjit/archive/`.
