# Values and object protocol

`Include/pyvalue.h` defines the tagged value representation. Exact int32 and
non-NaN doubles can be inline; object-valued cases retain object identity.
Representations must be canonical at producers and boundaries. Raw pointer
identity, language identity and value comparison are distinct operations;
use the declared API rather than treating every value as a `PyObject *`.

## Managed storage

Frames, list/tuple elements, dictionary entries, cells and positional instance
slots carry values. Traversal visits strong object-valued fields with their
actual initialized bounds. An inline scalar is not an object address; an
unaligned or stale tag is not permission to read a header.

Object and value forms meet at C API boundaries. Boxing can allocate and collect;
unboxing requires the promised type/range checks. Values and objects needed across
a collecting call must be kept by initialized frame slots or declared C roots.
An object-form API without an error channel is not interchangeable with a
fallible conversion.

## Attribute lookup

Shapes hold positional layout information and associated version facts.
Lookup plans distinguish stored fields, descriptors, methods, dictionaries and
custom hooks. Reading an existing value payload need not box it. A storage-type
promise does not prove that every lookup route returns that representation.

Keep owner shape, type version, descriptor precedence and dictionary/materialized
storage transitions coherent. A speculative fast path must retain the generic
behavior for subclasses, hooks, missing attributes, mutation and exceptions.
Type mutation and shape invalidation are part of the same contract as a load.

## Slots and calls

The maintained value slot interfaces carry attribute/descriptor/call values;
object receivers and object-form compatibility functions remain explicit.
Native call conventions differ, so a value argument path does not prove a
value receiver or result path. Generic calls, specialization, escaped methods,
generated wrappers and error paths must agree when an interface changes.

Do not introduce object-form twins or silent fallback interfaces merely to
avoid updating affected callers. Preserve externally visible data, errors,
identity and collection safety across a conversion.

The [value/native issue](../issues/value-native-interfaces/status.md) tracks
remaining interface work. The [GC contract](gc-roots-and-lifetimes.md) governs
root lifetime; relocation and borrowed-pointer updates need additional contracts.
