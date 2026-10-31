"""A valid suppressed-code RESUME must finish after monitoring versions advance."""
import faulthandler
import json
import resource
import sys
import types
import _testinternalcapi as capi

resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
state = (sys.flags.optimize, __debug__, capi.gc_verify_mode(), capi.gc_get_stress())
assert state == (0, True, (0, 0), 0), state
CO_NO_MONITORING_EVENTS = 0x2000000  # Include/cpython/code.h
mon = sys.monitoring
a, b = mon.PROFILER_ID, mon.COVERAGE_ID
mon.use_tool_id(a, "suppressed eligibility")
mon.use_tool_id(b, "advance global monitoring version")

def f(x):
    mon.set_events(b, mon.events.PY_START)
    mon.set_events(b, 0)
    return x + 1

plain = types.FunctionType(f.__code__, globals(), "ordinary")
f.__code__ = f.__code__.replace(co_flags=f.__code__.co_flags | CO_NO_MONITORING_EVENTS)

def generator():
    try:
        yield 1
    except ValueError:
        yield 2

generator.__code__ = generator.__code__.replace(
    co_flags=generator.__code__.co_flags | CO_NO_MONITORING_EVENTS)
flagged = (f.__code__, generator.__code__)
seen_suppressed, seen_plain = [], []
def record(current, offset):
    if current in flagged:
        seen_suppressed.append(offset)
    elif current is plain.__code__:
        seen_plain.append(offset)
mon.register_callback(a, mon.events.INSTRUCTION, record)
try:
    mon.set_events(b, mon.events.PY_START)
    mon.set_events(b, 0)
    requested = mon.events.LINE | mon.events.INSTRUCTION
    for code in (*flagged, plain.__code__):
        mon.set_local_events(a, code, requested)
        assert mon.get_local_events(a, code) == requested
    before = capi.get_ir(f.__code__, True)
    print(json.dumps({"phase": "enter_suppressed", "state": state,
                      "requested": requested, "word_count": len(before)}), flush=True)
    faulthandler.dump_traceback_later(2, exit=True)
    assert f(3) == 4
    faulthandler.cancel_dump_traceback_later()
    assert capi.get_ir(f.__code__, True) == before
    assert not seen_suppressed
    print(json.dumps({"phase": "suppressed_returned", "events": seen_suppressed}), flush=True)
    faulthandler.dump_traceback_later(2, exit=True)
    it = generator()
    assert next(it) == 1
    assert it.throw(ValueError) == 2
    it.close()
    assert not seen_suppressed
    assert plain(3) == 4
    assert seen_plain
    faulthandler.cancel_dump_traceback_later()
    print(json.dumps({"phase": "complete", "suppressed_events": seen_suppressed,
                      "ordinary_sites": seen_plain}), flush=True)
finally:
    faulthandler.cancel_dump_traceback_later()
    mon.set_events(b, 0)
    for code in (*flagged, plain.__code__):
        mon.set_local_events(a, code, 0)
    mon.free_tool_id(a)
    mon.free_tool_id(b)
