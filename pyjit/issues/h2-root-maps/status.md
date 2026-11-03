# H2 root maps

- State: deferred
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: a scheduled usable liveness/root-map consumer
- Branch/head: retained `boil/h2-root-effects` at `b3e6847348d`
- Worktree: none active

## Now

Inert decoder `5df33656912` and parser correction `b3e6847348d` are unmerged.
The decoder declares 27 supported pristine handlers and 126 unresolved handlers.
Its temporary-slot inputs/outputs are not complete semantic effects, purity,
CFG/liveness analysis or runtime root maps. No map reading or slot-clearing
retirement is active, and the maintained root coverage remains unchanged.

## Next

Resume for a concrete consumer. Review current handler semantics, exceptional
transfers and consumer failure behavior together. Unsupported results retain
the safe path; successful temporary decoding cannot justify stronger claims.

## Completion

Validate a complete useful producer/consumer slice before changing runtime root
reading or clearing. Otherwise retain the source and remain deferred. There is
no current implementation assignment or performance promise.

## References

- [Handler/effect contract](../../design/handler-and-observer-contract.md).
- [Root contract](../../design/gc-roots-and-lifetimes.md).
