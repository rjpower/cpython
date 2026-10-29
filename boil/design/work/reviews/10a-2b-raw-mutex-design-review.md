# ROOTS2b raw-mutex wrapper prerequisite design review

Verdict: **IMPLEMENT_RAW_MUTEX_MARKERS**, design only, 2026-10-29. No
blocking source-design finding. This does not authorize a source edit,
compiler/control execution, support retry, qualification or landing. The
coordinator must first settle the current source freeze and grant the next
bounded transaction. The reviewer executed no native build, test or probe.

## Reviewed identity and necessity

Source is clean ROOTS `1bfe1a57145f8d32c27558a981023146333ee37d` in
`.worktrees/boil-10a`. Evidence below is relative to its
`build/takeover-20261029/park-prerequisite/` directory.

| Artifact | SHA256 |
|---|---|
| `raw-mutex-design/DESIGN.md` | `fb29181708a7bd8480adcd5323ffdf9dcf9fa946ec887572a93c68f435e93c78` |
| `raw-mutex-design/proposal.patch` | `34b5e35e6b585675fd90f59ad5817ab813d2f1509caec2bc8d0904eddc8e44a4` |
| `raw-mutex-design/source-map.json` | `dd1d9c7c2a13b96a2253b77a772bd70a8b807d3a7a9cdad53ef74777aa2eda09` |
| `raw-mutex-design/pycore_lock.proposed.h` | `115df33d5cb7c8ed99298a5ec551a525d28947f7e711221c7ced46a6e3d3b5bd` |
| `support-builds/asan.log` | `27b2a2c876f9bdba7f9ad3b1f1e95e9b9a1ae8fa161d8d22049f715e8ab0f320` |
| `support-builds/asan.build.json` | `9c7dfe4b4c3fe95548ec3699c9b58e3b704cd4294ec922bd2d03c38fd53cc411` |
| `support-builds/asan.launch.json` | `61a5b63e54e8af09af7d64fc2195e77b452d38d39cb7f549cd40b9230de89825` |

The seven mapped source/tool hashes and three failure-artifact hashes match
the retained files. Comparing the complete proposed header with actual
source shows exactly the two stated marker additions at
`Include/internal/pycore_lock.h:121,131`; every body and slow-path declaration
is unchanged. The source map is a bounded seven-input map, not a complete
transitive platform inventory; the clean commit identifies the additional
sources examined below.

The actual ASan make returned 2 after 73.901 seconds. Its linked audit names
these two outlined static callees of precise `_PyParkingLot_Park`, outside
the precise section and absent from `OUTLINED_OPAQUE`. The receipt remains
`BUILD_FAIL`; no smoke ran and no tailcall build followed. This is a concrete
audit failure of the present source, separate from any allocator-test issue.

## Ownership and collection proof

Each wrapper owns only `_PyRawMutex *m` and one `uintptr_t` CAS expected
word (`pycore_lock.h:108–140`). No managed object, value, payload slot or
callback is introduced. The GCC, standard-atomic and MSVC implementations
of the CAS operate on raw words (`pyatomic_gcc.h:124`, `pyatomic_std.h:206`,
`pyatomic_msc.h:191,265`). Repository searches find ten calls, all in
`Python/parking_lot.c:338–422`, with `&bucket->mutex` in the static 257-entry
Bucket table (`:27–36`). No managed container owns the mutex argument.

`Python/lock.c:200–273` gives the slow paths native stack wait entries,
raw list links and a semaphore. The contended lock links its own waiter,
waits until an actual wakeup, retries acquisition and then destroys the
semaphore. The EINTR retry and waiter lifetime rules remain unchanged.
Unlock removes a waiter by CAS and wakes its semaphore, or releases the
word; an invalid unlock enters the existing fatal path. Neither slow body
receives a Python payload, invokes a Python callback, detaches or runs
pending calls. These external functions remain opaque and remain
default-reentrant calls to the checker. Precision of their wrappers does
not require an effect-table exemption or precision of the slow bodies.

