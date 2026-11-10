# A: class-receiver classmethod forwarding

Proposed second landing after B is qualified and its measurement window closes.
Runtime46 base is `b8e7e1bee6611eeda320ce163cd3ce89323da877`; implementation
must start from root's actual committed B head. No runtime edits, regeneration,
native execution or commits were performed for this plan. Astra approved this
family in the single grouped review; Sol owns this component's numbering,
ownership and actual-diff reviews. Root's explicit greenlight remains required.

## Scope and numbered route

Add one specialized LOAD_ATTR family handler, **`LOAD_ATTR_CLASSMETHOD=111`**,
for method-form access through a class whose metaclass is exactly builtin
`type`, resolving an exact builtin `classmethod` with a current exact Python
function target. Attribute-form retrieval, instance access, custom metaclasses,
classmethod subclasses and arbitrary wrapped callables retain ordinary behavior.
Do not add argument/binder specializations or change CALL/frame/return ABI.

Intentionally repurpose retired `STORE_FAST_LOAD_FAST111` with an explicit
format revision, following the documented `CLEANUP_THROW87` precedent:

* In `Tools/cases_generator/ir_numbers.txt`, replace the parsed
  `# removed: STORE_FAST_LOAD_FAST 111` with a plain explanatory revision comment
  plus `LOAD_ATTR_CLASSMETHOD 111`. Keeping both would make `parse_table` reject
  duplicate active/retired use; do not weaken that check.
* Update `Include/internal/pycore_magic_number.h` from64417 to**64418**, with the
  reason recorded. Confirm the actual B head still uses64417 before editing;
  coordinate if another admitted format change has consumed64418.
* Preserve every other number, MIN_INSTRUMENTED_OPCODE233, IR-only boundary267,
  real 8-bit opcode metadata and the existing `uint8_t specialize` argument.
  This is not an IR-only326 handler or a 16-bit family migration.

B changes warmed payload interpretation but not serialized generic IR. A's
format bump is an intentional pinned-number policy decision for retired-slot
reuse, even though ordinary marshal serializes original generic words rather
than warmed specialized handlers. Never reinterpret old-format slot111 as A.
Old pyc magic must fail the normal importer check and regenerate from source;
no compatibility branch or stale-cache fallback is added.

## One lookup and payload architecture

Extend the existing `Python/specialize.c:specialize_class_load_attr` switch with
`PYTHON_CLASSMETHOD`, retaining B's shared resolution and metaclass/MRO precedence
logic. Admit only when:

1. `instr_oparg(instr) & 1` requests method-form output;
2. `Py_TYPE(owner) == &PyType_Type` (builtin metaclass);
3. descriptor is exactly `PyClassMethod_Type`;
4. `_PyClassMethod_GetFunc(descriptor)` is non-NULL and exactly `PyFunction_Type`;
5. the build is not free-threaded until synchronized target acquisition has a
   separately reviewed contract.

Store the **classmethod descriptor** in B's `fb->class_value` and the accessed
class's current nonzero version in `fb->class_version`, then publish handler111.
Do not store a target pointer or function version in ATTR. The class/MRO dict
owns the descriptor; a class-version match makes the descriptor metadata valid.
Same-object `classmethod.__init__` changes the wrapped callable without changing
that class version, so validity does not freeze its payload.

The existing API is sufficient: `Include/internal/pycore_function.h:65` declares
`PyObject *_PyClassMethod_GetFunc(PyObject *cm)`, explicitly returning a borrowed
reference and requiring a classmethod receiver. `Objects/funcobject.c:2052`
returns `cm_callable`; it performs no descriptor invocation or allocation. The
header is already available to specialization/generated interpreter code;
verify inclusion at implementation rather than adding another getter.

## Hit guards and owned outputs

Proposed DSL composition in `Python/bytecodes.c`:

```text
LOAD_ATTR_CLASSMETHOD = unused/1
    + _GUARD_BUILTIN_METACLASS
    + _CHECK_ATTR_CLASS
    + unused/2
    + _LOAD_ATTR_CLASSMETHOD
```

`_GUARD_BUILTIN_METACLASS` has `(owner -- owner)` and no cache effect. Every hit
must check the owner has exactly builtin `type` as its type; selection-time
eligibility alone is insufficient. It precedes existing class-version checking
and guarded payload interpretation. Its object guard must reject inline/nonclass
receivers through the existing IR guard/deopt mechanism without boxing them.
Use a single guard uop, not duplicated class lookup logic.

`_LOAD_ATTR_CLASSMETHOD` has `(class_value/4, owner -- attr, self)` and asserts
the method bit. After both guards it loads the guarded descriptor into a
transient borrow, confirms the exact descriptor kind, calls the existing getter,
and exits if the live target is NULL or not an exact Python function. No input
or output ownership has changed on those misses. Then:

```c
attr = PyValue_DUP(PyValue_FromObjectBorrow(target));
self = TAKE(owner);
STAT_INC(LOAD_ATTR, hit);
```

