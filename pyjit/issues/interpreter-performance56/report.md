# Interpreter performance checkpoint

Status: latest hot-set checkpoint **f6ef4a81a87**; broad/PMU checkpoint
**6a00c6a08d4**. Neither is selected. Golden54 remains
61015a9452d9069646fb5fede9ff963896833b89. The [plan](plan.md) owns workstreams;
[status](../../STATUS.md) owns aggregate qualification. The [combined design](../../design/proposals/interpreter-performance56.md)
and [review](review.md) distinguish proposed contracts from selected behavior.

## Latest integration screen

At source/native **f6ef4a81a87**, the second scalar replay repair and checked
generator-recipient admission are integrated with regenerated products. Both
builds complete 117 modules/79 shared. Nine completion/observer modules pass
all 84 methods per role. The chained proof assertions now pass; its debugger
fixture needs a valid line to reach the intended active-call denial. Full
qualification is pending.

Two balanced pairs against immutable golden54 give the following elapsed
ratios. Source/native identities remain stable and all commands succeed.

| Case | /golden54 | Pair range |
| --- | ---: | ---: |
| DeltaBlue |1.022672 |1.003553–1.042155 |
| Generators |0.949424 |0.948526–0.950322 |
| Coroutines |1.065409 |1.026978–1.105277 |
| Richards |0.968734 |0.967106–0.970366 |
| Comprehensions |1.027499 |1.022269–1.032757 |
| Silent logging |1.017075 |0.956376–1.081626 |

Generators improve in both pairs, consistent with removing the direct-handoff
admission gap. This composition screen does not isolate that change from the
other integrated components. Coroutine and comprehension regressions block
promotion. Logging is too variable here for a useful small-difference claim.
There is no same-head upstream, broad or PMU result yet. Evidence:
`E56/recipient-hot6-golden`, build/test manifests under E55/perf56-recipient-*.

## Earlier broad and PMU checkpoint

This source combines model55 scalar/VALUE facts, the exact-tuple CALL_EX probe
and repairs to cold call-state observations. Both builds complete 117 modules,
79 shared. The native call-transaction module passes all 12 methods per role.
The earlier VALUE gate passes 33 host methods and full generation, but its new
chained-readability runtime witness fails in both roles; 35 other focused methods
pass. A reviewed repair and direct native-completion entries are integrated in
newer source 4642e92808c. Its release core gate passed 50/51 methods, with the
chained-read proof still failing. The second replay correction and generator
recipient fix are now integrated at f6ef4a81a87; both builds complete 117/79.
The new proof assertions pass. Its remaining witness failure is fixture geometry: the synthetic image has
no current line, so its redirect is rejected before the intended active-call
check. A valid line target is being prepared without changing that contract.
Nine completion/observer modules pass all 84 methods in each native role.
Its separate hot-set screen is reported above. No green full model55 suite or new golden
is claimed.

The two balanced control campaigns use unchanged workloads, GCC15.2, CPU15,
hash0 and JIT disabled. Source/native identities are stable and all timing
commands complete successfully. Lower ratios are better.

| Case | /golden54 | Pair range | /upstream | Pair range |
| --- | ---: | ---: | ---: | ---: |
| deltablue |1.036517 |1.017329–1.056067 |1.238249 |1.231110–1.245430 |
| generators |0.990394 |0.952077–1.030253 |1.475289 |1.450736–1.500257 |
| coroutines |1.069867 |1.067763–1.071976 |1.345727 |1.336884–1.354628 |
| richards |0.966148 |0.955407–0.977010 |1.179492 |1.139383–1.221013 |
| comprehensions |0.994715 |0.993666–0.995765 |1.399420 |1.395407–1.403444 |
| logging_silent |0.990105 |0.986458–0.993767 |0.746656 |0.733864–0.759671 |

All 29/upstream geometric mean is **1.161978**, with six wins. The golden screen
is mixed, including a repeatable coroutine regression in these two pairs; this
checkpoint is not promoted. The exact-tuple shortcut has zero executed hot six
sites in the diagnostic census, so the screen cannot establish its isolated
effect. It remains a bounded cleanup pending broad evaluation. Historical
8232 and selected54 measurements are separate campaigns, not multiplicative
controls; see the [model55 report](../call-model55/report.md) and
[selected54 report](../frame-state54/report.md).

Evidence prefix is BASE/pyjit-link-wave1/interpreter-performance56:
`call-ex-hot6-golden`, `call-ex-all29-upstream`, `call-ex-all29-command.json`.
E55 `perf56-call-ex-geometry-*` owns build/module logs; `perf56-call-ex-native`
retains ELF SHA256 04ad558cc971814c9c723360ed1413a1adad489fe0f32d1ee78a726d00888951
and disassembly. The exact arm branches before its owning DUP/object conversions;
the cold fallback still gives the function a substantial prologue. Native work
removal is established on that arm, not a broad throughput win.

## Matched counters and profiles

Three balanced PMU pairs per case measure the warmed workload interval, with
100% event running and stable identities; setup and warmup are excluded.

| Case | Native instructions/upstream | Cycles/upstream |
| --- | ---: | ---: |
| DeltaBlue |1.366815 |1.246914 |
| Generators |1.492557 |1.471863 |
| Coroutines |1.353759 |1.336578 |

Generators and coroutines execute substantially more native work at roughly
similar cycles per native instruction. This supports reducing executed work;
it does not identify frame clearing as the cause. DeltaBlue retires the extra
instructions with better aggregate instruction throughput. These are per fixed
workload, not per Python instruction or exclusive operation cost.

