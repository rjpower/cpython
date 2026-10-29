# 14: The handler contract

Status: design, 2026-10-28, revised the same day after review round 1
(`boil/design/reviews/14-handler-contract-review.md`, verdict revise;
section 7 maps each blocking issue to its change). For step H
(PROCESS.md section 0): after 10a rung 3, before step 8. The user's
ruling of 2026-10-28: the interpreter need not be tuned to LuaJIT's
level, but obvious improvements are not left on the ground; a mechanism
stays when it is principled and orthogonal, and one-offs go. This note
states what every generated handler does and does not do, so that the
rules apply through the generator to all handlers at once, and so that
tier 0's templates (step 8), which copy the handler bodies, inherit the
lean form rather than the present one.

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
integer add handler is 109 static instructions. Reading them, with the
ceiling each rule below can reach:

| Component | Instructions | Rule |
|---|---:|---|
| guards: `_PyValue_IsCompactInt` on two operands, the heap arm (range test, masked shape-word compare, size check, sign-and-digit extraction) | about 25 | 3 |
| clearing stores for the collector: `IR_CONSUME_INPUT` on each temporary input (load the local count, compare, store NULL) | 6 to 10 | 2 |
| prologue and epilogue (`push rbp; mov; sub; pop`): the unit keeps frame pointers | 5 | 5 |
| frame reload from the thread state and the position store | 2 | 1 |
| decode: shift and mask per 12-bit field | 6 | none; comparable to LuaJIT's byte fields |
| dispatch: load the word, index the table, indirect jump | 4 | none |
| the add, the overflow test, the result store | about 10 | none |

FOR_ITER_RANGE (69 static) adds the iterator object's type check through
the shape word, three 64-bit fields in memory and a narrowing check
(rule 4); JUMP_BACKWARD (23) is the eval-breaker test and the jump (rule
6). The runtime around the interpreter is a separate matter: instance
creation (1.19), raise (1.41), `str +=` (2.42), generators and coroutines
regressed for allocator, collector, frame and string reasons owned by
steps 7 and 9 and the call layer. This note is about the handlers.

## 2. The contract

Six rules. Each applies through the generator, the compiler or the
build, never by hand per handler.

**Rule 1: frame access follows the handler's dependencies; position is
published as upstream does.** In the tail-call build the frame is not a
passed register, so every handler reloads it from the thread state even
when it needs it only for the position store. The generator emits the
frame load when the handler body depends on the frame (constants through
the code object, jump targets through `ir_words`, feedback through
`ir_feedback`, the position store) and makes the frame a passed register
if the calling convention has one free (`preserve_none` gives twelve);
whichever is cheaper on the measurement is kept, the other is not
written. The position store itself stays in every word. The review
established that the frame's position has observers that are not
safepoints: `_YIELD_VALUE` advances `instr_ptr` and the resume reads it;
`faulthandler` dumps it from a native signal handler; the remote
sampling profiler (`Lib/profiling/sampling`, test_external_inspection)
reads it from another process at any instant. Publishing only at
safepoints would give those readers a position stale by the words since
the last safepoint, which is a contract change for one instruction per
word; it is not taken. The rule's ceiling is therefore the frame load,
one or two instructions per word, and rule 1 is the smallest of the six.

**Rule 2: a liveness map replaces the clearing stores.** The collector
marks, for a frame observed at word w, the locals and the temporaries
live at w, read from a per-code table the compiler emits beside the
exception table. Precisely:

- *What the map says.* For each word at which a frame can be observed
  (a word with a safepoint, a suspension word, and every word in an
  instrumented code object, since `INSTRUMENTED_INSTRUCTION` can collect
  before any word), the set of temporaries whose values are live into
  that word. Locals are always live for the collector (Python exposes
  them through `f_locals`), so the table covers temporaries only. The
  set comes from `_PyRegCfg_Liveness` (note 07 section 5; compaction is
  LIFO by holding, not by liveness, so the map is computed after
  compaction on physical slots, at assembly).
- *Values a handler writes before its collecting call* (the unpack
  helper's outputs, a call window being replaced, a result published
  early by `IR_RETURN_EARLY`) are not in the live-in set. They are
  rooted as today, by the handler's own precise C frame and by spans
  with an initialised-prefix length over the slots written so far (note
  11 section 4); the map does not describe them, and the C-mode checker
  continues to require the span. The generator's root checker treats
  residence in a frame slot as anchoring only for slots the map covers
  at that word.
