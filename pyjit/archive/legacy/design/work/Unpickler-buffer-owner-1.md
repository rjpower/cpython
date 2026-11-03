# Unpickler input acquisition owner

Source checkpoint only, based on `87b6d9c3ccbe790770dbba2a2435cd0ea83c5683`
in managed branch `boil/unpickler-buffer-owner`. No build or target execution.
The base's memoryview/managed-buffer readiness policy remains authoritative;
this port does not claim to repair every native buffer consumer.

## Ownership and interfaces

`Include/internal/pycore_memoryobject.h` exports the existing
`_PyManagedBuffer_FromObject(base, flags)` and the existing release operation,
renamed consistently to `_PyManagedBuffer_Release(owner)`. The only new
operation is `_PyManagedBuffer_GetBuffer(owner)`: a const pointer to the stable
embedded master, or NULL if its authoritative RELEASED bit is set. Its result
does not depend on `master.obj` or `buf` being non-NULL. The exact managed type
already supplies traversal, GC transport, idempotent inert-before-callback
release, and stable acquisition storage. No new type, lifecycle state,
Python-visible release operation, or type slot is introduced.

Unpickler replaces its embedded `Py_buffer` with a typed, traversed owner.
The owner acquires directly into its stable master using the unchanged
`PyBUF_CONTIG_RO` flags. There is no copied shape/stride/internal pointer to
consumer-local storage. The exporter and any Python-buffer forwarding wrapper
remain reachable through the owner. Ordinary Python may discover the owner
through `gc.get_referents`, but cannot release it or export a view from it.
An input memoryview retains its normal active-export protection.

`_Unpickler_SetStringInput` prepares the new owner before touching the old
acquisition. Failure retains the old acquisition; this does not rewind a
file object's already completed read. Success publishes the new owner,
pointer, length and cursor/frame fields before consuming the old owner.
`_Unpickler_DetachInput` zeros every cached input/cursor/frame field before
release. All explicit release and replacement occurs under the existing
running guard. `load`, `__init__`, memo clear and memo replacement reject
reentrant mutation; callbacks cannot clobber the new acquisition.

Explicit live reinitialization and functional load/loads completion consume
the acquisition while guarded. `tp_clear` and deallocation only invalidate
the input and drop the owner reference: they do not run an owner's queued
release from sweep. Existing managed-owner transport performs automatic
cleanup independently of an Unpickler subclass's overridable `__del__`.
A resurrected subclass can reinitialize with a fresh owner; the old owner
contains only its original master and cannot consume the new acquisition.

Initialization clears partial state on error. Clearing also resets memo and
mark counts/capacities with their pointers, so a failed/repeated init cannot
leave a later load using a NULL mark array with an old capacity. Functional
load/loads now enter the same running guard and clear their temporary reader
on success and setup/load failure. The returned value remains a live C local
under the existing opaque `_pickle` frame contract during release/clear.
No new precise annotation, arbitrary RAW-tracer claim, or refcount-based
automatic-finalization assumption is added.

## Raw input access audit

| Route in `Modules/_pickle.c` | Lifetime argument |
| --- | --- |
| Public `Unpickler.load`, functional `load` and `loads` → `load()` | All three enter the running guard; `load()` rejects a consumed owner before opcode dispatch. |
| `loads` initial input, `_Unpickler_ReadFromFile` read/readline/peek results | All acquisitions converge on transactional `_Unpickler_SetStringInput`; no other cached-input producer exists. |
| `_Unpickler_Read` fast path and `_Unpickler_ReadImpl` slow/frame path | Borrow only from the current sealed owner. `self` traverses it throughout the guarded operation; reentry cannot replace or clear it. Frame transitions change bounds, not ownership. |
| `_Unpickler_Readline` and `_Unpickler_CopyLine` | Scan current owner bytes, then copy into the reader-owned line array. The owner remains live across the fallible `PyMem_Realloc`; line handlers consume the copied line. |
| `_Unpickler_ReadInto` | Copies cached bytes before advancing/reading another chunk. The owner remains attached across frame transitions and `_Unpickler_SkipConsumed` callbacks. |
| `_Unpickler_ReadIntoFromFile` | Separate destination path: fallback requires bytes and copies while the local result is live; readinto supplies the existing destination memoryview. It does not install or borrow `input_buffer`. Retention of that callback view by user code is an existing separate API issue, not repaired here. |
| `_Unpickler_SkipConsumed`, opcode callbacks, persistent loaders, memo insertion | Keep the current owner attached across Python/collecting calls. No callback can invoke guarded input mutation. Integer/frame headers are decoded before a subsequent read replaces their source. |
| Clear, reinit, replacement and error exits | Detach invalidates raw fields before any explicit release; GC clear only drops ownership. No later raw access is permitted from the detached acquisition. |

## Focused validation proposed

Seven new methods in existing `test_pickle.CUnpicklerBufferOwnerTests` cover
the exact flags and collecting callbacks for loads/read/peek/readline; sealed
owner traversal and native export lifetime; replacement ordering plus load,
init and memo reentry rejection; failed replacement retaining its acquisition;
raising release preserving an existing load error; partial-constructor cleanup
and mark-stack reuse; and subclass finalizer resurrection/reacquisition with
exactly-once supported cleanup. `SizeofTests.test_unpickler` reflects the
embedded-buffer-to-pointer layout change, without claiming measured size/RSS.

Host checks only: parse the whole test module and count these seven methods;
`git diff --check`; inspect all managed-buffer release callsites and all
cached-input assignments. No native result is claimed. The old release-before-
acquire implementation should fail the acquisition-retention assertion without
invalid-buffer access; it is a possible bounded negative using the same test,
not an executed control.

After source review, the smallest native feedback is these seven methods plus
the existing Unpickler concurrency and sizeof controls on the next coherent
O1/Py_DEBUG candidate. Relevant ordinary pickle coverage and eventual coherent
release/ASan/full qualification remain required. No new build/provenance
framework or component-specific broad campaign is proposed.
