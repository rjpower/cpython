import math
seen = []
class Number:
    def __init__(self, label, value):
        self.label, self.value = label, value
    def __float__(self):
        seen.append(self.label)
        if isinstance(self.value, Exception):
            raise self.value
        return self.value
assert math.dist((Number('p', 3.0),), (Number('q', 7.0),)) == 4.0
assert seen == ['p', 'q']
seen.clear()
try:
    math.dist((Number('p', ValueError('bad')),), (Number('q', 7.0),))
except ValueError as exc:
    assert str(exc) == 'bad' and seen == ['p']
else:
    raise AssertionError('conversion error lost')
assert math.dist((0, 2**49, 1.5), (0, 2**49 + 3, 5.5)) == 5.0
try:
    math.dist((1,), (1, 2))
except ValueError as exc:
    assert str(exc) == 'both points must have the same number of dimensions'
else:
    raise AssertionError('dimension mismatch accepted')
print('math.dist value conversion probe passed')
