# Folded unpack normal transport

The integrated helper contract explains the two remaining freezer failures.
`FrozenImporter._fix_up_module` word 34 unpacks two values at scratch 10 into
locals 6..7; its later CALL needs scratch 11 absent. `zipimport._get_data`
word 2 unpacks eight values at scratch 20 into locals 2..9; the later child
CALL needs scratch 23 absent. Generic output geometry still describes those
temporary unpack outputs even after the folded-local transport consumes them.

`IREmitter.unpack_to_locals` runs after actual successful final output flush.
For a local destination, `_PyIR_UnpackToLocals` moves each element from
`window[n-1-i]` to `locals[i]`, NULLs its source, then closes the old binding.
Before a potentially reentrant old-owner close it hands off to
`finish_unpack_to_locals`, which completes the same sequence and refreshes
after each close. Failure drains remaining elements and takes the generated
error exit; it does not reach the ordinary normal successor. `IR_NO_DST`
selects ordinary scratch outputs and never calls the local transport.

Propose one family-closed `UNPACK_LOCAL_DRAIN` event emitted from that actual
local-transport adapter and successful final-flush capability. It replaces the
ordinary sole NORMAL_CLEAR recipe for that instruction; it is not granted
from the opcode's name. All specializations must emit the same adapter/event
before the canonical family gets the capability.

The cold normal transfer first applies existing unknown/common-input rules.
It proves consumed read inputs empty as NORMAL_CLEAR does. Additionally, only
when the validated destination is a named-local run, it establishes NO_HOME
for the unpack's entire scratch output window. It does not grant physical
NULL, initialized/readable values, exact phase coverage or named-local binding
contents. With IR_NO_DST the output window remains live/unknown. Zero outputs
still consume the original sequence through the common input rule.

Admission already bounds BASE plus max(reads,writes), validates extent
arithmetic, and requires a non-sentinel unpack destination to be a contiguous
local run large enough for every output. The capability must also require the
actual no-through, window-output helper shape. Broader helper/frame mutations
remain outside the normal-vacate source contract; changed/synthetic members
without that successful adapter lose the family grant. No argument-count
guess, runtime fill or admission bypass is needed.

GC lane owns the disjoint generator adapter/event collection and family gate,
the event enum and its semantic_step consumer, plus a separate unpack-transfer
test file. IR owns the concurrent observer/FOR repair. Root owns composition,
generation, freezer probes and execution. Tests should admit folded-local
tail reuse, reject the same tail reuse with IR_NO_DST, cover 0/2/many outputs,
exercise generic/tuple/list specialization and displaced-local callbacks, and
verify a missing sibling adapter removes the whole-family event.
