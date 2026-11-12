# A: bounded final-home construction

Reviewed proposal, later implemented and rejected; see the [final result](final-homes-result.md).

Proposed against detached golden51 in `BASE/worktrees/activation53-final-homes`.
The corrected Astra contract replaces the earlier broad construction proposal:
physical word zero must itself be RETURN_GENERATOR. No prefix execution,
CONSTRUCTING state, continuation kind or generic frame API change is included.

## Interfaces and admission

`_PyEval_TryMakeGenerator(tstate, function_home, code, args, supplied, binding)`
returns `_PyStatusValue`: 0/MISS or -1/ERROR leaves all function/argument homes
unchanged; 1/SUCCESS consumes those owners, NULLs their homes and returns one
canonical generator owner. `binding == NULL` describes exact positional count;
otherwise it is the existing fully validated positional `_PyFbCall` plan. The
helper reuses plan destinations/default offsets/empty-variadic rules, not the
generic binder. It performs no caller recipient publication.

The producer's one common branch tests the existing generative code flags.
The shared `_PyCode_HasDirectGeneratorEntry` fact checks exact generator/coroutine
flags, the actual immutable first word, physical successor and initial recipient.
The cold helper additionally checks the current executing first word. Existing constructor validation already requires
RETURN_GENERATOR's initial recipient to be IR_NO_DST; code.replace traverses
that same validation. No new code flag, executable allocation field or frame
field is required. Prefixes and transparent wrappers are declined, not skipped.
Generators and coroutines are admitted; async generators initially decline.

Admission also requires optimized code, standard Python provider, an existing
executable, recursion remaining >1 and current stack-capacity admission. Decline
free-threaded builds, PEP523, trace/profile hooks, any active monitoring version
or code monitoring, origin tracking and enabled DTrace. Debug admission declines
LLTrace environment settings and globals containing `__lltrace__`; use only a
known string-key dict for that nonescaping lookup, otherwise decline. Keep normal
polling after actual first resume. No callback/poll occurs during allocation
and commit; selected GIL GC allocation schedules collection only.

## Allocation and owner recipe

Pin the selected executing code before allocation. A small explicit-code shell
allocator in genobject.c uses that code's flags and frame size; it must not
reread mutable function code. Its exact generator/coroutine shell is CLEARED
with valid name/qualname, exception, executable and coroutine-origin fields.
Allocate any eager empty kwargs before moving owners. Every failure occurs
before moves, releases only a valid CLEARED shell/prepared empty dictionary and
temporary code pin, and leaves caller inputs intact with the exception kept.

Initialize all physical heap homes NULL, then call the shared frame initializer
once with function/code owners. Move each supplied argument and NULL its source.
Fill plan defaults and empty variadics/pending kwargs. No fallible operation or
callback-capable close follows the first owner move before seal. Install the
physical successor of word zero and its initial recipient, previous=NULL,
owner=GENERATOR and state=CREATED. Release the extra code pin only after the final
frame owns code. No current-frame or exception-chain publication, recursion
increment/decrement or temporary thread frame is necessary.

On MISS exact producers execute their existing frame construction unchanged.
On ERROR the generated exact terminal goes directly to the existing error
label with its intact PRE call window. Existing traceback/RAISE and unwind
recipes observe and retire those owners once; no private drain or input flush
precedes that transition. On SUCCESS it marks its window transferred and
terminates CALL through the current destination/continuation rules: canonical
publication, displacement/discard cleanup and redirect-aware PC reload. It
cannot fall through SAVE_RETURN_SLOT/PUSH_FRAME. The marker sits in its own
non-pure peek uop before the existing pure initialization uop. A bounded emitter
validator binds the guard prefix, consuming geometry and terminal shape, and
declares allocation/error/callback effects explicitly.

