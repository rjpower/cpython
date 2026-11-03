# H3 union source checkpoint

Coordinator review, 2026-10-29 16:43 PDT. Verdict: source composition clear
for isolated native build preparation at e9a1bcc65cf50c9316fc375500bcdaf4fb3a117b.
No primary landing, runtime qualification or speedup follows.

This combines frozen GC candidate f2a5b3ca33d with reviewed H3/stats parent
5d108561e7a. Component reviews remain authoritative; this review checks their
composition. The six shared paths preserve ROOTS precision/spans and its
two-argument iterable helper, bytes-writer root/CAPI fixtures, H3 monitoring
eligibility/storage/domain behavior and actual-target statistics. The actual
H3 delta in bytecodes.c, ceval.h, ceval_macros.h and the additive CAPI code was
read against f2a5; the generator/test changes and as-built were inspected.
Primary collector/writer code and global gate admission remain intact.

All seven generated products were regenerated serially from the combined
inputs, matched the automatic merge and were idempotent. Six match the stats
parent; generated IR correctly retains ROOTS's helper/temporary changes.
No renumbering or hand-edited generated resolution is introduced. The host
90-case run retains two stale expected-output failures and one existing
expected failure. Only the two NOFB counter expectations changed; both passed
once afterward. This is distinct receipt coverage, not a fictitious new
90-case green run.

The earlier component native results do not qualify this union. Build one
matching owned Py_STATS cohort, then verify real producer/reader facts and
the actual frame/mode/root contracts before any stats claim. Nonstats release/
debug and broader integration checks remain. Existing private counter drivers
must bind the new products before reuse; their old d462 source guard is not
weakened. GCC evidence supplies no actual tailcall qualification.
