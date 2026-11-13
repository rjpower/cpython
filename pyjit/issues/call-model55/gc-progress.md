# GC and observation implementation progress

Owner: GC lane. Source census: selected54 `61015a9452d9069646fb5fede9ff963896833b89`
at `BASE/worktrees/frame54-cold-construction`; edits go to
`BASE/worktrees/call55-gc`. Canonical runtime files are not the implementation.
The official CPython AI policy and applicable agent/project guidance were read.

2026-11-12: M0 source census and M1 typed-reader peer review completed. Approved
M1 consumer ports are implemented in seven files in the isolated worktree:
`Objects/frameobject.c`, `Python/gc_trace.c`, `Python/gc_free_threading.c`,
`Python/tracemalloc.c`, `Modules/_remote_debugging/frames.c`,
`Modules/_remote_debugging/asyncio.c`, and `Tools/gdb/libpython.py`.
`git diff --check` passes; coordinator/frames source post-review resolved the
remote torn-sample range-check ordering. Root integrated the patch with typed
frame producers as `49c081b035821feb472faf59d905ad73bdcc6d4b`; compile,
focused checks and native review remain root-owned. The task worktree was reset
to that integrated base before M2 edits.
NULL homes, full-home traversal, and conservative
arena scans remain authoritative. No build, native execution, test, or performance
claim comes from this lane. Root owns these activities and integration.

## Census and migration gate

| Authority / reader | Selected54 source | Required migration |
| --- | --- | --- |
| Active precise roots | `Python/gc_trace.c:_PyGC_TraceVisitFrames` | Traverse each active frame's typed pins, initialized locals and authorized scratch; environments are extra marking roots. Interpreter sentinels are skipped. |
| Hidden retiring/staged arena roots | `Python/gc_trace.c:scan_thread_struct` | Current chunk values through `datastack_top`, headers from `datastack_limit`, whole older chunks retain unlinked owners during finalizers. Add explicit active/transition enumeration first; remove this fallback only at M4 after all producers migrate. |
| Other conservative roots | `Python/gc_trace.c:scan_structs`, native-stack/register, global-segment and pointed-to memory scans | Separate policy. Do not remove or treat as proof that detached C owners are registered. Thread/interpreter raw structs can still retain aliases; precise arena migration alone does not remove those aliases. |
| Cyclic edge subtraction | `Python/gc.c:_PyGC_VisitValue`, `_PyGC_VisitFrameStack`, `subtract_refs` | Visit actual heap/container counted edges once. Embedded refcounts have special handling; use the value API. Thread roots and detached transition roots do not introduce container edges. |
| All physical homes | `Python/gc.c:_PyGC_VisitFrameStack`; FT equivalent | Current full scan rechecks every non-index home and skips NULL/inline/index on visit. M3 replaces extent with initialized locals plus authorized scratch, retaining NULL redundancy. |
| FT deferred and optional marking roots | `Python/gc_free_threading.c:gc_visit_thread_stacks`, `gc_visit_thread_stacks_mark_alive` | Both currently read every home and code pin. Strong typed pins need no deferred adjustment; both independent pins still need marking. FT publication/snapshot qualification is separate. |
| Heap generator edges | `Objects/genobject.c:gen_traverse`, clearing and construction; `Python/frame.c:_PyFrame_Traverse` | Generator owns suspended frame edges; attached frame object does not traverse those same homes. CREATED prefix and closing states need explicit phase roots. Frames lane owns these producers. |
| Escaped frame edges | `Objects/frameobject.c:frame_traverse`, `frame_dealloc`, `frame_tp_clear`; `Python/frame.c:take_ownership` | Frame object traverses homes only when `FRAME_OWNED_BY_FRAME_OBJECT`; transfer values and authority together. Deallocation/clear must not read stale scratch. Retain map/code authority through last reader. |
| Observer locals | `Objects/frameobject.c:framelocalsproxy_getval`, `hasval`, `setitem`, `frame_get_var`, fast-to-locals family | Initialized named locals retain NULL boundness. Existing overwritten-local tuple protects borrowed reads; do not remove until callback lifetime proof exists. Mortal bitmap rival must update these local writes too. |
| Debugger redirects | `Objects/frameobject.c:register_resume_*`, `frame_lineno_set_impl`; `Python/frame.c:_PyFrame_CompleteRedirect` | Null-state/liveness analysis, tagged method anchor reads, restore arrays and full scratch retirement loops all need certified observer recipes. Pending-call overlap belongs to child until it retires. Detached `retired` arrays need prefix roots before closing callbacks. |
| Frame metadata readers | `Tools/gdb/libpython.py:PyFramePtr`; `Modules/_remote_debugging/{frames,asyncio}.c`; debug-offset metadata | Typed code field is a direct pointer, including NULL sentinels. Remote sentinel owner check must precede NULL-code test. GDB named locals remain initialized; existing PyValue pretty-print quarantine stays. |
| Other typed header users | `Python/{tracemalloc,sysmodule,ceval}.c`, `Python/ceval.h`, bytecodes/generated cases, `Objects/{typevarobject,sentinelobject}.c`, `_testinternalcapi` | Port in owning lanes/root; retain independent code/function lifetime and nullable-function entry kinds. |
| Existing C roots | `Include/internal/pycore_value.h:_PyThreadState_PushCValue`, `Include/internal/pycore_structs.h:_PyCValue` | Current helpers do not link a GIL precise root chain. They cannot stand in for nested transition records; native-stack discovery remains fallback until producer ports. |

