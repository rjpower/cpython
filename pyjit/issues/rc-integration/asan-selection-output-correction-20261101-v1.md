# ASan selection reader correction and retained result adjudication

2026-11-01. The actual ASan target returned0 and its standard runner reported
SUCCESS: 3,827 tests, zero failures and77 skips. The enclosing batch returned1
because the profile helper incorrectly compared sixteen requested module
names directly with26 scheduled module names. I introduced that assumption
in the helper and missed the standard package-expansion boundary during
preparation. The original helper, raw evidence, failed result and batch
failure remain unchanged. No target was rerun.

The actual source explains the difference. `Lib/test/libregrtest/main.py`
applies `selected = split_test_packages(selected)`. The pinned
`findtests.py` SPLITTESTDIRS set expands the three requested multiprocessing
packages into4,5 and4 submodules, replacing three names with thirteen.
`test_capi` stays whole. The other thirteen requests are unchanged, yielding26.
The correction does not infer or whitelist26 names from the result log.

Additive reader files are in
`build/rc-asan-selection-reader-correction-20261101-v1`. `selection_expansion.py`
checks the original plan's source hashes for both standard files, extracts
their unchanged SPLITTESTDIRS and three discovery functions, verifies the
actual main call shape, and executes only host filesystem discovery. It
imports no target test modules and checks each discovered source origin
against the frozen input hashes. The actual source functions determine the
expected module multiset before it is compared with retained selection.

`corrected_contract.py` differs from the frozen contract by one import and
two expressions: the requested selection and protected requests use this
same source-derived expansion. Reversing those exact changes restores every
other original byte. Records, plan and SPEC are passed unchanged. The current
three protected requests—test_capi, test_gc and test_generators—do not split.
All original module state/statistics, PID/configuration/scheduling/return,
ABI/own-CAPI, ASan options/mapping, source/origin/traceback, and protected-test
execution predicates remain in force.

Five host controls passed. They accept the legitimate expanded set and
reject missing, injected, duplicate and unexpanded selections; wrong source
hashes and wrong split policy; and damaged module state, PID, ABI, origin,
ASan policy and end-record evidence. They also reproduce the original
contract's false failure and prove the exact inverse source projection.

The separate host audit rechecks all7,187 frozen inputs and ready artifacts,
the target argv and native0/outer1 return codes, raw logs and state hashes,
the exact original assertion, source HEAD/cleanliness, all584 original
products and allowed regular cache additions, retained compiled profile
witnesses, scoped pidfd cleanup receipt and current dual-executable idle
state. The corrected reader closes26 scheduled/returned modules and27
ordinary driver/worker PIDs, preserving standard module and test skips.
The returned module states are14 PASSED and12 SKIPPED, including twelve
multiprocessing submodules; this is not a claim that all requested modules
executed ordinary tests.
It claims no complete nested-process or per-method-ID census.

This is an **author proposal to accept the retained native ASan evidence
under the corrected host reader**, pending independent root/qualifier review.
It does not fabricate a successful original completion, post-command verify
or row cleanup receipt: those artifacts are absent because the frozen reader
failed first. The actual existing post-gate cleanup is verified idle and
before deadline. Original batch failure remains a failure, with this explicit
additive adjudication attached. Stackref's default-selection contract has no
fixed requested-set comparison and is unaffected. No new cohort, compiler,
target, sanitizer run, source edit or activation occurred here.
