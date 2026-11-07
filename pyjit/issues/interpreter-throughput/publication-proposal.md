# Publish attribute results once before receiver retirement

## Evidence

Five specialized attribute handlers (`LOAD_ATTR_SLOT`, `LOAD_ATTR_INSTANCE_VALUE`, `LOAD_ATTR_MODULE`, `LOAD_ATTR_WITH_HINT`, and `LOAD_ATTR_SHAPE`) previously emitted two identical `IR_STORE_TEMP_DST` statements before `retire_published_values()`. The first came from `SYNC_SP`; the second came from `IR_RETIRE_PUBLISHED` flushing the same result. The first store is required before retiring the receiver because releasing it can run finalizers. The final `IR_STORE_DST` for a folded local is also required. Generic `LOAD_ATTR` does not have this duplicate; its completion helper already tracks temporary publication.

## Reviewed design

Remove `SYNC_SP` from those five producer bodies. Make `IR_RETIRE_PUBLISHED` own the complete pre-retirement synchronization: validate that its arguments are owners, clear input bookkeeping, then flush live values before calling the retirement helper. Keep the final destination write unchanged. This puts publication at the operation that requires it and avoids generator state that would need mutation, control-flow, and callback invalidation rules.

The analyzer's `sync_sp` property must recognize both retirement macros so generated metadata retains its synchronization effect. Other retirement call sites keep their existing producer markers; clearing already-dead input bookkeeping is valid there as well.

## Scope and verification

Producer changes are limited to `Python/bytecodes.c`, `Tools/cases_generator/ir_generator.py`, and `Tools/cases_generator/analyzer.py`, with focused generator tests. The coordinator owns generated products, builds, semantic integration tests, and performance measurements. Do not change `ceval.c` or generated outputs in this workstream.

The generator test should verify the synchronization property, exactly one temporary publication before receiver retirement, and the final folded-local destination write. Existing owner-retirement validation covers an intervening callback/escape.
