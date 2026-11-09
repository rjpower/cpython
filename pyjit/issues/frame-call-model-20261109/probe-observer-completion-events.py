"""Coordinator-run bounded ordinary-source event and physical-stream probe."""
import json
import sys
try:
    from _testinternalcapi import get_ir
except ImportError:
    get_ir = None


def snapshot(code):
    return {'name': code.co_qualname, 'firstline': code.co_firstlineno,
            'positions': list(code.co_positions()),
            'words': get_ir(code) if get_ir else None,
            'adaptive': get_ir(code, True) if get_ir else None}


def generator_case(warm, value, opcode, mode):
    namespace = {'VALUE': value}
    exec(compile('def gen():\n    yield 1\n    return VALUE\n'
                 'def consume():\n    for item in gen():\n        pass\n',
                 '<completion-events>', 'exec'), namespace)
    gen = namespace['gen']
    consume = namespace['consume']
    for _ in range(warm):
        consume()
    events, modern = [], []
    codes = (gen.__code__, consume.__code__)
    def trace(frame, event, arg):
        if frame.f_code in codes:
            if event == 'call':
                frame.f_trace_opcodes = opcode
            row = [frame.f_code.co_name, event,
                   frame.f_lineno - frame.f_code.co_firstlineno, frame.f_lasti]
            if event == 'exception':
                row += [arg[0].__name__, repr(arg[1])]
            events.append(row)
        return trace
    def stop(code, offset, error):
        if code in codes:
            modern.append([code.co_name, offset, type(error).__name__, repr(error)])
    tool = 4
    sys.monitoring.use_tool_id(tool, 'completion-probe')
    old_trace = sys.gettrace()
    try:
        sys.monitoring.register_callback(tool, sys.monitoring.events.STOP_ITERATION, stop)
        events_mask = sys.monitoring.events.STOP_ITERATION if mode != 'legacy' else 0
        sys.monitoring.set_local_events(tool, consume.__code__, events_mask)
        if mode != 'modern':
            sys.settrace(trace)
        consume()
    finally:
        sys.settrace(old_trace)
        sys.monitoring.set_local_events(tool, consume.__code__, 0)
        sys.monitoring.register_callback(tool, sys.monitoring.events.STOP_ITERATION, None)
        sys.monitoring.free_tool_id(tool)
    return {'case': 'generator', 'warm': warm, 'value': value, 'opcode': opcode, 'mode': mode,
            'events': events, 'modern_stop': modern,
            'code': [snapshot(code) for code in codes]}


def init_case(warm, opcode):
    namespace = {}
    exec(compile('class C:\n    def __init__(self):\n        self\n'
                 'def construct():\n    C()\n', '<init-events>', 'exec'), namespace)
    construct, init = namespace['construct'], namespace['C'].__init__
    for _ in range(warm):
        construct()
    events = []
    codes = (construct.__code__, init.__code__)
    def trace(frame, event, arg):
        if frame.f_code in codes:
            if event == 'call':
                frame.f_trace_opcodes = opcode
            events.append([frame.f_code.co_name, event,
                           frame.f_lineno - frame.f_code.co_firstlineno, frame.f_lasti])
        return trace
    old_trace = sys.gettrace()
    try:
        sys.settrace(trace)
        construct()
    finally:
        sys.settrace(old_trace)
    return {'case': 'init', 'warm': warm, 'opcode': opcode, 'events': events,
            'code': [snapshot(code) for code in codes]}


results = []
for warm in (0, 100):
    for opcode in (False, True):
        for value in (None, 7):
            for mode in ('legacy', 'combined'):
                results.append(generator_case(warm, value, opcode, mode))
            if not opcode:
                results.append(generator_case(warm, value, opcode, 'modern'))
        results.append(init_case(warm, opcode))
print(json.dumps({'python': sys.version, 'results': results}, indent=2))
