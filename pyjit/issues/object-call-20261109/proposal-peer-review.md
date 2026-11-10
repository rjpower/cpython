# Lookup and keyword plans: independent proposal review

Reviewed 2026-11-10 against frozen runtime `4d666e1b482` and the current
[lookup proposal](lookup-proposal.md) and [keyword proposal](keyword-proposal.md).
Runtime source remained untouched during this review. No code was changed and
no native tests were run. The coordinator reports archival checkpoint publication
at `cb906f9bc52`; this review does not independently certify that publication.

**Approve both revised plans for bounded implementation.** Lookup approval is
for the revised `_PyValueGuarded` representation, not the original persistent
`PyValueBorrow` field. Keyword approval is for the one-to-three-keyword cache
capacity and stated exclusions, with the existing consuming frame convention.
Neither approval is runtime admission or evidence of a speedup. Root retains
regeneration, merged validation, measurement and integration ownership.

## Required correction, now resolved

The first lookup draft stored `PyValueBorrow class_value` in feedback and
proposed borrowed-value snapshot fields. That violates the canonical typed-borrow
rule: a borrow cannot escape into a heap field (`design/pyvalue.md:264`). The
runtime already has the precise solution in `Include/internal/pycore_typecache.h`:
`_PyValueGuarded`, a non-owning cache word whose interpretation requires a valid
version/owner proof. Its comment expressly distinguishes it from both an owning
`PyValue` and a transient `PyValueBorrow`.

The revised proposal moves that existing type and its helpers to a shared value
header, stores guarded metadata, and creates a transient borrow only after the
type/layout guards. Snapshot copying never loads a borrow, dereferences a
pointer or closes a payload. This resolves the representation blocker without
adding another encoding or an owning cache. Moving the existing definition must
update both consumers and avoid a duplicate definition.

There are no remaining design blockers in the reviewed revisions. The following
requirements are part of their approved implementation boundary, not optional
follow-up optimizations.

## Lookup ownership and guard assessment

The proposed ordering is correct: validate type and receiver state; load and
duplicate the guarded canonical value; detach the receiver; use
`IR_RETIRE_PUBLISHED_RESULT`; let generated publication finish. It protects the
selected result across finalization and uses the existing temporary/local/discard
contract. It does not treat a version guard as permission to keep a borrowed
object live across a callback.

The no-shadow proof is specific and adequate for the admitted scope. The
specializer requires absence from shared keys; shared-key insertion invalidates
the owning type before publishing the new name (`Objects/dictobject.c:1537`).
The valid-inline-values check handles detachment/replacement. The handler need
not acquire a new shared-key-version field, but the mutation tests must exercise
the actual specialized path. A key added on another instance deliberately
reduces coverage; it is not grounds to weaken the guard.

The proposal correctly retains descriptor/custom-hook classification and the
non-method restriction. One wording detail matters during implementation:
`NON_DESCRIPTOR` requires an **immutable descriptor type**, not an immutable
value. A `list` or `dict` object can be an eligible class value, since its type
is immutable and supplies no descriptor protocol. Conversely an instance of a
user-defined value class normally classifies as `MUTABLE` and cannot exercise
the retained non-descriptor handler. Do not broaden or narrow this eligibility
as a side effect of canonicalizing the payload.

The existing close-before-duplicate source hazard remains unconfirmed natively.
Root's first debug controls passed but `get_ir` showed generic `LOAD_ATTR`, so
they do not test the suspected path. Use eligible object values and assert the
NONDESCRIPTOR handler before the finalizer mutation. A passing generic witness
must not be presented as resolving the hazard. The safe ordering change is
required for this family regardless of whether a crash is reproduced.

Generator and feedback implementation checks:

- Cache declarations hold `_PyValueGuarded`, never reinterpret inline bits as
  a pointer. Only the guarded handler converts metadata to a transient borrow.
- Snapshot helpers need the recorded handler when choosing the active union
  member. `fb_row_of` currently receives only the header; update its interface
  and callers as necessary. Keep shape-transition and binary-descriptor payloads
  coherent, including partially populated observations and reset feedback.
