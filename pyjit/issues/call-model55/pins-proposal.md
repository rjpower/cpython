# M0 storage census and M1 typed pins

Status: proposal awaiting peer review. Source: selected54
`61015a9452d9069646fb5fede9ff963896833b89`; no runtime changes or native evidence.
Frames lane owns this file. The coordinator owns integration and execution.

## Storage decision for the first candidate

Keep shared placement, initialized/NULL named locals and the existing initialized
frontier through M3. This is the controlled reference, not a constraint of
register execution. The allocator grows values upward and headers downward in
one chunk; `saved_top` restores the physical reservation independently of `base`.
`_PyFrame_ReserveForCall` reuses a thread caller's argument cells in place and
extends only the uncertain tail. Heap callers copy arguments into the arena.
An ordinary exiting frame has already cleared scratch and clears only locals.
Warm admission therefore need not clear either whole frame or local range.

| Candidate | Admission and ordinary call | Retirement, cold growth and observation | Disposition |
| --- | --- | --- | --- |
| Shared cells with NULL locals | One base/reservation; existing argument overlap. M4 must NULL unbound locals that inherit stale scratch. Bound arguments/defaults install owners directly. | Clears local bindings; current frontier pays only uncertain tail initialization. Maps eventually authorize scratch. | Reference for static and bitmap candidates. Count added M4 local stores explicitly. |
| Separate locals and scratch | Two retained regions per activation; address translation or an extra base. Callee locals can reuse NULL local-only storage. | Must retire locals to NULL, grow/initialize each region, preserve caller scratch across a child, and repack or redesign argument overlap. Heap capture must preserve both regions. | Feasible but not a free entry-store elimination. Defer a separate placement increment until complete packing/base/retirement costs can be measured. |
| Shared cells with proved clean ranges | Reuse known NULL intervals without clearing. | Dirty scratch, overlap, cached chunks, header crossings, heap moves and callbacks must update interval authority; costs include branches and fragmentation. | Current frontier already supplies a restricted monotone proof. Broader cleanliness needs measured metadata and reuse benefit. |
| Explicit local-validity map | No value store solely to mark a local unbound; reset validity words and set bits for bound arguments/defaults. | Update on store/delete/observer mutation; test on potentially unbound reads and every locals walker/copy. Release existing owners independently. | Separate representation change. A mortal-owner bit cannot distinguish unbound from initialized inline/immortal values. |

This comparison does not establish a faster placement. M4's accounting must
include newly introduced local initialization, not credit all removed scratch
stores as savings. M5 uses exactly the same placement and local-boundness policy
as the static candidate.

## Typed ownership contract

Keep the existing field names and offsets: `f_executable` becomes
`PyCodeObject *`; `f_funcobj` becomes nullable `PyFunctionObject *`. This replaces
two eight-byte fields without changing the 112-byte GIL header, hot caches,
NULL-home authority, call overlap, return publication, or reservation geometry.
No sidecar, extra image owner, ownership tag, or per-call allocation is added.

Both fields own strong references, with ordinary immortal-aware
`Py_NewRef`/`Py_CLEAR` and object traversal. Selected54 already counts these
owners under FT: its `PyValue_DUP` counts mortal pointers; it does not produce
deferred header words. The stale deferred comments do not describe that code.
Typed pins retain this ownership policy even for objects eligible for deferred
refcounting. FT root/subtraction consumers must distinguish counted object
edges from deferred stack references; do not synthesize an extra deferred count.

The executing code pin remains independent of the function pin.
`function.__code__` replacement cannot change frame geometry, image pointers or
code lifetime. The code pin remains through frame clear where the public
generator/code API requires it. A copied frame transfers the function, mapping,
association and homes; escaped-frame transfer gives the departing source its
own code pin until its caller clears it. Preserve selected54's constructor
association acquisition and failure/publication ordering.

`_PyFrame_GetCode` and `_PyFrame_GetFunction` return typed pointers with assertions.
`_PyFrame_CodeHome` is removed; code acquisition is `Py_NewRef(code)`.
`_PyFrame_Initialize` retains its callable `PyValue` parameter for the value-call
ABI and consumes its already-proven function owner with
`PyValue_AsKnownObjectSteal`; there is no fallible box or generic decode on later
metadata reads. Keep callable conversion at that boundary rather than changing
the complete call ABI in M1.

Entry kinds use existing owner/state plus explicit nullable pins:

* Interpreter entry/base frame: `owner == FRAME_OWNED_BY_INTERPRETER`, both pins
  NULL; trampoline instruction pointers remain explicit execution authority.
* Actual trampoline activation: thread owner, nonNULL code pin, NULL function;
  locals are NULL and no borrowed environment is dereferenced.
* Ordinary/heap frame: code and function pins are present.
* Allocated generator before frame transfer: cleared state with NULL code;
  deallocation/traversal must tolerate this failure-safe construction state.

Safe traceback access still checks freed-pointer heuristics before type reads.
Remote readers must recognize interpreter entry owner before rejecting NULL
code, so their chain walk continues past sentinels. The debug-offset wire field
`executable` can retain its name and offset while reading a raw pointer; update
all local consumers and comments together. No compatibility shim remains.

## File and interface boundaries

Frames lane implements `Include/internal/pycore_interpframe{,_structs}.h`,
`Python/{frame,ceval,pystate}.c`, `Objects/genobject.c`, callable metadata reads in
`Python/bytecodes.c`, `Python/ceval.h`, `Python/sysmodule.c`,
`Objects/{typevarobject,sentinelobject}.c`, and the targeted internal pin helper
and Python regressions. Root regenerates `Python/generated*`, reviews shared
headers and integrates strict API declarations if needed.

GC/observation lane ports `Objects/frameobject.c`,
`Python/gc_{trace,free_threading}.c`, `Python/tracemalloc.c`, debug offsets,
`Tools/gdb/libpython.py` and remote/debugger/native readers. Changes to
`Python/frame.c` go through the frames lane. Every use found by a final
`f_executable|f_funcobj|_PyFrame_CodeHome` census must use the typed contract.
The `_testinternalcapi` hook consumers are coordinated with the root/IR lane to
avoid concurrent edits; the frames lane supplies their small required patch.

## Evidence and falsifiers

Source ledger: ordinary entry acquires one independent code count and transfers
one callable owner; ordinary clear drops function once and code once. Escape
adds the explicitly required temporary source code pin. Heap moves transfer
pins without adding counts. Retain all direct environment/feedback accesses.
The generated source ledger must count eliminated tag checks/masks separately
from compiler-optimized native work.

Root checks release/debug header size and offsets, exact and generic calls,
ordinary/keyword/default binding, generator/coroutine/asyncgen construction,
public-constructor failures, context/init trampolines, frame escape/clear,
PEP523, tracing/monitoring and collectors. Focused weakref regressions cover
active code replacement, suspended and completed heap frames, independent
function lifetime and escaped frames; mortal/immortal pin helper checks counts
without depending on PyValue tag bits. Existing frame/code tests remain.

Native review compares code acquisition, hot GetCode/GetFunction users,
call/return, SEND/YIELD, constructor and teardown. No native reduction is assumed
from a C type change: record actual loads, masks, branches and count operations.
Any lost owner, stale image view, changed constructor failure ordering, extra
hot dependent load, or changed GIL geometry blocks integration. FT source ports
are required; GIL validation alone does not qualify FT.
