# Final selected checkpoint review

Final disposition: **admitted with explicit performance and diagnostic limits**.
The closing section records both completed full runs and supersedes the earlier
pending assessments retained below.

Selected source: `bf810ec1a33cacb94102c73779d015c18f85aa8d`, based on A
`e014965013b`. C is rejected and retained only as separate experiment evidence.
Review scope is source, coordinator build/focused evidence, native artifacts and
performance admission. No reviewer runtime modifications or new measurements.

## Source and correctness

The selected six-file delta matches the independently reviewed composition:
the existing four-int exception handler becomes shared semantic recovery state;
the observer copies its already resolved recipe on the existing nonredirected
failure path; the matching current activation/frame consumes it once and clears
validity before cleanup. Nested activations remain separate. Chain invalidation
clears the recipe and frame. Existing frame attribution fields remain separate;
the write-only activation ordinal is removed. Ordinary exceptiontable lookup is
unchanged, and the unwinder no longer rereads the source-map container.

The general unavailable-position fixture creates fresh functions through
MAKE_FUNCTION from recursively stripped code, preserves executable, feedback
and exception bytes, and requires ordinary canonical method and exact CALL
specialization plus the correct result. It retains no C-specific assertion.
This proves runtime admission/execution independence for that fixture; it does
not prove source-independent compiler lowering. Existing compile-time line
restrictions remain the separate audit's future work.

Coordinator build identities are clean at the selected commit. Release, debug
and stats builds pass with 117 checked modules. Release/debug focused runs pass
804 tests each, with 7/5 skips; stats passes 150. Strict value API checking passes
306 translation units with zero findings. These establish focused component
qualification; full diagnostics remain a final integration gate.

## Native comparison and remaining work

Independent relocation-normalized comparison of `native50final` confirms all
six supplied handlers are instruction-identical against frozen A: exact CALL
339 instructions, canonical WITH_VALUES/NO_DICT/LAZY_DICT methods 62/61/62,
shape local-temp load 58, and RETURN_VALUE 127. Method sizes remain 249/244/249
bytes and exact CALL 1410. No additional per-instruction work appears in those
paths.

The cold recipe change is not exclusively a source-layout change. At the source
level each C evaluator activation now clears four recipe ints instead of one
ordinal, and the activation record grows. Recipe recording, consumption and
invalidation also copy/clear four ints on their existing observer-error paths.
None adds method/CALL dispatch work. Native evaluator-entry store/code-layout
comparison is not yet supplied. These facts do not establish why logging times
changed; neither initialization cost nor layout is a demonstrated cause.

## Direct selected/A result

`final50-canonical50a` has six balanced pairs, stable identity, no failure and no
postflight error. Selected/A time ratios are:

| Case | Ratio | Pair direction |
| --- | ---: | --- |
| DeltaBlue | 0.995403 | Mixed, near flat |
| logging_silent | 1.074052 | All six slower |
| nbody | 1.015991 | All six slower |

The 7.4% logging loss against A is a material cost of the selected composition
as built. Do not describe the cold recovery change as performance-neutral or
claim an explanation from identical method/CALL assembly. DeltaBlue provides
no clear additional gain over A. Selection trades that measured cost for an
explicit recovery boundary independent of attribution storage.

Full 29-case tables and repeated targets against golden39/upstream/frozen48 are
still running. Final performance admission remains pending, especially required
logging parity against golden39/upstream. No padding, CPI tuning or further C
iteration is proposed. Full release/debug diagnostics follow quiet timing.

## Bounded follow-up assessment, without implementation

The coordinator asked whether inactive recipe scalars need eager zeroing.
Source review finds a single recipe read, guarded by a matching nonnull frame
validity pointer in the current activation. The producer writes all four ints
before publishing that pointer, with no intervening callback or allocation.
There is no whole-activation copy, comparison or serialization; debugger access
uses the activation chain and entry frame. The recipe contains no owned values.
Consequently initialization, consumption and invalidation may clear only frame
validity, leaving inactive scalar contents unread. This is a sound state
contract simplification, subject to retaining publication order and existing
reentry/redirect tests. No source edit, performance claim or causal conclusion
is made by this assessment; the coordinator will decide after diagnostics.

