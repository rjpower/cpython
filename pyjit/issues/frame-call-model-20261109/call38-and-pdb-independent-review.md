# CALL/POP_TOP cold-query post-review and Pdb position-test peer

The final source38 diff in the runtime checkout contains only
`Include/internal/pycore_ir_outcomes.h` and `Lib/test/test_ir_contract.py`.
I accept it for the bounded normal-transfer contract. The CALL branch checks
the generated family, roles, operand count, extent, and destination geometry
before marking consumed window homes NULL; it excludes the result home and
returns UNKNOWN outside the window. The POP_TOP branch requires the exact
scalar scratch handler, marks its consumed A NULL, and preserves other scratch
homes on the normal successor. The generated POP_TOP consumption clears that
home before closing the detached owner. These facts do not describe error or
observer successors.

The recorded `call38-v3-minimal` receipts bind the same two-file diff and exit
successfully: 137 tests in each role, with one skip. `call38-originals-v2`
records four original trace-jump methods passing in each role. The
`call38-frame-observer` receipts also bind the same diff and exit successfully:
860 tests across 14 modules in each role, with two skips. This is focused
source and executed-result acceptance, not a full-suite or performance claim.

I accept `pdb-gh127321-position-test-plan38.md` for a test-only implementation.
The original doctest body stays intact; its expected entry line should be the
call expression. `Doc/library/pdb.rst` says `set_trace` enters immediately
since 3.13. The proposed companion checks a real Pdb interaction for both
backends, maps its recorded `f_lasti` to one public `dis` position on that line,
then continues and observes one binding and the expected return. The existing
`Pdb(backend=...)`, `interaction`, and `stop_trace` APIs support that shape.
The test must restore the previous tracing state and clear only its own cached
Pdb instance. Native results for this proposed test remain pending.

Coroutine NULL ingress is a separate design decision. A created coroutine can
throw at `RETURN_GENERATOR` pc0, and constructor validation permits an EH range
there. A worklist seeded only at pc1 would miss that handler. The first scope
should refuse an ordinary EH target at pc0 or a source-error record from pc0;
it should seed pc1 without inventing a normal pc0-to-pc1 transfer.
