# Call setup: test the boundary before changing it

This is a bounded read-only investigation. The question is whether the existing
coroutine/generator captures support one shared call/argument/value interface
change. No runtime, generated output, build, profile or timing is changed.

The source authority is the current primary runtime, byte-identical to the
captured `0b15b4bbc9b` in the files examined. The separately reviewed nursery
and shape candidates do not change the call/argument/frame helpers discussed
here. Their combined qualification proceeds independently.

## Required work and existing fast paths

[`CALL_PY_EXACT_ARGS`](../../../Python/bytecodes.c#L4199) already receives
canonical `PyValue` arguments. A thread-owned caller overlaps its call window
with the callee through
[`_PyFrame_PushOverlapping`](../../../Include/internal/pycore_interpframe.h#L506).
There is no generic keyword parser, argument boxing or positional-copy loop on
that path. A generator-owned caller has its slots inside the generator object;
its callee receives separate thread-stack storage. That branch copies the live
arguments and clears consumed caller slots. Removing these operations requires
an ownership/storage proof, not merely a common signature.

[`_PyFrame_Initialize`](../../../Include/internal/pycore_interpframe.h#L269)
serves unchecked, overlapping and general frame creation. It establishes the
header, executable/feedback entry and NULL slot tail. Existing `null_locals_from`
already preserves the supplied argument prefix. Publication and root visibility
must survive allocation, recursion errors, frame escape and monitoring. The
previous full-range frame-copy experiment was rejected; this investigation does
not revive it or assume another bulk copy will help.

[`_PyEvalFramePushAndInit`](../../../Python/ceval.c#L1964) is the general path.
Its callers include generic/instrumented Python CALL, general bound/unbound
calls, keyword calls, `CALL_FUNCTION_EX`, class initialization and the C-facing
[`_PyEval_Vector`](../../../Python/ceval.c#L2050). It initializes locals through
[`initialize_locals`](../../../Python/ceval.c#L1628), which preserves defaults,
keyword-only and positional-only rules, varargs/varkwargs, useful errors and
cleanup. The separate exact-call path already avoids this parser.

[`_PyFunction_Vectorcall`](../../../Objects/call.c#L628) already accepts a
`PyValue` array. Native special-method bridges still expose object-pointer
contracts: for example
[`maybe_call_special_no_args`](../../../Objects/typeobject.c#L3103) serves
`__iter__`, `__bool__`, `__hash__` and related slots, roots its lookup state and
calls the same shared machinery. Their C result contracts and error behavior
remain meaningful; this capture does not establish that replacing them saves
material repeated work.

## Conversion is not boxing

[`PyValue_FromObject`](../../../Include/pyvalue.h#L208) canonicalizes an existing
object: exact ints/floats may become inline values; other objects remain
pointers. [`PyValue_Object`](../../../Include/pyvalue.h#L284) boxes only when its
input is inline. A sampled outer `PyValue_Object` frame is therefore not evidence
that boxing happened. The internal
[`_PyValue_Wrap`](../../../Include/internal/pycore_value.h#L129) is unsuitable as
a faster constructor for live object fields: debug builds deliberately tag its
unsafe inter-uop pointer, and the GC must not see it.

A new trusted-object constructor could remove numeric checks for proved code or
generator objects, but that would be a separate type-fact optimization. The
small sampled canonicalization sites below do not justify selecting another
micro-optimization or a broader call ABI change now.

## Decision rule

Select an implementation only if the capture identifies repeated removable
setup, source review closes all affected consumers and one shared change
simplifies those consumers while preserving values, root visibility, exceptions
and observers. Reject the present hypothesis if it rests on outer entry frames,
all residual CPU, canonicalization mislabeled as boxing, or required argument
ownership work. [Results](results.md) apply that rule without a new native run.
