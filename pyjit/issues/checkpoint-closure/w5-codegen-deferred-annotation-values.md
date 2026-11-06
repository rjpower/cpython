# Deferred annotation compiler/list values

The captured codegen box is `Python/codegen.c:codegen_deferred_annotations_body`.
The producer stores `PyLong_FromVoidPtr(stmt)` values in `deferred_anno`; list
insertion may unbox pointer-sized integers into inline values. The consumer
previously boxed that owner again before `_PyValue_IntAsVoidPtr`. It now converts
the owned `PyValue` directly and closes it. Conditional annotation indices
also remain borrowed values through a new `_PyCompile_AddConstValue` and
`codegen_addop_load_const_value` path, rather than projecting inline integer
indices back to `PyObject *`.

Patch baseline: isolated source
`build/checkpoint-bulk-20261105` at `e983958bedeca23a192f3a566d2f27b4a0c9a7db`.
The diff was generated with `git diff` and reverse-checked against the edited
source. `Python/compile.o` and `Python/codegen.o` compile in the isolated D
build. Focused compiler tests remain with the coordinator's merged build.
