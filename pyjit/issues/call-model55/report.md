# Model55 integration results

This is an active report, not a final selection. Selected54 remains golden.
The [implementation ledger](implementation.md) owns stage state; the
[design](../../design/proposals/register-call-model55.md) owns the target.

## M1a: independent typed metadata owners

Candidate `dd3c9fc8d9e` replaces code/function PyValues in frame headers with
independent typed strong pins. It preserves argument overlap, direct returns,
cached environments/feedback, initialized homes and the 112-byte GIL header.
Source post-review is [GO](pins-post-source.md). Root's release/debug builds
each check 117 modules with no failed imports. Non-running GDB confirms header
112 and function offset 32 in both roles and both candidate/control.

The 13-module focused set passes 932 filtered tests per role: release has 15
skips, debug 13. Initial release failed one newly added escaped-frame test that
incorrectly expected function metadata release at frame.clear(); the same probe
fails selected54. The corrected test checks retained pins while the escaped
frame exists and release after frame destruction. A cold API correction
preserves None for interpreter-sentinel code, with a regression in the typed
pin helper. No full-suite pass or free-threaded qualification is claimed.

Two-pair hot screen, candidate/selected54 elapsed, lower is better:

| Workload | Geometric mean | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.992066 | 0.973661–1.010818 |
| Generators | 1.084665 | 0.987662–1.191196 |
| Coroutines | 1.045383 | 1.041113–1.049670 |
| Richards | 0.962854 | 0.943837–0.982254 |
| Comprehensions | 0.965199 | 0.962391–0.968015 |
| Logging silent | 1.183453 | 1.094471–1.279671 |

Campaign `E/m1-pins-hot-screen-golden` completes with stable clean source/native
identities and no pre/postflight failure. Six-pair confirmation
`E/m1-pins-hot-confirm-golden` also completes with stable clean identities:

| Workload | Geometric mean | Pair range | Interpretation |
| --- | ---: | ---: | --- |
| DeltaBlue | 1.020651 | 1.006815–1.040986 | All six slower |
| Generators | 0.970041 | 0.958438–0.980784 | All six faster |
| Coroutines | 0.992533 | 0.780832–1.050945 | Five slower; favorable outlier changes aggregate |
| Richards | 0.983934 | 0.970992–1.016851 | Five faster |
| Comprehensions | 0.981942 | 0.944161–1.072574 | Five faster; wide range |
| Logging silent | 1.167004 | 1.062202–1.257403 | All six slower |

Keep scopes separate. The screen does not earn golden admission; neither wider
qualification nor the complete ownership architecture has been established.

Independent two-pair instruction diagnostics use the unchanged benchmark
runner's whole-process loop difference, CPU15, hash0 and JIT off. These are
distinct from the final sealed-interval PMU protocol. Raw perf journals and
commands are under E as `m1-pins-instr-*`. They show fewer instructions for
DeltaBlue, generators, coroutines and silent logging. Candidate/control ratios
are 0.988961, 0.980078, 0.982227 and 0.998503 respectively. Fewer instructions
do not establish elapsed gains. The exact M1a release ELF is retained as
`E/m1-pins-native-python` with build ID
`ec33dff266d72576b46b3fbd7b13cc21a5e199af` before subsequent relinking.

Native inspection shows simpler code-pointer reads and removed tag decoding,
but the callable-to-typed-owner boundary adds count-ownership classification
and code acquisition uses ordinary object refcount checks. The shown RETURN
handler retains its existing instruction structure and stack reservation.
This is concrete work evidence, not a causal partition of elapsed changes.

The useful learning is to carry typed ownership across the call boundary,
not only retype header fields. M1a is preparation for the full model and remains
an experimental candidate. Subsequent gates must price its costs as part of
the composition; do not promote it solely for smaller source or cleaner types.

## M1b: common encoded-image admission

Candidate `ce71fcfc240` composes typed pins with a single `_PyCode_New`
admission boundary. Compiler, public construction/replacement and marshal now
apply the same operand/window/index/branch/feedback validation and inline
constant normalization before allocation. Allocation uses a private constructor
copy. This is not yet a full initialized-value or unique-owner proof.

