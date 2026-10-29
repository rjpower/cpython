# 14: The handler contract

Status: design, revised 2026-10-29 after two review rounds (both
returned revise). Round 1 is `reviews/14-handler-contract-review-1.md`;
round 2 is `reviews/14-handler-contract-review.md`. Sections 7 and 8
record the responses. This is a specification, not implemented evidence.
For step H (PROCESS.md section 0): before checkpoint I and step 8.
H1 starts when its checker/walk/build dependencies are qualified; H2's
maps require 10a rung 3's precise readers and spans. The user's
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
regressed for allocator, collector, frame and string reasons. Generations,
6e, the call layer and a coordinator-assigned string/compiler design own
those interpreter costs before checkpoint I. The numbers here remain
historical; this note specifies the handler part of that work.

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
if the calling convention has one free (`preserve_none` gives twelve).
Rung 1 compares frame reloads with a passed frame on the same tail-call
build and workloads, including leaf arithmetic and frame-heavy calls;
elapsed time decides, with instruction counts and register spills used
to explain it. An inconclusive comparison keeps the existing convention.
The position store itself stays in every word. The review
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
reads an explicit root-state key for each frame, independently of its
diagnostic `instr_ptr`. The key selects the locals and temporaries the
collector may dereference; spans cover values held during transitions.
Publishing a key and updating the covered slots must have no collecting
call between them unless a span protects the intermediate state.

- *What the map says.* There is an entry for every final IR word,
  regardless of whether monitoring is enabled or that word normally
  collects. An executing word's entry covers its live-in temporaries;
  locals are always covered because Python exposes them through
  `f_locals`. Instruction monitoring can therefore be enabled after
  compilation without constructing new maps. `_PyRegCfg_Liveness`
  returns virtual-register sets even after compaction: the compiler
  must project through `_PyRegCfg_Slot()` / `cfg->slot`, unioning live
  registers that name the same physical slot. Assembly's relaxed
  FOR_ITER and SEND add synthetic `IR_JUMP` words. Their entries use
  the live sets on the corresponding outgoing edges, not the parent
  instruction's live-in set. Final-word reconstruction below is the
  authority; compiler-projected sets are checked against it.
- *Root states.* Normal dispatch publishes the executing word's key
  before any monitoring callback or collecting helper. RERAISE keeps
  that key when it restores an earlier diagnostic position; it must
  not select the earlier instruction's map. Before `monitor_handled`,
  exception unwinding publishes the handler-entry key after writing
  the exception and lasti slots. Exiting without a handler publishes
  a locals-only state before `monitor_unwind`, with the pending
  exception anchored separately. A completed frame, including one
  copied by `take_ownership()`, also uses a locals-only state; frame
  metadata remains traced by `_PyFrame_Traverse`. It must not retain
  RETURN's operand merely because that remains its diagnostic position.
- *Suspension and calls.* A suspended frame uses a distinct key for
  the resume edge: live-across-yield temporaries remain covered, but
  the unwritten send-result slot is excluded. The yielded value stays
  rooted by its transfer span until the recipient owns it; it is not
  kept indefinitely by the suspended frame. Resume first roots the
  incoming value, writes its destination, then publishes the executing
  entry. `throw()`, `close()`, yield-from and coroutine paths each name
  their normal or exceptional edge before they can collect. A caller
  waiting for a callee retains the call-entry key; its early result
  and any replaced call-window slots need spans until normal dispatch
  publishes a map that covers them. Aliased caller/callee addresses
  are visited by the union of their applicable roots.
- *Values written inside a handler or helper* need explicit anchors.
  A precise C frame is excluded from conservative scanning; neither a
  C local nor a reference count is an anchor. H owns prefix-span
  conversion of `_PyEval_UnpackIterableValues()` (including starred
  unpack), coverage of call-window replacements and early returns
  (`IR_RETURN_EARLY`, `_PyFrame_SetReturnValue`), and the suspension
  and exception transitions above. It audits other helper outputs in
  the same inventory. Both `root_check.py` and `root_check_c.py` must
  separate frame-storage provenance from actual root coverage. A
  slot-derived value or store is accepted only with proven membership
  in the published map, or a covering span. A helper with no such
  membership proof must use a span. Inputs folded from mutable locals
  also need a span if reentrant code can replace their sole anchor.
