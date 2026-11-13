# M5 physical mortal bitmap implementation

2026-11-12. Design against the accepted M4 reference: shared arena, overlapping
argument transfer, initialized NULL named locals, and the same admitted images,
emitter events and initialized-value phases. Implementation is now active in
the isolated rival described below. The earlier GIL counted-owner schedule
does not authorize zero-bit reads or establish a complete comparison.

## Storage and hot addressing

Use one dense physical-address bitmap per arena chunk, coallocated at its tail
inside the existing total VirtualAlloc size. Reduce usable value/header space
to make room; do not append a small side allocation that rounds a 16KiB mapping
up another OS page. Keep `chunk->size` as total allocated/free size. Add one
`uint64_t *owner_bits` to the chunk and one to each interpreter frame. The
frame field points at the bitmap word covering its base. It increases the GIL
header from 112 to 120 bytes; it is not hidden in phase padding. Header frames
remain outside value ownership even though the bitmap covers their addresses.

Each bitmap word represents 64 physical cells aligned to a 512-byte address
boundary. Given existing handler `slots=frame->base` and logical home index i:

```
bit = i + (((uintptr_t)slots >> 3) & 63)
word = frame->owner_bits + (bit >> 6)
mask = UINT64_C(1) << (bit & 63)
```

All resulting word pointers stay inside the coallocated bitmap. Constructors
derive the frame pointer from the owning chunk once; hot writes have no
chunk-list search or thread-versus-heap branch. The pointer is valid after code
closure. C helpers obtain an equivalent words/first-bit descriptor once from
their actual frame/window capability. Raw windows without that capability
cannot update arena ownership. Prefix adapters add an optional word pointer
to their descriptor (24 to 32 bytes on 64-bit); record size remains 40. NULL
means the existing native PyValue buffer, which retains its existing visitor.

For a B-byte chunk, data offset d, T=floor((B-d)/8), and
a=((uintptr_t)data>>3)&63, reserve b=ceil((a+T)/65) bitmap words and C=T-b
physical cells. They fit because C+ceil((a+C)/64)<=T. Tiny end slack is allowed;
constructors check capacity rather than assume B bytes remain usable. With
16KiB and d=40, a=5 for the ordinary aligned mapping, b=32 and C=2011: 256 bitmap
bytes plus eight new chunk-header bytes reduce the old 2044 cells by 33. The
chunk-size doubling/cache admission calculation uses C, including the larger
frame header, spare home and separation cell. Zero b words on new allocation,
not on every call/cache reuse. Assert departing ownership and reused header
regions are empty; initialized/clean frontier bookkeeping remains separate.

Heap frames/generators coallocate bitmap words after N value homes. Allocate
ceil((N+63)/64) words, allowing an arbitrary first-bit alignment (at most one
extra word over ceil(N/64)); initialize them before any value publication.
Set the frame pointer to this tail. Allocation/free needs no code read after
closure; ordinary object free owns the whole block. Update all frame/gen
constructors, partial-failure paths and sizeof accounting. `_PyFrame_Copy`
must preserve the destination pointer rather than copying the source's.
Interpreter-entry homes live on the native C stack: their pointer is NULL and
their existing native-root contract remains. Fixed arena trampolines receive
the chunk pointer before their first self/result store. Public test/fake-frame
constructors must provide real bitmap storage or explicitly use the native
sentinel contract; NULL is not a fallback for an ordinary arena/heap owner.

| Hot lookup alternative | Concrete cost / decision |
| --- | --- |
| Chosen frame word pointer + derived first bit | One added header load when not CSE'd, base shift/mask, index addition, word shift and mask generation, then bit RMW. Same five handler arguments (six with stats). Compiler may retain pointer/phase in registers within a handler; do not assume this is free. |
| Thread chunk lookup on each write | Avoids a frame field only for thread frames; heap/generator frames need another route and suspended/ancestor writes need actual owning chunk lookup. Reject as initial implementation, not as a measured performance result. |
| Extra tailcall word-pointer argument | Hoists a load but changes every handler/entry/dispatch signature: six arguments, seven with stats. It still needs first-bit computation or another argument/register. Benchmark separately only if native evidence justifies its register/spill cost. |
| Two-field word pointer + cached first-bit | Adds another eight-byte field/alignment slot or cache parameter; header can reach 128. Saves phase calculation, prices more header stores/registers. Not selected initially. |

## Bit contract and actual write ports

One bit equals `PyValue_RefcountOnObject(canonical_home_value)`: an owning
count-tagged object edge. It is not local boundness, a global refcount test,
initialized inline-value authority, or a promise that the object cannot later
be immortalized. NULL, inline values, indices and no-count tags have no bit.
Apply classification after `_PyValue_Publish` canonicalization. Typed code,
function, mapping/frame-object headers and detached native owners stay outside
the bitmap, with their existing authority.

