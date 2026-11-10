# Object lookup and keyword binding: implementation review

Reviewed 2026-11-10 against the uncommitted runtime diff from `4d666e1b482` in
`execution-boundaries-20261108`, including regenerated handlers and metadata.
This review changed no runtime source and ran no native tests. Runtime file
references below are relative to the worktree recorded in HANDOFF.

**Source review finds no new semantic or ownership blocker in the two lanes.**
The first real release compile did expose a generated declaration error; its
producer correction is reviewed below and the corrected release build passed.
Final runtime admission still requires focused behavior checks and evidence that
the intended fast paths execute. This report does not qualify an unbuilt tree
or claim performance improvement.

## Findings and disposition

1. **Generated declaration error, identified by release compilation.** The first
   generated NONDESCRIPTOR handlers assigned `this_instr` after retirement while
   declaring it `const`. The release build failed at the assignments around
   `generated_ir_cases.c.h:13302,13385`. The producer's old opcode-name allowlist
   did not include the newly transactional readers. The correction derives a
   mutable declaration from the already checked `result_publication` contract;
   this is preferable to another opcode exception. Actual-handler codegen tests
   now assert the mutable declaration and subsequent reload. The coordinator's
   corrected `object40b` release build passed. Passing Python generator tests
   alone did not catch this error.
2. **Keyword hit coverage must be demonstrated separately from opcode coverage.**
   Several new runtime tests assert `CALL_KW_PY`, which also executed the generic
   provider before this patch. These checks establish specialization but do not
   prove the new binder ran. The new stats-only `keyword_plan_hits` counter is
   correctly incremented only after child publication and is included in dumping
   and merging. Capture positive hits for admitted shapes and fallback controls
   before attributing timings or calling the lane validated.
3. **Preexisting positional-plan default-shape assumption requires separate
   triage.** The positional branch still uses a cached `default_start` while
   functions built from the same code can share `func_version` and have different
   defaults. Its field rename did not introduce the assumption. The new keyword
   branch explicitly handles it correctly. Root has been notified; any positional
   repair is a separately reviewed increment, not silently counted among the
   keyword changes reviewed here. See its proposal approval below.

## Lookup path

The implementation follows the revised proposal. `_PyValueGuarded` moved from
the type-cache header into the shared internal value header without changing its
encoding. Feedback stores inert metadata rather than an owning home or escaped
`PyValueBorrow`. The cache emitter checks attribute-feedback kind and four-unit
width. `_PyFbAttr` and `_PyFbPoly` size assertions preserve the current 64-bit
layout; snapshot copying selects the active payload from the recorded opcode.
No historical snapshot is interpreted as a live borrowed object or closed.

`specialize_attr_loadclassattr` accepts a canonical borrow; only method admission
extracts an object pointer for the method payload. `INLINE_VALUE` shares the
existing conservative non-descriptor admission. Shadowing, custom hooks,
descriptor classification, shared-key invalidation and dictionary-layout
exclusions remain in place. The conditional deferred-refcount check first tests
whether the descriptor is object-form, avoiding object operations on inline
scalars. This review qualifies the configured GIL runtime, not free-threaded
concurrency behavior.

The generated NONDESCRIPTOR handlers validate the receiver type (and valid
inline values where needed), load the guarded metadata as a transient borrow,
and acquire `attr` before consuming the receiver. A temporary destination owns
the result before retirement; a local/discard result remains a C owner through
the existing completion helper. Errors close the held result or leave the
published temporary to unwinding. Successful reentry refreshes instruction and
slot state before final local publication. The updated metadata records the
result phase and possible error handling. There is no new return, root-map or
frame-state convention.

The tests cover inline and object values, mono/poly observations, dictionary
materialization/replacement, shared-key shadow insertion, class/base/MRO changes,
descriptor/hook mutation, and receiver finalization for local/temporary/discard
and alias destinations. They assert actual NONDESCRIPTOR handlers. Mutable
descriptor **types** remain generic; values such as lists can still specialize
because their type is immutable. This distinction fixes the earlier witness's
coverage problem.

The coordinator's retained `lookup-lifetime-before.json` reports return code
`-11` for both upstream and frozen39 release. Frozen39 stdout shows
`LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES`; the coordinator also reports a frozen39
debug crash. The reviewer inspected the JSON and frozen39 handler evidence,
not a debugger trace. These results support the source lifetime hazard, but do
not show a fork-only regression, and upstream must not be described as passing.

