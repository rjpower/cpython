# Exact-tuple CALL_EX preparation

Status: bounded implementation and semantic fixtures committed at
`71d99873c4e`, with GC actual source review GO. No native or performance
qualification. Root owns integration, generation and execution.
This is increment 1a of the [combined design](../../design/proposals/interpreter-performance56.md).

## Scope and contract

The isolated branch `pyjit/perf56-call-ex-tuple` in P starts at frozen I
`1413a9fe218`. The retained-EH branch remains at `80b3de991c6`. Only
`Python/ceval.c` and the existing native-call transaction test module change.
No generated product, instruction format, frame field, helper argument, value
tag or call-state flag changes.

`call_transaction_prepare_tuple` currently acquires an owning argument value,
an owning function value and a new object reference to the arguments before
discovering an already exact tuple. The call window already owns these values.
Upstream's `_MAKE_CALLARGS_A_TUPLE` tests a borrowed object first.

The new prefix reads the protected required argument home into `PyValueBorrow`
inside a lexical scope and computes only a boolean:
`IsObject(input) && PyTuple_CheckExact(AsKnownObjectBorrow(input))`.
The scope ends before refresh. No allocation, release, callback, owner transfer
or pointer escape occurs in the probe. A hit returns through the same
`call_transaction_refresh(tstate, frame, current)` as the old exact-tuple arm.
The window still anchors the tuple and callable throughout refresh.

A miss enters the unchanged owning preparation body, including its existing
tuple test, conversion, error cleanup, replacement publication and final
refresh. Tuple subclasses and inline/noniterable inputs use this body. Keep
`CALL_PREPARED | CALL_EFFECTS` promotion, required input admission, optional
self/kwargs roles and monitoring ordering unchanged. This is not permission
to borrow through iterable conversion, monitoring, a call or final refresh.

## Expected work and limits

| Path | Expected change |
| --- | --- |
| Exact object tuple | Remove two value DUP/CLEAR pairs and one owning AsObject/CLEAR pair; keep final refresh and the type probe. No home store changes. |
| Nonexact input | Add the representation/exact-type probe, then retain the old owning path verbatim. |
| Empty immortal tuple or uncounted callable | Fewer actual reference-count operations were present, so fewer can disappear. |

The upper bound is three mortal INCREF/DECREF pairs per exact hit, not three
pairs per CALL_EX. Count actual hits and counted acquisitions in a separate
diagnostic binary if needed; do not add production instrumentation. Ordinary
timing must use the uninstrumented binary. No helper hop or addressable record
is added. Inspect native code for the probe, bypassed acquisitions and unchanged
refresh. The boolean is native temporary state, not new runtime metadata.

This removes neither evaluated argument snapshots nor binder/default/native
workspace copies. Exact thread calls already overlap their argument homes.
Monitored callable/first-argument snapshots and copies protecting reentry keep
their own lifetime contracts. Returning a local or virtualizing values through
a callback is outside this increment.

## Review and qualification

Root accepted the bounded design after Astra's combined source review. GC's
actual proposal and implementation review confirms that the probe is
callback-free. The removed temporary closes cannot be last-owner callbacks
while the owning window remains intact. AST/compile, diff checks and a
byte-for-byte comparison of the unchanged owning fallback pass. Runtime
tests remain unexecuted here.

The new methods in
`test_pyvalue_native_call_transaction.NativeCallTransactionTests` are:

- `test_exact_tuple_preparation_all_routes`: empty/nonempty tuples, optional
  keyword dictionary, original/constructed/replaced/marshaled images.
- `test_nonexact_tuple_preparation_keeps_evaluated_inputs`: tuple subclass and
  custom iterable convert once while mutating caller locals and collecting;
  the original evaluated callable/arguments win.
- `test_nonexact_tuple_preparation_errors`: inline, None and noniterable
  failures, plus a tuple subclass whose iteration raises the exact exception.
- `test_exact_tuple_monitor_observes_evaluated_inputs_once`: CALL observation
  mutates locals, disables its own instrumentation and collects; the original
  input reaches one body invocation.

Existing iterable-retirement, primary-error, call-role/optional-kwargs and
source-reentry tests remain relevant. Allocation failure, accepted/rejected
redirect behavior and provider/code mutation use their existing focused
qualification cases; no new fault framework is introduced. NULL required
inputs remain the existing admission/read-role error, not a nullable tuple
probe permission. Static AST and unchanged-fallback checks are source gates;
runtime selectors above require root's candidate interpreter.

No exact hits in a workload means no claimed benefit there. No optimized native
work removal falsifies the mechanism. Extra nonexact-path cost, spills/code
layout or repeated elapsed regressions can reject the candidate even if the
exact path removes references. Compare against its frozen parent before any
broader borrowing or completion change is composed.

## Next completion candidate, proposal only

The existing `_PyEval_CompleteStep` boundary receives eight machine-word
arguments, including a literal step kind chosen by its sole generated caller.
One substantive follow-up is direct FOR/SEND/SEND_THROW entries generated from
the same `complete_step_impl`: FOR needs six arguments (no destination or
kind), SEND/THROW seven (no kind). Under x86-64 SysV this can remove two or one
stack arguments and specialize dead kind-dependent paths. Native inspection
must establish the actual reduction; existing compiler specialization may
already remove some work, and duplicated helper bodies may cost code space.

Preserve the shared PRE/POST observation, error origin, redirect, detach,
publication and retirement body and its existing action receipts. A wrapper
name supplies no ownership, canonicality or abrupt-prefix proof. Keep native
`canonical_input=false` unless independently proved. GeneratorReturn already
selects canonical/literal internal paths; retain its immutable-word decode and
native-entry case. No normal CALL descriptor lookup, caller-kind field, early
local steal or whole completion-body inlining is proposed. This follow-up has
IR design agreement, but no source implementation approval or cost result.
