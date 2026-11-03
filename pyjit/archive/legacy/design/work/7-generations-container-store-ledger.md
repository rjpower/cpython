# Generations: list, dict and set store ledger

Source-only handoff on `df29d0fd246323baadc4d9204930e005d1ed1ba9`
(`boil-pre-i-union`). No implementation, target execution or measurement.
The tree advanced to `5a45f9bb6a73` during inspection; all files inventoried
below remain byte-identical to df29. Existing full tracing is the baseline;
this document identifies new obligations when a minor stops tracing old
objects recursively, not established missing-root bugs in current tracing.

Read with primary `7-nonmoving-generations-design.md`,
`7-generations-domain-ledger.md`, and `7-generations-frame-store-ledger.md`.
Their owner-remembering, pinned-domain, safe-promotion and reentry rules
remain authoritative. No card/target-only remembered set or new-object
exemption is proposed. Classes, shapes, tuples, ports and other owners still
need their own finite ledger before activation.

## Owner and field classification

| Storage | Logical owner and strong contents | Not independent strong edges |
| --- | --- | --- |
| List `ob_item[0:Py_SIZE]` | `PyListObject`; each canonical `PyValue` object reference. NULL construction slots and immediates need no target edge. `list_traverse` at listobject.c:3160 scans this interval. | Raw PyMem vector, capacity, length and spare slots are not separately aged objects. A vector pointer is an ownership/storage handoff, not an ordinary Python target. |
| Dict combined entries | `PyDictObject`; general entries own key/value `PyValue`s; unicode entries own raw string key plus value. `dict_traverse` at dictobject.c:4149 visits unicode keys too, although strings are not cycle-GC containers. | Hashes, index bytes, versions, watcher bits, `ma_used`, frozen hash and key-table refcount are control data. `ma_keys` is raw owner storage. |
| Set/frozenset active entries | `PySetObject`; each active `setentry.key` is a canonical strong value. `set_traverse` at setobject.c:791 enumerates active keys. | NULL/dummy slots, hash, fill/used/mask/finger and table pointer are control/storage; inline `smalltable` and heap table share one owner. |

**Actual dict correction:** df29 has no `ma_values`/split dictionary and no
mutable shared keys. `dictobject.h` and dictobject.c's combined-table contract
give each nonempty dictionary its own keys table (`dk_refcnt == 1`); only
`Py_EMPTY_KEYS` is shared/immortal. The earlier broad inventory's split/shared
keys row is not an additional dict implementation to close on this source.
Instance shapes/type storage remain separate owners outside this audit.

The proposed primitive is conceptually “remember this old owner before
publishing new strong contents.” It does not exist yet. It must be
nonallocating/noncollecting/idempotent, use actual age membership rather than
tracked/HAVE_GC state, and run **after** the last allocating conversion,
watcher, hash, equality or iterator callback. A noncollecting bulk batch can
remember once. An allocating loop must recheck at each publication. Rooting
the container or the inputs does not substitute for this heap-store rule.

## Physical publications to close

Anchors below are paths under df29; generated interpreter bodies derive from
`Python/bytecodes.c` and must be regenerated rather than patched separately.

| List path (`Objects/listobject.c` unless stated) | Placement/interface obligation |
| --- | --- |
| Public `PyList_SET_VALUE`/`SET_ITEM` (Include/cpython/listobject.h:60/73), `PyList_SetItem`:281; `_PyList_AppendValue` (pycore_list.h:36) | Owner is available. Put the publication contract at the value setter, including NULL-capable construction calls. Public inline code cannot simply call an internal-only declaration: choose an exported low-level bridge or an out-of-line setter, migrate all usages, and bind its no-collection contract. `SET_ITEM` canonicalization precedes the barrier. |
| `ins1`:330/354, append-resize:375/382, assignment:912, iterator extend:1021/1070 | Direct raw writes bypass the macro; route through the same owner contract. Iterator production can collect/promote the destination on every iteration. |
| Slice assignment:729–798, extended-slice assignment:3621; fast extend:985/1016; set extend:1107/1127; in-place repeat:840–864 | Remember destination after preparation, before new external values become visible. Exact list/tuple and set-value bulk reads/copies are noncollecting here; one remember covers each such batch. Pure same-owner shifts/reversals/repetition add no new target set, provided its existing remembered state is retained. Do not clear state on resize/permutation. |
| Dict-view extend:1135–1192 | `_PyDict_Next` boxes inline values (dictobject.c:2296); `dictitems` also allocates a tuple each iteration. Recheck destination age after those calls and before each store. Unlike generic iterator extend, these paths expose final resized length before all slots are filled: establish a safe initialized-prefix/zero-slot contract before allowing promotion or minor traversal at those allocation points. |
| Slice/concat/repeat constructors:531–686; `_PyList_FromValues`:3047; extended-slice result:3353–3365 | These fill fresh output after its last object allocation using noncollecting raw copies, then publish length. A narrowly proven no-safepoint construction region can avoid per-slot checks; “new list” alone cannot. Caller inputs must stay rooted through the allocating constructor. |
| `list_sort_impl`:2706–2964, sortslice copies:1413 onwards | List is emptied while saved items, key array and merge buffers survive callbacks. Preserve the current opaque/raw-root reachability of detached arrays under minor tracing, including young descendants of old referenced objects. Remembering the now-empty list is insufficient. Before restoring saved array/length at :2951–2954, remember the actual list owner again; a collection during sort may have consumed an earlier remembered snapshot. Temporary sort keys are root storage, not new list contents. |
| `_STORE_SUBSCR_LIST_INT` (Python/bytecodes.c:1287–1314) | Raw `_PyList_ITEMS(list)[index] = value` at :1309 must use the owner-aware store. `_PyList_ITEMS`/raw `ob_item` still expose writes to callers; getter coverage is not closure. |

