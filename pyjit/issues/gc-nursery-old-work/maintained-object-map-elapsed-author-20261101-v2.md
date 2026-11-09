# Author outcome: reject the maintained page-map increment

2026-11-01 07:05 UTC. **AUTHOR audit, not independent acceptance.** Source
`76fb129ec28179488fd2ee012a0ae5f599779bc1`, release image SHA256
`1fd0c6fd8529f5501b45fa496cb729b5e2313fe113834246e7dca6a24485e280`,
fails both fixed pilot gates in both rounds. Retire this increment without
retiming, further annotation repair or broad qualification.

| Recomputed metric | Round 0 | Round 1 |
| --- | ---: | ---: |
| Coroutines candidate/parent | 1.080163 | 1.054832 |
| Four-row candidate/parent geometric mean | 1.044114 | 1.041969 |
| Four-row candidate/upstream geometric mean | 1.173180 | 1.197602 |

The fixed requirements were coroutines candidate/parent at most 0.99 and the
four-row geometric mean at most 1.0 in each round. Comprehensions is slower
by 5.22% and 11.08%; Richards by 3.15% and 0.32%; generators by 1.38% and
0.27%. The control losses exceeding 2% have no established mechanism here.
No control compensates for the failed primary gate.

The host replay independently of the recorded score recomputed all 120 raw
durations from 24 scored processes and parsed every GNU-time RSS file. It
replayed twelve actual origin traces, checked all 72 accepted quiet-policy
records, rehashed 253 launch artifacts and 86 plan inputs, verified all three
source/product/ELF closures and confirmed the effective matched compiler
configuration. The observed product sets remain exact. Root confirmed
supervisor 21198 terminal exit 0; terminal cleanup records all three images
idle under their locks before the deadline. No setup or identity fault was
found. This audit starts no target, compiler, probe or new sample.

Peak RSS in KiB, shown as round 0 / round 1:

| Workload | Candidate | Parent | Upstream |
| --- | ---: | ---: | ---: |
| Coroutines | 67596 / 67816 | 66880 / 67196 | 14520 / 14556 |
| Generators | 72120 / 72136 | 73696 / 71932 | 26996 / 27080 |
| Comprehensions | 85612 / 85488 | 86316 / 86288 | 17036 / 17052 |
| Richards | 30824 / 30788 | 30624 / 30836 | 14504 / 14496 |

The source removes repeated allocator block enumeration in favor of page/map
enumeration, but that local work-removal argument did not deliver the required
whole-workload improvement. This run does not measure inventory time saved,
bitmap traversal cost, collection frequency or code-layout effects. It cannot
identify the cause of the losses. RSS is diagnostic, without a size veto.

Five serial values per process are correlated. Some vectors have substantial
spread; these figures are bounded pilot medians, not universal slowdown
estimates or confidence intervals. The consistent primary and aggregate gate
failures provide no acceptance or authority for extra samples. This four-row
pilot establishes no all-29 parity result.

Focused native correctness remains useful separate evidence: debug sixteen
passes, release fifteen passes and one declared debug-only skip. Earlier v1
and v2 audit failures remain failed, with their sources and products intact.
The strict failed-prefix witness and callback-boundary lesson remain available
as retained evidence; they do not justify adopting this optimization.

Author receipt: `build/object-map-elapsed-author-v2/receipt.json`, SHA256
`c1bbbfc58265dde6eac5ed3cf00df5e5afc20aeaa5d9785519c62e03571a5cb8`.
Independent outcome review and the coordinator's final decision remain separate.
