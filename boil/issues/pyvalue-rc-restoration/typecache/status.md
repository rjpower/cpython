# Type foundations, cache and mutation admission

Root owns composition and admission. Sol 6.1 is reviewing the remaining held
source by cause; see the [board](../../README.md) for live assignments.

## Current state

The selected runtime uses canonical PyValues and reference counting with cyclic
collection. Type-cache entries, mutation, installed guards and interpreter
teardown must share a coherent invalidation/admission contract. The tracing-GC
work is closed.

Restoration checkpoint47 is `4f2734e77cd`. After the three pickle/state readers
landed in46, the complete current type translation unit and actual object compile
without errors or warnings. Eight host controls pass. This establishes compilation,
not semantic admission of the remaining held file or candidate execution.

The original typeobject provider/core/graph/MRO changes remain distinguishable
from committed reader ports. Their old source reviews do not automatically
cover every foundational body in the current composition. The held typecache
test also needs to travel with the appropriate admitted component.

## Required review and fixes

The current causal review has found concrete scalar-domain problems in held
method/slot/type-getattr paths: a heap-only decoder cannot materialize an inline
integer or float. Fallible owning conversion and its cleanup must be explicit.
The MRO error envelope also needs audit under the changed provider contract.
Ignored has_custom_mro statuses already occur upstream; that fact must not be
misreported as an entirely new bug. Concrete changed-provider failures require
finite fixes and caller-envelope review even though the file compiles.

Guard, terminal lifecycle and selected namespace-writer wiring remain distinct
checks. Sol is binding the exact current bodies to earlier reviewed endpoints,
classifying introduced defects against inherited upstream behavior and defining
small dependency-ordered fixes or extractions. Its [sealed causal matrix](../../../../build/pyvalue-typecache-causal-triage-20261102-v1/README.md)
is in independent peer review. No whole-file source grant follows
from a successful compiler run or an old provider review.

The earlier full transaction proposal requested stronger rollback, diamond and
cross-receiver guarantees. Its review constraints describe that proposal. They
are not automatically requirements to replace upstream's pointer-conditional
algorithm before the first baseline. Retain necessary lifetime/invalidation fixes;
choose further changes from concrete introduced failures and qualification.

Next: freeze the causal matrix and exact fix/extraction manifests, obtain peer
review, fan out bounded implementations, inspect actual code/native results and
commit each dependency. Once the runtime links, execute cache/mutation/scalar,
reentry, teardown and fault controls on the candidate before measurement.

## Evidence and history

- [Canonical provider source/native record](../../../../build/pyvalue-typecache-provider-20261102-v1/coordinator-code-review.md)
- [Core mutation component](core-mutation/status.md)
- [Graph source/native record](../../../../build/pyvalue-typecache-graph1-20261102-v1/coordinator-code-review.md)
- [MRO provider endpoint](mro-providers/status.md)
- [Pickle/state final composition](../../../../build/pyvalue-type-pickle-readers-composed-20261102-v2/independent-composed-review.md)
- [Latest merged build](../merged-build/status.md)

The recording-only counter foundation (`5a9d4841a67`) is committed; counter
recording alone is not transaction authority. The separate full publication
proposal has no implementation grant. Detailed prior proposals, native failures,
reviews and source endpoints stay with their experiments. The former page is
preserved byte-for-byte in [prior notes](history/notes-through-checkpoint45-20261102.txt);
its dated compiler counts and assignments are historical.
