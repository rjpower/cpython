# Recipient NONE: independent handwritten implementation review

Source GO for the corrected seven-file diff in `frame54-recipient-default`,
based on measured R `612fb4e9d170532947c007f0ccaaa04aff623329`.
No generated/native/runtime result is claimed. This reviewer ran no runtime,
compiler, generator, inferior or test. The final `git diff --check` is clean.

The actual sentinel change matches the proposal: ordinary initialization and
the thread base frame store UINT16_MAX. IsValid still rejects this non-ready
state; Index and NeedsReplace retain strict descriptor assertions. IsDiscard
permits known NONE in its debug precondition while returning false through
its unchanged release equality. SetReturnValue asserts a ready descriptor
after redirect completion. No release guard, numeric masking of NONE, generic
fallback or handler/emitter change is added. Real producer, native entry and
trampoline recipient setters remain unchanged. Remote asyncio's existing
validation rejects NONE before decoding; the pending debugger gate rejects
NONE through its discard test without an owner-based workaround.

The C API conversion now sets a real discard first-input contract under the
frame lock before R's existing two-argument Copy. It acquires an owning frame
object reference, sets GENERATOR ownership and initializes weakrefs,
exception linkage, names, origin/finalizer and async flags before associating
f with the destination and tracking the generator. The stolen input reference
is released after unlock. The old FRAME_OBJECT object's direct f_locals owner
moves with its frame; no locals-sidecar or universal copy API change is ported.
Coroutine origin is computed before the ownership move; failure directly
frees the untracked empty generator allocation and consumes the input frame.
The initial allocation failure likewise consumes f.

Independent review found a blocker in the first diff: unchanged PyCoro_New
recomputed and overwrote the newly stored origin after publication, leaking
the first owner and restoring a fallible postpublication path. PyAsyncGen_New
also retained redundant later flag writes. The author corrected both wrappers
to direct generic-helper returns. The reviewed final diff therefore has one
subtype initialization authority before publication and no second origin
allocation. No blocker remains in this source review.

The test helper constructs PyFrame_New then selects the public generator,
coroutine or async-generator constructor from code flags, preserving their
stolen-frame contract. Added tests cover a fresh frame's clear, generator
wrong-first-send rejection followed by send(None), yield and completion/close,
retained frame identity and escaped-frame cleanup, coroutine origin disabled/
enabled and first input, and async-generator first-iteration hooks/close flags.
The debugger regression uses sorted's key callback on the target's first
native call, checks the ancestor redirect is rejected, and verifies normal
result delivery. It does not claim to redesign stale real-descriptor provenance.

Root's qualification must run the focused tests plus existing recipient,
redirect, generator/SEND/yield-from, native provider and remote-reader tests
in release and debug. The new handwritten tests do not inject allocation or
coroutine-origin failure; exercise those paths separately before claiming
failure-path qualification. Actual native comparison must confirm constant
fresh-frame setup, unchanged real producer classification and retained entry/
trampoline descriptors. No frame-size reduction or throughput improvement is
inferred from this source change; preserve original R evidence separately.
