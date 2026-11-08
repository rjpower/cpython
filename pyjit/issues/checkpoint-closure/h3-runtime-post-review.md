# H3 producer post-review

Reviewed the integrated H3 v2 runtime producer and the coordinator's preserved `crash-probe` dump. No must-fix finding in this bounded review.

The generator emits handler IDs for the full 319-entry domain and flags/base metadata for 298 named entries. Generated dispatch targets call `INSTRUCTION_STATS` with their actual target ID. The macro indexes full-domain arrays, records pairs from the prior entry (CACHE-0 initially), and updates `lastopcode` even when the aggregate stats pointer is absent. The emitter writes version, dispatch mode, domain size, entry count, triples, target counts, and pair counts. Regeneration leaves `IR_NUM_HANDLERS` in `opcode_ids.h` only.

The old malloc-debug/stats crash indexed `opcode_stats[306]` in a 256-row table through `UPDATE_MISS_STATS`. In the rebuilt dump, `COMPARE_OP_JUMP_FLOAT` is handler 306 with base 303, 150 counted entries, and six recorded miss events. The 319-row table makes that path in bounds; the coordinator's exact `PYTHONMALLOC=malloc_debug PYTHONSTATS=1` import probe exited successfully. `BINARY_OP_ADD` is an IR-only target with 191 counted entries. The reader validated and rendered this real dump without relaxing the count or ownership evidence.

This review establishes the producer and one runtime path. The coordinator owns the focused stats tests and wider benchmark run; it should not treat this receipt as a full-suite or all-kernel result.
