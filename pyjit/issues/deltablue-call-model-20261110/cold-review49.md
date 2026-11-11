# Cold review of conditional leaf implementation

**Final integration decision:** this is a rejected performance experiment, not
selected runtime48. Source-review GO means proceed to qualification, not promote.
See the [rejection report](integration49.md), [checkpoint report](checkpoint-report.md)
and reviewed, unimplemented [next design](C2-proposal.md).

**No remaining source blocker in the corrected prototype. Proceed to coordinator
regeneration, compile and focused qualification.** This is source review, not
native qualification or a performance claim. No leaf hit has yet been demonstrated
by a native run in the evidence reviewed here.

Reviewed the stable uncommitted diff in
`BASE/worktrees/deltablue-leaf-prototype`, based on `986c65ee5ab`, including the
two new test modules. The author froze corrections for rereview at 23:25 UTC on
2026-11-10. Read the applicable instructions, CPython AI policy, C implementation
proposal and proposal review. This reviewer performed no runtime edits, native
execution, regeneration, builds or commits. BASE is defined in canonical HANDOFF.

## Findings and corrections

The initial prototype had one integration blocker: ordinary shape/class/builtin
global handlers kept their old guard uops and then called complete probes that
repeated those guards. For example, LOAD_ATTR_SHAPE performed type/shape checks
in its prefix and again inside `ir_probe_shape_value`. That added work to ordinary
loads even when no leaf call was eligible. This was a source design regression,
not a request to begin a CPI investigation.

The correction removes those prefixes and gives each ordinary reader one complete
typed probe. In the final `Python/bytecodes.c`, global readers at 2082/2090,
shape at 2668, and class readers at 2775/2784 perform their probe, miss check,
acquisition and existing retirement once. Cache positions remain unchanged:
shape type-version/shape/offset; plain class class-version/padding/guarded-value;
metaclass class type-version/class-version/guarded-value; builtin global globals/
builtins versions/index. Metaclass selection is literal false/true in separate
typed producers. The positive canonical-source validator checks those exact
arguments, cache types and acquisition/retirement bodies; strict suffix validation
remains. The guarded-word field is converted to a borrow only after version
validation inside the shared primitive.

All five ordinary bodies increment hit statistics **after** successful probe
miss checks. `_CHECK_ATTR_CLASS` remains at 2768–2773 for A's classmethod consumer.
This must survive the merge; deleting an otherwise unused L definition would
break the composed A path.

The review also identified three useful omissions, now corrected:

- `Python/ir.c:432` now checks generated roles, flags, exact effects, input/borrow
  shape and read/write/through extents for each admitted immutable handler.
  Simple loads have distinct effect contracts. Small-int forms record their
  explicit bounded immediate-constructor obligation. Generic read, comparison,
  entry and return effects remain obligations discharged by guarded execution,
  inert entry and nonfinal retirement; they are not treated as pure opcodes.
- `Python/ceval.c:1908` accepts supported live private proofs with historical
  MONO/POLY/MEGA state. Historical polymorphism no longer permanently excludes
  an otherwise valid current handler. It never executes a historical row.
  Same-code/different-globals tests now require increased hit counters before
  and after environment changes, rather than accepting correct fallback alone.
- The final precommit check rechecks asynchronous eval-breaker/epoch equality.
  It no longer repeats provider, observer and global-monitor scans after the
  callback-free GIL-held interval. Opposite immutable branch senses and ordered
  inline-float cases now have positive-hit test witnesses; NaN must miss.

No further required cleanup was found in this scope.

## Architecture and ownership judgment

The representation is bounded and does not introduce an opcode VM. Four paths
encode two predicate operands and two results; each path has one indexed root
and at most two attribute reads. Runtime work follows that fixed structure,
checks one scalar comparison and reads only the chosen result arm. It neither
walks executable IR nor grows a table of function-specific recipes. Shared
integer/float computations retain ordinary comparison admission; only the leaf
probe rejects unsupported numeric/operator cases.

`leaf_body` at `Python/ir.c:594` accounts for the complete final immutable body
with visited-word and dependency masks, current physical-slot definitions and
forward branch targets. Dead raising reads cannot disappear merely because the
return value ignores them. Folded argument/constant roots, constslot kinds,
indices and feedback geometry are checked. Calls, stores, loops, exception
regions and unsupported forms decline. The optional record is allocated after
acceptance, initialized for static execs, counted in sizeof and freed with the
exec; the no-feedback early return retains it correctly. It owns no Python edge.

The terminal marker is limited to the positively checked plain exact-call prefix.
The no-record path checks the existing exec/leaf pointers before invoking the
out-of-line helper. Stack and recursion guards remain before the marker. Entry
checks cover providers, observers, monitoring, entry handler/epoch, pending work,
redirects and unsupported builds. Debug conservatively declines; its fallback
results cannot be reported as positive leaf coverage.

