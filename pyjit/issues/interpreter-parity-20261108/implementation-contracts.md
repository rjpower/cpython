# Structural increment implementation contracts

2026-11-08. For coordinator peer review before runtime edits. Source is R5
`0953757f4e7` plus the separate P1/P2 increment in the isolated runtime
worktree. This document selects precise bounded forms from the broader
[mechanism comparison](mechanisms-proposal.md). No runtime edits, builds or
native workloads were performed to produce these contracts.

Implement M1 and M2b as separate reviewable increments. M1 removes generic
dictionary-reader machinery on a guarded canonical hit; M2b removes a tail
dispatch for a retained ordinary NOP boundary. Neither changes the IR word
format, call windows, frame representation, kwargs lifetime or Python code.
Do not implement the speculative M3 cached index or M2 return/discard path in
this batch. M1b remains a cheaper class/publication comparison if M1's guards
or helper ABI consume the expected savings.

## M1: dictionary first-probe hit and terminal leaf continuation

### Dictionary-owned provider

Add one internal provider, declared in `Include/internal/pycore_dict.h` and
implemented in `Objects/dictobject.c`:

```c
PyValue _PyDict_TryGetFirstProbeValue(PyValueBorrow container,
                                    PyValueBorrow key);
```

The signature is intentional: two borrowed values in registers, one owned
value returned in a register; no output-address argument or retained pointer.
Call with explicit `PyValue_Borrow` where an owning value is supplied, following
the internal value API conventions. Do not add a public C API or accept an
owning input merely to close it inside this accessor.

The caller keeps input owners alive for the entire call. The provider returns
one canonical owned value on hit, including a real increment for a mortal
value. NULL means **decline**, with no acquired owner or mutation. It does not
mean a missing-key error. Leave the pending exception unchanged on every path;
the provider cannot allocate, raise, call Python, poll, release an input owner
or invoke monitoring. It reads no frame state.

Implementation order:

1. Under `Py_GIL_DISABLED`, return NULL before touching table internals. The
   first increment does not add a concurrent dictionary protocol.
2. Require an object-valued container, `PyDict_CheckExact`, and
   `PyValue_IsInlineInt(key)`. Frozen dictionaries, subclasses, boxed wide
   integers, bool keys and every other key form decline.
3. Read `ma_keys`; require `dk_kind == DICT_KEYS_GENERAL` and `ma_values == NULL`
   (a combined table). Other layouts decline.
4. Compute the exact integer hash through existing `_PyLong_ValueHash` from
   `pycore_long_value.h`. Its guarded inline-int arm uses `_PyLong_WordHash`;
   this preserves negative hashes, -1 remapping and supported word widths
   without hand-written arithmetic or generic provider dispatch.
5. Compute the first probe `((size_t)hash & DK_MASK(keys))`, then use the
   existing `dictkeys_get_index`. Empty/dummy/negative indices decline. A valid
   live dictionary supplies an in-range entry; assert the structural bound in
   debug builds rather than inventing a corruption-recovery path.
6. Read that `PyDictKeyEntry` only. Require `PyValue_Is(entry->me_key, key)`
   and non-NULL `me_value`. Otherwise decline. Do not scan farther, compare
   equality, invoke __missing__, cache an entry or assign a keys version.
7. Assert the value's existing canonical-home invariant in debug builds and
   duplicate it with `PyValue_XDUP`. No scalar conversion or boxing is needed.
   The owned result survives return even if later input retirement destroys
   the dictionary. All entry/table pointers die inside this call.

Why first-probe identity is sufficient: generic lookup hashes this exact
inline integer without Python, examines this same first candidate and accepts
identity before equality. The proposed hit omits no earlier comparison.
`compare_generic` and `do_lookup` in dictobject.c establish that order. An
inline query alone is not a leaf proof: a collision with a user key could run
equality. Such a first candidate declines here, leaving ordinary lookup to
execute every callback once. The helper never treats bool/int equality or
mutable custom equality as value identity.

### Generated terminal edge, not a new phase engine

Add an explicit no-argument DSL marker, provisionally
`IR_TRY_DICT_FIRST_PROBE();`, as the **first statement** of
`_BINARY_OP_SUBSCR_DICT`, before either TAKE, INPUTS_DEAD, SYNC_SP or STAT_INC.
The existing provider guard preceding that uop is nonescaping. The marker's
generated hit path terminates at DISPATCH or error; decline falls through to
the original DSL body unchanged. This avoids joining partially consumed
inputs or propagating a canonicality flag through a generic provider call.

The generator must reject the marker unless all of these hold:

- Exactly one marker, at that top-level first-statement position in this
  checked dictionary-subscription producer. No aliases of the marker name,
  variable function pointer or unchecked C flag can assert the proof.
