# Generator/coroutine normal-path cost review at source32

Read-only comparison of current `488e85fa3a8` with immutable golden
`0765e9be47f`. Root's two-pair screen reports generators1.07753 and
coroutines1.07886. These elapsed ratios identify cases to investigate; this
source review does not assign their cost to a particular change. No candidate,
benchmark or build was executed by A.

## Workload routes narrow the common candidates

`bench/internal/kernels/generators.py` traverses a 100000-node binary tree using
recursive `yield from`; each tree iterator is created and completed, while values
are forwarded through multiple delegation levels. Its normal path exercises
VM FOR_ITER/SEND entry, yields, and generator completion.

`coroutines.py` recursively awaits Fibonacci children without an external
suspension point. One root `coro.send(None)` drives the whole recursive result;
the meaningful repeated operation is child completion, not a native `.send`
for every recursive child. From the unchanged source recurrence, one
`fibonacci(25)` evaluation creates242785 coroutine calls with242784 await edges,
assuming normal completion. Those are algorithmic counts, not measured handler
or instruction counts. The common regression therefore directs attention first
to creation, VM resume and return/completion. YIELD-only work cannot explain
both cases by itself.

## Structural differences worth profiling

1. **Shared completion commit.** Current generated RETURN_VALUE keeps the
   canonical result C-owned through frame retirement, then calls
   `_PyEval_CompleteGeneratorReturn`. For a VM parent it decodes the actual
   SEND/FOR_ITER and invokes `_PyEval_CompleteStep`. That generic eight-argument
   helper canonicalizes the result, detaches the outcome window, optionally
   publishes a destination, records continuation and retires up to six held
   owners. Kind tests cover FOR, SEND and THROW; unobserved execution skips the
   observer calls, not the owner commit. Golden's `_PyEval_GeneratorReturnTarget`
   selected a continuation and the resulting stream dispatched its cleanup
   operations. Current semantics remove those dispatches and fix completion
   ownership. Net work must be measured across both paths, not inferred from
   the helper's source size. Profile call counts, normal SEND completion's
   actual instruction path, register/stack argument traffic and receiver close.

2. **VM resume commit.** Current SEND_GEN detaches its sent input and installs
   caller continuation, child previous/current and exception links before
   disposing an ignored resume input. It checks CREATED/non-None admission and
   `_Py_EnterRecursivePy` before entering the child. Golden's generated
   `_PUSH_FRAME` decremented `py_recursion_remaining` without this immediate
   checked branch. This is a correctness constraint, not permission to remove
   the check. Inspect normal SEND_GEN and FOR_ITER_GEN routes for repeated frame
   state loads, destination/pending-redirect tests and redundant proven value
   classification. Preserve a fully published child before any last-owner close.

3. **Creation/copy/allocation.** Both workloads create many heap generator or
   coroutine frames. Current RETURN_GENERATOR copies explicit continuation,
   virtual-kwargs and pending-redirect fields and saves the explicit resume
   recipient. Its frame layout differs from golden; creation may encounter a
   different allocation size class. Profile `_Py_MakeCoro`, frame copy and
   allocation/free alongside completion before attributing the gap to resume.
   The initialized arena reduces repeated clearing but adds a reservation
   frontier check; its net contribution is not known from source.

4. **Repeated return recipient handling.** `_PyFrame_SetReturnValue` now checks
   pending redirect before handling NoDst or publishing the result. Native
   `gen_send_value_ex`, VM resume and yielded results share it. NoDst is an
   explicit resume-input sink; delegated yields keep real receiver windows.
   Measure the actual recipient distribution and compiled branch route before
   changing this shared primitive. A generator-specific shortcut would need a
   positive lifecycle proof, not an opcode-name assumption.

## Controls and architectural direction

IR_SKIP_RESUME still gates instrumentation version/eval breaker and skips only a
plain RESUME. The physical return_pc replaces golden's instr_ptr+1 convention;
it should not be blamed for a new dispatcher or metadata walk. Normal YIELD
restores exception/current-frame links and publishes its resume recipient in
both models. New cold observer scopes and debugger analyses are not executed
for ordinary unobserved VM sends; confirm monitoring remains inactive in any
profile rather than charging their source code to every operation.

If profiling confirms completion overhead, the coherent candidate is a small
shared normal completion commit with a cold observation/error adapter, allowing
known outcome kinds to eliminate irrelevant owner/destination work. Preserve
the same PRE/POST ownership contract, canonical publication, alias handling,
exception-preserving retirement and callback redirects. Do not reintroduce
cleanup dispatch or split equivalent semantics into handler-name shortcuts.
No implementation is proposed without the current dynamic/disassembly ledger
and a matched measurement of the selected change.
