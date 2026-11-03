# Independent RC closure source review

2026-11-01. **ACCEPT for separately reviewed first native feedback.** Exact
clean source `d7df12b1d81722927e71d9734c70eeafdccfee73`, parent
`1440f8c6fd1be82bc02028df5728c3cb3ad0c81a`, was reviewed in
`build/worktrees/rc-successor`. Capsule `build/rc-closure/capsule.json` SHA256
is `a3273d5c45eb0dcf689b0b9cfae8e986d0e9eb5250659980c655bea1c5b1a2eb`.
No concrete source blocker remains. This clears source readiness, with no
execution, adoption, parity, landing or platform qualification credit.

The scope is the ownership closure on the restored interpreter, rather than a
second review of all 887 restoration paths. I read the projection algorithm,
independently replayed it against the exact parent, and read the entire remaining
33-path policy delta. The replay matches all original/projected/current hashes:
69 files undergo owner-conditional projection; 50 of the 83 modified/deleted paths
are exactly its output. The remaining 33 match the retained policy patch byte
for byte. The added RC test module and two issue notes are separately bound.
Windows XML comparison respects the actual checked-out CRLF policy. Four
additional nested/mixed-branch fixtures preserve unrelated conditional choices.
All 86 source-path bindings, 22 capsule artifacts and the source grant rehash;
the original preparation failures remain intact.

`Py_GC` alternatives resolve to the RC bodies. Public headers reject an external
definition, so it cannot silently turn reference operations off. Debug builds
again imply reference debugging. Real INCREF/DECREF and mortal close operations
reach `_Py_Dealloc`; inline scalars and borrowed references retain their existing
ownership distinctions. The live owning collector source and its Unix/Windows
build entries are removed. The GIL cyclic collector remains; the alternate
free-threaded implementation is preprocessor-excluded in the proposed first
configuration. The surviving tracing verifier diagnoses reachability after that
collector and does not become a second owner. Root registration acquires no
reference; its public contract now requires ordinary ownership and timely
unregistration.

The compiler and generator retain their RC-capable ownership machinery. I checked
owned/borrowed stack-reference creation, stealing, duplication and closing;
replacement publishes a new local or return value before closing the previous
owner. The frame temporary-cleanup helper nulls the published home before closing
the saved reference. Generated container windows clear homes after ownership transfer or
explicit close, rather than using clearing as a replacement for decref. Return
and yield make borrowed results heap-safe. Frame copying transfers owned slots
and promotes borrows before suspension; generator destruction retains finalizer
resurrection, weakref, origin, frame, exception and code cleanup. These source
relationships support focused execution, not a proof that every inherited
compiler transformation is correct under RC.

ABI isolation is internally consistent for the proposed Linux configuration:
SOABI and debug ALT_SOABI use `boilrc1-`, the cache tag contains `-rc1-`, and
bytecode magic is 64401. POSIX/Cygwin automatic extension discovery excludes
upstream abi3 and generic untagged suffixes. Renaming or explicitly loading an
incompatible binary remains unsafe; this is not a general binary compatibility
guarantee. Windows suffix/loader isolation is explicitly unqualified. The manual
generated-configure counterpart agrees with the reviewed configure.ac changes
and passes shell syntax checking, but canonical autoconf/autoheader regeneration
is still required before landing.

Independent host replay regenerated both interpreter products exactly and ran all
26 existing generator methods successfully. Changed ordinary Python test modules
compile on the host. The inherited support module contains Python 3.16 lazy
import syntax, so its complete grammar remains a native gate; the changed
metadata statements were checked separately. The four new pending methods cover
immediate finalizer/weakref release with cyclic GC disabled, reentrant replacement,
actual container reference counts and cache/extension namespace isolation. The
17 deleted methods belonged to the owner-only class that RC previously skipped.

First feedback must use a fresh matched Linux/GIL/mimalloc/computed debug image
with the declared boxed-shapes configuration, the four new methods and existing
RC/cycle/weakref/finalization plus interpreter/frame/generator/coroutine controls.
Its helpers must bind the actual historical APIs: `gc_verify` returns holes (or
None), `gc_verify_holes` returns the last holes, and `gc_verify_count` counts
verifications. Current tracing `gc_verify_mode` or rootstats interfaces are absent
and must not be invented. Full extension, embedding, reinitialization,
subinterpreter, stackref-debug and port validation remain later qualification.

Reviewer evidence is
[`receipt.json`](../../../build/rc-source-review-20261101-v1/receipt.json)
(repository path `build/rc-source-review-20261101-v1/receipt.json`), SHA256
`448f9300aed4d58361b0cba62c3f9a95f03f7358f9fd809411b7ffd7f4d59e9e`.
The replay script SHA256 is
`80bbb1d4de2af19fd9e89d92085db8450f262789d6e953e274bc5db07a0ea193`.
All reviewer outputs are separate from the frozen author capsule. No compiler,
Boil target, workload, counter probe or timing ran.
