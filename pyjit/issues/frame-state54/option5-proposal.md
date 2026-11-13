# Option 5: one certified same-home integer overwrite

Proposed for independent review. Candidate source is
`BASE/worktrees/frame54-clears`, independently based on golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. No runtime edits or execution
yet. This follows Astra's S contract. Root owns generation/builds/tests/native
execution/performance/commits. Static inspection of the existing golden ELF is
the only native-artifact operation performed for this proposal.

## Native feasibility established, frequency unproved

Read-only `objdump -d --disassemble=_TAIL_CALL_BINARY_OP_ADD_INT` on
`BASE/round51-golden-release/python` shows the B scratch NULL store at
`0x355f76` (handler +0xa6), following the local comparison at +0xa2. C clears
separately at `0x355f89` (+0xb9); result A stores at `0x355fb5` (+0xe5).
There is no call between those stores on the successful inline result route.
Thus native DSE has not removed the proposed B store from the generic handler.
The guards already eliminate input decref work natively; do not claim those
nonexistent closes as another saving. Overflow boxing and its failure precede
consumption; its successful result rejoins before the same stores.

The remaining unknown is reached eligible A==B temporary frequency. Root must
count that route in unchanged workloads before throughput claims. A synthetic
test can establish semantics, not justify admission. No runtime was launched
to establish the disassembly facts above.

## Exact contract

Add one execution-only variant of BINARY_OP_ADD_INT using the established
ExecutionVariant machinery. Keep pristine words and the public canonical
family unchanged. Refine only when A==B, C!=B, B is a valid scratch home at or
above the executing code's local boundary, and A is not discard. Preserve the
existing inline-integer guards for both operands. Locals, overlapping input
aliases, discarded results, or unproved geometry use the selected handler.
Do not add SUBTRACT until ADD is correct and has meaningful reached sites.

Specialization selects the variant from the instruction geometry and existing
local-boundary authority. Generated debug entry checks assert its complete
geometry. A safe fallback to the ordinary specialized handler must run before
input ownership changes if the assumptions do not hold. Canonical decoding,
deoptimization, feedback polymorphic rows, instrumentation and reinstrumentation
must map the variant back to the ordinary family, as the existing shape variant
does. Arbitrary immutable constructor words must not gain a way to bypass the
producer/variant admission checks. Root regenerates handler IDs and metadata,
then checks 16-bit handler capacity and every canonical consumer.

All guards, overflow allocation and result-error checks occur with selected51
PRE homes still anchored. Once result production succeeds, B is a guarded
inline value, so it owns no counted object. Keep its canonical bits in C as
needed but omit the B-to-NULL store and overwrite the same physical home with
the canonical result. C follows its unchanged local-or-scratch consumption
contract. C's inline guard means its close cannot call a destructor. No
allocation, callback, GC poll, error/deopt edge, observation, dispatch or frame
switch may occur between the suppressed clear and the result store. At the
next possible observation B/A holds the result and consumed scratch C is NULL.
There is no bitmap and no stale-home interval across an observable boundary.

Do not implement this by generally sinking clears across arbitrary generated
operations. Extend ExecutionVariant with a narrowly certified overwrite fact
for this numeric family, checked by the generator against the actual input,
output and commit sequence. The canonical handler remains the fallback. Source
generator checks must reject escaping/fallible work inside the certified commit
or geometry that does not equate the exact physical homes. Keep actual semantic
PC/error/source authority unchanged.

## Work and source scope

Removed at admitted sites: one physical NULL store, B local classification,
and generic A local/discard selection made redundant by geometry. Added:
specialization-time geometry selection, one execution handler's text and
metadata, and any required entry fallback check. Do not claim a removed RC
operation unless candidate disassembly actually differs there. The output
result may be boxed; it remains owned and must not be leaked on any guard or
error route.

Expected handwritten files are `Tools/cases_generator/ir_generator.py`,
specializer selection/canonical-feedback handling in `Python/specialize.c`,
any shared IR variant admission needed by `Python/ir.c`, and meaningful tests
in `Lib/test/test_generated_cases.py`/IR or value suites. Add a DSL operation
only if the generator cannot express the bounded commit without duplicating
arithmetic/error semantics. Prefer the established variant framework and a
single arithmetic implementation. No general decoder, frame geometry or
recipient representation changes belong in this independent lane.

## Admission

Peer review must accept the generator proof and fallback before implementation.
Root checks generated success/overflow/error/deopt paths, canonical restore,
instrumentation/LINE/INSTRUCTION/PY events, and actual native disappearance of
the store. Focused behavior covers scratch alias result, local output fallback,
discard, C local versus scratch, forbidden B==C, inline-boundary overflow,
boxed result, guard miss and monitoring after specialization. Generator tests
must reject a callback, poll, fallible step or wrong alias inside the proposed
commit, rather than merely assert output strings. Existing value and IR
protocol suites remain authoritative.

After reached-route census, screen unchanged workloads and confirm promising
breadth with six pairs. Reject no-hit or negative breadth; do not broaden the
family to manufacture savings. This proposal is a falsifiable store-removal
experiment, not evidence that arena NULLing dominates execution.
