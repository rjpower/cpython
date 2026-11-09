# Generator phase/output contract: source inventory

2026-11-07. Read-only implementation map for runtime `7780e32e0e2`. Sol owns
the shared contract proposal; this note records current producer/consumer
boundaries and a focused test plan. It does not select the semantic flag
encoding or authorize edits to shared IR headers/generated products.

## Current producer and consumers

- `Python/bytecodes.c`: five specialized attribute loads call
  `IR_RETIRE_PUBLISHED(receiver)` after assigning their `attr` output. Their
  actual shape, slot, module, inline-value and hint lookups differ; one shared
  producer annotation should state only the result/retirement contract they
  have in common. Store attribute retirement flows are distinct and keep their
  own synchronization markers.
- `Tools/cases_generator/ir_generator.py`: `IREmitter.retire_published()`
  validates retired views, clears inputs, flushes, computes `failed_result`
  from a five-name handler test, emits retirement/error handling and refreshes
  state after a possible callback. The same name test sets
  `IRStack.published_temporary_result`; `IRStack.store()` uses this boolean at
  final destination emission. `complete_attr_reader()` sets the same boolean
  from a separate helper postcondition. `IRStack.copy()` inherits the boolean
  through `Stack.copy()`'s shallow object copy, so branch copies retain it.
  `IRStack.merge()` does not reconcile the boolean with the other incoming
  path; a semantic fact must either merge only when both paths establish the
  same output/home postcondition or reject conditional-marker joins.
- `Tools/cases_generator/analyzer.py`: `Properties.sync_sp` recognizes both
  retirement markers. No producer-result identity, phase, output-home,
  failure-root or callback postcondition is represented there. Error and
  release classification already recognizes retirement macros.
- `Tools/cases_generator/ir_metadata_generator.py`: generated word metadata
  includes effects, operand roles, read/write/through extents, and flags from
  `IRFormat`; it currently has no result publication or output-home fact.
  Any new metadata shape/field needs coordinator ownership of the shared
  runtime header and generated products.
- `Lib/test/test_generated_cases.py`: `TestIRCases.test_retirement_flush_reuses_published_temporary`
  checks five handler names, early temp publication, callback failure root,
  companion NULL output and folded-local final write. The adjacent escape test
  checks that no callback-producing operation slips between `OWNER()` and
  retirement. This is a generator-structure test; it does not execute the
  emitted C. Integration C separately ran the release/debug receiver-retirement
  semantic fixture covering reentry, finalization and local/temp destinations.

## Fact obligations for the proposed contract

The shared producer fact must identify the result output and retirement input
at the source operation, then let the analyzer bind them to a concrete handler
and its `DST` role. It needs to make these phase differences explicit:

1. Before retirement, a temporary destination has received the owning,
   canonical result in its frame home. A folded-local destination keeps the
   result in the C local, since replacing the old local can release it.
2. Receiver retirement may close synchronously or call out and reenter. The
   callback/error helper receives the result as `failed_result` only when the
   folded-local result is C-owned; the temporary result is already rooted in
   its home. Callback return refreshes instruction/frame/slot state.
3. On successful completion, only the folded-local path still needs the final
   destination store. It must happen after retirement and retain displaced
   local release/error order. On retirement failure, preserve error cleanup of
   the C-owned local output and the frame-rooted temporary output's normal
   unwind behavior.
4. The fact cannot authorize use after output reassignment, destination/home
   change, intervening escaping operation, or a handler path that does not
   establish the stated postcondition. Analyzer rejection should be tied to
   producer/control-flow semantics, not five opcode names.

The exact representation of success/error/callback facts is pending Sol's
proposal. Do not conflate a producer annotation with runtime proof that a
callback happened; the marker describes generated path semantics and the
emitter still has to guard the conditional retirement call correctly.

## Minimal test plan after interface approval

- Positive generator fixture over the five DSL producers: the declared result
  is the handler output, metadata reports synchronization and the reviewed
  output-phase fact, and generated C publishes a temporary before retirement
  while retaining the final folded-local write after retirement.
- Positive failure-root assertions: local results pass the result owner to
  the retirement helper; temporary results pass NULL because the value is
  already in the home. Both paths retain failure cleanup and companion output
  initialization.
- Negative analyzer/generator cases: result is not an output, receiver is not
  a consumed owner, marker is duplicate, marker follows an escaping callback,
  result/destination is changed after the marker, and incompatible branch
  paths merge. Each must fail before C generation.
- Preserve existing `OWNER()`/retire escape-rejection and check a pair-retire
  store handler still infers `sync_sp` without acquiring load-result facts.
- Re-run only `Lib/test/test_generated_cases.py` (and a narrow fixture for
  analyzer/metadata serialization if separated). Root owns regeneration,
  shared-header compilation, semantic runtime checks, and integration builds.

## Evidence boundary and candidate work

The final profile reports shape-attribute load self shares of 10.73% in
deltablue, 5.44% in logging_silent and 4.81% in comprehensions; method
attribute-load self shares are 5.95% and 6.32% in deltablue and logging_silent.
These samples motivate selecting attribute work but are not handler hit counts
or isolated helper costs. Retained GCC `_TAIL_CALL_LOAD_ATTR_SHAPE` disassembly
shows a guarded lookup, one `IR_STORE_TEMP_DST` publication path, conditional
`retire_published_values`, refreshed word/slot loads after that helper, and a
separate local destination path. Integration C already removes the second
temporary store by reusing the published-result postcondition. This batch's
first change should make that established result fact semantic and checkable;
it must not claim a second runtime deletion or throughput benefit without a
new producer-backed duplicate, actual GCC diff, and matched measurement.

The final report has the complete source/profile caveats and code-size facts:
[interpreter throughput report](../../bench/results/interpreter-throughput-20261106.md).
