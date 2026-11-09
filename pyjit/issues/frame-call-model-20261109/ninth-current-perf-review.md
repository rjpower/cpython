# Current workload PMU review

The `488e85fa3a8` candidate and `0765e9be47f` golden are compared at the
same warmed FIFO interval in `ninth-workload-perf-stat-v2`. All 18 invocations
returned successfully, were valid at 100% event running, and have byte-equal
before/after identity snapshots. I recomputed each count as the raw counter
divided by that invocation's hot loop count, then took the arithmetic mean of
three rounds per role. The ratios and CPI ratio reproduce
`ninth-workload-perf-summary.json` to floating-point precision.

| Workload | Candidate/golden cycles | Instructions | CPI |
| --- | ---: | ---: | ---: |
| nbody | 1.056129 | 0.986232 | 1.070872 |
| generators | 1.077414 | 1.056034 | 1.020246 |
| coroutines | 1.084348 | 1.052249 | 1.030505 |

Nbody now executes about 1.38% fewer instructions while taking about 5.61%
more cycles. Its present loss accompanies higher measured CPI at this workload
and control; the older `c142` extra-instruction finding must not be applied to
this binary.
Generators and coroutines each execute roughly 5% more instructions, with
additional CPI increases of about 2% and 3%. Those are whole-workload facts,
not attribution to `SEND`, generator allocation, frame clear or Native32.
The six `ninth-workload-perf-record` captures are complete and have stable
before/after identities, the expected executable build-ID among their hits,
and zero lost records. Each has about 2,000 user-cycle samples. A flat
`perf report --no-children --call-graph none` places samples as follows:

| Workload | Candidate location | Golden location |
| --- | --- | --- |
| generators | `SEND_GEN` 24.23%, `YIELD_VALUE` 13.54%, `LOAD_ATTR_SHAPE_LOCAL_TEMP` 5.88%; `_PyEval_CompleteStep` 1.45% | `SEND_GEN` 21.57%, `YIELD_VALUE` 15.59%, `LOAD_ATTR_SHAPE` 7.30% |
| coroutines | `CALL_PY_EXACT_ARGS` 10.55%, `RETURN_GENERATOR` 7.61%, `_PyEval_CompleteStep` 5.24%, `_PyEval_CompleteGeneratorReturn` 4.03% | `CALL_PY_EXACT_ARGS` 6.79%, `RETURN_GENERATOR` 5.49%, `END_SEND` 3.27% |
| nbody | `BINARY_OP_MULTIPLY_FLOAT` 15.50%, `LOAD_FAST_BORROW` 9.56%, `BINARY_OP_SUBSCR_LIST_INT` 9.68% | `BINARY_OP_MULTIPLY_FLOAT` 15.70%, `LOAD_FAST_BORROW` 11.56%, `BINARY_OP_SUBSCR_LIST_INT` 8.67% |

The generator and coroutine recordings point to related execution families,
but their hot distributions differ. The coroutine kernel recursively creates
and awaits coroutines, driven by an outer `.send(None)`; its recording warrants
examining the exact known-call, generator-return and completion routes. The
tree generator kernel traverses `yield from`, so `SEND_GEN` and `YIELD_VALUE`
are its first source locations to inspect. The candidate's
`_PyEval_CompleteStep` at `Python/ceval.c` publishes and detaches a semantic
window before retiring owners; the golden's corresponding work is distributed
across different handlers, so symbol percentages cannot be subtracted to
measure this helper's cost. Nbody's leading float arithmetic locations are
similar in both roles despite its higher current CPI. These one-record sample
shares locate code; they do not give per-handler speed ratios or establish a
single cause of the elapsed losses.

For nbody, a subsequent bounded branch question can use the committed
collector with an optional `--stat-group branches` switch, selecting exactly
`('branches', ('branches:u', 'branch-misses:u'))` from the sealed protocol's
`EVENT_GROUPS`. Keep `cycles-instructions` as the default; reject the switch
in record mode. `--cases nbody` would use the same repeat calibration, three
alternating rounds, FIFO interval, 99% running threshold and identity gates.
Branch counts and misses could distinguish one source of CPI pressure; they
would not by themselves prove the cause. This is a design-only option pending
the collector commit and root admission. No driver or runtime change is
proposed during the capture freeze.
