# Three-case upstream checker helper review

2026-11-01. **ACCEPT exact held comparison helpers with the required canonical
outer lock.** No upstream target/compiler or gate amendment occurred in review.
This is one new checker build and three fixed standard methods, not a full
upstream suite or a qualification pass.

Directory `build/upstream-stackref-control-helpers-20261101-v1`; original capsule
`fa68a311297eb0d33bf75fe49d415b9465496ec0aa7ec1bbf9d9b9ac1f5e6cd5`,
plan `bab54b9a204da518f125728a94529bd0f60579acfbad1f031633bc53ff37a2a8`,
launch `b838cd5e8ddc2960cc7333c6a3c5432b7e2ddf71e4a61bbb230632cff01d17d0`,
ready `d53afa8b929fa673648d5b29222f9c717182bb4c3fdda9ec021aff73ce3e4ba3`.

Root read the actual build/gate/argv/input/control/observer/result/cleanup
code and its narrow diff. Replay independently verified5,828 inputs,32 capsule
artifacts, exact source321925/cleanliness, existing upstream products and tool
bytes, composed builder imports and ten host controls. Only two git calls were
allowed during preflight; no compiler metadata query or target. Existing
run_row, idle and observe_cleanup bodies are AST-identical to the accepted
supervisor. Source entry kinds avoid treating directory symlinks as files.
Receipt `build/upstream-stackref-helper-root-review-20261101-v1/receipt.json`
SHA256 `37ca5324c0a060a72bd6e19d924814a378ded6e65c8a8bf939ca5cbf8746a1fa`.

The original manifest's inner `/tmp` lock differs from the canonical shared
lock. Additive `launch-lock-supplement-v2.json`, SHA256
`eecd4bc7ec7a8779f7ec149739ef76dc66344e60c570f579ca64431d9e85c993`,
requires root's exact outer flock command through terminal cleanup. Root must
bind that supplement, runner/tool identities and literal outer argv in activation
and use it. Frozen helpers remain unchanged; the inner lock is redundant.

The standard runner executes exactly the late-GC embedding, asynchronous
traceback and daemon-finalization methods once, without failfast. Natural
verbose IDs, standard JUnit/testsRun and exact origins/configuration close the
three methods; expected platform skips, missing methods, import/mechanical
failures and abnormal exits do not answer the comparison. Ordinary test
failures remain raw comparison data with their standard exit and signature
presence. Signature extraction alone does not establish causal equivalence.
No Boil CAPI, verifier mode or extra probe is used. Build witnesses verify
actual checker call targets. The new project build preserves old source/products.

Separate root activation remains necessary:1,800 seconds inclusive, build1,200
plus tests180 and210 reserve per row, downward clamps/no retry. Both machine
slots and the build lock span exact-executable/pidfd cleanup, including a failed
build with absent prospective images. Results require independent comparison
before any qualification amendment, repair or exclusion. The failed partial RC
full suite remains failed regardless of this control's outcome.
