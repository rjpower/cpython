# Recipient default: distinguish no recipient from discard

Proposal only; no runtime source changes. Research identity is measured R,
`frame54-recipient@612fb4e9d170532947c007f0ccaaa04aff623329`.
Keep that tree and its measurements immutable. Root will create an isolated
candidate after independent review, and owns generation, execution and timing.

Initialize an ordinary activation's `return_slot` to a reserved 16-bit
`_Py_RETURN_SLOT_NONE = UINT16_MAX`.
There is no pending child result at activation creation. Recording an actual
recipient remains the responsibility of the producer immediately before child
publication or suspension. Keep `return_pc = instr_ptr`: it is also the existing
initial landing, not evidence that a child has been published.

An actual discarded result remains exactly untagged `IR_NO_DST`. No-recipient
records absence of a transfer contract; discard records a producer's explicit
instruction to retire a result. This supersedes the initial default-discard
proposal: the debugger audit below shows why those states must remain distinct.

This removes `_PyFrame_EncodeReturnSlot(0, code->co_nlocalsplus)` from
`_PyFrame_Initialize`. It removes the default recipient's local-boundary read,
classification and encoding; it adds one constant descriptor store in their
place. Existing real producer classification and publication branches remain.
No header size, instruction format, handler family, ownership representation or
ordinary teardown changes. No speedup is predicted.

## Producer and lifetime audit

References below are to handwritten measured R source, not canonical runtime
copies. A field-name search covered `return_slot`, `return_pc`, setter helpers,
generated-case producer source, remote readers and debugger recipes.

| Path | Authority before recipient consumption |
| --- | --- |
| `_PyFrame_Initialize`, `pycore_interpframe.h:355` | Initializes a fresh THREAD frame; it has no child. PushUnchecked, overlapping push, binding and `PyFrame_New` use this initializer. |
| Generic Python CALL, `bytecodes.c:_SAVE_RETURN_SLOT` | Explicit setter plus continuation before switching to child. Exact CALL constructs the child first, then uses this call protocol. |
| Subscript/init call, `bytecodes.c:_BINARY_OP_SUBSCR_INIT_CALL`, `_CREATE_INIT_FRAME` | Explicit destination setter and continuation before child publication. |
| Native-to-Python transactions, `ceval.c:3344,3447,3694,3761` | Explicit caller destination setter and continuation at successful commit; binder/error cleanup publishes no result into the fresh child's default. |
| Generator creation, `bytecodes.c:RETURN_GENERATOR` | Copies frame, explicitly sets generator input recipient before marking CREATED/GENERATOR. `codeobject.c:1217` requires the normal RETURN_GENERATOR destination to be discard. |
| Suspension, `bytecodes.c:_YIELD_VALUE` | Records the suspended generator's next-input destination before switching to its caller. Resume consumes that recorded descriptor. |
| Exact SEND, `ir_generator.py` generator-entry emitter | Records caller completion recipient before switching; publishes input into child's already established creation/suspension descriptor. CREATED non-None input is rejected before this publication. |
| Native generator next/send, `genobject.c:gen_send_value_ex` | Publishes input before evaluator entry; normal generators already have RETURN_GENERATOR/YIELD authority. Cold C API construction needs the correction below. |
| Ordinary RETURN and completion | Publish into caller's recorded descriptor; do not consume dying child's initializer default. Native evaluator caller has an explicit real exit recipient. |
| Copy/takeover, `pycore_interpframe.h:_PyFrame_Copy` | Preserve installed descriptor or NONE together with return PC and pending redirect. A fresh no-child state can be copied; it confers no pending-call authority. |
| Clear, unwind and failed binding | Retire owned homes; no result is published into the fresh activation's default. Descriptor does not govern traversal or home validity. |

Preserve `_PyFrame_SetReturnValue` ordering: complete pending redirect first,
reread the winning descriptor, then discard/replace/empty publication.
`_PyFrame_CompleteRedirect` continues to require discard. This proposal neither
caches a descriptor across callbacks nor changes replacement-before-close.

Special frames are deliberately different:

* The evaluator entry in `ceval.c:2285` keeps its explicit empty recipient 1;
  returns and yields really publish into entry stack[0].
* `_PyFrame_PushTrampolineUnchecked` keeps its explicit supplied recipient
  (currently shim result slot 1). It must not inherit NONE instead.
* The thread base sentinel has no executable recipient. Change its explicit
  encoded zero to constant NONE for the same no-pending-child meaning; do
  not query an execution prefix there. Its return PC remains NULL.

Ready descriptors retain their existing numeric semantics. NONE has reserved
bits and must never be decoded as an index or classified for replacement.
`_PyGen_YieldFromRef` only reads a suspended delegating input recipient;
CREATED state never authorizes that lookup. Remote asyncio must reject NONE
before decoding, as it already rejects reserved bits. No remote offset sizes
change.

### Descriptor interface contract

Keep `_PyFrame_ReturnSlotIsValid` as the existing predicate for a ready
descriptor (empty, replace or discard), not a predicate accepting every frame
state. Therefore it continues to reject UINT16_MAX, tagged discard and all
other reserved-bit values. The index and replacement helpers retain their
ready-descriptor debug assertions. `_PyFrame_ReturnSlotIsDiscard` must accept
ready descriptors or exactly NONE in its debug precondition and return false
for NONE; its release implementation remains one equality test. Do not make
NONE pass the index assertion by globally widening IsValid.

Add an explicit ready-descriptor debug assertion at result publication after
pending redirect completion. Its release build must evaluate no extra
predicate and add no hot branch. Every actual publication path in the table
has a producer-installed ready descriptor. Entry/trampoline use their explicit
ready descriptors; C API-created generators get discard below. If this proof
fails on an actual ingress, fix that producer rather than treating NONE as
discard or adding a generic release fallback. `_PyFrame_CompleteRedirect`
installs/requires actual discard and does not decode NONE.

