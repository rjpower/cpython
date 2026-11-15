# Grouped interpreter review61: evidence and limits

Review date: 2026-11-14. This supports
[interpreter-parity61](../../design/proposals/interpreter-parity61.md).
It changes documentation only. No build, generation, test, native execution or
measurement was run for this review. Retained native text, source and retained
JSON were inspected. The coordinator owns implementation and measurement.

## Identities and prior outcomes

Canonical documentation is `/home/power/code/cpython/.worktrees/python-jit`.
Its runtime sources are stale. Source facts below use selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0` in
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/call55-integrated`.
Upstream is sibling `upstream-3a440b6`, pinned
`3a440b65695615e9fd9912342b79cdd54fced0d2`. No moving external implementation
is used as authority for this cycle.

Evidence root E59 is
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59`.
The selected ELF retained under sibling `call-model55/comp57-dictionary-native/python`
has SHA256 `db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
Native addresses below refer to that image, including retained extracts in
`E59/parity60-engine1-static-review/selected57-*.txt`.

All five round59 executable trials lost their broad screens; none is selected.
The [trial report](report.md) records identities and full results. Final-output
binding lost coroutines/spectral; separate builders lost DeltaBlue/logging;
builders plus Invoke lost DeltaBlue/logging/spectral. Isolated Invoke removed
the duplicate clear and reduced the FAST native frame from 64 to 48 bytes, yet
lost coroutines 8.1%, logging 12.6% and spectral 14.2%. Continuation classification
lost DeltaBlue 15.5%, generators 5.5%, logging 11.5% and spectral 10.1% despite a
small comprehension improvement. These are real counterexamples to selecting
source simplifications from a local instruction count.

Round60 engine `ec230e55641f646681d4961583595236465efe62` passed both 117/79 builds
and 465 focused methods per role. Its two-pair screen rejected all eight cases:

| Case | Candidate / selected57 elapsed |
| --- | ---: |
| comprehensions | 1.218648 |
| DeltaBlue | 1.338757 |
| coroutines | 1.185821 |
| generators | 1.438227 |
| logging | 1.599627 |
| raytrace | 1.300208 |
| spectral_norm | 1.291620 |
| nqueens | 1.302795 |

The [engine native review](engine-post-review.md) resolves the actual mechanism:
4,472-byte engine stack reservation, stack-backed frame/tstate/PC/word/slots/table,
one tail-merged indirect dispatch, and 55,424 additional ELF text bytes. SEND
also outlines result publication. It does not prove why GCC allocated state
that way or attribute elapsed loss exclusively to spills or branch prediction.
Selected handlers already use NCSR; they do not pay ordinary SysV r12–r15
save/restore at every dispatch. This review rejects a compiler/flag/alignment
search as the next architectural plan.

## Source-to-native findings

### 1. Facts exist but some consumers reconstruct them

Selected `Python/pylifecycle.c:913` initializes every common-table entry by
immortalizing its object and constructing canonical PyValue bits. The table
includes inline minus one as well as immortal pointers. `Python/pystate.c:713`
clears the table during teardown. `Python/bytecodes.c:1862` still DUPs a table
entry and its generated publication uses the generic writer.

Retained `LOAD_COMMON_CONSTANT` native at `0x35d080` has an ownership test at
`0x35d0b4–0x35d0c3`, a destination-locality comparison at `0x35d0d7`, and pointer/
numeric-type normalization tests in the temporary arm at `0x35d1a3–0x35d1e6`.
Pinned upstream `Python/bytecodes.c:1983` uses `PyStackRef_DupImmortal`.
The selected source establishes a stronger table contract than its generated
consumer uses. This is a concrete first producer receipt, not a claim that all
native results or all co_consts are no-count values.

The [common-constant proposal](common-constant-proposal.md) closes the first
increment: one typed table getter, current-result receipt and existing generic
destination writer. Root has source-peer GO for that increment and an
implementation is underway; no measurement exists yet. TEMP refinement is
separate. Existing consumers are `Tools/cases_generator/ir_error_effects.py`
typed result properties, `ir_generator.py:168` ExecutionVariant and
`IRStack.store` around line 3003. Reuse their current-value and invalidation
rules; do not create an opcode-name shortcut or a new alias lattice.

Selected ADD_INT native at `0x367370` proves the distinction between source
clutter and executed work. Its guards/arithmetic lead to locality tests for B
and C and local/scratch/discard classification for A around
`0x3673fb–0x36743d`. Inline input decrefs have already disappeared. Source
`IR_CONSUME_INPUT`/CLOSE occurrences are not independent native charges.
`phase_state` assignments and debug home scans also cannot be counted merely
because they appear in generated C. The ordinary local replacement's unknown
old owner can finalize and remains an effect.

SHAPE_LOCAL_TEMP is the negative control: its admitted local read and temporary
canonical result already avoid much of this work. The proposal must not wrap
it in a new dynamic proof lookup. Physical role selection is cold image work;
the first variant budget is one TEMP form plus the existing general form, not
all input-type/home/destination products.