Both roles build and pass the nine-module focus: 413 filtered tests, nine skips
per role. Eight new admission tests cover constructor, replace and malformed
marshal payloads without executing invalid images. Initial failures exposed
fixture issues: missing specialized handler lookup and synthetic operands whose
invalid domains prevented the intended semantic proof. Corrected fixtures keep
valid surrounding domains and assert specific malformed-domain errors; no
validator, test or functionality was disabled. Existing valid compiled images,
marshal, completion and operand-mode checks pass. The completed two-pair hot
screen versus selected54 is DeltaBlue 1.003606, generators 1.007832, coroutines
1.079173, Richards 0.966024, comprehensions 1.012161 and silent logging 1.151141.
Campaign `E/m1-admission-hot-screen-golden` has stable clean identities; logging
range is 1.080176–1.226767. These samples do not earn admission. The exact release
ELF is retained as `E/m1-admission-native-python`. Full qualification remains open.

## M2a: explicit retiring owners and observer prefixes

Candidate `2f7067cdde1` adds a nested transition-root chain for reached,
potentially reentrant retirement. The ordinary path first performs only proven
callback-free closes. Outlined continuations root the remaining initialized
homes, typed headers and independent result through the final code/exception
close. A detached observer-redirection array uses the same chain with an
explicit prefix cursor. This is still NULL-authoritative: arena scans and
scratch clearing remain. The [cold source review](m2-integrated-cold-review.md)
is GO for this bounded bridge, not for static maps.

Both builds pass nine precise-root cases, using a visitor that does not rely on
conservative scanning, and the 13-module frame/call/GC focus passes 940 filtered
tests per role (15 release skips, 13 debug). Two new fixtures initially selected
the wrong construction/retirement phase; closure cells and traceback escape
were corrected without changing runtime code or weakening root assertions.
The initial overly broad test selection also reran the same nine known debugger
errors; the runner's ordered positive filters now select the intended cases.

Two-pair `E/m2-roots-hot-screen-golden` versus selected54:

| Workload | Candidate/control elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 1.010977 | 0.990699–1.031670 |
| Generators | 0.985657 | 0.985638–0.985677 |
| Coroutines | 1.040258 | 1.039816–1.040699 |
| Richards | 0.993525 | 0.991457–0.995597 |
| Comprehensions | 1.014623 | 1.009436–1.019837 |
| Logging silent | 1.087839 | 1.079558–1.096184 |

Pre/postflight clean source/native identities match. This does not earn
admission or establish an isolated causal cost. The exact release ELF is
retained as `E/m2-roots-native-python`. Non-running GDB shows homes offset 112,
function offset 32, transition record size 40; struct size 120 includes its first
eight-byte value cell. Native handler/cleanup dumps are retained under
`E/m2-native-*`. No full-suite or FT qualification is claimed. Semantic transfer
admission and actual staged-arena adapters are the next integrations.

## M2b: generated normal-transfer admission

Candidate `b75e9cc3201` adds a generated, source-checked transfer column and
a construction-time worklist over the full admitted scratch extent. Definite
initialization/NULL intersect at joins; possible owner obligations union.
Checks run after convergence. Unknown families deliberately lose authority;
this is partial admission hardening, not a persistent map or permission to read
stale homes. The [independent peer](ir-m2-peer.md) caught and closed the initial
owner-intersection error before integration.

Both roles pass 425 filtered IR/code/marshal/completion tests (nine skips),
plus four targeted generated-contract tests. Initial checks exposed source
fixture formatting and old RETURN-helper assertions, now corrected. Four
synthetic observer witnesses read unavailable scratch; three now assert
construction rejection and also execute valid observer counterparts. The
distinct-local restoration witness instead preserves its valid source and
changes only the destination local. No admission bypass or new quarantine was
added. Source peers reviewed these corrections. The initial full generator
run and all focused reruns are retained separately; no green full-suite claim
is made. The completed two-pair screen `E/m2-transfer-hot-screen-golden`
keeps stable clean identities and shows the following selected54 ratios:

| Workload | Candidate/control elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.988641 | 0.978920–0.998458 |
| Generators | 0.985761 | 0.979232–0.992333 |
| Coroutines | 1.050681 | 1.047777–1.053592 |
| Richards | 0.985502 | 0.981323–0.989699 |
| Comprehensions | 1.004111 | 1.000617–1.007618 |
| Logging silent | 1.258762 | 1.249965–1.267620 |

This does not earn admission. Do not attribute the changed logging ratio to
the cold validator from timing alone; profiles/native work are needed after
the direct typed-owner boundary is integrated. Retained exact release ELF:
`E/m2-transfer-native-python`.

## M3a: staged unpack and typed function-owner transfer

