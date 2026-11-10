# Guarded class lookup: concrete implementation proposal

Base: runtime46 `b8e7e1bee6611eeda320ce163cd3ce89323da877`. This is a
read-only proposal pending the grouped architecture review and Sol peer review.
Implementation order is **B first, A second**, in separately measured commits.
B reaches repeated scalar reads inside conditional selectors. A removes
classmethod wrapper churn. Neither increment removes an ordinary frame.

## B: one canonical payload for existing class loads

Keep `LOAD_ATTR_CLASS=189` and `LOAD_ATTR_CLASS_WITH_METACLASS_CHECK=190`.
Do not add a scalar-specific handler, reuse retired numbers or change magic.
Generalize the two existing handlers to the same `_PyValueGuarded class_value`
field already used for instance class values. Existing admitted object payloads
and new eligible inline values share this representation. Native pointers used
by unrelated method/property handlers remain in their independent native union.
ATTR stays 48 bytes; polymorphic table stays 120 bytes; admission is unchanged.

### Producer and guards

In `Python/specialize.c:specialize_class_load_attr`, retain the existing common
class/MRO/metaclass resolution preamble and both existing guard selections.
This is the shared guarded-class lookup abstraction; do not implement parallel
lookup logic for scalar and object values.

Add `INLINE_VALUE` to its successful value cases. Preserve all existing
`MUTABLE` enum-like checks, `METHOD`/`NON_DESCRIPTOR` eligibility and descriptor
rejections. The successful producer writes:

* `class_version = tp_version` for the accessed class, including inherited
  resolution and MRO invalidation;
* `type_version = meta_version` when the existing mutable-metaclass path needs
  that additional guard;
* `class_value = _PyValueGuarded_FromBorrow(PyValue_Borrow(descriptor))`;
* the same selected existing handler, only after metadata is complete.

Under `Py_GIL_DISABLED`, call `maybe_enable_deferred_ref_count` only when the
canonical descriptor is an object; an inline value must never pass a NULL
object to that helper. Do not widen free-threaded object eligibility beyond
the existing class-load contract in this patch. Keep cache initialization and
handler publication through `fb_begin`/`specialize`/`set_opcode`.

The ordinary handler checks the accessed class's nonzero version with
`_CHECK_ATTR_CLASS`. Its metaclass-check sibling first checks the receiver type
version via `_GUARD_TYPE_VERSION`, then the accessed class version. The existing
metaclass descriptor classification/data-descriptor precedence proof remains
in the producer. Do not replace it with class-dict access or a metaclass-name
test. If review finds a missing existing guard, address it explicitly rather
than advertising broader eligibility under the new payload.

### Reader and source-checked publication

In `Python/bytecodes.c`, change only `_LOAD_ATTR_CLASS`'s operand and body:

```c
op(_LOAD_ATTR_CLASS, (class_value/4, owner -- attr)) {
    STAT_INC(LOAD_ATTR, hit);
    PyValueBorrow found = _PyValueGuarded_Load(class_value);
    assert(!PyValue_IsNull(found));
    attr = IR_CANONICAL_HOME_DUP(found);
    PyValue receiver = OWNER(owner);
    IR_RETIRE_PUBLISHED_RESULT(attr, receiver);
}
```

The existing macros, cache padding and `_PUSH_NULL_CONDITIONAL` remain:

| Handler | DSL cache sequence after initial `unused/1` |
| --- | --- |
| 189 | `class_version/2`, `unused/2`, `class_value/4` |
| 190 | `type_version/2`, `class_version/2`, `class_value/4` |

These are DSL cache units, not the 8-byte feedback allocation units. Their
generated selector positions remain unchanged; the value selector at legacy
position 6 now names `offsetof(_PyFb_LOAD_ATTR, class_value)` instead of `descr`.
The canonical IR word remains A=destination, B=receiver, C=feedback unit,
D=original LOAD_ATTR operand including its method bit. Preserve optional NULL
self output: a class attribute holding a plain function is an unbound callable,
not a classmethod. Existing object class loads must keep that behavior.

`Tools/cases_generator/ir_generator.py:write_uop` already recognizes
`class_value/4` as guarded metadata. `result_publication_contract` structurally
checks direct canonical acquisition and the final retirement marker. Its suffix
validator permits an effect-free final NULL-output suffix, but this is a required
host codegen check, not an assumed completed proof. Both handlers must acquire
after guards, publish before receiver retirement, retain the result through
reentry, refresh instruction/handler state, and expose the checked result phase.
Both target and non-target paths must get reloadable `this_instr`. If the real
NULL suffix fails validation, change the specific producer shape with a Sol
review; do not add a handler-name proof exemption or weaken suffix validation.

### Historical observations and selector snapshots

Move the two existing class-handler cases in `Python/specialize.c:fb_row_of`
from the native `payload.descr` branch to `payload.class_value`. Preserve
`row.type_version` and `row.aux` (the latter carries class_version through the
existing union). Copy guarded bits as metadata; no transient borrow or object
conversion is created while copying a historical row. `fb_put_row` already
copies the whole independently stored payload before publishing `handler[i]`.

