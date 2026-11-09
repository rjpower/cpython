# Initialized frame arena: independent design review

2026-11-09. Proposal-only review against measured `6ef45e391aa`, with source
inspection of chunk allocation/reuse, ordinary and overlapping pushes, copies,
clear/pop paths, heap frame construction and remote chunk readers. No runtime
changes or native jobs by this reviewer.

**Approve the initialized-frontier design for a bounded implementation.** The
frontier is a storage guarantee, not an owner map. Before removing the existing
initializer clears, the implementation must supply the retirement/copy census
listed below and update every reservation path. No performance or merged
correctness admission is implied.

## Why the selected invariant works

The proposal correctly rejects a bare allocation high-water mark. In the shared
two-ended arena, old frame headers can later become value storage. Truncating
the initialized frontier to the new header start before any header write makes
that memory uncertain again. A later reservation initializes the crossed range
even if it was once a valid value tail. A pop may retain the truncated frontier:
retired values below it are NULL, and the stale header remains outside the
guarantee. Capacity must prove the complete requested value range lies below
the newly reserved header before either initialization or header publication.

Initialization only fills an uncertain suffix. Every active value range is
already below the prior frontier, so it cannot erase an existing owner's live
cell or transferred argument. Reentrant retirement must use the chunk's current
frontier; restoring a saved frontier on outer pop would discard newer header
invalidation or initialization performed by callbacks. The proposal explicitly
forbids that restoration.

Fresh allocation must set the frontier to data and write PyValue_NULL itself.
`allocate_chunk` calls `_PyObject_VirtualAlloc`, so default OS zero-fill is not
an allocator contract. Cached chunks retain only the guarantee established by
their actual retirement and header history. Reconnecting previous/top fields
must not promote the frontier or assume the cache contains zeroed memory.

## Source obligations before removing clears

- Route checked/unchecked push, overlap, new/cached chunk, trampoline and
  external entry through the same reservation guarantee. The header address,
  value end and chunk must come from the actual selected reservation. Do not
  add a second unchecked skip-initialization ABI.
- Prove all departing thread-frame physical cells below the frontier are NULL
  after normal/error retirement and escape/generator copy. `_PyFrame_Copy`
  currently moves every slot and clears its source. Normal clear distinguishes
  locals from already-cleared scratch; audit the error-side scratch clear too.
  A successful-return assertion alone is insufficient.
- Retain the existing overlap rule: caller scratch above the transferred
  argument window is NULL. The caller's physical range can extend beyond a
  smaller child; those cells are still initialized even though they are not
  child homes. An arena guarantee does not authorize scanning stale owners or
  clearing an ancestor's live registers.
- Heap frame construction that calls `_PyFrame_Initialize` directly must first
  initialize its owned storage once. Full copies into newly allocated generator
  or escaped-frame storage already write every destination cell; avoid adding
  a redundant memset before those complete copies.
- Define active departing owners during cleanup re-entry precisely: unlinking
  a frame does not make its not-yet-retired cells free to overwrite. Reservation
  and stack pointers must continue to exclude that storage until retirement
  finishes. Test this under both ordinary and exceptional finalizers.
- Update allocation overhead/capacity arithmetic for the chunk field. Preserve
  pointer alignment, the spare owned_slots word and the strict value/header
  separation at maximum capacity.

Remote stack readers currently use their own local
`offsetof(_PyStackChunk, size/previous/data)`. A field inserted before data
changes their interpretation of foreign chunks. Rebuilding matching readers
is necessary but not proof for supported foreign targets: publish the layout
through the debug-offset contract or reject an incompatible layout explicitly.
Do not treat raw copied header bytes as initialized PyValues. The frontier is
not a new GC edge and needs no traversal.

## Alternatives and tests

A separate header arena removes header-to-value invalidation, at the cost of
another growth/caching/recovery mechanism. It is a reasonable fallback only if
the shared frontier proof fails. Full fresh-chunk clearing still needs header
invalidation on reuse; clearing a whole cached chunk spends additional work and
cannot repair missing retirement ownership. The selected suffix frontier is the
smaller change and directly targets the observed repeated tail fill.

Require dirty custom allocator memory, fresh and cached chunks, root reuse,
nested small headers followed by a large value reservation crossing their old
addresses, varying overlap sizes, constant/default homes, heap frame creation,
escape/generator copy, recursion/allocation failure, and reentrant retirement.
Debug checks should inspect only initialized cells and distinguish current
owners from inactive NULL homes. Add matched remote frame inspection.

Inspect release code for the added frontier checks and removed repeated fill,
then measure the same logging and broad-loss controls. The reported 11.99% fill
sample share is evidence of work, not an elapsed saving budget. The two-round
logging ratio 1.003646 is not parity; instruction/cycle ratios and timestamp/
metadata warnings retain the first report's limits. No benchmark or control
change is justified by this design review.

## Minimal remote layout contract

The remote await reader's IR_NO_DST exclusion is necessary but not sufficient:
`Modules/_remote_debugging/asyncio.c` still computes an address from an unchecked
return_slot. Add a target-derived upper bound before that address calculation.
The simplest source representation exports code offsets for co_nlocalsplus and
co_stacksize, then checks their nonnegative, overflow-safe sum as the number
of value homes. Alternatively expose co_framesize plus the target's header-cell
count. Do not subtract this reader's local FRAME_SPECIALS_SIZE from a foreign
code object's field.

Use one explicitly versioned fork target descriptor. A fixed discoverable
prefix should contain a fork-distinct eight-byte cookie, schema version,
descriptor size, Python version and a supported ABI/value-format identity.
Read and validate that prefix before reading/interpreting the full descriptor.
The current xdebugpy cookie and Py_Version cannot distinguish two different
fork layouts with the same Python 3.16 version. Unsupported schema/ABI must
produce a clear rejection, including command injection consumers, rather than
silently substituting local offsets.

Include current stack-chunk previous/size/data offsets in that descriptor in
the same coherent reader/validator/test batch. The arena can then change the
data offset value without another reader assumption or gratuitous schema
revision. Exporting the initialized frontier itself is unnecessary when no
remote consumer reads it. Update all repository discovery, initialization and
compatibility consumers of the cookie/prefix together; this rebuild permits
the private ABI break.

Source-only observation needs a separate coherent consumer update under this
same descriptor: interpreter-frame frame_obj, PyFrameObject f_frame, f_lineno,
source cursor/error offsets and validated source records. A reader must check
that the object still refers to the sampled interpreter frame and that its
active source record agrees with the actual compact predecessor before using
the override. Merely exporting offsets is not complete source observation.

The await bound is an immediate safety requirement for the current consumer.
The versioned descriptor and chunk-offset conversion should precede arena
integration; source-override consumption can be the next explicit observer
increment, with its incomplete status retained until tested. Avoid several
ad-hoc cookies or permanent fallback guesses for historical layouts.
