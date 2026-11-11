# Runtime48 instruction-cache audit

This is a read-only audit of selected `5559f60093e`, not rejected experiment49.
Runtime source is `BASE/worktrees/execution-boundaries-20261108`; upstream source
is `BASE/worktrees/upstream-3a440b6` at `3a440b65695`. `BASE` is the build directory
specified in HANDOFF. Native observations use the selected release executable.
Samples use only `BASE/pyjit-link-wave1/deltablue-call-model-20261110/final48-perf-record`.
No benchmark, build, or runtime edit was made for this audit.

## What the existing cache does

Each specialized IR instruction already addresses its feedback directly:
`IR_FB(IR_C(ir_word))` adds `IR_C << IR_FB_UNIT_SHIFT` to
`frame->ir_feedback`. There is no hash computation, associative search, or
global lookaside lookup on the specialized hit path. `_PyFbAttr` is 48 bytes.
It contains a header, type version, an auxiliary version/shape union, cached
index/offset, padding, raw descriptor/transition union, and a separate canonical
guarded class-value word. The opcode identifies the interpretation of these
fields. Four-row 120-byte polymorphic tables record historical observations;
the audited generated attribute hit handlers do not search those tables.

| Operation | Hit proof | Payload |
| --- | --- | --- |
| Shape attribute load/store | Pointer-space receiver; exact nonzero owner type version; exact instance shape | Cached byte offset; shape-add also has target shape |
| Method with inline values | Pointer-space receiver; exact owner type version; inline-values valid byte | Borrowed descriptor pointer; receiver becomes self |
| Method without dict | Pointer-space receiver; exact owner type version | Borrowed descriptor pointer |
| Method with lazy dict | Pointer-space receiver; exact owner type version; managed dict absent | Borrowed descriptor pointer; cached dict displacement |
| Direct class value | Class receiver proof and class version; metaclass version when mutable | Canonical guarded class value |
| Direct Python classmethod | Builtin metaclass proof and class version | Guarded classmethod descriptor; owned function/class result pair |

See `Include/internal/pycore_feedback.h:113`, `pycore_ir.h:540`,
`Python/generated_ir_cases.c.h:13211` and `:13612`, and
`Python/specialize.c:1683` in the selected source.

Type invalidation carries more than identity. Class/MRO mutation invalidates
the receiver version before its descriptor owner can be released. Adding a
previously absent shared instance key invalidates its owning type before
insertion (`Objects/dictobject.c:1538`). This is why a method hit need not repeat
a shared-keys version comparison: its type version also protects absence of
the shadowing instance name. Invalid inline values still require their own
guard. Materialization or abandonment can change that per instance.

Shapes are small indices in a table owned by each type, capped at 64. They
describe present slots and insertion order; they are not globally unique
identities. The shape field is a byte in inline values, reached through
`owner + Py_TYPE(owner)->tp_basicsize`. A matched shape proves the slot is
present, avoiding the generic instance-value null check.

## Why one integer comparison is not the immediate answer

The current hit already uses simple exact integer comparisons. A hash of type,
shape, and offset cannot replace them as a correctness proof: collisions could
load another field or a stale descriptor. The offset is cached *output* selected
by the fixed instruction/name and its proven receiver state, rather than
independent dynamic input that must be hashed each time.

Packing a 32-bit type version and 8-bit shape into 64 bits is collision-free,
but those facts live in separate objects today. Loading both and packing them
merely changes the comparison; it does not remove their dependent loads.
Putting a globally unique state token directly on each instance would need a
protocol covering shape transitions, dict invalidation, class changes, type/MRO
mutation, version rollover, and concurrent builds. Lazily refreshed tokens still
need to validate the type epoch; eager invalidation would require finding all
instances. A token design can be valid, but is substantially larger than this
round and has no demonstrated advantage here.

## What survives native compilation

`objdump -d --no-show-raw-insn --disassemble=...` on the selected executable
shows the following:

