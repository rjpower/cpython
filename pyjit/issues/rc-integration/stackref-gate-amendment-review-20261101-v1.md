# Independent StackRef profile scope review

2026-11-01. **ACCEPT the proposed whole-profile qualification amendment.**
This accepts withdrawing the added global `Py_STACKREF_DEBUG` profile from
this ordinary RC landing's supported scope. It does not accept the failed
checker run, establish absence of ordinary runtime defects, or authorize
landing, source changes or further native work.

I reviewed the proposal and final assessment, reconciled all twelve failed
families and all 43 assertion IDs and traceback hashes against the actual
standard worker results, and rehashed 56 retained source/evidence bindings.
The partial run remains 563 returned modules with twelve failures and two
unfinished modules. Root's stop after known failures remains distinct from a
natural timeout. Both upstream controls failed during bootstrap; none of the
three comparison methods ran. Their failure cannot establish upstream
reproduction of RC's shutdown, manager, REPL or other signatures.

The evidence supports a configuration boundary. The global checker substitutes
interpreter-local table IDs and lookup/close operations for ordinary stack
values. Its TLS-dependent async lookup and remote pointer reader have concrete
support gaps. The corrected upstream bootstrap also rejects a borrowed owner
while the source transfers ownership into a C temporary. These observations
justify treating this entire diagnostic profile as unvalidated; they do not
explain every retained failure. Manager, pyrepl, interrupt and other incomplete
attributions remain unresolved.

The fork's borrow bookkeeping is narrow: the frame's overwritten-local tuple
first retains a real object reference; its sole marker permits the original
closed diagnostic owner to remain until outstanding borrowers close. The leak
report skips closed records, not arbitrary open records. The marker is a no-op
outside this checker. I found no supported ordinary-build defect established
by this specific evidence. This review is not an exhaustive ownership proof;
any concrete ordinary-build defect remains blocking.

Accept the proposal as written: withdraw the entire added profile, preserve
all failures and both failed controls, add no twelve-module exclusions, change
no checker/test/runtime code, and stop this control ladder. A future claim of
checker support requires separate repair and fresh qualification. Ordinary
release/debug full results, bounded ASan, refcount/lifecycle and other accepted
evidence retain only their actual scopes. Packaging, regeneration, unchanged
C-extension/refcount intervals and main integration gates remain required;
they are not passed by this amendment. No additional ordinary full sweep is
required solely because an unchanged compiled input set is now described
accurately.

Reviewed inputs and exact hashes are in
`build/rc-stackref-gate-amendment-review-20261101-v1/receipt.json`.
The independent replay read retained files only; it invoked no target,
compiler, build, test rerun or probe and changed no author artifact.
