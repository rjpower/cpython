# Foreign interpreter lifecycle

- State: deferred
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: a decision to expand the supported interpreter boundary
- Branch/head: none assigned
- Worktree: none assigned

## Now

Supported execution is Linux, one main interpreter, ordinary threads and the
GIL. Subinterpreter creation rejects explicitly. Foreign owner retirement,
metadata transfer/adoption and cross-interpreter callback progress are outside
this support boundary; they are not unfinished gates for the landed runtime.

Main-interpreter shutdown, fork, buffers and callback lifetimes keep their actual
contracts. Deferring foreign lifecycle does not waive roots, resource behavior
or recoverable errors in supported callers.

## Next

Resume when foreign-interpreter support is selected. Define observable creation,
close, callback and fork behavior first; review metadata/root lifetime and
allocator ownership as one coherent scope.

## Completion

Complete and validate the chosen lifecycle before exposing the capability or
freeing foreign state. Until then, keep explicit rejection and visible unsupported
coverage. Deferred is neither abandoned nor supported.

## References

- [Runtime support boundary](../../design/runtime-architecture.md).
- [GC and native lifetime contract](../../design/gc-roots-and-lifetimes.md).