- *Slots stay NULL-initialised at frame creation.* Unbound-local
  semantics need it, and it keeps every slot well formed; a dead
  temporary may then hold a stale value, a pointer to an object an
  earlier collection freed. Every reader that dereferences slots moves
  to the map: `_PyGC_VisitFrameStack`, the generator and frame-object
  traversals, `_PyFrame_CheckValues` (which calls `Py_TYPE` on every
  slot today), the verifier. `_PyFrame_CheckValues` becomes map-aware;
  a separate bit-format check may inspect excluded words without
  dereferencing pointers. Readers that copy bits (`_PyFrame_Copy`)
  must preserve the root-state key along with those bits. The NULL-based
  assertions in `IR_STORE_TEMP`,
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
- *One construction contract.* A shared decoder reconstructs a CFG
  over pristine final IR words, using handler operand/window metadata,
  normal and exceptional successors, the exception table, local/cell/
  free-variable layout, slot count, code flags and the IR schema version.
  It computes physical-slot liveness by backward dataflow to a fixpoint,
  with edge definitions for exception and send results, then derives
  the transition entries above. It validates word decoding, operand and
  window bounds, branch/handler targets and exception entries before
  emitting a map. `assemble.c`, Python and C code constructors,
  `code.replace()`, unmarshalling and frozen-code creation all call this
  routine; none requires the original compiler CFG. Changing any input,
  including only the exception table or locals boundary, rebuilds the
  table. Specialization and instrumentation must preserve this root
  semantics; a new handler that changes it needs new metadata and maps.
  No public caller-supplied bitmap is trusted. Maps are derived in
  memory rather than marshalled as an independent authority; bump pyc
  magic when this changes code-format semantics. Invalid IR is rejected
  at construction, and allocation failure propagates before execution;
  there is no all-slots fallback. Reconstruction tests must cover valid
  hand-built code as well as compiler output.
- *Lookup and size.* Rung 2 starts with direct per-word entry indices
  into deduplicated full bitsets of physical temporary slots, plus the
  transition entries. Lookup allocates nothing and cannot fail: an
  index lookup followed by a bitset scan, bounded by frame size. It
  never reconstructs deltas from the beginning of a long block. Record
  table bytes and construction cost over the suite; compression is a
  separate measured change if this format proves too large. Internal
  map format and IR semantic versions are explicit; tier 1's spill and
  virtual-frame extensions must preserve these interpreter states.
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
  by itself a finding. B captures original excluded words and their
  provenance before any poisoning. Poison eligibility is by physical
  address, outside the union of all frame maps, spans and transition
  roots, including overlapping caller/callee windows. Precise readers
  must avoid poisoned storage; B may examine its saved words with the
  conservative lookup. Poisoning tests root selection, not the
  completeness of the compiler's liveness analysis.

**Rule 3: operands are canonical, and a guard tests one form.** Note 06
section 18.1: every exact int that fits int32 is inline, canonicalised
by `PyValue_FromObject` at ingress. A one-digit exact heap int therefore
cannot be a handler operand, and the heap arm of `_PyValue_IsCompactInt`
guards against a representation leak, not a legitimate form. Rung 1
adds debug-build assertions at semantic slot writes and value-API returns,
runs the suite on the debug builds, and fixes any producer that violates
the contract. Current `_PyValue_FromObject` already canonicalizes; review 2's
unchecked-helper suspicion is stale. The bounded
[ingress audit](work/pre-I-value-ingress-audit.md) identifies value-native
function returns that trust the contract, including specialized calls that
bypass the generic result checker. No actual leak was found in that audit.
Raw roots deliberately preserve actual boxes and are outside the semantic
canonical-value domain; converting them could lose a live C reference.
Revalidate those boundaries on the integrated ROOTS/H3 source. Then the
int guards test the inline form only and the compact accessors go.
Boxed NaNs remain legitimate floats; bools and int subclasses remain
objects. No
threshold decides this; the invariant does. The handler space step 8
needs is not spent on a second int form.