- Two scalar borrowed-input fields B/C, scalar first result at destination A,
  no dynamic arrays/window, and ordinary destination semantics. Check these
  properties through the actual format/roles rather than trusting operand
  letters alone.
- No preceding producer effect, input consumption, unpublished owner or box.
  Existing type/provider guards may precede it only through the known checked
  guard prefix. Reordering an effect into that prefix must fail validation.
- The trusted provider is exactly `_PyDict_TryGetFirstProbeValue`; its returned
  owned value is private to the generated terminal edge. Slow-path abstract
  stack state is untouched by this edge.

Register the marker with the analyzer's error/synchronization metadata and
emitter replacements as necessary, regenerate from producers, and test its
rejection cases. Keep the handler's full effects conservative: slow lookup,
input destruction and local displacement can all escape. No new handler ID,
feedback layout, bytecode format or permanent feature switch is needed.

The generator's concrete hit edge is:

1. Load borrowed container/key views from the two physical homes once, call
   the trusted provider, and branch on its NULL decline. Decline has no output
   to close and changes no home, so ordinary TAKE/provider/error code follows
   exactly as before.
2. On hit only, consume B then C with `IR_CONSUME_INPUT`, using those same
   borrowed snapshots. A temporary transfers its owner and becomes NULL; a
   local stays owning and supplies NULL retirement ownership. The supported
   dict/inline-int hit cannot have B and C alias one physical value. Do not
   insert TAKE duplication of local inputs: this provider cannot invalidate
   their borrows, and their homes remain owners throughout the read.
3. Count one BINARY_OP specialization hit. For a temporary destination, publish
   the canonical owned result through `IR_STORE_CANONICAL_TEMP_DST`. If A is B
   or C, consumption already made that input home NULL. For a local
   destination, keep its old home untouched and hold the result owner in C.
4. Retire key owner first, then dictionary owner, matching current
   `IR_COMPLETE_VALUE_READER(pending, held_sub, held_dict)`. **Immediately before
   each close**, check `_PyValue_MayDealloc`. Close a proven nondeallocating
   owner inline and set its C variable to NULL. Earlier closes can change a
   later alias's count: never precompute both checks.
5. At the first possible deallocation, pass this and remaining owners to
   `retire_published_values`, preserving order, with NULL boxes. Its
   failed-result argument is the held result only for a local destination;
   a temporary already owns its result. On failure use the existing error
   edge. On success reload `this_instr`, `ir_word` and `slots`, exactly as the
   existing published-reader emitter does. Do not reuse decoded fields or
   a raw home/table pointer across that callback.
6. If no retirement could deallocate, skip refresh and reload. All physical
   homes are canonical or NULL throughout. The same result owner is already
   installed for a temporary; do not store or publish it a second time.
7. Only for a local destination, run `IR_STORE_DST`, then the ordinary
   `finish_displaced_local` path. This publishes the new local before closing
   its old owner, after input retirement; callback-current local behavior and
   failure cleanup stay unchanged. The original instruction PC is still
   published on every fallible or observable edge.
8. DISPATCH. The generated leaf block has no continuing edge back into the
   original generic body. Its C owners are spent exactly once; a temporary
   result belongs to its home and a local result belongs to its updated home.

Extract/reuse the existing ordered retirement emission in
`IREmitter.retire_published`, which already performs sequential two-owner
MayDealloc checks. Reuse `release_displaced` for final local retirement. A
small emitter routine for this explicit terminal reader edge is sufficient;
do not introduce a general abstract effect-state framework or copy retirement
logic into a hand-maintained generated header. The full error and local-store
sequence must remain visible in the regenerated code for review.

### Correctness and discriminating evidence

Focused cases must cover first-probe hits with inline/scalar/immortal/mortal
results; local and temporary container/destination forms; self-referential
dictionary values; negative/-1 integer hashes; empty, deleted, resized and
colliding tables; boxed integers, bool/equal-numeric keys, subclasses and
__missing__; hash/equality callbacks that mutate inputs or observers; missing
keys and exceptions. An unsupported case must use the generic path exactly
once. Include last-owner dictionary destruction and callback-mutated local
destinations. The extracted generic two-owner emitter retains aliasing tests
even though this particular dict/inline-int input pair cannot alias.

The cheap discriminator is first-probe hit/decline classification, then GCC
code showing no generic `value_subscript`, generic reader completion or state
reload on the accepted nondeallocating hit. Retain separate helper-hit and
retirement counts. Root owns release/debug compilation, native diagnostics,
target/control timing and post-review. A hit that still pays the original
generic machinery, poor hit coverage, extra spills or no repeatable useful
target effect rejects expansion; it does not justify implementing M3 by default.

