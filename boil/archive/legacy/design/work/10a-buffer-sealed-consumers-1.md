# Sealed PickleBuffer and scanner release transactions

Source-only candidate from87b6d9c3ccbe790770dbba2a2435cd0ea83c5683 in
the managed boil-buffer-sealed-consumers worktree. No build or target execution
qualifies this candidate yet. The prior struct/readiness positive and negative
trees and their original failures remain frozen.

## Ownership and release

PickleBuffer and SRE scanner are sealed types without reinitialization. Ordinary
tp_finalize can therefore close their single embedded acquisition without a
subclass replacing the native finalizer or a later acquisition reusing its
once-only notification. No new owner object, type capability or queue kind is
needed. The existing finalizer transport roots the object and reachable closure
through callbacks. These are GIL-profile changes, not a free-threading proof.

PickleBuffer routes C release, Python release, clear, finalization and dealloc
through one guarded transaction. During release, C GetBuffer, raw and bf_getbuffer
report the existing released-buffer error; recursive release or explicit __del__
does nothing. The original embedded Py_buffer, including obj and all metadata,
stays intact until PyBuffer_Release returns. Both constructors initialize the
guard before acquisition. Outward bf_getbuffer requests retain their existing
independent upstream acquisition behavior. A borrowed C GetBuffer result gains
no lifetime beyond explicit release.

Scanner finalization sets state.start=NULL before releasing the original buffer.
Both search and prefixmatch (also exposed as match) already test that state
before touching raw input. A separate releasing guard makes explicit __del__
reentry harmless. Explicit finalization while executing is a no-op: invalidating
the input there would break the current C scan. A running scanner is rooted by
its active call, and its traverse visits pattern, string, buffer exporter and
debug exception; ordinary GC therefore does not select that object for death.
The existing debug interruption hook creates a Python exception while executing
inside the matching loop, providing a deterministic test of this boundary.

After release, buffer.buf=NULL prevents state_fini from releasing twice. Scratch
marks, data stack, repeat pools and string ownership remain with the single
deallocation-time state_fini. Dealloc first invokes the same scanner finalizer;
clear still only clears pattern. Constructor failure retains the existing
state_init error cleanup, with zeroed state and explicitly initialized guards;
later finalization/deallocation sees a consumed buffer. No generic SRE_STATE
stack-use or error-path cleanup is redesigned.

Neither acquisition is detached into a short-lived copy: exporter callbacks see
the original view and its original metadata. The Python buffer slot already
preserves incoming exceptions and reports callback errors as unraisable. Ordinary
C bf_releasebuffer contracts remain applicable; no arbitrary C callback is
classified as a leaf. Swept-exporter safeguards remain unchanged for other users.

## Meaningful checks proposed

Five new BufferLifetimeTests use the existing child mode/stress assertions:

* PickleBuffer exporter cycle: raw/getbuf reject reentry, recursive release and
  __del__ are inert, original callback bytes survive nested collection, once-only.
* Explicit PickleBuffer release with reentry and a raising callback: unraisable
  error exactly once and later finalization does not release again.
* Scanner exporter cycle: search/prefixmatch/match return None before callback
  code, recursive __del__, original metadata, nested collection and raising error.
* Scanner constructor fails after acquiring a bytes buffer for a string pattern:
  original TypeError survives callback failure, acquisition consumed only once.
* Debug-only scanner interruption: explicit __del__ while executing leaves the
  acquisition active, reentrant search still raises, the scan remains usable;
  later idle finalization closes it once.

The existing test_re scanner retention test now supplies __release_buffer__ and
requires exactly one callback when the scanner is collected. Existing
test_picklebuffer covers released/failed constructors, raw/GetBuffer-facing
buffer operations, independent outward views, arrays and explicit idempotency.
Propose new five first in actual O1/Py_DEBUG kill/N0, then whole test_gc,
test_re, test_picklebuffer and test_pickle once with the existing disclosed
exclusions. Root will choose whether to use an isolated cohort or coherent union;
no new full ladder, O3 or ASan cohort is presumed by this source plan.

A copied causal negative removes only scanner_finalize's pre-release
state.start=NULL. Run only test_scanner_cycle_release and require its named
"release must observe an exhausted scanner" assertion. Its global bytearray
keeps raw backing valid, so the control observes a live scan rather than trying
to manufacture stale storage. Crash/import/build/helper failures are not credit.

## Retention oracle migration and remaining boundary

The existing unready-memoryview retention/OOM test intentionally used an unported
PickleBuffer holder. That holder is now ported, so the same liveness, zero-delivery,
fired-allocation-hook/no-sweep rollback and eventual once-only release assertions
use the remaining native ndarray consumer instead. This fixture acquires from a
memoryview with no transformed/owned arrays and never exports the ndarray. It
reads one element to verify retained data, then explicitly removes the exporter's
reverse holder edge, allowing that native consumer to release its memoryview
registration. The ndarray port must revisit this fixture honestly; it is not
evidence for ndarray metadata, reinitialization or outward-export safety.

Unpickler, ndarray, external consumers, shutdown/fork and queue completion remain
separate work. This slice neither removes swept-exporter safeguards nor claims
universal progress for arbitrary acquisition-chain depth. It does not activate
generic cleanup slots or a completion graph.
