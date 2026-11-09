# Cold operand and output contract census

This census follows native13's TaskGroup._aexit rejection: SEND cursor B+1
was a valid NULL input after unwind. It describes current source contracts,
not measured performance or final qualification. Canonical opcode recovery
normalizes specializations and explicit instrumented twins; generic transparent
wrappers are execution metadata and do not invent an operand layout.

| Canonical reader family | Nullable semantic inputs | Required inputs / limitations |
| --- | --- | --- |
| CALL, CALL_KW | self at window+1 | callable, arguments, keyword names remain required |
| CALL_FUNCTION_EX | self+1, absent kwargs+3 | callable+0 and argument tuple+2 required |
| FOR_ITER, SEND | cursor+1 | receiver required; SEND incoming value required |
| CLEANUP_THROW | cursor+1, optional transferred-send owner+2 | receiver and exception+3 required |
| WITH_EXCEPT_START | exit self+1 | exit callable, lasti, exception required; previous-exception passthrough is not a fresh definition |
| POP_ITER, END_SEND | semantic cursor input1 | iterator/receiver and result required; these remaining release words still need structural removal |
| PUSH_EXC_INFO, END_ASYNC_FOR | none newly admitted | real exception input required; prior exception is canonical Python None when absent |
| COPY | no broad exemption | bottom DUP requires non-NULL; unused middle homes are passed unchanged, not defined |
| SWAP | raw NULL transport exists, not admitted as a fresh owner | preserve/permutate facts before supporting unavailable operands; current proof conservatively rejects them |
| LIST_APPEND, SET_ADD, MAP_ADD, DICT_MERGE/UPDATE, LIST_EXTEND, SET_UPDATE | no broad exemption | collection and inserted values are real reads; unused/passed prefix is not a fresh output |
| BUILD_LIST/TUPLE/SET/MAP/SLICE/STRING/INTERPOLATION, IMPORT_NAME, MATCH_CLASS, UNPACK_SEQUENCE/EX, LOAD_SPECIAL/SUPER_ATTR | none newly admitted | required values; variable output homes follow emitted mapping, not frontend stack counts alone |
| RAISE_VARARGS, RERAISE, YIELD_VALUE, RETURN_GENERATOR | none newly admitted | RERAISE reads lasti/exception, not implicit unwind owner prefix; scope/suspension outcomes are explicit |

Fixed nullable positions have direct source evidence. SEND and FOR_ITER generic
and specialized paths explicitly handle NULL versus virtual index. For THROW,
IR_ENTER_GENERATOR detaches SEND B+2 before VM child activation: a returned child
exception therefore may reach CLEANUP_THROW with no remaining sent owner; native
send error can retain one. This does not make the exception or receiver optional.
WITH_EXCEPT_START passes exit_self to with_exit_call with an explicit NULL test.
CALL_EX codegen emits PUSH_NULL for absent keywords, and both native and Python
providers accept NULL kwargs. No capability is inferred from variable spelling
or from a final XCLOSE alone.

A nullable role is not a NULL fact or a definition. Completion proof seeds
unavailable-original and may-nonNULL bits, applies actual unwind clearing and
exception/lasti definitions, then accepts a nullable read only with N clear.
Preserved PRE receiver prefixes remain unavailable/may-nonNULL; POST disposal
or publishing a different recipient does not reconstruct them.

## Physical fresh-output mapping

fmt.through describes the unchanged prefix, not semantic read suppression.
It never defines fresh ownership. A generated per-handler output_kind follows
the same actual store_mode as handler emission:

- REGISTER_DST: fresh output run starts at encoded DST, length writes-through.
- WINDOW: fresh run stays in the implicit window after through.
- WINDOW_DST_FIRST: the actual result at window+fmt.dst moves to DST; remaining
  new tail stays in the window. LOAD_SUPER_ATTR's variable NULL tail proves this
  must differ from moving every output to DST.
- OUTCOME: use the already reviewed edge-specific semantic outcome.

UNPACKS keeps its existing conditional destination contract: NO_DST means the
canonical window; an explicit destination is the receiving local run. SWAP is a
permutation and creates no fresh owners. COPY's unchanged prefix remains owned;
its new duplicate requires a valid source. Unknown STORE_LEAVE forms fail closed.
A generates this fact from the emitter. Shared cold _PyIR_DefinesHome consumes
it for constructor proof and the upcoming debugger recipe; semantic outcomes
remain a separate per-edge override. _PyIR_NullableInputs uses window-relative
bits for WINDOW readers and semantic IN0+k bits for register readers.

## Deliberate conservative limits

STORE_GLOBAL input0 explicitly implements NULL deletion and is included in the
shared contract. Generic STORE_ATTR also accepts NULL deletion, but its shape
specializations do not share that contract. The shared helper therefore grants
input0 only to the actual STORE_ATTR handler. Observation wraps that handler;
there is no separate instrumented attribute-store opcode.
Static deletion sites cannot specialize: _SPECIALIZE_STORE_ATTR gates on a
non-NULL value. Ordinary assignment values cannot be raw NULL. No ordinary
Python route to NULL at an already specialized site was found; no hot guard
was added. A debugger recipe admitting NULL must inspect the actual target
handler or deopt it before committing. This is not a general type verifier for
handconstructed bytecode that mixes deletion and assignment at one site.
Likewise unused array entries are not removed from reads merely because they
belong to fmt.through: LIST_APPEND's collection is passed through and actively used. Conservative rejection of
unknown NULL transports remains preferable to manufacturing an owner. These
limits are implementation work, not Python language lifetime requirements.


## Native15 sparse-read correction

The compile-only corpus admitted 2147 files and found four real constructor
rejections: an attribute deletion, DICT_MERGE's untouched prefix, and two
LIST_APPEND middle windows. These are operand-contract errors in the cold
proof, not evidence that NULL is a Python value.

Shared cold `_PyIR_ReadsHome` retains every explicit IN field, then audits the
physical window separately. APPEND/SET_ADD/MAP_ADD read the collection at B;
their trailing values are explicit IN fields. EXTEND/UPDATE read B and B+arg.
DICT_MERGE reads B, B+3 and B+arg+3. COPY reads its bottom; SWAP reads its two
endpoints. WITH_EXCEPT_START reads all five positions except the untouched
previous-exception position B+3. Every other window retains the full encoded
read extent, including opaque call and activation adapters. No use is inferred
from a variable name or the first uop's used flag. Read suppression never
creates a definition or a NULL fact.

Constructor tests place unavailable original owners in each audited middle
window, then move required collection/source inputs onto those same homes and
require rejection. Separate tests distinguish actual generic deletion handlers
from a STORE_ATTR specialization. Native validation is owned by the coordinator.
