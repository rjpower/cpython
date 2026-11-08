This packet moves the built-in `property` fields to `PyValue` ownership and
adds value-aware descriptor get/set entries. Constructor arguments, decorator
copies, `__name__`, `__doc__`, `__isabstractmethod__`, descriptor callbacks,
GC traversal/clear, and the property LOAD_ATTR specialization now handle inline
fields without converting them to temporary objects. The object descriptor
functions remain compatibility thunks at the object-slot boundary.

The changed struct has one consumer outside `Objects/descrobject.c`:
`Python/specialize.c` reads `prop_get` for the LOAD_ATTR_PROPERTY fast path. It
now checks for an object-valued exact function before taking a typed view.
`rg prop_(get|set|del|doc|name) --glob '*.[ch]'` in the source tree confirmed
there are no other direct field users.

The base patch's `test_value_fields_and_reentrant_getter` covers inline values
in all five fields, decorator-copy fallback, and a getter that reinitializes
its property during a callback. The additive review-fix patch adds deletion
checks for `__doc__` and `__name__`. Existing `Lib/test/test_property.py`
covers subclass constructors, setter/deleter, and abstract-method behavior.

Apply `w5-property-value-fields.patch` first, then
`w5-property-value-fields-review-fixes.patch`. Keep the original patch and the
review-fix delta separate so the first review remains reproducible.

Validation so far: `check_unit.sh build/checkpoint-bulk-build-20261105
Objects/descrobject.c` and the corresponding `Python/specialize.c` syntax-only
compile both passed for the base patch; the D build's Makefile is now empty, so
the final additive changes could not be rechecked with that compiler. `git
diff --check` on the source is clean. No runtime test was run for this packet;
the coordinator owns the merged release/debug test run.
