# Iterator producer review fixes

This is a separate additive patch to `w5-manual-itertools-six-iterators.patch`.
It preserves the first packet and corrects two behaviors found in post-review:

- `dropwhile`, `islice`, `compress` (both input streams), and `filterfalse`
  now use `_PyValue_IterStep` so a raw iterator's `StopIteration` payload is
  propagated, matching their prior direct `tp_iternext` behavior. `islice`
  transfers a returned payload instead of closing it.
- `batched` returns an error if resizing a partial tuple fails.

The regression test checks payloads from `dropwhile`, `islice`, both
`compress` streams, and `filterfalse`.

Apply the six-producer packet first, then this patch. The chain applied cleanly
against `pyvalue-rc-universal` HEAD `d6eae4b1c1c` and reproduced the S source
files byte-for-byte.

The first W5 attempt was blocked by an in-progress Decimal compile error and
ran no tests. After that source fix completed, a locked W5 release overlay
rebuilt successfully, checked all 117 configured modules, and ran
`test_itertools`: 141 tests passed, 5 skipped. The W5 source was restored and
its original-source release build completed before releasing the lock.
