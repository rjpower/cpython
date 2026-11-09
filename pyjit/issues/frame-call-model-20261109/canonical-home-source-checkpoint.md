# Canonical local duplicate: source checkpoint

At runtime parent `417a8bd6e2e3b32dce5ea2f052a4791a6bdb8e79`, the
generator recognizes the exact pure `GETLOCAL(oparg)` → `PyValue_DUP`
source in both `LOAD_FAST` and `LOAD_FAST_BORROW`. It reuses the existing
canonical-result final store, so a duplicate from a canonical frame home
skips redundant numeric publication. The source proof requires a fresh
logical output, matching local/immediate field, one uop, no input owner,
and exact unmodified body tokens. The four numeric-constructor proofs
share this result fact; the observer and terminal generator are unchanged.
The final store still handles a temporary, a folded local (including
source/destination aliasing), or `IR_NO_DST` through the same macro.

Changed source: `Tools/cases_generator/ir_generator.py`,
`Python/ceval_macros.h` (comment), `Lib/test/test_generated_cases.py`,
and `Lib/test/test_ir_canonical_home.py`. Regeneration changes only the
`LOAD_FAST` and `LOAD_FAST_BORROW` case bodies. The other 285 generated
bodies, four existing float constructor bodies, and generated metadata
are unchanged, according to
`canonical-home-generated-review.json` in the retained E11 evidence
directory. The complete reviewed source patch is
`canonical-home-duplication-full.patch`; its test fixture now includes
the actual `FB_HEADER` registration for the synthetic cache-shadow
mutant. The focused runtime test is in
`canonical-home-runtime-test.patch`. The first host check stopped on that
fixture's missing feedback layout before reaching the proof; the corrected
four-method host check passes.

Root's release and debug builds each completed 117 modules. The focused
native gate passed 25 tests in each profile, covering the new local-load
semantics, existing operand modes, discard completion, and the new
generator proof. Evidence:
`canonical-home-{release,debug}-build-checks.json`,
`canonical-home-minimal-{release,debug}.log`, and
`canonical-home-host-checks-v2.log` under
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/frame-call-model-20261109`.
`git diff --check` passed at the source checkpoint.

Retained GCC disassembly reports 134 → 85 static instructions for each
load handler, and two `PyLong_Type` plus two `PyFloat_Type` references
become zero in each. That verifies removal of the duplicate
classification in these compiled handlers. It is not a dynamic speedup
measurement. Component performance and broader regression results remain
to be assessed separately by the integrator.
