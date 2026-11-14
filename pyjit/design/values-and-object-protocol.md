# Values and object protocol

The selected core uses one canonical eight-byte PyValue in frames, containers,
cells, attribute fields and operation arguments/results. Integration under RC
is complete, including rebuilt bundled extensions. The [representation](pyvalue.md)
defines the encoding; [status](../STATUS.md) and the
[current performance report](../issues/interpreter-performance56/report.md) own
qualification and measurements. Integrated56 transport experiments do not
change which runtime is selected.

In the selected 64-bit GIL profile, exact int51-range integers and non-NaN double
values are inline. None/bools retain immortal
singleton identity; NaNs, large integers, subclasses and other objects retain
object form. Internal NULL/error/index sentinels are not language values.
Canonical producers and boundaries keep representations consistent. Raw bits,
pointer identity, language identity and comparison are different operations.

PyValue is private core representation. Rebuilt object APIs and opaque extension
handles have explicit [boundary contracts](extension-api.md); they do not expose
core bits or cast a tagged array into PyObject pointers.
The [value API](value-api.md) specifies the internal value-form API and the
`PyObject *` compatibility layer over it.

## Ownership and storage

Borrow, duplicate, move, steal and close are semantic operations even for a
tagged value. Heap fields own strong or immortal references; promote a mortal
borrow before storing it. Inline scalars need no object refcount. Traversal
visits initialized strong object edges and skips scalar/sentinel payloads.

Publish initialized slots and structural metadata before exposing storage.
Detach or replace old owners before releasing them; release may reenter Python.
Clearing containers and cycles follows the [RC lifetime contract](gc-roots-and-lifetimes.md).

Canonicalizing an owned scalar object cannot hide an early release. Prepare
the immediate while retaining its retired box, publish the destination and
caller state, then release displaced owners/boxes in the required order and
refresh callback-sensitive facts. Boxing at an outward boundary can allocate
and fail; it returns an owned result with an error channel.

## Attributes and calls

Distinguish normal instance fields, managed dictionaries, descriptors, methods,
builtin receivers and custom attribute hooks. A storage kind does not determine
the whole operation. Guard type/version, shape and descriptor precedence before
using a specialized access; keep generic Python behavior when guards fail.

Native boundaries capture roots → prepare arguments → invoke → stage → publish
caller state → clean up → refresh. Owned object views and arrays support ordinary
rebuilt parser bodies under the [native boundary contract](native-boundary.md);
borrowed scalar boxes cannot escape their scopes. Object bridges remain where
an object-form API requires them; the opaque HPy-compatible backend is unimplemented.
The same semantic/ownership contract applies to interpreter and JIT paths.

The [restoration design](../issues/pyvalue-rc-restoration/design.md) records
encoding and migration detail. Its original public-value-ABI proposal is
superseded by the private-core/opaque-extension design above. Do not restore old
tracing-only no-op ownership or keep boxed/value runtime modes.
