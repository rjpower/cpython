# Property value fields: composite source review

Reviewed `w5-property-value-fields.patch` followed by `w5-property-value-fields-review-fixes.patch`. **Source review approved for merged qualification.** The additive patch closes the object-slot getter argument before converting its independently owned result; both value getset setters now accept the NULL deletion sentinel with `PyValue_XDUP`. It also preserves the original decorator-copy behavior when the old property's name is absent: a subclass constructor's new name is cleared. The added test exercises deletion of `__name__` and `__doc__`.

The remaining value callbacks snapshot stored fields before user calls; setter publication precedes closing the old owner; `property_copy` holds source fields through the subclass constructor; traversal and clearing visit each value home; and the LOAD_ATTR fast path projects only an object-valued exact function. The `METH_VALUE|METH_O` methods use the existing checked descriptor trampoline.

The original two findings remain recorded in `property-value-fields-review-round1.md`. Base C units passed syntax checks, but the additive patch has no fresh compile or runtime receipt because its build Makefile was unavailable. The coordinator must validate the composite in merged release and debug builds, including `test_property` and GC/reentrant behavior, before integration approval.
