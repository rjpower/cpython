# R4: shared first-result publication contract

Integrated at runtime `b58036e9718b87b95ad83b72e10d4578942a61d6`, on parent
`225480a4250fa8b9760f355875aac30ec76c40c3`. R4 adds a checked success-path
publication phase shared by emission and metadata. Six rows declare canonical
first-temporary publication before receiver retirement: generic LOAD_ATTR and
five specialized loads. Generated interpreter bodies are unchanged; metadata
adds one `result_phase` field.

`IR_RETIRE_PUBLISHED_RESULT(result, receiver)` uses one checked producer record
instead of a handler-name list. The checker rejects ambiguous result/owner
assignments, aliases, result use before retirement and suffix effects or
overlapping positions. Variable-length companion output must be terminal. The
generic helper bridge has the same first-output phase but can invoke Python
earlier. Folded locals keep the old home installed until receiver retirement
begins; callbacks may mutate it. After successful refresh, final publication
replaces the actual current local. Failure preserves callback writes and
closes the C-owned result. This phase is not a root map, failure snapshot, whole-word
purity claim or native JIT implementation.

The initial cold review requested changes for two checker gaps: a continuing
generic completion could consume an input before the helper, and a result
assignment statement could also consume the assigned result. The original
[cold review](cold-r4-review.md) and its rejected source shapes remain part of
the record. The repaired checker rejects known input consumption, transfer,
mutation and address escape on continuing paths, while preserving the terminal
abort cleanup; it also requires a direct top-level result assignment whose
name occurs once in that statement. The bounded repair and review are in
[repair notes](r4-generic-prefix-repair.md) and
[cold repair review](cold-r4-review-2.md). These checks recognize declared
operations and source shapes; arbitrary C ownership and pointer provenance
remain review premises.

Validation is split by revision. Before the repair, the initial eight-file
release and debug runs each passed 1,100 tests with one skip, and lint covered
306 units with no failing findings. Those results do not validate the repaired
checker. After repair, the isolated generator passed 73 tests; release and
debug builds each checked 117 modules and the directly affected generator and
contract suites each passed 97 tests. Regenerated interpreter and metadata
products matched the initially validated R4 products. The post-repair focused
test count is 97 per profile, not 1,100. R4-specific value-API lint and initial
focused runs are retained evidence; current coherent-batch checks are recorded
in [R5 integration](integration-r5.md).

R4's sequential seven-kernel basic screen had candidate raw elapsed geomean
factor `0.99936338` and upstream-control drift `1.03883914`; results were
mixed and establish no speedup. Release/debug identity snapshots matched
`b58036e9718`; ELF IDs were
`a410f4a9c634117fdb4921079b2bcdcdfd190b92` and
`cc0a5cdc68227964971994955eb13b0c25f9cc0a`. The later coherent R5 boundary
passed release and debug full suites: each passed 54,380 tests with no
failures (2,467 and 2,405 skips); 537 files passed, 79 were skipped and 11
resource-denied out of 627. The stats build, identity link and smoke check
also passed. This qualifies the coherent R5 tree, not a standalone R4-only
tree. The direct R4/R5 whole-kernel comparison is complete and the independent
whole-workload review accepts keeping the bounded R5 optimization, with local
costs and uncertainty retained. The full R1–R5 report records the remaining
overall interpreter gap; independent final evidence/report review is
complete. See [whole-workload review](whole-workload-r4-r5-results-review.md)
and [final report](../../bench/results/interpreter-architecture-20261107.md).

Evidence is under
`python-boil-the-ocean/build/pyjit-link-wave1/interpreter-architecture-20261107/`:
`r4-checks.json`, `r4-repair-checks.json`, the release/debug build and test
logs, `r4-identities.json`, and `r4-basic/`. Earlier unit failures and both
rejected cold-review findings remain preserved as diagnostic history.
