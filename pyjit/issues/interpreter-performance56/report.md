# Interpreter performance checkpoint

Status: integrated and measured candidate **82e857384f8**, not selected.
Current broad/control timing, PMU and confirmation are complete; full
qualification remains pending. Golden54 remains
61015a9452d9069646fb5fede9ff963896833b89. The [plan](plan.md) owns workstreams;
[status](../../STATUS.md) owns aggregate qualification. The [combined design](../../design/proposals/interpreter-performance56.md)
and [review](review.md) distinguish proposed contracts from selected behavior.

The inline list-accessor followup 5e3ac28ca03 is **rejected**. Both builds and
117 targeted methods per role passed, but six-pair confirmation gave
coroutines 1.107292/golden (all six regress) and raytrace 1.035096 (no useful
improvement). It removed a call and six spills from the list handler, shrinking
831→771 bytes and 48→16 stack bytes, while whole .text grew 384 bytes. Native
work removal alone did not pass the performance gate. The source move has no
identified semantic defect; the regression's cause is unproven. Branch
pyjit/perf56-inline-list-borrow and E56/inline-list-* remain evidence.

Root reverted it at 7a21d4c5027. That commit's source tree is exactly the measured
82e857 tree (c48ca12bab75f49a9b704ea093860d040fa86cea). Both native rebuilds pass 117/79;
final coherent qualification remains pending; measurements below retain their actual
82e857 source/native identity until final repeat evidence is available.

## Qualification closure

Full release at7a21 completed55,023 tests with116 failures across13 modules;
576 modules passed. No green full gate is claimed. Debug was interrupted after
5,310 tests/128 files once a real scalar-state inconsistency was identified.
A focused native reproducer confirms consumed-slot reads are accepted on three
public routes and16 empty-retirement checks lose their certificate.

VACATE correctly records logical NO_HOME without promising physical NULL;
older read rejection and empty-home consumers still require SEM_NULL alone.
The reviewed codeobject repair uses one absence predicate at five consumers,
retains all complete/MAY_OWNER/INIT/nullable gates, and writes no new facts.
Physical dirty-inline retirement checks remain MISMATCH controls. Existing
semantic tests are preserved. The repair is integrated atdab1ffaed3d; ac4c7019f9b
adds peered current magic/table pins, exact call names, genuinely unknown unary
controls and the actual cold frame-capture sizeof formula. Both builds pass 117/79.
The three unchanged admission/retirement regression methods now pass in both
roles; broader five-module verification is active.

Seven generated-case failures require separate closure. All six current
sys_settrace jump error names also occur in selected54’s exact release and
debug logs; E56/qualification-fixes/debugger-baseline-comparison.json records
the comparison. They remain unresolved observation limits, rather than new
failures of this composition. No new quarantine or semantic waiver
is introduced. Current admission source differs from the measured82e857 tree;
final qualification and measurement must use the new identity. Evidence:
E56/final-qualification, E55/perf56-admission-diagnostic and
E55/perf56-qualification-verbose. All failed attempts remain sealed.

## Current source composition

Container transport and the exact-coroutine transfer are integrated at
cfb91380cff; global-label initialization repair c4b679bcc48 follows it. Actual
merged-source peers found no blocker. All 51 source methods pass. All-format
comparison against f291 has no changes: GET_AWAITABLE retains STORE_DST and
folding. The host runner's two initial extraction failures omitted os/re;
the corrected runner passes its full suite, then exposed the genuine missing
emitter initialization during full label emission. That source defect is fixed;
full regeneration now passes.

The generated diff changes nine handlers. Nine publication sites across eight
handlers now consume existing canonical facts: list temporary (one), tuple
temporary/final (two), GET_ANEXT final (one), and five NULL/boolean literal
stores. GET_AWAITABLE instead adds a callback-free exact-coroutine identity arm
that transfers its input owner. No new frame field or public operand layout is
introduced. Retained disassembly confirms native removal; current composition
measurements follow below, with older exact-head evidence retained separately.

An independent Sol cold review of actual 113a4ded473 source found no blocker.
Its five followups are: remove a redundant uninstrumented native-call refresh
if it survives compilation; centralize compiler fact invalidation/meet;
migrate the next measured numeric producer into shared facts; count terminal
completion before further ABI work; diagnose logging at the final identity
before widening PIC selection. These are prioritized followups, not speed
claims. The review confirms that own-result joins grant no replay/home
readability, coroutine fallback remains intact, and GIL SHAPE guard/read/acquire
holds no feedback pointer across a callback.