Candidate `01586ea4d0b` gives generic unpack an explicit initialized output
suffix through iterator and source cleanup. Both actual callers now transfer
the source owner into that helper. The frame initializer consumes a canonical
owned function directly, removing the generic borrowed/owned conversion arm.
The complete producer audit is in the [helper contract](helper-roots-proposal.md).
The [cold source/native review](m3-integrated-cold-review.md) is GO for this
bounded integration. Exported native workspaces retain their counted-owner
policy; the dead FT-only, unproduced C-value chain is removed, without a claim
of FT qualification.

Six new prefix/count cases pass per role, then the 15-module frame/call/GC/unpack
focus passes 961 filtered tests per role (15 release skips, 13 debug).
Native exact CALL loses the generic ownership-classification/acquisition arm
and keeps its existing 64-byte stack reservation; RETURN retains 16 bytes.
The reached generic unpack helper reserves 120 bytes plus saved registers,
including its transition/prefix records. No scratch clearing or arena scanning
is removed. A complete binder/provider lifecycle is still open.

Two-pair `E/m3-unpack-hot-screen-golden` versus selected54 has stable clean
identities:

| Workload | Candidate/control elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.994313 | 0.982491–1.006277 |
| Generators | 1.001126 | 0.999477–1.002779 |
| Coroutines | 1.051245 | 1.044862–1.057667 |
| Richards | 0.979027 | 0.974508–0.983566 |
| Comprehensions | 1.024117 | 1.023364–1.024870 |
| Logging silent | 1.238056 | 1.199267–1.278100 |

It does not earn admission. The direct transfer alone does not repair the
composition's persistent coroutine/logging losses. All29 versus upstream and
matched interval profiling are next, before attributing those losses. The exact
release ELF and handler/helper dumps are retained as `E/m3-unpack-native-*`
and `E/m3-native-*`. Full qualification and complete map authority remain open.

### Frozen M3 diagnosis versus upstream and selected54

Completed `E/m3-unpack-all29-upstream`: two balanced pairs, stable clean
identities, unchanged workloads. Overall elapsed geometric mean is 1.171628;
6/29 cases are faster. This is a direct upstream comparison of the M3
preparation candidate, not a selected checkpoint or full qualification.

| Workload | Candidate/upstream elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 1.190287 | 1.184318–1.196287 |
| Generators | 1.431423 | 1.411937–1.451178 |
| Coroutines | 1.324636 | 1.316873–1.332444 |
| Richards | 1.218718 | 1.174511–1.264588 |
| Comprehensions | 1.376634 | 1.372985–1.380293 |
| Logging silent | 0.928949 | 0.922065–0.935885 |

Sealed FIFO intervals count only hot execution after warmup, with CPU15, hash0
and JIT off. `E/m3-perf-cycles-instructions` completes 24/24 valid invocations,
three balanced pairs, stable identities and 100% counter running. Normalized
per-loop candidate/upstream ratios:

| Workload | Instructions | Cycles |
| --- | ---: | ---: |
| DeltaBlue | 1.366246 | 1.164945 |
| Generators | 1.396111 | 1.449748 |
| Coroutines | 1.390894 | 1.315266 |
| Logging silent | 0.991900 | 0.911121 |

`E/m3-perf-record` completes 8/8 valid five-second records, with matching build
IDs, stable identities and no lost records. Flat samples locate different
remaining costs: DeltaBlue spends 11.0% in exact CALL and substantial samples
in guarded attribute handlers; generators spend 26.4% in SEND and 13.7% in
YIELD; coroutines spread across exact CALL (10.4%), generator construction,
completion and cleanup. Call setup alone cannot explain the generator gap.

Fresh selected54 intervals, `E/baseline54-perf-cycles-instructions` and
`E/baseline54-perf-record`, complete 12/12 stat and 4/4 record invocations,
respectively. Their separate upstream-normalized logging instruction/cycle
ratios are 0.953890/0.723324; coroutine ratios are 1.386260/1.254979.
Keep these separate campaigns distinct from direct timing pairs. Logging's
extra elapsed work is larger than its instruction increase; coroutine
instructions change little. Do not infer a pure store-count explanation.

