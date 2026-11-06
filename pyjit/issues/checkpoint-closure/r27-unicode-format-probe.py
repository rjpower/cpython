assert '{} {name} {!r} {!a}'.format(2, 'é', 'é', name=3.5) == "2 3.5 'é' '\\xe9'"
assert '{x[2]}'.format(x={2: 42}) == '42'
assert '{x:{w}}'.format(x=7, w=3) == '  7'
assert '{a}'.format_map({'a': 5}) == '5'
try:
    '{0}'.format_map({})
except ValueError as exc:
    assert str(exc) == 'Format string contains positional fields'
else:
    raise AssertionError('positional format_map field accepted')
try:
    '{1}'.format(0)
except IndexError as exc:
    assert str(exc) == 'Replacement index 1 out of range for positional args tuple'
else:
    raise AssertionError('missing field accepted')
events = []
class Field:
    def __getattribute__(self, name):
        if name == 'value':
            events.append('attr')
            return {'key': 3}
        return object.__getattribute__(self, name)
assert '{0.value[key]}'.format(Field()) == '3'
assert events == ['attr']
class Bad:
    def __format__(self, spec):
        return 3
try:
    '{}'.format(Bad())
except TypeError:
    pass
else:
    raise AssertionError('invalid __format__ result accepted')
print('unicode format probe passed')
assert str.format('{}:{name}', 8, name=9) == '8:9'
import gc
import weakref
holder = []
events = []
class Reentrant:
    def __repr__(self):
        holder.clear()
        gc.collect()
        events.append('repr')
        return 'ok'
    def __del__(self):
        events.append('del')
obj = Reentrant()
ref = weakref.ref(obj)
holder.append(obj)
del obj
assert '{0[0]!r}'.format(holder) == 'ok'
assert events == ['repr', 'del'] and ref() is None, events
class Raising:
    def __getitem__(self, key):
        raise RuntimeError('getitem first')
try:
    '{0[x]!r}'.format(Raising())
except RuntimeError as exc:
    assert str(exc) == 'getitem first'
else:
    raise AssertionError('lookup error lost')
print('unicode format reentrancy and descriptor probes passed')
