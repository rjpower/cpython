# Complete the value-native cell getset

Preparation only, based on the current execution-boundaries runtime source.
No runtime edits, builds, native tests or commits were performed for this
proposal. Measure the combined lookup/keyword candidate before considering this
next increment. Independent proposal review and coordinator approval are still
required before implementation.

## Concrete bridge and existing mechanism

The unchanged `pyjit/bench/internal/micro/cell_contents.py` creates one closure
cell containing canonical inline integer 3000, then performs 1,000 reads per
outer loop. `Objects/cellobject.c:181` currently gets an independent owner with
`_PyCell_GetValue`, converts it to an object through
`_PyValue_AsObjectAt(..., BOX_SITE_CELL)`, and closes the value owner. Its
PyGetSetDef registers only the object callbacks. Consequently
`Objects/descrobject.c:getset_get_value` enters the ordinary object getter and
canonicalizes the returned owned integer box with `_PyValue_FromObjectResult`.
The result is already a canonical value before this round trip.

Use the existing mechanism in `Include/internal/pycore_descrobject.h`:

```
typedef PyValue (*_Py_valuegetter)(PyValueBorrow self, void *closure);
typedef int (*_Py_valuesetter)(PyValueBorrow self, PyValueBorrow value,
                             void *closure);
```

`_Py_VALUE_GETSET` installs shared object thunks and a static `_PyValueGetSet`
record. `PyDescr_NewGetSet` recognizes `_PyGetSet_ValueGet` and places the record
in `d_value`. The descriptor's value getter/setter then call the typed record
directly. Existing property, numeric and decimal getsets use this mechanism;
`_testinternalcapi.VGetSet` and `ValueGetSetTests` exercise both forms. No new
callback ABI, casts, descriptor specialization or cached box is needed.

## Minimum patch

Confine runtime production changes to `Objects/cellobject.c`:

1. Include `pycore_descrobject.h` for the existing typed record macro.
2. Replace the static object getter with a typed value getter. Borrow self,
   recover its already-checked cell object, and return `_PyCell_GetValue`'s
   independent owner directly. On NULL, set exactly `ValueError("Cell is empty")`
   and return `PyValue_NULL`, as attribute access does today.
3. Replace the static object setter with a typed setter. Pass the borrowed self
   object and borrowed value to `PyCell_SetValue`; NULL means deletion.
4. Register both through `_Py_VALUE_GETSET("cell_contents", ..., NULL, NULL)`.

Both callbacks must move together. Merely setting a value-getter record while
retaining the object setter is incorrect: `getset_set_value` dispatches the
legacy setter only when `d_value == NULL`. A record with NULL typed setter would
make the attribute unwritable. Converting the pair is smaller than changing
the descriptor engine to support another mixed convention.

The removed static getter is not a public C interface. The shared
`_PyGetSet_ValueGet` supplies the registered object's owned-result thunk.
`PyCell_Get`, `PyCell_GetValue`, `PyCell_Set` and `PyCell_SetValue` retain their
interfaces and behavior. In particular an empty `PyCell_Get` returns NULL
without setting an exception; the attribute getter raises ValueError. Do not
merge those distinct error contracts.

## Ownership, callbacks and type rules

`_PyCell_GetValue` rechecks the canonical home and duplicates it; the new getter
returns that owner without a close or borrowed escape. Exact inline scalars
need no allocation. Mortal objects retain an independent reference before
receiver retirement. The shared object thunk transfers a mortal result or
boxes an inline result for its caller, and propagates boxing MemoryError without
changing the cell. Public object access keeps its owned-box contract.

The setter borrows its input. `PyCell_SetValue` duplicates it before
`_PyCell_SetValue` swaps the canonical home and closes the previous owner.
An old-value finalizer therefore sees the new home. A nested setter may replace
it again; the outer setter performs no later home write. Deletion publishes
NULL before retirement and remains idempotent. Self-assignment acquires the new
owner before retiring the old one. No new exception handling or lifetime policy
is needed.

