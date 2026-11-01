# RC compatibility design review

2026-11-01 06:17 UTC. **Accept host-only helper preparation.** The
[next compatibility plan](rc-next-qualification-plan-20261101.md), SHA256
`88d986d37574ad3b862aefea7f9fa7ee21dfbda21351c1a3a9fc1f8adb90c5df`,
targets new C-API ownership and embedding coverage on existing canonical d7df
products. Its literal roster is frozen at SHA256
`56eca960b39a15368ced921b412770d91e09c9c81810d1fba6e6eb55d846f306`.

Root inspected selected repeated-init/subinterpreter/module/frame interfaces
and the debug expected-abort tests. Reusing both existing images avoids another
build and repetition of the passed39 cases. Fifty debug and48 release methods
are a useful next tranche. They are not full qualification or collector choice.

Actual exposure must distinguish ordinary workers, platform auxiliaries,
embedded C executables and expected-abort children. Exact36 target programs,
zero unexpected skips, pre-wait raw stream capture and cleanup covering both
Python and `_testembed` are helper review obligations. An unobserved child or
partial timeout output cannot be relabeled as passed coverage. Root does not
infer execution from the source-derived count.

Preparation ends06:40/checkpoint06:28. No source, compiler, native, new build
or activation is authorized. Independently review the narrow runner adapters
and meaningful timeout controls before a separately scheduled native tranche.
The hard GC cutoff remains unchanged; no RC integration authority follows.
