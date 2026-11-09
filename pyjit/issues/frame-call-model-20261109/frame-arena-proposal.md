# Initialized frame arena

Proposal, 2026-11-09, against measured `6ef45e391aa`. Requires independent
review before implementation. The separate semantic-completion increment is
already in progress; this proposal must not silently expand that admission.

## Evidence and problem

The [first report](integration-report.md) records near logging parity, remaining
broad losses and the counter/profile limits. Candidate self samples include
11.99% at libc offsets `0x198424`/`0x19842d`, reached from `_PyFrame_Initialize`
in exact and general overlapping calls. The recorded libc build's disassembly
shows short vector memory fill; the retained candidate calls `memset@plt` at
general-handler offset `+0x332`. This identifies actual initialization work,
not a promised 12% elapsed saving or a cause of every broad regression.

Current overlap knows only the caller's physical slot end. A child extending
beyond it clears its new tail on every call, although the previous child has
already cleared those cells. Logging's hot code has no constant homes. Keep
real argument/default/constant ownership; remove redundant initialization by
making initialized storage an arena invariant.

## Selected candidate and alternatives

Retain the existing two-ended chunk: value cells grow upward, frame headers
downward. Add one per-chunk initialized frontier. This costs no per-frame field,
per-register bitmap or ordinary instruction check. Reservation initializes
only a range not covered by its guarantee. Frame construction consumes
initialized storage; it does not rediscover that guarantee from caller size.

Separate header/value arenas would avoid mixed storage, but add independent
growth/recovery/caching mechanisms. Consider that if the frontier cannot be
proved cleanly. Clearing an entire chunk on every reuse spends more work and
hides missing retirement. Clearing only once on fresh allocation is unsafe:
freed headers can later become value cells. A bare high-water mark that ignores
header writes is therefore rejected. `_PyObject_VirtualAlloc` delegates to an
installed arena allocator; do not assume returned memory is zero-filled.

## Invariants

Let `data` be the chunk's first cell, `V` the active value-stack top, `H` the
lowest reserved header and `Z` the initialized frontier, all within that chunk.

1. `data <= V < H`, and `data <= Z <= H` after reservation completes.
2. Every value cell below `Z` has defined canonical bits. Active owners retain
   their declared frame/home ownership. Every **inactive** cell below `Z` is
   NULL. Initialization does not assert that live cells are NULL.
3. Every active frame's full physical value range is initialized. Suspended
   ancestors own no trailing scratch above the active overlapping argument
   window. Prove this from the existing call-window ABI and all other pushes;
   do not infer it from a non-NULL test or logical stack depth.
4. Retirement clears/moves each departing owner before its cells become
   inactive. Escaping frame/generator copies leave source cells NULL. The
   frontier itself is not an owner and is not traversed by GC.
5. Writing a header invalidates any initialized guarantee for that memory.
   A later value reservation crossing that former header initializes it again.

Use a typed frontier pointer or checked cell index, not packed unrelated fields
or a false local. Choose representation after reviewing remote/raw layout
consumers. One source of truth per chunk is preferable to a duplicate cached
thread-state cursor unless compiled work justifies the latter.

## Reservation and retirement

A common reservation primitive receives the actual new header address and
requested value end, after the existing capacity check and before publishing
the header:

1. Truncate `Z` to the new header start if header storage overlaps its former
   guarantee. Check the requested value end is strictly below that header.
2. If the requested value end exceeds `Z`, write NULL only in `[Z, end)` and
   advance `Z`. Existing live owners and transferred arguments lie below the
   old initialized frontier and are not touched.
3. Construct the header and commit stack pointers through the existing ABI.
   No Python callback or GC poll is added inside reservation/commit.

Same-chunk pop restores value/header stack pointers but retains `Z`: retired
value cells remain NULL, while stale header memory stays beyond the frontier.
Reentrant calls during owner retirement can extend or truncate the frontier;
the returning outer pop must not overwrite that newer state with a saved copy.

Fresh chunk allocation sets `Z=data` without relying on allocator contents.
Cached chunks retain their valid frontier after all departing frames retire;
reset links/top separately. Switching chunks uses each chunk's own frontier.
Freeing a chunk discards it normally. Audit root chunk reuse, large frames,
recursion overflow, allocation failure and nested external interpreter entries.

The shared frame initializer should initialize the header and actual constant
homes, with initialized value storage as an explicit precondition. Update all
thread push/overlap/trampoline callers together. Heap-owned frame construction
must initialize its owned storage once at allocation before calling it;
generator/frame ownership copies write every destination cell and clear sources.
Do not retain a boolean skip-initialization parameter or competing frame ABI.

## Required source proof and consumers

- `Include/cpython/pystate.h`: chunk layout and documented frontier.
- `Python/pystate.c`: allocation, push_chunk, cached reuse, pop/switch/free and
  thread lifecycle. Check capacity calculations after the header size change.
- `Include/internal/pycore_interpframe.h`: checked/unchecked push, overlap,
  frame initializer, trampoline, copy and pop. Keep argument/window ownership.
- `Python/ceval.c`, `Python/frame.c`, `Objects/frameobject.c`: generic binding,
  every ordinary/error clear, escape/move and heap frame constructor.
- Generator construction, suspension, exception unwinding and semantic
  completion: every path releasing a thread value range must establish NULL
  before reuse. Do not assume successful-return assertions prove error paths.
- Remote stack copying uses local `offsetof(_PyStackChunk, data/size/previous)`
  in `Modules/_remote_debugging/frames.c`; audit matched and supported foreign
  targets. Publish/validate layout through the existing debug-offset contract
  where required. Rebuild affected consumers; do not silently parse old layout.

Root has not yet proved every inactive-cell path. A missed write or premature
frontier advance would be a correctness bug; the next reviewer must try to
break these invariants before implementation is assigned.

## Validation and decision

First use host/source checks and targeted native probes, then combined tests:

- Repeated narrow calls extending beyond their caller, varying child sizes,
  defaults/kwargs, exact/general/native entry and argument overlap.
- Nested small frames followed by one large value range crossing former
  header memory. Exercise fresh and cached chunk growth and root reuse.
- Recursion/entry failure, generator construction, frame escape/copy and GC;
  mortal locals, callbacks/re-entry during ordinary and exceptional retirement.
- Supported frame/remote inspection. Debug checks distinguish live canonical
  cells from inactive NULL cells without reading beyond initialized storage.
- Compiled paths and profiles: repeated warmed calls must avoid tail memory
  fill while preserving header/owner stores. Compare added frontier guards with
  actual removed work; no sampling-percentage speedup promise.

At integration run the relevant ownership/frame suites, logging plus the broad
loss controls and independent post-review. Full release/debug and the broad
screen belong to coherent checkpoints. Keep this change only if correctness
holds and matched measurement supports the intended effect. No workload guard,
benchmark edit, collector change or second compiler is part of this design.
