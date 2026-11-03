# H3-1 stage 2b.3 independent design review

Verdict: **REVISE**. The storage, counted-entry and producer-metadata
directions are sound. The family report needs a concrete arithmetic and
attribution contract before implementation. This is a design-only verdict;
it authorizes no native edit, build, probe or landing.

Reviewed the untracked H3 draft `boil/design/work/H3-1-stage2b3.md`, SHA256
`141e85f1f4a407cc3183380c9c90c7080dab1b49f860c9c25cf4af46d3318f14`,
against H3 HEAD `ffc548603b823484fdf106b1a9de06785017c682` in
`.worktrees/boil-compiler-copyprop`. Read primary README/PROCESS, applicable
AGENTS instructions and the CPython AI policy. Review used source reads and
Git metadata only. No native interpreter, compiler, build, test, probe or
benchmark was executed, and no H3 source or document was changed.

## Blocking: specify what the family report can calculate

Draft lines 107–113 require aggregating entry counts through the base map
and combining them with existing semantic counters. That does not yet
specify valid denominators or resolve counters whose semantic association
differs from that map.

1. `Tools/scripts/summarize_stats.py:242–244` currently defines a family
   total as `execution_count + specialization.hit + specialization.miss`.
   After aggregating all raw family entries, specialized entries are
   already in `execution_count`; adding their semantic hits and misses
   counts overlapping events again. For example, 10 generic entries and
   90 successful specialized entries give 100 family counted entries,
   not the old formula's 190. Specializer redispatch, predicted-label
   bypass and the separate binary-op deoptimization route also mean that
   these counters cannot be assumed to partition logical operations.
2. The generated base map is not a complete semantic-counter attribution
   map. In `Python/bytecodes.c:3345–3360`, the truth-branch specializations
   belong to `POP_JUMP_IF_FALSE` and `POP_JUMP_IF_TRUE`. Their reused truth
   uops record hits under `TO_BOOL` (`bytecodes.c:488,494,530,540,563`).
   The generated `POP_JUMP_IF_FALSE_INT` target at
   `Python/generated_ir_cases.c.h:15665` shows this directly: its entry
   belongs to the branch family, but its successful truth operation
   increments `TO_BOOL.hit`. Specialization success is associated with the
   branch family by `Python/specialize.c:536`, and misses with the branch
   family/member by `Python/ceval_macros.h:300–305`. The deferred macro in
   the branch specializer names `TO_BOOL`, but its `opcode == opname`
   condition prevents treating that as a branch-deferred counter.
   Existing aggregate data cannot reconstruct an exact per-branch hit
   or deferred partition without changing the counter sites, which this
   slice correctly excludes.

Revise the design to define separate report quantities and their formulas.
The counted-entry family total can be the sum of raw entries mapped to that
base; keep semantic counters under their existing recorded names and label
them accordingly. Do not present their ratios as family execution shares
unless the required attribution and denominator are established. A
reasonable bounded solution is to show semantic event counts separately,
retain success/failure ratios with their own event denominator, and omit
unsupported hit/deferred shares. No new semantic remapping or specializer
edit is needed merely to widen statistics.

Specify the raw effectiveness partition as well: distinguish ADAPTIVE
targets from SPECIALIZED targets and ordinary/NOFB targets using producer
properties. If specialized misses are subtracted from raw specialized
entries, justify that particular relationship under the preserved entry
lifecycle rather than using aggregate family hits as a substitute.

Required reader fixtures should include the 10/90 example, a truth-branch
entry with its hit recorded only under `TO_BOOL`, NOFB entries sharing an
adaptive family base, and a guard failure. A copied old denominator or a
copied attribution-by-base assumption must fail a named numeric assertion.
Rendering a specialization section with boxes and metadata present must
also succeed: the current generic field loop at
`summarize_stats.py:246–262` admits `boxes`, while the section's documentation
lookup recognizes only specialization event labels. Select semantic fields
explicitly instead of treating every remaining numeric field as an event.

## Nonblocking implementation requirements

