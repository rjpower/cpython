# Accounting normalization correction

Apply `r27-accounting-normalization-v2.patch` after the frozen `r27-accounting-exact-shapes.patch`. It narrows path normalization to the first exact build product in an argv array and an explicit `-X pystats_dir=...` option before `-c`, `-m`, or `--`. Other arguments stay byte-for-byte exact, including source paths, arbitrary temporary paths, and code payloads that happen to contain a build or dump path.

The v1 normalizer could replace a matching path prefix in a `-c` payload. This correction prevents an unrelated child command from matching a reviewed launch rule through that substitution. An additional negative fixture covers payload and argument-position cases. The isolated checker suite passes 40 tests with one skip under `uv run --offline` using the configured shared cache. The additive patch applies cleanly after v1 in a scratch preimage.
