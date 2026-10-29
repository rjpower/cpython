# H3 after ROOTS: integration and source handoff preflight

Status: coordinator planning draft, 2026-10-29. **Source inspection only;
no merge, branch/worktree creation, native/generated edit, build, test,
probe or benchmark was performed or is authorized by this document.**
ROOTS2b must qualify and land before the H3 native union. Current native
and evidence branches stay frozen. No old receipt qualifies that union.
First preflight SHA256
`4c8bcb493cf5b4298ef27bdc318412b31f8dd71d1384a2abb7ebf5fe574cf98f`
proposed H1 before statistics. The coordinator revised that scheduling
choice to statistics before H1 for complete counter/prediction attribution;
this does not add a technical prerequisite to the H1 design.

## Inspected identities and prospective merge

| Object | Frozen identity / meaning |
|---|---|
| Primary | `58c98bec651`, native integration still queue `052d8342e56`; later primary commits include tooling and documentation |
| ROOTS candidate | `1bfe1a57145`, clean Park prerequisite on primary merge `d8be2e2c36d`; fresh qualification in progress, not landed |
| H3 live docs head | `81417aff097`; only stats design differs from the frozen storage/evidence head in native/test/tool paths |
| H3 storage/evidence | `ffc548603b823484fdf106b1a9de06785017c682`, branch `boil/h3-monitoring-storage-land`; native `76274ed06ec5116214fb3696f4f07e2ca8de4531` |
| H3/ROOTS and H3/primary merge base | `6c724c29408f4751b570c6980320801921be7e64` |

The storage evidence manifest remains
`a6c07b4ce7b2932a82c2bc8bcc187f8940359fe1674951f075ee1b98773e8557`.
[Frozen-checkpoint review](reviews/H3-1-stage2b2b-evidence-readiness.md)
clears that exact storage delta and evidence, **not primary LAND**. The
29 H3 paths under Include/Objects/Python/Modules/Lib/Tools differ from
the shared base; ROOTS changes 66 such paths. Five paths overlap, below.
These counts describe these two snapshots, not a future landing diff.

Read-only `git merge-tree BASE H3 ROOTS` reports all five as changed in
both, with no conflict markers. That is textual three-way preflight,
not an applied merge, generated closure or semantic compatibility result.
The actual future primary base must be named after ROOTS lands; repeat
this bounded ancestry/diff check if it differs from this native candidate.

Queue052 adds 13 non-boil paths absent from H3's ancestor: 12 native/test
paths and `Doc/c-api/subinterpreters.rst`. The native/test paths are
`pycore_gc.h`, `pycore_gc_trace.h`, `pycore_interp_structs.h`, the
multiprocessing/CAPI/GC tests, `_testinternalcapi.c`, `ceval_gil.c`,
`gc_live.c`, `gc_trace.c`, `pylifecycle.c` and `pystate.c`. ROOTS adds its
larger precise-root, helper-span and Park changes. An incoming primary
merge therefore changes native execution and checker/build policy;
calling it a documentation/tool refresh would lose that boundary.

## Shared source interactions to inspect on the union

| Shared path | Required composition, even if Git merges cleanly |
|---|---|
| `Lib/test/test_generated_cases.py` | Retain H3's handler-domain/pristine/relations/derived semantic-opcode cases **and** ROOTS's existing checker fixtures, including the DWORD positive and held-object negative. Preserve separate classes/imports and run the complete module. Neither owner may replace the other block to resolve drift. |
| `Modules/_testinternalcapi.c` | Preserve queue and ROOTS helpers/method entries plus H3's generated get_ir names, ir_scalar_boundary, code_safe_addr2line and three monitoring-storage hooks. Inspect includes and full method table for omissions/duplicates. H3's additive hooks use PyObject/VARARGS; Park's new METH_O hook lives in `Modules/_testinternalcapi/gc_walk.c` and uses PyValue argument/result with the cast. Never copy that ABI from an upstream example or infer it from equal register width. |
| `Python/bytecodes.c` | Keep H3's pristine/static-exec declarations, RESUME no-monitor eligibility guard, widened original-handler lookup and generated domain checks. Keep ROOTS's `_Py_Check_ArgsIterable(tstate, callargs_o)` call, matching its changed declaration/definition. Their current hunks are separate; emitted code must contain both. |
| `Python/ceval.h` | H3's monitor_handled no-monitor guard must coexist with ROOTS's precise declaration, exception root span and the other precise monitoring/raise helpers. Full-domain lltrace names must not discard ROOTS's supported outlined-tracing treatment. Inspect the composed collecting helper paths, not only conflict markers. |
| `Python/generated_ir_cases.c.h` | Regenerate from the composed DSL/generator. Retain the H3 eligibility/storage/domain expansions and ROOTS's args-iterable signature fix together. Never hand-merge this generated body. |

Other source interactions do not require same-file edits. H3's monitoring
storage/legacy tracing paths now run through ROOTS's precise call/monitor
helpers and queue-aware collector. The fixed five GetLine callers, alias
normalization, local-state preparation/OOM and CO_NO_MONITORING_EVENTS
suppression must still work with actual collecting callbacks. Synthetic
relaxed FOR_ITER/SEND and NOFB restoration stay actual-word controls;
uint16 65535 roundtrip remains a primitive storage claim, not execution
of a high canonical production handler.