- `_TAIL_CALL_LOAD_ATTR_SHAPE_LOCAL_TEMP` is 224 bytes. Its native path decodes
  receiver/feedback, validates type and shape, loads cached offset/value,
  duplicates a counted object if needed, writes the output, and dispatches.
  The generated receiver-retirement condition and callback machinery disappear
  because the local receiver remains owned by its register. They are not an
  optimization opportunity in this handler.
- The type guard loads `owner->ob_type`, then `tp_version_tag`. Shape validation
  next loads `tp_basicsize`, then the shape byte at `owner + basicsize + 4`.
  The cached value offset does not itself depend on loading `tp_basicsize`.
- `_TAIL_CALL_LOAD_ATTR_METHOD_WITH_VALUES` is 741 bytes. After the guards,
  `PyValue_FromObjectBorrow(descr)` retains exact-int/float checks and an
  immortal test. Output publication through `IR_STORE_TEMP` retains additional
  numeric-type tests on both the descriptor result and receiver. Those tests
  are real release instructions, although neither proven method result nor
  this receiver needs numeric recanonicalization.
- The method validity guard has the analogous `owner->type->tp_basicsize`
  dependent load, then reads validity at `owner + basicsize + 3`.

Upstream's generated method handler performs the same semantic type-version
and inline-validity guards and returns descriptor/self directly as stack refs.
Its cached descriptor already has an object-only representation. Upstream's
instance-value load checks inline validity and slot nonnull rather than this
runtime's exact shape. Upstream uses the conventional evaluator symbol in this
build, so it has no standalone `_TAIL_CALL_LOAD_ATTR_METHOD_WITH_VALUES` symbol;
do not compare nonexistent standalone symbol sizes.

The existing flat capture places 9.61% of DeltaBlue samples in the method
handler, 9.18% in shape-local, 6.05% in generic shape, and 3.39% in shape store.
The method `perf annotate --stdio --no-source` view has 238 samples. It shows
samples around both guards and surviving output checks, but instruction skid
and that sample count do not support precise attribution of each load or
comparison. Handler percentages are opportunity bounds, not predicted gains.

## Three discriminating hypotheses

1. **Preserve canonical method facts through feedback and output publication.**
   Record the already canonical method value in `_PyFbAttr.class_value`, acquire
   its owning result under the existing version/validity proof, and publish
   canonical outputs through an explicit generator contract. Apply the same
   rule to method-with-values/no-dict/lazy-dict, rather than adding a DeltaBlue
   opcode. The receiver comes from a canonical register; take/duplicate it
   without recanonicalizing its bits. First check the release disassembly:
   numeric-type branches must disappear on these paths. Then run matched
   DeltaBlue/logging timings and compare method-handler samples. Risks are
   incorrect proof propagation, stale non-owning cache payloads, and retirement
   order under reentry. Tests must cover descriptor replacement, instance method
   shadowing, dict materialization, local/temporary receivers, immortals, and
   finalizer reentry. Retain the guarded-word/raw-pointer separation.

2. **Cache the inline-values guard displacement.** At nomination, store the
   guarded type's inline-values displacement in the existing four-byte padding
   field. After exact type-version validation, read shape/validity using owner
   plus that cached displacement. This removes the `tp_basicsize` dependent
   load while preserving exact guards and 48-byte feedback. First establish
   that supported type-layout changes always fail the type-version proof and
   reject unrepresentable offsets. Then inspect native output before timing.
   Risks include inconsistent nomination for fallback/shape/method variants,
   free-threading field access, and failing to update historical feedback
   consumers if they must reproduce the handler. Test class reassignment,
   different layouts at one site, dict materialization and shape transitions.
   The expected gain is modest; this is not justification for a layout redesign.

