# Cold review of integrated B+A

**No must-fix correctness or ownership finding. No required abstraction cleanup.**
Reviewed actual runtime `5559f60093e8216d9399bea74d0f10c8cdfb9a6b`
against `b8e7e1bee6611eeda320ce163cd3ce89323da877`; A's production
change is `bfc9fe1aeff`. This review covers canonical class lookup through the
owned function/class pair into ordinary calls. It excludes the unfinished leaf
prototype, performance qualification and a new full-suite claim.

Read applicable repository instructions, the CPython AI policy, canonical
README/STATUS/GOAL/HANDOFF/PROCESS, grouped architecture review, integration47
and A proposal. Inspected actual source, generated bodies and metadata, all
twelve classmethod tests, and B's mutation/lifetime tests. No runtime edits,
regeneration, native jobs, builds or commits were performed by this reviewer.

## Contract and concrete witnesses

- **B reads canonical values under the existing lookup proof.**
  `Python/specialize.c:1558–1679` keeps metaclass provider/precedence and class
  version checks, mutable enum-like restrictions and existing descriptor
  classification. INLINE_VALUE joins the admitted class dictionary values;
  descriptor kinds do not broaden. Both generated readers validate versions
  before loading `class_value`, acquire with `IR_CANONICAL_HOME_DUP`, and use
  the established publication/retirement adapter. At
  `Python/bytecodes.c:2805–2826`, plain function access still supplies NULL
  self. The generated code publishes temporary results before receiver release,
  protects local/discard results on the error path and reloads instruction/slot
  state after callbacks. The new displaced-local test removes the class value
  during reentry while the returned list remains independently owned.
- **A caches the descriptor, not its mutable target.**
  `Python/specialize.c:1637–1677` admits exact builtin classmethods, exact Python
  function targets, builtin-metaclass class receivers and method-form lookup.
  `Python/bytecodes.c:2828–2859` repeats the receiver and class-version guards
  before interpreting guarded bits. `_PyClassMethod_GetFunc` is a field reader
  (`Objects/funcobject.c:2052`); the handler checks its live target before either
  owner changes. This matches `cm_set_callable`: same-object reinitialization
  changes `cm_callable` without a class-version change. The before/during-argument
  reinitialization test is the relevant witness; a cached function pointer would
  fail it.
- **The pair owns the function and the accessed class.** The generated
  `LOAD_ATTR_CLASSMETHOD` duplicates the live function, consumes the receiver
  home and uses `IR_TAKE`; local receivers are duplicated and temporary receivers
  transfer. Both output homes are written without callback-capable receiver
  retirement. Inherited lookup binds the accessed subclass. Argument evaluation
  can delete attributes, change bases or reinitialize the descriptor without
  invalidating this captured pair. The tests cover those mutations, weakrefs
  after dropping other owners, argument failure and escaping attribute access.
- **CALL does not re-resolve the descriptor.**
  `Python/bytecodes.c:4222–4240, 4303–4398` uses the captured function, current
  function version/code and effective arity, with the existing general binding,
  stack, recursion, PEP 523 and frame entry paths. A code/default mutation during
  argument evaluation therefore follows ordinary CALL guards and fallback.
  Exact/general-call tests and profile/monitoring tests exercise the two intended
  routes. A removes a wrapper allocation/unwrap; it does not remove activation,
  observation, binding or ordinary completion.
- **Feedback and generated contracts agree.** `fb_row_of` copies guarded words
  for all three class handlers; its `aux` field aliases `class_version` through
  the existing feedback union. Reset clears the full entry. Historical rows are
  observations and never executable target owners. Both B handlers retain their
  result-before-retirement phase and optional NULL suffix. A writes a fixed
  two-home window, borrows one input and has no result-retirement marker.
  The audited nonescaping readers explain the removed ESCAPES effect; A does
  not call Python on a hit. Conservative ALLOCATES metadata is shared with
  existing method pair handlers and is not a new effect claim to optimize here.
  Slot111 is revised explicitly with magic64418, generated base LOAD_ATTR and
  unchanged feedback geometry. The two corrected import fixtures pin that format.

## Redundant work and scope

There are two small redundant checks in A: `_CHECK_ATTR_CLASS` checks
`PyType_Check` after the exact builtin-metaclass guard, and line2844 checks the
exact descriptor type already established by producer plus class version.
Neither is a correctness gap or a required cleanup. The shared class guard is
preferable to duplicating version logic for this increment; the descriptor check
can become an assertion if subsequent measured work warrants it. The live target
NULL/function checks cannot be removed: descriptor reinitialization is independent
of the class version. Ordinary CALL's function checks also remain necessary
because explicit arguments run between lookup and invocation.

I found no useful case for a new resolved-call object, binder or frame interface
in B+A. The existing owned pair is the shared abstraction. Leaf-region completion
and effects must receive their own actual implementation review later.

## Retained validation evidence and limits

Evidence is under `BASE/pyjit-link-wave1/deltablue-call-model-20261110`.
Read `delta47-{build,focus,lint}-checks.json` and integration47: both B builds
and 571 focused tests per build passed, API306 clean. At review readback,
`delta48b-build-checks.json` records successful release and debug builds at clean
`5559f60093e`. The initial A release run passed thirteen modules, including all
twelve new classmethod tests; its failures were the two stale import-magic
fixtures and the known trace errors. Do not call that run green.

The corrected release import/classmethod rerun returned0.
`delta48b-debug-focus-checks.json` and its log record SUCCESS for all14 modules,
867 tests run and13 skipped. `delta48b-lint-checks.json` and the retained lint
log record306 units, zero failing findings and zero parse errors. These are
coordinator-run results inspected by this reviewer, including the required real
release compilation. No final A elapsed-time result or full B+A suite was
available in the reviewed evidence. Integration/performance decisions remain
with the coordinator.
