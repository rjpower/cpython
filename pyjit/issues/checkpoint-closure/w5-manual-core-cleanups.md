# Manual core value cleanups

Patch: `w5-manual-core-cleanups.patch`

The packet removes three unnecessary object conversions:

- `_abc` reads the already type-checked owned flags value directly with `_PyValue_AsLong`.
- `_PyEval_StoreName` passes the incoming borrowed value to `_PyValue_SetItem` for non-exact-dict namespaces.
- `mappingproxy` has a paired value-form subscript slot, with its existing object slot implemented as the value-entry thunk. A test covers an inline integer key and value.

Baseline: main runtime commit `d6eae4b1c1c200444b325240a00791b44288b1e3`. The patch applies cleanly to that baseline. The manual S tree has an unrelated `ceval.c` hunk in the keyword-dictionary construction path; it is intentionally excluded from this packet.

Validation is pending for `_abc`, `ceval`, and `descrobject`: the current D tree cannot compile those translation units because unrelated calls in each file still use APIs already marked unavailable. The first locked W5 mappingproxy build completed and imported all 117 modules, but the command output did not retain the focused test lines. A repeat hit `Text file busy` while another process used `Programs/_freeze_module`; source was restored. Do not treat the mappingproxy runtime test as confirmed until rerun on an uncontended build.