### 2. Remaining borrowing has a real consumer and a different ABI

Selected LOAD_FAST_BORROW uses DUP plus IR_STORE_CANONICAL_DST. It does not pay
generic numeric normalization. `Python/regcfg_passes.c:154–310` already folds
eligible single-consumer local reads using borrow masks and effect/local-write
checks. Call windows do not have the necessary ordinary borrowed-field format.
The prior closed COPY-web census of zero does not imply no remaining call
preparation or multi-use borrowing opportunity.

Pinned upstream `Include/internal/pycore_stackref.h:546,638,672,699` supplies
Borrow, DUP, MakeHeapSafe and CLOSE. Borrow marks a nonowning reference; DUP and
CLOSE respect the reference tag; escape materializes an owner. Its admission
is documented and implemented in `Python/flowgraph.c:2867`: supporting locals
must survive until consumption, f_locals replacement retains old bindings,
borrowed callee locals are restricted to arguments, and RETURN/YIELD or frame
escape becomes heap-safe. This is a coordinated ownership protocol, not just
an unchecked local read.

Selected `pycore_value.h:206–220` gives bit 0 the immortal meaning in canonical
stored values; C borrows retain the owner bits. Persisting mortal borrowed
cells would require a distinct ABI with admitted frame traversal, overwrite,
locals proxy, copy/escape, return/yield and activation-anchor readers. No such
change is smuggled into the first dialect packets. Required acquisition of an
owning result is not charged as avoidable duplication.

The actual `_add_widgets` image has word45 LOAD_FAST_BORROW A19←B4 followed by
word46 CALL_METHOD_DESCRIPTOR_FAST A17/B17/C1, each 648,000 monitored visits
(108 per tray). A two-word preparation/call consumer could write the required
snapshot directly to its argument home and continue the existing CALL body.
It removes a dispatch and generic destination choice; it does not remove a
necessary acquisition or introduce borrowed callee cells. This remains a
follow-on experiment, with no measured saving and no new provider fast path.

### 3. Whole creation has an unnecessary intermediate activation

Selected `_INIT_CALL_PY_EXACT_ARGS` (`Python/bytecodes.c:4353`) uses overlapping
arguments for ordinary thread-frame callers. For a heap generator/coroutine
caller it reserves an arena child and transfers arguments into it. The child
pins its function and bound code independently. RETURN_GENERATOR at line 5270
allocates the actual object, calls `_PyFrame_CopyCreatedGenerator`, then leaves
recursion, pops the arena frame and publishes the created object.

`Python/frame.c:191` has an ALL_CREATED_EMPTY path, but it still copies the
header and local owners and clears their source homes. `pycore_interpframe.h`
defines CopyHeader and Initialize, including code/function ownership, constants
and NULL homes. Pinned upstream also starts a frame and runs RETURN_GENERATOR
(`Python/bytecodes.c:4573,5806`); eliminating this staging can improve on both
models. It is not correcting a feature missing only from selected.

`Objects/genobject.c:1178` make_gen allocates a variable-sized GC object, sets
FRAME_CLEARED and f_executable NULL, acquires name/qualname after allocation,
then tracks it. `_Py_MakeCoro` selects the kind from its independently bound
code argument. Coroutine origin tracking uses the temporary incomplete frame
to find its caller. First direct creation therefore falls back for nonzero
origin depth and other observer modes until a complete alternate consumer exists.

Direct creation must snapshot **function, bound code and every argument owner
before allocation**. GC callbacks can mutate func_code or caller locals. A code
pin alone is insufficient, as is reacquiring the current local after allocation.
Use existing `_PyTransitionRoots` staged/external-prefix/result facilities and
call refresh/redirect handling. Preserve partial-object traversal and read
name/qualname after allocation as before. No stale arena pointer may survive
an effect. The existing frame's bound target must not be reselected after a
callback mutates its function.

Pre-allocation protection does not require duplicate owners for all inputs.
Selected already transfers scratch owners into the temporary arena child before
allocation. A direct path may transfer them into the existing external staged
prefix instead, acquire local snapshots once, and clean up the committed stage
on failure. Alternatively it may retain inputs until a later commit if a reader
requires that ordering. The implementation peer must close the chosen CALL
error/observer state and price any duplicate acquisition. Final heap publication
after allocation is distinct from consuming the caller's input owners; exact
upstream destruction timing is not the governing contract.

The first exact coroutine image has initial RETURN_GENERATOR and no proposed
defaults/keyword/closure binding complexity. Admission still checks the actual
image, ALL_CREATED_EMPTY and resume recipient; flags alone are insufficient.
The comprehension genexpr inspected here begins with RESUME/LOAD_FAST/GET_ITER,
so its creations are excluded from that initial entry-image contract.

### 4. Delegated yield repeatedly transports the same owner upward

Selected `_YIELD_DELEGATED` (`Python/bytecodes.c:1788`) is lowered by
`ir_generator.py`'s `suspend_delegated` emitter to a full frame suspension.
Generated YIELD_DELEGATED around line 25497 takes and clears the outgoing home,
updates resume PC, exception linkage and recursion, detaches the frame, publishes
the delegated phase/state, stores the owner into the parent's recipient, then
dispatches. Another YIELD_DELEGATED immediately takes and clears that recipient.

