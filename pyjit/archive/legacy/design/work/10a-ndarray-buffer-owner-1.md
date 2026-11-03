# ndarray acquisition owner: source checkpoint

This source-only change is isolated on boil/buffer-ndarray. Parentdb0256990c26
composes reviewed PickleBuffer/scanner627590 and Unpickler/owner-API48b021,
both based on87b6. No native build or target execution qualifies this patch.
The inherited scanner .group() fixture failure is retained separately; its
reviewed test-only .span() successor has not been silently applied here.

## One owner, original metadata

The private _testbuffer module uses the existing internal managed-buffer API.
Each consumer has one typed owner; PyObject_GetBuffer writes directly into that
owner's stable master. Its cached staticbuf.base is a borrowed copy. No release
operation ever receives the cached copy: multidimensional item selection shifts
its shape/strides/suboffset pointers, slicing clones/modifies arrays, and
add_suboffsets adds private metadata. None changes the original master, including
its exporter, buf, ndim, array pointers or internal release token.

| Path | Ownership / lifetime |
| --- | --- |
| Consumer construction | Prepare owner first, then publish owner + cached view + staticbuf head without another allocating call. Failed acquisition publishes no head/owner. Legacy obj==NULL acquisitions keep their existing external-storage contract. |
| Multidimensional item | New consumer owner acquires parent ND; cached shape/strides/suboffsets may advance, original master remains unchanged. |
| Slicing | New consumer owner acquires parent; copy_structure gives cached arrays ND_OWN_ARRAYS before mutation. Allocation/error cleanup can free those arrays independently of the original acquisition. |
| add_suboffsets | A consumer first clones borrowed shape/stride arrays if needed and records ND_OWN_ARRAYS. The added array belongs to cached metadata, never master. |
| Outward normal getbuf | Checked increments of both head.exports and owner.exports; returned view.obj is ND, retaining its transformed arrays and owner. |
| Outward releasebuf | Consumer decrements both counts exactly once; no allocation, Python callback or release of the upstream acquisition. Producer release retains its original ndbuf/internal-token path. |
| ND_REDIRECT | Acquires a separate upstream buffer; neither ND counter is incremented. Requires the ND owner still active first. |
| Traverse | Visits owner, which visits original master.obj. Cached base.obj is borrowed, not a second owning reference. |
| Dealloc | Frees only ND-owned cached arrays and drops owner; never calls PyBuffer_Release on cached metadata. Producer ndbuf storage behavior is unchanged. |

owner.exports is a readiness count for registrations/consumer exports, not a
count of strong references. A live ND roots its owner through traversal even
with zero outward exports. A dead owner with outward leases is unready. When
the last consumer export is released, a later collection may select the owner;
no new wakeup/scheduler is introduced. Core queued release marks the owner
RELEASED before callbacks and preserves original master metadata through them.

## Closed-state and constructor policy

Every ND entry that reads cached data/metadata, returns cached obj, mutates a
descriptor, hashes, slices or exports checks head/initializing/owner-active first.
This includes all fourteen getsets; item/subscript/assignment; list/bytes
conversion; push/pop; add_suboffsets; memoryview_from_buffer; hash; getbuf; and
the module's direct is_contiguous fast path. Generic module operations obtain
buffers through the guarded getbuf. Releasebuf, traverse and dealloc intentionally
do not use this access gate: they must finish bookkeeping after closure.

An exporter may retain/resurrect ND during release. Its owner is already closed,
so all these readers reject before inspecting cached exporter/array pointers.
Only a rooted current operation may use a borrowed active master; ND's strong
owner field keeps that acquisition live across ordinary collecting callbacks.
Original master never borrows ND-owned transformed arrays, so ND destruction
cannot invalidate release metadata retained by its owner.

Successful initialization is now explicitly one-shot, including producers. This
is an intentional private test-API behavior change approved for this slice.
No existing ndarray.__init__ reuse was found in tests/docs. Reentry is rejected
before argument conversion/acquisition; a successful object's state is unchanged.
Consumers already reject structural mutation via push/pop; producers have named
push/pop operations with their existing export checks. Prior repeated __init__
bypassed those checks or overwrote an embedded consumer acquisition.

An unsuccessful first init publishes no head and may be retried; flags reset at
the next attempt. A no-exports replacement policy alone would not suffice: scalar
pack/unpack can hold raw pointers across user conversions without an outward
export count. Supporting arbitrary replacement would need an additional active
operation lifetime contract. This patch does not add that framework. Existing
producer push/pop conversion-reentry hazards are not resolved or claimed safe.

## Controls and bounded validation proposal

Three new ordinary-API tests cover:

* Exporter→ND cycle: every closed reader/getset/reexport rejects during release;
  original callback data survives nested collection, a raising callback reports
  once, and a resurrected ND stays closed.
* Slicing/dimension reduction/private suboffsets and outward views: the callback
  sees original shape/strides/data, outward views close before upstream release,
  live outward leases preserve data/arrays, and multiple exports balance.
* Consumer/producer one-shot rejection with and without outward views preserves
  old data; failed first acquisition can retry; nested init and reads during
  acquisition reject without publishing partial state.

Propose a copied negative omitting only ndarray_tobytes' active gate. The cycle
test must fail its named "released ndarray remained readable" assertion. A
global bytearray keeps backing valid while the test observes that read; crash,
build/import/helper errors are not causal credit. No raw-address fixture is used.

## Synthetic unready-retention oracle

After all persistent consumers are ported, a normal dead leaf owner is queued
and resurrects its acquisition closure before unready retention. A rooted live
holder instead marks that closure live initially. Replacing the old intentionally
unported holder with another normal consumer would therefore stop exercising the
same failure boundary.

The retained retention/OOM test now uses a narrow _testinternalcapi hook on an
exact active memoryview and bool: only0→1 synthetic outstanding export and1→0
restoration are permitted. It is not a real acquisition/root and does not prove
production graph validity. The test creates exporter→view cycle, blocks that
view, and drops all strong roots before collection. It still requires fired
work-stack OOM, zero swept count on failure, a retained exporter/usable view,
zero delivery while blocked, and eventual exactly-once ordinary release.

Restoration is in finally, including assertion failure, using the retained
exporter's noncallback weak reference to recover the view; memoryview hook
weakrefs may legitimately have been cleared. The actual view is explicitly
released only after restoration. A separate hook contract test rejects wrong
types, non-bool requests, duplicate block, zero-count unblock and released views.
Unblock validates only count==1: it cannot distinguish the synthetic blocker from
one real export. The isolated caller must establish that no real export exists
throughout this interval. No arbitrary setcount, raw address or persistent holder
type is introduced.

Host AST/child compilation and diff checks will be recorded in ignored
build/ndarray-buffer-source/source-v1. Native validation remains held for focused
source review. Prefer one coherent O1 debug-kill campaign containing new four,
the migrated retention oracle, and affected whole GC/buffer/memoryview modules;
root will bind the actual union and preserve component receipts. No full ladder,
O3/ASan cohort, shutdown/fork policy or generic cleanup protocol is authorized by
this source checkpoint. Existing forced-zero/swept-exporter safeguards remain.