- Assert unchanged feedback sizes, or explicitly update accounting if they
  change. Preserve the existing GIL-only scope and do not accidentally evaluate
  object-only deferred-refcount checks on inline values in conditional code.
- Add codegen positives and wrong-size/type negatives for the cache binding;
  retain result-publication mutation/error tests. Regenerate, inspect and compile
  the actual output rather than reasoning only from the DSL.

## Keyword ownership and call-convention assessment

The six-byte destination payload is a principled small-plan capacity. It admits
arbitrary signatures, positional prefixes, permutations and default holes;
three is not a hardcoded function arity. Exact-name identity checks against the
guarded code avoid tuple/key lifetime problems. Nonidentical equal strings take
the existing provider, which remains responsible for language equality.

The plan's transfer handles overlap correctly. Detaching every keyword owner
and kwnames before `_PyFrame_PushOverlapping` both breaks permutation cycles and
prevents frame initialization from overwriting unremoved owners. The positional
prefix remains in place; a fixed array of at most three moved owners is bounded
and becomes empty as child bindings are installed. `PyValue_XDUP` of current
defaults is infallible; no callback or fallible action may enter this interval.

After all child homes and the parent recipient/continuation are valid, making
the child current and closing the detached exact tuple of exact Unicode names
does not retire arbitrary Python arguments. Preserve that exact-type admission
on every hit, not merely when building the plan. The generic transaction still
owns all errors and unsupported providers/shapes; a post-transfer miss is illegal.

Explicitly supplied keyword-only arguments make in-place `__kwdefaults__`
mutation irrelevant to this plan. Current positional defaults are read through
the version-guarded function. The exclusions avoid adding an independent
kwdefault cache, varargs binder, coroutine entry or frame flag.

Implementation checks:

- Define how the plan builder gets self presence. The existing public
  specializer receives a count that already includes self; either pass the bit
  explicitly or derive it from the current instruction's original argument
  count under a documented contract. Test both forms at the same shape boundary.
- Check kind/count, tuple shape, cached destination bounds, prefix exclusion
  and distinctness before any detach. A changed tuple/count must not read stale
  payload entries. The builder proves coverage; the hot guards must preserve
  every input that coverage depends on.
- Preserve the exact guarded-call prefix, recursion check and full keyword
  window proof. `CALL_KW_BOUND_METHOD` must not inherit the fast primitive merely
  because it previously shared `_PY_FRAME_KW`.
- A pre-effect miss calls the keyword transaction through `self.call_run`;
  preserve the current positional transaction for positional calls. Test the
  generated fallback selection and changed-transfer negatives.
- Demonstrate actual binder hits. A `CALL_KW_PY` opcode alone also describes the
  old generic path. Stats-only hit instrumentation is justified if existing
  evidence cannot distinguish them; it needs root's shared-file assignment.

## Shared files and integration ownership

These lanes are logically independent but do **not** have disjoint file lists.
The coordinator's proposed region ownership is sufficient if followed exactly:

| Shared file | Lookup region | Keyword region |
| --- | --- | --- |
| `Include/internal/pycore_feedback.h` | Attr payload and poly snapshot fields | Call payload and call flags |
| `Python/specialize.c` | Snapshot helpers and attribute specializers | Call specializers and positional member renames |
| `Python/bytecodes.c` | Attribute producers and their cache operands | Keyword specialization and guarded-call DSL |
| `Tools/cases_generator/ir_generator.py` | Typed cache binding and lookup publication requirements | Call invocation, completion and guarded window proof |

Lookup additionally owns the shared guarded-word move in `pycore_value.h` and
`pycore_typecache.h`. Keyword owns `ceval.c`, its specializer declaration and its
call tests. Use a separate lookup test module for runtime **and codegen** coverage,
or send an isolated codegen-test patch to root; do not concurrently edit the
keyword-owned `test_generated_cases.py`. Root alone regenerates after both
producer changes are present. If a helper crosses the agreed regions, stop that
edit and assign one owner rather than merging competing rewrites.

Both plans retain canonical ownership, initialized homes, one call ABI and
transactional publication. Admission still requires focused behavior checks,
release/debug compilation, independent actual-diff review and paired performance
evidence. Preserve the frozen control and its disclosed limitations; do not
expand either lane to rescue a weak measurement.
