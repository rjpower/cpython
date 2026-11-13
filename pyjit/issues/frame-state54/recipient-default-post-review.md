# Recipient NONE: independent source and native review

Source/native GO for the bounded R2 transformation, not performance admission.
The clean matching source is `fcc82220ddd42fc49ffd1885dcd79e6964889703` in
`BASE/worktrees/frame54-recipient-default`. Root regeneration produced no
handler-source diff. Both `recipient-default-initial-{release,debug}-build.log`
finish with117 modules checked and zero failed imports. The reviewer used
only source reads, nm/readelf/objdump and non-running GDB type inspection;
no inferior, build, generator or test was run.

| Executable | SHA256 | Build ID |
| --- | --- | --- |
| BASE/frame54-recipient-default-release/python |7191da167816b4cec0d8b8a666dfd1fd39bfcf99d134088e16634cf6ec168540 |bbff722e3731b8a88fa83b999e4eeedb17a4bff0 |
| BASE/frame54-recipient-default-debug/python |50770223d23e4e73dbaae31468d4a8f02253346df51a992821f263fd4aeb11a3 |fc6c975e9fd6019ed0b8f3b8a647441859ce42df |

## Fresh setup and retained real recipients

Release exact CALL starts `0x37a670`. Its ordinary child setup now packs
NONE=0xffff and pending_kwargs=0x0fff into one32-bit constant store:
`41 c7 43 f8 ff ff ff 0f` at `0x37a872`. Original R instead loaded child
co_nlocalsplus at `0x37aa02`, tested positivity, set/shl encoded slot-zero
classification, and stored the two defaults separately. That default
classification is actually absent from R2. Selected51 already used a packed
constant store at `0x37a862`, with zero rather than NONE for its low half;
R2 therefore restores that setup shape without claiming a reduction relative
to selected51's default initialization.

The real caller producer remains: R2 loads caller words at `0x37a8a6`,
extracts the destination, compares the prefix local boundary at `0x37a8c2`,
sets/shifts the replace bit and stores the real descriptor at `0x37a8d0`.
The return continuation store follows at `0x37a8d9`. This is retained R work,
not removed by NONE. No allocation or per-call arena protocol was changed.

The generic initialization helper similarly packs NONE/kwargs at `0x36d11b`.
Native evaluator entry keeps a real empty slot1 descriptor, packed with kwargs
as0x0fff0001 at `0xa7ab5`. The init trampoline remains distinct: in
CALL_ALLOC_AND_ENTER_INIT its actual supplied slot1 is compared against the
shim prefix boundary at `0x3819e5`, encoded, and stored at `0x381a00`.
Neither special entry inherits NONE. The thread base's explicit NONE state is
source-reviewed; no execution-prefix query was introduced there.

Both GIL ELFs retain header112 bytes and sizeof120, with homes at112.

| Release symbol | Selected51 | Original R | R2 |
| --- | --- | --- | --- |
| CALL_PY_EXACT_ARGS text |973 |1021 |989 |
| CALL_PY_EXACT_ARGS rsp local reserve |80 |80 |80 |
| RETURN_VALUE text / rsp reserve |546 /16 |547 /16 |547 /16 |
| YIELD_VALUE text |453 |501 |501 |
| RETURN_GENERATOR text |1002 |1070 |1070 |
| Generic init helper text |7484 |7516 |7484 |

These are text/local-stack facts, not instruction-frequency or elapsed-time
claims. The R2 exact-call body remains16 bytes larger than selected51 because
real producer encoding is retained.

## Publication has no new release predicate

RETURN_VALUE at `0x381410` tests actual discard through the existing equality
against0x0fff at `0x381497`, masks the numeric slot at `0x3814a4`, tests the
replace bit at `0x3814ae`, and directly stores an empty recipient at
`0x3814b7`. No comparison against0xffff, reserved-bit validation or NONE
fallback is added. The debug ready assertion compiles away in release.

The pending redirect call at `0x38150f` is followed by descriptor reload at
`0x381514` before discard/replace selection, retaining the winning-state
ordering. Real replacement still stores before retiring its displaced owner.
The strict source API keeps IsValid false for NONE and permits NONE only as
a non-discard input to IsDiscard; cold remote validation still rejects it.

## Cold C API conversion and tests

Actual release `gen_new_with_qualname` at `0x1709d0` is820 text bytes versus
original R's669. It adds subtype/origin handling and correct owning association;
this is cold added code, not part of the ordinary generated creation handler.
The subtype allocation is at `0x170a0f`. Enabled coroutine origin is computed
at `0x170c32`, before source recipient mutation or frame copy. Origin failure
calls GC_Del at `0x170c4a`, then consumes f on the common failure path.

The cold source recipient is explicitly discard at `0x170a3c`. Physical homes
move and clear at `0x170b00`–`0x170b1b`. The new owning f reference is acquired
before storing frame_obj at `0x170b35`; GENERATOR owner follows at
`0x170b3c`. Weakrefs, exception linkage, origin/finalizer and async flags are
initialized at `0x170b43`–`0x170b57`, followed by name owners. The association
is published at `0x170ba1`, then GC tracking occurs, then the stolen f owner
is released at `0x170be8` onward. The source critical section covers this
commit; these GIL ELFs do not establish free-threaded machine ordering.

PyCoro_New and PyAsyncGen_New are each25-byte tail wrappers to that helper,
versus144/54 bytes previously. Their actual tails at `0x170d64/0x170d84`
perform no duplicate origin allocation or later subtype mutation. The first
review's duplicate-wrapper blocker is corrected, as recorded in the source
review. Direct f_locals ownership moves with the copied frame.

The final committed tests additionally retain a weakrefable namespace through
C API generator move/completion and escaped frame, releasing it only after
frame.clear. Async-generator coverage now sends67 after the first yield and
observes67 before closing, as well as first-iteration hook behavior. These
additions are source-reviewed and align with the direct mapping owner and
suspension recipient contracts. Root owns their execution and separate
allocation/origin-failure probe. Preserve focused recipient/redirect/remote
coverage and balanced comparisons against selected51 and original R before
admission. No timing cause or gain is inferred from this native review.