**Rule 4: separate the interpreter spill from range-loop lowering.**
FOR_ITER_RANGE's cost is the iterator object: a type check through the
shape word, three 64-bit fields in memory, a narrowing check. The iterator
is a hidden temporary
that no Python code can name (the type is untracked, so
`gc.get_objects()` does not list it either). The original design deferred
a counter in the window because END_FOR and POP_ITER would then see two
forms of the loop. That representation change remains unapproved until
a reviewed lowering preserves one loop protocol and its exception,
monitoring and root contracts. Independently, H1/H3 investigate the
recorded C register-base spill before checkpoint I. Removing that spill
does not require a second loop representation or a JIT.

**Rule 5: the handler unit follows the build's frame-pointer switch, and
the gate builds omit frame pointers.** configure defaults to
`--with-frame-pointers` so that native profilers can unwind without
DWARF; the fork's walk (note 11) and its profiling use CFI, so the gate
builds for H configure `--without-frame-pointers` (the matrix is below;
these are future configurations), the ASan build keeps them, and the
`no-omit-frame-pointer`
attribute on `_PyEval_EvalFrameDefault` (Python/ceval.c, upstream's perf
trampolines) is removed in favour of the switch. Rung 1 confirms on the
omitted build that measured leaf handlers omit frame-pointer setup,
attributes remaining stack work to calls, spans or spills, and verifies
opaque/precise frame ownership at an actual collection in the walk's
compiled-stack fixtures. The walk's tests and zero fallbacks over the
suite are also required; neither alone proves correct attribution.

**Rule 6: the eval-breaker test stays** on every backward jump and at
every call. It is the one place signals, the collector's drain and
instrumentation requests are noticed; a dispatch-table swap would be a
second mechanism for the same job.

## 3. What this step is not

It is not tuning. No handler is rewritten for a benchmark; every change
is a generator rule, a compiler table or a build flag, and applies to
all handlers. The record at each rung: the micro set and the kernels
(instructions and elapsed execution time), allocation and box counts,
the table's bytes over the suite's code objects, the
compile-time cost of the table, the collector's root-phase time and
pause distribution, and RSS. Compare each rung against its direct
predecessor after 10a; keep the 6d comparison as historical context.
Include long basic blocks and large live sets when measuring map lookup.
Attribute regressions where evidence permits and retain uncertainty
with an owner where it does not. Intermediate correctness rungs need no
unrelated tuning. Checkpoint I separately requires matched release
elapsed-time evidence and closure of persistent interpreter deficits;
recording a regression here does not defer its disposition to the JIT.

## 4. Order and dependencies

H1's frame-passing comparison needs qualified checker/walk/build evidence,
including frame attribution at an actual collection; it does not require
H2's liveness maps. Rule 3 first proves canonical ingress, and rule 5
qualifies the changed frame-pointer policy. H1 can proceed once those
dependencies are clear.

H2 follows 10a rung 3: rule 2 reads frame slots by the map, which needs the
eval loop's own C frame precise and the walk attributing it, and the
spans with initialised prefixes that rule 2 relies on for in-flight
outputs are 10a's. Before step 8: the templates copy the handler bodies,
and tier 0's safepoint entries associate machine addresses with rule 2's
root states (one schema; tier 1 extends it for spills and virtual frames
in step 9). Call-window replacement and producer placement need the H1/H2
alias/liveness and initialized-span contracts before changing when roots
are published or overwritten.

Two rungs. Rung 1: rules 1, 3 and 5, generator, assertion and build
changes, with the micro set recorded; about one day. Rung 2: rule 2,
the compiler table, the readers' inventory and conversion, the verifier
extension, the clearing sites classified and the retired ones deleted;
about two days. Each rung goes through the loop of PROCESS.md section 1.

