# Handler tooling consumers

- State: deferred
- Updated: 2026-10-31
- Owner: unassigned
- Dependencies: a scheduled transport consumer or handler-renumbering change
- Branch/head: none assigned
- Worktree: none assigned

## Now

The generated handler domain exists, but external consumers do not all use it
without loss. Binary sampling records use 8-bit opcodes and an out-of-range NONE
sentinel; numeric separator exports remain. Wider transport, target-schema
identity and export migration require a coherent consumer scope. This is
separate from H1 frame passing and H2 maps.

## Next

Resume for a concrete consumer or renumbering change. Assign the writer, reader,
remote layout and exports together; reconcile their current representations
before changing formats.

## Completion

The admitted consumer preserves every supported handler and rejects incompatible
target schema/layout explicitly. Update all affected uses together, without an
approximate separator or silent format fallback. Otherwise remain deferred.

## References

- [Handler/observer contract](../../design/handler-and-observer-contract.md).
- [Binary record layout](../../../Modules/_remote_debugging/binary_io.h).
- [Writer](../../../Modules/_remote_debugging/binary_io_writer.c).
- [Opcode exports](../../../Include/opcode_ids.h).
