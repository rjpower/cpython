# Interim RC integration

- Stage: accepted and integrated for the declared ordinary supported profile
- Owner: coordinator; no active implementation assignment
- Decision: tracing-GC work closed and RC with cyclic collection selected

The selected RC successor was integrated locally and fresh release/debug images
built from `eba351fee7d`. Each passed 42 integration methods with zero skips.
[Independent outcome](main-outcome-review-20261101-v1.md) and
[integration decision](integration-decision-20261101-v1.json) record acceptance.
Full-suite, extension, packaging, regeneration and safety evidence retain their
actual scope in the linked [historical record](history/status-before-wiki-reset-20261102.md).

Profile: Linux x86-64/GIL, GCC computed dispatch, boxed-slot shapes, private
`boilrc1` ABI. Ten optional modules were unavailable and `_sqlite3` disabled.
Global Py_STACKREF_DEBUG is unsupported and unvalidated. No full-suite performance
parity claim follows from qualification.

These images retain NaN-boxed frame values but boxed heap fields; universal
PyValues were removed during this restoration and are now being
[restored under RC](../pyvalue-rc-restoration/status.md). That successor work is
required, not permanently deferred. The current images contain no new native
JIT. Use the [board](../README.md) for assignments instead of historical pending
steps in the preserved record.