## M2b: inline the ordinary NOP boundary at one boolean continuation

### Exact source equivalence

R5's ordinary NOP handler does only instruction-PC publication,
INSTRUCTION_STATS, next-word advance and DISPATCH. It performs no epoch check,
poll, value access, branch callback, frame switch or DTrace probe. NEXTOPARG
loads the current handler word directly; it does not inject observers before
an ordinary NOP. Line/instruction observation uses separate wrapper handlers.
`INSTRUMENTED_LINE` also retains the changed-PC/error behavior of callbacks.

Therefore compare the shortcut with **dispatching the current ordinary NOP**,
not with a hypothetical global absence of observers. Unlike IR_SKIP_RESUME,
this shortcut does not bypass a periodic/instrumentation check. No stronger
epoch predicate is needed; stale ordinary code would have run this same NOP.
Instrumentation failure recovery is unchanged for the same reason.

Initial producer selection: append one explicit zero-input/output final uop
to the ordinary `POP_JUMP_IF_FALSE_BOOL` macro. Its sole statement is the
checked `IR_EXECUTE_ORDINARY_NOP();` intrinsic. It follows the exact-bool guard, branch
history update, input consumption, resolved edge and final stack flush, just
before DISPATCH. This handler has no result destination, private boxes or
displaced local. Its boolean is immortal, so DEAD/consumption cannot run
Python. No instrumented branch handler or arbitrary DISPATCH receives this
optimization. The current handler's ordinary fallthrough and taken edges may
both use it if their live next word is NOP; there is no need for a second
taken-edge flag. Logging's taken edge is the measured opportunity.

### Predicate and action

In a GIL build, disable this shortcut when Py_DEBUG lltrace would run
(`frame->lltrace >= 5`). Then atomically load the word at the live
`next_instr`, using the same relaxed word load as NEXTOPARG. If
`IR_HANDLER(word) != NOP`, change nothing and use ordinary DISPATCH. This
includes INSTRUMENTED_LINE, INSTRUMENTED_INSTRUCTION and every other wrapper.

If the handler is exactly ordinary NOP:

```c
frame->instr_ptr = next_instr;  /* execute the retained boundary P */
next_instr++;                  /* continue at P + 1 */
```

Then run the ordinary DISPATCH to P+1. Do not delete or rewrite P, loop over
more NOPs, move the branch target or skip PC publication. No callback, owner
retirement, allocation or poll lies between reading P and completing it.
Under the GIL, another Python thread cannot install a wrapper during that
span. `Py_GIL_DISABLED` keeps the original dispatch, avoiding a new concurrent
instrumentation proof.

The intrinsic's shared emitter validates zero inputs/outputs, one top-level
intrinsic statement, final-uop placement, no trailing effect, and no live
private owner/box/displaced local. It flushes consumed-input state before
emitting the runtime macro. The macro changes only `frame->instr_ptr` and
`next_instr`; it does not change slots or the decoded word. Keep its
nonallocation/nonescape facts explicit in generator/analyzer metadata.

This makes producer selection visible in the DSL rather than hidden in an
`if inst.name` exception at every handler's completion. The shared intrinsic
may serve later reviewed zero-effect boundaries, but only this exact-boolean
producer appends it in this increment. It needs no new opcode, compiler flag
or extra test in every DISPATCH. Validate that the selected handler remains
nonescaping after its guard, has no outputs/boxes/local displacement, and still
resolves the same branch. If those conditions change, fail generation or
disable this rule.

Validate both the trusted prefix sequence and its continuing-path effects.
The trusted scalar passthrough primitive's semantic contract is exact bool
guarding (`EXIT_IF(!PyValue_BoolCheck(value))` in current source); the trusted
branch consumes that boolean, records history and resolves JUMPTO. Source
review checks those small C primitives against their contracts. Do not copy
their complete token bodies into another source string or build a general
proof engine. The generator validates sequence, scalar stack shapes, ownership,
terminal placement and nonescaping properties, rejecting new escapes, polls,
frame changes, error paths, boxes or outputs in the continuing prefix. The
guard's pre-consumption deoptimization exit remains allowed. Negative tests
remove/replace the trusted guard, insert an unknown uop or add an escaping call
and confirm rejection. A future semantic change to a trusted primitive must
update its contract and every consumer, including this optimization.

### Observer, PC and owner obligations

- **Active/new observers:** fresh target-word classification sees the actual
  wrapper after any previous callback. A plain target uses exactly its plain
  semantics; a wrapped target follows the existing line/instruction handler,
  including callback exceptions and f_lineno changes. Tracing enabled inside
  an earlier callee does not justify skipping a wrapped boundary.