- The producer facts remove the reporting process's opcode-table
  assumption for this slice. Carry out the draft's validation **per input
  file before summing counters**, and repeat it when loading exported JSON.
  The current directory loader (`summarize_stats.py:90–108`) adds every
  integer, which would multiply handler IDs/flags and coverage version by
  the file count. Keep one validated metadata/coverage object per cohort;
  reject missing, duplicate or conflicting facts, unsupported coverage,
  absent referenced bases, and inconsistent files. Add a positive with two
  identical-metadata files whose counts sum while IDs/flags/version stay
  unchanged, plus a one-file mismatch control. This realizes the existing
  draft requirement rather than expanding sampler-transport scope.
- Preserve the counter before `IR_PREDICTED` in both emission modes. Add a
  direct regression/control for moving it past that label; the planned
  `inst.name` control detects identity loss but would not detect this
  lifecycle change. Include initial CACHE-0, disabled-phase last-entry
  updates, clear and a call/frame transition in the pair expectations.
  Source admission must permit the initial CACHE-0 source sentinel even
  though no CACHE body is executed by the private fixture.
- Record exact file paths for the isolated extension and reader tests at
  the source handoff. HandlerDomain-only test hunks and the benchmark
  reader/docs still require their stated ownership grants. Do not edit
  ROOTS checker imports, fixtures, shared CAPI blocks or H1 frame-passing
  interfaces to accommodate the fixture.

## Findings that hold

The counter placement in `ir_generator.py:1805–1807` and the existing
`lastopcode` lifecycle support the draft's counted-handler-entry wording.
Changing only the emitted counter argument preserves semantic `opcode`,
box attribution and shared CALL behavior. Both pair axes must widen with
the row array; IR_JUMP adds coverage and changes neighbors explicitly.

The owning interpreter allocates one aggregate in the startup or lazy
enable path, threads attach to it, clear uses `sizeof(PyStats)`, and
`pystate.c:464–466` frees it. The empty merge/thread wrappers do not establish
a per-thread merge or free-threaded contract. The proposed dense matrix at
the actual generated count is a focused change. The stated +205,344 pair
bytes plus +25,024 other row bytes equals +230,368 per enabled interpreter
at the stated source widths. It remains arithmetic until the matching
build reports actual sizes and successful request sizes; it establishes
neither allocator overhead nor RSS.

ADAPTIVE as own feedback plus an analyzed specializing uop is the right
orthogonal property. Placeholder slice uops lack feedback use; GET_ITER now
has a real feedback-driven specializer and should be classified from that
fact. Truth branches have real specializing uops; ordinary branch-history
storage alone is insufficient. Resolve per-NB targets through their shared
instruction and actual format, and exclude NOFB, metadata-only heads,
specialized members and instrumentation wrappers as specified. Neither
cache width nor base inequality establishes these properties.

The producer-supplied IDs/base/flags, coverage version and raw/family
separation are sufficient directions for a local statistics schema; the
older failure-text source dependency is stated honestly. Remote sampler
identity/transport remains stage 3. Renumbering, threshold removal,
inspection/compiler closure and typed operations remain separate slices.

The coordinated `dispatches` to `counted_handler_entries` rename should
update the runner and its documentation together, with coverage metadata
and no fallback. The source search supports the draft's bounded reader
inventory; retained historical result files keep their earlier identities.

Implementation remains deferred until ROOTS settles and the coordinator
hands over the integrated source and ownership. Frozen H3 lacks queue052
ancestry; neither this review nor its retained storage evidence qualifies
the future union. Close the reporting contract above, then obtain the
bounded implementation handoff. Held Queue2 work is outside this review.

## Round 2: revised design disposition

Verdict: **IMPLEMENT**, subject to the existing integrated-source and
ownership handoff. No design blocker remains in revised
`H3-1-stage2b3.md`, SHA256
`c01c82d2fc95c8a5fa2b6441324fbdbcc1ca1ebfcdbd5fbe9a1efa9d8873563f`,
committed as docs-only `81417aff0971fe621fec296a324c7a48315a6082`.
This supersedes round 1's design verdict without changing its history.
It grants design readiness only: next H3 native edits, builds and execution
remain deferred until ROOTS settles and the coordinator hands over the
actual integrated source, build and file ownership.

