"""Coordinator-only bounded diagnostics; original unittest assertions stay active."""

import argparse
import importlib.util
import json
from pathlib import Path
import sys
import unittest

from _testinternalcapi import get_ir
from test import test_sys_settrace


ORDINARY = (
    'test_jump_across_with',
    'test_jump_across_async_with',
    'test_jump_backward_over_listcomp',
    'test_jump_backward_over_async_listcomp',
    'test_jump_backward_over_async_listcomp_v2',
    'test_jump_back_over_comprehensions',
    'test_jump_within_except_block',
    'test_jump_in_nested_finally_2',
    'test_jump_in_nested_finally_3',
    'test_jump_out_of_async_for_block_forwards',
    'test_jump_out_of_async_for_block_backwards',
)
PROBE_FIRST = ('test_jump_between_except_blocks',
               'test_jump_between_except_blocks_2')
TRANSITIONS = (
    'test_jump_out_of_with_block_within_for_block',
    'test_jump_out_of_async_with_block_within_for_block',
    'test_jump_out_of_with_block_within_with_block',
    'test_jump_out_of_async_with_block_within_with_block',
    'test_jump_between_with_blocks',
    'test_jump_between_async_with_blocks',
    'test_jump_from_except_to_finally',
    'test_jump_out_of_bare_except_block',
    'test_jump_out_of_qualified_except_block',
    'test_jump_out_of_complex_nested_blocks',
    'test_jump_from_yield',
)
ROSTER = ORDINARY + PROBE_FIRST + TRANSITIONS


class DiagnosticCap(RuntimeError):
    pass


def identity(value):
    return {'id': id(value), 'type_id': id(type(value))}


def safe_value(value, depth=0):
    row = identity(value)
    kind = type(value)
    if kind in (type(None), bool):
        row['value'] = value
    elif kind is int:
        if value.bit_length() <= 128:
            row['value'] = value
    elif kind is str:
        row['value'] = value[:256]
        row['length'] = len(value)
    elif kind is float:
        row['hex'] = value.hex()
    elif kind in (list, tuple) and depth < 2:
        row['length'] = len(value)
        row['items'] = [safe_value(item, depth + 1) for item in value[:64]]
    return row


def load_describe():
    path = Path(__file__).with_name('probe-debugger-context-inventory.py')
    spec = importlib.util.spec_from_file_location('_jump36_inventory', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.describe


def run_case(name, event_cap, describe):
    requests, comparisons, codes, caps = [], [], {}, []
    original_trace = test_sys_settrace.JumpTracer.trace
    original_compare = test_sys_settrace.JumpTestCase.compare_jump_output
    events = 0

    def code_key(code):
        key = id(code)
        if key not in codes:
            codes[key] = (f'code{len(codes)}', code)
        return codes[key][0]

    def snapshot(frame):
        pc = frame.f_lasti
        words = get_ir(frame.f_code, True)
        locals_copy = dict(frame.f_locals)
        row = dict(code=code_key(frame.f_code), pc=pc, line=frame.f_lineno,
                   actual_word=words[pc] if 0 <= pc < len(words) else None,
                   locals={key: safe_value(locals_copy[key])
                           for key in sorted(key for key in locals_copy
                                             if type(key) is str)},
                   handled_exception=identity(sys.exception()))
        if 'output' in locals_copy and type(locals_copy['output']) is list:
            row['output'] = safe_value(locals_copy['output'])
        return row

    def trace(tracer, frame, event, arg):
        nonlocal events
        request = None
        if not tracer.done:
            events += 1
            if events > event_cap:
                caps.append('event')
                raise DiagnosticCap(f'{name}: event cap {event_cap}')
            first = tracer.firstLine
            if first is None and frame.f_code == tracer.code and event == 'line':
                first = frame.f_lineno - 1
            if (event == tracer.event and first is not None and
                    frame.f_lineno == first + tracer.jumpFrom):
                ancestor = frame
                while ancestor is not None and ancestor.f_code != tracer.code:
                    ancestor = ancestor.f_back
                if ancestor is not None:
                    if len(requests) >= 32:
                        caps.append('request')
                        raise DiagnosticCap(f'{name}: request cap 32')
                    request = dict(event=event, tracer_code=code_key(tracer.code),
                                   effective_first_line=first,
                                   requested_line=first + tracer.jumpTo,
                                   before=snapshot(frame))
                    requests.append(request)
        failure = None
        traceback = None
        try:
            returned_trace = original_trace(tracer, frame, event, arg)
        except ValueError as error:
            if request is None:
                raise
            request['value_error'] = dict(identity(error), args=safe_value(error.args))
            failure, traceback = error, error.__traceback__
        if request is not None:
            # Leave our except suite before reading sys.exception(): its own
            # temporary ValueError must not masquerade as the handled context.
            request['after'] = snapshot(frame)
            request['tracer_done'] = tracer.done
        if failure is not None:
            # Preserve the exact original exception instance for unittest.
            raise failure.with_traceback(traceback)
        return returned_trace

    def compare(case, expected, received):
        comparisons.append(dict(expected=safe_value(expected), received=safe_value(received)))
        return original_compare(case, expected, received)

    result = unittest.TestResult()
    previous_trace = sys.gettrace()
    test_sys_settrace.JumpTracer.trace = trace
    test_sys_settrace.JumpTestCase.compare_jump_output = compare
    try:
        test_sys_settrace.JumpTestCase(name).run(result)
    finally:
        sys.settrace(previous_trace)
        test_sys_settrace.JumpTracer.trace = original_trace
        test_sys_settrace.JumpTestCase.compare_jump_output = original_compare
    category = ('ordinary' if name in ORDINARY else
                'probe-first' if name in PROBE_FIRST else 'transition')
    outcome = ('error' if result.errors else 'failure' if result.failures else
               'skipped' if result.skipped else 'pass')
    return dict(method=name, category=category, outcome=outcome,
                events=events, diagnostic_caps=caps, requests=requests,
                comparisons=comparisons, tests_run=result.testsRun,
                failures=[text[-12000:] for _, text in result.failures],
                errors=[text[-12000:] for _, text in result.errors],
                skips=[reason for _, reason in result.skipped],
                codes={key: describe(code) for key, code in codes.values()})


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--group', choices=('ordinary', 'all'), default='ordinary')
    parser.add_argument('--method', action='append', choices=ROSTER)
    parser.add_argument('--event-cap', type=int, default=2000)
    args = parser.parse_args()
    if args.event_cap < 1:
        parser.error('--event-cap must be positive')
    selected = (tuple(name for name in ROSTER if name in args.method)
                if args.method else
                ORDINARY + PROBE_FIRST if args.group == 'ordinary' else ROSTER)
    describe = load_describe()
    print(json.dumps(dict(python=sys.version, selected=selected,
                         caveat='Diagnostic snapshots allocate and read f_lineno/f_locals; '
                                'primitive IDs may name temporary Python view boxes. '
                                'no stopped-phase authority is inferred from event names. '
                                'Original assertions remain active. This is not qualification.',
                         rows=[run_case(name, args.event_cap, describe) for name in selected]),
                     indent=2))


if __name__ == '__main__':
    main()