The positional plan producer retains its existing ordinary-function admission
and admits generator/coroutine plans only through that same immutable entry fact;
async generators and creation prefixes stay excluded. This changes a producer
restriction that the initial inspection missed. The validated positional-plan helper tries the same constructor before its
thread-frame reservation; success publishes with the existing call completion
machinery and returns `_PyCallResult {0, NULL}`. The emitter recognizes that
finished outcome before the existing MISS fallback and reloads the winning
return_pc; this is an added finished-result test on known-plan calls and belongs
in native cost accounting. Keyword plans decline this new
path. Error retains the plan's original owning window for its normal error
recipe. Both exact IR and positional plans keep capacity/recursion admission.

## Native producer adapters

Include `_PyFunction_ValueVectorcall` and `_PyEval_Vector` after they build
owned canonical argument arrays and before `_PyEvalFramePushAndInit`. Require
exact positional count, no keywords/explicit locals, optimized code, no
variadics or required keyword-only arguments, current standard provider and an
existing executable. Keep a local function owner. Near Python/C recursion or
stack limits decline to existing frame/evaluator behavior.

MISS passes the same function owner and unchanged array to the frame API.
ERROR closes the intact function/arguments once, frees only array scratch and
propagates the exception. SUCCESS frees array scratch and returns the canonical
generator, converting to an owning object view only at the object boundary.
The shared constructor does not publish an IR recipient for native callers.
This is bounded native construction coverage, not a generic binder redesign.

## Files and verification

Handwritten files: `Python/bytecodes.c`, `Python/ceval.c`, `Python/specialize.c`, `Objects/genobject.c`,
`Objects/call.c`, `Include/internal/pycore_ceval.h`,
`Include/internal/pycore_ir.h`,
`Tools/cases_generator/{analyzer,ir_generator}.py` and focused existing/new
generator admission tests. Add bounded contract/effect and malformed-producer
coverage in `Lib/test/test_generated_cases.py` or its existing IR sibling.
Root owns regenerated products, native builds/tests, diagnostic hit counts,
native inspection, performance and integration. This worker runs only authorized
host generator/admission checks and reviews its diff. Diagnostic construction
hits must distinguish coroutine IR and native vector paths; unsupported shapes
and cold executable creation are reported as misses. Root may obtain counters
through source/native diagnostics rather than adding a common-path release
counter.

Monitoring admission conservatively rejects every nonzero global instrumentation
version, including after tools are removed. Root runs each constructor test method
in a fresh process when its direct path matters, separately from a whole-module
fallback regression run. Exact and known-plan finalizer redirects are separate
methods; the native map test warms the executable before invoking map. A changed
co_flags/no-RETURN_GENERATOR negative exercises the immutable entry authority.
Successful native/IR/known-plan constructor probes remain necessary: semantic
passes alone do not prove those producers hit the new path.

Independent B-worker peer review accepted the bounded interface and shell/native
ownership contract. Its required intact-window error correction is incorporated
above. The function home and supplied contiguous range explicitly include self
when present; a no-self NULL spacer remains NULL. Success saves the real caller
continuation before the shared recipient operation closes discard/displaced
owners, then reloads the callback-selected PC. Runtime implementation is now
authorized by the coordinator after that peer review.

The actual instruction cursor retains CALL during ordinary result cleanup, as
selected ordinary return and generic call completion do. return_pc records
future control separately. Eagerly setting instr_ptr to that future word made
a trace hook installed by a displaced finalizer miss the first landing LINE;
initial candidate validation exposed this regression. The correction omits that
eager write entirely, preserving callback-selected cursors without restoration
after effects. Existing LINE dispatch now retains the real predecessor and
processes ordinary source-edge events. This correction has independent source
approval; root owns rebuilt runtime confirmation.

Required candidate checks cover arguments/defaults/kwargs, discarded and local
CALL results, closure/genexpr prefix fallback, code.replace/mortal constants,
origin/hooks/LLTrace, weakrefs/GC/unawaited warnings, pre-first-send close/throw,
recursion and allocation failure. Actual native CALL inspection must show the
single ordinary entry branch, direct construction terminal and unchanged miss
initialization. Root's balanced measurement/admission gates remain required.
