# Final checkpoint cold review

Source GO for the correctness-only candidate `61015a9452d` in
`BASE/worktrees/frame54-cold-construction`. No new source blocker or unnecessary
abstraction found. Reviewed its actual complete diff against golden51 and the
extraction proposal/peer/source-review packet, plus the drafted round54 report,
tables, qualification and next decision. No build, generator, runtime, test,
inferior, profiler or timing ran in this review.

Exactly three files differ. Production changes stay in the existing cold public
generator constructor and coroutine/async-generator wrappers; the remaining
changes are its C test helper and three direct regression tests. There is no
NONE state, encoded recipient, same-home handler, layout removal, new sidecar,
or change to ordinary generated construction/publication.

The extraction stands on golden51's own raw discard contract. Setting
IR_NO_DST before Copy gives the public constructor the first-input behavior
already required of RETURN_GENERATOR. The new frame_obj reference supplies the
missing owner rather than merely deleting the reproduced impossible assertion.
Coroutine-origin allocation happens before ownership moves; its failure frees
only the still-untracked allocation and consumes the stolen input reference.
After the move, nonfallible initialization completes the subtype and name
owners before frame association and GC publication. The stolen input reference
is released after unlocking
([genobject.c:1227](https://github.com/rjpower/cpython/blob/61015a9452d9069646fb5fede9ff963896833b89/Objects/genobject.c#L1227)).
Direct wrapper returns prevent duplicate origin allocation and later mutation.
The direct locals owner and escaped-frame takeover retain the baseline contract.
These are concrete correctness repairs independent of the rejected performance
experiments.

The drafted performance rejection is supported by the presented tradeoff,
including combined all29 mean 0.993149 with 17/29 lower but confirmed coroutine
ratio 1.098464 against generator ratio 0.956948. It is a reasonable decision for
this round's target, while the report correctly avoids a universal single-case
veto. It preserves the distinction between real removed work, measured elapsed
results and unproved hardware causes. The matching S2 census supports its stated
reached-site count; it is not a relabeling of the earlier four-check census.
The compiled-region and predecoded-transfer priorities address larger repeated
work without claiming a speedup or reviving an incomplete bitmap contract.

No material overclaim requiring a report change was found at this draft stage.
Keep these boundaries when closing the checkpoint:

* The cold correction is still a candidate pending its own focused, failure,
  full-diagnostic and timing results. Neither earlier R2 passes nor a successful
  build qualifies this extraction's runtime behavior.
* Historical golden51/upstream ratios remain identified as historical selected51
  measurements. Unchanged hot source does not establish equal native placement
  or elapsed behavior in the newly linked correctness build.
* Report actual full-diagnostic failures and their baseline relation. A known
  failure is not a passing full suite; no free-threaded qualification follows
  from this GIL source review.
* Rejecting the performance candidates does not undo their semantic lessons or
  require absorbing their machinery into the correctness fix. Keep the three-file
  scope intact unless qualification reveals a concrete dependency.

This verdict supports final qualification of the bounded repair. It does not
select a new golden, assert performance admission, or certify a completed full
suite.
