# M1 first-probe hit diagnostic

2026-11-08: Saved the M1/P3 pre-diagnostic binary diff as
`E8/m1-p3-before-hit-diagnostic.patch`. Prepared the independent
`E8/m1-hit-diagnostic.patch`, adding two Py_STATS ObjectStats fields, their
print and merge handling, and increments at provider entry and after the
owned value is acquired. Counts are process-global interval totals, not
per-code attribution; attempts minus hits is the decline count. The patch
adds no Python API and is for stats-only capture, not release timing.

Forward and reverse apply checks passed. The diagnostic was reverse-applied;
the entire remaining working diff matches the saved pre-diagnostic binary
diff byte for byte, and the three modified files' mtimes were restored from
`E8/m1-hit-diagnostic-mtimes.json`. No native build, regeneration, capture or
commit was performed. Independent patch review remains pending.
