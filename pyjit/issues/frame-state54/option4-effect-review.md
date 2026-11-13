# Independent locals-lane effect-inference review

Source GO for the focused three-file correction. Moving CREATED publication
into _PyFrame_CopyData removed the direct state write from RETURN_GENERATOR's
DSL body. The old inference consequently mislabeled it PUSHES_FRAME rather
than SUSPENDS. This affects the generated observer/debugger contract; it is
not a harmless metadata spelling change.

The correction parses actual _PyFrame_Copy call arguments from comment-free
body tokens, respecting nested parentheses and brackets. It recognizes the
literal third argument FRAME_OWNED_BY_GENERATOR as the helper's CREATED
publication contract. The other three destination-owner literals do not imply
suspension. Missing arguments, an unterminated call or unknown/dynamic/
conditional owner expressions raise a source-analysis error rather than
silently changing the effect classification. Merely mentioning an owner in
a guard remains insufficient. The classification is still gated by the
instruction's existing frame-switch property in handler_effects.

The generator tests now remove the actual helper call when testing absence of
a suspension write, cover each destination-owner literal, and require unknown,
bitwise and conditional owners to fail closed. The contract test also requires
RETURN_GENERATOR not to carry PUSHES_FRAME. These checks address both the
positive helper contract and the regression's incorrect negative inference.
No unresolved source blocker was found. This reviewer performed source review
only; root owns regeneration, compilation, executed tests and native checks.
