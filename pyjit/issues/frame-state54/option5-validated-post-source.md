# Validated option 5: independent handwritten-source review

Source GO for the six-file diff in `frame54-clears-validated`, based on the
preserved initial clears candidate. No generated/native result is reviewed
here. No runtime, compiler, generation or test execution was performed by
this reviewer. `git diff --check` reports no whitespace errors.

The new shared `_PyIR_InlineIntOverwriteGeometry` proves A==B, C!=B,
B>=the full localsplus count, and B/C below both the physical frame extent
and IR_MAX_SLOTS. It rejects either input at the read/discard sentinel. C
can be local or constant; B must be writable scratch. Constructor counts
are nonnegative before their size_t sum is formed. Selection uses the
executing frame's pinned code and its actual physical slot count. This
closes the earlier selector's missing extent proof rather than assuming
that public `check_words` ran.

Hidden constants need no additional subtraction from the scratch extent.
`code_new_with_fbinit` inserts their CO_FAST_CONST|CO_FAST_HIDDEN names into
localsplus between cells and free variables; `co_nlocalsplus` includes all
of them. `_PyFrame_FillConstSlots` writes those table-named homes. Common
`check_constslots` verifies table positions against those kinds and calls
`check_const_layout` before the new refinement loop, including on marshal
input. Thus B>=the complete localsplus count excludes every constant home;
they are not an extra tail beyond this boundary. C may read them. No
constant-layout or generic admission change is needed for this variant.

The common `_PyCode_Validate` word loop checks actual refined handlers,
covering public construction/replacement, assembler and marshal. The
selection decision uses the same helper before installing the refinement
from ordinary ADD_INT. Existing immutable original-word copies and
instrumentation restoration retain that constructor proof, while
polymorphic feedback retains its ordinary ADD_INT canonicalization. The
trusted fixed-word/static exceptions audited in
`option5-constructor-peer.md` introduce no refined words.

The generator change replaces only the integer overwrite variant's release
fallback with a shared debug assertion before reading inputs. The unchanged
analyzed integer body still guards inline types, performs fallible boxing
before ownership commit, and publishes immediately after the omitted B
NULL store. Strict changed-commit/escape/error/deopt proof negatives remain
in place. The LOAD_ATTR variant's geometry path is untouched. In release
builds assert arguments are not evaluated; actual native removal remains a
root gate.

New focused tests use a real `(x+y)+y` compiled site with no hidden constant
homes and copy the matching source-map bytes so malformed construction
reaches this validator. They cover seven invalid geometries, changed frame
metadata, a valid direct variant, inline/boxing/type-deopt results,
instruction monitoring and original-handler restoration. Marshal tests
round-trip a valid refinement and corrupt same-length serialized words to
exercise common validation without relying on public `check_words`.
The revised generator test requires the shared assertion before IR_VIEW
and prohibits the old release fallback. These are source-reviewed test
intentions, not execution claims.

Root must regenerate, run the focused generator/operand/constructor/
monitoring/marshal tests in both roles, and inspect the matching native
handler for removed geometry comparisons and fallback. Retain the original
four-check candidate's benchmark identity; this is a new measured candidate.
