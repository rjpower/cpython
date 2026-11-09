# Accounting marker decoding and argv normalization, round 3

Apply `r27-accounting-normalization-v3.patch` after the frozen v1 and v2 patches. It changes only the checker and focused tests.

The v2 normalizer could rewrite a build-product path that appeared as a shell `-c` payload or another argument. V3 normalizes the executable only when `argv[0]` is an exact known build product; wrapper commands stay exact until separately reviewed. It walks interpreter options, skipping `-X` and `-W` operands, and normalizes only an explicit stats-directory `-X` option before `-c`, `-m`, `--`, or the script name. A preceding unrelated `-X` no longer hides a later stats-directory option.

The C emitter in v1 records raw argv bytes with JSON escapes. V3 decodes each marker argument from its Latin-1 byte representation through `os.fsdecode`, restoring UTF-8 text and surrogateescaped invalid filesystem bytes. A non-list, non-string item, or JSON code point outside the byte range fails accounting. This keeps exact rule matching tied to the actual process arguments.

Negative fixtures cover shell/code payloads containing the build path and malformed marker byte representations. Positive fixtures cover multiple `-X` options, UTF-8 text, and an invalid filesystem byte. The isolated checker suite passes 40 tests with one skip under `uv run --offline` using the configured shared cache. The additive patch passes `git apply --check` against the v2 preimage.