The preserved `controls/H3-1-stage2b3/design-superseded-v1.md` hashes to
the reviewed first draft, `141e85f1f4a407cc3183380c9c90c7080dab1b49f860c9c25cf4af46d3318f14`.
The H3 worktree is clean at the revised commit. Its native/source diff
against frozen storage candidate `ffc548603b823484fdf106b1a9de06785017c682`
is empty across Python, Include, Tools, Lib and Modules. This recheck used
source reads and Git metadata only; no native/build/test/probe execution or
H3 edit occurred. README, PROCESS and WORKLOG were not changed.

| Round-1 finding | Round-2 disposition |
|---|---|
| Overlapping family denominator | Closed: R and F contain raw entries only. The old `execution_count + hit + miss` denominator and raw miss-ratio column are removed. The 10/90 fixture requires 100, with its named old-formula control. |
| Base map cannot recover truth-event attribution | Closed: recorded events remain at their recorded keys; TO_BOOL hits stay at TO_BOOL even with no TO_BOOL entries. Family entry totals and semantic events are separate tables. No unsupported hit/deferred execution shares or counter-site rewrite. |
| Specialized-miss subtraction lacked an entry relationship | Closed: disjoint ADAPTIVE/SPECIALIZED/other totals sum to R. Guard-failure entries remain SPECIALIZED; no miss subtraction or inferred successful-entry count. |
| Boxes/metadata could enter event rendering | Closed: four explicit event fields, separate attempt-outcome table, and a mixed-field rendering fixture. |
| Metadata/coverage must survive aggregation unchanged | Closed: per-file validation before addition, identical cohort facts, separate nonadditive metadata, duplicate-key rejection for JSON and matching JSON validation. Two-file positive and conflict/missing/duplicate controls are explicit. |
| Predicted-entry and pair lifecycle controls | Closed with the dispatch-mode correction below: numeric normal-mode ordering control, actual-tailcall qualification limit, CACHE-0, disabled updates, clear, same-activation and recursive-activation expectations. |
| Exact fixture ownership and scope | Closed as a design boundary: reader/fixture paths are named, shared hunks still need grants, private sequence evidence is distinguished from live frame qualification, and ROOTS/CAPI/H1 edits remain excluded. |

The author's dispatch-mode correction is confirmed by source. At
`Python/ceval_macros.h:146,153`, computed-goto and switch
`JUMP_TO_PREDICTED` use `goto PREDICTED_name`. The generated target's
`INSTRUCTION_STATS` precedes `IR_PREDICTED`
(`Tools/cases_generator/ir_generator.py:1805–1807`), so those transfers
bypass the target-entry counter. At `ceval_macros.h:131–138`, tailcall
`JUMP_TO_PREDICTED` calls `_TAIL_CALL_name` at function entry with
`this_instr`, and the stats form forwards `lastopcode`. That target
therefore counts again and advances the pair sequence. Counter-before-label
placement is common; bypass behavior is not. Round-1 references to
predicted-label bypass must be read as the non-tailcall behavior only.

The revised normal guard fixture correctly has R=1, S=1; its tailcall
counterpart has R=2, S=1, A=1 when the fallback target is adaptive. Mode
is now producer metadata, homogeneous within a cohort and required to match
across compared cohorts. Preserve both native behaviors; do not make
tailcall skip its entry counter to match the normal fixture. A normal
extracted-span control proves only that mode's ordering behavior; an actual
matching tailcall Py_STATS build is required for any tailcall-runtime claim.

At the coordinated benchmark-reader handoff, retain dispatch mode along
with coverage in result metadata and validate both sampled files before
differencing counted entries. This follows the revised producer/comparison
contract; it does not authorize new measurements or a historical-result
rewrite.

The original storage widths, one-aggregate-per-interpreter ownership,
ADAPTIVE property, source-only memory arithmetic, separate sampler slice and
no-fallback reader rename remain sound. The existing H3 candidate still
lacks queue052 ancestry; design clearance is not native-union qualification
or landing approval. No held Queue2 work was attempted.
