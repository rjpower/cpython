# Type foundations, cache and mutation admission

Root owns composition and admission. Sol 6.1 reviews finite held-source
packets by cause; see the [board](../../README.md) for live assignments.

## Current state

The selected runtime uses canonical PyValues and reference counting with cyclic
collection. Type-cache entries, mutation, installed guards and interpreter
teardown must share a coherent invalidation/admission contract. The tracing-GC
work is closed.

Scalar closure checkpoint51 is `14fc009d42f`. Lookup/call50 and five scalar
boundaries51 are committed after native/source review; reference outcomes and
mandatory candidate error contracts are recorded [here](../core-consumers/type-scalar/status.md).
The earlier type attribute bridge is checkpoint48 (`dd26d8e34ca`). Its type attribute bridge and
optional-attribute caller pass both full-TU/object probes; four host controls
pass and candidate execution remains pending. After the three pickle/state readers
landed in46, the complete current type translation unit and actual object compile
without errors or warnings. Eight host controls pass. This establishes compilation,
not semantic admission of the remaining held file or candidate execution.

The original typeobject provider/core/graph/MRO changes remain distinguishable
from committed reader ports. Their old source reviews do not automatically
cover every foundational body in the current composition. The held typecache
test also needs to travel with the appropriate admitted component.

## Current admission queue

The fifteen scalar boundaries are committed. A complete-definition audit now
accepts the exact held 67-definition delta for a dependent source projection:
55 prior endpoint definitions and nine foundations match completely; the
remaining MRO/counter/deletion differences are explicitly reviewed. It also
accounts for signatures, declarations, includes and nonfunction code. The
independent inventory corrects an omitted unchanged same-line definition:
423→436 definitions, with the 67-definition delta unchanged.

- [Held admission review](../../../../build/pyvalue-type-held-admission-review-20261103-v1/README.md)
- [Mechanical inventory](../../../../build/pyvalue-type-held-extraction-inventory-20261103-v1/README.md)

Root still must validate the exact source/test projection and checkpoint it.
The audit found a concrete separate defect: obsolete borrowed _PyType_Lookup
can return a scalar box after releasing it. Sol designs removal and migration
of its actual callers/declaration to owning or canonical results. Hard
compatibility does not preserve that API. Guard/feedback wiring must also close
before runtime admission; terminal lifecycle behavior needs candidate qualification.

Ignored has_custom_mro statuses already occur upstream; version zero can be a
successful uncached result. The audit found no new unchecked scalar boxing in
the eight MRO envelopes and does not invent failure witnesses from invalid C
deallocators. Snapshot pins and displaced field owners remain separate owners.
No new graph journal, diamond or stronger rollback policy is a baseline gate.

After source and wiring closure, execute cache/mutation/scalar/reentry/teardown
and fault controls on the linked candidate before measurement. Recording-only
counters do not establish transaction authority.

## Evidence and history

- [Canonical provider source/native record](../../../../build/pyvalue-typecache-provider-20261102-v1/coordinator-code-review.md)
- [Core mutation component](core-mutation/status.md)
- [Graph source/native record](../../../../build/pyvalue-typecache-graph1-20261102-v1/coordinator-code-review.md)
- [MRO provider endpoint](mro-providers/status.md)
- [Pickle/state final composition](../../../../build/pyvalue-type-pickle-readers-composed-20261102-v2/independent-composed-review.md)
- [Nine-foundation actual review](../../../../build/pyvalue-type-canonical-foundations-peer-20261102-v1/independent-solshared-review.md)
- [Type attribute bridge](../core-consumers/type-getattro/status.md)
- [Accepted lookup/call design](../../../../build/pyvalue-type-lookup-call-envelope-design-20261102-v1/README.md)
- [Latest merged build](../merged-build/status.md)

The recording-only counter foundation (`5a9d4841a67`) is committed; counter
recording alone is not transaction authority. The separate full publication
proposal has no implementation grant. Detailed prior proposals, native failures,
reviews and source endpoints stay with their experiments. The former page is
preserved byte-for-byte in [prior notes](history/notes-through-checkpoint45-20261102.txt);
its dated compiler counts and assignments are historical.