The strongest incremental lead is retirement: M3 logging samples place 16.8%
in `_PyFrame_TryClearExceptCode` and 6.4% in `clear_thread_frame`, versus
selected54's 7.7% in `_PyFrame_ClearExceptCode` and 2.8% in its clear/pop
boundary. M3's classifier accounts for a substantial subset. Source and
native inspection show the new fast sweep classifies an owner to prove no
reentry, then classifies it again in XCLOSE. The proposed correction is a
single value-layer operation that consumes an owner only if it can close
without reentry; failure leaves it for the outlined rooted route. Use existing
RC/debug bookkeeping, with counted FT owners still outlined. This is a general
ownership operation, not a workload special case. Peer contract review is GO;
implementation and matched tests/timing remain required. Sample shares are
diagnostic evidence, not promised removable cycle budgets.

### Shadow certificate and reader composition

Integrated `a95bcbbf836` adds persistent normal-return certificates, explicit
retirement reasons, comparison status recorded at the actual cold producer,
staged binder records and the shared heap-frame clear loop. It retains NULL
authority and conservative arena scanning. The code execution header grows
from 32 to 40 bytes; the hot frame header does not grow. The bounded certificate
recognizes proved empty scratch at normal return, not general callback phases.

Thirteen selected retirement/binder tests and six generated-contract/heap-clear
tests pass in each role. The wider 25-module focus runs 1,373 tests per role:
24 modules pass, while `test_external_inspection` has one failure in each role.
Its deliberately corrupted source map now reaches lazy execution-image proof
before the inspector. Diagnosis and a correction preserving the malformed-map
reader check are pending; this composition is not a passing integration yet.
The focus groups overlap; do not add them into a deduplicated total.

Provider `be36adec43a` and classify-once retirement `31f2e3a2e22` have independent
source review but no native integration evidence yet. The provider keeps the
staged child visible through custom evaluator hooks and cleanup, including
nullable headers installed by the hook. It disarms before reservation reuse.
Constructor shim ownership and complete static/bitmap authority remain open.

The two-pair `E/m3-shadow-hot-screen-golden` completes with stable identities.
Candidate/selected54 ratios are DeltaBlue 1.018286, generators 0.913875,
coroutines 1.055014, Richards 1.047250, comprehensions 1.049970 and logging
silent 1.407701. Generator pairs span 0.869566–0.960441; this is a screen, not
confirmation. The preparatory composition does not earn admission.

### Classify-once retirement

Integrated `d5c1411fa0c` replaces proof-then-XCLOSE in the first retirement sweep
with `_PyValue_TryXCloseNoReentry`. True consumes the owner without callbacks;
false leaves it untouched for rooted retirement. It retains existing refcount,
debug and statistics behavior. Last owners remain on the slow path. Both-role
builds and 815 focused tests per role pass (11 modules; 12 release skips,
nine debug). Native inspection confirms one tag classification in the sweep,
with no additional record or stack allocation. Homes remain at frame offset 112;
the transition record and code execution header are each 40 bytes.

The unchanged two-pair screen `E/m3-fast-close-hot-screen-golden` completes
with stable clean identities:

| Workload | Candidate/selected54 elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.938676 | 0.930066–0.947367 |
| Generators | 0.980067 | 0.949498–1.011621 |
| Coroutines | 1.011305 | 0.999256–1.023499 |
| Richards | 0.967029 | 0.965312–0.968748 |
| Comprehensions | 1.021239 | 1.013067–1.029477 |
| Logging silent | 1.047776 | 1.042410–1.053169 |

This supports the retirement regression hypothesis but does not establish a
confirmed general win. Keep the shadow and classify-once campaigns separate;
do not divide their ratios into an isolated causal speedup. The retained ELF
is `E/m3-fast-close-native-python`. Provider integration `362fb793a4d` follows;
its native tests and timing remain pending.

Remote-inspection fixture correction `6acbdf3e432` prepares the valid executing
image before all three source-map corruptions; it keeps every malformed-map
reader assertion. Cold proof currently uses IRSM semantic handler entries to
seed observer-failure states, never positions. That is real coupling to a
container also holding positions, though no dispatch or retirement-query
decode is added. The shared phase work must move those recovery targets into
semantic metadata rather than making optional source positions authoritative.

### Provider composition and refreshed diagnosis

`de95475accb` includes both native helper corrections: a balanced owned argument
for the value provider and internal visibility for the actual object provider.
The first compile failure and the later make-success/import-failure logs remain
retained. The check runner now rejects an incomplete module roster even when
make exits successfully. Final release/debug builds each have 117 modules,
79 shared and zero import failures.

Eight precise provider/remote-reader tests pass per role. Core coverage passes
1,487 tests per role; release's command also included a mistyped module name,
which is a recorded command failure rather than a runtime failure. The actual
`test_capi.test_borrow_scope` and EvalCodeEx group runs separately: 28 tests per
role pass. Debug's core command names only the 27 real modules and passes.
These focused groups overlap. Full foundation diagnostics are recorded below.

