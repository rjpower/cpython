# Publication cleanup progress

- 2026-11-06: Read PyJIT entry/status/goal/handoff/process and CPython AI-tool policy. Inspected runtime generator, analyzer metadata, retirement macros, and specialized attribute handlers.
- Confirmed five specialized attribute handlers emitted duplicate temporary result stores: one from `SYNC_SP`, then one from `IR_RETIRE_PUBLISHED`'s flush. The first store must stay before receiver retirement; folded-local final writes must stay after.
- Sol and the coordinator reviewed the revised design: remove those five producer markers; make retirement validate owners, clear inputs, and flush before releasing them; report retirement macros as synchronization in analyzer metadata. No cache or cross-effect publication state is needed.
- Added a focused generator test for the one-store-before-retirement contract, retained final local store, and sync metadata. Added an escape rejection test. A direct generator harness passed these assertions.
- Attempted the focused unittest methods with the available build interpreter, but that executable's `_opcode` module lacks `IR_FUSED`, so it cannot import this worktree's `Lib/dis.py`. The full test suite was not run. Root owns regenerated-product comparison and integration/build/performance checks.
- Integration A subsequently ran the focused generator tests in its matching build and accepted the source. Root reported both new generator cases passing; the earlier harness limitation did not block integration.
- 2026-11-06 documentation sync: recorded Integration A's completed checks, cold review, and mixed basic timing; added the Integration B receipt and updated the plan and lifetime design. Integration B is applied and building, with its validation/performance still pending.
