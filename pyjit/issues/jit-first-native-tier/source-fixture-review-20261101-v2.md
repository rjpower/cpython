# C-stage fixture correction review

**ACCEPT** `30a0f5ab1bd02128495c5a0fe03128a417375460`, parent `f206d66979f1051d3664d3341150be421daa90b8`. Only two methods in `Lib/test/test_ir.py` change; its exact SHA-256 is `a13ff4a46d79dcf1cfb837df8751b5d96bbdd1871d2d9077b318f79d2876cf03`.

The malformed-input fixture converts existing handler names through `dis._all_opmap` before the unchanged encoder, preserving numeric unknown-handler values, fields and constructor assertions. The continuation fixture inspects a live exception inside `except ZeroDivisionError`, retaining exact code/IP assertions and explicitly failing if the expected exception is absent. All other method ASTs, the twelve new IDs and production/generated inputs remain unchanged.

Independent source/hash/AST checks passed. The retained host encoder control and source compilation establish fixture interfaces only; the corrected target assertions remain unexecuted. The original failed cohort remains [failed](../../../build/jit-prefix-c-stage-outcome-review-20261101-v1/review.md). An unchanged release image may be reused only through the separate exact product/compiled-input bridge and fresh library/origin bindings. This review grants no target execution, native encoder or elapsed credit.

Receipt: `build/jit-prefix-fixture-source-review-20261101-v2/receipt.json`.
