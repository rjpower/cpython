# Source origin and observer post-review

2026-11-09. Independent read-only review of the current uncommitted source in
the runtime integration checkout, against `4b3d739d867b`. Scope: original versus
propagated locations, source-edge assembly, observation capability and masks,
transparent wrappers, constructor/location consumers, and the proposed trace
helper port. No runtime edits or native jobs by this reviewer. Coordinator
evidence reports merge6 focused 112 passes and release build7 checking 117
modules; these are not this reviewer's test receipts.

## Assessment

No new concrete runtime correctness blocker was found in this repair batch.
The source-origin distinction addresses the previous duplicated-handler
failure at its cause. Observer dispatch now checks actual instrumentation
capability instead of assuming that every branch-shaped operation has a twin.
These repairs are suitable for the next focused native gate. They do not admit
unfinished debugger recipes, external source-cursor readers, or remaining
loop/send/shared-owner cleanup.

Original source identity is carried in the existing CFG opcode storage unit,
then a compile-time register-op flag. Location propagation alone does not create
an original source span; same-context NOP merges transfer it; copied blocks and
register conversion preserve it. Assembly filters propagated zero-word
locations before emitting edge records. Ordinary runtime dispatch acquires no
new source-origin field or metadata lookup.

Source instrumentation uses real branch twins where available. Otherwise a
no-save-IP predecessor uses a transparent instruction wrapper to establish the
actual observed predecessor, with separate tool masks so this private work
does not generate an unrequested INSTRUCTION callback. Landing observation
classifies the actual outcome and avoids replaying events already handled by
a branch twin. Removing a tool keeps wrappers needed for source observation.
This is a coherent use of the existing observer machinery.

I audited the actual opcode-view route as well as raw handler metadata:
`_PyIR_BaseOpcode` explicitly maps IR_JUMP to JUMP_FORWARD; `word_opcode` and
`base_opcode` use `_PyIR_EventOpcode`, and `_PyCode_HandlerFor` restores the
canonical handler. IR_JUMP therefore does not require a widened instruction
cache or a new constructor restriction merely because its raw handler number
exceeds 255.

## Trace helper port

Approve the bounded update to `lineno_matches_lasti(frame, event)` while keeping
the exact expected trace sequences unchanged. First accept the existing positive
co_lines range relation. Only for LINE mismatch, require an exact IRSM record
whose from equals f_lasti and line equals f_lineno, with valid compact PCs and
edge kind. Independently require a matching line range at the record's target:
positive coverage, an exact zero-width row, or a preceding matching row ending
at that target.

The last alternative is necessary: co_lines intentionally suppresses a
zero-width source row when its line equals the real landing line or the
previously emitted line. Requiring an adjacent zero-width row would reject
valid dynamic events. Requiring merely that the line occur anywhere in the
code would be too weak. This change checks the new offset/source contract;
it must not normalize away a wrong trace order or missing event.

Host-only extraction of the pure helper passed six boundary cases: positive
range success, exact source zero-row success, preceding coalesced range success,
and rejection for non-LINE, wrong predecessor, or the same line elsewhere.
The corrected GDB parser also passed a host extraction using its actual
Latin-1 string proxy shape. `git diff --check` passed at inspection. No native
observer result is implied by these host checks.

## Required focused evidence

The existing nested source EH, empty joins/backedges and native/Python delegation
tests cover the original failures. Extend them with two simultaneous tools:
LINE plus INSTRUCTION, a callback returning DISABLE at the privately wrapped
predecessor, one tool disabled while the other remains, local/global changes,
restart_events, and disable/re-enable inside a source callback. Verify exact
events, real instruction offsets and no CACHE/zero underlying opcode. Run the
same sequences on both sides of generator suspension.

Retain exact traceback lines and inner/outer exception selection after source
callback failure. Keep external GDB/remote source-override interpretation open
until those readers can observe the cursor/override during a source-only
callback; decoding IRSM alone does not provide that information.

For further simplification, keep one original-source marker and one shared
transition representation rather than adding more location-copy heuristics.
Likewise, derive observer capability from the existing opcode view and twin
tables rather than maintaining another handler whitelist. The next owner-aware
debugger increment should consume those same transition identities. There is
no reason to put source-map scanning into ordinary dispatch to solve these
cold-path requirements.

## Copy folding and debugger proposal follow-up

The new CopyProp source-origin transfer is also bounded by source line:
`Python/regcfg_passes.c:273` removes a pending candidate when its load line
differs from the current operation's line, before the use/exception test at
line 286 and origin transfer at line 290. It cannot combine two distinct source
lines into one reader merely by ORing the origin bit. The revised
`remove_with_line` declines producer/pop line or exception mismatch and only
suppresses a span when a same-context original-source neighbor carries it.

Approve `debugger-owner-recipe-proposal.md` for the next bounded implementation.
It correctly separates value provenance from counted owner identity, makes
conditional outcomes and call/suspension phases explicit, and requires dynamic
owner survival rather than static producer-PC equality across iterations.
Its commit-before-close rule and pending-child success/error split preserve the
existing overlap contract. This is design approval, not admission of current
useful-loop/protected/suspended debugger behavior.

Implement the two existing failures first, from their actual compact-word and
owner inventories. For the proposed connected-region proof, require the owner
to be available at the source and survive every admitted path, or require a
specific invariant anchor; mere graph connectivity is insufficient. Same-loop
iterator/index preservation can be a small explicit recipe. Do not implement a
general reconstruction engine before that recipe and the folded-assignment
case pass their existing behavior tests. Broader protected/suspended recipes
can extend the same representation after those gates; blanket rejection is not
the final debugger contract.
