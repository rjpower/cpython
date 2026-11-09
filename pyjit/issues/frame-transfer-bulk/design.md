# Full-range frame transfer experiment

Candidate worktree `boil-frame-transfer-bulk`, branch `boil/frame-transfer-bulk`,
base `b3f2bcc605c`. The one-header patch keeps `_PyFrame_Copy`'s existing contract
and replaces its scalar payload copy/clear loop with `memcpy` then `memset`.
Capture both bases, keep all header assignments, transfer every physical slot,
and leave the complete source payload NULL. No shortened prefix, DSL/generated
change, IR magic change, new frame representation or size threshold is proposed.

## Caller and lifetime proof

`RETURN_GENERATOR` allocates a fresh generator via `_Py_MakeCoro`; its embedded
owned slots cannot overlap the existing source frame's thread/caller window or
generator allocation. `_PyFrame_Copy` completes before state becomes CREATED,
ownership becomes GENERATOR, and the returned object is stored in the caller
before source-frame pop. Keep this exact order, including the newly fixed
pre-TRACK origin slot initialization and possible origin-allocation failure.

`take_ownership` moves a source whose owner is THREAD or GENERATOR into its
escaped frame object's separately allocated `_f_frame_data`. Its assertion
excludes an already FRAME_OBJECT-owned source. The source payload belongs to
thread storage or the generator allocation, not the frame object's spare data.
Keep `f_frame`/owner rebinding and the subsequent fallible `f_back` construction.

`gen_new_with_qualname` allocates a new generator and transfers the existing
frame object's `_f_frame_data` into it. Two concurrently live allocations cannot
overlap; preserve frame-object rebinding and the existing ownership sequence.
The generated RETURN_GENERATOR case is generated from the first caller, not a
fourth lifetime model. All callers therefore meet memcpy's nonoverlap contract.

The helper itself performs no allocation, callback, detach or publication
safepoint. libc copy/clear are already classified noncollecting. Two-pass copying
temporarily duplicates references but changes no observable lifetime: the source
remains anchored and there is no collection between copying and clearing.
`Include/pyvalue.h` defines `PyValue_NULL = {0}`; zeroing the bytes preserves
the existing cleared value. Physical slot initialization remains complete,
including unused NULL temporaries and a genexpr's live iterator temporary.

## Cost question and scope

The old O3 scalar loop occupies 2.476% of the captured coroutine CPU, within a
6.884% complete-copy context. This is motivation, not expected savings. libc
call overhead, two passes and small frames may make the candidate slower.
Inspect O3 instructions before the bounded matched comparison. Keep one generic
implementation; reject a losing experiment rather than adding a size cutoff.

Only `Include/internal/pycore_interpframe.h` is changed. Existing tests cover
generator payloads, resumers, escaped frame data and constructor collection;
do not add a test that just mirrors memcpy. Add a small behavior regression only
if review identifies an actual uncovered transfer contract. No GC source is owned
by this issue; serving/baseline products remain frozen.
