# Actual C marker byte round trip

On the frozen `af82c03c4e5` stats build, the coordinator compared the marker
of a live local child with its exact launch vector. UTF-8 text, an invalid
filesystem byte restored with surrogateescape, newline, space and empty
arguments all match after the checker decodes the C marker. There are no
accounting parse errors in that probe.

The first harness attempt lacked the tool's import path. The next attempt
expected `os._exit` to leave a marker, but this fork intentionally dumps stats
before `_exit` and removes it. Both attempts are retained; neither is a
runtime correctness failure or a passing marker check. The final probe reads
the marker while the child is alive, then kills and waits for that child.

Evidence: external build root
`pyjit-link-wave1/checkpoint-closure/marker-bytes-af82-v3/receipt.json` and
its marker. This verifies serialization and decoding together. It does not
establish complete full-suite subprocess accounting.
