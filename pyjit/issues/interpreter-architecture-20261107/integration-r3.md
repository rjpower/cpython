# R3: construct empty varargs from the canonical immortal singleton

Integrated runtime `225480a4250`, parent `25ce1654801`, 2026-11-07.
The overlap path initializes empty Python varargs using the existing typed
`_PyValue_STATIC_IMMORTAL` constructor instead of generic borrowed-object
conversion plus duplication. It preserves the tuple's canonical tag and
no-count ownership. Expanded call tests cover repeated empty-tuple identity
and the generator-owned fallback. `calls-empty-tuple-review.md` and fresh
`cold-r3-review.md` accept the actual implementation without must-fix findings.

Release and debug each check 117 modules and pass 916 focused tests, with one
skip, across the same five files as R1. There are no new distinct release
warning lines. Both executable build identities match R3; stats is still C.

Retained GCC common-overlap code shrinks from 0x881 to 0x86e bytes; fallback
stays 0x917. The common stack reservation grows from 0x58 to 0x68. Those facts
illustrate why smaller source/code alone cannot establish a speedup.

The seven-kernel basic screen versus R2 has raw candidate elapsed geomean
0.944431 and upstream-control change 0.978879 (ratio of those changes 0.964809).
Individual raw factors range from 0.835758 to 1.022093, with substantial
upstream drift in Richards. These sequential screens are noisy; do not
attribute a broad speedup or the larger row changes to this small constructor
edit. The final matched campaign will assess the complete batch.

Evidence: `build/pyjit-link-wave1/interpreter-architecture-20261107/` under
`python-boil-the-ocean`: `r3-checks.json`, release/debug logs,
`r3-identities.json`, `r3-gcc/`, `r3-basic/`.