User revision, 2026-10-29: these implementation rungs are H1 and H2.
Add H3, an explicit inventory and design of orthogonal interpreter
operations and dispatch costs before tier 0. Scope: remaining 8-bit
handler-table assumptions in the 16-bit IR, typed arithmetic and
conditional branches, frame passing/call windows, range iteration and
the recorded register-base spill. Preserve each operation's exceptions,
monitoring and root-state contract. The inventory may bring a tier-0
hand-off into the interpreter when it has a reusable lowering and no
second loop protocol; rule 4's current deferral stands until that design
is adopted. This is not approval for arbitrary fused instruction pairs.
H's results feed the pre-JIT parity checkpoint I in PROCESS section 0.4;
elapsed-time evidence is required alongside instructions. Re-estimate
H1/H2 after the root-reader inventory and measure the new H3 scope.

## 5. Tests

H's required x86-64 matrix: GCC release and debug switch-dispatch builds,
Clang release and debug tail-call builds, all without frame pointers;
the ASan build keeps frame pointers. Record exact compiler versions and
flags at execution. Kill mode is debug-only (`gc_trace.c` enforces it),
so the missing-root controls run on GCC debug, Clang debug tail-call
and the debug ASan configuration. Release builds run the semantic
probes without claiming kill coverage. Each child explicitly forwards
requested collector settings and asserts its active verification mode.

- Rule 1: the generator test that every handler's frame load is present
  exactly when the body depends on the frame; the existing position
  tests (tracebacks, `f_lineno`, test_external_inspection) stand since
  publication is unchanged.
- Rule 2: section 2's separate missing-root and retention probes, with
  a map bit removed or added as their respective controls. Both record
  the actual state key, physical slots and other roots at collection;
  an eventual result alone does not establish root coverage.
- Root transitions: collect in a RERAISE callback after reclaiming a
  map-excluded temporary; selecting diagnostic `instr_ptr` instead of
  the root key must fail. Separately save `sys._getframe()`, return a
  fresh temporary, discard the result, and show that the completed
  frame does not mark it. Neither probe substitutes for active-frame
  safety. Exercise handled/unhandled exceptions and yield/resume/send/
  throw/close, recording keys and slot coverage at each collection.
- Helper coverage: a negative fixture in each checker stores a fresh
  object in a map-excluded frame slot, collects and uses it. It must
  be rejected without a span. Dynamically collect on unpack's second
  `__next__`, record output-prefix coverage and alternate roots, and
  remove the span as the failing control. A separate early-return
  probe collects between publication and caller resume, with its own
  span-removal control. Checker fixtures do not validate runtime maps.
- Entry completeness: enable INSTRUCTION monitoring after compilation
  and warm-up, collect before a normally non-collecting word with a
  live temporary, and record the selected entry. Removing that entry
  must fail. Exercise relaxed FOR_ITER and SEND synthetic jump words
  separately, with event records; the ordinary monitoring case does
  not cover them.
- Construction: marshal and frozen-code round trips and Python/C code
  constructors collect during execution. Replace words, slot/local
  layout and only the exception table in separate cases. For the last,
  make an exceptional successor need a temporary, collect in the
  protected region, and record coverage; reusing the old map must fail.
  Malformed operand, window, target and exception entries have separate
  rejection cases. Also validate derived map bounds and index entries;
  semantic round trips cannot establish those checks.
- Poisoning: at injected collection, save B's original excluded words,
  then poison only globally uncovered physical addresses. Include an
  overlapping call and a partially filled unpack span, asserting that
  both retain their covered words. Ignoring either aliasing or the span
  must fail. Exercise frame creation, argument binding, ownership
  transfer, unwinding and completion. This checks poison selection and
  reader discipline; the independent liveness controls remain required.
- Rule 3: canonical-form assertions over the debug suites before the
  guard change; a bypassed-ingress control trips the assertion. Warm
  changed integer handlers with C-returned exact ints around +/-2**30
  and the int32 boundaries; exercise overflow, large ints, bools and
  subclasses. Record operand encoding and selected handler at execution.
  Include boxed NaNs. These cases cover their ingress paths, not every
  producer in the core.
- Rule 5: inspect measured leaf handlers for frame-pointer setup;
  attribute remaining stack work. Compiled-stack fixtures assert the
  owner and policy of opaque roots at collection; zero fallbacks over
  the suite are additional evidence. These tests cover only the exercised
  compiler/ABI layouts.

## 6. Implementation decisions

