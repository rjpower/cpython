# Canonical design documentation sync

2026-11-07. Luna medium. Updated the register-IR and call/frame design pages
for R5 at `0953757f4e7` on R4 `b58036e9718`, keeping the `7780e32e0e2`
qualification and timing evidence historical. The pages record the checked
canonical-source fact, its first-temporary-flush scope, the writer-side
canonical-home precondition, and the manual source proof behind that
precondition. Folded-local publication, callback/error ordering, and companion
outputs retain their existing paths. Links point to the proposal, RHS
refinement, implementation review, and cold review.

R5 release/debug builds each checked 117 modules; each six-file focused run
passed 630 tests with one skip, and API lint covered 306 units with no failing
findings. Full stats/build and release/debug suite qualification later passed;
both full profiles ran 54,380 tests with zero failures.
Paired shape microtargets improve while folded-local controls regress; the
basic screen is mixed, with no broad win or parity claim. Native JIT recovery
remains planned. No runtime edits or tests were made in this documentation
sync.