| Dict path (`Objects/dictobject.c` unless stated) | Placement/interface obligation |
| --- | --- |
| `insert_combined_dict`:1106–1144, `replace_value`:1152–1170, `insert_to_emptydict`:1208–1244 | Remember `mp` **after `_PyDict_NotifyEvent`**, before key/value/table publication. Insertion must cover both raw unicode keys and values, and general keys/values. `STORE_KEY`/`STORE_VALUE` macros at :202 receive only entry pointers; either add owner to a coherent publication helper or explicitly remember at each enclosing commit. |
| Public/value setters, known-hash setters, `setdefault`:3890–3954, fromkeys, generic update/merge and `_PyDict_FromItems`:1441 | These eventually use insertion/replacement above. Hash/equality/watchers can collect while the destination is partially filled, including untracked construction; a barrier at public API entry or once before the loop is too early. `_PyDict_FromItems` already roots the dict and caller anchors key/value arrays; that is current root coverage, not age exemption. |
| `dict_dict_merge` empty clone:3352–3385; `set_keys`:160 | CLONED watcher precedes raw-table adoption. Remember destination after the watcher and before attaching cloned contents. `set_keys` has an owner but also handles deletion/initialization; a raw table address is not itself an age target. |
| `dictresize`:1293–1387; `clone_combined_dict_keys`:724–749; `new_dict*`:671–723; copy:3590–3664 | PyMem resize/clone is a noncollecting storage operation; same-owner relocation preserves remembered membership. A clone destined for a *different* owner publishes external edges. `new_dict*` allocates the object after receiving the raw table: retain that raw payload/input root proof through allocation, then initialize fields before any safe-promotion evidence. A copied dict's temporary `ma_used == 0` does not hide entries from `dict_traverse`, which uses `dk_nentries`. |
| `_STORE_ATTR_WITH_HINT` (Python/bytecodes.c:2966–2995) | Actual owner is the backing **dict**, not the instance operand. After its MODIFIED watcher, raw `ep->me_value = value` at :2989 bypasses dict setters. Use the dict publication contract. Shape/slot stores adjacent to it require their separate ledger. |

| Set path (`Objects/setobject.c`) | Placement/interface obligation |
| --- | --- |
| `set_add_entry`:207–278, public/value add/update | Remember `so` after the final hash/equality/restart, before **both** freeslot and unused-slot key stores (:265/:273). An insertion may succeed before subsequent table growth fails; preserve the inserted edge and remembered state on that error. |
| `set_merge_lock_held`:680–750 | Empty-target fast table copy (:715) and `set_insert_clean` batch (:732) bypass add-entry. Remember the destination once immediately before these noncollecting batches. The general merge path can compare/call Python per key, so its add-entry commit must recheck each time. |
| `set_insert_clean`:326; `set_table_resize`:389–467 | Clean insertion accepts only a raw table. Its resize uses move existing same-owner edges; its merge use imports other-owner edges. Keep that distinction explicit, rather than blessing every call as construction. Resize's temporary NULL table and stack small-table copy are a bounded noncollecting transaction; failure before detaching leaves original storage/state intact. |
| `set_swap_bodies`:1376–1425 | Remember **both** owners before publishing the swapped tables/smalltable contents. No allocating call occurs in the swap, so one owner-pair batch suffices. A surviving temporary result can be old; do not exempt either operand by call-site naming. |
| `make_new_set_untracked`:1212–1244 and set/frozenset algebra/copy constructors | Existing value span roots iterable/result while user iteration/hash/equality runs. Fully initialized empty header/table is the safe start; subsequent mutable construction uses the same physical barriers even while untracked. `_PySet_Freeze`:1430 changes type in place; preserve age/remembering instead of treating freeze as a new object. |