Regenerated cache-selector tables must name the guarded field for both IDs.
Snapshots and inspection must identify the active handler before interpreting
metadata; old pointer-view interpretation is no longer valid for IDs189/190.
Historical poly entries remain non-owning observations, not live results.
Their class/metaclass versions are not permission to dereference after mutation.
Tests must exercise object→scalar→other-class-handler transitions at one site.
No new poly dispatch/replay mechanism is proposed.

Update the field comment in `Include/internal/pycore_feedback.h`: `class_value`
is guarded by receiver type_version for instance class values and by
class_version plus the required metaclass guard for direct class values.
This is a comment change, not a storage change.

### Exact edited and regenerated files

Handwritten B changes are `Python/specialize.c`, `Python/bytecodes.c`,
`Include/internal/pycore_feedback.h` (comment),
`Lib/test/test_ir_attr_cache.py`, and `Lib/test/test_ir_attr_cache_codegen.py`.
Add test/contract assertions in `Lib/test/test_ir_contract.py` only if that is
the existing home for the generated result-phase/selector property under test.
Change the generator only if the source-shape proof exposes an actual missing
contract, reviewed separately. No change to frame, CALL, type cache, descriptor
layout or checker policy is planned.

Coordinator regeneration must update all affected products together, including
`Python/generated_ir_cases.c.h`, generated tier1 cases, opcode metadata/cache
selectors and IR metadata/targets. Use the established regeneration target;
do not hand-edit generated files or predeclare a smaller generated diff.

## A: class-receiver classmethod split pair, second landing

Reuse B's common class resolution, class/metaclass guards and guarded payload.
For method-form loads of an exact builtin `classmethod` wrapping an exact Python
function, store the **descriptor itself** in `class_value`, not its current
wrapped function. First scope requires an exact builtin `type` metaclass;
decline free-threaded specialization until synchronized live target acquisition
is separately established. Instance receivers remain generic for this lane.

Proposed handler name is `LOAD_ATTR_CLASSMETHOD`; no number is reserved yet.
All real slots below MIN_INSTRUMENTED_OPCODE233 are active or retired, and
`specialize` currently takes `uint8_t`. An IR-only326 handler is not a drop-in
family member. Numbering is an explicit A gate: choose either a documented
format-bumped retired-slot reuse (precedent: CLEANUP_THROW87) or a separately
reviewed wider-handler route. Do not solve it as an incidental B change.

Its DSL shape is the same class_version/unused/class_value cache sequence as
189, with a shared guard ensuring builtin-type metaclass on every hit, followed
by `_LOAD_ATTR_CLASSMETHOD(class_value/4, owner -- attr, self)`. The guard must
exit before reading guarded descriptor bits if the receiver now has a custom
metaclass; a selection-time check alone is insufficient. Factor that additional
guard as one uop, rather than duplicate lookup or weaken B's general guards.

After those guards, create a transient descriptor borrow, validate exact
classmethod, live-read `_PyClassMethod_GetFunc`, and exit on a non-exact Python
function **before** acquiring outputs. Then acquire the owned function as
`attr` and transfer the owned class receiver as `self`, following the existing
method-pair output convention. No callback, release or allocation may occur
between descriptor guard and acquisition. No receiver is retired here: its
ownership becomes the class argument. CALL keeps its existing function-version,
binding, observer, recursion and stack guards and ordinary frame/return ABI.

Same-object `classmethod.__init__` replacement does not invalidate class version;
that is why the wrapped function is loaded live. Argument evaluation may then
replace/reinitialize the descriptor or mutate bases: the pair must retain the
lookup-time target and accessed class. Never reload at CALL. Changes to the
captured function's code/defaults remain ordinary CALL binding events.
Attribute-form classmethod access must still produce a real bound method.

A adds its descriptor-backed guarded payload case to `fb_row_of`; row selection
never freezes the mutable wrapped function. It uses existing `STAT_INC(LOAD_ATTR,
hit)` once after all guards; subsequent CALL records its own existing hit.
Per-handler dispatch counts on a stats build distinguish A from object/scalar
loads without adding global counters or release-path instrumentation.

## Measurement boundary

For B, existing IDs now cover both object and scalar loads, so handler counts
alone cannot identify scalar hits. Retain per-code site mapping alongside stats
build handler/call counts: the conditional-selector word3 is a known scalar
site, and warm snapshots establish selected handler, not dynamic frequency.
Use complete ordinary-call/DeltaBlue measurements and existing family hits;
only add bounded diagnostic counters if a missing dynamic quantity decides the
next design choice. Neither B nor A is an entry/return optimization, and their
operation savings must not be counted again inside a future selector recipe.
Root owns all native qualification and measurement. Mostly-better progress can
land with documented regressions under the revised user gate.
