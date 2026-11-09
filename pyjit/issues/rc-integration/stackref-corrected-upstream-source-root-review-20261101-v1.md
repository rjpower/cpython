# Exact corrected upstream control source review

2026-11-01. **ACCEPT source d2ca4122333cf89ef97dcb4904cafd61a02308c7
for the bounded compatibility control only.** Its parent is pristine
321925c92c9e67864cafa1566840927f6f07e90a. This is not an RC source change,
successful build, checker result or native activation.

Independent root replay checks the complete tracked tree entries, modes,
working source and the author's capsule/proof. The only delta is one assignment
left-hand side in Python/frame.c, one insertion and one deletion:
`new_frame->f_executable` becomes `frame->f_executable`. Candidate bytes equal
the parent bytes with that single replacement. All other tracked objects and
modes agree. The source checkout is clean and the original external upstream
checkout remains clean at321925.

The three complete standard test files and their selected method bodies, plus
Programs/_testembed.c, remain identical to pristine upstream. No test,
MakeHeapSafe, token check, close, report, finalization, GIL rule or build flag
changes. The source assignment agrees with the already selected RC behavior.
The debug MakeHeapSafe close/create and later source clear support the design's
token-ownership hypothesis; this does not attribute fatal ID55984 without a
dynamic creation trace.

Author source capsule644f0c57212220cb8d73d4ca3a03cecb675d4daebea65fde826455f1ba6f820d
and exact patch37973cdd24cc931d381a2b9a7562e44e6203a9c7c485c215307a347a1fe82ffc
are independently bound in the root replay receipt under
`build/upstream-stackref-corrected-source-root-review-20261101-v1/`.
The receipt supplies actual UTC, tree-entry counts and verified hashes.

New held helpers must bind this exact source and the accepted design, preserve
the failed pristine attempt, include bootstrap/freezer cleanup and retain the
same three-method comparison. Root helper review and one future fixed1800-second
activation remain required. No further upstream fix is authorized if it fails.
