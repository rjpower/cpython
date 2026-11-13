# Allocation alternatives: source work and cost census

2026-11-12. Reference source is selected54
`61015a9452d9069646fb5fede9ff963896833b89`, in
`build/worktrees/frame54-cold-construction`. This is a source accounting, not a
timing result or a placement selection. M1 typed pins and M2 reached transition
records do not change the allocation facts below. M4 must compare placement
and validity policy explicitly; M5 must use the selected M4 policy unchanged.

## What the selected source actually does

Let `L = co_nlocalsplus`, `S = co_stacksize`, `N = L + S`, `K =
co_nconstslots`, `H = FRAME_SPECIALS_SIZE`, and `A` be supplied positional
arguments including an actual receiver. Slot indices and windows use one
logical domain `[0,N)`, with locals `[0,L)` and scratch `[L,N)`.

* `pycore_interpframe_structs.h:29-45` gives a frame one `base` and a
  reservation restoration pointer `saved_top`. Values grow upward and headers
  downward in one chunk. `_PyFrame_NumSlotsForCodeObject` derives `N` from
  `co_framesize-H`; `_PyFrame_Stackbase` adds `L` to `base`.
* `pystate.c:1364` allocates a chunk with `_PyObject_VirtualAlloc` and sets
  `initialized` to its first cell. `pystate.c:3113` initializes newly exposed
  cells to `PyValue_NULL`. `_PyStackChunk_EnsureFrameStorage`
  (`pycore_interpframe.h:487`) first truncates the frontier at the new header,
  then fills only a value tail beyond that frontier. Previously cleared reuse
  pays no fill. Header storage is not a valid value range after reuse.
* `_PyFrame_Initialize` (`pycore_interpframe.h:300`) does no `L`- or `N`-cell
  initialization loop. It installs the header and writes `K` constant homes,
  taking counts for mortal replacement constants. Generic binding
  (`ceval.c:2685`) requires NULL destinations, moves supplied arguments, and
  writes defaults, varargs and kwargs. Generic binding already has copying and
  allocation costs; do not attribute these to exact overlapping calls.
* `_INIT_CALL_PY_EXACT_ARGS` (`bytecodes.c:4331`) chooses the caller argument
  address as the thread callee's `base`. `_PyFrame_ReserveForCall`
  (`pycore_interpframe.h:579`) reserves the header and extends the physical top
  only if needed. The argument cells transfer authority without a value copy
  or count change. The caller tail after the arguments is required to be NULL.
  A heap/generator caller instead transfers arguments into the thread arena.
  The general GIL overlap binder (`ceval.c:3269`) also supports defaults after
  revalidation, then publishes the child without a callback in the commit.
* Scratch consumers NULL taken homes (`ceval_macros.h:539`), and ordinary
  return/unwind leaves thread scratch NULL. Thread retirement therefore clears
  only `L` locals (`frame.c:224`); exception scratch cleanup walks the required
  suffix (`pycore_interpframe.h:211`). Pop restores `saved_top` and the header
  frontier without a release fill. Debug pop checks the physical departing
  suffix, including overlap (`pycore_interpframe.h:531`). M2 preserves these
  reservation and NULL rules while publishing roots during callbacks.
* `_PyFrame_Copy` (`pycore_interpframe.h:240`) moves all `N` cells to contiguous
  heap `owned_slots` and NULLs all `N` source cells. Generator creation and
  frame-object takeover use this path. A direct heap frame constructor fills
  `N` cells before initialization (`frameobject.c:4069`); a copy destination
  needs no preliminary fill.

The selected frontier is both initialized-storage bookkeeping and a restricted
clean-reuse proof because every departing owner home is NULLed. It is not a
general clean-range structure. Removing scratch NULL stores breaks that second
property even if every stale word remains a well-formed value.

Source-level value-cell work can be accounted separately from header/PC/count
work. Let `U` be the uncertain cells exposed past the truncated initialized
frontier, and `Q` the local cells not directly initialized by binding and not
otherwise proved clean. These are path-dependent quantities, not `N` on every
call.

