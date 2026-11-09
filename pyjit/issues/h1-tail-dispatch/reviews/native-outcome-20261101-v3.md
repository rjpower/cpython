# VM0 compiler/backend focused native outcome, revision 3

2026-11-01. **Accept focused correctness feedback and readiness for a separately reviewed elapsed experiment.** No backend adoption, speed, full qualification, stats-census or GC-parity credit.

Root confirmed supervisor 66715 terminal exit 0 at 05:42:05 UTC. All fourteen rows completed at 05:40:26.387184 UTC, before the fixed 05:29:19.976325–06:14:19.976325 window expired. Completion SHA256 `819a0de025ac87319da88abc3567738d775e8884d838198e30bb541d1afbbc35`; preparation capsule `99a8a7cc8366416e250a5a1275e6af63ff37c50f340fe96b32774eb2a761d7c1`; plan `3f1620b40a357fd93edde78b91a8c67e36a7ca1d9a5106338fd9eb109b0ed6a9` and manifest `0f1b1eeb7a77e665dc7d2ffcd00fb380d3997df3595b836c0f7acab4c6a420fd` remain unchanged.

Clang source is exact clean `417a9443a599e879c76c34c13ae76943bd6f6b3e`; the reused GCC image comes from qualified `e9e10d5b503eaf219797a5db44766eeab6fad4dc`, linked by the reviewed concrete runtime-byte/source bridge. G is GCC computed release; CD/CR are fresh Clang computed debug/release; TD/TR are fresh Clang tail debug/release. Actual Makefile/pyconfig, compile/configure receipts and runtime records establish O1 debug/O3 release, nonstats, GIL, mimalloc, IR and the intended tail/computed profile. No source/ABI increment was mixed into this comparison.

| Scope | Ordinary passes / declared skips | Focus / stale collections | Preserved native products | Image SHA256 prefix |
|---|---:|---:|---:|---|
| G | 20 / 1 | 391 / 709 | 588 | `289cf16e3baf` |
| CD | 20 / 1 | 401 / 713 | 588 | `b2c70bde61e0` |
| CR | 20 / 1 | 391 / 709 | 588 | `4db5c4cf5080` |
| TD | 21 / 0 | 401 / 713 | 588 | `e219d052c1f7` |
| TR | 21 / 0 | 391 / 709 | 588 | `cc5fd4ab300d` |

Each focus cohort selected the same exact 21 methods: 105 invocations, 102 ordinary passes and three skips. The only skipped case is `test.test_gc.StackWalkTests.test_tail_call_handlers_precise`, reason `requires the tail-call interpreter`, once in G/CD/CR. It executes without skipping in TD and TR, checking a nonempty precise-handler section and register-walk evidence in both normal and monitored RAISE contexts. These are five focused module selections per image, not five complete suites or 105 unique methods.

Actual selected-method telemetry contains four original subprocess children per focus cohort (twenty total), validated by scheduled/start/end/returned identities, original body digests, raw output hashes and native root PID bindings. Six ordinary driver/worker PIDs per focus cohort and its present platform auxiliary PID also bind to actual records. No stats method or stats capture ran. Root-only inner PIDs do not earn invented case or child credit.

Each of the five stale rows contains one physical target process with all six canonical controls reporting 100/100. This establishes five target children, thirty control-case executions and 3,000 internal iterations. All eleven function/class AST definitions in each probe match the canonical unchanged source. Actual stale start/end records show OFF/no extras and stress zero; each nominated child PID has positive native collections. Every cohort has zero fallbacks and observed zero unpinned; unpinned remains report-only. Stale rows earn no ordinary parent-method credit.

The unchanged full-result, original-child and stale validators were replayed against actual raw streams. All 232 completion artifact bindings, 36,825 scoped source inputs, master/scoped helper identities/modes and literal launch/timeout/verification contracts passed. Product inventories, image/configuration hashes, CAPI/library origins, selected tool identities and ELF evidence remain bound. Four fresh builds completed configure/make successfully and preserve both precise-source and linked-inline audit stamps; their Makefiles require those audits with the available pinned readelf. G's qualified native image was reused without rebuilding. All fresh builds report 116 modules, ten missing and zero failed imports; the missing modules receive no support qualification.

Clang 21.1.8 driver/frontend/LLVM/resource/dependency metadata is pinned, and actual build commands/runtime versions establish the compiler/backend profile. Tool discovery deliberately labels this metadata: an external assembler's presence does not prove it was executed, and this review claims no separate integrated-versus-external assembler trace. PGO, LTO, stats and other backends remain outside this feedback scope.

Every row's gate and verifier returned zero, with terminal cleanup idle and before deadline. An independent final `/proc` executable scan under all five existing image locks found no target PID. The initial reviewer-only partial replay omitted the unchanged supervisor's added `ROOTS_BATCH_DEADLINE` environment field; the host failure log was preserved and the check corrected to that exact reviewed environment. No compiler, target, replacement case or native probe ran during this audit.

This verdict permits preparing a separate fixed elapsed experiment. It does not select Clang or tail dispatch, attribute register/code-layout effects, amend a parity reference or extend the hard GC cutoff. Performance claims require actual scores under their own predeclared protocol.

Evidence: [independent receipt](../../../../build/vm0-outcome-review-20261101-v3/receipt.json), containing complete per-row counts, images, product counts, PID mappings, raw bindings and current-idle observation.
