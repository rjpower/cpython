# Integration C: independent cold source review

2026-11-06. **Accept `7780e32e0e2` on source correctness; no blocking finding.**
Reviewed against parent `ba37cc10058` in the runtime worktree
`python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`. The first pass read
the actual four-file diff, generator helpers, all five actual attribute producer
flows and their generated C before this increment's proposal or prior reviews.
Runtime source stayed clean and untouched. Source references below refer to
that runtime, not the canonical documentation checkout.

## Output ownership and reentry

`IREmitter.retire_published` (`Tools/cases_generator/ir_generator.py:2339`)
clears consumed inputs and flushes outputs before receiver retirement. The
existing five-load branch selects the local-only failure result; C now uses
that same branch to establish the existing `published_temporary_result`
postcondition. The final flush (`2081`) consequently stores only a folded
local destination. No second opcode list or general publication cache appears.

For a temporary destination, `IR_STORE_TEMP_DST` publishes the canonical
attribute into its exact frame home before retirement. That home owns the
reference; the C variable carries the published bits without another owner.
`retire_published_values` (`Python/ceval.c:1929`) retires detached receiver
owners and boxes, then refreshes state. It does not rewrite the result home.
Suppressing the final repeated publication neither removes an owning root nor
requires a new duplicate or release. Input/destination overlap works because
the temporary receiver is consumed and its home cleared before publication.

For a local destination, the first publication macro leaves the old local
untouched. The C attribute retains its owner until the final `IR_STORE_DST`.
A receiver finalizer therefore sees the old local; the displaced local's
finalizer sees the new result. If the receiver was read through that local,
`OWNER` yields NULL and the local keeps its original owner until replacement.
On retirement failure, the helper receives the C-held attribute only for a
local destination and closes it through the existing failure path. For a
temporary destination it receives NULL because the frame/unwinder owns the
result. C changes none of these error or displaced-local paths.

The generated INSTANCE_VALUE, MODULE, SHAPE, SLOT and WITH_HINT handlers each
retain one pre-retirement publication and one guarded final local store. Their
post-retirement operation is only `_PUSH_NULL_CONDITIONAL`; its companion is
the distinct destination-plus-one home. They do not reassign the attribute,
switch frames or select another destination before final completion. The
generated diff changes exactly five final-store wrappers and no metadata.

After a callback, the handlers still reload instruction word and slot base.
`value_operation_refresh` (`ceval.c:1341`) checks the active frame/executing
copy and restores the instruction PC. `_PyIR_SetHandler`
(`Include/internal/pycore_ir.h:485`) preserves all operand fields when
instrumentation or specialization replaces the handler. Thus a refreshed word
retains destination identity and local/temporary classification. Public frame
locals expose local homes, not arbitrary transfer temporaries. Skipping a
second temporary write does not weaken the existing refresh contract.

## Generator and test quality

The reused flag is a narrow success postcondition. It is set after the
required flush on every successful path through these five retirements.
`Stack.copy` uses a shallow object copy before copying mutable collections;
the boolean therefore survives copies used by the conditional companion
operation. All such outgoing paths already have the same publication fact.
There is no new mixed-path merge or intervening attribute assignment in this
packet. This reasoning does not license the flag for arbitrary future output
mutations, windows or conditional retirement flows.

The generator fixture now covers all five names and checks initial publication,
local-only failure ownership, the companion NULL and final local/displaced
handling. The semantic regression uses fresh receivers with SLOT and SHAPE
layouts and checks both compiler-selected destination classes. Receiver
destruction reenters Python and collects while the payload must remain alive;
events also check old/new folded-local observation. Weakrefs and the final
death assertion detect unintended retention. The test records callback results
for assertions outside finalizers, where failures cannot be swallowed.

Installed-handler checks before and after the observed call strengthen the
specialization evidence. They are not an exact executed-handler counter or a
proof that no guard fell back. Behavioral coverage is SLOT and SHAPE; coverage
of the other three producer sequences here is source/generator inspection.
Those limits are acceptable for this shared emission change.

## Composed call/frame contract and follow-up

The bounded broader read found the batch coherent. Common positional calls
prepare and refresh while caller homes own inputs, then commit without an
escaping operation before publishing the child (`ceval.c:3057`). Exact calls
use the same `_PyFrame_PushOverlapping` primitive. Its initialization boundary
uses the caller's logical slot extent, checks the skipped non-argument prefix
for NULL in debug, initializes the newly exposed tail and fills child constants.
The compactor preserves held-owner occupancy through consumption, including
values with no future read (`Python/regcfg.c:789`, `887`). The live-across-call
verifier supplements that invariant; it does not replace it.

Ordinary thread retirement already asserts clear temporaries and clears only
locals (`Python/frame.c:185`). Frame moves and generator transfer preserve the
all-homes-valid rule. C leaves those ownership boundaries unchanged. The old
declined-adapter assertion defect identified during A review is corrected in B:
only a successful child push requires executable-identity assertion.

Keep the shared overlap primitive and the narrow completion postcondition.
The useful next question is which common call shapes and remaining generated
publication/helper sequences actually dominate after this batch. Establish
executed path frequency and retained GCC work before widening the mechanism.
If another producer needs publication reasoning across mutation or conditional
effects, derive it from one operation contract with explicit consume, publish,
retire and observation phases. The current change needs no distributed state
tracker, runtime liveness map or binder rewrite.

## Executed evidence and limits

I read the coordinator's retained build/test summaries and log endings under
`build/pyjit-link-wave1/interpreter-throughput-20261106/`: three successful
builds, 117 modules each; 729 focused tests per release/debug profile with
11/10 skips; two generator unit tests; the revised semantic unit in each
profile; and value API lint over 306 units with zero failing findings.
These are coordinator-executed results, inspected as evidence, not independent
execution by this reviewer. The release compile requirement is supported by
the actual release build receipt.

This review ran only read-only source/Git/log inspection and wrote its two
canonical review files. It ran no interpreter, test, generator, build,
disassembly or performance command. It establishes no speedup or full-suite
qualification. Performance acceptance and broad merged validation remain the
coordinator's gates; source correctness does not block those next steps.
