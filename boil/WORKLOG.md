# Worklog

Append concise, high-level actions and outcomes. Link issue results for detail.

- 2026-10-31: GC cleanup and scanning are landed and measured; the matched
  29-workload result is 0.871038× upstream throughput.
  [Results](issues/gc-cleanup-scanning/performance.md).
- 2026-10-31: Documentation is organized around current contracts and issue-owned
  work. Bounded nursery and interpreter-cost experiments are the next tracks.
  [Work index](issues/README.md).
- 2026-10-31: Started parallel minimal young-collection and frame-transfer
  experiments. Existing-profile attribution selected the shared frame-copy
  boundary without another benchmark run. [Current work](issues/README.md).
