# Coordinator handoff

Operational snapshot: November 3, 2026. Read [STATUS](STATUS.md) for the project
summary, [ROADMAP](ROADMAP.md) for stage outcomes, [GOAL](GOAL.md) for standing
direction, and [PROCESS](PROCESS.md) before assigning work. This snapshot is a
restart guide, not a claim that workers remain active.

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
- Use Sol 6.1 for design and independent review; Luna for logs, inventories and
  finite mechanical recipes. The coordinator owns integration and shared builds.
- Follow design → independent review → implementation → focused tests →
  independent code/result review → commit → merged validation. A dependent
  source checkpoint may remain unbuildable; say so explicitly.

## Exact checkout and evidence locations

Paths below are relative to this repository root unless absolute. The root is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean`.

| Role | Location / identity |
| --- | --- |
| Canonical documentation | `pyjit/` in this checkout; older worktrees still contain historical copies |
| Primary branch before this documentation change | `weaver/python-boil-the-ocean`, `f9a11f991bb42b9bc08e4cf5035924c3b8a7ce87` |
| Restoration source W | `build/worktrees/pyvalue-rc-universal` |
| W branch / checkpoint56 | `boil/pyvalue-rc-universal`, `6bf198d1ef5ea9bce14b850bcff01e45d138cc41` |
| Configured debug build | `build/pyvalue-rc-debug-20261102-v1` |
| Latest completed merged probe | `build/pyvalue-merged-build-20261103-v22` |
| Documentation migration evidence | `build/pyjit-docs-handoff-20261103-v1` |
| Machine monitor | `build/jit-e2e-coordinator-monitor-20261101-v1/status.json` |

`build` is shared storage. Reuse the existing checkout and build rather than
creating another full tree. Use `uv` for Python commands, with the shared cache
at `/home/power/.cache/uv`. `/usr/bin/python3.14` has `test.support` for reference
controls; it is not the restored candidate.

The qualified interim source is `eba351fee7d`, with executables under
`build/rc-integrated-{release,debug}-20261101/python`. It has inline frame values
but boxed heap fields. Its ABI is `boilrc1`; restoration uses
`boilvalrc1-316d-x86_64-linux-gnu`. Neither is a new native JIT tier.

Restoration configuration is Linux x86-64, GIL, GCC, debug, mimalloc, computed
gotos and shapes, with `CFLAGS=-O1 -g`. Obtain actual commands and identities
from the configured build and retained native receipts rather than guessing.

## Actual source and build state

All runtime workers were stopped for this handoff. No background compiler,
benchmark or durable reporting scheduler is claimed. Recheck agents and
processes before assigning new work; old agent names are not running tasks.

W has an empty index and exactly two held modifications:

| Path | Current SHA-256 |
| --- | --- |
| `Objects/typeobject.c` | `ce8c0c45f1ea59c16b0762f9dc485f21e88c7cdfb641d4ce69d49005d3b59969` |
| `Lib/test/test_pyvalue_typecache.py` | `ae5d0a5e89587f3369e7d6d01c3926424d57dc19d89b77535d926705108876b4` |

Do not reset, overwrite or reapply patches to those files. The prepared held
projection already matches them; its eventual admission is an index-only
projection after fresh gates and independent review.

Merged v22 completed at **2026-11-03 01:46:23 UTC**. `make -k -j4 python`
exited 2 in 36.32 seconds: **247 compiler-error observations, 173 objects, no
executable**. All 7,362 input files stayed unchanged. Read `results.json`,
`identity.json`, `source-before.json`, `source-after.json` and `make.log` in its
directory. No later runtime build or restored-runtime timing has occurred.
Counts include cascades; they are not independent edits or a completion ratio.

Recent accepted source packets:

| Checkpoint | Commit | Outcome and limit |
| --- | --- | --- |
| 52 | `67210b9cf67` | Datetime twelve-function port: full C file/object clean, seven host controls |
| 53 | `5b993d71b1e` | Six itertools readers: 27 errors → one remaining batched error |
| 54 | `a714cb97c14` | Batched writer closes full itertools C file/object; four new host controls |
| 55 | `f7f53c043fc` | Enum full C file/object clean; selected map reduction fixed; builtin still has eleven other errors |
| 56 | `6bf198d1ef5` | E export lifetime provider/fixture: fixture object clean; getargs still has twelve unchanged errors |

None establishes candidate runtime execution. Detailed receipts remain under
the linked [consumer](stages/01-runtime/workstreams/consumers.md) and
[native-boundary](stages/01-runtime/workstreams/native-boundaries.md) records.

## First bounded task: held type projection

Inputs:

- `build/pyvalue-type-held-admission-review-20261103-v1`: independent finite
  audit, including `review.json` SHA-256
  `ef62e764cdedfd56a4a51127aca8b4c46a9d148a564b044501078c797df933ad`.
- `build/pyvalue-type-held-composed-20261103-v1`: materialized after-images,
  source patch, composition inventory and root-executed generator receipts.
  `coordinator-composer-results.json` proves all 7,362 source inputs and index
  unchanged. This is artifact generation, not native or runtime execution.

The audit accepts a dependent 67-definition delta: 55 prior endpoint definitions,
nine foundations and explicit residual MRO/counter/deletion changes. The full
inventory corrects an omitted unchanged same-line definition: 423→436 total
definitions, without changing the 67-definition delta. Includes and nonfunction
code are accounted for. Do not redesign a graph journal or MRO transaction.
Version zero can be a valid uncached state. Snapshot pins and displaced field
owners are separate owners even when their pointers are equal.

Remaining gates, in order:

1. Recheck HEAD, both held hashes, index and artifact receipts.
2. Run fresh configured native/full-object probes for the exact projection;
   earlier clean type probes predate the E shared-header change.
3. Run the nineteen isolated host reference controls. The full test module uses
   candidate-only `_testinternalcapi.type_cache_lookup` at class definition;
   the host cannot import it. Extract only `TypeGraphReferenceControls` (seven)
   and `TypeMroReferenceControls` (twelve), prove their ASTs exact, and label
   this isolated reference evidence. Forty-one full candidate methods remain
   unexecuted until the runtime links.
4. Obtain independent actual-code/result review. Stage only the exact two held
   files through the reviewed projection and commit the dependent checkpoint;
   do not apply its source patch to W again.
5. Validate the merged build. Lookup and guard work below remain mandatory
   before candidate semantic qualification.

## Type-system work ready for parallel preparation

**Legacy lookup closure:**
`build/pyvalue-type-legacy-lookup-closure-design-20261103-v1` has an accepted
independent review, `coordinator-grant.md`, and a required erratum. Author seal:
`3cf96e7fcd1111356baad1e08c085d3ae69e288e740dbdaefafee6922941c3c6`;
erratum SHA-256:
`b5f7703eb6f303f473f28a59453dad002318742d03d7e1ab52d8f34931b15c53`.
The old borrowed `_PyType_Lookup` can return a scalar box after releasing it.
Remove it and migrate its actual consumers. The corrected packet is ten full
operations: nine replacements and one deletion, plus declaration deletion and
two provider includes across seven paths. Eleven textual rows mean eight
calls, a definition, a declaration and a comment, not nine calls. The required
`_PyEval_SpecialMethodCanSuggest` parent pins its type through both fallible
lookups. Use the erratum's exact templates/hashes. No composer or implementation
exists yet. It depends on the held projection; native gates include actual
`Py_STATS` compilation, not a hardware-counter prerequisite. Six proposed
candidate controls remain unexecuted.

**Guard/feedback wiring:**
`build/pyvalue-type-guard-wiring-design-20261103-v1` is frozen under author seal
`85c1f0c893275146d0a30e628b0ae33cf8bd0fca362e0afbfa1d0b5f7699c7d3`.
Independent peer review is partial: **no verdict or independent receipt exists**.
Finish that review before assigning implementation. The minimal change uses
four DSL comparisons with `_PyType_MatchesVersion`, the current-state feedback
provider and its required nonescaping-function registry entry. Generated IR
has 23 comparison changes in 22 handlers. Eight baseline products reproduce;
only IR cases change, with the other seven products exact. This is generator
evidence, not C compilation or runtime evidence. Five proposed candidate
controls are unexecuted. The earlier claim that generated guards decode inline
scalars unsafely was withdrawn: existing PointerSpace checks already deopt
first. Do not reintroduce that rejected diagnosis. Free-threaded values are
outside the selected baseline profile.

## Native parser and Clinic critical path

E is committed; it retains actual exports in stable chunks through End. Keeping
only `view.obj` is insufficient. Star-output resources remain caller-owned.
E fixture has nine retained plus five new candidate methods, none executed.
Its receipt is `build/pyvalue-format-export-E-composed-20261103-v1/commit.json`.

The following **author-frozen templates are not peer-accepted or implemented**:

| Directory under build/ | Scope | Author seal |
| --- | --- | --- |
| `pyvalue-format-B-positional-design-20261103-v1` | Six backend/compatibility bodies and binding context | `10b1fc7d6483c6c05c2f552f3584d55a43cb02922a769e52c88712d537276b18` |
| `pyvalue-format-B-resources-design-20261103-v1` | Five conversion/resource bodies, export leases and pending cleanup | `680c2f7ce31be7db4f55af264f8552a18d876317000dc02bf501ec0da6eec44c` |
| `pyvalue-format-C-keywords-design-20261103-v1` | Five keyword bodies/four prototypes, selected owners and error snapshots | `a4642884d123b77e07987625472a15277a9a269a28e7b8481b3d17033f257a53` |
| `pyvalue-format-public-wrappers-design-20261103-v1` | Sixteen wrappers, owned UnpackTuple, public scope/End, three headers, six caller examples | `b121f2961da51a976e8a15f520317c7e75de571ad423317fac9dee5f3da1663b` |

Review these as one coherent contract before signature-changing overlays enter
live source. Selected before-bodies still match checkpoint56 despite E full-file
hash drift. Compose by exact full functions/declarations and the E receipt;
retain E's five added tests. Do not edit sealed old hashes.

Public first-error restoration is a proposed helper, not an already available
API. Preserve original scan/arity/allocation error precedence. Same-scope parser
reentry during an active parse is forbidden; End-only cleanup reentry has its
separate contract. Star buffers and O& resources require explicit cleanup.

**Clinic means CPython's Argument Clinic wrapper generator.** Producer changes
must emit the owning parser scope, preserve outputs through the implementation
call and converter cleanup, then End on every appropriate exit. Bulk generated
C edits alone will be overwritten and are not an accepted solution. Remaining
design work includes METH_O/setter fallback, optional-group parsing arms, keyword
fallbacks, collision-free scope names and object/int/status return cleanup.
Starting seams are `Tools/clinic/libclinic/{parse_args,clanguage,converter}.py`;
converter cleanup is assembled in `clanguage.py`. The six numeric caller
templates do not close the general caller/producer rule.

The lexical census (764 references, including tests and generated products) is
not a manual edit count. Freeze actual distinct caller families and reviewed
representatives, then fan out finite Luna/mechanical packets. Do not repeat a
broad census instead of preparing the next executable packet.

## Parallelism, integrity and next milestones

Use three bounded worker slots for type review/preparation, coherent parser
review/design, and remaining consumer recipes. Root serializes integration and
configured builds. Cluster the actual v22 diagnostics; the older v16 family
census is historical. Remaining builtin, exception, atexit, startup and bundled
consumers need recipes before bulk edits. Correct owning bridges need not be
made value-native before the first measurement.

An earlier enum/map artifact-only composer wrote live source. Root stopped it,
restored exactly its three hash-identified paths and excluded that v1. Corrected
v2 passed a full source/index guard. Review any rewriter's write targets before
execution; artifact-only workers treat W as read-only. Preserve failed attempts,
sealed reviews and distinct reviewer filenames. A prepared script is not a run.

The documentation rename preserves historical `boil` branch, ABI, schema and
build identities, frozen evidence bytes and `Tools/boil`. Original user changes
under `pyjit/archive/legacy/design/work/reviews/` remain unstaged: modified
`7a-owner-metadata-source-review.md` and four untracked 10a/7a review/design
files. Do not stage or clean them. Migration evidence records exact hashes and
the original index; no runtime source changed for this documentation task.

After the runtime links: run startup and targeted value/lifetime/parser/cache/
reentry/fault controls, then broad qualification. Only then obtain quiet,
matched JIT-off results across the full selected suite. Do not present host
controls, object files or compiler-error reductions as runtime performance.

Update the relevant workstream and parent STATUS at transitions, keep one short
WORKLOG line per concluded experiment, and mirror the project pages to local
Loom. Give the user a status report about every two hours while actively working
and at material milestones. A monitor due time is not a notification service.
For a new coordinator, first reconcile this snapshot with actual Git, processes,
agents and receipts, then assign the next reviewed packet.
