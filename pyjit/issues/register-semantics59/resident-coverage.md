# Resident-region coverage on selected57

This is an opportunity census, not an admission or performance result. Inputs
are the eight `E59/resident-selected57-{case}.json` files, all complete with
stable before/after identities for selected57 `0dcba75bb3df`. Adapter SHA256:
`37f743dd97d863fd881fc21d7191641e8bc59755dbab92150beef3261ad5cbc7`.
Only host JSON/source analysis was used for this note.

The weights are monitored **semantic-word visits assigned to the pre-monitor
warmed adaptive labels**. Monitoring resets many specialized entries: for
example, `FOR_ITER_LIST` becomes `FOR_ITER`, and SHAPE becomes `LOAD_ATTR` in
the after snapshot. These are not measured specialized hits or guard successes.
Immutable words, positions and recovery bytes remain unchanged. The adapter
includes kernel functions and, for logging, logging functions; it does not
count every library or native helper. Zero-count words remain in the inputs.

## Traffic and boundaries

Counts below are visits, not cycles or native instructions. A high count picks
a source path for review; it does not establish its cost.

| Case | Visits | Top warmed labels, weighted visits | Boundary words¹ |
| --- | ---: | --- | ---: |
| comprehensions | 13,674,014 | FOR_ITER_LIST 1,686,000; JUMP_BACKWARD 1,518,000; SHAPE_LOCAL_TEMP 1,260,000 | 12.90% |
| deltablue | 14,679,757 | SHAPE_POLY_LOCAL_TEMP 1,709,700; RETURN_VALUE_EMPTY 1,542,631; METHOD_WITH_VALUES 1,528,080 | 22.40% |
| coroutines | 9,711,441 | LOAD_SMALL_INT_INLINE 1,456,712; RETURN_VALUE_EMPTY 971,142; LOAD_COMMON_CONSTANT 971,142 | 40.00% |
| generators | 11,813,690 | SEND_GEN 3,137,890; YIELD_DELEGATED 2,937,892; JUMP_BACKWARD_NO_INTERRUPT 2,937,892 | 56.51% |
| logging_silent | 8,520,033 | RETURN_VALUE_EMPTY 1,200,003; POP_JUMP_IF_FALSE_BOOL 1,200,000; METHOD_WITH_VALUES 1,200,000 | 28.17% |
| nqueens | 10,307,053 | JUMP_BACKWARD 1,473,969; FOR_ITER_GEN 785,098; YIELD_VALUE 702,436 | 17.62% |
| raytrace | 28,032,312 | SHAPE_LOCAL_TEMP 6,507,307; RETURN_VALUE_EMPTY 2,867,044; METHOD_WITH_VALUES 1,960,768 | 20.27% |
| spectral_norm | 15,638,484 | ADD_INT 3,380,000; LOAD_FAST_BORROW 2,054,343; LOAD_SMALL_INT_INLINE 2,033,203 | 8.75% |

¹ Disjoint word category: CALL*, SEND*, YIELD*, FOR_ITER_GEN,
GET_AWAITABLE and RETURN_VALUE*. A boundary word is not wholly removable work.
Attribute labels in the table abbreviate their `LOAD_ATTR_` prefix.

Comprehensions has 648,000 `CALL_METHOD_DESCRIPTOR_FAST` visits, or **108
dict.get provider calls per tray**, and 972,000 `LIST_APPEND` visits, or **162
direct consuming append-helper operations per tray**. The latter are not
generic Vectorcall calls. Total CALL* traffic is 1,140,002, including Python
and other providers. Neither the append count nor all CALL* traffic licenses
a native-call saving estimate.

## Finite motifs, with overlap removed

The table is one left-to-right packing, preferring the three-word motif at a
shared start. A candidate must have an actual result-to-consumer operand
dependency, scratch intermediate destinations, the same starting source line
and EH interval, and no branch/EH/recovery entry into its interior. SHAPE
receivers must be local anchors. Weight is the minimum visit count of its
members. Once accepted structurally, its words cannot contribute to another
motif. This excludes nominal opcode occurrences without an eligible adjacency.