Both native builds pass 117/79 at 113a. The first 18-module release integration
run passes 17 modules and fails only the new executing-coroutine test's expected
exception type. A frozen three-way probe gives ValueError on upstream, golden54
and candidate. One-line fixture correction 82e857384f8 preserves message/state/
cleanup witnesses. Both builds pass 117/79 again; all 154 methods now pass in each role.
The five bootstrap and 27 compile probes also pass. Root retained native ELF/disassembly under
E55/perf56-transport-await-native (SHA256
e9d36ff85b79b03eeeb3d75e86dd59320f78965e1407f3696cee2b16e4555725).
The timing campaigns below use this retained ordinary executable.

## Current merged hot-set screen

Source/native **82e857384f8** passes 154 focused methods per role, the five-plus-27
bootstrap/compile matrix, unchanged-format comparison, full generation and
both 117/79 builds. The first two balanced pairs against golden54 complete with
stable identities and no command errors. Lower elapsed ratios are better.

| Case | /golden54 | Pair range |
| --- | ---: | ---: |
| deltablue |0.960183 |0.936213–0.984768 |
| generators |0.934380 |0.930759–0.938014 |
| coroutines |1.010237 |0.993797–1.026950 |
| richards |0.720826 |0.710400–0.731405 |
| comprehensions |0.975365 |0.974345–0.976387 |
| logging_silent |0.951952 |0.898421–1.008671 |

This screen supports broad qualification rather than selection. Richards,
generators and comprehensions improve in both pairs; DeltaBlue also improves.
Coroutine and logging pairs straddle parity. The earlier f291 logging regression
is not reproduced in this composition, but confirmation must price its variance.
The broad upstream comparison follows. Evidence: E56/transport-await-hot6-golden.

### Six-pair confirmation

Six balanced pairs at the same82e857 identity confirm Richards and generator
gains. All commands succeed and before/after identities match. DeltaBlue,
comprehensions and logging improve in every pair. Coroutine range still crosses
parity; raytrace's mean regression remains, with one faster pair.

| Case | /golden54 | Pair range |
| --- | ---: | ---: |
| deltablue |0.983547 |0.966438–0.995870 |
| generators |0.942484 |0.925687–0.961010 |
| coroutines |0.993560 |0.981109–1.014690 |
| richards |0.718632 |0.716043–0.722528 |
| comprehensions |0.959073 |0.868742–0.981809 |
| logging_silent |0.898868 |0.860832–0.936960 |
| raytrace |1.035032 |0.951701–1.059284 |

Evidence: E56/transport-await-confirm7-golden. This prices the complete
composition, not the individual transport or cache increments.

## Current broad upstream comparison

At the same 82e857 source/native identity, all 29 benchmarks complete two
balanced pairs against upstream with stable identity and no command errors.
The geometric mean is **1.151069**, with seven wins. This is a direct current
comparison; it is not obtained by multiplying previous campaign ratios.

| Case | /upstream | Pair range |
| --- | ---: | ---: |
| richards |0.896864 |0.889290–0.904503 |
| go |1.177112 |1.167363–1.186944 |
| nbody |0.869881 |0.865196–0.874592 |
| float |0.785703 |0.775221–0.796326 |
| spectral_norm |0.821323 |0.812386–0.830357 |
| raytrace |0.986409 |0.979418–0.993451 |
| deltablue |1.155816 |1.153464–1.158174 |
| chaos |0.982420 |0.980109–0.984736 |
| nqueens |1.361104 |1.263257–1.466530 |
| fannkuch |1.299044 |1.270645–1.328078 |
| generators |1.426371 |1.355676–1.500752 |
| comprehensions |1.337121 |1.303472–1.371640 |
| coroutines |1.246663 |1.244788–1.248540 |
| unpack_sequence |1.161511 |1.152529–1.170563 |
| deepcopy |1.378339 |1.370445–1.386279 |
| deepcopy_reduce |1.368204 |1.362243–1.374192 |
| deepcopy_memo |1.291558 |1.271001–1.312448 |
| json_dumps |1.360270 |1.355189–1.365370 |
| json_loads |1.232169 |1.205426–1.259506 |
| pickle |1.138317 |1.101790–1.176056 |
| unpickle |1.182505 |1.138224–1.228508 |
| regex_compile |1.201915 |1.200888–1.202944 |
| logging_format |1.205000 |1.204241–1.205759 |
| logging_silent |0.694034 |0.688199–0.699917 |
| logging_simple |1.224118 |1.181750–1.268004 |
| xml_etree_generate |1.310153 |1.300887–1.319484 |
| xml_etree_iterparse |1.264914 |1.257881–1.271987 |
| xml_etree_parse |1.154992 |1.144444–1.165638 |
| xml_etree_process |1.410014 |1.403358–1.416702 |

