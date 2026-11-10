# Ordinary calls: architecture review

**Proceed with direct class-scalar lookup, then class-receiver classmethod
forwarding as two bounded increments. Make complete conditional leaf activation
elision the next structural experiment. Do not begin a frame-layout migration.**
This revises the proposed A/B order and rejects a round spent accumulating tiny
constant/getter recipes. The proposal's owned call window and real continuation
are the right foundation; another call ABI is unnecessary.

This is the single broad design review, based on runtime46
`b8e7e1bee6611eeda320ce163cd3ce89323da877` in the execution-boundaries worktree,
the canonical design pages, both local audits, and retained46 evidence. No runtime
edits, builds or measurements were performed. The
[CPython AI policy](https://devguide.python.org/getting-started/ai-tools/) was read.
Subsequent component design, implementation and evidence reviews belong to Sol;
integration and measurement belong to the coordinator.

## Ranked recommendations

1. **Extend canonical class-scalar lookup first (B).** This closes an existing
   representation coverage gap using the independent guarded-word field, rather
   than adding a new cache or frame model. Each conditional selector performs
   this read. The semantic census records 25,424 selector calls out of 51,422
   (49.44%), so this is the best-supported small first increment. It removes
   generic lookup, not activation. Prefer one canonical reader: generalize
   existing `LOAD_ATTR_CLASS`/`LOAD_ATTR_CLASS_WITH_METACLASS_CHECK` to acquire
   both admitted objects and inline scalars from `class_value`. Preserve existing
   object classification and metaclass-guarded coverage. Newly admitted inline
   cases may initially require exactly builtin `type`, with existing MRO/version
   and descriptor-precedence proofs. Acquire before receiver retirement and
   update every producer, snapshot/replay/reset consumer together. Preserve
   ATTR48/POLY120 geometry; scalar coverage alone needs no new handler. Sol must
   check serialized initialization compatibility before deciding whether a magic
   change is needed. Global rebinding remains visible through the
   preceding global load. Risk: accepting a class version as proof of metaclass
   precedence, or loading stale guarded bits before validating their owner.
   Moving object payloads to guarded words does not authorize new descriptor
   kinds: the class dictionary must still own the admitted cached value.

2. **Forward classmethods through the existing owned pair (A).** Approve exact
   builtin classmethod descriptors on class receivers with builtin metaclass,
   in method-form lookup, with a current exact Python function. Live-read and
   acquire `cm_callable` at lookup; `classmethod.__init__` can change it without
   a class-version change. Transfer the actual accessed class as receiver,
   including inherited binding. Argument evaluation may then mutate descriptors,
   bases or names; CALL must use the captured pair while checking the captured
   function's current code/provider/binding facts. This removes wrapper creation
   and unwrap, but no callee activation. The three classmethod functions total
   2,717 semantic calls (5.28%); this is useful work, not the main call-volume
   strategy. Attribute-form retrieval, custom metaclasses, descriptor subclasses,
   arbitrary wrapped callables and instance receivers retain ordinary lookup.
   Risk: a cached function pointer quietly replacing live descriptor semantics.

3. **Target complete guarded leaf regions, including conditional selection (C).**
   This has the largest demonstrated structural reach: eliminate reservation,
   activation initialization, independent activation code-owner traffic, entry
   and return dispatch, and child cleanup together. Exact calls already overlap
   argument homes and reuse initialized storage; normal retirement already
   avoids sweeping temporaries. There is no further full-copy/full-sweep win to
   count. Constant/no-op and single-getter candidates together cover only 1.56%
   of observed calls. They are certificate test cases, not a sequence of separate
   optimization projects. Approve a bounded design/prototype for the region
   contract below, subject to Sol's missing proofs; this review does not certify
   an unimplemented frame-elision path. Risk: repeated dynamic guards and a new
   evaluator consume the saving, or a supposedly pure read invokes Python.

4. **Share the ordinary completion contract before adding a second consumer.**
   A leaf result must use the same recipient and continuation semantics as an
   ordinary return. Extract only the needed completion/refresh logic and an
   explicit input-retirement adapter. Keep the result owned through retirement,
   detach consumed input homes once, preserve callback-current locals, and honor
   redirects without replaying the completed call. Do not import native-call
   periodic-at-end behavior or a universal transaction record. This prerequisite
   is ranked separately because ownership bugs here outweigh a faster body.
   Exact destructor timing is not the contract; valid roots, bounded retention,
   coherent public state and once-only effects are.

5. **Make IR facts authoritative at the certification boundary (E).** Use final
   immutable IR, its complete control flow and def-use relationships, and shared
   operand/effect definitions. A `pure` marker or `_PyCode_ReturnsOnlyNone` is
   insufficient. Mutable executing handlers and warm feedback are predictions,
   not certificates. Add only facts needed by admitted operations, with a
   positive checked mapping to their guarded implementations. Avoid a general
   effects framework whose first consumer has not removed work. Code construction
   and `code.replace` must produce fresh verified metadata; frontend recognition
   alone cannot certify altered code objects.

6. **Keep ordinary activations coherent; defer hot/cold layout surgery (D).**
   Share a resolved function/code/binding view where it removes real duplicate
   preparation, then commit through the existing overlap ABI. Retain the
   independent code owner in every executing frame: mutable `function.__code__`
   makes a function owner alone insufficient. Moving globals/builtins or header
   fields behind accessors may replace cheap stores with repeated loads throughout
   execution. The Python frame object is already lazy. A further split requires
   an all-reader census and evidence of net saved work, not merely a smaller
   header. Do not introduce per-opcode slot-validity bookkeeping, a borrowed
   frame bank, or a second frame ABI. Revisit only if leaf admission is low or
   remaining measured setup/retirement dominates.

7. **Unify adapters by contract, not by forcing every descriptor into one path.**
   Classmethods can supply the existing function/receiver pair. A property read
   invokes a getter before any later call of its result; it is a different
   semantic boundary. A future property consumer may reuse the leaf certificate
   and completion primitives after its own live-getter/lifetime proof
   (`property.__init__` can replace the getter). DeltaBlue has no properties.
   Defer property expansion, instance-classmethod retirement complexity, keywords,
   coroutine entry, GC redesign and native JIT tiers from this experiment.

## Chosen abstraction: certified leaf region over a captured call

Keep `(owned callable, owned receiver-or-NULL, owned arguments...)`. Add immutable
per-code **region metadata**, not a per-invocation heap object or copied argument
array. It describes a bounded acyclic value graph over argument/constant inputs,
guarded instance/global/class reads, exact scalar comparison, conditional choice
and return. Conditional choice follows the selected path; it must never execute
an unchosen descriptor or operator. Explicit limits on nodes, depth and scratch
are implementation budgets, not this benchmark's nine-word signature.

The certificate covers the complete executable body after lowering/compaction:
all paths, definitions, returns, entry prefixes, exception regions and owner
effects. Initially reject loops, calls, writes, cells/freevars, handlers,
allocation and any operation without a callback-free guarded implementation.
Accept exact positional binding with all arguments supplied first. Constant and
argument returns are degenerate regions in this one representation.

The interpreter needs three phases:

1. **Admit and probe without effects.** Check captured function/provider, current
   selected code, effective arity, recursion and entry-work/observation eligibility.
   Preserve current stack-space admission initially. Walk the selected region
   using callback-free guarded reads and comparisons. Every intermediate borrow
   needs an original live owner chain; no decrement, allocation, instrumentation
   callback or generic descriptor/operator may occur. A failed guard leaves the
   call window untouched and executes the ordinary callee once, producing its
   normal errors and traceback. Read-only probes may be repeated on that miss.
2. **Acquire and commit.** Acquire the final canonical result while its protecting
   owners are live. Commit consumption of the original call window, publish a
   valid caller continuation, and forget every borrowed region/code/cache view
   before the first callback-capable release. No fallback is allowed afterward.
3. **Retire and complete.** Hold the result through input retirement, refresh the
   actual caller state, then publish/discard through ordinary completion. Errors
   or redirects during retirement use that completion's ownership rules, never
   replay lookup, arguments or the body. Hooks installed during retirement affect
   subsequent execution, not an invented replay of the completed leaf.

Crucially, **immutable code shape does not freeze its environment**. Functions
sharing code may have different globals/builtins and defaults. Region metadata
contains indices and operations; resolve globals through the captured function's
actual mappings and live guards. Class/field values require current version,
shape, presence and descriptor proofs. Exact scalar comparison must exclude
Python operator dispatch. Shared code/version alone cannot identify these values.

Use shared callback-free probe primitives for ordinary handlers and region reads
where practical. Do not call full attribute handlers: their publication and
retirement may run callbacks. A bounded region evaluator still adds execution
overhead; account for it honestly. Reject a growing general opcode dispatcher,
handwritten getter/selector pattern catalogue or per-function generated C. The
region metadata can serve a future compiler, but this experiment needs neither
SSA, OSR nor general native deoptimization maps. All recovery is a pre-effect
miss to the existing interpreter; supported observation simply declines elision.

## Sequence, remaining proofs and stopping rules

**B then A are approved to implement under the scoped contracts above**, with
independent Sol proposal/code review and coordinator integration. Keep them
separate enough to identify their effects. Their source anchors are
`specialize_class_load_attr`, the class-attribute DSL handlers and feedback
consumers; neither needs a frame change. Check inherited mutations, metaclass
precedence, scalar-to-descriptor transitions, descriptor reinitialization before
and during argument evaluation, function code/default changes, escaping attribute
retrieval, publication/retirement reentry and unchanged feedback geometry.

Next, Sol should settle two concrete interfaces before implementing C:

- A complete entry admission predicate matching RESUME/skip semantics: pending
  eval work, instrumentation epochs, local/global monitoring, tracing/profiling,
  PEP 523 and enabled DTrace/LLTrace. Define supported external observation;
  `frame_obj == NULL` alone proves nothing. Cold metadata preparation must not
  allocate inside the callback-free probe interval.
- A reviewed completion adapter for local, temporary, discard and aliasing
  destinations; input/function/result lifetime; retained callers; callback local
  writes, redirects and errors. Prove every code/environment pointer is dead
  before retirement, or explicitly owned across it.

Sol also owns complete-IR rejection tests and mutation/owner proofs for each
probe primitive. In particular, exercise same-code functions with different
globals, substituted comparison objects, missing selected fields, descriptor
replacement, recursion boundaries and observers enabled between calls. Use
general Python examples, not benchmark names as eligibility rules.

Record eligible-region hits and miss reasons separately from semantic call
counts. The 49.44% figure is a candidate-family ceiling, not admitted coverage,
saved time or a 2× forecast. Stop expanding C if observation or ownership demands
a general reconstruction framework, if it becomes a second Python interpreter,
or if measured guard/evaluator work erases the activation saving. Retain useful
A/B progress and revisit remaining ordinary activation costs instead.

Measure unchanged DeltaBlue first, with logging and a small varied control set;
broaden at a coherent batch boundary. Compare each useful checkpoint to the
moving development baseline while retaining immutable controls. Mostly-better
increments may land with explicit measured regressions and limitations; universal
improvement is not required. Do not double-count class-scalar savings if a leaf
region later subsumes that execution. The baseline remains 1.63233× upstream
elapsed time; only actual application measurements establish progress toward
parity.
