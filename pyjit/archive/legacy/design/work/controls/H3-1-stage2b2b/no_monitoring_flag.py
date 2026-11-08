"""Accepted code.replace flag preserves requests while suppressing events."""
import json
import sys
import _testinternalcapi as capi

assert sys.flags.optimize == 0 and __debug__
assert capi.gc_verify_mode() == (0, 0) and capi.gc_get_stress() == 0
CO_NO_MONITORING_EVENTS = 0x2000000  # Include/cpython/code.h

def f(x):
    return x + 1
f.__code__ = f.__code__.replace(co_flags=f.__code__.co_flags | CO_NO_MONITORING_EVENTS)
assert f(3) == 4
code = f.__code__
mon = sys.monitoring
tool = mon.PROFILER_ID
mon.use_tool_id(tool, "no instrumentation requested mask")
seen = []
def record(current, offset):
    if current is code:
        seen.append(offset)
mon.register_callback(tool, mon.events.INSTRUCTION, record)
requested = mon.events.LINE | mon.events.INSTRUCTION
before = capi.get_ir(code, True)
try:
    mon.set_local_events(tool, code, requested)
    actual = mon.get_local_events(tool, code)
    assert capi.get_ir(code, True) == before
    assert f(3) == 4
    assert not seen, seen
    print(json.dumps({"requested": requested, "actual": actual, "events": seen,
                      "words_unchanged_by_setter": True}), flush=True)
    assert actual == requested, ("configured request lost for uninstrumented code", requested, actual)
finally:
    mon.set_local_events(tool, code, 0)
    mon.free_tool_id(tool)