| Path | Selected54 cell work | What a candidate must charge |
| --- | --- | --- |
| Warm exact thread call | Zero argument moves; zero admission fill when `U=0`; `K` constant-home installs. Fixed header/window ownership stores still occur. | Shared/split NULL-local variants fill `Q` unbound locals when scratch becomes stale, unless a clean proof skips them. Local-validity variant initializes validity words and bound facts instead. |
| Cold or header-crossing reservation | `U` NULL stores in the outlined frontier extension, in addition to ordinary binding/constant installs. | Price actual local fills and any required initialized scratch outputs separately. Neither OS-zeroed pages nor an old header is automatically a valid `PyValue` home. |
| Heap caller exact call | `A` argument moves into the arena and source-home NULLing in `call_transfer_exact_args`, plus reservation/initialization. | Preserve this separate path; zero-copy thread-call claims do not apply to it. Split clean-local packing must not hide another `A` moves in the common thread path. |
| Generic positional binding | Up to `min(argcount,co_argcount)` argument moves, then flag/default-dependent writes; allocations can reenter. | Preserve the actual binder ownership/error paths. A changed placement does not eliminate defaults, kwargs or partial-publication roots. |
| Ordinary thread retirement | `L` local loads, `L` NULL stores, and conditional owner closes; scratch is already NULL. Pop changes reservation/header pointers. | Compare owner-map local/scratch sweep and any retained clean-producing stores. Do not count a removed entry store while omitting its replacement exit store. |
| Escape/generator move | `N` source reads, `N` destination writes and `N` source NULL stores in `_PyFrame_Copy`; direct heap construction separately fills `N`. | Maps may omit dead scratch copy/clear work, but locals/validity and observable heap state must remain correct; two spans need two extents. |

These counts describe C loop semantics, not emitted instructions. Zero-count
owners, NULL closes, batching, dead-store elimination and inlining can change
native costs. In particular a validity bitmap can initialize
`ceil(L/word_bits)` words and batch argument-prefix bits; it should not be
priced as necessarily one bit operation per local, nor as zero work.

## Comparable implementations

| Alternative | Entry and argument transfer | Exit, reuse and cold storage | Addressing, copies and map readers |
| --- | --- | --- | --- |
| Shared arena, NULL locals, mapped scratch | Retain overlapping argument prefix. Before callbacks, NULL every unbound local that could inherit stale scratch; directly install supplied/default/special bindings. Do not erase transferred argument owners. Constant homes still install. | Release authorized scratch owners without reading dead cells. Retire bound locals and restore the same reservation. For a local cell crossing unknown/header storage, produce valid NULL/binding before publication. | Keep one logical/physical base and direct slot loads. Active/staged/retiring maps replace full scratch reads; heap copy must copy only authorized initialized homes and establish safe destination local bindings. |
| Shared arena plus clean-range reuse | Same ABI; skip a local NULL write only when the complete relevant interval is certified clean, excluding argument/bound homes. Fall back to an explicit fill for dirty or unproved portions before publication. | Preserve known clean intervals through pop, cache reuse, header crossing and nested calls. Owner close and cleanliness publication have different times: no range is reusable while a callback can reach a retiring owner. Either retain clear stores to regain cleanliness, or record dirty ranges and pay later admission clearing. | Same direct base. Add cold/inline interval queries and updates, metadata storage and fragmentation costs. Maps still authorize scratch; cleanliness cannot authorize reading stale dead words. Heap moves update source/destination cleanliness independently. |
| Split locals and scratch, retain NULL locals | Give callee locals the caller's contiguous scratch argument prefix and reserve their remaining `L-A` tail there; put callee scratch elsewhere. This preserves argument moves at zero but does **not** guarantee clean nonargument locals. Initialize their dirty/unproved portion as above. | Retire local bindings and scratch owners separately. A single physical stack can reserve both regions with one restored top; genuinely independent arenas need separate growth/cache/restoration authority. Both must remain reserved through callback retirement. | Two physical bases, translated logical slot access or separately generated local/scratch operands. Heap frames can coallocate both regions, but copies/walkers must use two spans. Local initialization is not eliminated merely by moving callee scratch. |
| Split clean-local arena with arguments staged there | Produce each argument directly in the future local reservation while evaluating the call, then extend that reservation for remaining locals. Preserve callable/receiver ownership until commit. | Reserve/root partial calls across nested argument calls, exceptions, yield and dynamic `CALL_FUNCTION_EX`; reserve both arenas before commit. A competing design copies `A` values into clean locals at call time and must be charged for those moves. | Operand domains must address staged call outputs outside ordinary scratch. Extra staged reservation/root metadata and capacity checks precede ordinary child creation. Splitting argument and nonargument locals instead requires another span/indirection and changes contiguous local consumers. |
| Shared cells with separate local validity | Keep argument cells in place. Reset local boundness metadata; mark arguments/defaults/cells and observer writes bound. No NULL value store solely to represent an unbound local. Bound inline and immortal values still need validity; an owner bitmap alone is insufficient. | Release only valid local owners, invalidate before any close, and restore reservation. Validity must survive suspension/escape and update across callbacks, failed binding and debugger mutation. Scratch maps remain separate authority. | Every potentially unbound read and local walker tests validity before touching its word. Direct proven-bound reads can omit that test only with observer-aware proof. Heap copies transfer validity and only valid homes; foreign readers need the schema. |

