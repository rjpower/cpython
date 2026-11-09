# G0 word-capture helper review

2026-11-01. **ACCEPT the original frozen helpers plus the narrow launch
metadata errata for root's separate activation.** No target capture, native
entry, benchmark, compiler or JIT implementation ran in this review.

Directory: `build/jit-first-native-tier-word-capture-20261101`. Exact original
capsule SHA256 `8a0bf0875ae162ef25e860ecefe41881ea81d6a515791ede5a212f33d5c4618e`,
plan `96ae8b02944893e9bdd631529bca74e743cba22039548d3ad018baf987ac31a2`,
launch `eebddc6ede3a171b63c81d066db03291890c23be98c7f1afb14bad23a9a0b2ce`,
ready `0838c9b49392f5fe390da7a4e901682fe3b5be3367efd5db6d3253e52782722a`.
Source is the clean corrected RC `dce654018d5ae16ffe5e772976d9ccc2c9d1e01a`,
using the explicitly bridged historical release image, not a newly built one.

The original launch has two stale descriptive fields: it names the parent
full-suite runner path and describes the parent's longer clock. This review
found and reported them before activation. Original bytes remain unchanged.
The additive `launch-metadata-errata-v2.json`, SHA256
`a2a499ae86beb0dd71b21003714e7393feb9b14ea45f260abbea32c849a4ebbe`,
corrects only that metadata. Root activation must pin this errata and launch
the exact G0 `run_batch.py`, SHA256
`ec5dfdfc07b51272a18f0f9ab5edfc79a3f58b41bfc1e152df205cd5cfd79522`.
Actual authority/runner enforce 300 seconds inclusive; the single declared
target row is bounded to 60 seconds, leaving the existing 210-second reserve.
Those two descriptive fields are not consumed by runtime control flow. The
literal target/gate/lock/verification argv and all other metadata remain exact.

Independent replay checked 7,208 inputs, 22 ready artifacts, actual
source/image/configuration/tool-byte/import/output/idle preflight, and all
eight host controls. All passed. Source inspection confirms positional
`get_ir(code, adaptive)` calls, the full sixteen-bit handler and four twelve-bit
field layout, complete word-index bounds, and the five exported pristine-word
accessors with `(py_object, c_int) -> c_int` PyDLL signatures. The GIL remains
held. `FoldedInputs` returns a count; the helper reconstructs input/local slots
from source-pinned borrow masks and actual generated roles, then crosschecks
that count. It does not relabel the count as a mask or executing-word fact.

The actual registered kernel is loaded under a non-main name. Exactly 64
`eval_A` calls cover every ordered pair 0 through 7 once and consume a finite
positive checksum. Neither registered `run()` nor the benchmark loop is
called. Before/after evidence includes raw pristine/executing words, IR and
dis, generated formats, original fields, feedback, code/local/frame/source/
position/exception metadata and actual origins. `_opcode` is correctly bound
as a builtin to the interpreter image; own CAPI, ctypes/_ctypes, dis and other
required origins are hashed. Observer flags, callback absence, tagged-value
policy and source/config identities are checked at both boundaries.

The accepted supervisor/gate preserves raw streams, shared serialization,
both machine slots, own release lock, dual-executable idle checks, no retry
and terminal verification. This script creates no venv alias or nested target.
No blocking finding remains after the explicitly bound metadata correction.
Actual captured words may inform a later prefix decision; this observation
proves no native admission, execution frequency, per-PC hotness, speed or JIT
correctness. Those obligations remain in the separate implementation plan.

Reviewer evidence is
`build/jit-word-capture-helper-review-20261101-v1/{replay.py,replay.log,receipt.json,errata-receipt.json}`.
The original receipt preserves the initial metadata hold; the supplemental
receipt records its exact disposition without rewriting that history.
