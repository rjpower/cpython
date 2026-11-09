"""Coordinator-run ordinary source/inline debugger-stop diagnostics."""
import bdb
import json
import struct
import sys
try:
    from _testinternalcapi import get_ir
except ImportError:
    get_ir = None


def metadata(code):
    raw = code.co_linetable
    records = None
    if len(raw) >= 16 and raw[:4] == b'IRSM':
        magic, version, payload, count = struct.unpack_from('<4sIII', raw)
        assert version == 1 and 16 + payload + 40 * count == len(raw)
        records = list(struct.iter_unpack('<10i', raw[16 + payload:]))
    return {'firstline': code.co_firstlineno, 'positions': list(code.co_positions()),
            'lines': list(code.co_lines()), 'source_records': records,
            'words': get_ir(code) if get_ir else None}


def pattern_case(value):
    namespace = {}
    exec(compile('def target(command):\n    match command:\n        case 1:\n'
                 '            if False:\n                return 1\n'
                 '        case _:\n            if False:\n                return 0\n',
                 '<wildcard-boundary>', 'exec'), namespace)
    target = namespace['target']
    events = []
    def trace(frame, event, arg):
        if frame.f_code is target.__code__:
            events.append([event, frame.f_lineno - target.__code__.co_firstlineno, frame.f_lasti])
        return trace
    previous = sys.gettrace()
    try:
        sys.settrace(trace)
        target(value)
    finally:
        sys.settrace(previous)
    return {'case': 'wildcard', 'value': value, 'events': events,
            'metadata': metadata(target.__code__)}


def debugger_case(backend):
    stops = []
    class Debugger(bdb.Bdb):
        def user_opcode(self, frame):
            stops.append(['opcode', frame.f_lineno - target.__code__.co_firstlineno,
                          frame.f_lasti, frame.f_locals.get('x')])
            self.set_continue()
        def user_line(self, frame):
            stops.append(['line', frame.f_lineno - target.__code__.co_firstlineno,
                          frame.f_lasti, frame.f_locals.get('x')])
            self.set_continue()
    debugger = Debugger(backend=backend)
    def target():
        x = 1
        debugger.set_trace()
        x = 2
        return x
    value = target()
    debugger.stop_trace()
    return {'case': 'inline-debugger', 'backend': backend, 'stops': stops,
            'value': value, 'metadata': metadata(target.__code__)}


print(json.dumps({'python': sys.version,
                  'results': [pattern_case(1), pattern_case(0),
                              debugger_case('settrace'), debugger_case('monitoring')]}, indent=2))
