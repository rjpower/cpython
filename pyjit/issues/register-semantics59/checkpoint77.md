# Checkpoint77: the remaining value-rule rows

Integration record, 2026-11-18. Branch `pyjit/checkpoint77`, worktree
`BASE/worktrees/checkpoint77`, builds `BASE/checkpoint77-{release,debug}`.

## Contents

Checkpoint77 is [checkpoint76](checkpoint76.md) (`013fb42c922`) plus
`pyjit/values77` ([record](values77.md)):

- the remaining rows of rules 1–3;
- typed internal entries for dict, list, tuple and set;
- a native call scope that borrows its arguments.

`feb572b0541` also merged the complete value-slot table (`pyjit/slots77`).
Its full suite crashed in `test_code` and `test_monitoring`
(`E59/checkpoint77-{release,debug}-*-full.log`), and `3c8abb64776` reverts
it. Regeneration leaves the products unchanged, and the magic number is
unchanged (64428).

## Qualification

Full suite at `3c8abb64776` (`E59/checkpoint77r-*-full.log`): the release
and debug roles each ran 667 of 678 files and failed only the six known ones
([checkpoint76](checkpoint76.md#qualification)).

## Performance

All-29 two-pair campaigns on CPU 15 with CPUs 7 and 15 reserved
(`E59/execution77-checkpoint-all29-{golden73,upstream}`). The golden73
campaign rejected 2 invocations; all were rerun. The other L3 domain was
running test suites during both campaigns, as it was for checkpoint76.

| Workload | Checkpoint77 / golden73 | Checkpoint76 / golden73 | Checkpoint77 / upstream | Checkpoint76 / upstream |
| --- | ---: | ---: | ---: | ---: |
| richards | 0.936 | 0.985 | 0.819 | 0.865 |
| go | 0.970 | 0.990 | 1.099 | 1.119 |
| nbody | 0.927 | 0.943 | 0.783 | 0.814 |
| float | 0.965 | 0.993 | 0.733 | 0.748 |
| spectral_norm | 1.007 | 0.973 | 0.761 | 0.735 |
| raytrace | 0.979 | 1.010 | 0.951 | 0.959 |
| deltablue | 0.955 | 1.118 | 1.076 | 1.191 |
| chaos | 0.973 | 0.980 | 0.910 | 0.908 |
| nqueens | 0.998 | 1.008 | 1.238 | 1.247 |
| fannkuch | 0.990 | 0.995 | 1.193 | 1.191 |
| generators | 1.007 | 1.001 | 1.267 | 1.167 |
| comprehensions | 1.001 | 0.988 | 1.351 | 1.288 |
| coroutines | 1.077 | 1.063 | 1.254 | 1.234 |
| unpack_sequence | 1.017 | 1.001 | 1.237 | 1.242 |
| deepcopy | 1.009 | 1.029 | 1.369 | 1.389 |
| deepcopy_reduce | 1.007 | 1.018 | 1.356 | 1.377 |
| deepcopy_memo | 1.008 | 1.044 | 1.225 | 1.290 |
| json_dumps | 0.999 | 1.005 | 1.355 | 1.371 |
| json_loads | 0.980 | 0.952 | 1.207 | 1.159 |
| pickle | 0.972 | 0.980 | 1.084 | 1.106 |
| unpickle | 0.948 | 0.959 | 1.175 | 1.173 |
| regex_compile | 1.000 | 1.019 | 1.189 | 1.215 |
| logging_format | 0.996 | 0.973 | 1.156 | 1.262 |
| logging_silent | 0.944 | 1.069 | 0.743 | 0.829 |
| logging_simple | 1.010 | 1.026 | 1.234 | 1.233 |
| xml_etree_generate | 1.022 | 1.015 | 1.333 | 1.317 |
| xml_etree_iterparse | 1.063 | 1.055 | 1.312 | 1.318 |
| xml_etree_parse | 0.990 | 0.992 | 1.138 | 1.108 |
| xml_etree_process | 1.020 | 1.030 | 1.441 | 1.440 |
| **Geomean** | **0.9915** | 1.0067 | **1.1167** | 1.1292 |

Checkpoint77 is the first checkpoint faster than golden73 overall. It beats
golden73 on 16 of 29 workloads and upstream on 7.

values77 changes instructions per loop by less than 0.5% on six kernels, yet
the largest single-workload moves are large:

- DeltaBlue 1.118 → 0.955;
- logging_silent 1.069 → 0.944;
- richards 0.985 → 0.937;
- spectral_norm 0.973 → 1.007;
- json_loads 0.952 → 0.980.

As [checkpoint76](checkpoint76.md#deltablue-placement-not-work) showed,
moves of this size follow op-cache placement. They are not evidence about
the change. The geomean is the better guide, and it too carries placement.
