# W5 manual PyValue iterator producer packet

This packet ports six `itertools` iterator implementations to paired object and
PyValue entry points in `Modules/itertoolsmodule.c`: `batched`, `cycle`,
`dropwhile`, `islice`, `compress`, and `filterfalse`. The object `tp_iternext`
entries are compatibility thunks; each type also installs its PyValue
`_Py_value_iternext` implementation.

The value implementations keep yielded values owned through the consumer,
close skipped or discarded values, and preserve exhaustion and error status.
`batched` retains strict incomplete-batch behavior and batch-size state;
`cycle` retains its saved-list replay index and only clears source exhaustion;
`islice` retains skip/stop counters; `compress` keeps data-before-selector
consumption and closes a datum when no result can be returned. `dropwhile` and
`filterfalse` now keep their predicates in a `PyValue` field, including value
aware traversal and deallocation. This file has no separate reduction hook for
these objects.

The patch is apply-ready against `pyvalue-rc-universal` HEAD
`d6eae4b1c1c`:

- `w5-manual-itertools-six-iterators.patch`
- SHA-256: `ef8988a7b4a8a442b647068b1bf34fbce6ff9bb9111628312530026829b0a123`

Validation used the S source at `build/checkpoint-bulk-20261105` and its paired
D build at `build/checkpoint-bulk-build-20261105`. `make -j2
Modules/itertoolsmodule.o` compiled the changed translation unit. A locked W5
release overlay rebuilt successfully, imported all 117 configured modules, and
ran `./python -m test test_itertools`: 140 tests passed, 5 skipped. The W5
source was restored and its release build returned to the original source after
the run. `git apply --check` against the stated integration HEAD passed.
