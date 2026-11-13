# Round54 independent cold review

Source review of golden51 and the isolated R/F/N/L/S candidates, with particular
attention to compact F+N+L `82afe7aef2d` and validated S `e95328ba1d6`.
No runtime source changed. No build, generator, interpreter, inferior, profiler,
test or benchmark ran in this review. The canonical guidance, CPython AI policy
and round54 architecture, inventory and implementation-review packet were read.
Runtime source references below name the measured worktrees, not canonical C.

## Decision

No new must-fix semantic defect found in the reviewed changes. Source GO is
appropriate for coordinator qualification of compact and revised S. This does
not admit either implementation, certify a free-threaded build, or replace
the coordinator's native and runtime gates.

Keep golden51 selected pending direct candidate evidence. The supplied confirmed
F and R results include coroutine regressions of about 4.1% and 6.87%, despite
all29 means below one. Those are material failures of the stated breadth goal;
an aggregate win alone should not override them. L's supplied exec confirmation
is about 22.2% slower, with its coroutine/class screens also worse. Reject L
alone unless a directly measured composition earns its added cost. N's mixed
screen supports a bounded composition experiment, not standalone promotion.
The first S result belongs to the four-check implementation; revised S needs
its own native inspection and timing. Compact's actual 80-byte header and
focused semantic passes support further measurement, not a speed conclusion.

These performance statements use the coordinator's supplied results, rather
than a new analysis of raw pairs. Do not combine their ratios or attribute
their differences to instruction placement, caches or branch prediction.

## Highest-risk seams

**Mapping ownership and visibility are coherent, but costly.** Compact prepares
the mapping-bearing record before thread reservation, transfers its mapping
only after reservation succeeds, and leaves it private through binding
([ceval.c:3143](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-compact/Python/ceval.c:3143)).
That record is a full variable-sized frame allocation, including capacity for
all homes, rather than a small mapping box
([frameobject.c:4088](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-compact/Objects/frameobject.c:4088)).
Thus removing one fixed word can add an allocation and unused home capacity on
every supplied-mapping entry. This is a concrete total-work objection to L,
independent of any hardware explanation for its timings. Do not rescue it by
silently adding a second mapping-owner mode.

Its private/untracked phase and complete/tracked phase are real invariants.
Observer publication materializes kwargs using the association protected by
the record lock, then tracks it
([pycore_interpframe.h:476](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-compact/Include/internal/pycore_interpframe.h:476)).
Generator copying installs homes and owner/association before release-publishing
CREATED ([pycore_interpframe.h:282](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-compact/Include/internal/pycore_interpframe.h:282)).
The untracked record's counted mapping edge protects the mapping during ordinary
cyclic GC; complete heap publication tracks the record so mapping/generator
cycles can be collected. Moving tracking back into Initialize would reopen the
previously fixed incomplete-frame enumeration bug.

Unique retirement holds the mapping in C until after home retirement, then
closes it before the function owner. Escaped takeover keeps the owner on the
same frame object
([frame.c:213](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-compact/Python/frame.c:213)).
Traversal visits the mapping once from that object
([frameobject.c:3931](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-compact/Objects/frameobject.c:3931)).
These details justify retaining the corrected implementation for measurement;
the smaller header alone would not.

**Derived state retains the right authority.** Feedback comes from the prefix
of the executing word image, while globals/builtins come from the retained
function ([pycore_interpframe.h:69](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-compact/Include/internal/pycore_interpframe.h:69),
[pycore_interpframe.h:166](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-compact/Include/internal/pycore_interpframe.h:166)).
Neither derives execution from mutable function code. The accessors deliberately
assume an admitted normal frame; cold sentinel paths must establish that class
before using them. Existing sentinel fixes should remain at those boundaries.
Adding universal defensive checks would change their cost model without fixing
an identified normal-frame defect. Function clearing during cyclic collection
remains an ownership qualification concern, not a reason to recreate borrowed
caches that could themselves outlive their source members.

**R carries a valid narrow proof, but repeats classification at frequent
transfers.** Descriptor accessors consistently separate index, discard and
replacement. Publication handles redirect before rereading the descriptor,
and the empty route retains its physical NULL assertion
([pycore_interpframe.h:218](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-recipient/Include/internal/pycore_interpframe.h:218)).
The remote reader also validates and decodes the changed representation
([asyncio.c:345](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-recipient/Modules/_remote_debugging/asyncio.c:345)).
This removes real old-home work, but producer encoding and consumer branching
remain recurring work. Its coroutine regression is sufficient reason to avoid
promotion even though the ownership contract is sound.

**Revised S moves geometry proof to the correct boundaries.** Common constructor
validation rejects malformed directly supplied refined words
([codeobject.c:1390](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-clears-validated/Objects/codeobject.c:1390));
specialization checks the same predicate against the executing code's extent
([specialize.c:640](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-clears-validated/Python/specialize.c:640)).
The generated handler asserts geometry before loading inputs, guards both
inline types, performs fallible boxing before consumption, and overwrites B
after a nonescaping suffix
([generated_ir_cases.c.h:233](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-clears-validated/Python/generated_ir_cases.c.h:233)).
The intervening specialized closes receive inline integers and cannot call
destructors. The output may be boxed; that does not invalidate the input-owner
proof. The shared generator certificate checks the actual body, rather than
trusting the ADD name
([ir_generator.py:4528](/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/frame54-clears-validated/Tools/cases_generator/ir_generator.py:4528)).
This is a substantive improvement over four repeated release geometry checks.
Keep constructor/marshal/instrumentation restoration and generator negative
proof cases as admission gates; debug assertions cannot replace them.

## Next priorities if the gap remains

1. **Carry established facts across hot handoffs.** Use an execution refinement
   chosen at construction or specialization for a measured SEND/YIELD route,
   rather than recomputing its geometry on every transition. Preserve PRE/POST
   observation, first-send/discard, exceptions and redirects. Require a reached
   route census and actual removed native operations before expanding handler
   families. R's result argues against another generic per-transfer predicate.
2. **Extend S only where a whole commit is proved.** Wait for revised S's direct
   result, then consider another frequently reached arithmetic family using
   the existing geometry and nonescaping owner certificate. Include selection,
   dispatch text and constructor obligations in the cost. Do not extrapolate
   from successful overwrite counts to elapsed savings.
3. **Build a bounded native region around stable register facts.** The unfinished
   method/tracing tier can amortize dispatch and repeated value classification
   across several operations. Start with a small frequently reached region;
   define materialized homes, code pin, semantic PC and ownership at every
   guard exit, allocation, callback and exception before execution. This offers
   a larger removable-work unit than another isolated cached-pointer deletion.
4. **Make further frame changes conditional on a weighted lifecycle ledger.**
   Count frame starts, physical homes moved, mapping records allocated,
   feedback/environment uses and SEND/YIELD transitions outside quiet timing.
   Use those counts to choose the next storage contract. Keep function/code
   pins and continuation authority unless a complete replacement earns less
   total work. An 80-byte header already demonstrates why matching upstream
   geometry is not an architectural finish line.

Retain NULL homes and the initialized frontier. A V8-style safepoint root map
does not supply RC ownership, dynamic local boundness or callback mutation
authority. A bitmap proposal remains a separate representation migration with
all readers, transfers and exceptional phases owned; nothing in these results
justifies starting that migration as a clear-loop optimization.
