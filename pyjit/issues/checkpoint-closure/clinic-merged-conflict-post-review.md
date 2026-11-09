# Merged Clinic generator conflict resolutions

Reviewed the coordinator's three reconciliations after merging selective `@value_function` with existing `value_core` support. **Approved; no must-fix finding.** A file with `value_emission` or `value_core` keeps the stronger `Py_VALUE_CORE` generated-header guard. A selectively decorated file without either directive uses the core-build guard. Both codegen flags remain available. The parser mutates converter state only on render-parameter copies; an object wrapper in a value-core unit still uses value-aware numeric extractors.

The added mixed fixture checks a selectively decorated `METH_VALUE` method beside an object-convention `METH_O` numeric method under `value_core`, including the latter's `_PyValue_AsInt` conversion. The coordinator reports 483 Clinic tests passing on the merged generator. This review covers the three conflict resolutions and fixture, not unrelated integration changes.
