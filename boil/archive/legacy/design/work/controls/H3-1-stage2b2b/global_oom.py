"""Global requested masks retain documented partial progress on refresh failure."""
import gc
import json
import resource
import sys
import _testcapi
import _testinternalcapi as capi

resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
assert sys.flags.optimize == 0 and __debug__
assert capi.gc_verify_mode() == (0, 0) and capi.gc_get_stress() == 0
mon = sys.monitoring
tool = mon.PROFILER_ID
mon.use_tool_id(tool, "global partial progress")
seen = []
def f(x):
    y = x + 1
    return y * 2
code = f.__code__
def record(current, offset):
    if current is code:
        seen.append(offset)
mon.register_callback(tool, mon.events.INSTRUCTION, record)
gc.collect()
requested = mon.events.LINE | mon.events.INSTRUCTION
try:
    try:
        _testcapi.call_with_nomemory(0, 0, mon.set_events, tool, requested)
    except MemoryError:
        assert mon.get_events(tool) == requested
        # An identical global request can be a no-op; a stale code version
        # retries preparation when that code next enters the interpreter.
        mon.set_events(tool, requested)
        assert f(3) == 8
        assert seen
        print(json.dumps({"requested": requested, "public": mon.get_events(tool),
                          "refreshed_instruction_sites": seen}), flush=True)
    else:
        raise AssertionError("expected global refresh allocation failure")
finally:
    mon.set_events(tool, 0)
    mon.free_tool_id(tool)
