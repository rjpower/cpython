# Manual `math.integer` GCD and LCM value ports

Patch: `w5-manual-math-integer-gcd-lcm.patch`

This packet keeps converted operands and intermediates owned as `PyValue`s in `math.gcd` and `math.lcm`, then uses the exact-base `_PyLong_ValueGCD` provider and value arithmetic. Zero/one fast paths preserve the existing result behavior. Ownership is closed or transferred along each error path.

Baseline: main runtime commit `d6eae4b1c1c200444b325240a00791b44288b1e3`. The patch applies cleanly to that commit. It includes the `_PyLong_ValueGCD` declaration; the current integration worktree already contains the identical declaration from the provider packet, so deduplicate that one line when applying there. `Modules/mathintegermodule.c` also has a separate in-progress `isqrt` change in the integration worktree; retain it during integration.

Validation: D compiled `Modules/mathintegermodule.o`. Focused `test_gcd` and `test_lcm` passed in the W5 release build. `git diff --check` reported no whitespace errors.
