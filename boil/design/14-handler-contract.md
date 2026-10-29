# 14: The handler contract

Status: design, 2026-10-28, for step H (PROCESS.md section 0): after 10a
rung 3, before step 8. The user's ruling of 2026-10-28: the interpreter
need not be tuned to LuaJIT's level, but obvious improvements are not
left on the ground; a mechanism stays when it is principled and
orthogonal, and one-offs go. This note states what every generated
handler does and does not do, so that the rules apply through the
generator to all handlers at once, and so that tier 0's templates (step
8), which copy the handler bodies, inherit the lean form rather than the
present one.

## 1. What a handler does today

Measured at the 6d head on the tail-call build, `s = s + i` over
`range(n)` is three words per iteration (FOR_ITER_RANGE,
BINARY_OP_ADD_INT, JUMP_BACKWARD) and about 106 instructions:

| Row of the micro set | Baseline | 6d |
|---|---:|---:|
| empty range loop | 81 | 61 |
| int add | 164 | 45 |
| float add | 130 | 48 |
| attribute load | 71 | 42 |

LuaJIT's interpreter runs the same loop in 12 to 15 instructions. The
integer add handler is 109 static instructions. Reading them:

- **Position and frame, every handler.** `frame = tstate->current_frame`
  and `frame->instr_ptr = next_instr` before any work: a load and a
  store per word, so that any escape can find the position.
- **Prologue and epilogue, every handler.** `push rbp; mov; sub` and
  `pop`: the handler translation unit keeps frame pointers.
- **Decode.** Shift and mask per 12-bit field; three fields, six
  instructions. Comparable to LuaJIT's byte fields.
- **Guards.** `_PyValue_IsCompactInt` on each operand: an inline int32
  test, then for a pointer a range test, a masked shape-word compare
  against the int type, a size check, and a sign-and-digit extraction.
  About 25 of the 109.
- **Clearing stores for the collector.** `IR_CONSUME_INPUT` on each
  temporary input: load the local count from the code header, compare,
  store NULL. Up to two compares and two stores per word. FOR_ITER_RANGE
  has the matching `IR_MOVE_TO_DST`, a compare, a store and a NULL store.
- **Dispatch.** Load the next word, index the handler table, indirect
  jump. Comparable to LuaJIT.
- **The eval-breaker test** on every backward jump, about 4.

The runtime around the interpreter is a separate matter: instance
creation (1.19), raise (1.41), `str +=` (2.42), generators and coroutines
regressed for allocator, collector, frame and string reasons owned by
steps 7, 9 and the call layer. This note is about the handlers.

## 2. The contract

Six rules. Each applies through the generator or the build, never by
hand per handler.

**Rule 1: position is stored at safepoints only.** A handler stores
`frame->instr_ptr` and reloads `frame` from the thread state only if its
family has a safepoint: a call that can run Python code or collect, as
the generator's effect tables classify it (`call_effects.py`; the
runs-code property added by the copy-propagation fix, and the allocating
property of note 11 section 4.2). A handler without a safepoint keeps the
position in `next_instr` alone. The invariant that makes this sound: the
frame's recorded position is read only where another party can observe
the frame, and every such point is a safepoint. The collector reads it
at a collection (a safepoint); a traceback is built on an error path,
reached only from a safepoint; another thread sees the frame only after
this thread releases the GIL, at a safepoint; signal handlers run at the
eval breaker; tracing and monitoring run instrumented handlers, which
run code. The deopt jump passes `this_instr` to the generic handler,
which has a safepoint and stores it. A generator test asserts the store
is present in every handler with a safepoint and absent in every handler
without one.

**Rule 2: a liveness map replaces the clearing stores.** The collector
marks, for a frame suspended at word w, the locals and the temporaries
live at w, read from a per-code table the compiler emits beside the
exception table. The register CFG already computes temporary liveness
for compaction (note 07); the table records it at safepoint words only,
since a frame is observed only there. Locals are always live for the
collector (Python exposes them through `f_locals`), so the table covers
temporaries only. The clearing stores go: `IR_CONSUME_INPUT`,
`IR_MOVE_TO_DST`'s NULL store, and the slot clearing at calls. Locals
stay NULL-initialised at frame entry (unbound-local semantics);
temporaries need not be. This is the safepoint table note 11 wants for
tiers 0 and 1, in one format shared by the interpreter and the
templates; the interpreter's entry is the word's, a template's is the
machine address's. The encoding is decided in the work doc against a
measured size limit (the executing copy's feedback already added 424 to
875 bytes per code object in Phase 1); a per-block live-in set plus a
per-safepoint delta is the expected form.

