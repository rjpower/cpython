# Canonical shape result: implementation review

2026-11-07. **Accept the source and focused native checks for R5 commit
`0953757f4e7eb442896dcf42f6547009528defaa` on parent `b58036e9718`, including
the corrected callback-test oracle. No remaining must-fix source finding.**
GCC and performance admission remain separate. Runtime source was read-only
during this review.

The producer is the reviewed RHS expression,
`attr = IR_CANONICAL_HOME_DUP(found)`. The existing publication checker requires
that exact direct assignment with one identifier argument, its matching
result-retirement marker and the existing single-output/assignment/suffix
rules. Its frozen declaration record carries the canonical-source fact.
There is no handler-name list or mutable stack fact. The intrinsic is registered
as a nonescaping function and an in-place reader, like the duplicate it
replaces.

`Storage.prepare_flush()` factors the existing print, dead-input clearing and
defined-output preparation without changing their order. Ordinary flush uses
it, while the checked marker passes its one result name to `IRStack.flush()`.
Only that result's first temporary flush selects the direct store. The macro
keeps the temporary's NULL/same-value assertion and debug home check; a local
is untouched there. Final local publication still calls `_PyValue_Publish`.
Retirement, local-only failed-result cleanup, refresh/error exits and the
companion output retain their existing code.

Independent in-memory generation from old and new bytecode sources using the
final worker generator gives identical formats and handler contracts. Apart
from the source-path comment, the complete generated interpreter diff is two
lines in `LOAD_ATTR_SHAPE`: ordinary duplication becomes the asserted intrinsic,
and its first temporary store becomes the canonical direct store. No other
handler body or effect/boxing summary changes.

The canonical-home premise has a concrete supported-writer boundary.
`_PyObject_StoreInstanceValue` now documents its canonical borrowed input and
asserts `_PyValue_IsHome(value)` before duplication, callbacks or stores.
The predicate accepts NULL deletion; the assertion is fatal in debug even
when configurable home reporting is enabled. The object adapter and public
generic setter adapters canonicalize through `PyValue_FromObjectBorrow` or
`_Py_VALUE_ARG`; the native setter thunk does the same, including the decimal
override's route to the generic value setter. The internal store duplicates
that input into the inline home. Specialized shape stores transfer values
from canonical frame homes. Inline initialization writes NULL. Copies preserve
bits into detached storage; materialization, clearing and detachment invalidate
the shape, and specialization declines `_Py_SHAPE_NONE`. The shape read is
guarded before it duplicates the home value. These are manual source premises,
not a C pointer-provenance proof supplied by the generator. Rebuilt external
callers must satisfy the newly explicit internal API precondition.

The value-category test covers temporary/local destinations, actual shape
specialization, inline integer/float, signed zero, NaN, large integer, number
subclasses, static immortal and mortal object. Existing receiver-retirement
coverage remains relevant. The new folded-local mutation test observes the
original local, writes a sentinel from the receiver callback, and checks
sentinel retirement plus survival of the returned payload.

The first semantic precheck failed on unchanged R4 because the test expected
the original local's destructor before the sentinel's. Inspection found the
explicit owner in `framelocalsproxy_setitem`: `add_overwritten_fast_local`
retains the overwritten object in `f_overwritten_fast_locals` until the frame
dies, protecting outstanding borrows. The corrected order is receiver,
sentinel, then original local. The retained `callback-locals-oracle.json`
records the same order and live result on native upstream `3a440b65695` and
R4 `b58036e9718`; this was a test-oracle correction, not a runtime failure.

The final isolated generator passed 75 tests, and both corrected semantic tests
passed on unchanged R4 before integration. I inspected `r5-checks.json` and the
actual logs: every command returned zero; release/debug builds each checked
117 modules with no missing modules or import failures; both profiles passed
630 tests with one skip across `test_generated_cases`, `test_ir_contract`,
`test_ir`, `test_opcache`, `test_dict` and `test_descr`. Value-API lint covered
306 units with zero failing findings and zero parse errors. The nine reviewed
worker files match the integrated source; its generated tenth file contains
exactly the two-line shape change above. Metadata and targets are unchanged.
The failed oracle precheck remains separate evidence.

The committed runtime worktree is clean. `r5-identities.json` binds native Git
identity to `0953757f4e7` and records release/debug ELF build IDs
`9d98a70694382c4650742915f0560f12346c85bf` and
`842535c720c4e7b0a8d52044e4865d3474294756`. Evidence is under the established
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/`
root. Full coherent-batch qualification and GCC/paired measurements remain
pending at this review boundary. No speed claim follows from these checks.
