# Private heap-frame deallocation: GC peer

2026-11-12. Design GO for [heap-dealloc-proposal.md](heap-dealloc-proposal.md).
The private embedded-address/FRAME_OBJECT ownership gate must precede any
interpreter-frame read; delayed nonowned deallocation keeps its existing path.
Refcount-zero untracked storage is distinct from live mutable `tp_clear`.

The existing transition record supplies full initialized home and typed-header
authority before weakref callbacks, removes each owner before its close, and
keeps code fixed last through all home/metadata reads. It marks external owners
and never subtracts another container edge. There is no association takeover,
thread reservation or generator exception in this private owned-frame case.

Moving the five observer-field owners into a raw `PyObject *` native workspace
without DUPs makes their actual policy explicit. NULL their container fields
before callbacks, retain every native owner until its original close position,
and preserve field order. The selected conservative C stack/register policy
covers that workspace; an untracked heap-frame pointer alone does not cover
its fields. No global native head, PyValue reinterpretation or typed-prefix ABI
is warranted by this consumer. Future precise native migration is separate.

Focused fixtures must observe code alive after function release, a remaining
home through the precise bridge, metadata lifetime and final record exhaustion.
Live clear's mutation/resurrection tests remain unchanged. The proposed phase
reset precedes callbacks; HEAP_CLEAR does not acquire a normal-return certificate.
Root approval, actual source postreview and merged native execution remain gates.

Frames' two-file source/fixture increment has GC source GO. The owned helper
links before weakref callbacks, removes function/mapping authority before their
closes, uses the existing reverse home cursor, retires native metadata in its
prior order and closes code fixed last. The embedded-address ownership gate
still short-circuits before touching possibly expired external frame storage.
The fixture checks weakref-time roots, lower-home precise authority, nested
records, native metadata lifetime and code/function/frame weakref order. The
phase reset must precede callbacks when the IR field integrates. Host AST and
whitespace checks passed in frames' lane; root owns merged native validation.
