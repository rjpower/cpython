# DeltaBlue round 50: profile and candidate

Runtime48 `5559f60093e` remains selected. Candidate A50 `e014965013b` is
committed, reviewed and measured against frozen48; batch admission is pending.
The grouped [proposal](proposal50.md) received [Astra review](astra-proposal50-review.md)
before implementation. The adjacent method/CALL design has [component approval](C50-peer-review.md)
and implementation has started; it is not yet complete or measured.

## Fresh profile

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

## Candidate and next decision

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
to infer one. Qualification and selection remain coordinator decisions.

C would carry the captured pair into the ordinary activation while retaining
both real instruction positions. The positions attribute lookup and call errors
and observation separately; equality or a same-line test is not an admission
condition. Control-flow, exception and observer requirements must be checked
directly. A captured-target fallback must resume CALL without repeating lookup.
C has no completed implementation or speedup claim yet.
