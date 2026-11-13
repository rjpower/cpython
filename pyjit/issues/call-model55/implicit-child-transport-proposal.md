# Normal transport through an independent child

The LOAD_ATTR family loses its normal consumed-input receipt because its
property and overridden-getattribute members enter Python children. Those
children use `_PyFrame_PushUnchecked` at the current arena top, rather than
reusing the parent's scratch. Their normal return can therefore preserve
unrelated parent homes and consume a scratch owner even when the recipient is
a named local. This is a normal-edge proof, independent of error handling.

The selected source certificate checks the actual constructor and transport:
read-only function/version/capacity setup, one independent reservation, an
actual `TAKE(owner)` into child argument zero, the optional immutable name
argument, and the wrapped child. It checks the complete shared recipient and
push AST, including preprocessing structure. The emitter records success only
when the actual `SAVE_STACK` has drained the original input ownership map.
Unknown setup, extra parent/child writes, aliases or changed publication lose
the certificate. Instruction names confer no authority.

The resulting NORMAL_CLEAR event uses bit 2, DISJOINT_CHILD. Bit 1 remains
WINDOW_RESULT and every result test is bit-specific. Physical normal transfer
preserves an unrelated parent home through PUSHES_FRAME only with this sole
checked event. Actual input and output recipients remain subject to the
ordinary transfer rules; consumed input absence is installed only when that
same home is not an output. A missing family recipe removes flagged receipts
as well as unflagged ones. No C handler, call layout, frame header or runtime
guard changes.

The five-file packet is in the pins lane: pycore_ir.h,
pycore_ir_outcomes.h, ir_generator.py, test_generated_cases.py and the new
test_ir_disjoint_child_transfer.py. Tests cover source perturbations, emitted
receipts and family revocation, local recipients across joins, untouched
sibling CALL_EX holes, output alias rejection, parent binding mutation during
the child and an exceptional child route. Root owns generation and native
validation. The parser-only source check, Python AST checks and diff check
pass; these are not runtime evidence.

The shared failure appears in stdlib and test entry images. For example,
test_exceptions consumes scratch home 5 into local 2 before a later CALL uses
home 5 as its absent-self slot; test_buffer instead needs a sibling absent
home retained across LOAD_ATTR. Both depend on the same family-closed
independent-child proof. There are no module-specific exceptions.

Root, GC and IR give actual source GO. The isolated five-file commit is
5f2d245b96d; the pins lane is clean. Emitted receipts, native compilation and
runtime behavior remain coordinator validation gates.