Verification falls out of the existing A/B verifier: mark A reads the
map, mark B marks every slot; an object B reaches that A does not is a
slot the map wrongly calls dead, and kill mode traps on it. The opposite
error, a dead temporary the map calls live, is a retention; the finalizer
test of section 5 catches it.

**Rule 3: one value class per guard.** A specialised handler guards one
named value class and has one body over that class's extraction. A class
may be a union when the predicate is named, orthogonal and used
everywhere the class is meant: `_PyValue_IsCompactInt` (an inline int32
or a one-digit heap int) is such a class and stays. What the rule
forbids is a second guard form inside a handler for a case the class does
not name. Whether the specialiser should pick an inline-only handler when
a site's feedback has seen only inline ints is a feedback-driven
specialisation like any other, decided by a measurement in rung 1: the
share of compact-int operands that are heap ints on the kernels. If the
share is below one percent, the inline-only handlers are added and the
compact form stays for the sites that see heap ints.

**Rule 4: the range loop is not this step's.** FOR_ITER_RANGE's cost is
the iterator object: a type check through the shape word, three 64-bit
fields in memory, a narrowing check. A counter in the window would be a
second form of the loop (END_FOR and POP_ITER would see two), and the
iterator object stays reachable to `gc.get_objects()`, so the check
cannot go. The loop variable becomes a typed register in tier 0, where
6c's hand-off already sends it. Rule 1 and rule 2 take FOR_ITER_RANGE
from 69 static instructions to about 50; the rest waits for step 8.

**Rule 5: frame pointers are omitted in the handler translation unit.**
`-fomit-frame-pointer` on `Python/ceval.c` unless the build asks for
frame pointers (ASan and the perf-map builds set
`-fno-omit-frame-pointer` explicitly, configure.ac). The attributed stack
walk (note 11) unwinds by CFI, not by frame pointers; rung 1 confirms
this with the walk's own tests on the omitted build before the flag
lands.

**Rule 6: the eval-breaker test stays** on every backward jump and at
every call. It is the one place signals, the collector's drain and
instrumentation requests are noticed; LuaJIT's dispatch-table swap would
be a second mechanism for the same job.

## 3. What this step is not

It is not tuning. No handler is rewritten for a benchmark; every change
is a generator rule, a compiler table or a build flag, and applies to
all handlers. The micro set and the kernels are recorded before and
after, with attribution, as PROCESS.md section 3 requires, and the
numbers do not gate the step; correctness does.

## 4. Order and dependencies

After 10a rung 3: rule 2 reads frame slots by the map, which needs the
eval loop's own C frame precise and the walk attributing it (10a), and
rule 1 changes what the walk finds in `instr_ptr`, so the walk's tests
must be in place. Before step 8: the templates copy the handler bodies.

Two rungs. Rung 1: rules 1, 3 (the measurement and the decision) and 5,
all generator and build changes, with the micro set recorded; about one
day. Rung 2: rule 2, the compiler table, the collector's reader, the
verifier, the clearing stores deleted; about two days. Each rung goes
through the loop of PROCESS.md section 1.

## 5. Tests

- Rule 1: the generator test above (static); a probe that raises inside
  each handler family that has a safepoint and checks the traceback's
  line and the frame's `f_lineno`, under kill mode. Negative control: the
  store removed from one safepoint handler makes the traceback test fail.
- Rule 2: a dead temporary holding the only reference to an object with
  a finalizer, and a collection at the next safepoint, collects it; the
  negative control marks that slot live in the map and the object
  survives. The A/B verifier under kill over the suite is the oracle for
  the other direction; a map that calls a live slot dead traps.
- Rule 5: the walk's tests on the omitted build; `PYTHON_GCROOTSTATS`
  shows zero fallbacks over the suite.
- Rule 3: the heap-int share recorded; if inline-only handlers are
  added, their deopt on a heap int is a test.

## 6. Open questions for the coordinator

1. The liveness table's encoding and its size limit, decided in rung 2's
   work doc with the measurement.
2. Whether a frame suspended at a yield needs the map (a generator's
   temporaries are dead across a yield by construction in the register
   CFG; if so, the yield word's entry is empty and the map need not
   include it).