This is the ordinary owned function/receiver pair. The class is transferred as
the implicit first argument; the defining base is not substituted for an
accessed subclass. A borrowed local receiver is duplicated by the existing TAKE
contract; a temporary receiver's owner moves. There is no receiver retirement,
bound-method allocation, callback or failure after acquisition. Do not apply
the single-result retirement marker to this two-output transfer. Existing
method-pair emission/publication machinery stores both canonical owners.

Explicit argument expressions run after lookup as before. If they reinitialize
the descriptor, delete attributes, change bases or remove class names, the owned
pair retains the lookup-time function and accessed class. CALL must not reload
the descriptor. Function `__code__`, defaults and provider mutations are checked
by ordinary CALL against the captured function's current state. Stack/recursion,
PEP523, observation, binding errors, entry and result retirement remain ordinary.

Free-threaded specialization is declined in the producer. Include an explicit
reader-side free-threaded miss if the scoped contract cannot otherwise prove
that an executable111 cannot appear there; do not acquire the borrowed live
target while another thread may replace/free it. Sol should select the smallest
clear enforcement, rather than introducing a lock protocol in this increment.

## Cache, history and generated metadata

Cache operands are `class_version/2` and `class_value/4`, at legacy selectors2
and6, with the same padding as handler189. Native entry offsets are generated
from `_PyFbAttr`; do not hardcode byte offsets. ATTR48/POLY120 and4096-unit
admission remain unchanged. In `fb_row_of`, add111 to B's guarded payload branch.
`row.aux` records class_version; the recorded payload is the descriptor word,
never a frozen `cm_callable`. Existing reset clears the entry, and history copies
metadata bits before handler publication. There is no historical execution/replay
path to add. Guarded historical words do not create owners or live borrows.

Required generated contract facts:

| Fact | Value |
| --- | --- |
| Specialized handler |111, base LOAD_ATTR78 |
| IR fields | A destination, B receiver, C feedback unit, D original method-form operand |
| Reads / writes | one receiver / fixed two-result window |
| Payload type | `_PyValueGuarded`, not PyObject pointer/typed borrow heap storage |
| Cache selectors |2→class_version,6→class_value |
| Deopt | ordinary LOAD_ATTR; misses precede acquisition |
| NULL suffix | absent: A supplies the bound class itself |
| Stats | existing LOAD_ATTR hit once after checks; per-handler dispatch count for111 |

Root regenerates all products together: opcode IDs/names/specializations,
opcode metadata/cache/deopt tables, stack effects/uop/dispatch targets, IR
metadata/targets/generated cases, and Python opcode metadata used by dis.
Use normal regeneration, not a hand-maintained output list. No generator
numbering exception is required. `Lib/test/test_ir_contract.py` and `test_dis.py`
contain generic map/family checks; `test_ir_boundaries.py`/operand-mode tests use
name lookups. A read-only search found no literal111 or classmethod-specific
adaptive disassembly fixture in those tests that needs preemptive rewriting.
Add an explicit adaptive-dis test for111’s name/base/method window, then update
only fixtures whose actual regenerated output changes. Original generic
disassembly and source evaluation order must stay the same.

## Focused tests and actual file boundaries

Handwritten implementation: `Python/specialize.c`, `Python/bytecodes.c`,
`Tools/cases_generator/ir_numbers.txt`, `Include/internal/pycore_magic_number.h`.
Tests: extend the owned attr cache/codegen modules, or add the small dedicated
`Lib/test/test_ir_classmethod_cache.py`; contract/dis assertions belong in their
existing modules. There is no descriptor/API/frame/cache-layout edit planned.

Require warm111 plus ordinary exact/general Python CALL at the same tested
caller, with no bound-method CALL for that site. Cover ordinary class and
inherited subclass binding; zero/one/multiple explicit arguments; current
defaults and error behavior; and escaping attribute-form retrieval with correct
bound `__self__`/`__func__`. Test same-object descriptor reinitialization before
lookup and during an explicit argument expression: the next lookup sees the
new target, but an already captured pair invokes its old target once. Change
captured function code/defaults during arguments and retain normal CALL behavior.

Warm then replace/delete attributes, change bases, switch the wrapped function
to a non-function callable, activate observers, or make argument evaluation
raise. Remove other function/class owners during argument evaluation and use
weakrefs/finalizer-bearing payloads to check valid ownership and once-only
cleanup; exact CPython finalizer timing is not the oracle. Custom metaclasses,
descriptor subclasses and instance shadowing must keep ordinary behavior and
never claim111 hits. Alternate multiple classes/functions at one caller, inspect
recorded111 descriptor rows only after execution exists, mutate the same
descriptor again and confirm the next lookup reloads its live target.

Host tests check the actual DSL: exact metaclass/class-version guard order,
guarded descriptor typing, NULL/nonfunction misses before acquisition, owned
two-result outputs, absence of receiver close/NULL suffix, and stats placement.
Pinned-number validation must reject active+removed111 duplication and accept
the intentional revision without changing other IDs. Serialization/import tests
must observe magic64418 and fresh source loading, without a broad cache framework.
Root owns API gate, regeneration, release/debug builds, focused descriptor/
class/super/call/monitoring tests and measured comparison to committed B.
Record coverage plus elapsed/instruction regressions explicitly. This saves
wrapper lookup/allocation/unwrap work, not callee activation or frame teardown.