Independent [cold source/native review](m3-shadow-integrated-cold-review.md) is
GO for this bounded composition. External object/value providers reserve
200/168 stack bytes plus 48 saved-register bytes; each includes a 40-byte stage
and 64-byte argument array. These are total costs, not an incremental delta.
Exact CALL/RETURN still reserve 64/16 bytes and contain no token/map search.

`E/m3-provider-hot-screen-golden` completes two balanced pairs with stable
clean identities:

| Workload | Candidate/selected54 elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.940065 | 0.930389–0.949843 |
| Generators | 0.994338 | 0.990035–0.998660 |
| Coroutines | 1.014167 | 1.010954–1.017391 |
| Richards | 0.970037 | 0.967672–0.972407 |
| Comprehensions | 0.992827 | 0.991367–0.994289 |
| Logging silent | 1.083916 | 1.036127–1.133909 |

The direct unchanged all29 campaign `E/m3-provider-all29-upstream` completes
with stable clean identities, geometric mean 1.162061 and 6/29 faster:

| Workload | Candidate/upstream elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 1.122475 | 1.100471–1.144919 |
| Generators | 1.436127 | 1.435602–1.436652 |
| Coroutines | 1.281462 | 1.276078–1.286869 |
| Richards | 1.216978 | 1.190603–1.243937 |
| Comprehensions | 1.351905 | 1.342274–1.361605 |
| Logging silent | 0.793543 | 0.787277–0.799858 |

Matched sealed PMU intervals in `E/m3-provider-perf-cycles-instructions`
complete 12/12 valid invocations, three balanced pairs, 100% counter running
and stable identities. Per-loop candidate/upstream instruction/cycle ratios
are 1.340815/1.125337 for DeltaBlue and 0.948870/0.799318 for silent logging.
The normalized values and pair ranges are retained in `normalized-summary.json`.
`E/m3-provider-perf-record` completes 4/4 valid five-second records with stable
identities and no lost records. Flat reports show logging retirement at
10.17% in TryClear plus 3.36% in clear_thread_frame, down from the earlier
M3 preparation's 16.78%+6.40%. Distinct campaigns support the diagnosis without
establishing an isolated speedup from dividing their ratios.

DeltaBlue still spends 11.03% of flat samples in exact CALL, 8.90% in the local
shape attribute path, 6.60% in RETURN, and 5.26% in TryClear. The remaining gap
spans call/return, ownership and guarded object access. Generators/coroutines
retain substantial gaps and need their full lifecycle representation, not
another claim that external call setup explains everything. This is a passing
bounded integration and useful diagnostic checkpoint, not a promoted golden.

### Constructor entry and fallback ownership

`9136c202e8e` integrates the reviewed constructor component and its generated
handler. Exact positional init uses the existing typed initializer and positional
mover with a separate child reservation. Generic binding moves evaluated
scratch owners into native workspace and NULLs caller homes, with nested
shim/child authority and an explicit staged-caller redirect barrier. Named
locals remain mutable. No extra per-argument DUP/close pair preserves the old
temporary window. A current function/code/count/capacity check selects entry;
custom vectorcall providers deopt before instance allocation.

Both builds retain 117/79 modules and zero import failures. Three precise
constructor fixtures and 942 core tests pass in each role. Generated failure
branches precede `IR_CLEAR_SLOTS`; preparation failure therefore leaves the
original inputs for real unwind. Successful staged-pop to child publication
contains no callback. Root retained the ELF and six native dumps under
`E/m3-init-native-*`. Full qualification remains pending.

`E/m3-init-hot-screen-golden` completes two balanced pairs, stable identities:

| Workload | Candidate/selected54 elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.955028 | 0.931411–0.979245 |
| Generators | 1.023736 | 1.013518–1.034057 |
| Coroutines | 1.017553 | 1.013565–1.021556 |
| Richards | 0.975054 | 0.971210–0.978912 |
| Comprehensions | 0.969607 | 0.969194–0.970019 |
| Logging silent | 1.119406 | 1.046472–1.197423 |

No promotion follows from this screen. Logging remains variable and the
preparatory composition still loses generators/coroutines. This closes an
ownership gap and gives exact constructors a coherent fast entry; it does
not establish an isolated throughput improvement over the provider checkpoint.

### Heap teardown and full foundation diagnostics

