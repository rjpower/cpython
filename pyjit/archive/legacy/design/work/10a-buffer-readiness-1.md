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

## First native feedback and finite source correction

Original d6bf31251dd O1 build/cleanup passed, including116module checks. New5
actual-kill/N0 tests passed with129collections/0fallback/0unpinned. Whole4 then
failed:479tests/5failures/23skips. Three memoryview GC cases retained a cycle;
the ordinary Python buffer-release and owner-routing cases missed release.
Raw failure9d09d4b0 and post-gate verification471d383e remain unchanged. No
negative build/run or native retry followed. Whole-run unpinned3252 is report-only.

Two source defects were exposed. A ready native view had no DEAD_HOOK, so its
unready managed owner could retain a path back to that view before it unregistered.
Also, releasing an outer wrapper could make its returned view ready, but memory
DESTROY consumption did not trigger the existing explicit-collection second pass.

The source successor discovers every active view registration and managed master
acquisition, including NULL-release exporters. The private predicate is renamed
to _PyMemoryView_HasAcquisition; the unreferenced Python-only callback classifier
is removed. Actual consumption contributes to the existing completion count
after error reporting. Already consumed and still unready nodes contribute zero.
The existing maximum of two explicit passes is unchanged: this is not a universal
chain-depth guarantee or a new pending-wakeup protocol. More native-only views
now require queued unregister work; performance remains unmeasured.

Struct and test source bytes are unchanged from d6bf. No assertion or gc.collect
loop was altered. Successor qualification is pending source review and a fresh
make-only build in the owned configured O1 tree; old native receipts keep their
original source identities and cannot qualify this C correction.
