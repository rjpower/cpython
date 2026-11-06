# Long range iterator value fields: source post-review

Reviewed `range-longiterator-values.diff` against the pre-port `Objects/rangeobject.c`. No must-fix or should-fix finding.

The three fields are owned values at every constructor. The forward path duplicates the range's three owners; the reverse path initializes all fields to null before fallible arithmetic and transfers each successful result. Deallocation tolerates partial initialization; traversal visits only object-valued fields. The iterator step publishes the new start and length before closing the old length, returns the old start as the next owned item, and preserves the null-with-error versus null-with-exhaustion distinction. `__setstate__` retains its exact-int guard and clipping order, computes into temporary owners, and publishes both updated fields only after all operations succeed. `__reduce__` holds the start and step through arithmetic and transfers owned copies to `make_range_object` under its existing contract.

Producer reports W5 all-build, `test_range` (29 tests), and direct mixed inline/large, reverse, pickle, exact-int state, and clipping probes passed. This review did not independently rerun those tests.
