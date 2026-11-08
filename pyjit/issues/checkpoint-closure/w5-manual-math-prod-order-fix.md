# Additive `math.prod` finalizer-order fix

Apply `w5-manual-math-prod-order-fix.patch` after `w5-manual-math-fsum-prod.patch`; the original packet remains frozen. The generic multiplication loop now releases the old accumulator before the current input, preserving the prior observable destructor order. A regression test uses finalizers on both values to check the order.

Baseline: main runtime commit `d6eae4b1c1c200444b325240a00791b44288b1e3`, after the frozen fsum/prod packet. W5 release rebuilt successfully; the focused `test_prod` passed. D compilation of `Modules/mathmodule.o` passed. `git diff --check` is clean.
