# Counted CValue workspace cleanup peer review

2026-11-12 source review of the four-file `call55-gc` patch over `d2c5233fdf6`.
Disposition: GO under the selected [collector policy](collector-policy.md).
No candidate compilation or native execution performed by this reviewer.

No producer links `c_stack_refs`; `_PyCValue` contains only its owned value and
has no `next` member. Its mortal values already carry actual counts. Removing
the FT-only unproduced head and impossible deferred-adjustment walk is therefore
a source consistency correction, not removal of a working root protocol.
Comments now accurately describe native counted workspaces and owner transfer.
The patch introduces no GIL registration stores or larger scalar records.

Explicit transition roots remain independent. Arena and heap-edge authority
must still be complete before M4 removes arena scans. Retained native
stack/register/PyMem discovery, including live numeric leaf boxes, is documented
policy and is not a claim of fully precise native GC or FT qualification.