3. **Reduce redundant ownership work at method-to-call boundaries.** A method
   read immediately consumed by CALL creates a function owner and self owner,
   then the call machinery transfers and retires them. A shared checked-call
   contract could preserve ownership/facts across that boundary without a
   miniature evaluator or dynamically constructed ledger. First use existing
   semantic call census and native CALL annotation to bound the affected work;
   count actual optimized boundary transitions separately if needed. Prototype
   only after the simpler canonical-output path is measured. Risks are tracing,
   monitoring, fallback/reentry coherence, and repeating experiment49's cost.
   This is a larger follow-up, not a prerequisite for hypotheses 1 or 2.

The first two proposals are orthogonal: one removes repeated representation
work after the hit; the other shortens receiver metadata dependence during
the hit. They should receive the requested grouped Astra review before runtime
implementation and be measured separately before judging their combination.

## A implementation preparation: producers, consumers and contracts

This section prepares implementation without changing runtime source. Astra's
group review remains a prerequisite.

### Cache producer and consumers

The minimum cache change has one nomination producer and three execution
consumers:

- `Python/specialize.c:specialize_attr_loadclassattr` currently branches on
  `is_method` at lines 1756–1761. Store `_PyValueGuarded_FromBorrow(descriptor)`
  for methods as well as nondescription class values. The incoming descriptor
  is already a canonical borrowed PyValue; no new raw-object conversion is
  needed. Preserve the existing versions, inline validity and dict guards.
- `Python/bytecodes.c:_LOAD_ATTR_METHOD_WITH_VALUES`,
  `_LOAD_ATTR_METHOD_NO_DICT`, and `_LOAD_ATTR_METHOD_LAZY_DICT` currently
  declare `descr/4`. Change these operands to the existing `class_value/4`,
  load the guarded canonical borrow, acquire `attr`, then take the receiver
  for `self`. Their assertion can inspect the borrowed descriptor as an object
  in debug mode; release need not canonicalize a raw pointer. This does not
  change cache-unit counts or opcode numbers.
- `Python/specialize.c:fb_row_of` must move those three method cases from
  `row.payload.descr` to `row.payload.class_value`, alongside the existing class
  value cases. `fb_put_row` copies that tagged payload verbatim. Never copy
  bits into a raw `PyObject *` union member and later reinterpret them.
- `Include/internal/pycore_feedback.h` already has separate 8-byte
  `_PyFbAttr.class_value` and `_PyFbPayload.class_value` storage. Keep feedback
  at 48 bytes and poly rows at 120 bytes; update comments describing method
  payloads. No additional root, reference owner or GC visitor is introduced.

`Modules/_testinternalcapi.c:get_feedback` exposes only versions, handler and
state; it does not dereference payloads and needs no representation change.
A repository-wide source search found no consumer of poly payload elements
apart from `fb_put_row`; snapshots remain metadata rather than owners. Preserve
the existing version-before-borrow rule even though a historical row is not
executed today.

`Tools/cases_generator/ir_generator.py:write_uop` already binds `class_value/4`
as `_PyValueGuarded`, validates its width and attribute-feedback domain, and
does not need a new typed cache exemption. `opcode_metadata_generator.py`
derives cache-to-feedback mappings from operand names, so regeneration should
move all three method mappings at cache unit 6 from `descr` to `class_value`.
`Tools/cases_generator/analyzer.py` already recognizes `_PyValueGuarded_Load`
as a borrowing helper. Do not weaken the cache domain checks.

Regenerate `Python/generated_ir_cases.c.h`, `Include/internal/pycore_ir_metadata.h`
and `Include/internal/pycore_opcode_metadata.h` through the established runner.
The normal generation run may update other generated products; inspect that
diff rather than editing generated consumers by hand.

### Existing canonical metadata is insufficient for paired outputs

There are two distinct mechanisms today:

1. `canonical_result_source` produces one `CanonicalResult`. It rejects named
   cache operands, non-single outputs, and non-`store_dst` formats. These three
   method handlers return two temporary outputs and therefore cannot qualify.
