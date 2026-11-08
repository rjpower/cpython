# H3-1 stage 1 implementation review

Reviewer: `/root/compiler_review`, gpt-6.1-sol, xhigh reasoning.
Date: 2026-10-29. Read-only source/artifact review of `e7cc59743e1`
against `f46276debf3`; no tests, builds or edits. The coordinator supplied
the complete findings below; wording is normalized for this record.

**FIX_FIRST — one narrow blocker in the retained control runner. The
generator refactor itself clears review.**

## Blocker: the reproducer mutates source outside its lock

`controls/H3-1-stage1/run_controls.py:42` writes the broken live generator
before invoking `gate_run.sh`, which acquires the machine slot and build
lock afterward. Restoration at line 48 occurs after the runner releases
that lock. A running gate can therefore observe the variant while the
control waits, and another gate can acquire the lock before restoration.
This contradicts the reproduction guarantee in `H3-1-stage1.md:115`.

Move mutation, testing and restoration into one locked transaction without
nesting the same lock; restore on failure. The recorded quiescent runs and
restored hashes remain valid. This finding concerns the committed
reproducer's concurrency guarantee.

## Remaining audit

The rest of the audit is clean:

- No consumers of the removed analyzer numeric fields or old generator
  signatures remain. The unrelated `dis.Instruction.opcode` use is
  unaffected. Six numeric CLIs build one domain and pass it explicitly;
  IR cases and targets share the product.
- Mappings are copied read-only. Duplicate IDs/names, bounds, metadata
  completeness and base idempotence checks are sound. Base normalization
  remains distinct from specialization kind.
- The absent-LINE reservation correction preserves production IDs and
  fixes minimal DSLs without dummy declarations.
- All seven generated products are byte-identical to the base.
  Independently parsed 264 opcode/pseudo entries, 287 dispatch handlers
  and 302 slots match `baseline-domain.json`.
- Final source/interpreter hashes match provenance. The seven retained
  patches reconstruct the recorded control-source hashes, and their logs
  show the intended failures. All 20 baseline source hashes match
  `f46276debf3`.
- Limits are stated honestly: identity checks do not detect mutation of
  the same AST. This stage proves no runtime widening, pristine
  enforcement, atomic publication, sampler closure or renumbering.

Stage 1 can land independently once the runner is fixed. Stages 2–4
remain required. This was a read-only review; no tests, builds or edits
were performed.
