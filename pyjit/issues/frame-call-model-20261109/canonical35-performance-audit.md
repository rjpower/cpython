# Canonical35 timing audit

The retained evidence is `E11/canonical35-broad-golden/`,
`E11/canonical35-broad-upstream/`, and `E11/canonical35-repeat-golden/`.
The candidate is committed `df12a12b131384976466310b7a6138417b7a36eb`.
Ratios below divide candidate time by the named baseline time, so lower is
faster. I independently calculated each paired ratio from the seven raw
`values_ns` in each command row, divided by its calibrated loop count, then
took the geometric mean of pairs and cases. Every recorded pair and case
geomean matches its summary within `2.3e-16`.

| Campaign | Rows | Cases | Equal-weight case geomean | Selected case geomeans |
| --- | ---: | ---: | ---: | --- |
| Golden, two pairs | 116 | 29 | 0.954489 | generators 1.025333; coroutines 0.988132; nbody 1.002886; unpack_sequence 0.820556; float 1.011232 |
| Upstream, two pairs | 116 | 29 | 1.163732 | generators 1.457779; coroutines 1.312460; nbody 0.866260; logging_silent 0.899568; pickle 0.900400 |
| Golden, four-pair repeat | 24 | 3 | 0.997040 | generators 1.025477; coroutines 0.979644; nbody 0.986605 |

Each broad campaign has exactly one row per round, case and role. Round one
runs baseline then candidate; round two reverses that order. The repeat uses
the same alternating order across four rounds. Loop counts are constant per
case and role, all rows have seven samples and return code zero, and the
recorded affinity is CPU 15 with JIT disabled and hash randomization off.
The complete `identity-before.json` and `identity-after.json` objects match
in each campaign, their error lists are empty, and all three status files
report stable identity without a failure or postflight error. Candidate
metadata matches the committed revision in every row; the golden and upstream
baselines are `0765e9be47f55d97ba5d31d2ae1c02a55ea2e5ae` and
`3a440b65695615e9fd9912342b79cdd54fced0d2`, respectively.

The broad screens establish these workload ratios for this build and capture.
They do not isolate the canonical35 change from earlier interpreter work or
establish a cause for any case difference. The four-pair repeat gives a more
stable check of the three selected golden comparisons, not a full parity gate.
