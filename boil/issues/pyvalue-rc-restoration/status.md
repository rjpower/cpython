# Restore universal PyValue with reference counting

The user reaffirmed on 2026-11-01 that the core must use one LuaJIT-style
NaN-tagged value representation throughout interpreter operations, frames,
containers and instance storage. Returning to reference counting did not remove
that requirement. Boxed heap storage is an interim regression, not the target.

## Provenance

This functionality existed before the RC restoration:

- `98f38e1da76` changed `_PyStackRef` into the core `PyValue` type and added
  `Include/pyvalue.h`.
- `20693f43a82` changed lists and tuples to store values without boxing on stores
  or reads. Later work extended the value-based core and native interfaces.
- `1440f8c6fd1be82bc02028df5728c3cb3ad0c81a`, “Restore coherent
  reference-counted compiler and runtime core,” deleted `Include/pyvalue.h`,
  `Include/internal/pycore_value.h`, `Python/pyvalue.c` and associated value-based
  container/API work. Its parent is
  `74f1705774b10a225e3342b8a952f96b7e3bc8f8`.

The coordinator's [GC fallback decision](../gc-parity-deadline/decision-20261101.md)
explicitly deferred these features. That broadened a collector fallback into a
representation rollback. The user's clarification supersedes that deferral;
the original decision and its evidence remain unchanged.

## Required outcome

Keep reference counting with cyclic collection as the single collector. Restore
one canonical tagged PyValue encoding across frame homes, core operations,
argument/result paths, lists, tuples, dictionaries, sets, cells and instance
storage. Keep internal absent/error/index sentinels distinct from Python values.
Retain the established scalar identity policy and explicit object-form boundaries.
Do not introduce permanent StackRef-versus-PyValue or boxed-versus-value runtime
modes.

Object-valued references still need correct strong/borrowed/moved ownership,
publish-before-release order and cyclic traversal. Inline scalars need no object
reference count. The old tracing implementation's ownership-free copies and
no-op reference operations cannot be restored unchanged under RC. Object-form
C interfaces require an explicit allocation, error and lifetime contract;
fallible boxing cannot be hidden behind a borrowed-pointer API without one.

## Current work

- restart_audit: read-only provenance and dependency audit; concrete restoration
  breakdown separating value representation from tracing collector assumptions.
- qualification_review_resumed: independent scope/dependency audit and review
  of the restoration design, including RC ownership and C interface boundaries.
- coordinator: update the active plan, preserve existing work and turn the
  reviewed design into implementation and qualification steps.

The RC images remain a qualified interim base. JIT source95be1d28 is frozen and
independently source-accepted, but its next native batch is held after user
steering. Its controller is bound, not activated; no corrected runtime result is
claimed. Failed90cdb7bb source/image/results and all earlier evidence remain
preserved. The method/tracing JIT stays in scope and must share the restored
value/ownership contract.

No new representation source changes or candidate executions have begun. Next:
review the restoration dependency map and API/ownership contracts, implement
coherent increments, then qualify the complete universal-value RC runtime.
