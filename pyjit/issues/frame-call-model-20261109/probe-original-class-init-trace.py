"""Coordinator-run original fixture, original tracer, no callback instrumentation."""
import json
import sys
import unittest
from test import test_sys_settrace as fixture

try:
    from _testinternalcapi import get_ir
except ImportError:
    get_ir = None


def snapshot(code):
    return {'name': code.co_qualname, 'firstline': code.co_firstlineno,
            'positions': list(code.co_positions()), 'lines': list(code.co_lines()),
            'linetable_hex': code.co_linetable.hex(),
            'words': get_ir(code) if get_ir else None,
            'adaptive': get_ir(code, True) if get_ir else None}


def original_case(base):
    receipts = []
    class Captured(base):
        def run_and_compare(self, func, expected):
            # Observe only outside tracing. The superclass installs its actual
            # Tracer, including every-event co_lines reads and opcode setters.
            codes = [func.__code__]
            for cell in func.__closure__ or ():
                value = cell.cell_contents
                if isinstance(value, type) and '__init__' in vars(value):
                    codes.append(value.__init__.__code__)
            receipt = {'before': [snapshot(code) for code in codes]}
            receipts.append(receipt)
            try:
                super().run_and_compare(func, expected)
            finally:
                receipt['after'] = [snapshot(code) for code in codes]

        def compare_events(self, offset, events, expected):
            receipts[-1].update(offset=offset, events=events, expected=expected)
            super().compare_events(offset, events, expected)

    case = Captured('test_correct_tracing_quickened_call_class_init')
    result = unittest.TestResult()
    case.run(result)
    return {'class': base.__name__, 'tests': result.testsRun,
            'failures': [text for _, text in result.failures],
            'errors': [text for _, text in result.errors],
            'receipts': receipts}


print(json.dumps({'python': sys.version,
                  'cases': [original_case(fixture.TraceTestCase),
                            original_case(fixture.TraceOpcodesTestCase)]}, indent=2))
