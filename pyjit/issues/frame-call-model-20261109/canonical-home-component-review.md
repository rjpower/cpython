# Canonical local duplication: component result review

The four-pair retained-417 versus clean-31 campaign completed with stable
pre/post identity, CPU 15, seven timed samples per child and no reported
failure. The source HEAD was `3cde040a1afdc433db274ff3771dabff08d7b777`
throughout. Raw commands, child outputs, imported-input manifests, executable
hashes and the before/after snapshots are retained in
`canonical31-component-v2` under the E11 evidence directory.

| Kernel | Paired after/before ratios | Ratio of round medians |
| --- | --- | ---: |
| nbody | 0.9966, 0.9875, 0.9877, 0.9497 | 0.9708 |
| logging_silent | 1.0425, 1.0342, 1.0305, 1.0363 | 1.0346 |
| pickle | 0.9874, 1.0312, 1.0211, 0.9929 | 1.0112 |

All four nbody pairs favor the new build, though three are near 1% and one
is near 5%; their median paired ratio is about 0.988. All four logging
pairs disfavor it by about 3–4%. Pickle changes sign across pairs. This
is a targeted nbody improvement signal alongside a repeatable
`logging_silent` regression, not a general speedup. The GCC result of
134 → 85 static load-handler instructions proves removed classification
work but cannot explain either elapsed-time effect by itself.

Recommendation: **retain the isolated commit for diagnosis, but do not
promote it as a finished performance win**. Compare the affected logging
route directly with the upstream control under the same identity protocol
and inspect a location profile before deciding whether to recover or
revert this optimization. Do not infer a code-layout cause from these
elapsed timings. The broader release diagnostic remains a separate
qualification gate.
