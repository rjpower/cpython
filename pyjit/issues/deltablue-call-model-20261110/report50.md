# DeltaBlue round 50: selected checkpoint

The selected and admitted round 50 checkpoint is `bf810ec1a33`: A50 canonical
method capture plus explicit source-observer error recovery. Against golden39,
six-pair DeltaBlue elapsed time is 31.4% lower; it is 7.3% lower than frozen48 but 12.3%
higher than upstream. Silent logging is 10.5% lower than golden39 and 19.5%
lower than upstream, but 4.3% higher than frozen48 and 7.4% higher than A50.
Admission retains the diagnostic limits and performance tradeoffs below.
The grouped [proposal](proposal50.md) received [Astra review](astra-proposal50-review.md)
before implementation. The adjacent method/CALL experiment and its bounded
direct-overlap follow-up were rejected on performance and retained as evidence.

The 29-kernel, two-pair geometric mean is 0.996588 against golden39, 1.177045
against upstream and 0.997971 against frozen48. The nearly flat golden39 mean
masks losses: six-pair spectral norm is 1.370828 and coroutines 1.110488 versus
golden39; spectral norm is 1.106127 versus frozen48 in the two-pair screen.
These include inherited and incremental regressions and warrant follow-up rather
than an overall win claim. The [full comparisons](benchmark-tables50.md) show
all kernels and target confirmations.

## Qualification

Selected release, debug and stats builds each check 117 modules. Focused
release/debug checks pass 804 tests each, stats smoke passes 150, and the strict
value API gate passes 306 units. Both full diagnostic runs executed 54,708 tests
and exited with failures. Release reported 16 GDB display failures and nine
`sys_settrace` errors across two failed modules. Debug reported 21 GDB display
failures, one LLTrace failure and nine `sys_settrace` errors across three failed
modules. These match the preceding checkpoint's failure groups and counts, with
no additional failed modules; quiet logs do not establish individual case
identity. The suites are not green.

Evidence is under `BASE/pyjit-link-wave1/deltablue-call-model-20261110`, where
BASE is `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build`.
The full logs are `delta50final-rfull-release-full.log` and
`delta50final-dfull-debug-full.log`. The independent
[final review](final-review50.md) supports admission with the stated tradeoffs.
Benchmark controls are golden39 `4d666e1b482`, upstream `3a440b65695`, and
frozen48 `5559f60093e`; the unchanged kernels run with GCC 15.2, CPU 15, hash 0
and JIT disabled. Full screens use two balanced pairs and target confirmations
use six.

## Final profile and next work

Against upstream, selected DeltaBlue executes 1.353105 times the instructions
and 1.129640 times the user cycles per iteration; silent logging uses 0.957476
and 0.799730. DeltaBlue's instruction count fell from 55.34 million in the
earlier selected48 capture to 52.67 million here, about 4.8% fewer. These are
separate diagnostic campaigns, not a causal decomposition of elapsed time.
Four final cycle captures had stable identities and no lost samples.

Selected DeltaBlue samples place exact CALL at 13.39%, shape-local load 9.28%,
method load 7.69%, frame clear 5.68%, shape load 4.72%, return 4.40% and frame
pop 1.99%. These locate work to inspect; they are not removable fractions or
speedup estimates. In the separate final/A diagnostic, silent logging executes
effectively the same instructions (ratio 1.000000052) but takes 1.085878 times
the cycles and 1.074645 times elapsed time. More executed instructions do not
explain the measured loss; the cause of the cycle difference remains unproved.
The [tables](benchmark-tables50.md) give the normalized counts.

The evidence suggests five next actions:

1. Investigate the remaining DeltaBlue instruction gap, starting with ordinary
   exact activation and teardown.
2. Carry reusable representation and ownership proofs through recurring hot
   conversions where measurement shows avoidable work.
3. Analyze spectral norm and coroutine losses with matched controls.
4. Investigate logging's cycle efficiency with controlled measurements, without
   assuming a layout cause or tuning padding from the current evidence.
5. Separate semantic recovery and observer state from source attribution in
   lowering, with explicit reconstruction before removing compiler line gates.
   Lazy inactive recipe state is a separate possible cleanup with no speed claim.

## Initial runtime48 profile