The semaphore representation and all three backends were read
(`Include/internal/pycore_semaphore.h`; `Python/parking_lot.c:67–273`).
Windows uses handles and native waits; its SIGINT branch resets the event
and returns an interrupt status. `_PyOS_SigintEvent` only reads that handle
(`Modules/signalmodule.c:2082`), and `_Py_IsMainThread` compares the native
thread identifier (`Python/pystate.c:2975`). POSIX indefinite waits use
`sem_wait`; pthread waits use native mutex/condition/counter state. Error
paths retain their existing fatal behavior. Other-platform conclusions are
source review, not new runtime coverage. The marker is empty outside the
supported core ELF compiler configuration (`Include/pyport.h:280–290`),
and unsupported stack-walk platforms retain full conservative scanning
(`Include/internal/pycore_gc_walk.h:46–64`).

Park still borrows caller-owned storage through pinned detach and later
reattachment (`parking_lot.c:324–386`). Its output-object slot remains rooted
by the caller under the existing Park contract. Unpark's callback occurs
in its unchanged opaque caller frame, between lock and unlock
(`:392–410`); it is not a callback inside either wrapper. UnparkAll's native
list transfer and wakeups also remain opaque (`:415–429`). The markers
exclude only the two wrappers' frames. The walk still unwinds through their
CFI and attributes preserved caller registers to their owning outer frame
(`Python/gc_walk.c:931–980`). No caller-root obligation is removed.

## Checker and falsification requirements

The existing source-build scan includes marked headers
(`Makefile.pre.in:984–994`), and `gc_inline_audit.marked_sources()` includes
them too (`Tools/build/gc_inline_audit.py:237–251`). Its linked outlined
check at `:420–435` and cold/clone checks remain intact. This classification
fits the existing mechanism; neither an opaque allowance nor an inlining
flag change is needed. Retain these bounded controls in the next separately
authorized plan:

1. Check the actual complete proposed header with the real callee inventory,
   requiring both precise definitions to be parsed and checked. On copies,
   inject an unanchored object made before, and used after, each slow call.
   Require the held-local diagnostic naming that local and the corresponding
   slow call. A zero-error result without body/definition accounting is
   insufficient. Leave the unknown-call/default-reentrant classification
   intact (`root_check_c.py:1622–1672`).
2. The extracted linked fixture must retain byte-identical wrapper bodies,
   both live outlined symbols, collecting external slow stubs, full DWARF
   and the precise caller. Force outlining only in the fixture. Old helpers
   must produce exactly the two named outlined findings; both markers must
   pass; omitting each marker must restore its own finding. Check actual
   symbol sections and audit the linked image: a relocatable object alone
   skips the outlined check. Preserve compiler/flags, extraction inputs,
   ELF/DWARF and logs. These controls test the audit oracle, not reclamation.
3. A fresh real ASan make must pass the unchanged source and linked audits
   and freeze its new native inventory before any later qualification.
   Preserve the old failing artifacts. An additional outlined dependency or
   managed state finding stops this scope; it does not grant more markers,
   slow-body changes, checker changes or allowances. Existing caller-slot
   and saved-register fixtures remain relevant; their Park-detach witness
   is not a claimed witness of a contended wrapper's slow path.

Only the two production marker lines are within this design. Control copies
and retained artifacts belong to the task's build evidence directory. No
shared checker, committed fixture, runner, callback, semaphore or slow-path
edit is covered.

## Source and native reconciliation boundary

The default/debug nineteen-iteration matrix remains bound to clean `1bfe`
and its original products. Finish or otherwise settle that freeze before
any header edit. Do not relabel its receipts with a new source identity.

At the coordinator's quiescent handoff, retain the complete old source,
compiler/configuration, helper and native maps and completed receipts. Bind
the exact new two-line source delta and all new products. Compare default
and debug emitted wrapper/caller code, symbol sections and unwind metadata;
do not assume the wrappers were inlined or that a section attribute leaves
code generation/layout unchanged. A matching body alone is not a complete
native bridge. Include all rebuilt products and alias targets, not only the
interpreter executable.

The later read-only owner inventory `raw-mutex-design/current-wrapper-symbols.json`
(`ecc13bcf4041147cf6a556dd13a0bd0d1364ed1d037c801ea77e997e412bf6ce`)
records no wrapper symbols or clones in current default/debug binaries and
both outlined wrappers in the failed ASan binary. This explains the observed
build difference; symbol absence before annotation proves neither future
inlining nor complete output equality. No reuse ruling follows from it.

