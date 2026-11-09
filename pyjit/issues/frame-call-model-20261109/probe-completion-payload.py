"""Coordinator-run completion payload identity and actual observer-order probe."""
import json
import sys


def run(payload, warm, mode, kind, mutate=False):
    def gen():
        yield 1
        return payload
    delivered = []
    if kind == 'FOR':
        def consume():
            for item in gen():
                pass
        drive = consume
    else:
        def consume():
            result = yield from gen()
            delivered.append(result)
        def drive():
            list(consume())
    for _ in range(warm):
        drive()
    delivered.clear()
    events = []
    def record(who, error):
        value = error.value
        events.append({'who': who, 'type': type(error).__name__,
                       'value_type': type(value).__name__, 'value': repr(value),
                       'value_is_payload': value is payload,
                       'exception_is_payload': error is payload})
    def trace(frame, event, arg):
        if frame.f_code is consume.__code__ and event == 'exception':
            record('legacy', arg[1])
            if mutate:
                arg[1].value = 99
        return trace
    def modern5(code, offset, error):
        record('modern5', error)
        if mutate:
            error.value = 123
    def modern0(code, offset, error):
        record('modern0', error)
    old_trace = sys.gettrace()
    tools = ((0, modern0), (5, modern5))
    acquired = []
    try:
        if mode != 'legacy':
            for tool, callback in tools:
                sys.monitoring.use_tool_id(tool, 'payload-probe')
                acquired.append(tool)
                sys.monitoring.register_callback(tool, sys.monitoring.events.STOP_ITERATION, callback)
                sys.monitoring.set_local_events(tool, consume.__code__, sys.monitoring.events.STOP_ITERATION)
        if mode != 'modern':
            sys.settrace(trace)
        drive()
    finally:
        sys.settrace(old_trace)
        for tool in acquired:
            sys.monitoring.set_local_events(tool, consume.__code__, 0)
            sys.monitoring.register_callback(tool, sys.monitoring.events.STOP_ITERATION, None)
            sys.monitoring.free_tool_id(tool)
    return {'payload_type': type(payload).__name__, 'payload': repr(payload),
            'warm': warm, 'mode': mode, 'kind': kind, 'mutate': mutate,
            'returned_payload_identity': [value is payload for value in delivered],
            'events': events}


rows = [run(payload, warm, mode, kind)
        for kind in ('FOR', 'SEND')
        for warm in (0, 100)
        for payload in ((1, 2), StopIteration(None), None, 7)
        for mode in ('legacy', 'modern', 'combined')]
rows.extend(run(7, 100, 'combined', kind, mutate=True) for kind in ('FOR', 'SEND'))
print(json.dumps({'python': sys.version, 'results': rows}, indent=2))
