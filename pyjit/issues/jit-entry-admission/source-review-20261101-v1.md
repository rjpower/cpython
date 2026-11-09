# Inline admission source review

ACCEPT clean source11aefbdd6405953a0bf3605b3a9f703c01285bf6, parent599897,
for separately granted bounded native feedback. Exact source capsule
69ec0977fd47b2a573fe0860d8bc847af4d07af9881a7ddde7c1a1a025a851a8 and patch
7a6fc3b0662811a2aa472bcc4eec435a882580d85c800577bf3cf312ea2245e5 match.
This is source acceptance only, not emitted-mechanism, performance or adoption
acceptance. The rejected parent and its scores remain unchanged.

The shared state values/layout remain exact. MayEnter masks only success_seen
0x80, admits0/1/3 at word1, and rejects BUILDING, terminal/invalid states,
NULL and static cleanup. Its SIZEOF_VOID_P==8/!GIL condition is source-equivalent
to pycore_stackref.h:105–107, including when that feature macro has not yet been
defined; the remaining Linux/x86/!Py_STATS guards match full entry. No profile
is newly supported. The false path preserves supported-build debug decline
accounting through the correctly declared existing NoteDecline; unsupported,
NULL/static and release paths add no counter update.

Both hooks load raw code->_co_exec, preserving the full entry's ability to
decline identity mismatches rather than adding a prevalidation debug assertion.
Existing notification/RESUME ordering remains. Reversing the two canonical and
14 generated admission blocks restores the complete parent files. Other six
generated products are byte-identical. All prior core JIT production source
restores exactly after the shared-state namespace move; the only additional
core function is the cold test fixture. Full TryEntry, instrumentation, decoder,
native compiler/leaf, owner/lifetime and prefix header remain unchanged.

Static cleanup is explicitly rejected. Ordinary exec initialization seeds
immutable flags/exception-table/short-word exclusions before both return paths
and publication. Flags are prior entry exclusions, not a claim of identical
Probe diagnostics for synthetic flag-only code. No transient tagged/observer,
breaker, error or input condition becomes a permanent rejection.

The cold PyAPI fixture is in core, with declaration in allowed ir.h; the module
adds only its type-checking shim/method entry and no hidden-data relocation.
Its local three-word array supports all256 byte-state/three-PC cases and the
NULL/static tests without mutating a live owner. Empty executing copies do not
form words+1; allocation errors propagate, and only scalar results survive to
module boxing. Root's source-disposition0622f2f5 explicitly authorizes this
narrow fixture relocation and zero-word correction within the same seven paths.

All12 test IDs remain; two existing methods add fresh0→Probe1, cached rejection
and pre-Probe generator/exception/short/empty/coroutine seed checks. Empty code
is never executed. Independent whole-file projections, source AST/layout
checks and26 existing host generator tests pass. Source lexical checks are not
C compilation. The initial direct host source-support lazy-syntax failure is
preserved; the accepted harness imports host support before source generator
tests. My initial ID-count assertion included five class helpers; correcting
the test_ classifier required no source change and is recorded.

Next gates remain fresh release/debug54, actual skipped-call/IP/unchanged leaf
inspection, then one separately fixed all29 comparison directly. Native calls,
compiler execution, probes and timing remain held for root authority. No new
cohort, counter project or text/RSS veto is requested.