- *Suspension.* Temporaries survive a yield (`make_object() + (yield
  None)` keeps the left operand; `for x in g(): yield x` keeps the
  iterator), so a suspended frame's entry is the live set at its resume
  word, which `instr_ptr` points to after `_YIELD_VALUE`'s increment.
  The yielded value's slot and the not-yet-written send result are
  stated in the entry explicitly. `throw()`, `close()`, `yield from`
  and coroutine suspension are the same entry.
- *Slots stay NULL-initialised at frame creation.* Unbound-local
  semantics need it, and it keeps every slot well formed; a dead
  temporary may then hold a stale value, a pointer to an object an
  earlier collection freed. Every reader that dereferences slots moves
  to the map: `_PyGC_VisitFrameStack`, the generator and frame-object
  traversals, `_PyFrame_CheckValues` (which calls `Py_TYPE` on every
  slot today), the verifier. Readers that copy bits (`_PyFrame_Copy`)
  need no change. The NULL-based assertions in `IR_STORE_TEMP`,
  `IR_STORE_DST`, `_PyFrame_SetReturnValue` and the return cleanup are
  inventoried in rung 2's work doc and each is rewritten against the
  map or deleted with its reason.
- *What retires and what remains.* Retired: `IR_CONSUME_INPUT`,
  `IR_MOVE_TO_DST`'s NULL store, `IR_CLEAR_SLOTS` at calls, and the
  frame-exit clearing whose purpose was the collector. Remaining:
  stores that are semantics (a local deleted, an unbound local, the
  NULL that `LOAD_FAST_CHECK` tests) and stores that publish a value
  for a reader (the yielded value). The work doc lists each clearing
  site in one column or the other.
- *The table's lifetime.* The map is part of the code object: built by
  `assemble.c`, marshalled with the code (pyc magic bumped), produced by
  the frozen-code generator and by `code.replace()`, which regenerates
  it when words or the slot count change and rejects a mismatched map;
  there is no all-slots fallback. The encoding is a per-block live-in
  set with a per-entry delta; its size is recorded on the suite's code
  objects and has no limit that rejects valid code.
- *Verification.* Two probes at the event. Missing root: a temporary is
  live into a safepoint and holds the only reference to an object; at
  the collection the record shows no other root reaches it; the object
  is used afterwards. Clearing its map bit must make kill mode trap.
  Retention: a dead temporary holds the only reference to an object
  with a finalizer; at the collection the record shows whether it was
  marked and from which root, and the drain records the finalizer's
  run. Marking the slot live must change both records. Mark B of the
  A/B verifier is extended to the map-excluded slot words with the
  conservative lookup and provenance (code, word, slot, collection), so
  a kill-mode trap on such a word names the entry at fault; B's reach
  beyond A is otherwise expected (a correctly dead temporary) and is not
  by itself a finding.

**Rule 3: operands are canonical, and a guard tests one form.** Note 06
section 18.1: every exact int that fits int32 is inline, canonicalised
by `PyValue_FromObject` at ingress. A one-digit exact heap int therefore
cannot be a handler operand, and the heap arm of `_PyValue_IsCompactInt`
guards against a representation leak, not a legitimate form. Rung 1
adds a debug-build assertion of canonical form at the slot-write macros
and at the value-API returns, runs the suite with it on both builds, and
fixes at its ingress any producer that fails it (the unchecked
`_PyValue_FromObject` is the suspect, review 2 section 5.8). Then the
int guards test the inline form only and the compact accessors go. No
threshold decides this; the invariant does. The handler space step 8
needs is not spent on a second int form.

**Rule 4: the range loop is not this step's.** FOR_ITER_RANGE's cost is
the iterator object: a type check through the shape word, three 64-bit
fields in memory, a narrowing check. The iterator is a hidden temporary
that no Python code can name (the type is untracked, so
`gc.get_objects()` does not list it either), so a counter in the window
would be sound; it is not taken because END_FOR and POP_ITER would then
see two forms of the loop. The loop variable becomes a typed register in
tier 0, where 6c's hand-off already sends it.

**Rule 5: the handler unit follows the build's frame-pointer switch, and
the gate builds omit frame pointers.** configure defaults to
`--with-frame-pointers` so that native profilers can unwind without
DWARF; the fork's walk (note 11) and its profiling use CFI, so the gate
builds configure `--without-frame-pointers` (recorded in PROCESS.md
section 3), the ASan build keeps them, and the `no-omit-frame-pointer`
attribute on `_PyEval_EvalFrameDefault` (Python/ceval.c, upstream's perf
trampolines) is removed in favour of the switch. Rung 1 confirms on the
omitted build that the emitted handlers have no prologue, that the
walk's tests pass and that `PYTHON_GCROOTSTATS` shows zero fallbacks
over the suite.

