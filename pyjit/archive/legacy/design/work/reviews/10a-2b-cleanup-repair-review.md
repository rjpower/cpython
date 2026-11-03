# ROOTS cleanup and queue annotation repair review

2026-10-29. Independent read-only review of172b406be0c. The four reviewed
files are unchanged by tooling merge67477efa986. Verdict: LAND for these
bounded repairs. No broader ROOTS2b clearance or O1 adoption is implied.
No tests, builds or edits by the reviewer.

The cleanup reorder respects both dependencies: stop the forkserver before
removing its directory, and drain semaphore finalizers while the resource
tracker can still unregister them. The regression uses the actual helper,
a real semaphore, a restored finalizer whose weakref callback encountered
an empty registry, and an active forkserver/tempdir. It asserts tracker
exit0, forkserver termination, directory removal and absence of the
relevant child diagnostics. The old-helper negative fails both contexts
at exit1, with retained leak/double-unlink evidence. Recorded hashes match.

The opaque exception correctly moves to run_queue_ex, which holds the
saved exception and active call payload. The thin run_queue wrapper has
no managed locals and becomes precise. This transfers the existing
contract; explicit spans and the exception fixture remain rung3c work.

Quiet natural-case passes do not prove clean successful stderr. Visible
reruns are required before that claim; the deterministic regression
checks its child stderr directly.