The nonfinal proof counts actual decrements by object identity after result
acquisition: function/self/arguments plus old local or discarded result. The
strict aggregate inequality covers aliases that per-owner tests would miss.
MISS releases only the acquired duplicate while its original root chain remains
live. On DONE, homes detach once, only preflighted nonfinal releases run, and the
existing return recipient publishes the held result without a child activation.
The surviving function protects current code/globals through retirement. No code,
feedback or borrowed intermediate is used after releases. The independent owner
review reached the same conclusion; no additional semantic blocker was found.

## Qualification still required

The author reports twelve host generator checks passed. I inspected the updated
source and negative/positive test structure and ran patch whitespace checking;
I did not rerun those host tests. Native tests must still establish the declared
counter deltas, both arms/senses, environment guards, observer fallback and alias
misses. Source tests alone cannot establish admission coverage or saved time.

Root must inspect regenerated products, compile release and debug, run the strict
value API gate and focused semantic tests, and review the merged A+B+C tree.
The required real release compile has not occurred for this prototype, so the C
post-review remains incomplete until that evidence exists. Application timing
must count the evaluator/guard/owner-preflight cost honestly; activation omission
is structural work removed, not proof of an elapsed-time improvement.

## Final source follow-up: 24d97fa8c3b

**No new source blocker found** in the four-file follow-up on integrated
`656326dc375`. Read `C-call-admission-diagnostic.md` and the actual committed
change. The common-constant root is the missing provider for ordinary `return
None`: `leaf_format` requires its exact PURE contract, construction bounds the
index by NUM_COMMON_CONSTANTS, and execution borrows the current interpreter's
canonical home. Interpreter initialization immortalizes all 14 entries; the
existing clearing guard protects this callback-free interval. It adds no opcode
evaluator, new call ABI or changed retirement rule.

The corrected fixtures create eligible targets through real MAKE_FUNCTION,
preserve shared target-code identity across environments and keep explicit code/
provider mutation fallbacks. Observer witnesses run in fresh subprocesses,
preventing persistent monitoring metadata from contaminating later positive-hit
tests. Hit assertions remain strict. Neither invalid function versions nor
allocated-but-inactive monitoring metadata should be admitted merely to repair
these tests; each would require a separate policy/guard review.

Reviewed `census_leaf.py`: it installs no observer or instrumentation, checks for
active trace/profile/global monitoring, warms the unchanged kernel once and
subtracts actual per-code counters across one further unchanged run. Top-level
functions and methods cover this kernel; shared code is counted once. The output
correctly calls these admission counters, not timing or all semantic calls.
Uncertified functions are omitted, and generic CALLs never reaching the hook are
outside the counters. Zero hits therefore cannot by themselves identify missing
certificates, invalid function versions or low semantic frequency.

Read `delta49b-build-checks.json` and both build log tails: clean 24d97fa8c3b
release/debug builds return 0 and each checks 117 modules. The real release compile
requirement is now satisfied. Corrected native hit/miss witnesses, focused merged
semantics, final API qualification and elapsed-time evidence remain coordinator
gates; no all-green 49 or performance result is asserted here. This reviewer ran
no native job and changed no runtime file.

Three structural follow-up hypotheses, to rank again after actual census/timing:

1. **Direct scalar comparison returns.** `Strength.stronger`, `Strength.weaker`
   and `BinaryConstraint.is_satisfied` return comparisons directly. The current
   certificate recognizes COMPARE_OP_JUMP, so these bodies remain outside it.
   Encode a checked comparison return as the same select with canonical True/
   False result roots. This reuses the region, scalar guards and terminal owner
   proof; it needs no new evaluator. Establish actual eligible calls and saved
   time before broadening further.
2. **Compose a certified predicate with selection.** `Strength.weakest_of` and
   `strongest` call a comparison helper before returning an argument. A bounded
   whole-region composition could remove both activations after live target/
   code/provider guards. It must retain complete-body and environment proofs,
   never invoke arbitrary CALL from the probe or become an opcode interpreter.
   This follows the direct-comparison case only if remaining call volume merits
   the additional proof and guard cost.
3. **Share resolved exact-call facts.** Exact-call guards, the leaf helper and
   normal frame setup reread selected function/code/binding facts. A small shared
   resolved view could remove duplicate preparation for hits and remaining
   effectful callees. Keep the owned call window and independent executing-code
   owner. This is a targeted call-path change, not frame-layout surgery; pursue
   it if measurements show preparation remains material after elision.

These are candidates for later reviewed increments, not predicted gains or
remaining requirements for the current source change.