## Construction, shared helpers and boundary limits

- `PyList_New(n)` (:163) allocates a zeroed value vector; NULL slots are
  traversable. `list_new_prealloc` (:191) has size zero over uninitialized
  capacity. Record those *specific* states as promotion evidence only after
  initialization. `list_resize` (:60) and preallocation use MEM-domain
  allocation, currently noncollecting, but later iterator/boxing/tuple calls
  invalidate a broad constructor exemption. Error reporting itself can
  allocate; the pre-error object must remain valid/rooted.
- Dict and set storage allocation is likewise MEM-domain. This is distinct
  from PyObject allocation and callbacks. Untracked/frozen/private status
  proves neither youth nor traversal safety. Subtype allocator return and
  partially filled untracked constructors must use the domain ledger's
  per-object safe-promotion/SHALLOW policy, not only `_PyObject_GC_TRACK`.
- `PyValue_FromObject` (Include/pyvalue.h:208; Python/pyvalue.c:120) canonicalizes
  exact numeric objects without constructing a box. `PyValue_Object`, list
  object getters, `_PyDict_Next` and object-array conversions can allocate.
  `_PyValue_FromObjectArray` (pycore_value.h:317) and `_Py_memory_repeat`
  (pycore_list.h:61) have raw destinations but no owner: caller must supply
  the owner/batch proof; making them silently infer one is not a design.
- Dict Keys/Values (:2997–3088) create/fill a list in a noncollecting value
  loop after allocation. Dict Items (:3100–3150) first allocates tuples into
  that list, then writes tuple fields. The list can promote during tuple
  allocation; its list stores need coverage. Its final tuple mutations
  require the **tuple owner's** future ledger even though this file owns
  the loop. Set repr likewise builds a list through its value setter (:646).
- Strong `it_seq`, `di_dict`, `dv_dict` and `si_set` fields in container
  iterator/view objects are distinct heap owners. Their initial publications
  and any iterator result-tuple reuse need a small iterator/tuple closure,
  not a claim that borrowing a list/table pointer is itself an edge store.
  Probe cursors, transient borrowed key/value locals and raw entry pointers
  are root/temporary state; handle them with existing root contracts.
- Public list setters and C container APIs must be covered before porting.
  Direct layout clients (`ob_item`, dict entries, set tables) need migration
  to supported owner-aware interfaces. This finite source audit is **not** a
  completed repository/ports raw-write inventory. No compatibility fallback
  is required by the user, but missing clients block enabling minor GC.

## Small implementation boundary and meaningful omissions

First decide the public-header bridge, owner-aware raw publication/bulk
contract and detached-root minor policy. Then migrate the three containers,
their two bytecode bypasses and shared setter callers. Keep storage movement
noncollecting; do not add age to tables or a second remembered-set scheme.
Before activation, independently close tuple/iterator/type/shape/ports
stores, young-index/SHALLOW promotion, pinned-domain and earliest-reentry
metadata commit. Existing note11 and the allocator/frame ledgers still
block a blanket “all stores covered” verdict.

The independent young oracle must enumerate actual old-owner strong fields
without trusting remembered bits or the barrier under test, and reject an
old-to-young edge not supplied by the minor roots/remembered closure. Future
copied controls should omit exactly one owner publication at a time:

1. Old-list macro store and optimized subscript store; then a callback-driven
   extend which promotes the destination before its next young-item store.
2. Old-dict unicode **key** insertion, value replacement and direct hinted
   attribute-dict store, including a watcher that collects before commit.
3. Old-set dummy-slot insertion, empty-target bulk merge and body swap; the
   omitted owner must be the destination, not merely a young input target.
4. Detached list sort with a collecting key/comparison callback, followed by
   restored young contents; observe both detached-root and owner reattachment
   obligations independently.
5. A constructor that survives/promotes during iterable/tuple production,
   then gains a young child. Unsafe partially initialized cases must remain
   young until safe traversal is proven; simply disabling barriers for them
   must fail an oracle/retention assertion, not count as expected survival.

Each negative must fail the named missing-edge oracle before dead-memory
access; content-only tests and crashes are insufficient. Existing container,
weakref/resurrection and allocation/error tests complement that oracle;
no runs or implementation are authorized by this ledger.