2. `result_publication_contract` proves publication of a *first result before
   receiver retirement*. Its `IR_CANONICAL_HOME_DUP` marker currently requires
   `IR_RETIRE_PUBLISHED_RESULT`. Method lookup transfers the receiver to self
   and does not retire it. Adding a dummy retirement marker would misstate the
   operation. Simply using `IR_CANONICAL_HOME_DUP(found)` in the method body
   currently fails analysis with “canonical home declaration requires result
   marker.”

Even broadening detection is insufficient: `IRStack.store` selects canonical
stores only in its single destination branches. Its final ordinary temporary
output branch always calls `IR_STORE_TEMP`/`IR_PUBLISH_TEMP`. Both outputs must
carry an explicit positive fact through this branch to remove publication
rechecks. `IR_STORE_TEMP` calls `_PyValue_Publish`; `_PyValue_CheckTemp` itself
is debug-only and should remain in canonical stores.

The clean contract is **a canonical owning output**, separate from the
**publication-before-retirement effect**. A bounded implementation can extend
the existing canonical source analysis to a set of per-output facts and teach
the pair-output store branch to use an explicit canonical temporary-store
primitive. Facts must follow the producer and surviving stack value, not an
opcode-name allowlist. For A the positive roots are:

- An owned duplicate of `_PyValueGuarded_Load(class_value)`, recorded from
  canonical nomination and read only after the existing validity/version proof.
- `TAKE(input)` when that input is an unchanged canonical register borrow. Its
  local case acquires a count; its temporary case transfers the slot owner.

Reuse source-parser checks for a sole unconditional top-level assignment,
unmodified result through the remaining uops, no address escape or helper
shadowing, no array/alias output, and unchanged stack position. Recognize the
audited constructor/transfer expressions as positive roots. Do not certify
arbitrary `PyValue_DUP` inputs: raw boxes and transient bridges can also be
duplicated. If recognition fails, use ordinary publication. This provides a
small general contract usable by future paired producers, without pretending
the whole C DSL has a general type checker. Keep the receiver-retirement
contract's independent checks intact.

An alternative explicit annotation still needs these producer and suffix
checks; a bare “already canonical” marker would only move the unchecked
assumption. The source-root approach avoids needing that annotation for A.

### Focused qualification

First run host generator checks in `test_ir_attr_cache_codegen` and
`Lib/test/test_generated_cases.py`. Add paired-output positive checks and negative cases for reassigned
results, overwritten outputs, aliases, arrays, boxed/raw constructors, helper
shadowing and preprocessor changes. Preserve the existing tests rejecting
`class_value` outside the attribute-feedback domain or at the wrong width.
Check all three generated method handlers: guards precede guarded load;
attribute/self owners precede publication; no `_PyValue_Publish` or raw-object
recanonicalization remains in their final output stores.

For native behavior use `test_ir_attr_cache`, `test_ir_attr_cache_codegen`,
`test_ir_canonical_home`, `test_tagged_values`, `test_opcache`,
`test_ir_classmethod_cache`, and `test_ir_contract` as the small focused set.
Add method-cache cases to the existing attr-cache tests rather than creating
an independent runner. Warm and assert the selected handler for with-values,
no-dict and lazy-dict variants, then cover:

- Class/base replacement and deletion; adding a shadowing instance key;
  materializing or replacing an instance dictionary.
- Local and temporary receivers, including an otherwise unreferenced temporary
  whose method invokes a callback before returning, proving self remains owned.
- Two or three receiver classes at one site followed by mutation, exercising
  the historical row-copy path without treating it as an executable PIC.
- Python functions and builtin method descriptors, and pointer ownership in
  frame snapshots (`test_tagged_values` already checks independent attr/self
  owner slots around `LOAD_ATTR_METHOD_WITH_VALUES`).

Run a real release compile and inspect disassembly before a quiet benchmark.
The desired evidence is removal of descriptor numeric conversion and both
output publication checks, with no new callback or allocation in the hit path.
Parent/coordinator owns matched DeltaBlue/logging timings, broader screens and
integration; no measurements or implementation occurred during preparation.
