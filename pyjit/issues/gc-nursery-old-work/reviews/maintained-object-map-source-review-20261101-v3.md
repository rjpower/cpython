# Ordinary page-callback boundary: exact source review

2026-11-01 06:44 UTC. **ACCEPT exact
`76fb129ec28179488fd2ee012a0ae5f599779bc1` for a separately reviewed fresh
focused gate.** This accepts the new callback-boundary hypothesis, not the
rejected v2 implementation or its failed release image. Any further linked
precision edge stops this hypothesis without exemptions or closure expansion.

The clean successor changes two files from
`3a9f70e065bf4112ef3a070746011df7b08892f3`: one marker in `Python/gc_trace.c`
and a results document. Capsule
`build/worktrees/gc-object-map-maintained/build/object-map-maintained-source-v3/capsule.json`
SHA-256 is `8c5c109e7798677f7031cdec8c2ef41b69c4c52d08068e83a4c2e4ebbfd72012`;
patch is `da0d2f819245838dafa312c5e2d83f79c645c2bf4eace32ffaeb369058b6cde6`.
Two file hashes, sixteen artifacts, the exact committed patch and design
review `671ab5297fdad03c1803bd71cd5c98d9669f193f58123325f9638a7c3e3d76c0`
were independently verified.

The runtime file equals its parent byte for byte after replacing exactly
`Py_GC_PRECISE static bool` with `static bool` on `live_walk_page`. Its entire
body, ordinary allocator callback target, classifier/type/weakref/kinds
semantics, other annotations, fault and recovery assertions remain unchanged.
No checker, table, compiler flag, indirection or inlining directive changes.
The accepted design accounts separately for collection below the recorded
entry, object listing, failed clearing and cold address-oracle callers. It
does not assume that every diagnostic runs with `collecting=1` or declare
`tp_is_gc` pure. The metadata change uses the existing opaque visitor
convention; actual fresh linked audit results must establish its feasibility.

Independent host replay passed 22 root methods (21 ordinary passes and one
existing expected failure), three root audits, seven strict guard mutations,
the missing-result-root mutation and all seven unchanged assembled children.
The one named release skip is unchanged. The initial reviewer output remap
mistakenly changed the wrapper's derived worktree and failed before writing
author artifacts; its log is retained. The corrected replay retains actual
source paths and redirects only reviewer outputs. No frozen source or
author artifact changed; no compiler or target ran.

Evidence and receipt are under
`build/maintained-object-map-source-review-20261101-v3`. Source acceptance
grants no activation, native result, elapsed benefit, all29 parity,
qualification or landing credit. Root must review the exact held helper
rebind and independently authorize the one fresh gate.
