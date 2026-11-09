# One statically owned attribute reader

Proposed 2026-11-08; runtime baseline `0765e9be47f`. No runtime edits yet.

## Decision to review

Add one execution-only handler, `LOAD_ATTR_SHAPE_LOCAL_TEMP`, selected when
`LOAD_ATTR_SHAPE` specializes a word whose receiver B names a local and whose
destination A names a temporary. Keep the 64-bit word and four operand fields
unchanged. Keep generic, temporary-receiver and folded-local-destination paths
unchanged. Selection happens once at specialization after slot allocation; it
adds no per-execution mode table lookup or dispatch guard. One variant, not a
product over the LOAD_ATTR family.

The retained `warmed-ir.json` is an older `0953757f4e7` capture. Its counts agree
with the final `0267d428808` `warmed-shape-comparison/candidate-frame-borrows.txt`
for the admitted shape:

| Function | Local receiver / temp destination sites | Warm executions per logging outer loop |
| --- | ---: | ---: |
| bench_silent: LOAD_ATTR_METHOD_WITH_VALUES | 10 | 10 |
| Logger.debug: LOAD_ATTR_METHOD_WITH_VALUES | 1 | 10 |
| Logger.isEnabledFor: LOAD_ATTR_SHAPE | 2 | 20 |
| Logger.isEnabledFor: BINARY_OP_SUBSCR_DICT | 0 (temp dict / local key) | 10 |

`isEnabledFor.disabled` and `isEnabledFor._cache` are both reached on the warmed
silent path. The existing method reader has only one input class comparison;
the shape reader also conditionally publishes its output and retains a possible
receiver-retirement path. It is a better bounded first experiment. Final spectral
IR contains no LOAD_ATTR_SHAPE site in its scalar kernel functions; handler text
and layout can still affect it.

## Removable work and limits

At the C level each admitted execution can replace input consumption with
`PyValue_NULL`, publish the already canonical result unconditionally to its
temporary, eliminate conditional local-output publication, and prove that there
is no receiver owner to release. This removes two distinct slot-class decisions
(B and A); repeated source occurrences may already share one compiled decision.
The non-deallocating receiver path becomes a compile-time fact. The budget is
40 slot-class decisions per ten-debug outer loop, not 40 retired branches or a
predicted percentage speedup. Type-version, shape, value-tag/canonical checks,
result duplication, feedback, and dispatch remain. Inspect baseline and candidate
GCC assembly before crediting savings. Reject if this only renames macros or
adds equivalent selector work on every execution.

## Producer and generated interfaces

Runtime source belongs to the separate `operand-modes-20261108` worktree.
Canonical documentation is this directory. Planned edits:

- `Tools/cases_generator/ir_generator.py`: one explicit variant descriptor
  mapping the new handler name to LOAD_ATTR_SHAPE and field modes `{B: local,
  A: temp}`; inherit its instruction, format, contracts and guards. Generate
  consumption/publication from those static modes. Validate descriptor fields
  against the base format; do not infer purity or canonical facts from a name.
- `Tools/cases_generator/ir_numbers.txt`: reserved IR-only handler number 319 (boundary experiment reserves 320–322).
  `handler_ids()` exposes it, so the existing numbers checker can validate it.
  Pristine bytecode never contains it; no serialized layout changes.
- `ir_metadata_generator.py`: variant shadow is LOAD_ATTR_SHAPE; canonical base
  is LOAD_ATTR; flags include SPECIALIZED, DISPATCH, METADATA and IR_ONLY,
  excluding PRISTINE/ADAPTIVE. It inherits the base format and conservative
  effects/result-publication contract. Name/domain export includes the variant.
- `Python/specialize.c`: after choosing LOAD_ATTR_SHAPE, compare B/A against
  `current_frame()` code nlocalsplus once and select the variant. Preserve
  feedback, success accounting and fused selection. Its canonical opcode is
  LOAD_ATTR_SHAPE wherever an eight-bit opcode table is indexed; the physical
  handler can exceed 255. Normalize this physical variant to LOAD_ATTR_SHAPE in both feedback row insertion and incoming record comparison. `fb_begin_ex()` saves the physical handler directly; failure to normalize that route would hide shape alternation from `fb_saw_other_shape()`. Preserve existing fused handler identities.
- Regenerate cases, targets, IR metadata, Python opcode metadata and any changed
  generated opcode products using their normal producers.
- Add `Lib/test/test_ir_operand_modes.py` for runtime behavior and focused
  generator/metadata assertions; extend existing contract tests only if needed.

The generated C handler retains `opcode = LOAD_ATTR_SHAPE`, so deopt assertions
and fixed 256-opcode stats stay in their existing domain. INSTRUCTION_STATS uses
the canonical specialized opcode. Its physical IR handler has a generated full
IR domain row and dispatch target. A warmed IR check proves the variant is
selected; aggregate stats cannot independently distinguish its hits.

## Deoptimization, observers, and ownership

All guards execute before consumption or result creation. A guard failure tail
calls generic LOAD_ATTR on the same untouched word/fields and receiver local.
`shadow` maps to the canonical specialized opcode for disassembly, feedback,
base-opcode reconstruction, and instrumentation. Instrumentation resets to the
ordinary family path; transparent line/instruction wrappers execute the pristine
generic word, as today. Removing instrumentation permits later specialization
to select the variant again. There is no new instrumented twin or new event.

A local receiver remains owned by its frame; the handler borrows it without
clearing or incrementing that edge. The shape/version guards and canonical
home duplication are unchanged. The result is owned and canonical before
publication to the previously NULL temporary. The local receiver cannot alias
that temporary because the two index domains are disjoint. Other aliases in
object graphs remain ordinary counted edges. No receiver retirement can run a
finalizer on this path, and no displaced local exists. Canonical result
publication still precedes the existing optional NULL output. Generic and
folded-local destination paths retain their delayed finalizer ordering.

Any callback/error that occurs during an unsuccessful guard follows the
unchanged generic path before consumption. No new fallible operation is added.
Existing instruction/line events, exception ranges, jumps, frame traversal,
lasti and f_lineno use unchanged words and unchanged operand formats. Neither
boundary workstream depends on this variant; integration must review their
shared generator diff and combined instrumentation behavior.

## Gates and scope cut

1. Independent design review; reconcile findings before implementation.
2. Host generator/number checks and focused generator tests: exactly one variant,
   same format/contract, canonical shadow/base, no opcode-table out-of-bounds
   indexing, and generated static local/temp operations.
3. Coordinator release/debug compile and runtime cases: warmed selection,
   temporary receiver, folded local result, receiver/result aliases, class/shape
   mutation and deletion, descriptors, weakref/finalizer order, guard failure,
   tracing and monitoring toggles, exception cleanup and canonical value kinds.
4. Independent code/evidence review, including release compile and generated
   products; coordinator integrates and inspects GCC instruction/text delta.
5. Logging versus golden/upstream and broad controls including spectral. Keep
   only an observed useful improvement with declared regressions and limits.

If the implementation requires broad opcode-domain changes or cannot remove
compiled ownership work, stop with this census and report the failed budget.
No native builds, benchmark runs or commits by this worker.