The emitter's actual transport events select narrow common home commits:
vacant store, replacement, detach and move. Their value store and bit update
form one callback/poll-free commit; do not maintain an independent handwritten
opcode summary. Known nonmortal vacant outputs emit no bitmap write only with
a positive vacant-bit proof; replacing an old mortal by a scalar still clears.
A mortal
vacant publication sets its bit. A detach clears a mortal bit before any close
or callback. Replacement classifies old/new tags already in C: unchanged
occupancy (including mortal-to-mortal) needs no RMW, otherwise set or clear.
Same physical source/destination is a no-op; distinct-cell mortal moves clear
source and set destination, batching same-word masks when callback-free. A
local borrow/TAKE-DUP leaves the local bit unchanged; its new native owner has
the selected native policy until published into another home.

| Actual source hooks / writers | Required port |
| --- | --- |
| Generator `IRStack.consume`, stores/final outputs, `IR_CONSUME_INPUT`, all IR_STORE/PUBLISH/MOVE/COMMIT variants and IR_CLEAR_SLOTS | Commit bits with the actual destination/consumed home. A redundant successful clear after helper consumption must not retire another owner. Discard/result-native paths never invent a home bit. |
| `_PyFrame_FillConstSlots`, `_PyFrame_Initialize`, trampoline/self setup, generic `initialize_locals`, defaults/varargs/kwargs/cell/freevar writes | Install bits for every mortal local/hidden constant as well as scratch. Constants replaced by mortal objects classify their actual values. Partial construction advertises only completed initialized homes; full NULL initialization starts with zero bits. |
| `call_transfer_exact_args`, overlap binder, call detach/refresh, native/step completion, return-slot publication, init child/shim | Transfer source/destination bits with the actual owner commit; current nullable local replacement reloads after callbacks. Plain thread overlap leaves argument bits physically unchanged. Header callable transfer clears its source bit. |
| `_PyEval_UnpackIterableValue`, unpack-to-locals fast/slow loops, NativeScope publication/abort | Write each value then set its bit before cursor publication. Moving to named locals updates both cells; native C/PyMem workspaces have no arena bits. Failure prefix remains covered through sequence/iterator cleanup. |
| `framelocalsproxy_setitem`, kwargs materialization, cell creation, redirect restore/retired-array detach | Replace/delete bits with observable binding commit before old close. Cell contents belong to the cell container; the frame bit describes its cell object only. A debugger move clears source before external retired authority. |
| `_PyFrame_Copy`, RETURN_GENERATOR, public frame-to-gen construction, resume sent-value installation, `_PyFrame_SetReturnValue` | Transfer selected readable values and mortal bits, reset/publish lifecycle together, never copy stale inactive words or reuse an old suspended recipient bit. |
| `_PyFrame_ClearTemporaries`, normal/error/gen/private-heap retirement, live `_PyFrame_ClearLocals`, observer prefix cleanup | Retire actual bit-selected owners within the correct initialized/phase region and progress cursor. Keep the distinct live local-mutation behavior below. |

This census includes raw `base[i]`, `fast[i]`, `locals[i]`, pointer-window and
`*sp` stores in ceval/frame/frameobject/bytecodes, beyond generated macros.
Before implementation, turn it into the same checked writer capability census
used by M4 and port test helpers/extensions that synthesize homes. Global
PyValue operations alone cannot update a bitmap because they lack home identity.

## Handoff, collection and lifecycle

Overlap keeps argument bits in place but changes interval authority: parent
WAIT excludes child/clobber aliases, child includes them, and callable-to-header
transfer removes its source bit. Caller survivor bits remain outside that
interval. Child pop proves no departing bit remains and restores saved_top/
limit using existing reservation state, independent of closed code. New chunk
capacity and the extra header word are charged; no per-call bitmap allocation.
Heap/arena moves transfer only authorized readable homes and their bits. Bit
masks may be copied in aligned batches, with shift/carry for different first
bits; source authority ends and destination begins in one nonreentrant commit.

The existing active/heap collector entry enumerates words intersecting the
published owner's physical intervals, masks endpoints, then visits set bits.
Associated frame objects must not duplicate active/gen edges. The same selected
value/phase authority remains necessary for inline/control reads, copies,
observers and diagnostics: unset bits grant no read permission. At shadow time,
compare every selected canonical value's count tag with its physical bit and
reject claimed bits outside owner intervals after endpoint masking. Bits for
adjacent owners sharing an endpoint word remain theirs and are not mismatches;
idempotent object marking alone cannot
prove unique edges. Cyclic subtraction and external root marking stay distinct.

Staged/retiring records retain actual range bounds, typed headers, exception
and result roots. Their visitor enumerates bitmap words for that frame range;
it never decodes code after its bit disarms. Arena output prefixes carry the
word pointer above; native prefixes keep their existing value visitor. Bit
clear precedes close and external detached-owner publication precedes any
callback. A detached native value/result has no stale frame bit. The record,
bitmap and reservation all outlive the final code/exception close.

Retirement reloads bitmap words after callbacks; it must not drain an old word
snapshot after a callback changed local occupancy. Live `tp_clear` preserves
its descending one-pass cursor: zero named homes in skipped bit-free ranges
without reading them, retire each mortal home before close, and preserve any
new binding above the passed cursor. Remaining lower bindings follow the
existing one-pass rule. Such passed bindings stay heap-owned and visible to
full local traversal, not a shrinking external-prefix fiction. Thread/private
deallocation cannot accept unsupported writes through a dead frame. Keep the
same M4 local NULL/cleanliness policy: bitmap scanning does not erase required
local NULL stores or Q admission fills. Suspension/resume and accepted redirect
retain the same exact phase/site and observer-mutation requirements as M4.

