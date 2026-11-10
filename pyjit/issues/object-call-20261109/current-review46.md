# Candidate 46 qualification and bounded feedback census

**The representation/API repair and stale disassembly fixture are resolved by the retained evidence. Full-suite limitations remain; performance admission awaits candidate 46 measurements.** Reviewed clean runtime `b8e7e1bee6611eeda320ce163cd3ce89323da877`. Its only diff from 45 is the approved `test_dis` expected-offset list and explanatory comment; production C and the 64417 format identity are unchanged.

Evidence root `E`: `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/object-call-20261109`.

## Qualification audit

Inspected `object46-build-checks.json`, `object46-focus-checks.json`, `object46-stats-checks.json`, their retained logs, the 45 API log, and both 45 full-suite logs.

- Candidate 46 release/debug builds record returncode 0 at the expected clean source and 117 checked modules each.
- Candidate 46 complete `test_dis` runs pass on both builds: 143 tests each. The later cache-field/state assertions now execute successfully; the earlier eight stale-fixture failures are not merely removed from a report.
- Candidate 46 stats build and smoke pass; the smoke reports 150 tests across two modules.
- Candidate 45 API qualification reports 306 value-native units, zero failing findings and zero parse errors. Because46 changes only the test fixture, this covers identical production C/checker inputs. It is not a newly executed46 API campaign, and non-failing checker notes remain.
- Candidate 45 corrected focused release/debug campaigns previously passed 564 tests each. Both full 45 runs later completed 54,674 tests; neither was green.

Full 45 release reports 16 GDB pretty-printer failures, eight `test_dis` failures and nine `test_sys_settrace` **errors**. Debug reports 21 GDB failures, eight disassembly failures, one `test_lltrace` failure and nine settrace **errors**. Summary failure totals 24/30 exclude those nine errors; do not accidentally add them into the wrong counter. The failing module categories match the coordinator's known debugger/frame-jump/trace limitations plus the now-fixed fixture. These aggregate logs omit most individual tracebacks, so this review does not independently reclassify every error against historical witnesses. Preserve both failed full-suite records and the separate successful fixture reruns. Do not claim a full 46 suite was run or passed.

## Census scope and integrity

Audited all eight `coverage46-{golden,candidate}-{class_attr_load,logging_silent,deltablue,coroutines}.json` files and `coverage46-commands.json`. All invocations return 0. Each report has equal runtime identity pre/post and equal input manifests pre/post. Native/source identities are frozen 39 `4d666e1b482…` and candidate 46 `b8e7e1bee661…`; magics are 64416/64417 respectively. The revised script checks configured source, expected `Lib`/`dis` origin, logging origin when included, release/JIT/hash/no-site/no-bytecode conditions, executable hash/build ID and build-file hashes. The fragile inferred poly-capacity output has been removed.

Recomputed every total from the per-code/site rows. Matched code scopes using source digest, qualname and first line; all four pairs have identical matched scope with no unmatched keys. Site operation ordering and private/shared admission agree for every matched code object. Inspection checks that feedback presence and bytes do not change while reading it, and the native getters do not force an executing copy.

One limitation remains: input manifests are taken after module loading/setup and before the census/warmup, then afterward. They prove unchanged recorded source during that interval, not a pre-import/pre-setup manifest. Clean runtime identities and matching benchmark/scoped-source digests strengthen provenance, but this is not an assertion that the entire loading phase was bracketed. No helper was edited during this audit or the active performance campaign.

## Actual footprint and admission consequences

The scope is module-defined functions, descriptors and nested code constants. Logging additionally includes its stdlib module, including dormant functions. Module top-level code, unrelated callees, execution frequency, poly tables, code words, feedback-block headers and allocator overhead are outside these totals.

| Case | Matched code objects | Private attr-family sites, both | Planned entry bytes 39→46 | Allocated entry bytes after one warmup 39→46 |
| --- | ---: | ---: | ---: | ---: |
| Class load | 1 | 1 | 344→352 | 344→352 |
| Logging silent | 165 | 633 | 62,264→67,328 | 12,144→13,104 |
| DeltaBlue | 67 | 328 | 25,568→28,192 | 24,200→26,664 |
| Coroutines | 3 | 1 | 720→728 | 720→728 |

All scoped attr-family sites remain private; none becomes shared. The planned increases are exactly 8 bytes times the scoped attribute-site count: 8,5,064,2,624,8 bytes. Actual allocated entry increases after warmup are 8,960,2,464,8 bytes because dormant code has no allocated executing feedback. These are increases of approximately 2.33%,7.91%,10.18%,1.11% in the measured entry-byte totals, not whole-process RSS percentages.

The scope therefore shows **no private-site admission loss in these four cases**, despite larger entries. It does not negate the synthetic capacity result: attr-only functions now fit 682 private entries rather than818 at a two-unit start; mixed allocation can have a different limit. Near-limit functions elsewhere can still lose specialization coverage. Poly rows separately grow 88→120 bytes; this census intentionally makes no allocated-poly-byte claim.

After setup, allocated entry bytes are 0→0 for class/coroutines, 9,456→10,208 for logging, and 232→256 for DeltaBlue. “After setup” is not a cold-interpreter baseline. One benchmark warmup allocates more executing copies; it does not execute all scoped functions.

The class case's one existing-feedback handler changes from generic `LOAD_ATTR` in 39 to `LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES` in 46. This is direct evidence of the intended specialized route at that snapshot. Logging, DeltaBlue and coroutine handler histograms are equal across the paired scopes. In particular, DeltaBlue retains 35 generic LOAD_ATTR and four generic STORE_ATTR snapshots alongside its specialized sites. These counts identify places for later inspection; they are neither misses nor dynamic weights and do not prove those generic sites were hot.

## Decision

Candidate 46 is suitable for the coordinator's fresh performance campaign and final evidence consolidation. The split storage preserves observed admission in this bounded sample at a measurable metadata cost. It resolves the prohibited guarded-word union without a checker exception; the magic change correctly versions serialized geometry; the fixture-only successor fixes disassembly expectations.

Do not transfer candidate 43 throughput numbers to 46 or infer that unchanged handler histograms imply unchanged latency. The larger feedback footprint, emitted offsets and previously confirmed application regressions require the new measurements. Known full-suite limitations remain explicit even if those measurements pass the performance gate. No new entry/call/lookup optimization is approved by this review.

This review used source and retained evidence with lightweight host parsing. It performed no runtime edits, builds, native workloads or new perf captures.