The practical root census includes active caller/callee partitions, staged
binding/native adapters, dynamic initialized output prefixes, unlinked retirement,
independent C result, code/function/mapping/header owners, heap suspended/closing
generators and escaped frames. Each transition must state its unique counted
owner before and after commit. Aliased addresses may mark twice, but one counted
edge must never be subtracted twice. A reservation prevents reuse; it is no root.

The remaining gate is producer-by-producer publication coverage and shadow
comparison, followed by migration of every full-home/copy/clear/observer reader.
This census does not authorize stale scratch or deletion of any scanner.

## Immediate implementation boundary

M1 after review: ported typed header consumers in `Objects/frameobject.c`,
`Python/gc_trace.c`, `Python/gc_free_threading.c`, and remote/debugger readers.
Keep field names and offsets. Use `Py_VISIT`/`Py_CLEAR` for strong typed pins.
`Python/frame.c` and interpreter-frame headers remain frames/root ownership.
The M2 [interface](gc-proposal.md) was accepted by root/frames and implemented
in the GC lane: new `Include/internal/pycore_transition_roots.h`, one nullable
head in `Include/internal/pycore_tstate.h`, direct prefix/header/result visitor
in `Python/gc_trace.c`, and the optional FT marking consumer in
`Python/gc_free_threading.c`. No conservative scan is changed. The visitor is
compiled in all configurations so precise test-helper enumeration is available
without the verifier allocator.

The record covers only the first real retirement consumer: nested previous,
frame, remaining initialized prefix, header authority mask and independent
result. Source review found direct heap exception anchors could otherwise rely
on an untracked generator; root approved the actual-used EXCEPTION=16 mask bit.
The precise visitor marks it by generator/frame containment without code lookup;
producer removes it before exception close, including direct generator cleanup.
Future detached batches and dynamic prefixes land with actual producers.
Frames owns reached slow retirement through final code close/heap exception
cleanup and reservation assertions, plus the test helper that snapshots precise
root IDs during nested finalizers. Source peer review and merged candidate tests
remain gates. A precise-ID assertion must prove bridge coverage independently
of conservative scan success.

The GC M2 component is committed as `530f26f084e` for root integration. The
approved next [observer prefix](observer-prefix-proposal.md) adapter is implemented
as a separate delta over it: transition header/visitor, frameobject line-jump
retirement and a real line-jump precise-ID/mutation fixture. Prefix-only roots
authorize no frame homes, headers or result, so nested active-frame writes cannot
invalidate their root recipe. Fast closes allocate/link no record; the outlined
continuation owns the optional descriptor until pop, before array free. Host AST
parse and diff check pass. Frames source post-review and merged execution pending.

Observer prefix component is committed as `d2c5233fdf6`; frames source post-review
GO is in its peer file. Root owns candidate execution of the real jump fixture.
The [collector-policy audit](collector-policy.md) separates cyclic RC needs from
verifier/native policies and prices blanket scalar registration. Root retained
native conservative roots for M4 and approved a narrow CValue cleanup instead:
remove an unproduced FT head/impossible link loop and correct workspace comments,
with no new heads/stores or producer ports. Four-file delta is source-reviewed
GO by frames, diff check/reference census clean, pending root integration.

The narrow cleanup is committed as `65f72baec98` (root integrated separately).
Later components include stored retirement reason/status `f007a3aa68a`, cold
certificate probe `92291157adc`, existing-API heap binding clear consolidation
`6ef8868ac53`, and explicit record kinds `00b1b3b09f4`. Provider source/helper
and nullable-header peer dispositions are in provider-handoff-peer.md.

Constructor shim roots are implemented and frozen as `02bf14d56ec` after frames'
full source GO in init-shim-peer.md. The eight-file component shares typed exact
initialization/argument moves, outlines generic moved-owner binding with full
shim/child stages, blocks unsupported parent redirects with a scoped flag, and
keeps named bindings mutable. Failure branches precede generated DEAD clears;
root must inspect regeneration and the callback-free publication gap. Three
focused fixtures cover exact authority, failing heap-caller GC/local mutation/
redirect callbacks and changed-code child geometry. Host AST and whitespace
checks pass; merged generation/native execution remain root's gate.

The separate shared-emitter consumer delta now adds stored phase comparison to
the two existing frame-stack walks and `get_ir_frame_phases()` observation,
with the frozen IR phase field/ABI and shared trampoline initialization. FT
comparison requires an actual stopped world, since generic traversal is not a
foreign-frame quiescence protocol. Existing tag/edge policies remain unchanged.
This delta is committed as `b27a1a21330` after IR source peer GO. It requires
IR's real plan/query/publication implementation and tests. No scanner or NULL
authority has changed.

The following bounded GC delta adds accepted-redirect phase resets and
`check_ir_frame_phase` to exercise the actual cold selector/comparison with
initialized inline-only control homes. IR source peer is GO; no native run is
claimed. Integration uses IR's unified home-view declarations rather than the
temporary declarations copied into the GC lane.

The remaining CALL-hole prerequisite is specified in `call-role-proposal.md`:
canonical no-self and EX-kwargs roles preserve virtual NULL provenance without
reading reserved inactive cells. Runtime helpers use explicit roles; arena
windows initially withhold vectorcall OFFSET and retain native-copy prepend.
IR design peer is GO, frames peer and root encoding/source freeze are pending.
Selected-home capture/record lifetime conditions are in `copy-home-view-peer.md`.
