# Owner retirement architecture peer review

Read-only source review while source20 is frozen. No implementation admission or
performance claim.

The semantic last-read/edge direction is sound, and the proposal correctly
separates shared canonical owners from unique producer sinks. The first terminal
increment has a concrete source blocker: ordinary thread-owned frames do not
already close every surviving scratch home at return.

`_RETURN_VALUE` asserts `_PyFrame_AssertTemporariesClear(frame)` before leaving
the frame. The canonical-return checked body in
Tools/cases_generator/ir_generator.py:3994 explicitly includes that assertion.
Python/frame.c:225 repeats the assertion for FRAME_OWNED_BY_THREAD, then clears
only co_nlocalsplus homes. Generator-owned or escaped frame cleanup does not
establish the ordinary thread-owned return contract. Deleting adjacent scratch
releases without another ownership transition would trip debug assertions or
leave live homes for reuse in release builds.

Revise the terminal proposal before implementation. Options include a precise
retained range encoded in a real semantic return variant/outcome, or a reviewed
change to thread-frame retirement with its actual scanning cost. A range operand
must describe normal/error/source-callback owners and hold the returned value
outside the retired homes through reentry. It must not become a standalone
release-only word or silently add scanning to every ordinary return. Static
POP adjacency alone does not supply that representation.

Next-source measurements and allocated witnesses can select between these
options; the existing proposal's assertion that whole-frame cleanup is already
free must not carry into source admission. The remaining branches, finally and
comprehension restoration obligations in the proposal remain applicable.