The incoming ROOTS Makefile widens precise-header discovery and adds the
ceval.h dependency; its root checker recognizes DWORD, and linked inline
audits include outlined collecting-callee closure. H3's old 13-marked-file
check is historical. Discover and audit the **actual union** marked set
and final linked image; do not retain the old list as a reduced gate.
The two new DSL tokens in H3's lexer are not permission to skip C checker
branches/effects or alter ROOTS precision classifications.

## Documentation, helpers and receipt boundaries

Primary README/PROCESS/WORKLOG contain the current queue/ROOTS integration
order, O1 qualification exception, worker.main launcher correction,
source/product identity rules and shared-file grants. H3 has not edited
those files since the shared base; take the authoritative primary state,
then update it only through the coordinator's normal landing record.
H3's stage2b2b-work ledger retains its original source-scoped counts and
failures. H1-1's older paused wording and Og baseline are historical;
its primary readiness review supplies the current dependency interpretation.

Several shared H3-worker-main control artifacts have identical changes on
both lines. Preserve their immutable receipts. Newly imported primary
verification helpers are different run inputs: freeze the actual imported
closure and new plans, rather than editing old manifests or treating live
helper imports as interchangeable. Older frozen control logs and copied
negative variants remain historical references, not automatic new-head
executions. No Queue2 held work or probe is part of this handoff.

## Proposed minimal sufficient qualification of the actual union

This is the existing PROCESS ladder applied to a native eval/monitoring
union, not a waiver and not a replay of all historical campaigns. Freeze
one reviewed source state after normal primary merge and regeneration;
freeze exact plans, helper imports, configuration/compiler and complete
native products before any qualification. Actual state must identify
driver/workers, mode/stress, optimize/assertions and local CAPI origin.

1. **Union source closure and first feedback.** Review the five shared
   compositions and generated products; preserve the complete generated
   domain/property invariants, root checker and linked inline/traversal
   audits. Build/smoke default and the approved debug configuration.
   O1 is currently authorized for ROOTS qualification only; use existing
   H3 Og unless the coordinator explicitly extends/adopts O1 after its
   actual qualification. Never label an O1 build as Og.
2. **Affected runtime and falsifying controls.** Run the eleven H3 modules
   (`test_generated_cases`, `test_dis`, `test_ir`, `test__opcode`,
   `test_peepholer`, `test_compile`, `test_monitoring`, `test_code`,
   `test_marshal`, `test_sys_settrace`, `test_sys_setprofile`) on both
   builds and actual debug kill. Run actual queue1 and ROOTS/Park
   integration selectors, including all three thread-pool release-task
   cases and Park caller-slot/classification checks. Freeze discovered
   counts/skips/protected IDs; historical 1,370/12 and 22-test counts are
   comparison points, not presumed new inventories. Rebind the bounded
   allocation/synthetic-line/NOFB/OOM/eligibility probes to union binaries,
   including the exact watchdog previously failing on old H3. Preserve
   actual ROOTS stale-root/compiled-walk controls where stack layout and
   helper boundaries changed. No wholesale old31/11 ×20 replay is implied.
3. **Rung-end gates on that source.** PROCESS requires full release normal,
   full debug normal and full debug kill for this eval-loop change, plus
   cext, unchanged regeneration/Clinic, traversal audit and optional-module
   syntax checks. Rebind accepted ASan14 plus sort/functools actual-kill
   coverage and stress11 at N=1000, using only the existing applicable
   exact exclusions/options and actual mode checks. Keep each of the two
   eval probes at twenty independent invocations per required build, not
   just a method's internal loop: PROCESS explicitly requires these for
   eval-loop changes. This is separate from the old31/11 reclamation
   matrix, which is not replayed wholesale. No new failure exclusion or global
   opcode threshold follows. A second full-release stress100k campaign
   is not an automatic H3 rung requirement; ROOTS's once-only profile
   remains its own source-scoped obligation.
4. **Landing review against actual primary.** Freeze/push a new union
   candidate without moving the immutable ffc storage ref. Record the
   post-ROOTS primary BASE and new HEAD, final source delta, controls,
   failures and exact source/build-bound gate receipts. Only then use
   PROCESS's detached landing review on the managed review checkout
   (the reserved `boil-h3-storage-review` name is still only a reservation).
   A review of ffc against its old parent cannot become this LAND verdict.
   Findings are new fixup commits; rerun only levels affected by each fix.
5. **Primary integration.** Coordinator merges normally, regenerates if
   needed and runs the required fresh merge smoke/target/root quick-test.
   PROCESS §1 item 6 additionally requires integration full gates when its
   triggers apply. To avoid running the same final native tree twice,
   the coordinator may explicitly rule on carrying the immediately
   preceding union full receipts only after a complete source/helper/
   configuration/product identity bridge establishes that the final
   integration adds documentation alone. Without that ruling, or after
   any relevant difference/conflict, the integration full gates remain.

