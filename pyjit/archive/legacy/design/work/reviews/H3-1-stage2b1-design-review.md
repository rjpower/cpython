# H3-1 stage2b.1 design review

2026-10-29. Read-only review against native624c75f6455 of design
H3-1-stage2b.md at SHA256
51e579d063f5f78329cc5fde7661658567b28e3079bec52f37770edb73962afa.
Verdict: IMPLEMENT_2B1, section2 only. No edits, tests or builds performed.

The full-domain numeric-index name table fixes the generator's actual
omission. The minimal IR_BACON oracle distinguishes that failure from the
current repaired split lookup. Immutable per-NB operations and generated
flag/base/operation tuples support separate exact-NB operand decoding and
adaptive specialized identity versus semantic-base display. Nonadaptive
display must continue normalizing pristine inline specializations.

HandlerFor must keep its pristine-base match first. Generated NOFB/layout
properties replace only the numeric range predicate. Actual installation
continues through _PyIR_SetHandler, preserving the upper48 bits. The
copied-NOFB control must reach an instrumented NOFB twin transition, then
restore the exact pristine ID and all A–D fields. Generic-base restoration
alone would miss that branch. These are implementation/gate obligations,
not execution evidence.

The separate get_ir name-selector hunk still requires coordinator/ROOTS
ownership approval. No2b.2–4 or native integration clearance is implied.
The asynchronous line-reader lifetime prerequisite remains separate.
Carry Python/ir.c's per-NB range/subtraction consumers into2b.4 semantic
closure, alongside the Python consumers moved in2b.1.
