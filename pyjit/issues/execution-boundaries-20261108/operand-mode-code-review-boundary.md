# Operand-mode implementation source review

Reviewed the actual operand-modes-20261108 diff on 2026-11-08, including producer,
generated shape reader, specialization/feedback changes and new tests. No native
validation by this reviewer; coordinator release/debug results remain required.

The descriptor inherits the actual LOAD_ATTR_SHAPE instruction and format. Its
input mode is restricted to a borrowable local field, output mode to the actual
destination. Generated consumption becomes NULL, canonical home duplication
remains, and output publication is unconditional to the temporary. The receiver
release suffix receives NULL; no displaced local exists. Type/shape checks and
fallback remain before result creation. Canonical opcode/stats and full handler
metadata are separate, and both saved feedback rows and new row comparisons
normalize the physical alias before the multiple-shape policy consumes them.

No blocking source finding remains from the design review. Runtime tests cover
selection, excluded modes, object aliasing/result reachability, descriptor and
shape failure, feedback shape history, trace activation, inherited contract and
negative mode declarations. Host generator checks were reported by the worker;
their output is component evidence only. Native compile/runtime and compiled
instruction/text comparisons are still admission gates. Keep the final merged
boundary/operand generator review explicit because both modify handler aliasing.

## Native-failure repair source review

Reviewed the incremental family applicability filter, stable-version shape
history fixture, and adaptive disassembly repair. The descriptor filter now
requires a declared family, so synthetic generator inputs that reuse a handler
name without its actual operand contract do not receive that variant. Negative
mode tests still operate on the real full-source family. The shape test creates
both shared-key shapes before warming, keeping its type version stable when it
switches objects. The adaptive reader uses an execution-only alias's canonical
specialization shadow, while structural IR_JUMP keeps generic reconstruction.
Original opcache expectations are restored. These address source causes rather
than weakening expectations. No blocking source finding; coordinator native
rerun remains required, and generated handler composition needs merged review.
