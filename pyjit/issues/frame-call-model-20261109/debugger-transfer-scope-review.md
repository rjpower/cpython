# Debugger jump contract and lexical context

`Doc/reference/datamodel.rst` describes `frame.f_lineno` as a writable jump to
a line from a trace function on the bottom-most frame. `Doc/library/sys.rst`
describes a line event as occurring before the new line executes. Neither text
specifies how an artificial jump must skip an active `with` cleanup, synthesize
an exception-stack link, or reuse a different lexical context manager.

The supported contract should therefore be a jump to a real executable line
whose incoming value owners, named-local bindings, and handled-context roots
can be proved from the encoded program and the actual stopped phase. A failed
proof must leave owners, locals and the continuation unchanged and raise a
documented error. Ordinary same-context jumps, monitored branch redirects and
saved-local restoration with a proved source remain positive behavior; they
should not be weakened merely to pass a suite.

Crossing an active `with` or exception protocol is a different capability. If
the requested target requires a manager or handler root absent from the
current frame, the setter cannot manufacture it from a line number. If the
jump skips a pending `__exit__`, the language documentation gives no specific
promise that the debugger calls that method; doing so would itself execute
user code during a setter transaction. Refuse such jumps until an explicit,
reviewed action protocol proves their semantics and close order. Tests that
assert CPython's particular permissiveness across those boundaries should be
reviewed against this contract individually, while safety and no-mutation
negatives remain.

This is a semantic scope recommendation, not an instruction to port failing
tests wholesale or to expand the current nonneutral POST packet.
