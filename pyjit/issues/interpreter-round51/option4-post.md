# Option4 source and native post-review

Reviewed implementation `e3b904b1056` and final test-only revision `4bb16138d64`
against bf810. No source/ownership or release-focus blocker remains. Debug
qualification and performance admission remain coordinator gates.

The bounded proof matches both inline guards, exact ADD/SUB scaled operation,
corresponding unscaled overflow expression, scaled constructor, NULL branch,
owner forwarding and both original pop bodies. `canonical_body_tokens` binds
the original source delimiters to the emitted AST. New calls, preprocessing,
helper shadowing, changed guards, arithmetic arguments and retirement reject the
proof. The caller still requires the original scalar destination/input geometry,
no named feedback bindings and a fresh scalar result. The helper is independent
of opcode names and disjoint from option2's window proof.

Confirmed `_PyValue_BoxInt64` asserts an out-of-inline-range value and returns
either a fresh larger int owner or NULL. The unchanged inline operand guards and
scaled-overflow predicate establish that precondition; their unscaled sum or
difference fits int64. The nonoverflow constructor is canonical. Actual arithmetic
and value API code are unchanged. Generated C differs only in the two final
destination macros, preserving shared local/temp/discard and error/retirement
behavior.

Inspected `E/option4-release-build.log`: 117 modules, none missing or failing
import. Captured read-only disassembly into
`E/native/option4-review-add.txt` and `option4-review-subtract.txt`. Each handler
is 0x282 bytes, compared with the supplied baseline size 0x33e. Both retain their
scaled arithmetic/overflow jump, overflow `_PyValue_BoxInt64`, NULL error route,
and displaced-local finalization. Neither contains the generic result
canonicalization/type-inspection arms. This establishes native work removal,
not dynamic cost or a measured speedup.

The source tests include real/renamed positive proofs and meaningful negative
changes to predicates, box expressions, helpers, NULL checks, ownership, flushes
and the emitted AST. Behavior tests cover both inline endpoints and overflow
directions, local/temp/discard, subclass/mixed fallback, finalizer reentry and
existing allocation-failure injection. Initial tests warmed calls without
asserting ADD_INT/SUBTRACT_INT installation. The owner has added explicit get_ir
handler assertions after warmup in `4bb16138d64`.

Inspected `E/option4-codegen-release-focus.log`: four selected generator cases
pass, including renamed positives, mutated-source/AST negatives and numeric
constructor coverage, using implementation executable `e3b904b1056`. Final
`E/option4-specialized-release-focus.log` passes all 13 test_pyvalue cases. Its
journal confirms clean source and executable `4bb16138d64`, correct source Lib
and magic 64418; this run includes the new specialization assertions. The final
release build reports 117 modules with zero missing/import failures. The final
commit changes tests only, so the reviewed native implementation remains valid.

Final disposition: approve the source, native removal and completed release
focus for continued measurement. Root still owns debug checks and matched
target admission with the stated guards. No native execution, builds,
regeneration or timing was launched by this reviewer; this component approval
does not establish integration or a performance gain.