All full suites serialize because of shared profiler outputs. Reuse the
existing managed builds at quiescence, with standard locks, fresh caches,
timeouts and two normal test slots. Do not submit a second transaction
behind the same active build lock: it can occupy the other machine slot.
Source/helpers/native products stay frozen for each run. Gate4 remains a
step-end measurement; no loaded correctness run is a performance result.
The once-only broader tailcall/retain campaign remains its recorded
10a/H1 obligation, while changed union compiled-stack checks remain due.

## Source/build handoff before H1 and statistics

The coordinator's prospective sequence is **storage-union landing,
stage2b3 statistics, bounded H1 frame passing, then remaining H3 transport
and final renumbering**. One owner holds the shared generator at a time.
This is scheduling, not a new technical prerequisite: H1 remains
independent of H2 maps and frame-pointer policy. Neither readiness review
grants a native edit now.

Current INSTRUCTION_STATS guards updates with `op < 256`; valid higher
handlers are omitted from its counters. This source does not establish
an out-of-bounds update or memory corruption. The reviewed stats design
uses the generated domain (currently302) for complete entry/pair storage
and producer metadata. Close that known omission first and establish an
actual matching tailcall Py_STATS predicted-entry baseline. H1 must then
compile coherent stats and non-stats signatures/predicted argument lists.
Matched non-stats release timing still decides H1's benefit; complete
stats improve attribution without replacing that timing.

| Next owner | Required bounded handoff |
|---|---|
| H3 stage2b3 after union | Revalidate its ready design against the actual union. H3 retains generator/products and separate HandlerDomain hunks; coordinator explicitly grants only INSTRUCTION_STATS's domain/count changes in `ceval_macros.h`, preserving existing calling-convention and prediction behavior. Stats structures/producers/readers and metadata own the remaining declared scope. Coordinator separately owns the benchmark result-key/doc update. Qualify actual normal/tailcall Py_STATS entry behavior before the next handoff. |
| H1 after statistics | H3 freezes statistics and yields `ir_generator.py`, generated handler/target declarations and the eight generator expectations; coordinator assigns coherent `ceval_macros.h` tail-call signatures/predicted arguments and `ceval.c` start/error entries. Preserve the new stats counters/metadata and both prediction modes. ROOTS supplies exact positive/negative checker, preserve_none ownership/CFI and compiled-walk controls plus a quiescent qualified tailcall build. No frame-pointer-policy, H2 map or clearing-store change is bundled. |
| H3 transport/renumber after H1 | Freeze the qualified H1 interface and return generator/products to H3 under a new bounded assignment. Revalidate transport target identity and all generated domain consumers before final renumbering/removal of inherited numeric category exports. Do not change H1's coherent signatures or reopen ROOTS fixtures without a specific handoff. |
| Build ownership | Record exact source base, compiler/effective flags, Py_STATS/normal/tailcall mode, retained frame pointers and PGO/LTO state. ROOTS's existing tailcall build is not borrowed or rebuilt during its gates. A matching actual tailcall Py_STATS build is required before stats claims tailcall entry/prediction runtime behavior; normal-mode fixtures cannot supply that result. |

H1 must pass the actual current frame coherently through every handler,
label, initial/error and predicted transfer, asserting equality before
dereference and preserving IP publication/root/store protocols. Stage2b3
must count actual target entries while retaining semantic opcode/event
owners: normal predicted gotos bypass entry, tailcall prediction calls
entry again. Their shared generator/macros are why concurrent native
implementation is a poor default. No arbitrary operation-count target
or new fused operation is part of either bounded handoff.

The separate [string S0](pre-I-linear-string-proof-owner.md) now clears
readiness for owner-interface review only. It requests a pre-CREATE
code-owned pure-data pointer, private-constructor owned parameter,
explicit NULLs and actual-deallocation lifetime/accounting. Storage H3
currently changes monitoring/code validation interfaces but has not added
that field. A later code-layout assignment must review the actual union's
`_PyCode_DEF`, constructor/static/marshal/deepfreeze and remote offsets,
and H1/ROOTS segment/bitmap lifetime. Its requested complete effect decoder
is not already supplied by the H3 domain tables. This planning note grants
no layout, S1 or string native work.

## Coordinator rulings needed to execute

- Confirm ROOTS landing BASE, release the H3 native merge, name the union
  owner and approve an exact frozen qualification plan. Preserve ffc and
  every old receipt unchanged.
- Decide debug configuration from completed O1 evidence, exact compiled
  root/tailcall subset, accepted profile exclusions and adapter bindings.
  Any source-scoped integration receipt reuse needs the explicit bridge
  and ruling above; it is not assumed by this proposal.
- Confirm readiness at each boundary of the chosen statistics -> H1 ->
  remaining H3 sequence and grant precise shared source/build hunks. The
  benchmark rename and later string code-layout decisions remain separate
  coordinator assignments.

No source-conflict finding in this preflight requires changing the current
ROOTS candidate or reopening its frozen qualification. Unexpected union
failures are traced and recorded at the union boundary.