The safety requirement is that a selected live result cannot become a dangling
borrow when receiver retirement happens. It does not require every Python
implementation to retire a receiver at an identical time. Finalizer-order
witnesses stress this selected RC runtime's ownership contract; they must not
be generalized into an exact Python destruction-timing promise.

## Keyword path

`specialize_keyword_binding` produces at most three destination indices in the
existing six-byte payload. It rejects unsupported providers and signatures,
nonexact name tuples/strings, duplicate destinations, positional collisions and
uncovered required/keyword-only parameters. Self presence is passed explicitly
to specialization, resolving the count ambiguity raised in proposal review.
Names and defaults remain owned by the live function/code, not by feedback.

The generated `CALL_KW_PY` keeps the PEP 523, function-version and recursion
guards. Its binder checks frame ownership, plan kind/count, self presence,
current name tuple shape, provider, executable and capacity. Each current name
must match the current code name at its bounded destination; destinations remain
distinct and outside the positional prefix. Every miss occurs before a home is
detached and selects the keyword transaction. Bound-method and instrumented
handlers retain their ordinary paths. The positional guarded call still selects
the vector transaction.

The shared-function-version issue is handled explicitly in the keyword hit.
It reads the current default count, computes `first_default`, and verifies that
every required positional hole is covered by a cached keyword. The constant-time
early bound avoids a long failed scan; at most three required holes can remain
eligible. Reads of default values use the current tuple and current offset.
Keyword-only defaults need no extra guard because all keyword-only parameters
must be supplied. The new same-code/different-default-count regression is aimed
at the right failure mechanism.

Once guards finish, all keyword owners and kwnames are detached before
`_PyFrame_PushOverlapping`. This breaks permutation cycles and keeps frame
constant initialization from overwriting argument owners. The positional prefix
stays in place. Keyword owners move into their distinct child locals, uncovered
positional locals receive owning default duplicates, and the parent continuation
and recipient are published before the child becomes current. The remaining
names owner is an exact tuple of exact strings, so its close cannot invoke
arbitrary Python finalizers. No fallible operation or fallback occurs inside
the transfer interval. Error and unsupported cases retain the original complete
transaction.

The fixed C array is moved ownership, not a second bank of borrowed homes. No
new per-frame flags, argument layout, callback replay or result adapter was
introduced. The union keeps every call feedback entry the same size. Existing
positional field users and specializer declarations were updated consistently;
source searches found no stale old field or polymorphic descriptor-array uses.

## Verification boundary

The reviewed generated diff was inspected alongside its producers and metadata;
`git diff --check` reported no whitespace errors. Root's first regeneration
succeeded, but its first real release build failed as described above. Evidence
is under `build/pyjit-link-wave1/object-call-20261109/`:
`object40-regen-checks.json`, `object40-build-checks.json`,
`object40-release-build.log`, and `lookup-lifetime-before.json`.

The reviewer subsequently inspected `object40b-build-checks.json`: the real
release build ran in `execution-boundaries-release` and returned zero. Its log
ends with 117 checked modules. This meets the repository's release-compile review
requirement for the corrected two-lane image. It does not qualify later source
edits automatically.

## Additional positional-default proposal approval

The [bounded positional guard proposal](positional-default-guard-proposal.md)
is approved for implementation. When the direct positional plan needs defaults,
guard the current default count, required-argument coverage and equality of the
cached offset to the current right-aligned offset before allocation or detach.
Absent/short defaults must miss; equal-sized tuples can remain hits and supply
current values; differently sized tuples use the established generic transaction.
No new binding semantics or ABI is needed. Same-code functions with equal,
larger, shorter, empty and overlong default tuples are the right regressions.
This approval covers the proposal, not an as-yet-unreviewed implementation.

The coordinator owns the corrected release/debug builds, focused tests, API
checks, fast-path counters and unchanged paired workloads. Before admission,
retain actual keyword hit evidence, review and rebuild the positional repair,
and compare the merged image against frozen39
without claiming the previously known full-suite failures disappeared. No
speedup, full-suite pass or positional-default repair is established by this
source review.