The starting encoding, lookup bound, construction failure path and
version contract are in rule 2. There is no new size limit rejecting
valid code. `_PyFrame_CheckValues` is map-driven; excluded pointer bits
are never dereferenced. Rung 2's work doc must inventory every collector,
debug reader, transition, helper output and clearing site before any
clearing stores are removed. The two-day rung estimate predates that
inventory; re-estimate from it rather than treating the estimate as a
correctness deadline.

## 7. Review response (round 1)

| Blocking issue | Change |
|---|---|
| 1. Instruction liveness does not describe roots written inside a handler | Rule 2: explicit spans cover in-flight outputs; both checkers require root coverage, not mere storage provenance. Round 2 corrected the claim that a precise C frame was itself an anchor |
| 2. Safepoint-only entries omit suspension and instrumentation | Rule 2: every final word has an entry before instrumentation; suspension has its own root state excluding the unwritten send slot. Rule 1 preserves diagnostic publication |
| 3. Removing initialisation breaks other slot readers | Rule 2: slots stay NULL-initialised; dead slots may be stale; every dereferencing reader moves to the map, inventoried in rung 2 |
| 4. The A/B oracle confuses reclamation with missing roots | Rule 2's verification rewritten as two event probes with negative controls; B extended to excluded slots with provenance |
| 5. Rule 3 preserved a representation the value contract forbids | Rule 3 rewritten: canonical operands asserted and enforced at ingress; the heap arm goes; no threshold |
| 6. Rule 1 conflated frame access, publication and observation | Rule 1 rewritten: frame access by dependency; publication unchanged, with the observers named |
| 7. The table's lifetime contract was missing | Rule 2: one derivation routine for compiler output, constructors, replacement, marshal and frozen code; all semantic inputs validated, no trusted supplied map and no fallback |

Non-blocking: rule 5 now states the switch, the default, the attribute
and the intended gate builds' configuration; the liveness claim is
corrected in round 2 (virtual sets projected through the slot mapping,
then checked against final-word reconstruction). The record lists table
bytes, compile time, root-phase
time, pauses and RSS; rule 4's rationale is corrected (the iterator is
unnamed and untracked; the objection is the second loop form).
Questions: no size limit that rejects code; H commits the interpreter's
schema, which tier 0 uses by address and tier 1 extends with a version;
the clearing sites are classified in rung 2's work doc as semantic
(kept) or collector-only (retired).

## 8. Review response (round 2)

Coordinator disposition, 2026-10-29: accept all five blocking findings.
The changes below resolve the specification issues; their proposed
evidence remains an implementation gate, not a passed result. No third
design review is implied (PROCESS section 1 limits design review to two
rounds).

| Blocking issue | Contract change and evidence |
|---|---|
| 1. Diagnostic position can name a different root state | Independent root key; explicit RERAISE, unwind, suspension, resume and completion rules. Section 5 separates the callback, active-frame and escaped-frame probes |
| 2. Slot provenance is not root coverage | Both checkers require map membership or spans; H owns unpack, early-return and transition conversions. Static fixtures and dynamic span-removal controls are distinct |
| 3. Entries cannot depend on instrumentation being enabled | Complete final-word table at construction; explicit virtual-to-physical projection and synthetic-word edges. Late monitoring and relaxed loops have separate controls |
| 4. Map inputs and reconstruction were incomplete | Shared final-IR decoder and dataflow for every constructor, including exception edges and local layout; no independent marshalled bitmap. Exception-only replacement and malformed-input tests are separate |
| 5. Poisoning map-excluded slots corrupts valid spans or aliases | Address-based exclusion from the union of all roots, with B captured first; overlapping windows and partial output spans have explicit controls |

Non-blocking findings are accepted: rule 5 tests frame-pointer setup in
leaf handlers and actual root attribution; section 5 names the build
matrix and restricts kill claims to debug configurations; integer tests
record encodings and selected handlers; measurements include elapsed
time and the direct predecessor; the debug slot checker is map-aware.
Rule 1 uses elapsed time to choose the frame convention. Rule 2 bounds
lookup and names its allocation-free path and version obligations.
PROCESS section 0 is reconciled with these decisions.