For the dual-base split on one stack, let `B` be the child's local base and `P`
the physical top before the call. The current reservation ends at
`max(P, B+L+S)`. A simple split that refuses caller-tail reuse for callee scratch
places scratch at `max(P, B+L)`, ending at `max(P, B+L)+S`. Its additional
reservation is `min(S, max(0, P-(B+L)))` cells. Reusing scratch at `B+L` restores
the existing contiguous placement; it is not an independent clean-local arena.
These equations describe same-chunk geometry, not a measured memory peak.

A simple two-base frame adds one pointer unless another retained field is
replaced with an equally explicit derivation. Keeping both bases in interpreter
registers can add register pressure; resolving each mixed logical operand can
add a comparison/select or a branch. Generating distinct operand domains can
avoid that per-access choice but changes encoding, generator proofs and
admission. The existing local-role distinction already governs borrow versus
consume; it does not currently compute two addresses. Actual instructions and
spills require compiled evidence.

## Required reader and ownership changes

For every alternative, admission must retain the canonical logical domain and
map ownership through call overlap: after commit, the caller must exclude the
child-owned aliases, including initialized prefix outputs. Stale callee words
cannot become caller owners after pop. Staged binding/unpack prefixes and M2
retirement records need the same region/validity interpretation as active maps.

A split changes `_PyFrame_Slots`, `GETLOCAL`, `IR_SLOT`, window addressing,
return publication, `_PyFrame_Copy`, temporary cleanup and frame initialization;
`co_framesize` alone no longer describes both physical spans. Audit direct
`base[i]` users in `ceval.c`, `bytecodes.c` and generated cases, `frame.c`,
`frameobject.c`, `genobject.c` and `_testinternalcapi`. The generator and encoded
IR validator must preserve contiguous call/unpack windows within a declared
region and reject a window crossing its physical span. Split pointer arithmetic
must not manufacture a fictitious contiguous array across allocations.

A validity change also ports locals-proxy read/write/delete and cell/freevar
access, missing-argument checks, debugger restoration/redirect, frame evaluation
hooks and other direct local readers. A local's unbound state is externally
observable and can change through an escaped frame; an instruction-site static
map cannot alone replace that dynamic state. Preserve `pending_kwargs` as a
virtual bound empty kwargs binding until materialization; physical NULL does
not mean that binding is unbound. A valid cell/freevar home containing a cell
is also distinct from whether the cell's contents are bound. Validity metadata
cannot collapse these two levels.

Collector ports include `_PyFrame_Traverse`, `_PyGC_VisitFrameStack`, both FT
stack walkers and `_PyGC_TraceVisitFrames` plus the transition visitor. Ordinary
cyclic-GC container edges must remain distinct from external root marking.
Neither split layout nor validity permits reading dead/stale words while
enumerating heap-frame edges. Port remote frame/debug-offset readers and GDB
local rendering if base/validity layout changes; continue their existing
validation/quarantine policy. Replacing conservative arena enumeration requires
full active, staged, suspended, escaped and unlinked-retiring coverage under
each tested placement; native conservative policy remains as agreed in
[collector-policy.md](collector-policy.md).

## Evidence needed before selection

Implement alternatives as isolated placements over the same admitted images,
owner maps, call behavior, local semantics and workload set. First record source
counts by path: exact thread call, heap-caller transfer, generic binder, unwind,
frame escape and cold/cache chunk reuse. Then price actual local NULL stores,
argument moves, metadata stores, address instructions/spills, reservation bytes,
heap-copy cells and map iteration work. Distinguish cold fills from warm reuse;
report overlap misses and nested/staged reservations rather than hiding them in
an aggregate. Count `K` constant installs and callback-capable closes equally.

The source hypothesis is that the simple two-base split retains the same
mandatory local-validity work while adding address and reservation traffic.
That is not a timing conclusion. A clean-local staging design or explicit
boundness policy could change the tradeoff, but each needs the complete entry,
observer and teardown ports above. No allocation alternative is selected by
this census.

Frames peer GO is recorded in [allocation-peer.md](allocation-peer.md). IR peer
also reports GO: canonical logical domains must remain distinct from physical
spans, all windows stay within their declared span, aliases translate consistently
for calls/maps/copies/retirement, and local validity remains dynamic separate
state. Both peers checked the reservation equation and source traffic ledger;
neither found a required correction or selected placement. Documentation
whitespace checks pass. This work performed no native or timing jobs.
# Placement ledger peer

GC source/arithmetic peer GO for [placement-cost-model.md](placement-cost-model.md).
Its extra reservation equation, 112-to-120-byte base/validity charge, concrete
128-byte two-arena header, Q versus warm-U distinction, argument/copy traffic,
and large-local bitmap allocation/pop extent are consistently charged. Clean
argument production keeps its staged-owner and allocator obligations. Shared
placement with NULL local authority is a control recommendation, not a measured
winner or a rejection of a complete validity implementation. No runtime layout
change follows from this peer disposition.
