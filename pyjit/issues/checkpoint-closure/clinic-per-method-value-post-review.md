# Selective Clinic value emission: post-review

Reviewed the refreshed seven-file `clinic-per-method-value.diff`, including the generator, alias handling, output routing and fixtures. **Approved for the proposed Decimal migration; no must-fix finding.** The decorator independently opts a method into the existing value parser and `METH_VALUE` convention. Undecorated clones keep object wrappers because the clone overrides the flag, and conversion state is changed only on each function's render-parameter copies.

The first candidate leaked its private value include to every file destination because Clinic collects includes globally. The final patch rejects selective output when more than one file destination exists, before writing outputs. For the supported single-file case it places a core-build guard before the private include; buffered fragments carrying value types or flags have their own guard. `_decimal.c` uses one default generated file destination. This restriction is intentional; routing includes per destination would need separate design and tests.

The supported-form predicate rejects slots, accessors, constructors, optional groups, unsupported converters, parameter aliases, deprecated parameters, keyword variadics and limited-API output. It is shared by wrapper and parser generation. I found no object/value signature mismatch in the reviewed paths. This approval covers the generator interface, not any future Decimal method body: each annotated family still needs source and behavior review.

Author validation on the frozen patch: 480 Clinic tests pass, and a generated mixed-mode header compiles as `Py_BUILD_CORE_MODULE` while the non-core compile fails at its guard. I reviewed these claims and the source paths; I did not rerun the full suite.
