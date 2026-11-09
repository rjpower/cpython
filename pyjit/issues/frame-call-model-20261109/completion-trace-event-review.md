# Completion trace-event review after engineering23

Read-only during coordinator timing/full-test freeze. No runtime change and no
worker native execution. The earlier failure inventory is not a new23 receipt.

## Five exception-event failures share one source case

ownership22-trace-details.log repeats
`test_no_line_event_after_creating_generator` across five tracer subclasses.
The observed extra event is one caller `exception` after the generator's final
return, not another LINE before generator entry. The generator yields1 then
implicitly returns None.

There is a source-grounded mechanism. gen_send_value_ex distinguishes iternext
from send: return None through iternext becomes PYGEN_RETURN with PyValue_NULL.
The old generic `_PyForIter_VirtualIteratorNext` fabricated RAISE only for a
non-NULL, non-None returned value. Current helper preserves the returned owner;
INSTRUMENTED_FOR_ITER normalizes its NULL payload to None for semantic completion.
ObserveStep PRE then creates artificial StopIteration(None) for a generator
receiver. The legacy bridge maps STOP_ITERATION to PyTrace_EXCEPTION. This
explains an added exception event even though no actual iterator exception was
raised on this route.

The distinction belongs to semantic completion/event policy, not to the test's
name, source line, generator identity or blanket StopIteration suppression.
SEND completion with None and FOR_ITER exhaustion are different operations;
non-None generator returns and real iterator-raised StopIteration still need
their correct events. Existing test_monitoring implicit_stop_iteration expects
one event for generator return2, so eliminating all completion events is wrong.

A remaining policy question is whether modern STOP_ITERATION should preserve
its full artificial-return coverage while the legacy adapter preserves its
historical FOR/None behavior, or the shared completion event itself should
exclude valueless FOR exhaustion across routes. The current repository's old
generic and optimized mechanisms differ; source inspection alone cannot settle
the intended public API boundary. Compare the ordinary probe on matched upstream
and candidate before admitting either policy. Do not introduce a callback filter
based on a particular fixture.

## Opcode-traced class initialization: separate unresolved mechanism

Only TraceOpcodesTestCase reports an extra LINE for a class whose __init__ body
is the pure expression `self`, both before and after quickening comparisons.
Its expected call→bodyLINE→return gains a second identical bodyLINE. This is not
the generator completion exception and needs separate evidence.

Current LINE and INSTRUCTION wrappers can nest at one real word; source-only
LINE may be delivered on an incoming edge. The instruction callback's legacy
bridge can enable opcode tracing, and monitoring epoch changes invalidate LINE
suppression. These mechanisms can legitimately interact, but no actual captured
PC/epoch/word witness yet identifies which produces the duplicate. Repeated
f_trace_opcodes=True normally returns early in set_opcode_trace_world_stopped
when INSTRUCTION is already enabled; therefore claiming that every callback
increments the epoch would be incorrect.

The coordinator-run `probe-observer-completion-events.py` compiles fresh sources,
warms0/100, toggles opcode tracing, and reports event PCs/line offsets plus
pristine/adaptive words/positions. It also records independent modern STOP
callbacks alongside legacy events for None and non-None generator returns.
The optional get_ir import permits the same ordinary-source probe on upstream.
Host AST passes; no probe result is claimed yet.

## Context-recipe peer

The bounded real-LINE identity proposal preserves all owners and caught-exception
state; source cursor equality is correctly excluded. General context recipes
must not update tstate->exc_info blindly while executing a Python callback.
That callback may have its own handled LookupError, with the target's handled
ZeroDivisionError held in an intervening saved-prev owner. Replacing the current
exc_info could corrupt the callback's sys.exception(), and its POP_EXCEPT could
restore the target's old exception afterward. Resolve the actual target-context
restoration owner or keep exits unadmitted until it is explicit. Same-region
moves that preserve exception ownership are a separate, smaller proof. B is
preparing the concrete two-exception probe; no schema implementation is admitted.