`descr_check_value` already rejects unrelated receivers before callback entry;
class-level access returns the descriptor. Keep those paths untouched. Cell
itself lacks `Py_TPFLAGS_BASETYPE`, so subclass construction remains rejected.
Subclasses of the stored value, including int/float subclasses, remain ordinary
objects with identity and behavior preserved. Exact integers outside the inline
range, NaN and other object-form contents also stay objects. This change must
not canonicalize subclasses or reinterpret an object's bits as an inline value.

## Expected bridge ledger

These are source-derived predictions to verify, not measured results. Let
`N = loops * 1000` for the unchanged cell-read microcase, with setup/warmup
outside the counter interval.

| Operation | Current path | Proposed path |
| --- | --- | --- |
| Canonical cell read | One independent value owner | Same |
| Inline-3000 result boxing | N `BOX_SITE_CELL` boxes and PyLong allocations | Zero attributable to these reads |
| Owned-number canonicalization | N `_PyValue_FromNumberSteal` conversions and box releases | Zero attributable to these reads |
| Receiver boxing | None: self is already a cell object | None |
| Object API getter | Required owned box for an inline result | Still required |

The owned-number conversion does **not** increment `entry_unbox`; do not claim
that counter must fall by N. Existing stats count boxes by site/kind/owner and
already suffice to distinguish the eliminated cell getter bridge. The site is
shared with struct sequences and C cell getters, so measure a controlled delta
rather than requiring a process-wide zero. Shared object descriptor thunks use
attribute/THUNK attribution instead of the old static getter's CELL attribution;
that expected accounting change is not eliminated compatibility traffic.

Converting the setter also removes its unnecessary inline-input object adapter
box/canonicalization on value-native assignment. That is a consequence of
registering a coherent typed getset pair, not a reason to expand this packet
into other native boundaries. Attribute lookup, result publication, loop work
and ordinary owner transfers remain; the full historical cell micro ratio
(2.028× raw upstream instructions) is not the removable boxing budget.

## Focused validation and boundaries

Production scope is one C file. Extend `Lib/test/test_pyvalue_cell.py` using
existing `value_getattr`, `value_setattr`, `value_delattr` and
`getset_direct_get` fixtures where helpful. Keep the shared descriptor engine,
getset ABI, compiler/generated handlers and other types unchanged.

The cheapest meaningful ladder after approval is:

1. Review the typed signatures and macro registration; compile real release
   and debug builds. Run the value API checker because object/value boundaries
   are the subject of the change.
2. Run existing cell and `ValueGetSetTests`, then new controls for exact empty
   error text, repeated deletion, assignment, closure nonlocal mutation and
   attribute mutation observing the same home. Cover inline int/float, big
   integer, NaN, bool, None, mutable objects and numeric subclasses.
3. Cover descriptor class access and unrelated receivers via `__get__`/`__set__`,
   plus rejection of cell subclassing. Call the direct object getter only with
   a valid cell: that fixture bypasses descriptor receiver validation.
4. Check result retention after deleting/replacing the cell's source home,
   alias self-assignment, cycles and temporary receivers. On setter/deletion
   retirement, finalizers should observe the published replacement/empty state;
   nested mutations must survive. Use collection when needed rather than
   asserting exact CPython destruction timing.
5. Retain `cell_value_boxing_errors` in
   `Modules/_testinternalcapi/tuple.c` for `PyCell_Get`. Extend that existing
   helper to exercise the registered object descriptor getter under forced
   boxing failure if the shared-thunk route lacks a cell-specific control.
   Resolve its descriptor before enabling the failing allocator, restore the
   allocator before assertions/cleanup, and verify MemoryError plus unchanged
   contents. This is the only possible additional C test-file scope; no new
   exported test API is needed.
6. Root checks controlled stats: N fewer cell-read boxes, unchanged value kind
   and zero added receiver boxes. Profile the typed descriptor route so an
   accidentally retained object callback cannot pass solely on result equality.
   Independently review the actual diff and executed evidence before integration.
7. Root measures the unchanged cell microcase and applicable application/closure
   workloads against the measured combined candidate. Report box reduction,
   instructions and paired elapsed time separately. No throughput benefit is
   claimed before those comparisons.

Stop at this cell getset. Do not add a borrowed box cache, change canonical
numeric identity, modify public object-result ownership or port unrelated
getsets to rescue a small application effect. Preserve the measured candidate
while preparing and reviewing this proposal.
