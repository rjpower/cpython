# Attribute result publication contract

2026-11-07. Proposal at runtime `7780e32e0e2`; design-only for this owner.
First implementation budget: 60–90 minutes for a generator/analyzer result,
then coordinator review and integration. The runtime has one selected register
IR and frame-home representation. This packet declares one phase/output fact
already true of five specialized loads; it does not change their execution
order or assert an elapsed-time win.

## Problem and minimum result

`IREmitter.retire_published` recognizes `LOAD_ATTR_SLOT`,
`LOAD_ATTR_INSTANCE_VALUE`, `LOAD_ATTR_SHAPE`, `LOAD_ATTR_MODULE` and
`LOAD_ATTR_WITH_HINT` by handler name. That branch supplies a local-only
`failure_result` to `retire_published_values` and sets
`IRStack.published_temporary_result`, so final flush omits the duplicate
temporary store but retains a folded-local store. The five `Python/bytecodes.c`
producers all end their result-producing uop with
`IR_RETIRE_PUBLISHED(receiver)`. The generic `IR_COMPLETE_ATTR_READER` also
publishes the first result before releasing its receiver, through its
completion helper. `handler_contracts` and
`_PyIRFormat` currently expose word-level effects but not this phase/output
postcondition. A new attribute producer could copy the five-way pattern yet
silently miss the fact, or mutate its output afterward while the boolean
remains true.

Replace those five source markers with the explicit DSL form
`IR_RETIRE_PUBLISHED_RESULT(attr, receiver)`. It names the owning result and
the input owner to retire; the emitter derives the same local-only failure
argument and same final-store choice from the marker, with no handler-name
test. Keep the current single `published_temporary_result` emitter fact and
the existing `IR_COMPLETE_ATTR_READER` path, with the same semantic phase.
No runtime bitmap, second opcode
table, distributed publication cache or general effect framework is needed.

## Exact semantic fact

For a handler with `STORE_DST`, the declared marker says: on its successful
path, output `attr` is published canonically to the destination *if that
destination is a temporary*, before `receiver` can be released. If the
destination is a folded local, the prior home remains owning until receiver
retirement begins; the result stays C-owned and is installed only after
successful refresh. A finalizer or callback during retirement may mutate
public locals, so this fact does not promise the prior local's bits remain
unchanged across the callback. The handler does not install its result on
failure and preserves callback mutations. On success, the final store replaces
the current home and retires its actual displaced owner. This conditional
postcondition is about one named output, not every output of the handler and
not the whole operation being pure or non-escaping.

| Point | Temporary destination | Folded-local destination |
| --- | --- | --- |
| After result creation, before marker | `attr` owns the duplicate in C; destination NULL | `attr` owns the duplicate in C; prior local home remains owning |
| Marker flush, before release | `_PyValue_Publish(&attr)` writes canonical bits; temporary home owns result, C value is only an alias of those bits | Prior local remains installed; `attr` still owns the result in C |
| Successful receiver retirement | Temporary home still owns the exact published result; refreshed frame/PC/word identify the current execution; `receiver` is consumed once | The prior home remains owning until retirement begins. A callback may mutate the public local; after refresh, `attr` still owns result in C |
| Retirement or refresh error | Error path unwinds the owned temporary home; do not close `attr` as another owner | `retire_published_values` closes `attr` through `failed_result`; the handler does not install its result and preserves callback mutations |
| Final successful flush | Omit repeated temporary publication/store | Store `attr` to the current local home, then retire that displaced owner in existing order |

The callback can mutate public locals and instrumentation. It cannot reach the
temporary home through frame locals. On success, the existing retirement helper
revalidates current-frame/executable identity and restores the word/slots
before the suffix uop runs. An error never reaches final flush. The companion
`_PUSH_NULL_CONDITIONAL` output at `dst+1` remains distinct and unchanged.
`IR_COMPLETE_ATTR_READER` supplies the same *semantic* first-destination
fact through a trusted helper contract. Both the method and non-method paths
of `value_attribute_complete` publish the first result before closing the
receiver and refreshing (`ceval.c:1850–1861, 1904–1917`). The generic
lookup may call Python before it creates the result; the fact does not imply
whole-instruction purity or publication before entering the helper. The
method form also publishes a second temporary result, which this first
increment does not describe or optimize.

