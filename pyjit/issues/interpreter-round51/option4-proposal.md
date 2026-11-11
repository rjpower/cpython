# Option 4: prove the existing inline ADD/SUB result join

Status: proposed; source unchanged. Runtime `BASE/worktrees/round51-option4`
starts at bf810. Proposal review precedes edits; implementation timebox ends
23:35 UTC, 2026-11-11. Coordinator owns regeneration, builds and measurement.

Selected release ADD_INT is 0x33e bytes at 0x35e110 and SUBTRACT_INT is 0x33e
at 0x35da80. ADD native includes `_PyValue_Canonicalize` result-publication
arms although both existing producer success paths are canonical. The guarded
inline-input closes already disappear; no savings are claimed for them.

Use Astra's bounded whole-producer proof alternative, preserving the existing
arithmetic and value API unchanged. Add one recognizer alongside the existing
canonical list-child proof, called only after canonical_result_source has
checked scalar destination geometry and excluded named feedback bindings and
unsupported parts. It recognizes exactly two inline-int guards, the ADD or SUB
producer, and two unchanged integer-pop suffixes. Match input/output shapes,
complete original source tokens and the emitted AST through
canonical_body_tokens; ignore opcode names. Skip padding remains inert.

The producer match includes scaled inputs, the exact add/sub overflow builtin,
the matching unscaled expression passed to `_PyValue_BoxInt64` on overflow,
`_PyValue_FromIntScaled` on success, NULL error propagation, input forwarding,
and INPUTS_DEAD. Inline operands are signed int51: their sum/difference always
fits int64. Overflow of the scaled int64 operation proves the unscaled result
is outside the inline range, satisfying the box helper's essential precondition.
Its success is therefore canonical; its NULL remains an allocation error.
The other arm's exact scaled constructor is canonical. This proof grants only
the final result owner to existing canonical local/temp/discard publication.
It does not bless arbitrary `_PyValue_BoxInt64` calls or changed producers.

No new value encoding, helper interface, SSA/type framework, arithmetic opcode,
multiply/divide scope or input cleanup policy. Encoding remains inside the
existing value APIs; arithmetic source remains identical. Recognizing complete
bodies avoids an unnecessary extra helper boundary and directly binds the
BoxInt64 precondition to the accepted overflow predicate and operands.

Files: `Tools/cases_generator/ir_generator.py`,
`Lib/test/test_generated_cases.py`, and focused behavior cases in
`Lib/test/test_pyvalue.py`; coordinator regenerates generated products.
Existing numeric proof tests currently expect ADD/SUB to be unproved; update
those affected expectations and add renamed-handler positive coverage.

Negative generator cases must reject changed guards, overflow builtin/operator,
box arguments, scaled constructor argument, missing NULL handling, result
reassignment/escape, inserted callback, altered input retirement, named helper
bindings, preprocessing and changed AST. Runtime cases exercise both inline
endpoints and both overflow directions, negatives, zero, local/temp/discard
recipients, subclass/mixed fallback and replacing a mortal local whose finalizer
reenters Python. Allocation failure is included if the existing fixture can
reliably target the boxed-result allocation without introducing a test API.

Root runs test_generated_cases and focused test_pyvalue/arithmetic behavior
checks on candidate release/debug. Native success is removal of result
normalization from ADD/SUB local and temporary success paths while preserving
scaled overflow, boxing errors and displaced-owner retirement. Root measures
spectral_norm and coroutines, with richards/DeltaBlue and logging guards. Smaller
source or symbols alone are not admission; no performance claim precedes native
inspection and matched timing.
