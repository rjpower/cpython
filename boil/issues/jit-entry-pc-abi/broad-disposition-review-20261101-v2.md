# Complete all-29 result: reject progression

REJECT source599897 for publication/lifetime and full-stdlib qualification.
The completed campaign fails the predefined GM29<1 gate in both rounds. No
additional statistical veto or retrospective threshold is needed.

| Fixed result | Round0 | Round1 |
|---|---:|---:|
| GM29 candidate/RC | 1.022646 | 1.019360 |
| Spectral candidate/RC | 0.872678 | 0.861373 |
| GM29 after any single-value deletion, range | 1.021960–1.023151 | 1.018200–1.019950 |
| GM28 excluding spectral, diagnostic only | 1.028454 | 1.025510 |
| Improved / regressed workloads | 8 /21 | 7 /22 |

The aggregate is 2.26%/1.94% slower even with the retained 12.73%/13.86%
spectral benefit. Every predefined single-value aggregate sensitivity remains
above1. Spectral and its own ten deletions pass. GM28 remains a concentration
diagnostic, not a new rejection gate. These serial observations and sensitivity
checks are not confidence intervals.

Thirteen workloads exceed2% loss in both rounds:

| Workload | Round0 slower | Round1 slower |
|---|---:|---:|
| richards | 4.65% | 4.72% |
| go | 4.54% | 6.20% |
| raytrace | 5.57% | 4.78% |
| deltablue | 7.19% | 6.89% |
| nqueens | 3.71% | 3.75% |
| generators | 12.20% | 11.87% |
| comprehensions | 4.65% | 4.00% |
| deepcopy | 3.62% | 3.23% |
| deepcopy_memo | 8.07% | 7.09% |
| json_dumps | 5.28% | 2.08% |
| logging_format | 4.17% | 5.07% |
| logging_silent | 14.98% | 14.42% |
| logging_simple | 2.26% | 3.29% |

Each is disposed as an unacceptable regression for this candidate under an
already-failing generalization gate. No loss is averaged away or attributed to
noise. Other one-round losses and per-workload RSS ratios remain in the bound
score/disposition data. Resource observations do not overturn the elapsed rule
or add an RSS-only veto. No causal claim follows from the workload names or
static call counts.

The pilot's limited claim survives: spectral improved12.65%/10.13% and the
selected GM4 improved2.78%/1.86%, with Richards already slower5.15%/3.49%.
The new campaign repeats spectral benefit and Richards loss but does not show
registry-wide improvement. Coroutines and unpack are near parity here; their
pilot values are retained separately, not pooled or replaced. The pilot-only
decision never promised full-registry benefit or unconditional adoption.

Recommend retaining qualified RC as default and preserving this JIT source,
native mechanism evidence and every score as a rejected candidate. Do not pay
for costly publication/full-Lib qualification of599897, append samples, narrow
the workload set or revise the region to rescue this score. Any further source
hypothesis would require separate authorization and evidence. Offline IP
register recovery and unchanged leaf bytes remain valid static findings, not
proof of a net performance win.

This review independently recomputes geometric means from retained per-workload
score ratios and compares the pilot separately. The other independent reviewer
owns full raw-vector/provenance/cleanup acceptance. Root reported session46516
terminal0 and cleanup13:50:50 with all16 locks/native idle and bindings intact.
Launch-result SHA90ba9b9f is pinned. No target, compiler, counter or additional
sample was run in this disposition review.