- **Branch events:** the ordinary BOOL handler emits none today. Instrumented
  branch forms stay untouched and still run INSTRUMENTED_JUMP, preserve both
  branch events, honor a changed destination and use its error PC. Do not
  replace an instrumented branch with the ordinary specialized handler.
- **Epoch/failure recovery:** NOP and NEXTOPARG do not check epochs. The
  shortcut introduces no new epoch-dependent behavior and retains every
  subsequent RESUME/poll. Adding an epoch equality check is permissible as a
  conservative decline, but is not required by ordinary-NOP equivalence.
- **LLTRACE:** debug lltrace>=5 follows the original NOP dispatch, retaining
  its PRE_DISPATCH output. The P+1 dispatch still performs its own debug
  assertions. Optionally assert frame/current-copy consistency on the fast
  edge; do not run an extra lltrace callback with a forged opcode.
- **DTrace:** WITH_DTRACE currently provides function entry/return probes;
  OR_DTRACE_LINE is defined but unused. Ordinary NOP has no DTrace call, so no
  DTrace predicate is needed and no existing probe is skipped. If a future
  dispatch-level line probe is added, its producer must update this rule.
- **Asynchronous state:** publish P exactly as NOP does, retain the executing
  word copy, then let P+1 publish its PC normally. Do not defer the store or
  add a safepoint. Native sampling may have fewer C handler frames, which is
  the intended implementation change; the same sequence of published Python
  instruction locations remains available. Pending signal/breaker state is
  checked at the same existing polls, never at ordinary NOP in either path.
- **Homes:** no new owner/result is created. The boolean input temporary is
  already NULL, or its local retains ownership; other homes are untouched.
  A NOP has no operand homes, so there is no NULL result slot to fabricate.

### Stats, validation and rejection

Keep INSTRUCTION_STATS defined as actual entered-handler accounting. Do not
synthetically count an entered NOP handler when its boundary is executed in
the boolean handler. This matches the existing skipped-RESUME precedent. The
semantic NOP boundary still executes; a disposable diagnostic can count it
separately. If all ten unobserved logging targets qualify, ordinary handler
entries change from 161 to 151 per loop, with ten virtual NOP boundaries and
nine outer POP handlers unchanged. This is an accounting prediction, not an
elapsed-time prediction.

Tests must compare line and instruction event locations before/after tracing
activation inside a callee, per-code monitoring, both branch events, callback
errors and changed f_lineno destinations. Keep the NOP word/offset available
for exception tables and disassembly. Include ordinary bool branches with
NOP/non-NOP targets and tracing recursion, and run relevant debug lltrace
coverage. Inspect GCC to confirm the direct NOP handler call disappears on
hit and no extra target load/spill dominates the fail edge. Reject if the
guard replaces the saved dispatch with comparable work or regression controls
lose materially. Do not extend this to arbitrary producers merely because
NOP appears next in source.

## Comparison and scope

The reviewed route diagnostic now reports ten direct exact-thread entries and
ten accepted overlap entries per loop; all ten overlap entries create kwargs
and empty varargs, with no fallback, decline, native or instrumented loop
delta. Its partitions reconcile at 0/1,000/2,000 loops with exact scaling.
There is no hot consuming-binder argument-copy path to remove. Specialized
call-entry proof propagation removes repeated
preparation checks on accepted outer entries; it cannot remove observable
kwargs, frame lifetime or generic dictionary lookup/completion inside the
inner callee. M1 and M2b reach that inner path without changing entry ABI.
Their opportunities overlap the same disabled call, so timed savings are not
additive source shares.

M1b's temporary-only LOAD_FAST_BORROW/LOAD_COMMON_CONSTANT form can avoid
local-destination class/publication work on twenty producers per loop, while
retaining the snapshot increment and PC. It needs selected forms or compiler
home-class proof; no such class bit exists in the current word. Start with
GCC evidence that those branches survive before adding variants. This is a
valid alternative if cheaper than M1, not justification to erase PC stores
or replace all ownership-aware call windows.

Review the two actual diffs and evidence independently before integration.
Keep diagnostics outside release timing, retain failed attempts, and use one
selected runtime after each decision. No flags preserve competing shipped
designs.

## Accepted M2b scope addendum

The producer's zero-I/O `_EXECUTE_ORDINARY_NOP` uop is tier1: it is an
interpreter continuation and must not become an optimizing semantic uop.
The existing metadata generator consequently omits the entire legacy
POP_JUMP_IF_FALSE_BOOL expansion, while actual register-IR metadata is
unchanged. A future frontend consuming that currently unused legacy table
needs explicit bool-branch lowering without this suffix. See the
[review addendum](m2b-review-addendum.md), which also records the initial
observer test's dis API correction and passing retained-control comparison.
