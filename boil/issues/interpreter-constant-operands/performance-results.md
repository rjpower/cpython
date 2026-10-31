# Constant-operand elapsed comparison

Decision: reject this implementation on elapsed performance. The
[independent outcome review](reviews/quiet-outcome-20261031-v1.md) verified the
actual run and accepts rejection. No runtime change has landed.

The single reviewed campaign started at 2026-10-31 23:08:14.661586 UTC. Root
confirmed terminal exit zero at 23:09:15 UTC. All eight workload/build origin
witnesses and sixteen scored cells completed; both images were idle under
their build locks at cleanup, within the original 45-minute limit. The campaign
retained 80 elapsed samples: five serial calls in each fresh scored process.
These calls are not independent randomized pairs.

The matched GCC O3 images were parent `e9e10d5b503` and candidate `a84965eaa902`.
The latter contains runtime change `7f0b1819f84` plus reviewed test corrections.
This comparison includes no new GC adoption or cadence source. Workloads,
registered loop counts and timing code were unchanged. Execution used OFF/N0,
CPU 0 with sibling 8 reserved, and the predeclared load cap of 1.5.

| Workload | Round 1 median increase | Round 2 median increase |
| --- | ---: | ---: |
| coroutines, primary | 4.47% | 11.64% |
| spectral_norm, secondary | 8.69% | 19.08% |
| generators, control | 23.41% | 21.96% |
| unpack_sequence, control | 50.38% | 42.79% |

Positive percentages mean slower elapsed time. The primary fails the
predeclared requirement of at least 1% improvement in each round. Spectral
timings vary substantially, and no confidence interval is inferred from the
five serial samples. Both control medians also regress in both rounds.
These data provide no basis to accept the increment or add automatic samples.

The evaluator text increase of 18.4248% remains an observation, not the
rejection rule. The run does not establish an instruction-cache, branch or
register-allocation cause. Instruction counters were unavailable after the
restart; the separately reviewed permission amendment omitted them before
scores, without replacement samples or system changes.

The frozen capsule is `build/constant-operands-quiet-v1`. It retains the literal
plan, activation, actual commands, original duration vectors and stdout/stderr,
all origin results, quiet checks, source/product/configuration/tool bindings,
gate return and terminal cleanup. Plan SHA256 is
`1da3b0de4583d1a99b8650e9a3aa4cb32dfcf3dfde9521b439f00aa383bd3ec2`;
activation SHA256 is
`89ab33c0e3129386c397d59c0e490beb89b9e7cb68c82d0f0d02b03881db2aeb`.
The [preparation review](reviews/quiet-preparation-20261031-v1.md) accepts the
elapsed-only amendment and exact helpers. Earlier native/semantic passes remain
valid for their stated scopes; they do not establish a performance win.
