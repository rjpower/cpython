# Private universal-value ABI namespace

- Stage: independent review accepted; source committed as `359d10f4d4d704aa24aedbda598107724b707813`
- Owner: coordinator; wiki_audit reviews
- Scope: configure.ac, generated configure, sysmodule cache tag and two existing tests

Use `boilvalrc1` for SOABI/ALT_SOABI and `valrc1` in cache tags. Ordinary extension
discovery and bytecode caches must not reuse interim RC artifacts after the
layout change. This is a rebuild boundary; the eventual opaque extension API
remains separate. No bytecode wire change is needed.

The repository-pinned cached autoconf container regenerated configure with
exactly the two intended prefix replacements. All five files match the expected
string-only transforms. Shell syntax, test AST parsing and diff checks passed.
The actual sysmodule TU still reports two preexisting removed structseq-read
calls; the unchanged baseline reproduces both. These are queued as a separate
consumer port. Runtime namespace tests await the coherent candidate.

[Design, patch, hashes and checks](../../../../build/pyvalue-abi-namespace-20261102-v1/design.md)
record source evidence; no linked candidate or runtime acceptance is claimed.

[Independent review](../../../../build/pyvalue-abi-namespace-20261102-v1/independent-review.md)
and [commit receipt](../../../../build/pyvalue-abi-namespace-20261102-v1/commit.json).
