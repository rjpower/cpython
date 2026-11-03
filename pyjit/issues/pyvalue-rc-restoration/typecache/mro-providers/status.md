# MRO providers and public wrapper

Stage: independent code/native attribution accepted; exact source delta held
with the remaining typeobject work. Owner: coordinator integrates. This is
selected-snapshot acceptance, not publication or runtime admission.

The fourteen C3/MRO/validation/readiness provider functions and immediate
type_mro_impl wrapper now use canonical metadata with explicit owners through
callbacks, conversion and cleanup. Boolean identity predicates remain pure
0/1. Default/custom computation and conversion occur once. Two initial review
findings were fixed: install a buffered MRO conflict error only after owner
retirement succeeds, and normalize the private C3 list to logical zero before
allocation-success-plus-error can enter callback-capable cleanup.

Final typeobject SHA256: d07219973716cab2749c7f9992ee268dd014815d28fd574c6d6c2d84b74dcf1d.
Fifteen bodies changed; 413 other recognized functions and all masked text are
exact to the graph endpoint. Twelve host controls pass without skips; all 41
candidate methods in the focused file remain unexecuted.

Actual stable-input full-TU checks retain 307 input hashes. Before: 47 errors
and 39 warnings. After: 38 errors and 23 warnings. Both fail. All remaining
diagnostics are in unchanged functions, including two missing-API first-use
messages relocated there. There is no full TU/object/link/runtime success.
The subsequent merged v4 build also fails before linking.

The next transaction must carry owned bases/base-MRO inputs through final pure
validation and actual field publication, then retire them under closed mutation
admission with conditional rollback preserving newer nested state. That work,
guards/static/terminal lifecycle and scalar slot providers remain required.

Evidence: build/pyvalue-typecache-mro1-20261102-v1 retains design/scope, initial
REVISE, exact two-body correction, final independent-code-review-v2 and the
independent-native-v2-addendum. Raw corrected checks live in native-v2; the prior
receipt was preserved. See the [type-cache page](../status.md) and
[work board](../../../README.md).