Matched PMU captures of unchanged DeltaBlue and logging compare selected48 to
upstream with GCC 15.2, CPU 15, hash seed 0 and JIT disabled. Three counter
intervals per role show DeltaBlue at 1.42137 times upstream's native instructions
and 1.17719 times its user cycles per iteration. Logging is at 1.03636 and
0.74143 respectively. These diagnostic captures identify excess work; their
elapsed times are not balanced promotion measurements. The [tables](benchmark-tables50.md)
retain the values, identities and evidence locations.

In selected48's sampled DeltaBlue cycles, exact CALL accounts for 13.98%,
shape-local 10.70%, method lookup 7.83%, shape 5.50%, RETURN 4.32%, frame clear
4.21% and frame pop 2.30%. These are sampled locations, not additive removable
costs. Upstream's large evaluator symbol prevents a direct symbol-by-symbol
comparison. The [static audit](static-path-audit50.md) identifies 43 adjacent
zero-argument method/CALL pairs among 180 CALL sites; this is static opportunity,
not a measured hit count.

## Component evidence and decision

A50 preserves canonical descriptor and receiver facts through guarded method
capture and publication. Existing lookup guards and owner transfers remain.
The [component review](A50-peer-review.md) finds that generated release code
retains those owners while removing numeric recanonicalization from all three
method variants. Handler sizes change from 741/720/710 to 249/244/249 bytes;
the ordinary exact CALL remains 1,410 bytes. This is evidence of the mechanism,
not a workload speedup.

Release and debug builds each checked 117 modules; regeneration and the
306-unit value API gate passed. Release and debug focused checks each passed
1,405 tests, with 10 and 8 skips respectively. Source post-review approved A.
Two balanced pairs against frozen48 show elapsed ratios of 0.922656 for
DeltaBlue, 0.976941 for logging and 0.971432 for nbody. These component results
are not a comparison to upstream and separate campaigns must not be multiplied
to infer one. These are A50 component results, not final checkpoint results.

The provisional two-pair 29-kernel screen of frozen A against golden39 has a
geometric mean of 0.998194. DeltaBlue is 0.680512 and silent logging 0.827462,
while spectral norm is 1.283569, coroutines 1.088057, simple logging 1.054447
and logging format 1.050384. The near-flat mean conceals substantial regressions
alongside improvements and does not establish an overall win. The
[full table](benchmark-tables50.md#a50-against-golden39-all-29-kernels) is for
frozen A alone, before the selected cold recovery change.

C carries the captured pair into an ordinary exact Python activation while
retaining both real instruction words and positions. The positions attribute
lookup and call errors and observation separately; equality or a same-line
test is not an admission condition. Control-flow, exception and observer
requirements are checked directly. A post-capture miss publishes the captured
owners and resumes CALL without repeating lookup; success shares ordinary frame
entry and return. Native review found that the success path omits function-home
publication and reload while retaining owner transfers, code ownership and
frame initialization. A stats run counted 42,710 refined entries and 40,629
committed activations per DeltaBlue iteration after warmup; this is coverage,
not timing.

Both release and debug focused runs passed 1,939 tests (19 and 17 skips), and
the 117-module stats build passed its smoke run. In clean six-pair tests, C/48
elapsed ratios were 0.964298 for DeltaBlue, 0.949135 for logging and 1.011550
for nbody. The direct C/A comparison was 1.043648, 0.968599 and 1.038921;
all six DeltaBlue and nbody pairs were slower with C. The original C/48 run
overlapped debug testing and is excluded from admission. The
[tables](benchmark-tables50.md) provide the clean results.

The final direct-overlap follow-up `f267e9209cf` reduced the refined handler
from 1,396 to 1,052 bytes, but direct six-pair C/A ratios were 1.028054 for
DeltaBlue, 1.140367 for logging and 1.008522 for nbody. Five of six DeltaBlue
and nbody pairs and all logging pairs were slower. C was rejected; neither its
refinement nor its supporting generator changes appear in the selected source.

The [source metadata audit](debug-metadata-audit50.md) finds no line or column
lookup in ordinary dispatch, C admission or frame activation. Selected source
copies an explicit semantic recovery recipe from a failing observer into its
activation, and the unwinder consumes it without a source-map decode. The frame
display ordinal is separate. Compiler line restrictions and the mixed serialized
observer container remain limitations; removing coordinate decoding from the
ordinary hot path would remove no demonstrated work.
