# Option 1: encode the recipient's replacement requirement

Proposed for independent review. Candidate source is
`BASE/worktrees/frame54-recipient`, independently based on golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. No runtime source changes or
execution yet. Follow Astra's R contract; root owns generation, builds, native
execution, measurements, and commits.

Retain the 16-bit `return_slot` field but make its meaning an explicit recipient
descriptor. Low 12 bits are the existing slot; bit 12 is REPLACE_REQUIRED;
bits 13–15 must be zero. Keep the discard value exactly the untagged IR_NO_DST
(4095). Add explicit internal encode/index/is-discard/needs-replacement
interfaces. A descriptor is never used directly as an array subscript.

Recording a named local/cell/free destination sets REPLACE_REQUIRED. Recording
a scratch destination clears it only under the existing empty-home contract.
Discard stays distinct. This is an ownership fact, not an object-type fact or
future-read-liveness fact; no PyValue tag, home validity rule, initialized
frontier, or overlap ownership changes.

At publication `_PyFrame_SetReturnValue` first completes any pending redirect,
then reads the winning descriptor. Discard closes the held owner once. Replace
installs the result before closing the actual old owner. Empty asserts the
decoded physical home is NULL in debug, then installs directly without loading
the old value or calling XClose. Keep canonical incoming-home checks. Any
callback-capable retirement sees the same committed PC/home/frame state as
selected51. Do not cache the old descriptor across redirect completion.

Prefer recording from an existing local-boundary authority, such as the
executing image prefix, rather than decoding the function's mutable code.
Initialization uses the known local boundary. `_SAVE_RETURN_SLOT` can share an
inline encoder using the frame's execution prefix. Native entry's known scratch
slot is explicitly empty. Generator yield records its incoming destination
class; exact SEND records its caller publication destination. Copy preserves a
valid descriptor alongside its owner state. Debugger redirect selects discard
and must erase a previous proof. Special interpreter/base/trampoline frames
receive explicit descriptors and must not query a nonexistent image prefix.

## Complete port

Handwritten files: interpreter-frame struct comments/header helpers, ceval
macros and native call producers, bytecode DSL recipient producers,
`Tools/cases_generator/ir_generator.py` exact-entry producers, `Python/frame.c`
redirect invariants, `Objects/genobject.c`, `Objects/frameobject.c`, and
`Modules/_remote_debugging/asyncio.c`. Internal test interfaces are added only
if meaningful behavior cannot exercise descriptor boundaries. Root regenerates
products; do not hand-edit generated handlers.

Exact field-name search includes `_SAVE_RETURN_SLOT`, YIELD, RETURN_GENERATOR,
native entry/call paths, initialization/copy/base sentinel, and generated SEND
entry. Readers include `_PyFrame_PublishReturnSlot`, `_PyFrame_SetReturnValue`,
`_PyFrame_CompleteRedirect`, pending-child debugger validation,
`_PyGen_YieldFromRef`, and remote asyncio stack inspection. Mask before
sentinel/range comparisons and before `return_slot - 2`. Remote code must reject
reserved bits or tagged discard instead of interpreting them as huge indices.
Its raw field remains uint16, so debug-offset size stays unchanged; numeric
semantics still require the port. Search GDB/remote/debug offsets and test C
helpers independently, not just generated source.

The actual maximum index 4094 and discard 4095 remain distinct. Internal encode
assertions reject out-of-range slots, reserved bits, and tagged discard. Public
code words do not contain this descriptor: constructors cannot assert an empty
proof merely by naming a physical scratch slot whose live occupancy differs.
The runtime producer's existing consume/freshness contract remains authority.

## Cost and validation gate

Removed on the empty route: old-home load and XClose/type classification. Added:
producer local/scratch classification, encoding, recipient kind test, and masks
at numeric consumers. No field shrinks and no NULL-on-consume store disappears.
Native comparison must show this tradeoff in ordinary return, exact CALL,
SEND/YIELD and generator/native entry, without hiding extra code-owner loads.
Count reached empty/replace/discard classes before assigning performance weight.

Root's release/debug semantic gate covers ordinary and delegated return,
send/throw/close, first send, local replacement with a last-owner finalizer,
finalizers mutating caller locals, discard, suspended yield-from inspection,
frame escape/copy, native generator providers, pending debugger redirects
including same-PC intent, and remote asyncio decoding. Add focused behavioral
tests for missing boundaries; retain existing completion/frame tests rather
than mirroring accessor arithmetic. Internal boundary tests cover index4094,
discard4095 and malformed descriptors when an appropriate existing test API is
available. Run unchanged workload screens and confirm promising breadth with
six balanced pairs. Reject a net loss; do not add a handler family to rescue it.

No implementation until the independent peer and root resolve this proposal.
