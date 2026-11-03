# Compiler object metadata readers

Adopt D's critical review of compiler-metadata-reader-cut-v1.md in
build/pyvalue-rc-restoration-design-20261101-v1. Separate ten-minute source cut,
root implements and D reviews: conservative start2026-11-02T02:13:23Z,
concrete checkpoint02:18:23Z, ceiling02:23:23Z. The compiler cache/interner
checkpoint is frozen separately and is not extended.

Only one private borrowed object-kind-checked metadata accessor and the seven
listed compile.c getter replacements. Proven element kinds are compiler-unit
capsules, Unicode names and PySTEntry objects. Preserve actual owning containers
and algorithms; no arbitrary constants, scalar boxes, public getter changes or
new runtime option. Source review plus one full actual compile.c syntax check.
No new behavioral tests for these direct reads; existing compiler tests remain
pending candidate execution. Old native dictionary getters still prevent a
whole compiler/runtime claim, even if this translation unit compiles.

Window closed early. Concrete source checkpoint7e2340ae at02:13:54 UTC meets
the five-minute checkpoint. Independent reviewf3b04a69 accepts exactly the
seven reads/private accessor and verifies the prior cache/interner source is
unchanged. Actual full compile.c syntax passes with real headers; no runtime
test or broader native/dictionary semantics acceptance is claimed.
