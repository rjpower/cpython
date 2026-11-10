# Candidate 42 compiled inline-policy review

Reviewed runtime `478e7978845` against retained41 `0a35a382909` and frozen39
`4d666e1b482`. The complete runtime diff from41 is one `Py_ALWAYS_INLINE`
annotation on `_PyFrame_PushOverlapping`. Root reports release/debug builds
complete with 117 checked modules; its focused tests are separate validation.
This review ran only source/ELF inspection and lightweight host parsing, with
no runtime edits, builds, native workloads or commits.

**Ready for a bounded performance comparison after root's focused checks pass,
with an important mechanism limit:** all three calls to PushOverlapping are
removed, but GCC outlines `_PyFrame_Initialize` inside those same commits.
The experiment partly moves the native boundary rather than making the whole
frame commit inline. No throughput benefit or promotion follows from this review.

## Actual ELFs and emitted bodies

Paths are below `/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/`:

- Frozen39: `golden-20261109-release/python`.
- Retained41: `pyjit-link-wave1/object-call-20261109/gap41-perf-record/candidate-python`.
- Candidate42: `execution-boundaries-release/python`.

Read-only `nm -S`, `objdump -d --no-show-raw-insn` and `size` inspect the existing
executables. A host script compares instruction lists with relocated symbol
addresses and RIP displacements removed. Static counts include cold branches;
they are not dynamic work or latency measurements.

| Symbol | 39 bytes | 41 bytes | 42 bytes |
| --- | ---: | ---: | ---: |
| Positional general handler | 0x5c8 | 0x3ff | 0x4f3 |
| Generic overlap provider | 0x7cb | 0x600 | 0x6be |
| Keyword Python handler | 0x13d | 0x5f8 | 0x6d3 |
| Positional exact-args handler | 0x592 | 0x592 | 0x582 |
| Bound-method exact-args handler | 0x85a | 0x85a | 0x86a |
| Outlined PushOverlapping | Absent | 0x20c | Absent |
| Outlined Frame_Initialize | Absent | Absent | 0x115 |

The three42 bodies inline initialized-frontier handling, frame-storage
reservation, storage checkpoint and value/header stack updates. They then call
`_PyFrame_Initialize.isra.0` at:

- `0x3473cc` in generic overlap;
- `0x37f755` in positional general;
- `0x381211` in keyword Python.

The new initialization helper begins at `0x33d4f0`. Its ordinary path is a leaf:
no saved-register pushes, stack reservation or canary. It initializes the frame
header/code owner and fills constant homes; only its fatal type-error path has
a native prologue/call. Thus42 removes the previous outlined helper's six-save
prologue/epilogue, but retains a call/return boundary plus caller argument
handling. The original goal of fully inline frame construction is not met.

Generic overlap's frontier-fill arm now emits a NULL-store loop instead of41's
memset inside the shared helper. Positional general still has a memset arm;
keyword also has inline frontier filling. These remain conditional initialization
work, not a new second frame-storage policy. Their frequency needs measurement.

ELF text is 7,527,587 bytes: +432 versus41 and +2,896 versus39. Data and bss
remain 975,152 and 298,104 bytes. Removing one standalone symbol is therefore
not evidence of a reduced hot footprint.

## Stack costs, misses and guards

| Path | 41 stack reservation | 42 stack reservation | 41→42 static rbp references |
| --- | ---: | ---: | ---: |
| Positional general | 0x50 | 0x80 | 42→77 |
| Generic overlap | 0x48 | 0x58 | 89→94 |
| Keyword Python | 0x80 | 0xa0 | 74→81 |
| Positional exact args | 0x50 | 0x50 | 57→58 |
| Bound-method exact args | 0x60 | 0x60 | 103→102 |

These static references include cold paths and the newly inlined reservation
body; they do not establish more dynamic spills. Positional general and generic
overlap retain no canary references. Keyword retains its scratch-induced canary
(one TLS load and four exit checks) without adding another protector.

The generator-owner early miss of generic overlap remains exactly 13
instructions. The owner compare/branch stays at entry+0x1d/+0x21; the native
prologue still starts only at +0x27, so this miss performs no saved-register or
stack-allocation work. Its normalized early path matches41, including the +0x200
return island. Forced inlining has not defeated this shrink-wrapping.

The source guard/ownership interval is unchanged by the single-line diff.
Compiled checks remain before commit: function/provider/version/recursion,
keyword count/self/name/destination checks, and current-default shape/coverage.
For example,42's positional cached-default offset check branches to fallback
before the first callable detach at `0x37f683`. Its conditional empty-kwargs
allocation remains before that detach. Keyword default coverage completes at
`0x38110f`; keyword-owner detachment starts at `0x38113e`, followed by names
and callable detach, inline reservation and initialization. Saved keyword homes,
default homes and the existing return/current-frame publication retain their
source contract. The new leaf initialization call introduces no fallible
operation or Python callback in that interval.

## Exact-call and coroutine correction

Retained41's actual coroutine profile, supplied by the coordinator, places
CALL_PY_EXACT_ARGS at 10.28% self, RETURN_GENERATOR at 8.25%, GET_AWAITABLE at
6.09%, CompleteGeneratorReturn at 5.89%, make_gen at 5.88% and SEND_GEN at
5.52%, with no YIELD samples. My earlier general-fallback assumption in the
triage was incorrect and is now corrected there.

`function_kind()` classifies optimized signatures without variadics/kwonly as
simple; CO_COROUTINE does not prevent CALL_PY_EXACT_ARGS. That handler's
generator-owner arm pushes a non-overlapping frame, copies argument owners and
clears the caller window. It already inlines its initialization rather than
calling the new outlined helper. Its total static instruction count stays339
in39/41/42, while its register/stack-home assignment and branch placement change
in42. The handler shrinks16 bytes; the bound-method twin grows16 bytes while
staying at500 static instructions. Their native call sets are unchanged.

SEND_GEN (170 instructions), RETURN_GENERATOR (223), GET_AWAITABLE (312),
CompleteGeneratorReturn (443), make_gen (51), RETURN_VALUE (127), the vector
transaction runner (127), generic fallback provider (530) and start_frame (65)
all remain normalized-instruction-identical across39/41/42. This does not prove
equal cycles or whole-program execution. It does prevent attributing a future
coroutine timing change to saved calls at the three general/keyword overlap
sites without dynamic evidence.

## Performance readiness and limits

The annotation preserves one helper body and ABI and leaves debug expansion
governed by the existing no-op Py_DEBUG macro. The compiled review finds no
guard, ownership or early-miss regression requiring a source repair before
the planned focused checks and performance run. Root owns their executed results.

Compare42 with retained41 and39 in balanced logging/keyword runs and raw
instructions, with coroutine and DeltaBlue as controls. Record the residual
initialization helper and larger caller stacks when interpreting results.
If42 does not improve application evidence, do not call the experiment successful
because PushOverlapping disappeared. Extending forced inlining into another
helper or changing scratch/layout would require a separate reviewed experiment.
Candidate42 is not promoted by this review.
