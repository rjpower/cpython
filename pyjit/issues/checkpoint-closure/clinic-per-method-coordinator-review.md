# Selective Clinic value methods: coordinator peer review

The proposed explicit per-function marker is appropriate for Decimal's mixed
method inventory. It reuses the existing value calling convention and gives
the parser and renderer one selection predicate. File-wide `value_core`
remains a separate declaration about scalar readers, not method signatures.
Implementation is approved with these requirements:

- A clone must own its selection and converter state. Resetting only the
  function flag is insufficient if rendering its decorated parent has already
  mutated shared converters. Exercise decorated-to-object and object-to-value
  clones, including aliases, without inherited flags or converter state.
- Reject unsupported groups, accessors, slots, converters and limited API
  before changing converter state. An explicit request must fail rather than
  quietly emit an object wrapper.
- Aggregate guards across the actual generated destinations and aliases;
  retain the stronger existing whole-file guard. Compile both accepted core
  contexts and a rejected external context.
- Verify concrete self and defining-class positions, optional Context
  arguments, defaults, method flags, return ownership and error exits against
  the existing trampolines. The Python signature stays unchanged.

First deliver the generator, stable regenerated fixture and focused positive
and negative tests for independent source review. Annotate the 65 Decimal
macro-family methods only afterward, one coherent family at a time. Slots and
constructors remain separate work. This review authorizes implementation; it
does not certify generated source or the Decimal migration.
