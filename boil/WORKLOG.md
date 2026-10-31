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
- 2026-10-31: Rejected bulk frame transfer after bounded comparison found no
  repeatable benefit. Selected shared shape admission next; minimal young
  collection passed source review and moved to feedback.
  [Current work](issues/README.md).
- 2026-10-31: Minimal young collection passed its first behavior checks and
  independent outcome review; bounded compatibility checks continue. Shared
  polymorphic shape admission moved to build feedback. [Work index](issues/README.md).
- 2026-10-31: Accepted the minimal nursery hypothesis after bounded behavior,
  compatibility and negative controls; selected the opt-in candidate for
  qualification. Shape admission passed behavior/emission feedback and moved
  to independent review. [Work index](issues/README.md).
- 2026-10-31: Selected the exact nursery/shape combination for one reviewed,
  finite qualification batch. Started parallel call-setup attribution and
  completed benchmark documentation consolidation. [Work index](issues/README.md).
- 2026-10-31: Combined GCC builds and early nursery/shape controls passed;
  sanitizer and tail builds followed. Closed call-setup attribution without
  selecting an interface rewrite. [Work index](issues/README.md).
