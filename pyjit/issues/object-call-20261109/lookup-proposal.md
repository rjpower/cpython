# Guarded canonical class values through instances

Proposal only, against frozen runtime `4d666e1b482cb4983c7683f91c5922af14d0ab40`.
No runtime changes, native execution, commits or performance claims accompany
this document. The coordinator must publish the requested checkpoint before
new runtime changes. Independent proposal review precedes implementation.

## Problem and bounded outcome

`class K: k = 1; o = K(); o.k` cannot specialize its unshadowed class value.
`Python/specialize.c:764–770` classifies the canonical scalar as `INLINE_VALUE`;
`do_specialize_instance_load_attr` at 1273–1277 explicitly refuses it. Object
non-descriptors use the guarded `LOAD_ATTR_NONDESCRIPTOR_*` family instead.
The feedback and handlers encode a borrowed `PyObject *`, although the value
already has a canonical representation. The observed class_attr_load micro
is this instance route, not a direct `K.k` load.

The smallest useful increment admits inline class scalars to the existing
non-descriptor family and stores a borrowed canonical value. Keep its current
instance-layout admission, custom-hook exclusions and method-bit restriction.
Use the same result-acquisition and publication transaction as shape/module
reads. No new opcode, object layout, owning cache, global cache, or alternative
interpreter mode is needed. Direct class-object scalar loads, expanded layout
coverage and dictionary bucket hints are separate increments.

The closing micro evidence reports 4.54× upstream raw instructions and 8.83×
after separate skeleton subtraction. These identify a missed path, not its
whole-program cost or a promised speedup. See the preceding packet's
[next priorities](../frame-call-model-20261109/next-steps.md) and
[source addendum](../frame-call-model-20261109/final-micro65-priority-addendum39.md).

## Source audit and guard contract

All source references below refer to the frozen runtime worktree in HANDOFF,
not implementation files in this canonical documentation checkout.

1. `analyze_descriptor_load` (`specialize.c:820–880`) admits the generic
   value getattribute route, classifies hooks/descriptors, and obtains an owned
   canonical descriptor and type version through `_PyType_TryLookupValueNoAlloc`.
   Preserve this admission. Mutable descriptor classes stay excluded.
2. `specialize_instance_load_attr` (`1312–1325`) reads the type version before
   `instance_has_key` reads the shared keys. An existing instance name takes the
   instance-value/dictionary route. An absent shared name can take the class
   route. Keep that ordering and require a nonzero type/shared-key version.
3. `specialize_attr_loadclassattr` (`1635–1717`) admits either inline-value
   objects or a class with no instance dictionary. For a non-descriptor it
   declines other dictionary layouts. Keep that conservative scope. A shared
   key belonging to another instance also prevents this class specialization,
   even when this receiver's slot is empty. Improving that coverage is deferred.
4. `_GUARD_TYPE_VERSION` and `_CHECK_MANAGED_OBJECT_HAS_VALUES`
   (`bytecodes.c:2637–2661`) guard the inline-values handler. There is no live
   shared-key-version comparison: `shared_keys_version` is an admission check,
   not a stored hot guard. This is intentional only because shared-key insertion
   invalidates the owning type. `insert_split_key` (`dictobject.c:1536–1573`)
   pins `dsk_owning_type`, begins a type mutation, calls
   `_PyType_Modified_Unlocked`, then rechecks and inserts the name. Thus an
   instance gaining the formerly absent name invalidates the cached type
   version before the name becomes visible. The implementation must preserve
   this mechanism and test it; `values->valid` alone does not prove absence.
5. Replacing/detaching an instance dictionary invalidates embedded values and
   removes its shape (`dictobject.c:1756–1764`, `7433–7493`). Materializing the
   existing sharing dictionary can retain valid values; mutation through it
   must still use shared-key insertion or invalidate embedded values.
