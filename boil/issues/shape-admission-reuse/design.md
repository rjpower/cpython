# Match polymorphic shape admission once

2026-10-31, source candidate with bounded O3 feedback. Managed worktree
`boil-shape-admission-reuse`, branch `boil/shape-admission-reuse`, starts at
`abd5c0926df`. Its runtime matches qualified `0b15b4bbc9b`; the rejected frame
transfer patch is absent. Reviewed source is committed as `c46f1bef67f`;
integration and applicable landing qualification remain separate.

The existing polymorphic attribute family finds a shape/version row in a guard
operation, then searches again in its load/store payload. First-row duplication
is already optimized away in O3. Later-row comparisons survive. Remove that
source duplication through the existing DSL, without another handler protocol,
row registry, stack temporary or type-specific opcode.

## Implemented boundary

Admission now sits in each existing `_LOAD_ATTR_SHAPE_POLY` and
`_STORE_ATTR_SHAPE_POLY` body. The established `inline_ok` annotation and
explicit pointer check preserve the old guard's rejection of inline
receivers before admission. The body first
checks `_PyValue_IsPointerSpace(owner)` and exits to the generic family on miss.
It then retrieves the existing table and calls `_PyFb_ShapeRow` once, exiting
on a negative row before any payload access, side effect or input consumption.
The local integer row serves the unchanged slot/method load or store. The
unused guard operation and its two macro memberships are removed.

The matcher remains the single shared algorithm for both operations. LOAD's
existing row kind covers plain data and method/self layout. STORE keeps its
kind check, value write and optional shape transition in their current order.
No row or table pointer survives a call, safepoint or word boundary. No new
generator annotation or effect-table entry is needed.

## Preserved behavior

- Inline/nonpointer receivers miss before `PyValue_ObjectBorrow`, without boxing
  or consuming inputs; generic behavior and error ordering remain authoritative.
- Match cached addresses only against the live owner's shape/version. Never
  dereference a borrowed cached shape, remove version validation or accept row 0
  merely because the address matches. Type mutation/address reuse stays safe.
- Pointer and row misses precede hit counters, stores, shape publication and
  output/input consumption. A store-kind miss remains before the write.
- Preserve descriptor/self output layout, ordinary LOAD NULL output, result
  rooting, miss/hit accounting, monitoring and instruction boundaries. The
  supported GIL path cannot run another Python callback between admission and
  payload; this does not expand the free-threaded support contract.

Host generation confirms one pointer check and one match in each handler,
before payload work, with no receiver boxing. Five generated outputs for the
handler domain, layout and metadata are byte-identical. Internal uop IDs lose
the deleted guard; see [results](results.md) for the exact affected files and
limitations.

## Why this may be worth doing

The change removes duplicated operations and source obligations for the
family, not 6.8% of total CPU. The frozen Richards profile assigns only five
samples, 0.774% of total sampled CPU, to the second LOAD lookup. Most shape-row
samples are required admission. This is a maintainability/work-reduction
candidate; it does not justify a campaign to detect a tiny elapsed gain.
Reject it if preserving the contracts needs an extra path, metadata waiver or
new abstraction. Source and necessary behavior/emission evidence can decide it.