The finite shapes are two mono SHAPE reads → INT compare; mono SHAPE read →
typed scalar use; INT arithmetic → INT arithmetic/compare; FLOAT arithmetic →
FLOAT arithmetic/compare; and exact list/tuple integer-index read → typed
arithmetic/compare. FLOAT pairs are a follow-on family, not part of the first
INT template. The packing is not an optimal packing or a ceiling for every
possible future region.

| Case | Structurally possible disjoint runs, by family | Removed-dispatch ceiling² | Share of visits |
| --- | --- | ---: | ---: |
| comprehensions | two-SHAPE/compare 108,000 | 216,000 | 1.58% |
| deltablue | two-SHAPE/compare 57,390; SHAPE/scalar 9,060; INT/scalar 5,940 | 129,780 | 0.88% |
| coroutines | none of these motifs | 0 | 0 |
| generators | none of these motifs | 0 | 0 |
| logging_silent | none of these motifs | 0 | 0 |
| nqueens | index/scalar 449,064 | 449,064 | 4.36% |
| raytrace | SHAPE/scalar 3,103,920; FLOAT/scalar 199,463; INT/scalar 10,000; index/scalar 47,997 | 3,361,380 | 11.99% |
| spectral_norm | INT/scalar 676,000; FLOAT/scalar 676,260 | 1,352,260 | 8.65% |

² For this packing, even a fully successful n-word region replaces at most
n−1 dispatches. Actual eligible/successful executions may be **zero**. Runtime
type/range/filled-field guards, complete use/ownership/effect proofs, observer
deoptimization and exit costs are not established by these counts. Dispatch
share is not an elapsed-time or instruction saving. No call-boundary saving
is added to these overlapping hypothetical budgets.

The first motif is concrete in `WidgetTray._add_widgets`, words 62–64:
`SHAPE_LOCAL_TEMP A12←L2`, `SHAPE_LOCAL_TEMP A13←L0`, then
`COMPARE_OP_INT A12/B12/C13`, all on line 57. Each has 108,000 visits over
6,000 trays: 18 possible runs per tray. Its nominal second-read/compare pair
also has 108,000 visits and is **not** counted again. In Delta the same shape
occurs in `Strength.weaker` (36,150) and `Strength.stronger` (21,240).
Even a hypothetical 20 native instructions removed per first-motif run would
save only 360 instructions per comprehension tray, about 0.43% of the retained
84,620-instruction upstream deficit. That is a scale check, not a measured
budget.

Useful follow-on anchors are `Vector.dot` pairs at words 4–5, 7–8 and 11–12
(509,871 visits each); `eval_A` ADD_INT → MULTIPLY_INT at 5–6 (676,000);
`part_A_times_u` and `part_At_times_u` FLOAT multiply/add at 15–16
(338,000 each); and the nqueens additive generator's tuple read/add at 8–9
(322,560). Nominal overlapping arithmetic pairs in Ray total 936,794 FLOAT
runs, but only 199,463 remain after the stated packing and boundary filters.
Those larger nominal numbers must not become additive savings claims.

## Consequences for the next contract

The first motif can test common resident lowering and exits, but cannot explain
parity by itself. Broader arithmetic/projection intervals have material static
coverage in Ray, spectral norm and nqueens; their actual owner and no-callback
guards need source admission before implementation. Dictionary predicates stay
excluded without a complete callback-free lookup contract.

These motifs do not cover the dominant coroutine/generator mechanism. Coroutines
has 971,141 exact no-self Python calls, 971,136 GET_AWAITABLE and SEND_GEN visits,
and 971,142 returns. The generator case additionally spends 24.87% of visits on the
backedge following delegated suspension. Crossing these sites requires a
call/resume/continuation contract and materialized owners, not permission to
extend a callback-free region through them. Logging likewise has 1.2 million
calls, 1.2 million returns, 1.2 million branches and 1.2 million SHAPE reads,
but no adjacent motif from this finite set. Its method/branch/call boundaries
must be reviewed separately. Cold or uncounted library work remains outside
all percentages above.
