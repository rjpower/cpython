# Independent guarded cache storage: implementation handoff

Prepared on runtime43 `47dc23f48cf1470d4a98137beb5d313b5093033d` in the
execution-boundaries worktree. Root has taken over the commit, release/debug
builds and focused native checks. Their final HEAD/results are not yet recorded
here. No performance or full-suite run was performed by this lane.

## Change

`Include/internal/pycore_feedback.h` now places `_PyFbAttr.class_value` outside
the native object-pointer/shape union. `_PyFbPayload` is a struct containing a
native descriptor/shape union and an independent `_PyValueGuarded` member.
Existing handler selection, canonical value representation, hit guards and
acquire-before-retire transactions remain the same. There is no checker change,
union exemption, new encoding or frame flag.

Compile-time assertions require attr **48 bytes**, payload **16 bytes** and
four-observation poly aggregate **120 bytes**. The preceding sizes were 40, 8
and 88: +8 per attr entry and +32 per poly allocation unit. The existing
`Python/ir.c` poly memory accounting uses `capacity * sizeof(_PyFbPoly)`;
generated feedback metadata uses `sizeof` of the layout type. No independent
hardcoded allocator/accounting size was added.

## Capacity and regression changes

Feedback remains capped at 4096 eight-byte units, with two units for the shared
header. Attr entries now use six units instead of five:

- Pure attr sites: `(4096 - 2) // 6 = 682` private entries, ending at unit4094;
  two units remain. Their vector occupies 32,752 bytes including its header.
- With the existing compare/contains sites taking units2–9 first, attr capacity
  is `(4096 - 10) // 6 = 681`, previously817. Later sites retain shared entry0
  and their generic handler; they are not granted invalid private feedback.

The new `test_guarded_attribute_feedback_capacity` in `test_ir_attr_cache.py`
compiles 700 class-value reads and checks the exact private-unit sequence,
18 shared sites, vector byte length, warmed private/generic handler split and
class mutation behavior. These are test assertions awaiting root's native run,
not reported passing results.

Affected existing expectations were updated in `test_ir.py` (shared-vector
capacity, layout size and constructor-table bounds) and `test_ir_contract.py`
(the three attribute-family layout sizes). No unrelated test behavior changed.

## Verification and outstanding work

The changed test modules parse with host Python; `git diff --check` was clean.
The existing compile database's API gate ran first, with the unchanged checker,
logging to `E/object-call-20261109/object44-release-lint.log`. The coordinator
reports **306 units, zero failing findings**. This removes the two guarded-union
policy violations; it is not a substitute for compiling the changed layout.

Root owns the final commit identity, real sizeof assertion checks in release/
debug builds, and focused attrs/feedback/IR/calls tests. Application feedback-site
coverage, metadata footprint and source44 retiming also remain root's work;
passing the bounded synthetic capacity case alone would not establish unchanged
application cache coverage. Startup RSS is not an admission gate. No further
runtime edits or native jobs will be started by this lane during that handoff.
