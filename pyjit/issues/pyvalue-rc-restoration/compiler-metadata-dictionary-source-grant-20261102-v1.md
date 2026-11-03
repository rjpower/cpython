# Canonical compiler dictionary consumers

Root implements design6a0239cf in
build/pyvalue-rc-restoration-design-20261101-v1/compiler-metadata-dict-cut-v1.md;
D's independent design review2ac96274 accepts this finite cut.
Conservative start2026-11-02T03:40:07Z, concrete checkpoint03:55:07Z,
ceiling04:25:07Z. Record actual first edit and preserve earlier accepted bodies.

Only compile.c dictbytype/add_cell_names_from_symbols/dict_lookup_arg,
assemble.c dict_keys_inorder/compute_localsplus_info, flowgraph.c
build_cellfixedoffsets; necessary includes and a focused new test file.
Use owned canonical dictionary reads/iteration and direct inline integer
conversion, retaining heap owners through PyLong conversion. Preserve long→int
policy in dict_lookup_arg, cursor/group boundaries and dropped-cell offsets.
All branch cleanup preserves primary errors and releases each snapshot once.

Validate current dictionary size after tuple/offset allocation and bounds before
stores; do not rely only on an old size/assert across allocation. Keys have
proven exact Unicode producers, with explicit owners through callbacks and
publication. Initialize/transfer tuple slots using accepted canonical contracts.
No provider/API/legacy behavior change, symtable/codegen/marshal/AST/type/cache
edit, or claim that this closes all compiler consumers.

Review actual source/ownership and focused tests, then at most three actual
changed syntax TUs with real headers/30seconds each. Retain unrelated failures.
No object/link/candidate/runtime/timing. Prior fixtures remain unchanged.

Window closed early after checkpoint61a1d727 at03:46:25.540899 passes
independent source review39a69223. First source edit lies after the baseline
capture03:41:32 and no later than03:44:40; the earlier conservative03:40:07
clock governs the window. All other source is exact after masking six bodies
and two includes. Full actual compile/assemble/flowgraph syntax passes.
Four new fixture expectations pass on the host reference interpreter only;
candidate execution remains pending. No whole compiler/runtime acceptance.
