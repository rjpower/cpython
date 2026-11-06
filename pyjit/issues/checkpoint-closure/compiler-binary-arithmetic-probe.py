"""Focused behavioral probes for the value-native binary constant folder."""

import dis
import math


cases = {
    'x = 2*3': 6,
    'x = 2**10': 1024,
    'x = 1<<17': 131072,
    'x = 25%7': 4,
    'x = 0*123456789123456789123456789': 0,
    'x = 3*"abc"': 'abcabcabc',
    'x = (1, 2)*3': (1, 2, 1, 2, 1, 2),
}
for source, expected in cases.items():
    namespace = {}
    exec(compile(source, '<fold-probe>', 'exec'), namespace)
    assert namespace['x'] == expected, source

namespace = {}
exec(compile('x = (-1)**0.5', '<fold-probe>', 'exec'), namespace)
assert abs(namespace['x'] - 1j) < 1e-15

for source in ('x = 2**1000', 'x = 1<<1000', 'x = "x"*4097'):
    code = compile(source, '<refusal-probe>', 'exec')
    assert any(ins.opname == 'BINARY_OP' for ins in dis.get_instructions(code)), source
    namespace = {}
    exec(code, namespace)
    assert namespace['x'] is not None

code = compile('x = (1, 1.0, -0.0, 0.0)', '<cache-probe>', 'exec')
namespace = {}
exec(code, namespace)
assert math.copysign(1.0, namespace['x'][2]) == -1.0
assert math.copysign(1.0, namespace['x'][3]) == 1.0
print('arithmetic, refusal, and cache probes passed')