6. Class replacement/deletion, base changes and MRO mutations invalidate the
   receiver type version. Existing type-cache rules support inherited borrowed
   values (`specialize.c:1697–1712`). A cache is not a root. A hit acquires an
   owned result while this guard still proves that a class owns the value.
   Resetting/clearing feedback must never close a borrowed payload.

There is a related ownership problem in the existing object path:
`_LOAD_ATTR_NONDESCRIPTOR_{WITH_VALUES,NO_DICT}` closes `owner` before duplicating
`descr` (`bytecodes.c:4055–4083`). The emitted handlers retain that order
(`generated_ir_cases.c.h:13278` and the adjacent WITH_VALUES case). Receiver
retirement can execute Python, mutate the class, and release the cache's sole
object owner. A version guard checked earlier cannot protect a borrowed result
across that callback. Acquire first, then publish/retire. This is required for
reusing the family safely, including its current object values.

## Representation and coherent factoring

Use one explicit borrowed value member, provisionally `PyValueBorrow class_value`,
in `_PyFbAttr`'s existing final payload union. Keep `descr`, `fget`,
`getattribute` and shape-transition views for the operations whose payloads
really are those kinds. The selected handler determines the member's meaning.
Both object non-descriptors and inline scalars write `class_value` using
`PyValue_Borrow(descriptor)`. Methods continue to write/read `descr`; mutable
objects never become eligible merely because the field can represent them.
This preserves the entry size on the supported 64-bit GIL runtime.

Adapt `specialize_attr_loadclassattr` to accept the canonical descriptor;
extract a `PyObject *` only in the method-specific checks. Merge the unshadowed
`INLINE_VALUE` and `NON_DESCRIPTOR` admissions, retaining `(oparg & 1) == 0`.
Do not box an inline scalar to pass an assertion. The result path is:

1. Read/validate type and receiver-state guards with no callbacks.
2. Duplicate the borrowed canonical cached value into an owned `attr`.
3. Capture the receiver owner with `OWNER(owner)`.
4. Retire through `IR_RETIRE_PUBLISHED_RESULT(attr, receiver)`, then let the
   normal generated destination store finish the transaction.

`_LOAD_ATTR_SHAPE`, `_LOAD_ATTR_INSTANCE_VALUE` and `_LOAD_ATTR_MODULE` already
use this result-retirement contract (`bytecodes.c:2662–2731`). Use their pattern;
a new universal lookup dispatcher would add machinery without solving this gap.
Check local, temporary, discarded and aliased destinations in generated output.

The generator currently treats every four-unit cache operand as `PyObject *`
(`Tools/cases_generator/ir_generator.py:4027–4032`). Add an explicit typed mapping
for the new canonical borrowed cache operand and reject a size/type mismatch;
do not cast tagged bits to an object pointer. Keep pointer operand generation
for the existing object-only operands. Regenerate all affected products from
this producer, including proof/contract output required by the repository.

Polymorphic observations must preserve payload bits without interpreting a
canonical scalar as a pointer. `fb_row_of`/`fb_put_row` (`specialize.c:373–422`)
and `_PyFbPoly` currently copy `descr`. Give snapshots an explicit payload union
with pointer and borrowed-value views, or a fixed-width raw storage member
copied with `memcpy`; handler identity determines its meaning. Prefer the union
if it avoids incidental conversions at binary descriptor sites. Copy the
correct active member according to the recorded handler, and never dereference
or close a historical payload. Update all writers/readers together. The current
`_testinternalcapi.get_feedback` exposes versions and handlers, not payload
pointers; preserve that interface. Review memory accounting in `Python/ir.c`
if any structure size changes. This is a necessary representation port, not a
new polymorphic execution engine.

## Relationship to shape, dictionary and C lookups

The consistent abstraction is a guarded storage recipe yielding a canonical
borrow, followed by an owned result and the existing publication transaction.
Guard recipes differ: type plus shape for filled instance slots; dictionary
keys version/index or checked hint for table entries; type plus no-shadow state
for class values. Keep these concrete proofs explicit rather than pretending
one version means the same thing everywhere.