The coordinator can then review an explicit bridge and affected rerun plan:
fresh actual linked audits, applicable stale/walk/Park caller-slot/root and
thread-pool coverage, and the separate support-kind/debug/assertion and
failure-control qualification. Any changed evidence must retain its own
source/product binding. This review prescribes neither automatic reuse nor
an automatic repeat of all 120 transactions. Tailcall make, support smoke,
full tests, variant adapters and later landing each keep their existing
separate authorization and review boundaries.

## Round 2: task-copy control evidence

Verdict: **TASK_COPY_CONTROLS_CLEAR**, 2026-10-29. The preceding design
review remains history. This closes its source-checker and linked-oracle
controls, not its real ASan build or source/native reconciliation boundary.
Production source remains clean `1bfe`; no source update or build retry is
authorized by this disposition. The reviewer read retained files and
checked hashes without running the checker, compiler, audit or fixtures.

Reviewed `raw-mutex-design/controls-v1/plan.json`
`2d80a1c24d6ea3900a96314f83e34943ddd3d40ac53ebc7a4a68e9db940fe7da`,
`result.json`
`a6c0617cdf24ca58720e7b22df3d8defd3911c6b6d6d67e1b4df0eddf21593ea`,
and `build/takeover-20261029/raw_mutex_controls.py`
`6adfa48c0df3a5ef6ee47d6dfc916e2f5a4aa2fc76281b6d5252a4bce24b07fd`.
All 32 retained artifact hashes, the plan's 813 default/debug/ASan native
entries, the pilot's complete frozen source map and the actual GCC binary
hash match current bytes. The plan binds the unchanged proposal/header and
pilot identity. The runner checks the actual inherited ASan build lock and
one of the two machine slots before compiling, then repeats source/helper/
compiler/native guards before writing its non-overwritten result.

Both complete proposed-header definitions were explicitly parsed as
precise, and each extracted definition was checked equal to its old bytes
plus the marker. Both actual-header source positives pass. Each copied
header inserts `held = PyLong_FromLong(7)` immediately before its own slow
call and reads it afterward. Each produces exactly one held-local error at
that unchanged slow call. These are expected checker failures; no managed
object was actually created or collected by the control.

The four retained fixtures are linked PIE executables with `.debug_info`,
both outlined wrapper symbols and recorded precise-section membership.
Their wrapper bodies match the extracted source. GCC15.2 uses `-Og -g
-fsanitize=address` and fixture-only outlining/IPA restrictions. The old
variant has exactly both named outlined findings; the fully marked variant
has none; each omitted marker restores exactly its own named finding.
The recorded ELF/nm/DWARF/audit logs agree with those outcomes. The CAS
implementation and collecting slow stubs in this fixture are simplified
oracle scaffolding, not replacement production synchronization code.

The runner contains no target-interpreter or compiled-fixture invocation;
its subprocesses compile or inspect retained files. No runtime, contention,
platform, reclamation, ASan-safety or performance positive follows from
these controls. The original actual ASan `BUILD_FAIL` remains unresolved
until a separately authorized real-source build. Any new outlined finding
still stops the two-marker scope. All original source-freeze, native bridge,
qualification and coordinator-grant requirements remain in force.

## Round 3: post-quiescence transition design

Verdict: **TRANSITION_DESIGN_CLEAR**, 2026-10-29. Reviewed ROOTS
`raw-mutex-design/TRANSITION.md`, SHA256
`8efe77a5151ca5bdb250a84180d2471c2fc92dd8c5985a69d3dde86164a832a6`.
No native execution, source change, preservation copy or owner-file edit
was performed by the reviewer. This clears the schedule and evidence
boundaries; exact boundary/snapshot and executor plans remain unreviewed,
so no production edit or rebuild starts from this disposition alone.

The plan preserves both complete `1bfe` matrices and their pilot iteration
zero before any edit, with immutable source/helper/configuration/compiler/
product bindings. At review time source was still clean `1bfe`; the retained
release completion matches its stated hash
`63235055669267369b32a1ae04101f599b222d0176f7ce1c045ac1aa7d558c06`.
This review does not newly audit or claim completion of either matrix.
Both completed receipts and actual process/build quiescence must be checked
at the handoff. Preserve every old product path's alias/target identity and
bytes, including separately labeled failed ASan and old tailcall products.
Content-addressed copying may deduplicate bytes without collapsing path or
build provenance. The 542-path default/debug set is the accepted runtime/
configuration inventory, not a claim to preserve every intermediate object
in the build directory. Keep its original coverage definition explicit.