Prepared, without execution, the evidence-owned
`capture-final50-canonicalA-stat.py`. It reuses the sealed FIFO protocol's
invocation, child environment and roster, and checkpoint snapshots. It validates
the complete six-pair/three-case timing calibration, uses honest canonicalA and
candidate roles, captures source/native/module identities before and after,
and schedules three alternating pairs of approximately two-second grouped
user cycles/instructions intervals on CPU 15. Raw perf and child intervals
remain available. This diagnostic can separate per-loop dynamic instruction
growth from CPI/cycle changes without establishing a causal layout explanation.

## Completed initial full-table screens

Both completed two-pair 29-case screens have stable identity and no failure or
postflight error. Golden39 geometric mean is 0.996588; upstream is 1.177045.
Selected/golden39 ratios are DeltaBlue 0.681829 and logging_silent 0.878900;
selected/upstream ratios are 1.137799 and 0.812169 respectively. Logging parity
holds in these screens despite the measured cost against A, but repeated target
qualification remains pending. DeltaBlue still trails upstream in this screen.

The aggregate near-flat golden result hides spectral_norm 1.369933 and
coroutines 1.116468. They must remain visible in final qualification. A direct
frozen48 screen is needed to assess the selected increment; a prior separate A
campaign cannot establish how much of either gap this change introduced.

The completed frozen48 29-case screen is also clean: geometric mean 0.997971,
DeltaBlue 0.920804, logging_silent 1.042419 and spectral_norm 1.106127.
Thus the near-flat aggregate also contains incremental regressions against the
previous checkpoint; they are not exclusively inherited golden gaps.

Completed six-pair target qualification is clean. Against golden39, DeltaBlue
0.685934 and logging_silent 0.895185 win all six pairs; spectral_norm 1.370828
and coroutines 1.110488 lose all six. Against upstream, logging_silent 0.804557
wins all six, while DeltaBlue 1.123299 loses all six. Required logging parity
against both controls holds. The DeltaBlue improvement against golden is clear;
an upstream gap remains. Frozen48 target repeats and full diagnostics remain
pending, so this review does not yet approve final deployment.

Conditional performance admission is supported under the coordinator's stated
policy that an increment should be mostly beneficial, subject to complete
frozen48 repeats and full release/debug diagnostics. The target DeltaBlue gain
and retained logging parity justify this focused canonical-fact change and
explicit semantic recovery boundary. Broad means are near flat and most
frozen48 screen case directions improve. The spectral and logging costs must
be reported rather than hidden behind the target gain. This is not a broad
parity or universal speedup claim. PMU evidence remains diagnostic, not a
requirement to invent a causal explanation or change the frozen source again.

The final six-pair frozen48 target campaign is complete with stable identity
and no failure/postflight error. DeltaBlue 0.927374 and nbody 0.970471 win every
pair; logging_silent 1.043134 loses every pair. Use these repeated results for
the final target rundown: about 7.3% less DeltaBlue time and 3.0% less nbody time,
with 4.3% more logging time against the previous checkpoint. Performance review
supports the frozen candidate with these explicit tradeoffs. Full release/debug
diagnostics remain the outstanding integration gate.

## Final/A PMU diagnostic

The coordinator executed the prepared wrapper after fixing a Python local-name
collision before any interval. The final retained script and raw evidence are
in `final50-canonicalA-stat`. All 12 intervals are valid, event running is 100%,
identities are stable, and there is no failure or postflight error.

| Case | Instructions ratio | Cycles ratio | Elapsed ratio |
| --- | ---: | ---: | ---: |
| logging_silent | 1.000000052 | 1.085878 | 1.074645 |
| DeltaBlue | 0.999984765 | 0.999888 | 0.999650 |

These are candidate/A ratios of counts normalized per benchmark loop. Logging
executes effectively the same number of instructions while requiring more
cycles and elapsed time. Added dynamic instruction count does not explain its
measured loss. Placement or other cycle-efficiency effects remain possible,
unproven explanations; this measurement neither localizes stalls nor proves a
particular layout mechanism. DeltaBlue remains effectively unchanged against A.