**Rule 6: the eval-breaker test stays** on every backward jump and at
every call. It is the one place signals, the collector's drain and
instrumentation requests are noticed; a dispatch-table swap would be a
second mechanism for the same job.

## 3. What this step is not

It is not tuning. No handler is rewritten for a benchmark; every change
is a generator rule, a compiler table or a build flag, and applies to
all handlers. The record at each rung: the micro set and the kernels
(instructions), the table's bytes over the suite's code objects, the
compile-time cost of the table, the collector's root-phase time and
pause distribution, and RSS, all against the 6d records with
attribution. The numbers do not gate the step; correctness does.

## 4. Order and dependencies

After 10a rung 3: rule 2 reads frame slots by the map, which needs the
eval loop's own C frame precise and the walk attributing it, and the
spans with initialised prefixes that rule 2 relies on for in-flight
outputs are 10a's. Before step 8: the templates copy the handler bodies,
and tier 0's safepoint entries use rule 2's table keyed by machine
address (one schema; tier 1 extends it with a version for spills and
virtual frames in step 9).

Two rungs. Rung 1: rules 1, 3 and 5, generator, assertion and build
changes, with the micro set recorded; about one day. Rung 2: rule 2,
the compiler table, the readers' inventory and conversion, the verifier
extension, the clearing sites classified and the retired ones deleted;
about two days. Each rung goes through the loop of PROCESS.md section 1.

## 5. Tests

- Rule 1: the generator test that every handler's frame load is present
  exactly when the body depends on the frame; the existing position
  tests (tracebacks, `f_lineno`, test_external_inspection) stand since
  publication is unchanged.
- Rule 2: the two probes of section 2 under kill on both builds, each
  with its negative control (the map bit cleared, the dead slot marked
  live); a poisoning probe at an injected collection that writes a trap
  pattern into every map-excluded slot and asserts no reader visits them,
  across frame creation, argument binding, ownership transfer, unwinding
  and completion; marshal, frozen-code and `code.replace()` round trips
  that change words or the slot count, each collecting during execution
  and asserting the entry at the event; the A/B verifier under kill over
  the suite with B extended to excluded slots.
- Rule 3: the canonical-form assertion over the suite on both builds
  before the guard change; a negative control that bypasses
  canonicalisation at one ingress and trips it.
- Rule 5: no prologue in the emitted handlers (objdump in the gate);
  the walk's tests; zero fallbacks.

## 6. Open questions for the coordinator

1. The map's encoding, decided in rung 2's work doc with the size
   distribution over the suite; the constraint is "no limit that rejects
   valid code".
2. Whether `_PyFrame_CheckValues` keeps a whole-frame well-formedness
   check (every slot NULL or a valid value, which stale pointers break)
   or becomes a map-driven check; the latter is the proposal.

## 7. Review response (round 1)

| Blocking issue | Change |
|---|---|
| 1. Instruction liveness does not describe roots written inside a handler | Rule 2: in-flight outputs are rooted by the handler's precise frame and prefix spans, not the map; the checker treats slot residence as anchoring only for map-covered slots |
| 2. Safepoint-only entries omit suspension and instrumentation | Rule 2: entries at safepoints, suspension words and every word of an instrumented code object; temporaries survive yields and the suspended entry is the resume word's live set; section 6's empty-yield premise deleted. Rule 1 no longer changes publication at all |
| 3. Removing initialisation breaks other slot readers | Rule 2: slots stay NULL-initialised; dead slots may be stale; every dereferencing reader moves to the map, inventoried in rung 2 |
| 4. The A/B oracle confuses reclamation with missing roots | Rule 2's verification rewritten as two event probes with negative controls; B extended to excluded slots with provenance |
| 5. Rule 3 preserved a representation the value contract forbids | Rule 3 rewritten: canonical operands asserted and enforced at ingress; the heap arm goes; no threshold |
| 6. Rule 1 conflated frame access, publication and observation | Rule 1 rewritten: frame access by dependency; publication unchanged, with the observers named |
| 7. The table's lifetime contract was missing | Rule 2: marshal, frozen code, `code.replace()`, magic bump, rejection of a mismatched map, no fallback |

Non-blocking: rule 5 now states the switch, the default, the attribute
and the gate builds' configuration; the liveness claim is corrected
(compaction is LIFO; the map is computed on physical slots at
assembly) and the record lists table bytes, compile time, root-phase
time, pauses and RSS; rule 4's rationale is corrected (the iterator is
unnamed and untracked; the objection is the second loop form).
Questions: no size limit that rejects code; H commits the interpreter's
schema, which tier 0 uses by address and tier 1 extends with a version;
the clearing sites are classified in rung 2's work doc as semantic
(kept) or collector-only (retired).
