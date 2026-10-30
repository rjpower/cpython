# Same-interpreter memoryview sharing

Source-only checkpoint from clean composed base
`dc7bd5c19f7563c04318edf0d1698352567f066f`, managed branch
`boil/buffer-xid-main-only`. No native execution or build. The separate NDArray
port and eventual buffer safeguard removal are not part of this patch.

## Cut and ownership

Same-interpreter queues/channels remain supported even though new interpreter
admission is rejected. Their memoryview XID factory previously created another
native holder, CrossInterpreterBufferView. Its ordinary finalizer could consume
the original acquisition after another queued user finalizer resurrected the
receiving view. That receiving view had cached raw input without an active-owner
gate. A new generic lifecycle protocol is unnecessary for the main-only profile.

`Modules/_interpretersmodule.c` now rejects a foreign XID owner before touching
the shared object, then returns `PyMemoryView_FromObject(shared->view.obj)` for
the current owner. `memory_getbuf` acquired the original exact memoryview with
FULL_RO; its temporary export prevents release during construction. FromObject
on a memoryview registers a new view with the existing managed buffer and copies
the particular view's metadata. The received view retains format, shape,
strides, offset, readonly state and mutable backing without copying bytes or
depending on the lifetime of the sender's view object.

XID continues to own its raw xibuffer acquisition and explicit PyGC root until
disposal. There is no used flag or ownership transfer. On successful receive,
the new memoryview is already rooted by the caller and has its own managed
registration before XID disposal releases the temporary original export.
Failure leaves the same disposal owner. Queue and channel receive call the
factory then dispose the XID on both success and failure; clearing/discarding an
unreceived item also invokes that disposal. XID clear nulls its data pointer
after its free callback, preventing repeated disposal of that storage.

The free callback removes the root, releases the original stable Py_buffer and
raw-frees the holder. The removal and release have no intervening Python call;
the release's C local anchors the exporter across its callback under the current
opaque-frame contract. In the same-owner path the exporter is a memoryview,
whose bf_releasebuffer only decrements its external export count. No root or
release is delegated to the received object.

The original and received view may each release independently; another live
registration keeps the shared master active. Resurrecting the received view
keeps that ordinary managed-buffer graph reachable. There is no separate native
wrapper finalizer that can invalidate its raw input.

The private `_interpreters.CrossInterpreterBufferView` export, all its helpers,
module lookup/type state and now-empty module traversal/clear/free plumbing are
intentionally removed. Module state size becomes zero. No repository test or
API-document reference depended on this private type. Foreign buffer restoration
now raises RuntimeError explicitly; there is no legacy foreign fallback,
subprocess shim or new subinterpreter support claim.

## Tests and proposed native feedback

Four new methods in each existing queue/channel module cover:

* noncontiguous view metadata and readonly/writable variants, with sender-first
  and receiver-first release and mutation of shared backing;
* receiving-view allocation failure through existing
  `_testcapi.call_with_nomemory`, followed by an empty receive queue and balanced
  original acquisition disposal;
* a user-finalizer cycle that resurrects the received view, continued reads
  across collection, and exactly-once Python release after explicit view close;
* explicit discard/destruction of an unreceived buffer and eventual native
  backing resize, proving its temporary acquisition was released.

The first source draft79ada incorrectly assumed the channel's first receive
was allocation-free before pop. It was not executed and is preserved under
`build/buffer-xid-main-only/reviewed-v1`. The V2 tests perform an expected empty
receive before enqueueing or arming a fault: this initializes the queue parser
and associates the channel's receiving end. A subsequent association finds that
existing open end without allocating (`_channelends_associate`).

The fault call uses the low-level C receive method with prebuilt arguments and
automatic collection disabled. Its queue ID is a plain int subclass prepared
before arming. The exact-int PyValue canonicalizer would otherwise inline it,
and the generated queue wrapper's PyValue_Object conversion could allocate a
box before pop for a non-small ID. The subclass stays a heap object;
qidarg_converter calls PyLong_AsLongLongAndOverflow, whose PyLong_Check fast
path reads subclass digits directly without __index__ or allocation. ChannelID
is already a native heap object; its converter reads cid directly. Neither
proof depends on the process-wide queue/channel ID still being small.

After those warmups/conversions, lookup/pop allocate nothing; the receiving
memoryview allocation is the intended first failure. call_with_nomemory builds
its argument tuple before arming and restores hooks before propagating
MemoryError. No hook/helper/C API changes or guessed allocation ordinal are
needed. Actual receipts must still prove that the failed receive consumed the
item: an unrelated earlier allocation failure would leave it queued and fail
the unchanged external empty/backing-balance oracle.

Channel send_buffer creates a temporary sender view. Tests collect after its
root is disposed before claiming that the native backing can resize. The
release callback and the returned memoryview's own registration are separate
resources; one Python callback does not prove that an as-yet-uncollected view
has already unregistered its native master.

Host checks only: parse both complete test modules, count the exact eight new
methods, inspect all former wrapper/state references, and check the diff.
No native assertion, MemoryError path or lifetime outcome is credited yet.
After source review, use the next coherent O1/Py_DEBUG candidate for the eight
new cases and existing same-interpreter send-buffer/outlives-sender controls,
then relevant whole-module qualification under the existing recipe. Foreign
rejection is an explicit defensive boundary; this test plan does not create an
unsupported interpreter merely to reach it. No standalone configure/full-suite
campaign is proposed for this source cut.
