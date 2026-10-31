# Handler and observer contract

`Python/bytecodes.c` and the cases-generator analysis define operations,
operand formats, effects and generated handlers. One generated handler domain
must serve execution, specialization, instrumentation, statistics and decoders.
Consumers must not guess the domain from an approximate separator or an 8-bit
range.

## Execution

- A handler consumes initialized operands and publishes its outputs and frame
  state according to its declared operation, including exceptional paths.
- Call, return, yield, resume, unwind and direct redispatch use the actual current
  frame. Preserve instruction-position, root and monitoring state at observation
  boundaries; generated local variables do not prove machine-level redundancy.
- Specializers run no Python code. Helpers that can collect or reenter need
  their actual effect and root contracts, even when they usually take a fast path.
- A noncollecting leaf declaration requires a concrete transitive source proof.
  It is not a family exemption or a way to bypass a linked inline audit.
- Guarded specialization retains complete deoptimization/fallback semantics.
  Result representation and receiver admission are distinct facts.

## Effects and consumers

Implicit frame state, object mutation, exceptional transfer and observer effects
matter as well as explicit scalar operands. An inert temporary-slot decoder is
not complete liveness, purity or a runtime map. Unsupported effects must fail
closed while the maintained root path stays intact.

Monitoring and instrumentation use the executing instruction/handler identity.
Storage and export formats must preserve every declared handler and reject
incompatible schema/layout explicitly. Writer, reader, remote and generated
consumers must move together when their common format changes.

The maintained dispatch convention is not changed by the deferred
[H1](../issues/h1-tail-dispatch/status.md) experiment. Usable root maps are
separately deferred in [H2](../issues/h2-root-maps/status.md). Remaining transport
work belongs to the [handler consumer issue](../issues/handler-tooling-consumers/status.md).
Neither a quota of typed opcodes nor an inert metadata table is a performance goal.
