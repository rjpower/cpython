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
| H6 | Hot-code layout of `ir_evaluate`: diagnose logging_silent's front-end stalls; hot/cold separation, hot-handler order, alignment. Lands only if logging_silent improves clearly and the screen geomean and every screen case hold | `pyjit/gap82-h6` | parked, no commits (`E59/gap82/h6/diagnosis.md`). The loss is µop-cache set clumping (Zen 2: 64 sets × 8 ways), not capacity: about 140 hot runs fit, but placement decides whether a few sets overflow. Decoder µops are 0.49G on checkpoint78 against 2.7–3.8G on H5. H5 is bad at 7 of 8 start phases of `ir_evaluate`, and lucky alignment flags fail at other phases. Hot-first order, the `hot` attribute and outlining unsampled handlers do not help, and GCC ignores `cold` on computed-goto labels. The robust lever is fewer runs on the hot call path (`call_bind_direct`, 49 of about 210 ways), which goes to TX and K2a |
| CS | Constant slots with no fixed cost: 0/1/3/7 slots at about 502/505/507/511 instructions per call (H5: 502/522/551/606) | `pyjit/gap82-cs` | merged (`167a76860ab`). Pair-copy fill from a padded image; the clear tests the tag first. 497/514/532/567 per call; kernels −0.4% to −1.4%. The target is unreachable with constants in frames: the first slot has a floor of about 14–18. CS made `test_gc_verify` flaky (5/12): a conservative-scan test whose stale addresses in PyMem blocks depend on code layout. The NULL-above-top invariant holds (gdb). The test now allows 40 fresh-address attempts, merged as `3f8be39eb43` |
| TX | One call mechanism: generic, keyword, `CALL_FUNCTION_EX`, instrumented and guarded-miss calls on the operand helpers; the call transaction, view and record deleted | `pyjit/gap82-tx` | merged (`dd2ed0b5688`). `ceval.c` +471/−1,235. Megamorphic sites: plain −11%, keyword −28%, `f(*a)` −24%. A bound-method operand is now released after the callee binds (finalization timing) |
| SL | List slice assignment: snapshot only `a[i:j] = a`, as upstream does (A1 item 1: about 10% of fannkuch's instructions) | `pyjit/gap82-sl` | merged (`f962cf96ab7`). The release order already matched upstream's. fannkuch −13.4% instructions (0.918× upstream); BUILD_SLICE inlined |
| G | Plain generator transfers inline with one plain-transfer test; cheaper generic `FOR_ITER` and list-subclass iteration; unpack straight into local homes (A1 items 4, 7, 8) | `pyjit/gap82-g` | merged (`4df142af734`). Plain send, yield, return and FOR completion run inline in `ir_evaluate` (hot labels, two shared exits); six non-plain steps remain helpers. Unpack stores straight to locals and closes the window once, only for owned values. Subtypes iterate by index while they keep their base's `__iter__`; dict-iterator next specialised; FOR_ITER results go straight to the local. Instructions: generators 0.873× H5, unpack_sequence 0.868×, coroutines 0.962×; comprehensions unchanged. Allocator +2.81M |
| K1 | Operand reads and releases: a proved per-site bit for plain local object operands, skipped releases of non-owners, fresh destinations (H2's dead markers), the polymorphic shape guard (A1 item 6) | `pyjit/gap82-k1` | merged (`859fa0dc7ee`). No per-site bit, because receivers may be inline ints. Instead: in-place pointer guards with no retest; null-free DUP and CLOSE where a guard proves non-null; taken views from the generator's nullable facts; the polymorphic row from the feedback pointer. Kernels −0.4% to −2.5%; allocator −1.75M |
| K2a | Python calls: decode once, single-pass argument fill, one plain retire path, C-to-Python re-entry (A1 items 2, 5, 10), and binding plans for kw-only, `*args`/`**kwargs` targets and `CALL_FUNCTION_EX` with a tuple (A2 N1: +600 to +1,000 per call today) | `pyjit/gap82-k2a` | merged (`b9d07b6d130`; the unit merged `3b0cdcde978` first). One inline plain return (RETURN_VALUE_EMPTY 145 → 116). One binder, compiled three ways, covers kw-only defaults (up to 16), `*args` (up to 8 extras), `**kwargs`, up to 3 keywords and CALL_FUNCTION_EX with an exact tuple; every guard runs before the first move, and anything else goes to the general binder. The exact push decodes once, and the RESUME skip is arithmetic. Per call (before / after / upstream): `f(x)` 375 / 340 / 343, kw-only 1,367 / 880 / 794, `*args, **kw` 1,226 / 867 / 822, `f(*a)` 759 / 655 / 584. Taken branches per plain call 31 → 24 (upstream 45). Instructions 0.988× the pre-K2a merge. Allocator +0.21M. Re-entry sharing was dropped: it would be a second form of the same fill |
| K2b | Native calls: handlers that read operands in place and call the C function after the guard, decode once, plain-store publish (A1 item 3, A2 N2) | `pyjit/gap82-k2b` | merged (`3b0cdcde978`). Builtin, method-descriptor and class calls gather up to four operands and call the value entry, or the object entry through the adapter; arity and receiver guards in the helpers; callable guards compare bits; the generic native tail drops the frame refreshes. abs 333 → 268 (upstream 199), d.get 603 → 479 (360), append+pop 607 → 473 (482). isinstance unchanged (854 against 527; its cost is in the callee chain). Allocator +1.85M |
| N34 | The type-cache lookup reads the tag directly (admission folded into the tag); a leaner `generic_getattr_value`; `_elementtree` getters as value getsets (A2 N3, N4) | `pyjit/gap82-n34` | merged (`18dc78fc0d2`; magic 64433). The type-cache lookup returns the version or 0, with admission checked only at tag assignment and insertion; `generic_getattr_value` and `_PyValue_GetMethod` borrow the class attribute and pin it only before Python can run; four `_elementtree` getters are value getsets, and `LOAD_ATTR_GETSET` (handler 406) calls them directly. xml_etree −6 to −11% instructions, logging −3%, json −1.3% |
| N56 | Extension accessors built in place and borrowed; borrowed dict iteration where no Python code runs; a one-pass str-key insert (A2 N5, N6) | `pyjit/gap82-n56` | merged (`61988bb9357`). Tuples packed in place, a one-pass insert for exact str keys, and borrowed reads in json, pickle, textio and dict views, each with its no-Python argument in a comment. json_dumps −5.6%, json_loads −6.9%, pickle −5.4%, unpickle −9.5%; xml −0.6 to −1.2% (the rest of the xml gap is evaluator, call, attribute and GC-visit work) |
| ST | Startup: `python -I -c pass` runs 530M instructions against upstream's 46M, and a five-module import 1.56G against 144M, mostly in code-construction proofs (`owner_handler_safe`, `_PyIR_NormalPhysicalTransfer`, `semantic_leaf_step`). Attribute, classify and cut | `pyjit/gap82-st` | merged (`e0723064126`). `-c pass` 530M → 65.6M (upstream 45.5M); the import line 1,557M → 205M (144M). Proofs that only reject malformed input run only under validation (`!NDEBUG` or `PYTHON_VALIDATE_CODE=1`); the empty-return dataflow runs only for compiler output; the first-execution plan build is gone; `owner_handler_safe` is linear. Compiled output is byte-identical over 765 `Lib` files |
| IP1 | Merge TX, SL and CS (`167a76860ab`); full suites, release then debug; kernels; per-call figures; timed all-29 against H5 | `pyjit/gap82` | done; suites: the known set, plus `test_gc_verify` from CS (fixed later). Kernels −0.6% to −1.0% instructions against H5. The timed campaign was refused by the core-quiet gate while agents used CCX1, so agents moved to CPUs 0-3,8-11 |
| IP2 | Merge the remaining units (G, K1, K2a, K2b, N34, N56, ST); full suites; timed all-29 against H5, golden73, upstream and checkpoint78; this record's results; the report | `pyjit/gap82` | done at `5194e957c5d`; see [Results](#results) |

Each unit is one Sonnet agent with:
- a time budget (3–4 hours);
- a small-first test ladder;
- a 30-minute progress log;
- a scope cut.

The analysis units change no source. During phase 1, H6 alone uses CPU 15,
for its PMU probes and screens. Root runs no campaign until H6 reports.

## Results

IP2 is `pyjit/gap82` at `5194e957c5d`: H5 plus every unit except H6. The
merged allocator product is 45.45M of 50M (H5: 41.9M).

### Timed all-29

These are time ratios, gap82 against each control. Each control ran 2
rounds on CPU 15, giving 58 paired rows (`E59/gap82/bench/ip2-summary.txt`).
The H5 columns are H5's own campaigns from the same day.

| Control | gap82 | H5 |
| --- | ---: | ---: |
| upstream | **0.992** | 1.065 |
| checkpoint78 | 0.899 | 0.958 |
| golden73 | 0.884 | 0.944 |
| H5 | 0.928 | — |

Against upstream, by the groups in [Where the gap is](#where-the-gap-is):

| Group | H5 | gap82 | gap82 / H5 |
| --- | ---: | ---: | ---: |
| Library-heavy (12) | 1.237 | 1.106 | 0.898 |
| Generators and iteration (6) | 1.185 | 1.117 | 0.924 |
| Attributes and calls (3) | 1.075 | 1.016 | 0.956 |
| Faster than upstream (8) | 0.782 | 0.765 | 0.967 |

The summed log-excess goes from +1.816 to −0.223.

- **Faster than upstream (10 cases):** spectral_norm 0.496, float 0.680,
  nbody 0.687, richards 0.768, chaos 0.820, raytrace 0.882, go 0.950,
  deltablue 0.959, logging_silent 0.960 and fannkuch 0.975.
- **Within 6% (6 cases):** unpickle 1.011, regex_compile 1.043, pickle
  1.048, deepcopy_memo 1.050, xml_etree_generate 1.050 and json_loads 1.057.
- **Above (13 cases):** generators 1.337, deepcopy_reduce 1.217, deepcopy
  1.196, logging_simple 1.141, xml_etree_iterparse 1.137, comprehensions
  1.135, unpack_sequence 1.133, logging_format 1.128, json_dumps 1.107,
  xml_etree_process 1.101, xml_etree_parse 1.098, nqueens 1.090 and
  coroutines 1.062.
- **Slower than H5:** logging_silent 1.150 (1.480× checkpoint78), nbody
  1.038 and generators 1.022. The largest gains against H5 are fannkuch
  0.825, xml_etree_generate 0.826, spectral_norm 0.826, xml_etree_process
  0.829, unpickle 0.836 and json_dumps 0.867.

### Instructions

Per-loop user instructions for all 29 cases (`E59/gap82/int/k2a/instr-table.txt`):
- the geomean is 0.912× H5 and 1.035× upstream, against H5's 1.135×;
- every case runs fewer instructions than in H5;
- logging_silent (0.978) and nbody (0.975) move least.

Startup (`E59/gap82/int/ip2/startup.txt`):

| Command | H5 | gap82 | upstream |
| --- | ---: | ---: | ---: |
| `python -I -c pass` | 530.0M | 63.6M | 45.5M |
| `-I -c "import json, pickle, logging, copy, xml.etree.ElementTree"` | 1,556M | 197.5M | 144.3M |

### Time does not follow instructions in small hot sets

The timed gain against H5 (0.928) is smaller than the instruction gain
(0.912), and three cases lost time while running fewer instructions. In
each case the loss is code placement, which H6 diagnosed for
logging_silent:
- **logging_silent.** All unit builds run 13.5–13.9K instructions per loop.
  Cycles range from 3,330 (SL) to 6,187 (G), and legacy-decoder µops from
  2,021 (K2b) to 9,947 (merged). H5 and SL run identical instructions but
  differ by 30% in cycles (`E59/gap82/int/st-n56-g-k2b/cycles-units.txt`).
- **generators.** IP2 and G's own build retire the same instructions,
  branch misses, taken branches, i-cache misses and decoder µops, but IP2
  takes 12–16% more cycles. The difference matches the i-cache fetch
  stalls (65M → 82M per loop); store forwarding and queue stalls are flat
  (`E59/gap82/int/ip2/generators-pmu.txt`). The interim campaign before
  K2a measured generators at 0.911× H5; IP2 measures 1.022×.
- **deltablue** shows the same thing in the interim campaign: 1.019× H5 at
  0.926× the instructions. It improved to 0.917× at IP2 after K2a cut the
  taken branches per call.

H6 found no robust layout fix, and the plan forbids padding to find a
favourable placement. The lever that held up is fewer taken branches on
hot paths (K2a: 31 → 24 per plain call).

### Correctness

The IP2 full suites ran release and then debug, 55,194 tests each
(`E59/gap82/int/ip2/full/`). Both fail exactly H5's counts:
- test_external_inspection 1;
- test_ir_error_effects 5;
- test_generated_cases 1 error and 2 failures;
- test_sys_settrace 8;
- test_tools 1.

Two problems were found and fixed during integration:
- **test_gc_verify.** CS made it flaky. Its conservative scan is
  layout-sensitive, and the invariant was confirmed with gdb.
- **test_generated_cases.** K1 and G added 23 failing subtests. They went
  unnoticed because the module was on the known list by name; known
  failures now count by subtest. All 23 were intended generator-output
  changes, and the tests now check the new forms' properties.

The release suite now takes 4 min 40 s. At IP1 it took 16 min under heavy
load; ST's startup cut accounts for most of the difference.

### Open

- **Remaining gap to upstream** (largest first):
  - generators: helpers G left alone, namely `RETURN_GENERATOR` and the
    allocation slow path, generator finalization, and the `FOR_ITER_GEN`
    and `YIELD_VALUE` decode;
  - deepcopy and deepcopy_reduce;
  - comprehensions: `any(genexpr)` re-entry;
  - the xml cases: evaluator, call, attribute and GC-visit work;
  - logging_format and logging_simple.
- **Code placement** now limits the call-heavy cases with small hot sets.
  logging_silent is 1.48× checkpoint78.
- **isinstance** is 854 instructions per call against upstream's 527, in
  the value-layer callee chain.
- **Release builds trust hand-built code.** They no longer run the
  malformed-input proofs. `PYTHON_VALIDATE_CODE=1` turns them back on.
