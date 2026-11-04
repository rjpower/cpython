# Canonical values and reference ownership

The private 64-bit GIL runtime uses one eight-byte `PyValue` payload for language
values. Exact signed int32 values and non-NaN exact floats are immediate. Other
values, including bool, None, numeric subclasses, large integers and NaNs, are
object pointers. Heap objects still use reference counting and the cyclic
collector. A value does not select a collector or a representation mode.

Pointer payloads carry an ownership bit. `PyValue_Borrow` creates a view rooted
by an existing owner; `PyValue_NewRef` creates an independent strong owner;
`PyValue_Close` consumes one strong owner. Raw bit copies do not create owners.
Scalars have no heap owner. `PyValue_NULL`, tagged iterator indices and
wrapped internal pointers are internal states, not language values.
They must not cross the owning object bridge. Construction holes may be NULL,
but a published language container must satisfy its own hole invariant.

`PyValue_FromObjectBorrow` reads an object's canonical representation without
allocation or release. Its input owner must remain live until the result is
promoted or no longer used. `PyValue_PrepareOwned` consumes an object result
without releasing it: the returned value owns an ordinary object, or the
separate `retired` output owns the exact primitive box replaced by an immediate.
Publish the canonical value and valid live state before releasing that retired
owner. An allocator hook can reenter even when releasing an exact numeric box.

`PyValue_AsObject` returns a new, fallible object reference. It boxes an immediate
when necessary; it does not install a boxed value in a frame or maintain a
permanent cache. Private object views are borrowed only while their explicit
owners remain live. Legacy internal `FromPyObject*` functions describe such C
views; publishing them into a language home must use canonical preparation.
They are not an alternative storage format.

The generated interpreter and shared semantic helpers use `_PyValueTemps` for
private exact-scalar boxes. Preparation is pure; home stores publish first and
queue displaced scalar boxes for scoped release. Records initialize after all
predicted-entry labels. Handler exits release nonempty records before decoding
the next word and revalidate live frame, executable and observer state after
callback-capable cleanup. A previously pending exception remains primary;
cleanup errors do not create cycles in its context chain. Variable call windows
own all evaluated inputs before the first conversion or cold allocation.

Constants are read directly from value tuples. Tuple/list unpacking and virtual
iteration transfer values directly. Proven Unicode metadata such as names may
use an internal checked object-only borrow; generic constants and arguments may
not. Python/C calls publish canonical results before releasing source or
replaced owners. The interpreter's final outward conversion has its own error
cleanup because the entry sentinel is not an ordinary Python frame.

This foundation is a dependency checkpoint. Universal container storage and the
value-native operation/call/parser/Clinic ABI must complete together before
adoption or performance qualification. Object-protocol bridges are explicit
intermediate boundaries, not evidence that the entire native API migration is
complete. The old environment switch and global ID-based value layout are
removed. The 32-bit and free-threaded profiles fail at compilation rather than
silently selecting a different representation. Other platforms remain outside
this foundation's qualification scope.
