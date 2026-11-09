# Third engineering integration

Runtime `87f13087ed9b97254b983bfb1adef61ae1fcde88` is a clean engineering
integration, not a qualified replacement for golden. Its two-pair matched
logging/upstream screen is **0.84554× elapsed time**. Both pairs are below 1.0;
the final two independent six-round campaigns remain pending qualification.
Nbody still regresses against golden. The full release diagnostic ran 54,504
tests and failed in 13 modules, reporting 126 failures and 99 errors.

## Implemented changes

`84a5f476eed` adds bounded debugger resume recipes. A current-NULL requirement
needs a whole-code no-writer proof and the actual reader's NULL capability.
All aliased reads must accept NULL. A backward replay within a FOR body preserves
the current iterator/cursor only with all-path phase availability, target
dominance and a cut-free path to the current operation. The setter detaches all
surplus owners, commits its target, then closes the detached values without
later frame writes.

Successful debugger commits advance a cold frame-object generation, including
same-address commits. Observed source, jump and completion transactions use it
alongside the physical PC before returning cached continuations or restoring
source context. Public line getters and frame-object creation do not count as
intent. Same-address setter-then-raise callbacks use the committed traceback
location. Ordinary dispatch acquires no generation check.

`87f13087ed9` changes final publication for four float arithmetic producers.
Parsed constructor, error and complete retirement-chain proofs establish the
canonical result. Named cache bindings, preprocessing and unsupported effects
prevent the optimization. One shared destination implementation retains local,
temporary and discard ownership behavior. A local-first category order is also
included. These changes do not establish a component speedup.

## Verification and limits

Release and debug each build and import-check 117 modules. Each passes a
21-test targeted sequence, including the unchanged folded-store and loop-replay
tests, then 320 tests across 11 focused modules with five skips. These sets
overlap. The native numeric matrix passes 12 destination/operator combinations
with 108 edge cases, plus four displaced-local cases with 400 callbacks per
build. It checks actual warmed specialized handlers and destination categories,
NaNs, signed zero, infinities, mixed values, boxed fallbacks and division errors.
The selected RC callback checks do not assert a Python prompt-finalization rule.
All 87 host generator tests pass; regeneration changes exactly four final stores.

The preceding wider tracing inventory still has substantial failures: 89 errors
and 27 failures, mostly protected/suspended or incoming-owner recipes. Its
116 failing subcases include 21 refusal-diagnostic checks and six observer-event
checks. The new FOR repair passes its original test; those earlier counts are
not relabeled as results for the new source. The full release diagnostic confirms
the same tracing counts and additional failures in debugger entry, source events,
static format fixtures, frozen embedding and frame-size expectations. The modules
are test_dis, test_doctest, test_embed, test_exceptions, test_ir, test_monitoring,
test_patma, test_pdb, test_peepholer, test_remote_pdb, test_sys, test_sys_settrace
and test_value_slots. Final release/debug qualification, API lint and a current
stats build remain open.
The last complete compile-only library check belongs to `3e5cbdb1123`.

Earlier failed receipts remain: one focus command selected two nonexistent
module names, followed by the corrected seven-module pass; a direct unittest
trace inventory enabled the large CPU-resource fixture and timed out at 120
seconds. The proper regrtest inventory completed and supplied the failure groups.
These are not runtime passes.

## Matched performance

GCC 15.2, JIT off, CPU 15, fixed hash seed, unchanged workloads, seven inner
durations per process and balanced process order. All elapsed campaigns retain
stable source and executable identities. Ratios are candidate/control.

| Workload | Two-pair screen / golden | Four-pair repeat / golden |
| --- | --- | --- |
| logging_silent | 0.59044 | 0.58952 |
| nbody | 1.04188 | 1.05620 |
| pickle | 1.01363 | 1.01038 |

The separate logging/upstream pairs are 0.83249 and 0.85880, geometric mean
0.84554. Do not derive a ratio between controls from separate campaigns.
The 29-workload screen and final parity campaigns remain open.

Twelve warmed nbody PMU captures pass with stable identities and 100% counter
running. Median candidate/golden ratios are:

| Counter | Ratio |
| --- | --- |
| Instructions | 1.03325 |
| Cycles | 1.05381 |
| Branches | 1.05861 |
| Branch misses | 0.99994 |

This is about 41.8 million additional instructions and 19.9 million additional
cycles per unchanged outer kernel iteration, including 20,000 advance steps and
energy bookends. Cycles per instruction are about 2.0% higher. These are not
counts per VM dispatch. The earlier integration's instruction ratio was 1.03194;
the new combined image does not demonstrate a reduced retired-instruction gap.
Compiler folding and code layout need investigation before attributing a benefit
to the checked publication change. Sampling shares are not speedup budgets.

## Next work

Repair the grouped full release diagnostic failures. Implement the
reviewed identity-of-real-LINE recipe, then a coherent protected/suspended
context design with compiler metadata and constructor ownership checks. The
current EH tuple alone cannot describe caught exceptions, context managers or
saved comprehension locals. Remote/GDB active source display remains a separate
consumer port. Diagnose completion events from matched ordinary-source probes.

For nbody, establish dynamic completion/dispatch counts and inspect compiled
paths before changing the hot completion model. Preserve one semantic ownership
contract while separating work known by the operation from cold observation.
Recheck the broad screen after the repair, qualify release/debug, then run the
two final logging campaigns and independent review before promotion.

## Evidence

Build-root `pyjit-link-wave1/frame-call-model-20261109/` retains
`ownership21-*`, `ownership22-*`, `ownership23-*`,
`third-engineering-{release,debug}-identity.log`,
`third-engineering-{golden,repeat,upstream}/`, `third-nbody-stat/`,
`third-nbody-stat-summary.json` and the completed `third-integration-*` diagnostic.
Golden source, builds and tags remain immutable.