The C type lookup already returns canonical values:
`_PyType_LookupValueAndVersion` (`typeobject.c:7094–7150`) receives canonical
owned results from `_PyTypeCache_Lookup` or MRO lookup. Object APIs such as
`_PyType_LookupRefAndVersion` deliberately box at their owning boundary. Leave
that boundary in place. This increment connects attribute feedback to the same
value model; it does not replace the C type cache or promise a common table
layout. Other C object-returning bridges remain separate measured work.

A source correction matters for the LuaJIT bucket survey:
`_PyDict_TryGetFirstProbeValue` (`dictobject.c:503–537`) admits only inline
**integer** keys, exact dictionaries and general combined tables. The fixed
string key `'c'` in dict_subscript cannot enter that fast path at all. The
specialized opcode invokes it before the generic value reader
(`bytecodes.c:1200–1215`; generator `try_dict_first_probe:2896–2920`). Therefore
measure key-kind admission separately from first-bucket hit rate. An exact
Unicode identity first probe or a guarded slot hint could be useful later, but
neither is necessary to fix canonical class-value loads. Preserve string
subclasses, collisions, equality callbacks, missing keys and table mutation
fallbacks in any such proposal. No dictionary layout rewrite is proposed here.

## File boundaries and review questions

Expected implementation boundaries are `Include/internal/pycore_feedback.h`,
`Python/specialize.c`, `Python/bytecodes.c`, the typed-cache emitter in
`Tools/cases_generator/ir_generator.py`, their generated products, and focused
existing test modules. Audit feedback snapshots and cache clearing together.
Do not touch frame layouts, call binding, native getsets, hashing policy or
collector behavior. Scope cut: land scalar admission and safe result acquisition
for the two existing non-descriptor handlers; defer direct-class loads and all
new dictionary optimizations.

Independent review should settle: exact borrowed payload/snapshot typing;
shared-key insertion and dictionary replacement completeness; callback-free
interval from guard to result acquisition; generated root/publication behavior;
and unchanged descriptor/custom-hook exclusions. A review finding that this
requires another general representation is a reason to reduce scope, not to
silently expand the packet.

## Verification and decision criteria

The coordinator owns all native execution and timing. These are proposed
checks, not reported results:

- First regenerate and inspect handler/proof output. Release compilation is
  mandatory in C post-review; compile debug too before native semantic checks.
- Extend existing `test_opcache` coverage to assert that warmed `o.k` actually
  uses the retained NONDESCRIPTOR handler for inline int/float/singletons and
  immutable object values. Keep CPython's existing test conventions. Verify
  object value identity and mono/poly feedback across several receiver types.
- Warm a site, then replace/delete `K.k`, mutate an inherited base, change MRO,
  assign/delete an instance shadow, add the same shared key on another instance,
  materialize/replace `__dict__`, and add data/nondata descriptors or custom
  hooks. Assert current values or exact exceptions after each change. Include
  classes with no instance dictionary; unsupported layouts stay generic.
- Use a temporary receiver whose finalizer deletes/replaces the class value.
  The warmed object result must remain live and be the value selected before
  retirement. Cover local/temporary/discarded output and result/receiver alias
  through the existing result-publication tests. Avoid relying on exact prompt
  finalization as a language guarantee; this is an ownership safety stress.
- Run focused opcache, object-attribute, typecache, codegen and applicable
  ownership checks, then independent post-review. The coordinator integrates
  and schedules broader checks at a coherent checkpoint.
- Before timing, retain actual warm-handler/hit evidence. Compare instruction
  counts and boxing traffic on class_attr_load, shadowed/object/no-dict controls,
  then paired application timing including DeltaBlue and logging. Retain only
  a measurable useful path with no material unexplained application regression.
  Keep micro raw/net counts distinct from elapsed time and whole-program share.

No native result, speedup, green-suite claim or admission is implied by this
source proposal. It is ready for independent proposal review after coordination
with the architecture survey and checkpoint publication.
