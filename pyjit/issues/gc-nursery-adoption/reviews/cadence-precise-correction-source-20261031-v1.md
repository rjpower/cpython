# Independent exact precise-frame correction — 2026-10-31 v1

Disposition: **ACCEPT exact two-annotation source for fresh, separately reviewed feedback.** No behavioral blocker found. This does not declare a successful linked audit, authorize a native retry, or grant cadence acceptance.

Source is clean `5cc1e5310e2cb5f11feade09195b5a3695188161`, parent `c8cc7718ef3037500aa3902ba5c31acc2b8d7c60`. Capsule SHA `357ecc9b2efb409303e8a7f583ce0e5ccc619caf9cea095c13754f9bdb06cf46`; patch SHA `fd6c944c65334033efbabcc22f1c2dc219fc56bcb1876ff16d64f86fe1001b1f`, in isolated `build/nursery-cadence-precise-correction`.

The first candidate build failed `check-gc-inline`: `select_automatic_kind` was an outlined collecting static callee outside the precise section. No focus or negative case ran. The preserved failed ELF independently shows local `minor_capable` (51 bytes) and `select_automatic_kind` (271 bytes) in section14 `.text`, and the selector calls `minor_capable`. The conservative source classifier treats both as reentrant. Merely fixing the selector would leave its outlined static callee subject to the same contract.

The exact diff changes only `Python/gc_live.c`, replacing `static int` with `Py_GC_PRECISE static int` on those two definitions. I reconstructed the expected file from the parent using exactly those two substitutions and compared it byte for byte. No body, transition, condition, callback, test, API, audit rule or opaque exception changed. This follows the existing precise-frame convention instead of weakening the linked audit. The [earlier exact-source reasoning](cadence-source-20261031-v1.md) continues to apply to unchanged logic/tests, with a fresh native build required for the new annotations.

The host precise-root check independently reports zero errors for the corrected file. The macro places these definitions in `py_gc_precise` on the supported target; actual outlined/inlined placement, DWARF/frame coverage, and the complete linked audit still require fresh products. No native success is inferred from the annotations or source check.

All 587 failed-build products independently match their preserved index, SHA `5dbca2cd56904150fbcccd0a18c86780d870f3778443fce294ca9c3239d80bac`; failed ELF SHA `1af2570292019a2bbe8e7dfc03f51fd483d63a387256914090babb50fc44d9be`. The original failure, gate terminal/cleanup evidence, source and product index remain intact. Existing precise-collection helpers are unchanged; no failed image is patched or reused as a corrected build.

Any successor helper batch must bind this exact source, reviewed derived negative commits, and fresh `debug-cadence-precise` products. It must retain the **original 2026-11-01 00:14:59.201344 UTC batch deadline**, not start another 45 minutes. Focus, the three causal auxiliaries, coherent GC feedback, release performance and qualification remain unexecuted/pending as appropriate. Coordinator helper review and activation precede native work.

Host receipt: `build/nursery-adoption-review/cadence-precise-correction-source-20261031-v1.json`, SHA `0303072fc87396f39cbff3b5598c244e6bf94462314619ec6131c170aec6a58d`. No target, build, suite, workload or timing was run by this reviewer.
