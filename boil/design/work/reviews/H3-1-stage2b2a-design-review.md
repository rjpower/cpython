# H3-1 stage2b.2a design review

Independent read-only ROOTS review, 2026-10-29, of monitoring draft SHA256
`151a774b236f251d22fa5a8d312698ff555a9f5f3db595e96d7733e97b27e5a2`
against frozen native `c5cf6ba6d288`. Verdict: IMPLEMENT_2A, after the
wording correction and exact shared-hook grant below. No tests or builds.

Removing the mutable monitoring-cache dependency from SafeAddr2Line is a
bounded change. The immutable fallback allocates nothing. Detached lifetime
has a concrete root chain: initialized frame f_executable, domain thread/data
stack traversal without an attached-state filter, frame traversal and code
co_linetable traversal. The pending3c C-stack attribution issue does not remove
that precise chain. InterpreterGuard prevents finalization; it does not pin
code. Emergency borrowed/dead-frame heuristics remain a separate limitation.

Require ordinary instruction heads before/during/after monitors, negative
offsets and out-of-range rejection. Correct the draft's old-behavior claim:
the old monitored function consults its cache before bounds checking, so
old out-of-range behavior cannot be described as uniformly returning -1.

The deliberate synthetic-offset behavior is acceptable: immutable locations
cover the relaxed instruction's extra words, whereas event cache deltas can
be NO_LINE there. Real FOR_ITER/SEND relaxed words and a copied old helper
must distinguish the behavior. IR_JUMP does not save the ordinary frame IP;
do not claim every word's cached and immutable locations are equal.

Shared-file scope is only one code-plus-int safe-lookup helper and method
entry, using the existing pycore_code include. Exact hunk approval is still
required before editing _testinternalcapi.c. Monitoring-storage helpers,
instruments includes and all2b.2b implementation require separate review.