Matched cycle profiles have about 2,500 samples per process. Candidate generator
self samples concentrate in SEND_GEN (27.91%) and YIELD_DELEGATED (13.94%). The call
audit confirms their values are already canonical; return-slot publication
handles recipients and is not redundant numeric normalization.
Coroutines show exact-call entry 9.81%, GET_AWAITABLE 7.40%, generator deallocation
6.47%, CompleteGeneratorReturn 6.28% and SEND_GEN 5.75%. DeltaBlue shows exact-call
entry 11.25%, shape-read variants 7.48%/5.01%, method lookup 6.81% and fast frame
retirement 6.54%. These disjoint physical-symbol self buckets include inlined
work; they are not removable fractions or an additive speedup prediction.
Upstream places much of this work in one EvalFrameDefault symbol, so subtracting
symbol percentages across runtimes is invalid.

Evidence: `call-ex-perf-stat-affined` (18/18 valid), its `paired-summary.json`,
and `call-ex-perf-record` (6/6 valid) with retained binaries, raw perf data and
flat reports. The first stat invocation was rejected before collection because
the orchestrator lacked CPU15 affinity; it is retained separately.

## Experiment decisions

- Exact-tuple preparation: source/native path removal and semantic checks pass;
  no hot six frequency or broad speedup claim. Keep isolated from parity claims.
- Initial multi-use COPY web: close at zero eligible candidates. Across 21 unique
  kernel sources/29 cases and 426 CFGs, existing passes remove 1,798/3,092 fresh borrow
  roots; 204 encounter callbacks, 1068 require nonborrowed terminals, 12 cross line/EH
  and 10 cross unsupported COPY identities. This rejects the initial narrow slice,
  not broader borrowing. No compiler pass is added. Frozen parser/input hashes and
  source bounds are under `multi-use-borrow-census`.
- Polymorphic managed shape reads: propose direct use of complete scalar rows.
  Corrected instrumented visits give 1,709,700 DeltaBlue and 1,253,922 Richards visits
  at sites with pre-warm polymorphic shape history. This is coverage, not actual
  row hits/misses. Other four hot cases have no polymorphic attribute sites. The
  original diagnostic divided word offsets by 8 incorrectly; its results are
  explicitly invalidated. Use `site-counts-corrected-196284`, not that first output.
- Generator handoff: source audit finds local/discard recipient admission rejects
  FOR_ITER_GEN despite the handoff supporting those destinations. A typed recipient
  capability and all-publication caller audit passed source review and are
  integrated; focused runtime checks and timing are in progress. No speedup
  is assigned yet.
- Canonical list/tuple transport: one checked helper/action contract replaces the
  list body recognizer and supplies tuple canonical stores. No executed tuple
  specialization appears in hot six; broad applicability/native removal must be
  established before expansion.
- Direct native completion: three literal-kind entries share the same body. This
  addresses terminal completion, not ordinary generator yields or Python returns.
  Qualification and code-size/spill measurements are pending.

## All 29 direct upstream screen

| Case | Ratio | Pair range |
| --- | ---: | ---: |
| richards |1.179492 |1.139383–1.221013 |
| go |1.142874 |1.138904–1.146857 |
| nbody |0.860349 |0.858752–0.861948 |
| float |0.776499 |0.771998–0.781026 |
| spectral_norm |0.758322 |0.755049–0.761609 |
| raytrace |0.990191 |0.989406–0.990976 |
| deltablue |1.238249 |1.231110–1.245430 |
| chaos |0.935079 |0.928605–0.941597 |
| nqueens |1.233972 |1.229147–1.238816 |
| fannkuch |1.166003 |1.088058–1.249531 |
| generators |1.475289 |1.450736–1.500257 |
| comprehensions |1.399420 |1.395407–1.403444 |
| coroutines |1.345727 |1.336884–1.354628 |
| unpack_sequence |1.241036 |1.236419–1.245670 |
| deepcopy |1.358876 |1.357546–1.360206 |
| deepcopy_reduce |1.376390 |1.368892–1.383930 |
| deepcopy_memo |1.252065 |1.248598–1.255543 |
| json_dumps |1.358912 |1.354756–1.363081 |
| json_loads |1.195562 |1.186498–1.204695 |
| pickle |1.042056 |0.974893–1.113846 |
| unpickle |1.223882 |1.218986–1.228797 |
| regex_compile |1.198640 |1.181583–1.215944 |
| logging_format |1.307181 |1.230162–1.389022 |
| logging_silent |0.746656 |0.733864–0.759671 |
| logging_simple |1.258614 |1.258298–1.258930 |
| xml_etree_generate |1.331602 |1.303280–1.360539 |
| xml_etree_iterparse |1.293528 |1.291685–1.295374 |
| xml_etree_parse |1.140457 |1.138828–1.142089 |
| xml_etree_process |1.426324 |1.414941–1.437800 |

## Remaining gates

Qualify the current replay/completion composition, then the reviewed ordinary
transport, shape-cache and generator-recipient increments. Run focused checks
per change and full diagnostics at coherent shared-contract/lifecycle gates.
Confirm promising performance with more pairs, inspect regressions, and measure
the final composition directly against golden54/upstream. Preserve logging gains.

Full static M4, arbitrary allocation, IRSC2 native qualification and the bitmap
monitor/profile/debug fixtures remain independent unfinished work. The bitmap
candidate is unselected and has no current timing. Selected54 debugger/LLTrace,
GC-verifier and profiling limits remain documented; only GDB pretty printing is
quarantined. Startup RSS and zero-box counts are not selection gates.
