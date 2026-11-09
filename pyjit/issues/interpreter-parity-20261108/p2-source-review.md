# P1/P2 independent source review

2026-11-08. Reviewed the five-file uncommitted diff in the isolated
`interpreter-parity-20261108` runtime worktree against R5 `0953757f4e7`.
Verdict: accept the source for coordinator-owned build and focused validation.
No must-fix finding. This is not a compile, executed-test or performance pass.

P2's `_PyValue_PointerBorrow` constructs only the canonical pointer tail. Its
comment correctly restricts callers to objects already proved to have that
representation. Generic `PyValue_FromObjectBorrow` still returns NULL before
calling it, still checks exact int/float identity and returns inline scalars,
and reaches the pointer helper only when the old implementation reached its
immortality tail. Wide exact integers and NaN therefore retain their previous
pointer representation; number subclasses remain objects. The helper preserves
the address-limit assertion and immortal no-count tag.

`_PyFrame_CodeHome` takes typed `PyCodeObject *`, asserts the type, and duplicates
the pointer borrow. Mortal code gains one reference; immortal code keeps the
no-count tag. Both `_PyFrame_Initialize` and `_PyFrame_PushTrampolineUnchecked`
use it without changing frame ownership, clearing or publication order. The
ordinary and overlapping entries share the former initializer. No other
production caller bypasses numeric canonicalization through the new helper.

The direct C test checks mortal pointer bits, increment and balanced close,
and immortal `_Py_InitCleanup` bits/tag. Python tests cover active code after
`function.__code__` replacement, suspended generator ownership, and ordinary/
context initialization with an exception. Existing BorrowedEntryTests include
inline boundaries, wide integers, NaN and subclasses; these remain relevant
because the shared generic tail changed. The `same_bits` field in the existing
borrow test compares the same inline function twice, so its independent value
comes from the expected-kind/round-trip checks, not that field alone.

P1's immediate return on successful refresh is valid: the refresh has already
checked the current frame and execution copy and published the original PC,
with no intervening escape. Failure retains the second validation/error path;
constructing the original error can run cleanup and must not inherit a success
proof.

The diff has no whitespace errors. The coordinator must still compile release
and debug and run the applicable focused checks before landing; this review
claims neither runtime execution nor throughput improvement.
