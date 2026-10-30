# Finite struct and memoryview acquisition readiness

Source checkpoint from safety9e223040a2070c75f88be10366e0f0a7b35f6ed3.
This changes one sealed consumer, not the remaining four buffer families.

Struct unpack iterators use their existing stable embedded Py_buffer. One guarded
release transaction makes the iterator inert before release and serves ordinary
finalization, exhaustion, deallocation and invalid-length construction cleanup.
No extra acquisition owner or public type capability is introduced.

Memoryviews unregister only with zero outward exports; managed buffers release
only with zero registrations. The collector selects all ready hooks first, then
resurrects the full closure of every still-dead unready view/managed buffer,
including native-only acquisitions. Checked worklist failure enters the existing
no-sweep rollback. Delivery rechecks readiness and sets memory-specific NOTIFIED
only after actual consumption; a skipped acquisition remains eligible.

The other persistent consumers and external ports remain migration work. An
unported cyclic holder can remain retained until explicit release. General
memoryview deallocation's forced-zero path and PyBuffer_Release's swept-exporter
safeguard remain; this slice removes forced-zero only from queued release.
No scheduler, shutdown/fork, owner-routing, type layout or root policy changes.

Five new BufferLifetimeTests cover cycle/reentry, returned-view forwarding with
another holder, a new export acquired between selection and delivery, raising
release/invalid constructor/exhaustion, and native-only waiter retention with
existing work-stack OOM recovery. Host parsing/child-script compilation and
`git diff --check` pass. No native result is claimed by this source checkpoint.

Root cleared source64bc25cd. First native feedback is one fresh GCC O1/Py_DEBUG
build through the existing configure/make/audit/module-check recipe, then the
new five tests once in actual kill/N0. If they pass, run test_struct,
test_memoryview, test_buffer and test_gc once in debug kill, with unchanged known
exclusions disclosed. The copied inert-transition negative removes only the
pre-release Py_CLEAR(so) and expects the named exhausted-iterator assertion.
O3 and focused ASan are deferred to the coherent GC union. Stop on failure.

Frozen source/negative and the fuller proof/limitations are retained in ignored
build/buffer-readiness/source-v1. Qualification and integration remain pending.
