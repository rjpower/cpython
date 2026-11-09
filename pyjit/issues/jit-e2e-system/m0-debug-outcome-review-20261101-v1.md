# M0 actual feedback outcome

FAILED at the first debug build; no semantic test ran. Root observed supervisor 78057 exit 1 at 17:00:19 UTC. Configure returned 0; make returned 2 while compiling ceval.c. The generated header places `#if ENABLE_SPECIALIZATION` after `assert(oparg <= NB_OPARG_LAST);` on physical line 98, with the same defect repeated in operation-specific BINARY_OP helpers. The stray directive causes missing-label and unmatched-endif diagnostics. This is a real generator/C syntax defect; host generator checks did not establish C validity.

All eight selected runtime methods, helper CALL/RETURN/error behavior and later M1 work remain unvalidated by this batch. No successful interpreter image, native JIT, elapsed or qualification credit is claimed. The original source, raw compiler diagnostics, partial products and first-failure receipt remain preserved.

Independent audit checked source/input/config/tool bindings and all retained partial products. Row cleanup was before deadline and idle. Root additive cleanup at 17:00:30 UTC verified all 20 locks, native idle and 30 unchanged parent files, then removed only the owned empty m0d1 cache.

A possible narrow successor is to start a physical output line before emitting the specializer-end preprocessor directive, regenerate the header, and add a generated-output directive-line invariant. That is a proposed repair, not authority to edit or rerun.
