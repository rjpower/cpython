# Native method lookup and scalar type boundaries

Checkpoint50 `3a3ee2ac368` closes ten method lookup/call functions. The producer
materializes a selected canonical value once into an owned native object view.
Consumers check status and pending before borrowing; results survive protected
cleanup. Presence and finalizer errors have explicit caller envelopes. Two
forwarding signature dependencies are also checkpointed.

Checkpoint51 `14fc009d42f` closes five scalar boundaries. Slot nomination uses
pure object-kind guards. Descriptor/new calls use owning getters. Special-method
outputs keep owned object views in existing PyValue slots, covering actual VM
exceptional-with consumers; ordinary type attributes remain canonical. No new
helper, header, generator or stored representation is introduced.

Both full held type compositions and emitted objects pass without diagnostics.
The checkpoint51 object is 2,447,336 bytes, with 217 stable common dependencies. Reference
discovery passes six ordinary controls for50 and all five for51. The50 module
also contains two mandatory candidate contracts: preserve the primary presence
lookup error and report finalizer descriptor lookup errors. Upstream3.14 fails
those assertions; the complete eight-test reference run is explicitly not a
pass. Candidate execution and native fault qualification remain pending.

Root projected only complete reviewed definitions and forwarding dependencies
into HEAD, including the canonical provider rename and its adjacent return-format
comment. Every other HEAD byte and the held typecache test remain unchanged.
The remaining canonical foundations/helper still need extraction and MRO,
guard and lifecycle admission; these are dependent source checkpoints.

Evidence: [lookup final review](../../../../../build/pyvalue-type-lookup-call-envelope-composed-20261103-v3/independent-solport-composed-review.md),
[lookup commit](../../../../../build/pyvalue-type-lookup-call-envelope-composed-20261103-v3/commit.json),
[scalar final review](../../../../../build/pyvalue-type-scalar-boundaries-composed-20261103-v1/independent-solport-composed-review.md)
and [scalar commit](../../../../../build/pyvalue-type-scalar-boundaries-composed-20261103-v1/commit.json).
