# Public free and published sweep fixture design review

2026-11-01. **Accept for a bounded cold test/source attempt.** No native execution, original-case acceptance, elapsed readiness, qualification, or production-policy change is granted.

Exact input: [design](../../../build/span-wrapper-source-design-v5/design.md), SHA256 `1334f7117af5b13fa093a0b27d5fe04724e509b468d5dd046281032b9013cbb2`, against source `7d10d5bfd33cb7302df8008065c4dd637e453478`. The original release failure and diagnostic nonreproduction remain distinct; the cause of the original failure is unresolved.

No blocking design finding remains. The split preserves two actual dispatch obligations. Public `PyObject_Free` forwards through the installed object allocator. Published sweep instead uses `gc_live.c`'s final dead-record loop and `_PyGC_FreeDeadBlock`, which clears the map and calls that allocator directly. A raw reservation alone would not prove the latter; this design explicitly retains it.

The raw fixture uses the existing typed route into public malloc, verifies actual debug client/raw geometry and identical live descriptors, then public-free forwarding and allocated→owned-free (-1) before reuse. The keeper distinguishes slot retirement from backing retirement. Direct SpanFree/MiFree cannot earn this evidence. The published fixture retains the real coroutine only on its worker, ends that worker before the fixed collection, and keeps the wrapper installed during the actual sweep. Main carries a weakref and volatile encoded target key, without decoding the referent address before collection. Target-specific forwarding, weakref death, ordinary new-coroutine admission decline, and exception restoration remain strict requirements.

Source review checkpoints, not new design alternatives:

- Bound worker event waits and joins; surface worker errors and join in failure cleanup before the stack-backed allocator context expires.
- Restore allocator, heap and route on every allocation/error prefix; never double-free the retired target, free a debug raw prefix through the public API, or allocate between target retirement and its free-slot observation.
- Keep the encoded key volatile and never predecode it in a collection-time native frame. Check the actual generated helper classification; no opaque/audit exception is authorized for the cold descriptor reader.
- Root the returned callback object during result packaging after allocator restoration. Validate actual raw and published targeted counts independently; neither can substitute for the other.

The added worker removes an opaque native-root obligation from the new lifetime assertion. It does not explain the prior O3 result or prove arbitrary absence of conservative retention. New case identities and fresh source/product feedback must be bound independently; prior failed selections remain immutable.

Verification was source-only: exact design digest, typed allocator and geometry definitions, public/free-dead dispatch, headerless ownership/state semantics, current wrapper and original test. No target, compiler, ELF command, source mutation, or diagnostic replay ran in this review.