The supplied evaluator-entry disassembly confirms a real initialization-code
difference: final zeroes an XMM register and stores a 128-bit recipe, replacing
the old scalar ordinal store and shifting nearby stores. Both reserve the same
0xc0 bytes of stack. This source/native difference is not a demonstrated cause
of the logging loss, especially given the unchanged measured instruction count.
Do not change the frozen candidate based on that causal guess. Lazy inactive
recipe state remains a sound separate simplification, without a speed promise.

## Recommended next work

1. Rank the remaining DeltaBlue/upstream gap using final counters and profiles
   before proposing another cache or fused execution path.
2. Extend validated canonical/ownership facts where measured recurring
   conversions remain hot, using reusable proofs instead of opcode exceptions.
3. Investigate spectral and coroutine regressions in a separate matched lane,
   preserving their ownership and observer contracts.
4. Diagnose logging's cycle-efficiency loss with controlled comparisons;
   unchanged instruction count does not justify padding or an alignment fix.
5. Continue separating semantic control/recovery identities from optional source
   attribution. Broader compiler line restrictions require observer/state proofs;
   lazy inactive recipe state is a separate safe cleanup without a speed claim.

Rejected C does not establish direct method-call fusion as a future win.

## Final upstream profile and review disposition

The final upstream PMU campaign passes all 12 stat and four record intervals,
with stable identity, 100% counter running and zero lost samples. DeltaBlue
uses 1.353105 times the instructions and 1.129640 times the cycles per loop;
logging uses 0.957476 times the instructions and 0.799730 times the cycles.
Thus DeltaBlue retains a substantial instruction-budget gap despite its
checkpoint improvement; logging retains clear upstream headroom.

Candidate DeltaBlue cycle samples place exact CALL at 13.39%, shape local-temp
load at 9.28%, canonical method load at 7.69%, frame clear at 5.68%, shape load
at 4.72%, return at 4.40% and frame pop at 1.99%. These identify where to inspect
next, not removable fractions or predicted gains. Prioritize exact activation
and teardown, then recurring shape/ownership work, using generated proof reuse
where supported. A new lookaside cache or direct fusion is not established by
these samples. The rejected C experiment is evidence against assuming that
less publication or smaller native code necessarily wins.

**Selected source, focused correctness, native and performance review passes
with the explicit regressions above.** No source blocker remains. Full merged
release/debug diagnostics are still running and remain the deployment gate.
No broad upstream parity or causal explanation for logging's A-relative cost
is claimed. Frozen A may be retired after its identities, native artifacts and
all completed matched results are retained; no additional A probe is needed.

## Completed full diagnostics and final admission

Reviewed both final journals and completed full logs at clean selected identity
`bf810ec1a33`; both commands return 2 and both results are **FAILURE**, not green
suite passes. Release runs 54,708 tests: 16 GDB pretty-print failures and nine
sys_settrace errors, with exactly those two failing modules. Debug runs 54,708:
21 GDB pretty-print failures, one lltrace failure and nine sys_settrace errors,
with exactly those three failing modules. Release/debug skip counts are
2,570/2,502. The journal's native preflight confirms the selected interpreter,
source library and extension paths for each build.

Compared with retained checkpoint48 full logs, the failing module groups,
per-group failure/error counts and skip counts match; the selected run executes
11 additional tests. There are no additional failing modules. Quiet logs do
not enumerate all individual GDB and sys_settrace case identities, so this
review establishes group/count correspondence, not independently inspected
case-by-case equivalence. The diagnostic failure state remains explicit.

**Admit checkpoint50 under the project's baseline-diagnostic policy.** Source,
focused checks, native evidence, matched performance and completed full-suite
diagnostic comparison support admission with the documented logging/spectral
tradeoffs and remaining baseline failures. No new review blocker remains.
This disposition supersedes the earlier pending full-diagnostic gate; it does
not claim an all-green suite, broad upstream parity or proven regression cause.
