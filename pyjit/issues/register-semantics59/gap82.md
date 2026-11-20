# Gap82: the gap to upstream after H5

Approved 2026-11-20. After H5 landed, the user asked to pursue every area of
the post-H5 analysis at once:
- fan analysis and implementation out to agents;
- root integrates and benchmarks at integration points;
- root reports at the end.

As in the handlers81 rollout, gates are correctness. Measurements are recorded
and attributed, not gated, with one exception: H6, whose only purpose is
speed.

Base: H5, `pyjit/handlers81` at `d6d1ed5ac93`
([record](handlers81.md)). Integration branch `pyjit/gap82`. Evidence and
briefs are in `E59/gap82/`; the shared setup is `common.md` and the running
log is `coordinator.md`.

## Where the gap is

H5's timed all-29 geomean is 1.065 against upstream. The interpreter-bound
cases already beat upstream: spectral_norm 0.58, nbody 0.68, float 0.72,
richards 0.82, chaos 0.84, raytrace 0.90 and go 0.98. The rest of the gap
falls into four groups. The first column is the group's share of the summed
log-excess, where the whole gap is 100%.

| Group | Share of the gap | Cases (H5 / upstream time) |
| --- | ---: | --- |
| Library-heavy | 140% | xml_etree ×4 (1.14–1.35), deepcopy and deepcopy_reduce (1.32, 1.33), json (1.17, 1.29), pickle and unpickle (1.12, 1.15), logging_format and logging_simple (1.22) |
| Generators and iteration | 56% | generators 1.30, comprehensions 1.23, unpack_sequence 1.21, fannkuch 1.18, nqueens 1.15, coroutines 1.07 |
| Attributes and calls | 12% | deltablue 1.05, deepcopy_memo 1.09, regex_compile 1.09 |
| Faster than upstream | −108% | the seven cases above, plus logging_silent |

H0–H5 improved the library-heavy cases by only 0–4.5% over checkpoint78, and
nobody had profiled them against upstream by symbol. logging_silent runs at
1.31× checkpoint78's time. Its cycles rose at H4, when front-end stalls
roughly doubled.

## Units

| Unit | Content | Branch | State |
| --- | --- | --- | --- |
| A1 | Analysis: H5's evaluator against upstream, by category, call case and handler ledger; the two H4 hypotheses (outlined generic heads, the generator steps' calls) | — | done: ten ranked items (`E59/gap82/a1/report.md`); generic heads small overall; the generator cost is the step protocol, not its calls |
| A2 | Analysis: the 12 library-heavy cases against upstream, by symbol and DSO; causes; units for N | — | done (`E59/gap82/a2/report.md`): instructions explain the whole time gap; the evaluator holds 0.30 of the summed log-excess; the rest is library-side value-API paths. Proposes N1–N6 (12-case instruction geomean 1.250 → about 1.163, estimated) and found the startup cost |
| H6 | Hot-code layout of `ir_evaluate`: diagnose logging_silent's front-end stalls; hot/cold separation, hot-handler order, alignment. Lands only if logging_silent improves clearly and the screen geomean and every screen case hold | `pyjit/gap82-h6` | running |
| CS | Constant slots with no fixed cost: 0/1/3/7 slots at about 502/505/507/511 instructions per call (H5: 502/522/551/606) | `pyjit/gap82-cs` | running |
| TX | One call mechanism: generic, keyword, `CALL_FUNCTION_EX`, instrumented and guarded-miss calls on the operand helpers; the call transaction, view and record deleted | `pyjit/gap82-tx` | running |
| SL | List slice assignment: snapshot only `a[i:j] = a`, as upstream does (A1 item 1: about 10% of fannkuch's instructions) | `pyjit/gap82-sl` | running |
| G | Plain generator transfers inline with one plain-transfer test; cheaper generic `FOR_ITER` and list-subclass iteration; unpack straight into local homes (A1 items 4, 7, 8) | `pyjit/gap82-g` | running |
| K1 | Operand reads and releases: a proved per-site bit for plain local object operands, skipped releases of non-owners, fresh destinations (H2's dead markers), the polymorphic shape guard (A1 item 6) | `pyjit/gap82-k1` | running |
| K2a | Python calls: decode once, single-pass argument fill, one plain retire path, C-to-Python re-entry (A1 items 2, 5, 10), and binding plans for kw-only, `*args`/`**kwargs` targets and `CALL_FUNCTION_EX` with a tuple (A2 N1: +600 to +1,000 per call today) | | after IP1 (builds on TX and CS) |
| K2b | Native calls: handlers that read operands in place and call the C function after the guard, decode once, plain-store publish (A1 item 3, A2 N2) | | after IP1 (builds on TX) |
| N34 | The type-cache lookup reads the tag directly (admission folded into the tag); a leaner `generic_getattr_value`; `_elementtree` getters as value getsets (A2 N3, N4) | `pyjit/gap82-n34` | running |
| N56 | Extension accessors built in place and borrowed; borrowed dict iteration where no Python code runs; a one-pass str-key insert (A2 N5, N6) | `pyjit/gap82-n56` | running |
| ST | Startup: `python -I -c pass` runs 530M instructions against upstream's 46M, and a five-module import 1.56G against 144M, mostly in code-construction proofs (`owner_handler_safe`, `_PyIR_NormalPhysicalTransfer`, `semantic_leaf_step`). Attribute, classify and cut | `pyjit/gap82-st` | running |
| IP1 | Merge CS, TX, SL and an accepted H6 (and G and K1 if ready); full suites, release then debug; kernels; per-call figures; timed all-29 against H5 | `pyjit/gap82` | |
| IP2 | Merge the remaining units (G, K1, K2a, K2b, N34, N56, ST); full suites; timed all-29 against H5, golden73, upstream and checkpoint78; this record's results; the report | `pyjit/gap82` | |

Each unit is one Sonnet agent with:
- a time budget (3–4 hours);
- a small-first test ladder;
- a 30-minute progress log;
- a scope cut.

The analysis units change no source. During phase 1, H6 alone uses CPU 15,
for its PMU probes and screens. Root runs no campaign until H6 reports.