Generators and nqueens have wide ranges in this small broad screen. Hardware
counters and confirmation must support narrower causal or small-change claims.
The direct golden comparison follows. Evidence:
E56/transport-await-all29-upstream.

## Current broad golden comparison

The direct all-29/golden campaign at 82e857 completes two balanced pairs with
stable identity and no command errors. Geometric mean **.989410** means about
1.1% less elapsed time overall; 12 cases improve. Richards improves about 29.5%,
generators 6.7% and comprehensions 2.5%. Coroutines are flat. Raytrace regresses
4.6% in both pairs, and several other cases regress 1–2%; these costs remain
visible rather than hidden behind the mean. The six-pair confirmation above
includes raytrace, DeltaBlue and the noisy logging result.

| Case | /golden54 | Pair range |
| --- | ---: | ---: |
| richards |0.705191 |0.695446–0.715073 |
| go |0.992271 |0.987545–0.997019 |
| nbody |1.017068 |1.009451–1.024742 |
| float |0.993251 |0.966341–1.020910 |
| spectral_norm |1.019316 |1.010191–1.028523 |
| raytrace |1.046273 |1.044723–1.047825 |
| deltablue |1.008931 |0.988984–1.029281 |
| chaos |1.017170 |1.012512–1.021849 |
| nqueens |1.021627 |1.018695–1.024569 |
| fannkuch |1.019785 |1.012780–1.026838 |
| generators |0.932627 |0.928299–0.936974 |
| comprehensions |0.974993 |0.973435–0.976555 |
| coroutines |0.998766 |0.997297–1.000237 |
| unpack_sequence |0.992390 |0.984979–0.999857 |
| deepcopy |1.011066 |1.010773–1.011358 |
| deepcopy_reduce |1.015637 |1.015234–1.016040 |
| deepcopy_memo |1.004093 |1.003663–1.004522 |
| json_dumps |1.004954 |0.996084–1.013902 |
| json_loads |1.002591 |0.996391–1.008831 |
| pickle |0.994092 |0.973160–1.015473 |
| unpickle |1.007651 |0.998526–1.016859 |
| regex_compile |1.004233 |0.998206–1.010296 |
| logging_format |0.981771 |0.977204–0.986359 |
| logging_silent |0.956771 |0.898312–1.019035 |
| logging_simple |1.004851 |0.957561–1.054476 |
| xml_etree_generate |1.021530 |1.008064–1.035175 |
| xml_etree_iterparse |1.013624 |1.001751–1.025637 |
| xml_etree_parse |0.992645 |0.987386–0.997933 |
| xml_etree_process |0.997083 |0.989859–1.004359 |

Evidence: E56/transport-await-all29-golden. No isolation claim follows from this
composition comparison, and no promotion precedes coherent qualification.

### Native transport removal

Retained f291→82e857 disassembly confirms canonical publication work disappears.
List subscript text changes 899→831 bytes; tuple 1216→743; GET_ANEXT 927→732.
Static _PyValue_Canonicalize callsites change 1→0, 3→0 and 2→0 respectively.
Tuple local stack reservation falls 32→16 bytes; list 48 and GET_ANEXT 96 remain.
GET_ANEXT retains its required FromNumberSteal constructor and numeric type
tests: canonical publication does not erase result construction. These are
native code changes, not executed call counts or attributed speedups. The
hot6 tuple census is zero; no tuple speed claim follows from this removal.
The list reader remains a native call and gains one pre-call spill plus stricter
object-tag branches; not every instruction improves. Static old disassembly is
retained under E56/transport-disasm-comparison.

## Current matched hardware counters

At 82e857, all 24 stat invocations pass with stable identity and 100% counter
running. Three alternating pairs per case count only the warmed fixed kernel
interval, normalized per completed kernel loop. Ratios are directly against
upstream, independently of the timing table.

| Case | Native instructions /upstream | User cycles /upstream |
| --- | ---: | ---: |
| deltablue |1.361387 |1.148821 |
| generators |1.411737 |1.350387 |
| coroutines |1.307831 |1.237332 |
| logging_silent |0.967588 |0.675614 |

Generators and coroutines still execute substantially more native instructions;
aggregate throughput does not turn that into parity. DeltaBlue also executes
36.1% more instructions despite its smaller cycle gap. Logging executes 3.2%
fewer instructions and about 32.4% fewer cycles. These do not isolate frame
clearing, dispatch or one ownership mechanism. Profiled physical-symbol work
and source/native branch removal must choose the next experiment. Evidence:
E56/transport-await-perf-stat (complete24/24 and paired-summary.json).

### Current physical-symbol profiles

