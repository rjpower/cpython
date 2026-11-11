# Option1 proposal review

Approved for the isolated implementation, subject to the conditions below.
Reviewed the proposal against bf810 feedback layout, `fb_row_of`, inline-values
addressing, specialization offsets and type-layout mutation rules. An exact,
nonzero receiver type-version guard is a suitable witness for that type's fixed
positive inline-values displacement. This removes a redundant metadata load
without changing Python lookup or storage semantics.

- Keep the type-version guard before every cached-offset dereference, including
  generated local/temporary variants. Do not use this witness for module,
  slot/descriptor, managed-dict pointer or other layouts. Debug assertions may
  check current type flags/basicsize; release helpers must not reload them.
- Check the complete `tp_basicsize` and complete attribute displacement before
  narrowing. Preserve atomic validity reads and existing free-threaded guards.
  There must be no callback between the type witness and address use.
- Preserve 48-byte ATTR, 16-byte payload and 120-byte poly geometry. The first
  payload union may hold `{shape_to, values_offset}`; guarded `class_value`
  remains distinct. Method WITH_VALUES snapshots require both their offset
  scalar and guarded class value. Other recorded handlers must never interpret
  pointer payload bytes as this scalar pair.
- Audit all feedback snapshot, restore/clone, poly selection and test/debug
  readers. Initialize the new field on every eligible nomination; zeroed rows
  alone do not prove that each active cache writer fills it. Shape-add keeps
  both transition shape and displacement.
- Extend canonical-output helper recognition only for the positively reviewed
  nonescaping helper contract. Do not waive existing source-body proofs by
  opcode name. Keep other output/recovery checks unchanged.

The proposed mutation, polymorphism and layout tests discriminate the failure
modes. Assert specialization before invalidating it. Add mixed method/ordinary
shape traffic where practical to catch payload misinterpretation. Root must
regenerate, compile release/debug, execute focused tests and inspect release
native output before admission. This proposal review establishes no speedup or
executed qualification. Broad shape/store coverage is acceptable within the
same helper and geometry; use the proposal's scope cut if that becomes unclear.
