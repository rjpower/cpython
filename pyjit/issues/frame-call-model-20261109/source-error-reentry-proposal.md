# Pending source errors belong to an evaluator activation

The independent [post-review](semantic-arena-post-review.md) finds a pending
source-error recipe in thread-global state. A nested trace callback failure
during completion retirement can replace and consume it while the original
exception remains pending. The outer frame then chooses the semantic
instruction's handler instead of its failed source event's handler.

Move the existing frame/record pair from `_PyThreadStateImpl` into
`_PyEvalActivation`, initialized before publishing that activation. Keep one
pending recipe per activation. Nested C entry already links a new activation
and restores the prior pointer on both exits; it must not modify its parent's
recipe. VM child transfers share their activation, but cannot run while an
outer pending error is being retired except through a new C entry. Verify
finalizer, trace callback, explicit call-tracing and recursion-error routes.

`source_line_event` records the recipe through one internal cold setter in
`pycore_ceval.h`, implemented beside evaluator entry in `ceval.c`. This keeps
the activation layout private to `Python/ceval.h`. Exception lookup consumes
and clears only the current activation's matching frame recipe. Its existing
frame-object source cursors are cleared at the same point.

The frame-object cursor remains display/traceback state; it is not the sole
error recipe. Creating that object can itself fail before a cursor exists.
Activation-local storage needs no allocation, so this failure still reaches
the selected source handler. Do not add a second thread-global fallback,
ordinary frame field or per-instruction update.

No owning reference is added: the affected frame/code remains reserved and
rooted until exception lookup and retirement finish. Removing the private
thread fields changes internal offsets; rebuild matched readers and keep the
target-described remote layout contract. GDB accesses activation members by
debug symbols; the existing previous/entry links keep their meanings.

Validation starts with a real compiled source-only LINE event whose handler
differs from the predecessor's. Its callback raises while a completion-owned
result is retired; that result's finalizer calls a second observed function
whose source callback error is caught. Require both handlers and the outer
traceback location to survive. Compare the control without nested failure.
This tests the selected runtime's reentry protocol, not a language guarantee
about destruction timing. Add allocation-failure coverage if an existing
bounded frame-allocation hook reaches the same path. Then run focused source
boundary/completion, monitoring and finalization checks in release/debug.

## Native reproduction before repair

The [plain Python probe](probe-source-error-reentry.py) reproduces the finding
on the `semantic16b` release ELF, both before warmup and after 20 calls. The
control records outer event, retirement, then the outer handler. With a nested
source callback failure, the inner handler runs but the outer error escapes.
Both use ordinary compiled code and preserve the test workload. Evidence is
`E11/semantic16b-source-error-probe.{json,log}`, including ELF/probe identities.

The proposal remains pending peer review, implementation and passing repair
validation. The reproduction is a failing control, not a qualification result.
