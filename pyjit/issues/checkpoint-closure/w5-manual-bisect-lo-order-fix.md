This additive correction preserves the public error order for the value-aware
bisect search entry points. Both left and right search validate `lo` before
examining an inline sequence argument, matching the prior object implementation
and the pure-Python implementation. The regression covers `lo=-1` with the
default `hi` and with `hi=0`, and asserts the established ValueError message.

This packet only repairs search error ordering. The insort entry wrappers still
box their arguments, so the paired-item/search packet does not close all
`_bisect` call sites. The merged integration test run will validate this
additive change.
