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
