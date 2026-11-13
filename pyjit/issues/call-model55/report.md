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
