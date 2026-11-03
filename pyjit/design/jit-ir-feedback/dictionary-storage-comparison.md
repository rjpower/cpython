# Dictionary prediction versus shapes and slots

Read-only addendum to the IR proposal, 2026-11-01. No storage rewrite is
authorized. The first draft is retained as `design-initial-draft.md`; the main
proposal's corrected worked example publishes BEFORE state before boxing,
revalidates after callbacks, and reserves AFTER recovery before the store.

The LuaJIT comparison is useful: a dictionary/table lookup need not remain a
full hash search in optimized code. But “Lua tables versus hidden classes” is
not a choice between unoptimized dictionaries and optimized objects. Both
systems specialize access under guards; Python already combines dictionaries,
shared keys, predicted locations and optional shapes.

## What LuaJIT actually does

For suitable constant-key hash accesses, the recorder predicts a node location,
guards the hash mask, reloads the table's current node base, and emits HREFK.
The assembler checks that the expected key is at that location. HREF covers a
more general reference; HLOAD/HSTORE access it and NEWREF handles insertion.
This is guarded location prediction, not a raw slot pointer cached forever.
[Recorder](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_record.c),
[IR operations](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_ir.h),
[HREFK emitter](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_asm_x86.h).

Table resizing can allocate new node storage, reinsert entries and release the
old storage. A stable table object therefore does not imply stable hash-slot
addresses. Alias/effect reasoning constrains reference and load forwarding.
[Table resizing](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_tab.c),
[memory optimizer](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_opt_mem.c).

Lua's own-key case can also bypass `__newindex` when its value is non-nil.
Python cannot transplant that rule: a data descriptor or custom `__setattr__`
can intercept assignment even if `obj.__dict__` already contains the name.
Python setter/MRO/descriptor proof must precede the storage-location proof.
[Lua setter semantics](https://raw.githubusercontent.com/LuaJIT/LuaJIT/v2.1/src/lj_meta.c).

## The Python implementation is already a hybrid

Current STORE_ATTR_WITH_HINT checks dictionary existence, key-table kind,
cached-index bounds, the key at that index and existing-value presence. It
selects either a combined entry's value or split `ma_values[index]`, not one
universal pointer layout. It preserves dictionary notification and old-value
release semantics. See `Python/bytecodes.c:3013–3058` and
`Python/specialize.c:915–953`.

Shared-key dictionaries avoid repeating the same attribute names/hash structure
for every instance; split values and inline values make locations predictable.
The optional per-type shape records present slots/insertion-order structure,
not the types of stored values. `Include/internal/pycore_dict.h:249–305` and
`Objects/dictobject.c` describe these layouts. V8's Maps are another explicit
layout/transition scheme, not evidence that Python must adopt all of V8's
storage choices. [V8 Maps](https://v8.dev/docs/hidden-classes).

| Backing/access strategy | Potential simplicity or benefit | Cost and contract |
|---|---|---|
| General per-instance dictionary | One ordinary dynamic name→value container; fewer shape-transition cases | Hash/key storage per instance, lookup/cache guards, growth/deletion/order/notifications and aliasing remain |
| Shared keys + split/inline values | Shared name metadata and predictable indexed values | Must guard layout/key identity, presence and changes/materialization |
| Shape over instance values | Compact presence/layout discrimination and guarded field offsets | Extra shape metadata/transitions/fallbacks; must retain type/version and storage proof |
| Declared member slots | Fixed locations for declared descriptors and C layouts | Not arbitrary dynamic attributes; descriptors, inheritance, `__dict__` and extension ABI still govern behavior |

Putting all *ordinary dynamic attributes* in dictionaries is a legitimate
policy hypothesis. Putting every Python/C-extension attribute there is a much
larger semantic/layout change: `__slots__`, member/getset descriptors,
read-only fields and native extension structures cannot simply be redirected
without changing their contracts. `__dict__` exposure/identity and direct
`vars(obj)` mutation must remain observable. Generic storage also does not
remove `__getattribute__`, `__getattr__`, `__setattr__`, descriptor or metaclass
dispatch.

## A concrete shared-IR experiment

First add a `DictAttrLocation` access strategy beneath the existing SetAttr IR:

```text
prove receiver type/MRO/setter/descriptor strategy
load the receiver's current exact attribute dict
guard key-layout identity/version or bounds+key at predicted index
guard split-versus-combined representation and present/non-NULL value
derive the current value address from the current base
perform the existing notification/ownership-aware store operation
```

Do not retain that address across growth, rehash, delete/reinsert, dictionary
replacement/materialization, callbacks, finalizers or an unknown aliasing
write. Recompute the base and validate location afterward. Structural key
guards and value facts are different: replacing `x` can preserve its location
while changing its value/type. Do not require every unrelated value write to
invalidate a sound key-location cache, and do not preserve a cached value just
because key structure stayed stable. Keep watcher/notification behavior through
the existing core operation until an equally precise raw-store protocol is
proved. No callback may occur between the final validated address and a raw
store without another validation.

The first comparison should lower this dictionary strategy and the existing
shared-key/shape strategy through the **same** semantic IR, snapshots, generic
fallback and backend. Exercise stable fields, mixed classes, additions,
delete/reinsert, forced growth, `__dict__` aliases, setter/descriptor mutation,
watched dictionaries and destructor reentry. Measure whole-program elapsed,
allocation/retained memory, compiled size, guard/lookup work and churn. These
are hypotheses, not present measured advantages.

This can test whether cached dictionary access gives sufficient speed with
less machinery before choosing a storage simplification. It does not require
maintaining permanent alternate runtime modes, adding an allocator, or
rewriting every object's storage before the method/trace compiler works.
