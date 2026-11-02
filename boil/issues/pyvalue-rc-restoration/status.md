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

The [restoration design](design.md) passed independent review. The
[foundation implementation](foundation-implementation-20261101-v1.md) froze early
and passed independent component source review on `95be1d28` in the isolated
universal-value worktree. Dependent storage and native cuts continue.

- restart_audit has frozen the foundation, specialized O-call and shared
  local/slot retirement source; dictionary VM consumers have a timely source
  checkpoint and are now independently accepted as source.
  The reader/feedback source cut is independently accepted, including owned
  specialization lookups and cold/warmed dictionary reads. Its next attribute
  nomination/store design is under review; the previous window is closed.
- value_api_consumer_map independently reviews source and maintains the exact
  native consumer inventory (660 paths in its sealed first version).
- coroutine_design_review implements list/structseq storage and iteration;
  that partial source component is independently accepted. Its native-scope
  engine, bounded dictionary clear/release and owning-read components are
  source-accepted; mutation/growth and the value-native constructor are now
  independently source-accepted. Copy/merge, conditional insertion and managed
  storage now also pass independent source review. The next native vector-call
  engine and generic VM caller are designed, awaiting separate grants. The coordinator
  implements tuple/cell/member/iteration and set storage; partial tuple/member
  cell/iteration, set, instance-slot and common-constant components are
  independently accepted as source only. Compiler constant indexing/compaction
  and checked value-list replacement are independently source-accepted;
  canonical typed constant-key production and managed initialization before
  CREATE are independently source-accepted. Recursive cache/interning remains
  in design; the canonical repr guard is independently source-accepted with
  actual Object/fixture syntax checks passing and four unexecuted methods.
- The coordinator maintains the log and monitoring loop, prepares qualification
  and the interpreter performance checkpoint, and integrates reviewed changes.

The RC images remain a qualified interim base. JIT source95be1d28 is frozen and
independently source-accepted, but its next native batch is held after user
steering. Its controller is bound, not activated; no corrected runtime result is
claimed. Failed90cdb7bb source/image/results and all earlier evidence remain
preserved. The method/tracing JIT stays in scope and must share the restored
value/ownership contract.

Source implementation is underway; no restored candidate runtime result is
claimed. Next: review and exercise the foundation, finish universal container,
attribute and native-operation coverage, then qualify the coherent runtime.
The user's next performance checkpoint measures the register interpreter with
unified PyValue and basic optimizations, with JIT execution disabled and checked.
Compare matched configurations against the interim RC interpreter and upstream,
report the suite aggregate and individual workloads, and distinguish startup,
lowering and warm execution. Use results to choose specific implementation
improvements. Universal values and the selected collector remain requirements.

[Implementation progress and shared API contracts](implementation-progress-20261101.md).

The user explicitly requested an [opaque extension API and gradual migration](extension-api-direction-20261101.md).
Internal PyValue layout is not the future public extension contract. Rebuilt old
extensions may use slower owning object bridges and migrate incrementally;
hard C API/binary compatibility is not required. This design step supersedes
the immediate dictionary API-removal action and its approval question.