Cold readers are explicit: pending-child admission's discard test returns false
on NONE; remote awaited-frame inspection rejects NONE through unchanged ready
validation; yield-from lookup and direct publication assert a ready descriptor.
Copy, traversal, clear, debug-offset metadata and field-address reporting do not
interpret descriptor bits. Update the field comment to distinguish activation
state from ready recipient. No on-wire instruction operand uses this sentinel.

## Cold C API construction is a correction, not a slot-zero exception

`PyFrame_New -> init_frame -> _PyFrame_Initialize` supplies a FRAME_OBJECT-owned
frame to `PyGen_New`, `PyGen_NewWithQualName`, `PyCoro_New` or `PyAsyncGen_New`.
Those constructors bypass RETURN_GENERATOR. The old helper copies a NULL
`frame_obj`, asserts it equals the passed frame, and later sends the first None
into an invented slot zero. Root has reproduced the association assertion on
golden51 (`pygen-new-probe-round51-golden-debug.log`); L's association correction
then exposed the erroneous slot-zero input. Neither is behavior to preserve.

Port only the bounded cold helper correction from L
`frame54-locals@b48fe2d3989`, adapted to R's existing two-argument Copy and direct
`f_locals`. Do not port sidecars, header changes or L's Copy API:

1. Allocate subtype and compute coroutine origin, if enabled, before moving
   frame ownership. On origin failure free untracked allocation and consume
   the stolen input frame reference.
2. Under the frame critical section, set source recipient to discard, then
   perform the existing two-argument move. Explicitly establish CREATED and
   GENERATOR owner in R; acquire `frame->frame_obj = Py_NewRef(f)`.
3. Initialize all subtype-shared fields before publication: weakrefs,
   exception linkage, origin/finalizer, closed/hooks/running flags, names and
   qualname. Link `f->f_frame` to the new frame and GC-track only after coherent
   initialization; release critical section then the stolen frame reference.

The explicit cold discard establishes the public constructor's first-input
contract; an ordinary fresh frame's NONE is not that contract.
The added frame-object owner balances the eventual generator/frame teardown;
do not drop the input reference before acquiring that association.

Add a small `_testcapi` construction helper and behavioral tests derived from
L's regression. Check retained frame identity, first `next`/`send(None)`, yield,
completion/close and GC with an escaped frame. Cover coroutine origin and
async-generator close/hooks initialization through corresponding C API
constructors. Do not copy L's sidecar-specific mapping-retention expectations.

## Pending debugger call recipes retain their boundary

`frameobject.c:3513` treats discard as one condition for permitting an ancestor
redirect while a child executes. A discard default would make that condition
true before any producer had installed a discarded recipient.
In particular a native C callback reenters through an INTERPRETER shim:
`ceval.c:_PyEval_EvalFrameLoop` links child -> entry -> observed ancestor.
That entry is not the pending Python child for which this recipe was proved.
The existing `external` bit is not a substitute: no selected-source setter was
found. NONE makes the existing discard test false, preserving the recipe's
producer requirement without an interpreter-child workaround.

Keep this gate unchanged except for its helper's permitted NONE input.
No claim is made that a native redirect reproducer with a discard default has
passed later analysis. Include a first-native-call
callback regression before any real producer has installed a recipient; it
attempts the ancestor jump and verifies safe rejection
and normal result delivery. Existing supported discarded Python-child jumps
must continue to work. Any other newly admitted recipe found during testing
must be resolved rather than accepted as a side effect of initialization.

## Bounded implementation and qualification

Expected handwritten files: `pycore_interpframe.h` state/helper contracts;
interpreter-frame struct comment; `pystate.c` base sentinel; `genobject.c` cold helper;
`Modules/_testcapi/frame.c` and focused frame/C API tests. No handler/emitter
changes and no new recipient helper abstraction are needed.

Root's release/debug gate should exercise ordinary local/scratch/discard
returns, fresh escaped-frame NONE copy/clear, first native/exact call,
constructor failure, init trampolines,
generator first send/rejection, send/throw/close, nested yield-from and C native
providers, C API creation above, copy/escaped frame/GC, finalizer reentry,
pending and same-PC redirects, and remote yield-from decoding. Preserve existing
recipient tests; add only the missing cold ingress/default cases.

Read-only native comparison must confirm constant ordinary child header
initialization, absence of default locals-boundary classification, unchanged
real producer classification, and retained native entry/trampoline recipient
stores. Root then measures unchanged workloads against both golden51 and the
original R identity. Correctness and removed source work alone do not establish
performance admission.

Independent fields peer review approved this contract in
`recipient-default-peer.md`; root implementation authorization is still
required. Current
evidence is source audit plus root's cited preexisting C API failure; this
worker has run no native commands, builds, tests, regeneration or timing.

## Handwritten handoff

Implemented only in isolated `frame54-recipient-default`, based on the R
identity above; original R remains unchanged. Seven handwritten files implement
NONE state, preserve ready-descriptor readers, correct cold constructor frame
association, and finish subtype initialization before publication. PyCoro_New
and PyAsyncGen_New now return the common constructor directly, avoiding duplicate
postpublication origin allocation and subtype mutation.

Four new C API/fresh-frame test methods cover fresh-frame clear, generator
first-input rejection/discard and mapping lifetime with an escaped frame,
coroutine first input and depth-0/2 origins, async-generator first input/hooks,
asend and close. One debugger test covers the first native callback before a
real producer establishes a recipient. The two edited Python files parse with
host Python and the diff passes whitespace checks; native qualification remains
root-owned. Independent actual-diff source GO is recorded in
`recipient-default-post-source.md`.
