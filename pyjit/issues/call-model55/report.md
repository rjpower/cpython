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