The proof is transitive. `OWNER(owner)` consumes a temporary owner or borrows
from a still-owning local; the generator already rejects an escaping statement
or deopt/error exit between it and retirement. The marker's initial flush
uses `_PyValue_Publish`, which may canonicalize a boxed number, before a
deallocator could run. `retire_published_values` closes detached receiver and
box owners, then refreshes frame/PC, and can fail. Fast non-deallocating close
does not need refresh, but shares the same output state. No raw field pointer
or unowned C view may cross an escaping release. A result statement that
assigns, consumes or takes `attr` after the marker invalidates this fact and
must be rejected, not silently ignored.

The phase field reports only the first output's successful temporary-home
postcondition. It is not a second callback or failure protocol. The checked
explicit marker supplies the temporary-home/error and folded-local
`failed_result` rules in the table above; `retire_published_values` and
`value_operation_refresh` implement release and revalidation. For the generic
helper, validation failures before publication close the C-owned result
without overwriting the current local home, including any callback mutation
already made. After temporary publication, a refresh error leaves the result
rooted in its home for the unwinder. For a folded local,
`value_operation_refresh` closes the C-owned failed result without installing
it over any callback mutation to the public local. The generic method form
requires temporary output homes and publishes both there before closing the
receiver. Existing
`_PyIR_RAISES`/`_PyIR_RUNS_PYTHON` effects remain whole-handler summaries;
neither those bits nor `result_phase` alone authorizes moving a release or
skipping a refresh. A later JIT recovery contract must encode failure homes
and callback checkpoints explicitly.

## Producer, checker, metadata and consumer

1. `Python/bytecodes.c`: use the new marker at the five listed result sites.
   Other `IR_RETIRE_PUBLISHED` and pair sites keep their existing semantics.
2. `Tools/cases_generator/ir_generator.py`: parse the two identifiers in the
   marker; require the first to be the handler's scalar `STORE_DST` output and
   the second to be the pending owner from `OWNER(owner)`. Build one checked
   declaration record from that syntax and pass it to both emitter and
   `handler_contracts`. A narrow `result_publication_contract(inst, fmt)`
   checker can return `(kind, result_name, owner_name, producer_uop)` or no
   fact; `kind` is explicit marker or trusted generic completion. The emitter
   and metadata builder must call the same checker, rather than reconstruct
   a second name list. Reuse the existing retirement expansion; derive its
   local-only `failure_result` and final-store choice from the declaration
   record. Do not retain a handler-name switch for either choice. Preserve
   the generic completion path. Add the new marker to known releases and
   pending-owner readers.
3. Validate the declaration at the producer and whole-handler boundary. It
   must be one **top-level** `SimpleStmt` in the result-producing uop's body,
   exactly `IR_RETIRE_PUBLISHED_RESULT(attr, receiver);`, and its final
   meaningful statement. Reject a marker nested in `if`, a loop or another
   statement, a second marker, a missing `OWNER()` pending owner, or a
   conditional/joined result path. Verify that `attr` is the scalar output
   selected by the format's `ROLE_DST`, that the producer assigned it before
   the marker, and that no source mutation, address-taking or escape of it
   follows. For subsequent uops, map stack positions through the existing
   `external_io`/`SlotLayout` and `ROLE_DST` facts. Permit a suffix only if
   its read/write positions are disjoint from the declared output position,
   its effects cannot run Python, release an owner, deopt, error or switch
   frames, and it cannot alter the first output. Thus the existing companion
   output at `dst+1` qualifies by position/effects; no opcode-name exemption
   is needed. Reject ambiguous joins rather than add a parallel state cache.
   Update `analyzer.py`'s hidden-error and synchronization recognition for
   the new marker. `IRStack.copy()` shallow-copies the current boolean via
   `Stack.copy()`; this is no proof that a conditional marker joins safely.
