# Decimal current-context lifetime review, round 2

The additive `decimal-context-ownership.diff` resolves the three blocking lifetime findings from round 1. `localcontext` owns `global` through context-copy and callback-bearing setters, closes it on each failure, and transfers it to the manager on success. Formatting owns its Context before warnings or fallback callbacks and closes it on every early and normal return. Rich comparison owns the Context through Rational checks/getters and status handling, then closes it before its pure result switch. The cleanup paths preserve the prior result and error behavior; no further blocking source finding in this patch.

The author's focused C Decimal run passed 90 tests with 1 skip. Its direct reentrant probes replaced the current Context and forced collection during fallback construction, signal-flag truth, and Rational numerator access. The coordinator's merged validation remains the final integration evidence.
