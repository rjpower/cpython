# Collector policy and C-root registration costs

2026-11-12 source audit. GIL/default collection and tracing verification have
different consumers; M4 removes conservative arena-value enumeration, not all
native conservative roots. No native instruction/store measurement is claimed
by this ledger.

| Consumer | What correctness needs now | Role of explicit C roots |
| --- | --- | --- |
| `Python/gc.c:subtract_refs`, `deduce_unreachable` | Start from actual object refcounts, subtract actual counted container edges once; transferred owners disappear from the old heap/frame edge set. | A counted C owner already leaves positive external `gc_refs`. Registering its address does not change RC correctness; visiting it as a container edge would be wrong. |
| `Python/gc_trace.c:_PyGC_TraceAfterCollect` | Post-collection tracing verifier finds objects retained by RC but missed by root/traverse coverage. It frees nothing; release defaults OFF, debug defaults explicit collections. | Precise records establish concrete bridge coverage and prepare arena scanner removal. They are not a separate GIL memory reclamation mechanism. |
| `scan_thread_struct` arena ranges | Current/older arena values and headers retain unlinked remaining owners in the verifier. Chunks use `_PyObject_VirtualAlloc` (`Python/pystate.c:allocate_chunk`), rather than ordinary PyMem object allocation. | M4 needs mapped active roots and retiring reservations/records before removing these ranges; native-stack pointer discovery does not prove the same complete arena coverage. |
| Native C stack/register/PyMem traversal | Separate retained conservative policy in `gc_trace.c`, including pointed-to PyMem blocks. | M4 need not register every native scalar/range merely to remove arena scans. Keep this policy explicit and avoid claiming a complete precise native migration. |
| FT `gc_visit_thread_stacks` | Deferred owners missing from physical refcounts require adjustment; current mortal PyValue owners and typed pins are already counted. | Do not add deferred counts for counted CValue/CObject roots. Existing `next`/record mismatch is a source consistency issue, not proof these counted owners need another GC count. |
| Optional FT `gc_visit_thread_stacks_mark_alive` | Mark from the roots selected by its optional marking policy. | If a new chain is part of that policy, enumerate it and use FT synchronization rules. GIL evidence does not qualify FT. |

The [C-owner census](c-owner-census.md) is required ownership accounting, not a
mandate for blanket always-on registration. Correct refcounts, no stale home
interpretation, unique container edges and preserved callback lifetimes remain
GIL correctness requirements even when native conservative marking is retained.
An invalid/double-subtracted moved edge is not repaired by adding a C head.

Static added-work estimate for turning the existing CValue API into an always
linked chain:

- Record grows from one eight-byte value to value plus link: 8 to 16 bytes on
  the measured 64-bit ABI; maximum existing three-record scope adds 24 bytes.
- Push gains next/head stores beyond the existing NULL initialization; pop/steal
  gains head unlink and field NULL stores: four added source stores per record.
  There are 16 records across ten existing scopes, with 13 normal/3 steal pops.
- One unconditional CValue head adds a pointer to thread state. A separate typed
  CObject head adds another pointer and each reached object record needs its own
  link/storage. Registration itself need not INCREF.
- Inlining, compiler elimination, stack layout, spills, call ABI and dynamic
  frequency still require source/native/count evidence. These are not measured
  instruction counts or an elapsed prediction.

Prefer bounded real outlined adapter consumers for new precise registration;
price their reached costs. A full C API native-precision migration should not
silently become a prerequisite for the GIL arena switch. Numeric-box workspaces
remain live leaf roots under the explicit native conservative policy; no outgoing
GC edges does not mean they are not roots, or make them a general owner bank.

Coordinator-approved narrow cleanup is implemented over `d2c5233fdf6`: comments
describe CValue's counted local workspace, and the unused FT-only `c_stack_refs`
field plus its impossible `next` loop are removed. No producer ever linked that
head; current CValue values already have mortal counts. No new head, store,
record-size change, GIL behavior or sixteen-scope registration port is introduced.
Frames source peer GO; diff check and source-reference census pass. Root owns
integration/compile and any FT qualification; this source correction alone does
not establish FT support.

## Bitmap writer-validation snapshot

2026-11-13. Root froze a separate GIL counted-root mode in the bitmap rival.
`gc_trace.c` scans each physical chunk bitmap once, including staged and
unlinked reservations, then visits typed current/transition headers and C
results explicitly. THREAD transition homes and bitmap-described arena prefixes
do not repeat that physical scan. Active heap roots use their own bit interval.
Only a set bit authorizes an object read; zero never establishes an inline,
control or language value. All writer ports remain an admission gate.

The private counted transition entry is separate from the existing logical
entry used for root IDs and FT. Generic container traversal/referents keep their
no-count object semantics. Counted verifier roots rely on its existing global
immortal-root pass. FT deferred-reference and tag scans remain unchanged.
Native prefix storage has no bitmap descriptor and keeps its declared readable
prefix policy; frame-backed unpack prefixes carry their actual bitmap word.
Cold address membership checks distinguish arena prefixes without reading a
possibly redirected EXTERNAL record's frame geometry. The prefix grows from
24 to 32 bytes; the 40-byte transition record does not grow.

This source snapshot retains conservative arena scans and the explicit old
NULL-shadow phase check/store while scratch is still initialized. Before stale
scratch is admitted, that diagnostic must use a bitmap-compatible comparison
with readable phase authority or truthfully return uncovered. It must not read
inactive zero-bit values or fabricate a checked result. No phase tests are
disabled, and this intermediate root mode is not a full bitmap comparison.
