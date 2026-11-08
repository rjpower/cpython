# Matched checkpoint controls

The historical upstream and interim builds lack `_decimal` and several other
development-header-dependent modules. Decimal behavior probes using `decimal`
therefore selected `_pydecimal`, which reads Rational attributes in a different
order from the C implementation. Both actual C source bases read numerator
first; the current Decimal port preserves that order.

The coordinator prepared replacement measurement builds using the same
development sysroot as the candidate, preserving the historical builds:

| Role | Build under the external build root | Source |
| --- | --- | --- |
| Upstream | `upstream-release-devlibs-3a440b6` | clean existing `worktrees/upstream-3a440b6`, `3a440b65695` |
| Interim | `interim-release-devlibs-eba351` | task-created detached `worktrees/checkpoint-interim-eba351`, `eba351fee7d` |

Both use GCC 15.2, the existing default release `-O3` configuration and mimalloc,
without PGO or LTO. Interim additionally enables its shapes implementation.
Both complete all 116 expected modules with none missing or failing import;
candidate has 117 because its module roster differs. Isolated imports confirm
C `_decimal`, `_ssl`, `_sqlite3`, `_lzma` and `_ctypes`. Source identities and
configuration/import paths are retained under
`checkpoint-closure/matched-controls/` in the external evidence directory.
These are prepared controls, not performance results. Recheck actual stdlib
imports, allocator, JIT state, shapes, backend differences and page-cache
conditions when freezing the final timing window. The measurement draft now
selects these replacement builds.
