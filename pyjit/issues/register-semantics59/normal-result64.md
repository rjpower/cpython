# Normal result receipt for global loads

Source implemented and independently approved at
`cf9310fbd23ee635bb7b349ecf8d3233f28ac9d9`; root generation, compilation and
native qualification remain pending. This transports an existing producer fact
into common replay; it does not change execution bodies or establish private
await qualification by itself.

## Failure and contract

The await6 diagnostic reaches the four-word prefix and passes its geometry.
Its argument home has initialized, nonNULL and owner facts. The callable from
`LOAD_GLOBAL` has none: generic publication uses the checked array-home writer,
and neither it nor the two guarded global specializations exports a complete
normal result receipt. CALL must continue declining that missing proof.

Add `_PyIR_OWNER_NORMAL_RESULT`, a single normal-only ownership event. Its
sole consumer in `semantic_step_core` first calls `semantic_leaf_step` with
the existing UNKNOWN transfer, preserving that fallback's conservative
invalidation. After success, only a real scratch DST receives
`SEM_INITIALIZED | SEM_NONNULL | SEM_MAY_OWNER`; its old NO_HOME/NULL facts
are replaced. Local and discard destinations acquire no scratch fact.
`complete` remains false. The event grants no callback snapshot, physical NULL,
error ingress, static readable home range, CALL_DRAIN or complete-image proof.
Before invalidating incoming facts, inspecting replay rejects a known
MAY_OWNER scratch destination: the new receipt cannot erase a detected
overwrite conflict. Unknown is neither a vacancy proof nor a readable-home
grant; this normal postcondition describes the reached new publication.
Do not use `_PyIR_TRANSFER_FRESH`: it would preserve unrelated incoming facts
across callback-capable lookup and cleanup.

## Source and reached emission

Use one bounded producer-source query returning the actual producer statement,
output name and success boundary. It must check the complete parsed uop chain,
IO/cache geometry, suffix, binding names and absence of substitutions. It must
not select a contract from an opcode name or declared output alone.

The initial admitted recipes are:

* The generic name acquisition followed by
  `IR_WRITE_OUTPUT(res, 0, value_load_global(GLOBALS(), BUILTINS(), name))`,
  `IR_FINISH_VALUE_OPERATION()` and `ERROR_IF(PyValue_IsNull(res[0]))`.
  The specialization prefix and optional NULL-slot suffix must match their
  actual unchanged recipes. The helper returns an owned canonical value or
  NULL: `_PyEval_LoadGlobalValue` obtains an owning result, handles lazy import
  replacement and publishes every nonNULL result before returning. FINISH
  closes bridges and refreshes the current frame before the success test.
* Each complete module/builtins cached chain: exact dictionary/version guard,
  unicode-table assertion, entry bounds guard, borrowed entry load, NULL deopt,
  owning XDUP and the unchanged optional NULL-slot suffix. All guard/cache
  operands and the duplicated entry must match, including the globals guard
  before the builtins chain. Dict entry home canonicality supplies the
  representation fact; XDUP supplies the distinct owner.

The emitter records success only when it actually reaches the checked writer
and post-FINISH NULL guard, or the checked guarded XDUP and final destination
publication, in that order with the current output unchanged. Missing helper,
publication, guard, owning duplicate, changed index/version, output overwrite,
address escape, shadowed binding or added effect loses the optional receipt.
The generic recipe's success guard is after refresh; a remembered pre-refresh
native value is insufficient. Record and no-record emission must agree.

Every member of the ordinary global cohort must earn the same event. If any
member loses it, suppress the cohort receipt, as ordinary pristine words can
redispatch to a specialization. Keep the existing error routes/checkpoints and
normal-storage graph absent unless their independent source analysis proves
them. No new helper annotation may accidentally enable those stronger routes.

## Files and consumers

Runtime source changes are limited to the event enum in `pycore_ir.h`, its
normal replay arm in `Objects/codeobject.c`, the checked source/emission query
and cohort intersection in `ir_generator.py`, and focused source fixtures.
Generated metadata changes are root-owned. No bytecodes/helper-body change is
planned.

The owner-program census is `Objects/codeobject.c`,
`Include/internal/pycore_ir_outcomes.h`, `Python/ceval.c`, and the metadata
serializer. Only normal replay recognizes the new event. Window-result,
physical transfer, call absence, created-empty, completion/null-destination,
error-snapshot and phase selectors continue accepting only their existing exact
event kinds. The serializer merely interns the new event. Recheck every
switch/default and numeric event-domain assumption in the actual diff.

## Gates

Focused host controls must show all three real producers earn the receipt in
both emission modes, while source mutations above lose it. Rename an ordinary
instruction without changing its source to show names are not authority.
Exercise full metadata generation in memory: only the three global rows gain
the normal-only event, with transfer UNKNOWN and all error/home/phase fields
unchanged. Compare generated runtime bodies byte-for-byte with the pre-fix
source packet. Keep private OUTCOME/UNKNOWN and ordinary CALL_DRAIN intact.

Root then regenerates, compiles both roles and runs the existing native14
activation/owner/observer fixtures. The decisive admission witness is cold
pending followed by actual private entry for the compiled natural await,
without weakening callable or argument proof. Allocation failure and bare
YIELD callee negatives remain independent whole-unit gates. No throughput or
selection claim follows from this correction.

The four host source methods pass, including both-mode body equality, source
mutations, reached-publication denial, family intersection, full metadata in
memory and fresh global-label emission. Evidence is
`E59/deferred-await-source-host/normal-result2.log` (4 passes, 6.098 seconds).
The initial frozen packet lacked emitter constructor defaults; independent
review found it before full generation. The corrected patch is
`normal-result-final.patch`, SHA256
`096524d809721c769e2aad6a7e46d3b4fb9402ee8238d9a41f6d9cc7d3390e45`.
Only the three global ownership rows change in the in-memory metadata control;
all their other fields and all other contracts remain equal. No generated
products or target execution were performed by this source worker.


Root integrated source at `ba603b73c1a` and generated metadata at `791c48d0df5`.
Full generation and independent generated-product review pass: one new event,
exactly three changed contracts across 333 normalized rows, no changed
execution product. Both await7 builds pass 117/79. Root cold replay confirms
restored callable facts and pending nomination. Private activation remains
unqualified because later SEND quickening revokes its installed head; that
separate certificate-lifetime issue does not justify a stronger result receipt.
Evidence is `E59/deferred-await-native-controls/{root-normal-result-products.json,normal-result-generated-peer.json,admission3.log,specialize4.log}`.
