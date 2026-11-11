# Option1 source and native post-review

Source reviewed through final `22f4ce1508e` (base change `e97fb8d0c8d`) against
bf810. No source/ownership or remaining release-focus blocker found. Debug
qualification and performance admission remain coordinator gates.

The nomination helper checks positive full-width basicsize before narrowing;
instance slot displacement is calculated in uint64 and checked independently.
Both eligible nomination paths populate values_offset. Generated instance,
shape/store and WITH_VALUES guards retain the exact nonzero type-version check
before address use. Atomic validity reads and existing lock/shape semantics
remain. The offset helper adds no release-mode callback or metadata lookup.

ATTR48/payload16/poly120 assertions remain. Snapshot rows copy offset only for
eligible handlers, then separately retain guarded method/class values; shape-add
preserves both scalar fields. Whole-row copying remains typed and no pointer
payload is interpreted as an offset. Repository source search found no additional
payload reader needing scalar reconstruction. The metadata-generator fix filters
zero-unit feedback fields out of physical operand expansion while retaining
their typed generator bindings; actual C generation compiled in release.

Reviewed root's `E/native/option1-shape-local.txt`. The release shape/local/temp
handler compares receiver type version at 0x3670e7, loads values_offset from
feedback at 0x3670f0 and reads shape through receiver+offset at 0x3670f4. It does
not reload tp_basicsize after the guard. This establishes the intended native
removal for this supplied handler, not every changed handler or a timing win.

Executed evidence inspected read-only: `E/option1-fix-release-build.log` reports
117 modules with zero missing/import failures. The focus journal identifies executable
and source `8b056c4ca5b`, expected magic 64418 and the correct source Lib.
`test_opcache` and `test_ir_attr_cache` passed. `test_generated_cases` failed with
one error and one failure. The separately verbose metadata fixture log identifies
an invalid fixture cache-declaration comma; owner is fixing it. The other failure
is not diagnosed in the quiet full-module log. These failures are unresolved
qualification blockers, not approved skips or evidence of a passing generator.

Final repair review: `22f4ce1508e` counts nonzero physical caches consistently
when deciding uop viability, matching expansion-table operand filtering. It
tests that two physical operands plus a zero-width field remain viable while
three physical operands do not. The fixture retains ordinary syntax; it does
not bypass viability with a replacement annotation. Removing the two dormant
object-reader names matches the final bytecodes callsites. No C handler body
changed in this repair.

Inspected `E/option1-repaired-release-focus.log`: both formerly failing cases
pass under executable revision `22f4ce1508e`. Root reports the other 95 generator
cases already passed and reuses that unchanged evidence. The final release build
again reports 117 modules with zero missing/import failures; source is clean.
The initial failures remain in their original logs. Final disposition: approve
source, demonstrated native removal and completed release focus for continued
measurement. Root must still complete planned debug qualification and matched
performance admission. This review does not call the candidate integrated or
faster. This reviewer launched no native execution, builds, regeneration or
timings.
