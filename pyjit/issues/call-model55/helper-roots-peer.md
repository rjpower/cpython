# Helper-root proposal source peer review

2026-11-12: GC lane review of [helper roots](helper-roots-proposal.md) against
the [C-owner census](c-owner-census.md). No native/build evidence.

The callback matrix and ownership distinctions are sound: existing OWNER sites
have no callback before published retirement; TAKE can detach inputs before a
provider, so a cleanup-only record is insufficient. Mapped PRE borrows, detached
values, raw objects, output prefixes and pending local/discard results need
separate authority. Numeric-box workspaces cannot become general owner banks.

The proposed linked scalar/typed object schema is coherent as a future precise
native interface, with field storage representing an existing owner and no
registration INCREF. Unlink/NULL precedes close or steal; aliases may mark twice
without inventing container edges. Raw object boxes retain their object ABI.

The coordinator narrowed implementation after the [collector-policy audit](collector-policy.md):
do not introduce unconditional CValue/CObject heads or port all sixteen scalar
workspaces solely for the verifier. GIL cyclic RC already accounts for counted
external owners; M4 removes arena-value scans while retaining explicit native
conservative roots. Numeric boxes remain live leaf roots under that policy.
This is a scope/cost correction, not approval to ignore detached arena prefixes.

Prioritize actual staged arena/binder/unpack/native-scope omissions from active
frame authority. Use reached outlined adapters and the real prefix interface;
keep provider results rooted through callbacks/cleanup and bind complete child
ownership before replacing parent authority. Full callback ledgers and reader
migration still gate M4. A small separate cleanup may document existing CValue
workspace semantics and remove the unused broken FT head/loop, after root approval.
