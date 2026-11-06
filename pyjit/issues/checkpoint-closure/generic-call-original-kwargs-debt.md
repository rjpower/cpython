# Callable instance keyword validation order

A direct C `PyObject_Call` with a non-Unicode kwargs key exposes a separate semantic gap from the AST/type-call failure. The reproducer is `generic-call-original-kwargs-repro.py`: a callable instance's `__call__` descriptor records its `__get__`, and `ctypes.pythonapi.PyObject_Call(instance, (), {1: 2})` is invoked.

The upstream `3a440b6` interpreter raises `TypeError: keywords must be strings` after recording `['descriptor']`. The candidate release interpreter raises the same error with `[]`. In the candidate, `Objects/call.c:_PyValue_Call` converts the kwargs dictionary to a value window before `Objects/typeobject.c:slot_tp_call_value` can look up `__call__`; the object `slot_tp_call` uses `call_method`, which looks up first.

This is a confirmed callback-order difference. The narrow AST selected-type patch does not cover it. A later fix should preserve the original dictionary through a selected `slot_tp_call` tuple/dict continuation, then validate keywords at the callable's own call boundary. Keep the global vectorcall Unicode-key invariant and do not treat this as an external boundary.
