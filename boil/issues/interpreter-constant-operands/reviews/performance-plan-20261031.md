# Independent held performance-plan review

2026-10-31. **ACCEPT the revised held plan for later activation preparation.**
Activation remains held until native feedback, separately reviewed raw perf
capture and the concrete measurement manifest are accepted. This host-only
review grants no measurements or native execution.

Original reviewed plan SHA-256:
`7956bc7b269d077f2a56e7354c431aa03e5d87e26f38b463e30452795550a927`.
Reviewed source-inventory SHA-256:
`fec8a1041625d33ceb86287a75219b62d0a8432b2e7056d34a698a35891cf02f`.
All three module hashes in that inventory match the current files. I also
read the registry, measurement functions, target driver, four workload bodies
and measurement contract.

The primary selection is sound: coroutine `fibonacci` evaluates both `n - 1`
and `n - 2` inside `run(loops)`. Spectral norm's two `+ 1` sites are a useful
secondary exposure. Source spelling alone does not prove eligibility; the
required actual-word and native-work witnesses appropriately precede timing.
The registered loop counts are 4, 1, 2 and 4000 for coroutines, spectral norm,
generators and unpacking. Neither timed generator traversal nor unpacking
contains the selected literal arithmetic. Generator tree construction does,
but it runs in `setup()`.

`internal/driver.py:38` loads and sets up the module before timing. Time mode
then makes one warm-up call and five serial timed calls in one process.
Instruction mode counts fresh whole processes at L and 3L after a cache
warm-up, retaining their normalized difference. Setup is included in each
instruction process; differencing is an estimator of its cancellation, not
an observation that setup costs are exactly zero. The plan states this limit
and avoids treating the serial elapsed values as independent pairs.

**Required correction:** `run.py:292` currently saves only
`(high - low) / (2 * loops)` in `values`. `perf_instructions()` overwrites
`scratch/perf.txt` on every call, and the enclosing temporary directory is
removed. The current runner cannot supply the promised raw low/high pairs.
Before activation, require a separately reviewed runner-level capture of
each original low/high count and its perf evidence before overwrite, together
with the normalized differences and their exact row/arm/round identity.
Do not weaken the raw-vector contract or change workloads to accommodate
the present JSON shape.

The activation manifest should pin `--instr-runs 4 --time-runs 5`, registry
counts, row/arm order and finite timeouts. Four rows, two arms and two rounds
then yield 64 low/high pairs and 80 serial elapsed samples, excluding probes
and warm-ups. The runner strips inherited `PYTHON*` variables and restores
only hash seed zero; describe benchmark children as OFF/N0 unless a reviewed
mechanism explicitly changes that. Inherited verifier or root-stat variables
alone cannot establish child policy.

The primary 1% instruction gate plus lower elapsed medians in both rounds is
a reasonable selection rule for further qualification. It is not evidence
of statistical significance. Make the stated noisy/inconclusive branch
explicitly non-accepting, retain control regressions, and do not promote the
secondary row after a failed primary. Review the actual evaluator growth
cost before inspecting performance scores; a score should not decide the
acceptable representation cost retrospectively. No extra samples are implied.

Additional exact inputs, SHA-256:

- `boil/bench/run.py`: `7ec1cdf621835b84b273219bbde1966b711032c81effbb15268b377af614eaf8`
- `boil/bench/internal/driver.py`: `7e61be5cff4b2a963a510ae8daab6f2040489538cc549806d6107670d59e25c6`
- `boil/bench/internal/kernels/unpack_sequence.py`: `716158d751ed3555f62df0bb96ee09ac02b7f92485b57c21110b02ac4265a3b2`
- `boil/design/measurement.md`: `b49331ac2fe7f93b9c0447c7e43044e96cdb015c7e99b41ebb344b7fca23c237`

No benchmark, build, target interpreter or profile was run. The source review
clearance remains limited to bounded native feedback.

## Revised plan disposition

At 21:06 UTC I reread the complete revised plan, SHA-256
`103c37231e718fd5d6bc013ea4edebe3696460769ef6b640bf8edf4f1a59401a`.
It explicitly requires separately reviewed original low/high perf capture
before activation, pins the 4/5 sample counts, states OFF/N0 child scope,
prohibits acceptance of noisy/inconclusive evidence, and adds a pre-score
evaluator native-text growth cap of 5%. These changes resolve the original
plan finding and the decision clarifications above.

No further held-plan blocker remains. The later concrete manifest must
identify the parent/candidate native text measurement consistently, bind the
capture and raw record identities, and retain the declared counts/order and
quiet-window cleanup. The separately reviewed capture is still a prerequisite;
this disposition does not certify a capture implementation that does not yet
exist or authorize the timing window.