The proposed checked ascent loop retains all real frames and every state
transition, carrying that owner between successive eligible parents. Admission
must prove recipient identity and vacancy from the SEND/continuation contract,
not compare slot numbers alone. Instrumented successors, redirects, hooks,
unsupported recipients and any required callback/poll stop the composition.
The source peer must close remote/phase readers over the C-owner interval;
reuse an existing transition result root if needed and count its cost. This
contract is proposed, not an implemented proof or measured gain.

## Coverage and cost limits

The proposal's eight-case table was derived from retained
`E59/resident-selected57-{case}.json`; the census adapter identity is
`37f743dd97d863fd881fc21d7191641e8bc59755dbab92150beef3261ad5cbc7`.
See [resident coverage](resident-coverage.md) for collection and limitations.
All values are monitored semantic visits weighted by pre-monitor adaptive
labels. They are not natural specialized-hit counts, admitted variant counts,
or native instruction attribution. Some library execution is outside the
recorded kernels. Initial creation before RESUME can be uncounted.

The coroutines image contains 971,141 CALL_NO_SELF_PY_EXACT_ARGS visits and an
initial RETURN_GENERATOR with zero monitored visits. The generator-tree image
has 2,937,892 YIELD_DELEGATED visits, 3,137,890 SEND visits and 200,000 leaf
YIELD_VALUE visits. These make creation and ascent substantial candidates;
their admissible populations still require actual implementation diagnostics.
Do not infer that the ascent packet helps coroutines: their recorded
YIELD_DELEGATED count is zero.

Comprehension selected57/native ratios are 1.436416 instructions and 1.333179
cycles versus pinned upstream. Parity requires eliminating 30.38% of candidate
instructions or 24.99% of candidate cycles at that interval, not 43.6% of the
candidate. The retained estimate is 278,517 versus 193,897 instructions per tray.
The 57 common loads per tray cannot carry this budget. Nor can the 108 remaining
call-preparation pairs alone. No closed per-category attribution accounts for
all 84,620 excess instructions; this review does not invent one.

Selected dict.get is METH_VALUE through a non-NULL value-vectorcall provider;
selected LIST_APPEND directly calls its consuming append helper. Counts are
108 descriptor-provider calls, 162 direct appends, 54 dict subscripts and 42
predicate calls per tray. Generic object-array boxing is not the explanation
for all these operations. Provider checks, recursion/entry work, call preparation,
publication and dispatch remain possible contributors, but rejected57's checked
provider/private append rewrites are not rescheduled unchanged.

The actual source comparison is selected `Python/bytecodes.c:5010` calling
`_PyValue_VectorcallTstate`, then `Objects/descrobject.c:602` through
method_value_vectorcall_descr's argument/keyword checks, method_value_invoke,
method_enter_call and the value clinic implementation. Upstream
`Python/bytecodes.c:5290` extracts the checked descriptor's cfunc and calls
`_PyCallMethodDescriptorFast_StackRef`. Their cleanup DSL phases form a single
generated handler in each interpreter; they are not separately dispatched
words. Upstream LIST_APPEND at line 1382 calls `_PyList_AppendTakeRef` directly;
selected's line 1239 likewise has a direct consuming append path. This is a
source-path distinction, not a newly measured attribution of their native costs.

The selected common path is therefore faster upstream for several concrete
reasons—borrowed argument/reference protocols, specialized providers and cheaper
physical transport in applicable paths—but no measured allocation of the whole
deficit among them exists. Selected has compensating strengths in canonical
inline arithmetic, shape reads and overlapping exact calls. Architecture review
must compare complete paths rather than assume every design difference is loss.

## Disposition and executable order

1. Finish the tied common-table producer receipt already underway; inspect
   native removal and broad screen. Admit TEMP role separately. Extend only
   where a source fact and surviving native checks establish a consumer.
2. Implement direct exact coroutine creation with the bounded entry/binding/
   root contract, then separately admit supported generator images. Require
   full-path native accounting and failure/observer qualification before screen.
3. Implement checked delegated ascent after closing recipient and reader phases;
   charge every guard and any transition root against removed publication and
   dispatch. Keep downward SEND unchanged for this packet.

The pending scalar proposal is not selected or credited. A fixed pair with
original PCs and defusion may be economical; a vector/reverse-index/generation
framework is not a prerequisite for these three consumers. Persistent borrowed
callee cells remain a larger explicit ABI alternative, not an implicit part
of the owning-frame rollout. An eight-byte physical encoding remains revisable
when a concrete consumer requires it; none of the first consumers needs an
extra per-word descriptor.

Root should retain selected57 controls, screen all eight cases after each
candidate, and perform full supported regression/benchmark qualification at a
coherent winning checkpoint. No losing packet becomes a prerequisite for the
next. Current evidence supports these as concrete discriminating experiments;
it does not establish a complete parity budget or predict a 2x gain.
