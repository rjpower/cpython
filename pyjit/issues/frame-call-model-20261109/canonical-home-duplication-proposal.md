# Bounded canonical-home duplication

Proposal only; no runtime edit or speed claim. The retained sixth n-body
cycle record puts 12.66% of samples in `LOAD_FAST_BORROW`, including 2.86%
attributed to inlined `_PyValue_Publish`. Both retained golden and candidate
disassemblies still test exact `PyLong_Type` and `PyFloat_Type` after the
local is duplicated. The candidate's 3.33% instruction gap has no isolated
cause in this record. This proposal tests one removable classification path.

## Source and owner proof

`Python/bytecodes.c:LOAD_FAST_BORROW` currently asserts a non-NULL local,
then does `value = PyValue_DUP(GETLOCAL(oparg));`. The generated case calls
`IR_STORE_DST(IR_A(ir_word), value)`, which repeats numeric canonicalization
through `_PyValue_Publish`. `GETLOCAL(i)` is `slots[i]` in
`Python/ceval_macros.h`; its indexed cell is a canonical frame home. The
`LOAD_FAST` body has the same source pattern, so a name-independent proof
also covers it; the hot-record attribution only identifies `LOAD_FAST_BORROW`.
The format assigns `oparg` the `LOCAL_IN` field, and code construction checks
the encoded local. `PyValue_DUP` increments the mortal count when
needed and returns identical bits. Therefore its result is a separately
owned canonical value before the destination store. The home premise is the
existing frame ABI, not an inference from the handler name. It includes
inline int/float, boxed large int and NaN, number subclasses, static
immortals, and mortal objects. Raw pointer-form exact boxed small numbers
are invalid frame-home inputs; supported object and debugger setters must
canonicalize before storing them. Debug `_PyValue_CheckHome` and value API
tests should continue to enforce this boundary.

The first increment recognizes only one scalar-output, one-uop, pure local
duplication with no effectful suffix or cache beyond unused padding. Require
`store_dst`, a nonwindow format, `LOCAL_IN`/immediate field identity, no input
owner, a single logically fresh owned `ROLE_DST` output with no aliased output
view, and exact top-level tokens:

```c
assert(!PyValue_IsNull(GETLOCAL(oparg)));
result = PyValue_DUP(GETLOCAL(oparg));
```

Physical A may equal the source local B: duplication takes its own count
before replacing and retiring that local. `result` is taken from the declared
output, never an opcode or uop name.
Reject any other statement, conditional, alternate helper, address escape,
second assignment, preprocessor directive or later uop. Also reject local,
stack-output or cache bindings that shadow `GETLOCAL`, `PyValue_DUP`,
`assert` or `PyValue_IsNull`, even in otherwise unused parts. This deliberately
excludes `LOAD_FAST_CHECK`, whose error and binding path needs its own proof.
The source test should rename the producer and result to show eligibility
does not come from a name list. Mutations that shadow/redefine `GETLOCAL`,
change its argument or format role, replace `PyValue_DUP` with a raw value
call, rewrite the result, or append a callback/fallible suffix must return no
canonical fact. A negative result retains ordinary publication; malformed
DSL may still fail generation for its independent reason.

## One shared emitter fact

Generalize `CanonicalNumericResult` and
`canonical_numeric_result(inst, fmt)` in
`Tools/cases_generator/ir_generator.py` to one `CanonicalResult` /
`canonical_result_source(inst, fmt)` result-name fact. Keep the current
numeric-constructor recognizer as one source proof and add the exact home-dup
proof as the other. Rename `SlotLayout.canonical_numeric_result` to
`canonical_result`; the existing `IRStack.store` final-destination selection
then uses `IR_STORE_CANONICAL_DST` for either proof. Update its current tests
and callers, with no fallback field, parallel flag, opcode table, or C
pointer-provenance parser. This does not alter
`result_publication_contract`: the shape-reader
`IR_CANONICAL_HOME_DUP(found)` plus retirement marker still governs its
*earlier temporary* publication. The local duplicate has no such marker or
pre-retirement callback. It needs no new DSL intrinsic and leaves metadata,
`result_phase`, `observer_outcome`, and the observer generator unchanged.

`IR_STORE_CANONICAL_DST` already shares `IR_STORE_DST_IMPL` with ordinary
storage; update its constructor-only comment to describe both checked sources.
For a temporary it writes the same owned bits and runs the same
home check. For a folded local it duplicates before replacement, then retires
the displaced owner in the same order; source and destination may be the same
local. `IR_NO_DST` closes the duplicate once. A displaced-owner finalizer can
still raise after the store, through the existing `finish_displaced_local`
path; removing the numeric recheck changes neither owner nor continuation.
No raw/native-result or arbitrary local-store producer acquires this fact.

## Review and admission

The generator and bytecode source are currently owned by A's separate
terminal-provider packet. A should peer the exact source rule and file
ownership before root admits an edit. The first source checkpoint should
show the two source-identical LOAD_FAST and LOAD_FAST_BORROW handlers changed,
with the four existing float numeric-constructor handlers unchanged and no
metadata change from this proof. A combined diff with the separate terminal
provider packet may also contain its three new instrumented handlers; account
for those separately from these two ordinary-handler changes, and retain the
earlier ten observed providers. Do not rewrite that packet's sealed host
receipt. Host tests should cover the positive, renamed
positive, and each mutation above. Debug/release semantic checks should cover
temporary, folded-local and sink destinations; numeric categories; f_locals
replacement; same-local replacement; and a displaced-owner finalizer that
observes the committed result or raises. Inspect actual GCC output for removal
of the exact-number classification in `LOAD_FAST_BORROW` without added branch
work, then compare paired n-body and a nonnumeric control with the same
identity and PMU protocol. A performance gain remains hypothetical until
those receipts exist.