4. Extend `HandlerContract` and generated `_PyIRFormat` by one bounded
   `result_phase` field: `NONE` (no declared phase fact) or
   `DST_TEMP_CANONICAL_BEFORE_RETIRE` (the first result is in canonical form
   in a temporary destination before its receiver is released). This is a
   **success-path conditional** fact, regardless of whether publication is
   generated by the explicit marker's flush or by the trusted
   `value_attribute_complete` helper. The generic fact is derived only for
   the existing `IR_COMPLETE_ATTR_READER(pending, pending_self, receiver,
   method); attr = pending;` flow after checking its four arguments and
   single first-output assignment. It must not imply publication before
   the helper call; generic lookup can escape earlier. `NONE` does not
   assert that other handlers never publish. The method helper's second
   output is outside this first-output fact.
   `ir_metadata_generator.py` emits the field in the existing format table;
   the coordinator updates `pycore_ir.h`, generated header,
   `Modules/_opcode.c`'s `IRContract` row and tests together. Metadata
   consumers may use the phase only on successful completion, not as a root
   map, failure policy, or JIT recovery snapshot.

Do not widen `_PyIR_RUNS_CODE` into a phase proof. It is a conservative union
over a handler and cannot say whether a particular result has been published
before one release. Conversely, this positive result fact does not prove an
opaque helper safe to skip, and does not relax the all-homes-valid observer
contract. The optimizing SSA/recovery design remains separate.

## Cost opportunity after this first packet

The first packet should preserve generated C behavior and measure no claimed
speedup. A later, separately reviewed experiment can ask whether one *known
canonical* result needs the first `_PyValue_Publish` check. For
`LOAD_ATTR_SHAPE`, `Python/bytecodes.c:_LOAD_ATTR_SHAPE` reads a `PyValue` from
an inline-value home and `PyValue_XDUP` preserves its bits; the existing
`IR_STORE_TEMP_DST` then calls `_PyValue_Publish` on that result. In the GCC
release ELF inspected after the call-record cleanup (build ID
`c042ee42cf98f5531c606910a692f799bc3879dc`),
`_TAIL_CALL_LOAD_ATTR_SHAPE` still has the first-publication exact-int/float
type tests at `0x352572–0x352588` and a conditional
`_PyValue_Canonicalize` call at `0x3525a4`. A separate local-destination
canonicalization remains at `0x352758`. The final-v4 deltablue profile gives
the entire shape handler 10.73% physical-symbol self share; neither those
addresses nor that share measure the frequency or savings of the type tests.

To remove the first check, prove every producer of that particular home stores
canonical bits, including extension and alternate paths, then declare a
separate canonical-result fact at the source site and make the emitter choose
one direct temporary publication. Verify an inline int/float, boxed large
number, mortal object, subclass, deallocation/reentry, folded local and
error path; compare actual GCC and same-workload counters before admission.
If the transitive home proof fails or GCC already eliminates the check for the
selected shape, stop. The contract packet does not make this optimization by
implication.

## Validation and decision

Generator fixtures should show: the five declaration sites retain first
publication, local-only failed-result cleanup, refresh/error branches, a
local-only final store and the companion NULL output; no five-name switch
remains. Add negative fixtures for wrong output, no pending owner, marker
inside a conditional, second marker, escape or result reassignment after it.
Metadata fixtures check the phase field for the five specialized handlers,
generic attribute completion and an ordinary retire-only handler. Execute
the generated handler/metadata tests and focused attribute/finalizer/frame
tests; the coordinator runs release/debug compile, lint and integration gates.
Compare regenerated C around the five handlers against the frozen version.
Accept this first packet only if emitted behavior stays equivalent and the
new checker detects invalid producer changes. A canonical-result experiment
has its own diff, review and matched measurement.
