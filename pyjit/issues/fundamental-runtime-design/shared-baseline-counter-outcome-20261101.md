# Current shared counter evidence

2026-11-01. Bound debug/stats observation, not elapsed performance. See the
[independent outcome review](shared-baseline-outcome-review-20261101-v1.md)
for source/image/raw/cleanup identities and scope. Values below are
`(high-low)/(2 × registered loops × inner iterations)` from the retained full
dictionaries, not one individual process's totals.

| Registered consumer | Handler entries/loop | Inline boxes/loop | Object allocations/loop |
| --- | ---: | ---: | ---: |
| coroutines | 4,005,960.125 | 2 | 242,790.5 |
| generators | 6,508,572.5 | 0 | 100,000 |
| Richards | 5,790,115.5 | 9 | 2,387 |
| json_loads | 4,158.0075 | 0 | 7,240 |
| unpickle | 312 | 0 | 4,720 |
| regex_compile | 18,353,641.5 | 992,173 | 1,194,644 |

Coroutines has 242,785 `COMPARE_OP_INT` entries and 242,785
`COMPARE_OP_INT→POP_JUMP_IF_FALSE_BOOL` pairs per loop. Its
`BINARY_OP_ADD_INT→RETURN_VALUE` pair has 121,392. These establish actual
family activity for the provisional comparison/branch region. They do not
identify a particular code object/PC, prove compiler admission for that site,
or count successful future region entries. Current pristine/executing IR,
source/exception context and closed-call-graph attribution are still required.

Generators is chiefly a different execution shape: 1,568,946 YIELD_VALUE
entries, 1,568,945 SEND_GEN, 1,468,946 JUMP_BACKWARD_NO_INTERRUPT and 399,999
LOAD_ATTR_SHAPE per loop. Its largest pair is
JUMP_BACKWARD_NO_INTERRUPT→SEND_GEN at 1,468,946; the equally large
YIELD_VALUE→YIELD_VALUE crosses nested generator/frame execution. There is
no positive comparison pair in this observation. Do not treat all counted
pairs as physically adjacent same-frame instructions eligible for fusion.

Richards shows 1,000,644 LOAD_ATTR_SHAPE_POLY entries, 482,125
CALL_PY_EXACT_ARGS, 481,351 RETURN_VALUE and 442,457 false-bool branches.
COMPARE_OP_INT→false-bool contributes 88,331 pairs per loop, but several
operand-loading families precede comparison; aggregate evidence does not
select one site. Its leading shape-poly→call pair is 327,245, and
call→shape-poly 284,697. No fraction of these counts is automatically removable
dispatch or elapsed time.

The first three workloads exhibit millions of interpreter entries alongside
at most nine inline-box conversions per loop. This rejects a blanket
boxing-dominance attribution from their names. It does not show that native
code or collector cost is negligible. The native-heavy json/unpickle rows
similarly have no counted inline boxes, while regex has many. These six
consumers are a small shared observation set, not a native-interface roadmap;
regex implementation remains paused.

Counter caveats are material. Entries/pairs include adaptive same-word
redispatch and across-frame transitions; they are not dispatch-only costs.
The legacy by_instruction boxing map remains empty in all six reports and
does not gain coverage merely because full raw records now survive. All 18
native box-site tables are full, and their final bucket also aggregates
overflow callers. Resolved image+offset coordinates are ASLR-stable for the
bound image, but cannot separate that bucket. Py_STATS GC fields stay zero
despite positive actual collection witnesses, so this set does not rank GC
death/reclamation families. Signed union differences, full raw bytes and
actual per-process maps remain available in the audit receipt.

Next useful interpreter observation is the separately scoped current region
site attribution, using the already bound image and unchanged registered
workload. GC storage/death selection needs its independently reviewed state
evidence. Neither follow-up is activated by this outcome.