**Commit-order ruling.** PROCESS section 1.3 and gate 1 normally require
targeted tests before every commit. Section B instead commits the exact
marker change before fresh native builds to obtain a clean guard identity.
The coordinator explicitly accepts this **one-time exception**: only after
both old matrices finish and the preserved, quiescent boundary verifies,
the exact two-marker patch `34b5e35e6b58` may be committed on the strength
of the independently reviewed source and task-copy source/linked controls
(`a6c0617c`; round2 review `b7d5eaf5`). This exception establishes source
identity only. It is neither native gate1/PASS nor landing qualification,
does not cover another source hunk, and creates no general policy. A new
clean HEAD, exact header hash and complete approved-delta source guard must
be recorded. Old HEAD-bound `loaded()` guards must fail on new source and
remain unchanged. No amend, rebase or attribution trailer is introduced.

The make-only sequence is sound: retain exact configuration/compiler
inputs, admit one transaction per build under normal slot/build locks,
run fresh default/debug/ASan makes with bootstrap actions identified as
build steps, then capture complete NEW products under lock before smoke.
Tailcall has its own make/product freeze. Any additional outlined finding
stops this two-marker scope. Executor identities, timeouts, pre/post guards,
failure receipts and lock lifetimes still need their concrete plan review.

Full retained ELF payloads make later independent decoding possible.
Compare every changed runtime product and alias, including executable code,
symbol/section membership, data/relocation layout, CFI and root-relevant
DWARF/inlined ranges/locations. Summaries should identify changed sections,
symbols and relevant root/unwind differences; use focused decoding where
needed rather than enormous text dumps for every ELF. Preserve complete
bytes and tool identities so concise reporting does not discard evidence.
Report version/build metadata differences too. Current absence of default/
debug wrapper symbols does not establish future output equality.

The mandatory **STOP for coordinator disposition** after those comparisons
is correct. Neither a clean source delta nor equal selected machine code
automatically rebinds old 120-transaction evidence. Old receipts always name
`1bfe`. The coordinator must decide the native/source/precision/unwind bridge
and affected repetition scope from actual results. No automatic carry-forward
or automatic complete 120-transaction repetition is prescribed here.

The proposed bounded fresh set is appropriately scoped: strict stale100,
complete StackWalkTests with real Park/caller-slot/detached-register controls,
the three original thread selectors, both separate eval invocations and
whole test_generated_cases on release/debug/debug-kill. The StackWalkTests
source includes the real Park control; its method listing is not a runtime
count guarantee. Apply actual-kind/debug/assertion/mode/seed/main0/domain/
zero-fallback/protected-skip checks in a newly frozen adapter, preserving the
39-case policy and honest selection-based success inference. New ASan and
tailcall qualification remain bound to their actual fresh products, with
only accepted selectors and no invented release-kill or ASan1000 claim.

The full new-source chain in section E stays required through separately
reviewed plans and grants. The old source-only full draft is not an execution
identity after the header change. Fresh driver/scheduled/returned module
rosters and actual counts/skips/resources must be observed; the source roster
alone does not prove execution. Once-only tail/retain work remains the
existing 10a4/H1 handoff. No O1 adoption, baseline setup, benchmark, root
policy change or weaker exclusion follows from this transition review.

## Round 4: executor and actual preserved boundary clear

Verdict: **TRANSITION_EXECUTOR_AND_BOUNDARY_CLEAR**, 2026-10-29. This
clears the exact executor and completed preservation boundary for the
coordinator's separate production grant: exact two-marker commit under the
round3 exception, `bind-new`, then sequential make-only default/debug/ASan,
stopping on failure and before qualification. No reviewer execution,
production edit, commit, copy or owner-artifact change occurred.

Reviewed ROOTS transition identities:

| Artifact | SHA256 |
|---|---|
| `build/takeover-20261029/raw_mutex_transition.py` | `4e44a595c303ac72b4203f2675ce6ef2070aa677077ae7450424b20b870b448a` |
| `transition/spec.json` | `ef5fb2fb5532cc9096f142bfa2b68b941bf2736907d7b9525bfaae27ce58a6af` |
| `transition/frozen-manifest.json` | `713fa2f3fccca053918c5b85eee4b51c592e16f240675666b94a9c52cd28be8a` |
| `transition/REVIEW.md` | `5a344f98daed04296fbdbc26ef1fee4178f377e10c2dfda8536f3949f95b4d18` |
| `transition/host-controls.json` | `2f00258c1858c8fda65ca3ab2418e24283e95743b4b7f74fdd71a039d3716ab6` |
| `transition/before.json` | `8b7c757892f9729655ce48e2b90306d76112611921b51e01658d30db03b6c860` |
| `transition/boundary-validation.json` | `799623d57b2f8f356ffce75b915f2786a953adc1ae35cc25ba6ab734bcbc39c1` |

