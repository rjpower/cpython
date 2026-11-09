# Factorial value continuation receipt

Patch `r27-factorial-values.diff` is based on runtime `b9888ea3b40`. W5 release shadow build passed three focused `test_math_integer.IntMathTests` factorial tests, including an independent Python reference for 0–999. `r27-factorial-differential.py` compared full-result SHA-256 digests for 101 deterministic boundary and random inputs up to 1500 against upstream `3a440b65695`; all matched. W5 source and release build were restored. This is worker validation; independent source post-review and merged qualification are separate.
