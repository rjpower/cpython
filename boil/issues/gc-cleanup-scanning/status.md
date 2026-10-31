# GC cleanup and scanning

- State: closed
- Outcome: landed
- Updated: 2026-10-31
- Owner: coordinator for result maintenance
- Dependencies: none outstanding for this supported milestone
- Branch/head: retained `boil/gc-native-cleanup` at `0b15b4bbc9b`
- Integration: `aee46aacae2`; qualified source/native `0b15b4bbc9b`

## Now

Selective death processing, guarded native suspended-generator cleanup,
direct managed scanning, warning delivery and coroutine initialization repairs
are landed. Independent review clears the source-delta qualification composite
and both postmerge smokes. The supported boundary is Linux, one main interpreter,
the GIL and ordinary threads.

The completed matched 29-workload result is **0.871038× upstream** geometrically,
with seven faster and 22 slower rows. Measurements describe the combined change;
five serial samples supply descriptive ranges, not confidence intervals.

## Next

No implementation is assigned here. Keep the source/products and raw results
sealed. Current follow-ups are on the [work board](../README.md).

## Completion

Landing, applicable qualification, postmerge behavior and the finite performance
report are complete. The composite names the source actually executed for each
scope and preserves failed runs; it does not claim fresh full suites for every
source revision.

## References

- [Independent landing review](reviews/landing.md).
- [Performance report](performance.md).
- [Current runtime contracts](../../design/runtime-architecture.md).
