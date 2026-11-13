# Frames lane review of delegated consumers

Bounded production source GO for the GC lane's genobject and remote consumer
packet, with the unused-C check removed. This is source review, not native
validation or a complete suspended home-map claim.

The GIL getter reads only while suspended; FT acquires YIELD_FROM_LOCKED and
holds it until the receiver reference exists. Resume/close/throw wait on that
claim. Close and throw use their saved prior delegated state after claiming
EXECUTING, retain the phase through delegated callbacks, and restore state if
finite lookup fails. Actual resume publication remains the invalidation point.
The public constructor preserves only the bit selected by the frames copy,
with exact captured combinations.

Remote traversal remains suspended-only. It validates the exact marker,
execution pin and ready image identity, continuation, immutable word and
matched A/D before reading a receiver. No old return-slot subtraction remains.
The debug descriptor grows by 48 bytes to 1080: one phase offset, two code
fields, and a three-word execution-image section. Validation covers each new
field and its local read buffer. Pointer arithmetic is checked before reads.
The remote unused-C condition was removed to match the frozen operand API.

The descriptor-corruption and unrelated-recipient tests are meaningful. The
phase-zero control now combines invalidation with invalid nonsentinel slot
65535, proving the reader stops before stale role validation; GC applied the
requested strengthening before fixture freeze. Existing FT race tests remain root's actual
claim validation. Matching IR positive tuple admission and frames source must
be integrated before generation or runtime tests.
