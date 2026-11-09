# Clinic per-method value emission proposal

Decimal needs selective value-signature wrappers before its entire C unit can
be marked value-native. The existing file-wide `value_emission` is too broad:
it switches every eligible function, including concrete-only and unaudited
entry points. This proposal adds an explicit per-function `@value_function`
Clinic decorator. It changes generated internal C signatures and method flags,
not the Python signature.

- Add a `value_function: bool = False` field to `Function`; add corresponding
  state reset and `at_value_function` in `DSLParser`. Set the field when
  constructing a new function. Reject repeated decorator use, limited-API
  output, accessors, slots, legacy optional groups and converters lacking a
  value form; fail generation rather than silently emitting an object wrapper.
- In `parse_cloned_function`, override the cloned flag with the decorator on
  the clone, even when the original has the decorator. Each method is an
  independently reviewed opt-in; an undecorated alias retains object emission.
  Parameter and return converter cloning and the Python-visible signature
  otherwise remain unchanged.
- Make the single `is_value_function(f, codegen)` predicate accept either
  file-wide emission or the explicit flag. Both `CRenderData.value_emission`
  in `clanguage.py` and `ParseArgsCodeGen` already call it, keeping wrapper
  flag, parser, object-argument conversion and return conversion in agreement.
  Place unsupported-form validation where parameter groups and converters are
  fully known and before rendering, and test both predicate call paths.
- Keep the separately proposed file-wide `value_core` property independent:
  object wrappers in a value-native translation unit use value scalar readers;
  it does not select a value method. The explicit decorator works without
  `value_core`, which allows incremental Decimal migration.
- Emit the existing `METH_VALUE` calling convention and value impl signatures;
  do not add projection wrappers or special Decimal semantics in Clinic. Each
  annotated method implementation and its conversion macros must be ported in
  the same reviewed increment. Concrete receiver projection is allowed only
  after its established type invariant is checked; polymorphic arguments stay
  values. Preserve default argument handling and return ownership.

Generator tests must cover a decorated callable, an undecorated neighbor, a
mixed decorated/undecorated clone pair in both directions, decorated class
methods/defining-class parameters, positional and keyword defaults, numeric
and object converters, value and object returns, and rejection of unsupported
forms and limited API. Regenerate a small fixture twice for stable output and
compile it. Then migrate one Decimal family at a time with focused behavior
and accounting tests before annotating further families. Existing file-wide
`value_emission` behavior and generated fixtures must remain unchanged.

Generated-header constraint: `app.py` currently emits
`VALUE_EMISSION_GUARD` only for whole-file emission, requiring
`Py_VALUE_CORE`. Mixed per-method output is an intentional intermediate in a
core build and cannot receive that whole-file guard yet. Require the explicit
decorator to generate the necessary internal value-call includes and be usable
only in `Py_BUILD_CORE` or `Py_BUILD_CORE_MODULE` contexts; add a distinct
compile-time guard for that condition, emitted when at least one decorated
method appears in the destination. Header emission must aggregate the flag
across all blocks and destinations, including aliases, and preserve the
existing stronger guard for file-wide emission. Test both positive and
negative guard compilation, including files with no decorated methods.

Review narrowed the initial implementation to a core-only source with at most
one generated file destination. Clinic's include set is shared across file
destinations, so all generated headers receive the core guard before includes
and a second file destination is rejected before the output-writing loop.
Individual value fragments also carry the guard when routed through buffers or
dumped inline. Decimal uses the default single file destination. Per-destination
include tracking can be designed separately if a future use needs it.