All eight record invocations pass, with stable identity and no lost samples.
Physical-symbol flat reports disable inline expansion and child attribution.
In generators, SEND_GEN is 29.89% self, YIELD_DELEGATED 14.80% and monomorphic
SHAPE_LOCAL_TEMP 9.90%. The changed terminal completion helper is only 1.66%:
more terminal ABI tuning cannot plausibly erase the full resume gap.

Coroutines show exact-call handler 11.28%, CompleteGeneratorReturn 6.29%,
SEND_GEN 5.44%, generator deallocation 5.44%, ClearGenerator 4.78%, MakeCoro 4.63%,
GET_AWAITABLE 4.29% and CopyCreatedGenerator 2.68%. Creation, execution and
retirement all remain targets; the transfer patch does not redesign them.
DeltaBlue shows exact-call 11.91%, PIC-local read 8.28%, method load 6.22%,
monomorphic shape load 6.11% and TryClearExceptCode 5.43%. Logging spends time
in its existing mono loads, call/binding paths and frame retirement, with no
PIC selector execution. Upstream mostly attributes evaluator work to one
EvalFrameDefault symbol; flat buckets cannot be subtracted or summed into a
removable cycle budget. Evidence: E56/transport-await-perf-record and its
flat-report-commands.json.

## Cache execution and previous checkpoints

A separate f291 diagnostic ELF counts only the warmed kernel interval. It is
never timed. Actual managed SHAPE selector outcomes are:

| Kernel | Attempts | Hits | Misses | Hits by row0/1/2/3 |
| --- | ---: | ---: | ---: | --- |
| DeltaBlue |1,709,700 |1,709,700 |0 |1,453,710 /255,990 /0 /0 |
| Richards |1,253,922 |1,253,909 |13 |381,968 /448,386 /340,261 /83,294 |

The other four hot cases execute no selector attempts. All temporary source
paths were restored and baseline hashes checked before ordinary rebuilding.
Patch, counters, ELF and outcomes remain under E56/pic-selector-diagnostic-f291.
These actual counts supersede site-history applicability estimates. The first
site census divided offsets incorrectly and is explicitly invalid; use
site-counts-corrected-196284 rather than site-counts-6c192.

| Earlier head | Result and disposition | Sealed evidence under E56 |
| --- | --- | --- |
|6a00c6a08d4 | All29/upstream1.161978, six wins; mixed golden screen, not selected |call-ex-* |
|f6ef4a81a87 | Generator/golden.949424 but coroutine1.065409; not selected |recipient-hot6-golden |
|f29106e4557 | Richards/golden.712132 and/upstream.892020; logging/golden1.206807 blocked selection |polymorphic-hot6-{golden,upstream} |

These are distinct source/native campaigns. Their ratios must not be multiplied
or transferred to the current source. Historical8232 and selected54 evidence
remain in the [model55 report](../call-model55/report.md) and
[selected54 report](../frame-state54/report.md). Earlier build/harness failures
and corrections remain in their command manifests and logs; no failed result
was overwritten.

## Experiment decisions and remaining gates

The exact-tuple CALL_EX shortcut removes acquisitions on its exact arm and
passes semantic checks, but has zero executed hot-six sites. No broad speedup
is assigned to that slice. Direct completion entries share the same body but
add 1508 text bytes; they apply to exhaustion/terminal completion, not every
yield or ordinary Python return. Further work there needs actual terminal
counts. The generator-recipient fix instead removes the local/discard direct
handoff rejection; the caller audit confirms return values were already
canonical, with recipient and retirement work still necessary.

The initial multi-use COPY-web slice closes at zero eligible candidates across
21 files/426 CFGs. Of 3,092 fresh borrow roots, existing passes remove 1,798;
204 cross callbacks, 1,068 require nonborrow terminals, 12 cross line/EH and 10
have unsupported COPY provenance. No speculative pass is added. This rejects
that morphology, not broader borrowing; frozen inputs/parser are under
E56/multi-use-borrow-census. A new slice must first demonstrate surviving uses
under a sound anchor/effect/materialization contract.

The current container/coroutine/cache composition has source, product, focused
native and performance evidence. Complete its coherent full qualification,
classify failures against selected54, and select only after reviewing the
regressions. The inline-list followup is rejected after repeated coroutine regression. Preserve
logging gains and state the limitations of noisy small differences.

Full static M4, arbitrary allocation, IRSC2 native qualification and bitmap
monitor/profile/debug repairs remain independent unfinished gates. The bitmap
candidate is unselected and has no current timing. Selected54 debugger-jump,
LLTrace, GC-verifier and profiling limits remain explicit; only GDB pretty
printing is quarantined. Startup RSS and zero-box counts are not throughput
gates. The [plan](plan.md) owns followup work, and the [handoff](../../HANDOFF.md)
owns exact active execution state.