Here `transition/` is below `park-prerequisite/raw-mutex-design/` in the
existing build evidence directory. The earlier executor/spec/controls are
retained under `review-v1/`. V2 adds actual archive/checker/auditor tools and
corrects make-versus-wrapper failure handling; it does not change runtime
source or broaden the two-marker patch.

The actual boundary remains clean `1bfe`, tree
`712048b168386b94222ccb8c9da282e4bc3b2e3f`. Both completed matrices are
represented by 962 preserved completion/result/log/state/root paths. The
executor replays each build's 114 exact new result identities and six pilot
transactions through unchanged raw state/root/skip-contract validators,
checks aggregate correspondence, and takes all four build locks without
waiting before copying. The reviewer checked all 962 path hashes, all 6,322
source-map hashes, all 597 copied payload hashes and all 1,144 current
expanded product payload paths. Manifest source-audit/tool hashes also
match. Actual quiescence and the executed preservation step were separately
authorized and observed by the coordinator; this review does not rerun them.

Each build now has 286 prospectively preserved paths, with aliases/resolved
targets and type/size/device/inode metadata. The original 271-path maps
remain separately retained. The 597 deduplicated payloads occupy
1,207,028,085 bytes; deduplication confers no shared-build/lifetime claim.
Default/debug retain their completed `1bfe` scope, ASan retains BUILD_FAIL
without smoke, and tailcall remains historical. ELF build programs,
archives and generated sysconfig/build-selection inputs are now included
prospectively. No thin archive was found; thin-member closure would be
required before claiming such an archive as a qualified input.

The inspected old launcher/selected-source paths use the covered Python
driver for GC and multiprocessing children and do not invoke pre-existing
testembed/bootstrap/freezer programs. This is source correspondence, not
complete process-lineage proof. The coordinator's accepted historical
loader-fixture ruling remains explicit: temporary shared-library bytes were
not retained; fixed C strings/literal compiler command/toolchain provenance
and actual load/unload/walk assertions suffice for that historical scope.
No retrospective ELF hashes, extra wrapper or old-matrix rerun is required.
Prospective compiler/configuration binding is required for later such runs.

`inherited_lock` now proves the inherited open description already owns a
FLOCK WRITE lock, confirms actual file identity and tests independent-open
conflict before touching its flock. The sixteen host controls exercise
held/unheld/wrong-inode locks, raw-root domain and roster/completion failure,
extra-source/config/alias drift, make-environment overrides and exit-code
polarity. Raw make2 plus host1 is retained failure evidence; make0/host1 or
make2/host0 cannot qualify. These are host guard/control results, not native
build or runtime passes.

Actual archive tools, GCC-selected assembler/linker/frontends, host checker,
ELF auditor, shell/UV/host Python and compiler bytes/versions are bound.
Make's allowlisted environment excludes ambient make/compiler/loader/Python
overrides. Exact Makefile/pyconfig guards run pre/post; expanded config/log/
sysconfig inputs are preserved as before/after evidence rather than falsely
treated as immutable make outputs. Serial explicit stages, fixed make-j4,
1,200-second bounds and normal gate slots/build locks retain the reviewed
scope. New complete products are copied under lock before any smoke; the
outer launch re-enumerates the complete expanded roster and aliases after
cleanup and binds its log/build receipt. Unexpected failure or incomplete
evidence stops progression.

The successor guard requires exactly one clean child commit, the sole
approved header difference and exact substituted full-source hashes. It
does not call old HEAD-bound guards to qualify new source. Other builds
must retain their old or already-frozen new products. All new products
remain explicitly unqualified. Code/precision/CFI/root-DWARF comparison and
the coordinator's bridge/affected-repeat ruling are still required before
separately reviewed smoke, affected checks and fresh full-chain plans.
No old120 relabel, automatic carry-forward/repetition, tailcall build,
landing, baseline or timing grant follows from this clearance.
