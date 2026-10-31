# Handler tooling consumers

- State: deferred
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: a scheduled transport consumer or handler-renumbering change
- Branch/head: none assigned
- Worktree: none assigned

## Now

H3's landed generated domain/instrumentation work does not close every
external consumer proposed in H3-1. Sampling binary records still use
8-bit opcodes; out-of-range values become the NONE sentinel. Numeric
separator exports such as `HAVE_ARGUMENT` also remain. The planned wider
transport, target-schema identity and export migration need a current scope
reconciliation, not a claim that all H3 work remains unimplemented.

This is separate from archived H1 frame passing and H2 liveness maps.
No transport implementation, renumbering or new measurement is active.

## Next

Resume when a concrete consumer needs the generated domain or renumbering
is scheduled. Assign the full writer/reader/remote-layout/export boundary;
reconcile H3-1's inventory against current code before changing formats.

## Completion

The admitted consumer preserves every supported handler and validates actual
target schema/layout identity, rejecting incompatible records explicitly.
Update declared exports and all callers together. Otherwise retain the
deferred scope and its limitations without an approximate separator or
silent legacy-format fallback.

## References

- [H3-1 consumer design](../../design/work/H3-1.md).
- [Historical compatibility decisions, §0.2](../../archive/process-2026-10-31.md).
- [Binary record layout](../../../Modules/_remote_debugging/binary_io.h).
- [Writer](../../../Modules/_remote_debugging/binary_io_writer.c) and [opcode exports](../../../Include/opcode_ids.h).
