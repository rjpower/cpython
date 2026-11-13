# Validated same-home ADD revision

Preparation only, pending independent peer GO and root's isolated
`frame54-clears-validated` tree. Do not edit or relabel measured `frame54-clears`
at4fbd. Root owns generation, builds, native execution and measurements. This
revision is justified as a bounded test of removing repeated geometry checks;
the initial screen and reached-route counts remain separate evidence.

## One geometry authority

Add a pure shared inline predicate in pycore_ir.h over word, actual local
boundary and physical home count. It proves A==B, C!=B, B scratch, both read
operands below the physical extent, B/C below IR_MAX_SLOTS, and thus no discard
or invalid read sentinel. It neither reads homes nor allocates, observes,
decodes arbitrary instructions or changes owners. Use exactly this predicate
at refinement admission and selection; do not create subtly different copies.

The common constructor validator must call it for actual
BINARY_OP_ADD_INT_OVERWRITE_TEMP words, with localsplusnames length and declared
stacksize. Keep size arithmetic wide/safe and reject unrepresentable refinement
geometry before narrowing. Existing generic instruction validation is unchanged.
Report the bad word index and geometry on failure. This does not rely on
_PyIR_H_PRISTINE: that flag does not reject constructor-supplied specializations.

Specialize's existing ADD_INT nomination calls the same predicate using the
pinned executing code's local boundary/home extent before installing the
refinement. Its cost is paid once on selection, not every successful addition.
The generator replaces the release fallback with a debug assertion of the same
predicate. Preserve both inline-value guards and the actual analyzed arithmetic/
commit certificate. Keep physical B publication through the corrected narrow
old-inline/new-canonical macro. No geometry branch remains in the release
handler. No change to error, overflow, consumption or observation phases.

## Constructor route closure

| Route | Existing authority | Required revision |
| --- | --- | --- |
| Assembler | Python/assemble.c calls _PyCode_Validate before _PyCode_New | Common refinement check applies even if a future compiler emits it directly. |
| Python/C public constructor and code.replace | code_new_with_fbinit calls _PyCode_Validate, then check_words | Common check applies; source map identity checks must not mask negative fixtures. |
| Marshal load | Python/marshal.c calls _PyCode_Validate, then _PyCode_New | Common check must include B/C physical extent: marshal does **not** call public check_words. |
| Internal _PyCode_New | Explicit trusted validated-input contract | Audit all callers; do not silently assume this routine validates or add a second full generic pass. |
| PyCode_NewEmpty | Direct _PyCode_New caller with fixed assert0[3] literal | Literal contains RESUME/LOAD_COMMON_CONSTANT/RAISE, no refinement; retain this bounded proof. |
| Static init cleanup, interpreter exit and deepfreeze | Fixed ordinary words or compiler/marshal-produced code | Verify no unchecked static refinement literal; no new path to handler326. |

The full selected-source call census found no other _PyCode_New caller beyond
those rows. Keep that census as an implementation/post-review check, including
extension/test providers, rather than infer closure from a public API alone.

## Installer closure

make_exec copies admitted immutable words. _PyIR_SetHandler changes only low16
handler bits, preserving all operand fields. Specialization is the dynamic
refinement source and must check the shared predicate. Instrumentation restores
original words through _PyCode_HandlerFor; a constructor-supplied original
refinement therefore has the common constructor proof. Both fb_record and
fb_put_row continue storing canonical ADD_INT in polymorphic rows. Review every
handler installer and restore route; do not assume a metadata flag enforces it.
Changing function.__code__ cannot change executing-code authority. Creating a
new code object, including code.replace and marshal, must validate anew.

## Focused tests before handoff

Extend the existing numeric operand-mode suite with a real compiler expression
whose warmed final addition uses the variant. Rebuild one code word with the
numeric IR-only handler ID and preserve its valid feedback/source metadata.
Clone source-map bytes with memoryview(...).tobytes(), not bytes(existing_bytes),
which can retain identity and fail at the earlier changed-code guard.

- A valid constructor-supplied refinement executes inline, boxed overflow and
  guard-miss cases with correct values and source observation.
- Parameterize A!=B, B==C, B local, B discard, B outside physical homes, C
  outside physical homes and C read sentinel. Require the geometry error so
  fixtures prove they reached the intended validator, not the source-map guard.
- Marshal round-trip the valid constructed code; mutate its unique serialized
  code-byte payload at equal length to test malformed geometry through marshal's
  common-validation route. Assert payload uniqueness before replacement.
- Change a valid code object's local boundary/stack metadata through replace
  and verify proof is recomputed instead of carried with handler identity.
- Enable/disable instruction observation on the valid constructor function and
  verify original-refinement restore retains behavior; exercise normal
  compiler-specialized restoration too.
- Keep the strict generator mutation negatives. Update the positive generated
  proof to require the shared debug assertion and narrow B publication, and
  reject a release JUMP_TO_PREDICTED geometry fallback in the variant.

Existing tests continue covering locals/discard generic routes, both C ownership
geometries, overflow, deopt, tracing, canonical metadata and ordinary owner
retirement. Do not execute a deliberately malformed generic instruction to
test the new refinement; this lane does not repair unrelated generic admission.

## Root gates

Independent source peer closes all routes before release checks disappear.
Root regenerates and runs constructor/marshal/IR/proof/value suites on release
and debug. Inspect actual native removal of all four geometry comparisons and
branches while B NULL remains absent, C consumption intact and overflow error
precedes mutation. Record text/stack/selection cost; original4fbd offsets and GDB
success address do not apply automatically to this revision. Update a census
probe only after new ELF identity/address proof. Measure the unchanged corpus
against actual golden51, then decide whether the revision merits confirmation
or composition. Do not replace the initial candidate's recorded result with it.
