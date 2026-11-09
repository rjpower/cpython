"""Coordinator-run native matrix for checked canonical numeric final stores.

The displaced-owner case checks the selected RC implementation's publication
protocol; it does not assert a Python language guarantee of prompt finalization.
"""
import gc
import json
import math
import operator
import sys

from _testinternalcapi import SPECIALIZATION_THRESHOLD, get_ir


OPERATIONS = {
    'add': ('+', operator.add, 'BINARY_OP_ADD_FLOAT', (math.inf, -math.inf)),
    'sub': ('-', operator.sub, 'BINARY_OP_SUBTRACT_FLOAT', (math.inf, math.inf)),
    'mul': ('*', operator.mul, 'BINARY_OP_MULTIPLY_FLOAT', (math.inf, 0.0)),
    'div': ('/', operator.truediv, 'BINARY_OP_TRUEDIV_FLOAT', (math.inf, math.inf)),
}
WARMUP = max(100, SPECIALIZATION_THRESHOLD + 16)


def check_value(actual, expected):
    assert type(actual) is type(expected), (type(actual), type(expected))
    if isinstance(expected, float) and math.isnan(expected):
        assert math.isnan(actual), actual
        return
    assert actual == expected, (actual, expected)
    if isinstance(expected, float) and expected == 0.0:
        assert math.copysign(1.0, actual) == math.copysign(1.0, expected)


def make_target(name, symbol, destination, extra=None):
    expression = f'a {symbol} b'
    body = {
        'local': f'    result = {expression}\n    return result\n',
        'temporary': f'    return ({expression},)\n',
        'discard': f'    {expression}\n    return None\n',
        'displaced': f'    result = Resource()\n    result = {expression}\n    return result\n',
    }[destination]
    namespace = {} if extra is None else dict(extra)
    source = 'def calculate(a, b):\n' + body
    exec(compile(source, f'<canonical-numeric:{name}:{destination}>', 'exec'), namespace)
    return namespace['calculate']


def require_float_route(target, handler, destination):
    words = get_ir(target.__code__, True)
    matches = [word for word in words if word[0] == handler]
    assert len(matches) == 1, (destination, handler, words)
    dst = matches[0][1]
    if destination in ('local', 'displaced'):
        assert dst < target.__code__.co_nlocals, (destination, dst, words)
    elif destination == 'discard':
        assert dst == 4095, (destination, dst, words)
    else:
        assert target.__code__.co_nlocals <= dst < 4095, (destination, dst, words)
    return {'handler': handler, 'dst': dst}


def matrix(name, symbol, reference, handler, nan_result, destination):
    target = make_target(name, symbol, destination)
    for _ in range(WARMUP):
        target(1.25, 2.5)
    route = require_float_route(target, handler, destination)
    cases = [
        ('finite', 1.25, 2.5),
        ('mixed-inline-int', 3, 2.5),
        ('negative-zero', -0.0, 2.0),
        ('two-negative-zeros', -0.0, -0.0),
        ('infinite', math.inf, 2.0),
        ('nan-result', *nan_result),
        ('boxed-nan-input', float('nan'), 1.0),
        ('large-int-fallback', 1 << 100, 2),
        ('zero-right', 2.0, 0.0),
    ]
    checks = []
    for case, left, right in cases:
        try:
            expected = reference(left, right)
        except Exception as error:
            try:
                target(left, right)
            except Exception as actual:
                assert type(actual) is type(error), (case, type(actual), type(error))
            else:
                raise AssertionError((case, 'missing exception', type(error)))
            checks.append({'case': case, 'exception': type(error).__name__})
            continue
        actual = target(left, right)
        if destination == 'discard':
            assert actual is None
        else:
            check_value(actual[0] if destination == 'temporary' else actual, expected)
        checks.append({'case': case, 'reference_type': type(expected).__name__})
    return {'operation': name, 'destination': destination, 'route': route, 'checks': checks}


def displaced(name, symbol, reference, handler):
    observed = []
    target = None

    class Resource:
        def __del__(self):
            frame = sys._getframe(1)
            while frame is not None and frame.f_code is not target.__code__:
                frame = frame.f_back
            assert frame is not None, 'selected RC displacement callback lost caller'
            observed.append(frame.f_locals['result'])

    target = make_target(name, symbol, 'displaced', {'Resource': Resource})
    expected = reference(1.25, 2.5)
    for _ in range(WARMUP):
        check_value(target(1.25, 2.5), expected)
    route = require_float_route(target, handler, 'displaced')
    gc.collect()
    assert len(observed) == WARMUP, len(observed)
    for value in observed:
        check_value(value, expected)
    return {'operation': name, 'destination': 'displaced', 'route': route,
            'callbacks': len(observed), 'contract': 'selected RC publication before retirement'}


def main():
    results = []
    for name, (symbol, reference, handler, nan_result) in OPERATIONS.items():
        for destination in ('local', 'temporary', 'discard'):
            results.append(matrix(name, symbol, reference, handler, nan_result, destination))
        results.append(displaced(name, symbol, reference, handler))
    print(json.dumps({'python': sys.version, 'warmup': WARMUP, 'results': results}, indent=2))


if __name__ == '__main__':
    main()
