# Proposed scope amendment for the stack-reference diagnostic profile

2026-11-01. **PROPOSED; independent review pending.** This changes the
[broad qualification plan](broad-qualification-plan-20261101.md), not the
runtime, tests, checker or prior outcomes. It grants no native work or landing.

Withdraw successful `Py_STACKREF_DEBUG` qualification from this landing's
supported scope. Keep its fresh build and failed partial full run as diagnostic
evidence. Do not replace that failed run with selected passing modules, twelve
exclusions, an expected-failure list or a fabricated complete pass. Ordinary
Linux/x86-64/GIL/GCC computed RC, private extension ABI and boxed-slot shapes
remain the proposed supported runtime. This amendment introduces no runtime
collector choice or checker suppression.

The whole diagnostic profile is currently unvalidated. Pristine upstream321925
failed during bootstrap before any of the three planned comparison methods.
One separately reviewed ownership-assignment correction produced successor
d2ca412; that fresh build also aborted in bootstrap, now on a borrowed-reference
check. Both attempts stopped mechanically and remain failed. No comparison
method ran. These failures show that the requested upstream comparison cannot
serve as a working control; they do not establish upstream reproduction of
any of RC's twelve failed modules. Stop the upstream repair path here.

The retained RC partial run has449 passes,94 skips,12 failures and eight
resource-denied results among563 returns, with two modules unfinished. Its
actual shutdown/fork open tokens, last-GC coroutine cycles, TLS-free async
lookups and remote readers of table IDs remain unresolved findings. Source
similarity alone does not excuse them. Formal review must inspect every failed
family and the fork-specific borrow/open bookkeeping before deciding whether
any finding also identifies a supported ordinary-build defect. Such a defect
continues to block landing. A future checker-support claim requires its own
repair and fresh qualification; this proposal schedules neither.

Retain the accepted ordinary release and O1/debug full suites,50,827 tests
each with zero failures and visible standard skips; accepted scoped ASan
3,827/0/77 with its explicit fake-stack/LSan limits; standard refcount/refleak
and focused lifecycle/ABI evidence; traversal disposition; source syntax and
canonical reproduction. These establish their actual scopes, not universal
ownership correctness or checker support.

The remaining mandatory integration gates do not change: reviewed packaging
successor and exact source/image bridge, regeneration without source drift,
affected ensurepip/venv on both ordinary profiles, two real public pip rejection
controls, unchanged C-extension/refcount interval, independent landing review,
preserved user files and historical products, exact integrated source bridge,
fresh main release/debug builds and42 selected consumer methods per image.
No additional ordinary full sweep is required for an unchanged compiled input
set. A concrete new failure or source change receives an affected decision.

Only an independent acceptance of this proposal and the actual failure/source
assessment can make this scope amendment authoritative. It does not close the
checker issue as fixed, clear the packaging gates or authorize JIT code. The
collector decision remains RC; tracing-GC work stays closed.
