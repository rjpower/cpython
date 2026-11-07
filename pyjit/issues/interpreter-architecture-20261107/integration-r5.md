# R5: canonical shape-result publication

Integrated at runtime `0953757f4e7eb442896dcf42f6547009528defaa`, on R4
parent `b58036e9718b87b95ad83b72e10d4578942a61d6`. The bounded optimization
uses `attr = IR_CANONICAL_HOME_DUP(found)` to record a canonical-source fact
on the existing checked publication contract. Only the named result's first
temporary flush uses the direct canonical store. The checked declaration,
assignment/output/suffix rules, result-retirement marker and existing
publication phase remain shared. The generated interpreter delta is two
lines in `LOAD_ATTR_SHAPE`; formats, metadata and targets are unchanged.

The premise is explicit at `_PyObject_StoreInstanceValue`: its borrowed input
must be a canonical home value or NULL, and an entry assertion checks
`_PyValue_IsHome` before duplication, callbacks or stores. The in-tree writer
chain was manually audited: object adapters canonicalize, specialized shape
stores transfer frame-home values, and detach/materialize paths invalidate
the shape. This is a source proof backed by a fatal debug assertion, not a
static C pointer-provenance proof. Rebuilt external callers of this internal
API must meet its precondition.

Folded-local publication still uses `_PyValue_Publish` after receiver
retirement, reading the current home so callback mutation is preserved.
Failure cleanup, refresh, receiver retirement and companion output keep their
existing paths. R5 adds no `FrameState`, error root map, post-effect recovery
protocol or native method/tracing backend.

The initial shape-result audit stopped because the internal writer boundary
did not state or enforce the canonical-home precondition. Its proposal reviews
and source audit remain preserved. After the boundary amendment and RHS
refinement, implementation review and [cold review](cold-r5-review.md)
accepted the bounded source with no must-fix finding. The generator checker
still relies on the manual writer premise; it does not infer where `found`
came from.

The isolated final generator passed 75 tests; the two corrected semantic
tests passed on unchanged R4 before integration. On R5, release and debug
builds each checked 117 modules and their six-file focused runs each passed
630 tests with one skip. API lint covered 306 units with zero failing findings
and zero parse errors. Native identities bind both focused builds to R5; ELF
IDs are `9d98a70694382c4650742915f0560f12346c85bf` (release) and
`842535c720c4e7b0a8d52044e4865d3474294756` (debug). The coherent stats build,
identity link and smoke check passed. The full release and debug suites each
passed 54,380 tests with no failures; release skipped 2,467 and debug skipped
2,405. Across both profiles, 537 test files passed, 79 were skipped and 11
resource-denied out of 627. Full correctness qualification is complete.

The basic seven-kernel screen has candidate R5/R4 factor `1.022097` and
upstream-control drift `0.998896`. It is mixed/adverse context, not proof of a
2.21% R5 regression. The reviewed paired comparison has six rounds across
five value categories and two destinations. Temporary object, bigint and NaN
reads improve repeatedly; folded-local controls regress about 0.85–2.60% by
category geomean. Temporary integer results are anomalous/noisy, and the float
effect is small. The forms differ in store and retirement work, so controls
are not a subtraction estimator. The independent
[paired results review](paired-shape-results-review.md) records the initial
recommendation pending whole-workload evidence. The completed four-round
balanced 29-kernel comparison has paired geometric factor
0.992103, all four round aggregates below one and 19/29 kernel gains.
The [whole-workload review](whole-workload-r4-r5-results-review.md) accepts
keeping R5 with the local penalties and uncertainty documented. This closes
the bounded keep/revert decision. The completed broader campaign reports
candidate/upstream and candidate/interim factors of 1.247431/1.044650 on 29
kernels and 1.048074/0.878774 on the selected 12-result subset. All 54
perf-stat invocations are valid, and six DWARF captures have zero lost samples.
No universal speedup or upstream parity is established.

Evidence is under
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/`:
`r5-checks.json`, `r5-identities.json`, `r5-basic/`, `r5-paired-shape/`,
`final-checks.json`, `final-build-identities.json`, and the final build, smoke
and suite logs. Collection and the bounded keep decision are complete; the
independent final evidence/report review is complete. See the
[final report](../../bench/results/interpreter-architecture-20261107.md).