FT qualification needs a separate tag-policy review: count bits do not encode
deferred references. Preserve the selected initialized-home/tag scan for actual
deferred missing-count adjustment and noncounted external marking until an
equally explicit FT representation exists. Do not silently treat a zero bit as
absence of an object or add duplicate subtraction for counted owners.

## Cost and implementation gates

Persistent costs are eight bytes per frame/header, eight per chunk header,
8b chunk bitmap bytes (paid by capacity reduction), and
8ceil((N+63)/64) per heap frame/gen. For N=24 the heap tail is 16 bytes;
for N=224 it is 40 bytes. Header plus tail overhead is respectively 24/48 bytes,
before allocator rounding. Prefix descriptors add eight bytes on reached
adapters. No extra exec pointer or handler tailcall parameter is selected.

For P new mortal publications, T distinct-cell mortal moves and R removals,
the unbatched upper ledger is P sets + 2T updates + R clears. Occupancy-preserving
replacements and same-cell overlap arguments add zero RMWs; generic tag tests,
address calculations and conditional branches still count. Charge bitmap cache
line dependencies, local-bit work, word endpoint masks, zeroing on new chunks,
copy shift/carry, header capacity misses, helper spills and code bytes. Teardown
reads intersected words plus set-bit homes; keep named NULL stores and mutation
reload costs. Compare against static shape lookup/range/tag work and identical
phase publication, not an artificially uninstrumented bitmap baseline.

First freeze the M4 event/home capability and interval/phase selectors; then
freeze this storage/API with frames, GC and IR. Implement in a separate rival
from the same complete M4 commit. Start shadow-only with bits checked against
actual selected owners, then switch actual collector/retirement consumers in
one coherent variant. Reuse all call/binder/provider/unpack/weakref/escape/
generator/observer tests, add mixed inline/mortal local replacement, unaligned
overlap/move, bitmap word-boundary and reentrant passed-local tests. Root owns
builds, native update/address/spill census and direct selected54/static/bitmap
measurements. A full usable rival precedes representation selection.

The chunk-capacity inequality was host-checked for all 64 first-bit alignments
and T=1..8192. Native instruction counts and allocator effects remain unmeasured.
GC's bounded peer finds the GIL storage/handoff viable after correcting the
worked heap-tail costs and making shared-word endpoint masking explicit. The
FT deferred-reference limitation above remains a qualification gate.
IR's bounded peer is GO for the future rival under the same initialized-value
and event authority, with the positive vacant-bit proof and physical-home
capability requirements above explicit. Both peers approve the design direction,
not implementation or static authority. Freeze against the completed M4
interfaces before source changes.

## Authorized implementation and earlier GIL authority

2026-11-13. Root authorizes the full rival in isolated `call55-bitmap`, initially
at `9f28c0fa16d`. The actual storage/header slice has GC source GO: coallocated
chunk/heap tails, reduced capacity and exact cache admission, explicit 120-byte
GIL header, native-entry NULL pointer, frame/heap binding and address/update/
endpoint iterator helpers. The fixed API is in `pycore_home_bitmap.h`.

Generated consume/store/move/commit/clear hooks and the real C writers are
being ported. Native interpreter-exit homes have a checked emitter capability,
avoiding a nullable-bitmap branch on every ordinary home write. Return
publication handles its actual native boundary explicitly. Bit operations
update only when occupancy changes; physical distinct-cell moves currently
perform separate clear/set operations rather than same-word batching. These
costs remain in the eventual native ledger.

Root approves an earlier GIL-only authority schedule after **complete writer
ports** and a GC policy peer. A set bit is positive initialized counted-object
edge authority because actual canonical publication established it. Thus
counted roots and private retirement can read set-bit homes without completing
every general static callback map. Zero bits continue to grant no inline,
control, boundness or object read. Keep the same admitted images, finite
protocols, NULL named locals and physical transfer/call ABI.

The proposed arena root walk visits each chunk bitmap once, including live
unlinked reservations; typed active/staged/retiring headers and detached
results remain explicit. Shared parent/child argument cells must not become
two physical counted edges. Heap subtraction uses its own owner interval,
and every moved/detached source bit clears before callbacks. GC must peer
these shared-root/header/reservation rules before switching consumers.

Generic tp_traverse and gc.get_referents still need observable no-count and
immortal referents. Existing verifier-wide immortal roots justify reachability
for its explicit root mode, not replacing arbitrary visitproc semantics.
Collector/root-specific bit consumers must preserve that distinction. FT
deferred/no-count adjustment and marking keep their selected/tag policy; this
earlier scheduling makes no FT bitmap-authority claim.

No native job or authority switch has occurred. The full static comparator
remains open. Merge the complete M4 interfaces into the rival before the final
comparison; storage/writer progress is not performance or completion evidence.
