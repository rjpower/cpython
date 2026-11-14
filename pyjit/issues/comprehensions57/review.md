# Final review: selected57 dictionary readers

**GO** for `0dcba75bb3df40c32137db453b001f7e8f101de0`, selected on
`pyjit/round57-golden`, tag `pyjit-golden-20261114-dictionary-readers`.
The lifetime repair and shared provider cleanup justify selection with explicit
performance tradeoffs. No source, qualification or evidence blocker remains.
No comprehension improvement or parity claim is supported.

The reviewed comparison base is immutable golden56,
`883524c180e4486b47df77ee2276c9276c0ec32c`; the actual Git parent is
`015e2e3bcc958e117fb3f13da449234ab6235f5d`. The complete comparison changes
only `Objects/dictobject.c` and `Lib/test/test_pyvalue_dict.py`. Method entry,
list iteration, append, generator, frame and IR changes are excluded.

## Source and qualification

A cached exact-string hash excludes hashing callbacks but not stored-key
Python equality in a GENERAL dictionary table. GetValue and ContainsValue
now share admission requiring a cached exact string and Unicode-only table
under the GIL. Other paths retain dictionary/query owners across callbacks.
Free-threaded code avoids a new unlocked table-kind read; this is conservative
source behavior, not free-threaded qualification.

One helper performs lookup and acquires the found result before input cleanup.
Its caller supplies dictionary/query lifetime; the general provider retains
those owners while avoiding nested query-pin work. Typed `dict.get` enters
below general API validation. Cleanup can reenter without invalidating the
returned owner. Ordinary borrowed-input callers still owe valid lifetimes;
the internal borrowed-holder regression does not establish a public-extension
vulnerability.

Both release/debug builds complete 117 modules, including 79 shared extensions,
with zero failed imports. All 203 methods across three dictionary modules pass
per role. The 28 focused reader methods also pass and overlap that coverage.
Tests exercise mixed-table equality retirement, mutation/restart, reentry,
result/default ownership, subclass behavior and hash/equality errors.

This focused qualification is proportionate to the two-file successor with no
generated protocol changes. Selected56 supplies the inherited coherent
full-system checkpoint; no fresh full-suite run is claimed for selected57.
The rejected composition's 55,045-test runs are separate evidence, retaining
six baseline debugger errors and a subsequently corrected release lint finding.
Only the existing GDB PrettyPrintTests quarantine applies; qualification is
GIL-only. Exact qualification heads and logs remain in the [report](report.md).

## Measurements and accepted tradeoffs

Final all-29 campaigns retain 116 commands per control; the separate six-pair
nine-case confirmation retains 108. Candidate build ID is
`89fb8d381b59926c48c8d579b4c5b5b413c6732b` throughout. Before/after identity
files are byte-identical. The retained native SHA256 is
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.

All-29 candidate/control ratios are .995586/golden56 (16 wins, 13 losses) and
1.127137/upstream (six wins, 23 losses). These two-pair observed aggregates
are not statistically established speedups. Upstream XML iterparse has opposing
paired ratios .685998–1.905215, so its aggregate is not a stable precise parity-gap
estimate. All observations remain; campaigns are neither pooled nor multiplied.

| Case | Six-pair /golden56 | Observed range / implication |
| --- | ---: | --- |
| Comprehensions | .997484 | .974490–1.011757; mixed |
| DeltaBlue | 1.000239 | .994835–1.010206; mixed |
| Silent logging | .992049 | .946656–1.028672; mixed |
| Richards | .991167 | All six improve |
| Spectral norm | .931732 | All six improve |
| Nqueens | .987729 | All six improve |
| Coroutines | 1.019635 | 1.015298–1.028476; all six lose |
| Raytrace | 1.020117 | 1.012356–1.027468; all six lose |
| Generators | 1.024797 | .992243–1.119177; wide, mixed |

Root explicitly accepts coroutine/raytrace losses and generator uncertainty as
performance debt. Generator confirmation includes large candidate samples;
all-29 separately loses both pairs at 1.014391. Neither dropping those samples
nor claiming a uniform 2.48% slowdown is warranted. The small broad benefit
does not erase these losses; selection rests on correctness and provider cleanup.

Final upstream PMU has 18/18 valid invocations with stable identities. Matched
three-pair instruction/cycle ratios are DeltaBlue 1.361424/1.118423,
comprehensions 1.436416/1.333179 and coroutines 1.307831/1.267788. These establish
remaining native-work gaps, not speed causation or a cache-specific explanation.

The four-lane bundle, alignment/local-append trials and minimal lifetime-only
72ee variant remain rejected; their source and evidence are preserved. The
[report](report.md) and [tables](benchmark-tables.md) retain their results and
final measurement limits. Canonical status, handoff and contracts agree with
this selection. The full review history remains in E57/cold-selection-final.md;
this final review consolidates that completed work without new native execution.