`0851fd51801` adds the reviewed owned-heap-frame teardown. Its full-home/pin
root scope starts before weakref callbacks; the reverse retirement cursor
shrinks the actual remaining owner set. Five observer metadata owners move
callback-free into native workspace, and code remains pinned until the end.
Nonowned frame deallocation never dereferences an expired external frame.
The precise nested/non-nested deallocation fixture and 420 core tests pass per
role. Both builds retain 117/79 modules and zero failed imports. The actual
ELF and outlined/hot native dumps are retained as `E/m3-heap-dealloc-native-*`.

`E/m3-heap-dealloc-hot-screen-golden` completes two balanced pairs with stable
clean identities:

| Workload | Candidate/selected54 elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.965879 | 0.947184–0.984944 |
| Generators | 0.982174 | 0.981860–0.982489 |
| Coroutines | 1.035898 | 1.035215–1.036582 |
| Richards | 0.963935 | 0.962555–0.965317 |
| Comprehensions | 0.970366 | 0.957506–0.983400 |
| Logging silent | 1.122722 | 1.048128–1.202623 |

These are foundation-composition comparisons, not isolated heap-port gains.
No promotion follows. Full diagnostics run 54,763 tests per role, with 635/646
files run, 74 skipped, 11 denied resources and the existing GDB class empty.
Release retains GC-verifier instability and nine debugger-jump errors; debug
retains LLTrace and nine debugger errors. Debug's profiling and GC-verifier
modules pass this run. Individual debugger identities are not yet rechecked
verbosely at55. Both builds newly fail the unchanged dictionary iterator OOM
subprocess: release SIGSEGV, debug assertion/SIGABRT. The suites are failures,
not qualified passes. Logs and commands are `E/m3-null-foundation-*`.

### Specialization readiness failure

The crash reaches `_PyFrame_InitIRInstrPtr` from specialized exact CALL with
NULL code exec. `function_kind` returned zero both for SIMPLE_FUNCTION and
for SPEC_FAIL_OTHER after clearing failed `_PyCode_GetExec`. The cold plan's
extra allocation points expose this ambiguous interface. Source is unchanged
from selected54 at that helper; selected54 passes the original OOM test in
both roles, whereas the foundation fails both focused repeats. That establishes
the new candidate failure, not a claim that the baseline fault reproduces.

`7c42db43224` integrates the [reviewed fix](exec-preparation-oom-proposal.md):
pure typed signature classification, plus positive boolean execution readiness
before every ready-dependent cache nomination. Diagnostic-only shape checking
does not allocate. General and keyword call nomination also require readiness.
No guard is added to warmed CALL/property/getitem execution. The original OOM
fixture is unchanged; it passes both roles after the fix. Broader dict, call,
specialization, class, property, descriptor, frame, C eval and admission checks
pass 837 tests per role (11 release skips, nine debug). These focused checks do
not replace the failed full diagnostics or prove every allocator fault path.
Commands/logs are `E/m3-oom-{before-*,selected54,fixed,fix-core}*`.

The two-pair unchanged screen `E/m3-oom-fix-hot-screen-golden` has stable clean
identities:

| Workload | Candidate/selected54 elapsed | Pair range |
| --- | ---: | ---: |
| DeltaBlue | 0.932542 | 0.924295–0.940864 |
| Generators | 0.983220 | 0.978364–0.988100 |
| Coroutines | 1.024588 | 1.024037–1.025139 |
| Richards | 0.984563 | 0.979096–0.990061 |
| Comprehensions | 0.974889 | 0.969928–0.979876 |
| Logging silent | 1.132377 | 1.062399–1.206963 |

This remains a mixed foundation composition, not a nominated golden or a
causal estimate of the cold readiness fix's throughput effect.

### Shared-emitter phase integration

`6ff13875cfe` composes the IR phase component, GC reached-walk comparison,
cold mismatch probe, accepted-redirect resets and regenerated cases/metadata.
The GIL phase word fits the existing padding: header remains 112 bytes.
One immutable code-owned semantic plan contains normal-return certificates
and interned site/phase scratch ranges. The generic binary provider emits its
actual owner TAKE, provider, result-retirement and error-restoration events;
admission and plan construction consume that same graph. Nullable shadow
candidate ranges are not stale-word readability certificates. Unsupported
families remain uncovered; NULL homes and arena scanning are still authority.
Merged builds/runtime/native checks are pending. This is partial coverage,
not the M4 representation switch.
