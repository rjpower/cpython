# Benchmark consumer review correction

`process-identity-benchmark-consumer-v2.patch` supersedes the original frozen consumer patch; apply it to the original checkout, not after the v1 patch. It retains the same pair and identity checks. Before rejecting a symlink, directory, or unreadable artifact, it writes a journal manifest naming every artifact and recording each readable regular file's exact bytes and digest. It never follows nonregular entries. This closes the review finding that a failed run could lose its only evidence when the benchmark scratch directory is removed.

No active Python consumer of the old stats journal's `source_path` or `text` fields exists in `pyjit/bench` or `pyjit/tools`; the current journal schema now records an `artifacts` list with paths, base64 bytes, and SHA-256 digests. The original patch and receipt remain unchanged as review history.

`uv run --no-project python -m unittest discover -s pyjit/bench/tests -p 'test_*.py'` passed: 27 tests, 2 skipped. Targeted `git diff --check` passed. A real child probe still awaits coordinated integration with the C producer and R27 reader.
