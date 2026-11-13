# Recipient no-default proposal: independent peer review

GO for an isolated implementation of the revised NONE proposal, subject to
root qualification. This reviews the reserved UINT16_MAX state, not the
superseded discard-default draft. No runtime source, native job or test was
changed or executed for this review. Keep measured R at
`612fb4e9d170532947c007f0ccaaa04aff623329` immutable.

A fresh ordinary frame has no pending transfer, so a constant NONE state is
more precise than encoding invented slot zero. CALL/init/native transaction
producers install real descriptors before transferring control. Normal
generator creation installs RETURN_GENERATOR's destination before first
resume; YIELD installs the next-input recipient. Entry and init trampoline
recipients remain explicit real slots. Clear/unwind and moving a frame do
not publish into a default recipient. The base sentinel has no executable
publication contract and may use NONE without reading an execution prefix.

The descriptor API distinction is required. Keep IsValid ready-only and false
for NONE. IsDiscard must permit exactly NONE or a valid descriptor in its
debug precondition, and return false for NONE with the same release equality
test. Index and NeedsReplace must remain strict. SetReturnValue must assert a
ready descriptor after redirect completion and before discard/publication;
no release fallback, silent discard, new universal branch or numeric masking
of NONE is justified. Pending redirect completion precedes descriptor reread
as before. Ordinary result publication must never see the fresh state.

The debugger concern is concrete: the ancestor pending-call gate currently
requires actual discard, while native callback reentry can interpose an
INTERPRETER frame. NONE keeps the existing condition false on a fresh parent
without adding an owner-based workaround. A stale *real* prior descriptor is
an existing separate state; this proposal must not claim that NONE redesigns
all call-recipe provenance. The new first-native-call regression should
exercise rejection specifically before any producer has installed a recipient,
while existing supported discarded-Python-child redirects remain covered.
The proposal's last sentence mentioning a default that 'is now discard' is
stale wording; the revised default is NONE.

Remote asyncio currently validates IsValid before Index, so NONE is rejected
without changing offsets or accepting reserved bits. _PyGen_YieldFromRef's
strict ready/index assertions remain correct: only a suspended delegating
yield authorizes that receiver-window lookup, not CREATED. Copy/takeover,
traversal, clear and address reporting preserve or ignore the state rather
than decode it. Update struct comments accordingly.

The cold C API correction is necessary and separately bounded. PyFrame_New
creates FRAME_OBJECT ownership with frame_obj NULL; the existing generic
conversion copies NULL then asserts it equals the supplied object. Root's
baseline failure evidence establishes the existing bug. The conversion must
acquire a real owning reference to f, move its frame under f's critical
section, initialize every shared generator/coroutine/async-generator field,
set the destination owner/state and associate f only after coherent data,
then track/publish and release the stolen input reference after unlocking.
Coroutine-origin allocation must happen before moving ownership, and failure
must free the untracked empty allocation and consume f. Shared subtype fields
must not be initialized later by wrappers after publication. Retain R's
two-argument Copy and direct f_locals; no locals-sidecar or universal-copy
change belongs here.

The explicit C API first-input descriptor is discard even though ordinary
fresh frames now use NONE. This is supported by the constructed IR contract
requiring RETURN_GENERATOR's destination to be IR_NO_DST, and prevents first
send(None) from writing an invented slot zero. Test generator construction,
retained frame identity and escaped-frame teardown, both next/send(None),
completion/close, coroutine-origin success/failure, and async-generator hooks/
close state. Preserve wrong-first-send rejection and existing SEND/yield-from
coverage. Allocation/origin failures must not expose partial associations.

Root's source/native gates should prove constant ordinary initialization and
unchanged real producer/entry/trampoline classification. Focused release and
debug behavior must establish that no actual transfer reaches NONE, including
first calls, native reentry, constructors, redirects, generators and remote
inspection. These contracts do not establish throughput improvement or
admission; balance timing against both selected51 and the original R identity.
