# Read-only property accessors

Apply this additive fix after `w5-property-value-fields.patch` and
`w5-property-value-fields-review-fixes.patch`. The value-aware getsets for
`fget`, `fset`, and `fdel` previously had no setter callback. That changed the
upstream `AttributeError("readonly attribute")` raised by the old read-only
member descriptors into the generic getset error. The explicit value setter
restores the old message for assignment and deletion while leaving the fields
read-only.

The regression extends the existing `test_descr` read-only property check to
assert deletion raises the same error. Source diff is confined to
`Objects/descrobject.c` and `Lib/test/test_descr.py`. The isolated source has
not been compiled or run; the coordinator owns the merged `test_descr` and
release/debug validation.
