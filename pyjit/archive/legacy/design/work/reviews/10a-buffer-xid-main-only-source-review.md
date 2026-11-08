# Same-owner memoryview XID source review

Independent review of frozen diff79ada83d7c95e40b4320b88aa3d3859f9e72f50efabc8e6467a86e574d681181,
checkpointca5507973e3cb92f42b1a47f713922649636d2c8baa90f6d2192dc0035626b0f,
in boil-buffer-xid-main-only againstdc7bd5c19f7563c04318edf0d1698352567f066f.
All four source hashes match. Both test modules parse on the host. No target,
compiler, native test or owner-file edit was performed by this reviewer.

**Runtime source clear for the bounded main/GIL slice; test fix first.** The
channel allocation-failure fixture needs the already identified receive-end
association preparation below before this complete packet is launch-ready.
No other source blocker found. Native outcomes remain unqualified.

## Required fixture correction

`test_channels.py:test_buffer_receive_failure_disposes_acquisition` currently
arms call_with_nomemory(0,1) before this channel's first receive. Actual
`_channel_next` at1177 first calls `_channelends_associate` at1187, before
`_channelqueue_get` at1192. A previously unassociated receive end allocates
`_channelend` at859 through `_channelends_add`. The injected failure can therefore
precede popping the XID and creating the receiving memoryview.

Prime the same receive-end association outside fault injection, for example an
ordinary empty receive whose expected ChannelEmptyError establishes that path.
Then retain the MemoryError assertion, item-consumed/empty-channel oracle and
balanced-acquisition/backing-resize check. The empty oracle correctly prevents
an earlier unrelated allocation failure from receiving factory-failure credit.
The queue path has no analogous channel-end association. This finding concerns
the intended fault location, not the proposed runtime ownership change.

## Ownership and failure proof

* `_pybuffer_shared` at127 still owns the sole raw xibuffer. Exact memoryview
  getbuf returns view.obj equal to that view and increments its external-export
  count. `PyGC_AddRoot` roots that stable field until disposal. GetBuffer/root
  failures release whatever was acquired and free raw storage; Init/free-callback
  publication adds no allocating step. Incoming source arguments remain anchored
  under the existing opaque extension-frame contract.
* `_memoryview_from_xid` at99 rejects a foreign owner before dereferencing the
  shared object. Its same-owner temporary export prevents original-view release
  during construction. `PyMemoryView_FromObjectAndFlags` atmemoryobject.c820
  takes the exact-memoryview branch; `mbuf_add_view` at695 copies the selected
  view's metadata into the receiving view's own arrays and registers it with
  the existing managed buffer. Allocation failure publishes no registration.
* On success the received view owns a managed registration independent of the
  original view's lifetime. It retains offsets, strides, readonly state and
  shared backing rather than copying bytes. A resurrected received view keeps
  this ordinary managed graph live; no separate wrapper finalizer can close it.
* Queue get1199 and channel recv1944 construct the result, then dispose the XID;
  both error branches also dispose it. Their result locals retain the object
  under the existing opaque-frame contract before publishing it. Disposal runs
  after queue/channel locks are released. Normal discard/destruction routes
  through the same XID release path.
* `_pybuffer_shared_free` at119 now always removes the explicit root, releases
  the original Py_buffer and frees xibuffer. That release is the exact original
  memoryview's native external-counter decrement, not a user-exporter callback.
  `_xidata_clear` atcrossinterp.c339 clears its data pointer after this callback;
  no ownership transfers to the receiving view. Repeated normal XID disposal
  consequently does not release/finalize this acquisition twice.

The private allocator/non-detach and opaque-root contracts are inherited. This
review does not establish arbitrary replacement allocator callbacks or foreign
interpreter teardown safety. Foreign reconstruction is explicitly rejected;
the first milestone admits no new subinterpreters.

## Removal and meaningful coverage

The deleted private type was the only remaining consumer of the removed module
state, module lookup and module traverse/clear/free callbacks. No source/test/API
documentation reference to CrossInterpreterBufferView remains. State size zero
fits the resulting stateless multiphase module; builtin memoryview XID class
registration retains its existing persistent registration policy. The private
type removal and foreign-restoration rejection are intentional compatibility
changes, not fallback paths.

The eight new queue/channel tests address metadata and both release orders,
receiving-view allocation failure, resurrected receiving views and unreceived
discard. All are same-owner operations; base setup/teardown does not require
creating an interpreter. Explicit sender/receiver releases balance queue leases;
channel send_buffer additionally creates a temporary view and its tests collect
after root disposal before requiring native backing resize. Python exporter
notification and reclamation of a returned view's independent native acquisition
remain distinct. Source-only tests do not yet establish their native outcomes.

No general buffer-safeguard removal, NDArray implementation, completion DAG,
shutdown/fork lifecycle or free-threading clearance follows from this review.

## Round 2: narrow fault-fixture successor

**SOURCE_CLEAR for bounded native validation**, subject to coordinator grant.
Reviewed V2deltae822dbfa21bf083549f4d2dc923ef04b53a86f6f88f79f3bc15e36cdeea899f8,
fullsource420e2948887f95e868642875c98af1b6529f9cdd948ef6664fb3a3d3f9a6e074,
checkpoint67577aaa645fc50714c4bd8bf729f74b4ddf80411d6bb362ef1591e8cd5d1eed.
All four current source hashes match; runtime C is byte-identical to V1, which
remains preserved under reviewed-v1. Host AST parsing and diff check pass.
No native action or owner-file edit by this reviewer.

Both fault tests now perform an expected empty receive before enqueueing or
arming. The channel receives its persistent open association, so its next
_channelends_associate takes the existing-node path rather than allocating
before pop. The queue's generated parser is also warmed normally.

The queue additionally uses a preconstructed plain HeapQueueID(int) subtype.
Generated `_interpqueues_get` atclinic/_interpqueuesmodule.c.h341 calls
PyValue_Object before its converter; an exact int can require boxing there.
Include/pyvalue.h208 canonicalizes exact ints only, leaving this subtype as a
heap-object value. idarg_int64_converter at_interpqueuesmodule.c103 calls
PyLong_AsLongLongAndOverflow; its PyLong_Check branch atlongobject.c1709 reads
subclass digits without invoking __index__ or allocating. Channel's prebuilt
native ChannelID similarly takes channel_id_converter's direct field read.
Thus neither intended fault relies on a conveniently small process-global ID.

call_with_nomemory prepares its argument tuple before installing allocator
hooks. After these preparations, the source path reaches receiving-view
allocation after consuming the item. Actual execution must still prove that
placement through the unchanged empty-item and balanced-release assertions;
this source clearance gives no MemoryError/native coverage credit by itself.
