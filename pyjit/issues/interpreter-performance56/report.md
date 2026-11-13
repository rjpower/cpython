# Interpreter performance checkpoint

Status: combined design review, before implementation. No stage56 performance
claim and no new golden. The [active plan](plan.md) owns execution; the
[project status](../../STATUS.md) owns aggregate qualification. The canonical
architecture describes selected code; the incoming Astra design is a proposal
until implemented and qualified.

## Baseline

Selected54 remains61015a9452d9069646fb5fede9ff963896833b89. The latest measured
static experiment is 8232dd8adde, built117/79 in release/debug. It passes 23
focused and 233 IR/code cases per role plus5+27 bootstrap/compile probes.
The six cases below each have two balanced pairs; lower elapsed ratios are
better. Campaigns use fixed workloads, matched GCC, CPU15/hash0 and JIT disabled.

| Case | /golden54 | Pair range | /upstream | Pair range |
| --- | ---: | ---: | ---: | ---: |
| DeltaBlue |1.017884 |1.002705–1.033292 |1.177991 |1.152459–1.204088 |
| Generators |0.997913 |0.989817–1.006075 |1.430017 |1.421912–1.438169 |
| Coroutines |1.022717 |1.014778–1.030718 |1.292308 |1.281182–1.303531 |
| Richards |0.978000 |0.976435–0.979568 |1.242869 |1.241195–1.244545 |
| Comprehensions |0.981699 |0.980158–0.983242 |1.332579 |1.322621–1.342613 |
| Silent logging |0.981101 |0.979771–0.982432 |0.756278 |0.745550–0.767161 |

Both campaigns finish with stable source/native identities and no failures.
The result against54 is mixed. These short screens establish neither broad
speedup nor a cause for each gap. There is no8232 all 29/PMU result. Historical
model55ed08 all 29/upstream mean is1.165467 with six wins; matched DeltaBlue
instruction ratio 1.368099 identifies excess work but does not isolate its cost.

Evidence: BASE/pyjit-link-wave1/call-model55/scalar-storage-hot-screen-{golden,upstream}
and scalar-storage-native. The [previous report](../call-model55/report.md)
retains exact commands, ELF hash, historical scope and qualification limits.

## What the new design must establish

- Which conversions and count operations survive existing borrow folding and
  canonical producer proofs, and how frequently they execute.
- Which call/return/resume adapters and publications repeat established facts,
  and what ordinary work a common convention removes.
- How one specialization/effect contract drives lowering, generated handlers,
  fallback and observation without new normal-path metadata decoding.
- Whether frame storage maintenance is material after counting the other work;
  it is not the default explanation for a43% generator gap.

PyValue homes and rebuilt storage are canonical already. Private native boxes
and C-local borrow/owner transfers are distinct boundaries. Exact calls already
overlap arguments and return inline. Existing adaptive caches and borrow
selection must be extended consistently, not proposed as absent features.

## Pending evidence

The bitmap execution-reader candidate now builds both roles, but debug synthetic
HomeRead and monitoring/profile qualification remain open. The IRSC2 retained
transfer component has source reviews/Clinic only. Neither is a qualified
performance result. The active combined plan decides which source pieces to
reuse and which prerequisites each ordinary-path increment actually needs.

Astra high's combined design is in progress. No speedup has been assigned to
any proposed stage56 change. Each selected implementation will add its own
source/native identity, focused checks, removed work, measurements and decision
here; the final checkpoint includes all 29 and matched profiles versus upstream.
