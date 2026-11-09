class D(dict):
    def __missing__(self, key):
        return key + 1

for typ in (dict, frozendict):
    obj = typ({1: 2, 1.5: 3.5})
    assert obj.__getitem__(1) == 2
    assert typ.__getitem__(obj, 1.5) == 3.5
    for call in (lambda: obj.__getitem__(), lambda: obj.__getitem__(1, 2),
                 lambda: obj.__getitem__(key=1), lambda: typ.__getitem__([], 1)):
        try:
            call()
        except TypeError:
            pass
        else:
            raise AssertionError(call)
assert D().__getitem__(9) == 10
for call, expected in ((lambda: D().__getitem__(), 'dict.__getitem__()'),
                       (lambda: dict.__getitem__(D()), 'dict.__getitem__()')):
    try:
        call()
    except TypeError as exc:
        assert str(exc).startswith(expected), str(exc)
    else:
        raise AssertionError(call)
class BadHash:
    def __hash__(self):
        raise RuntimeError('hash')
try:
    {}.__getitem__(BadHash())
except RuntimeError as exc:
    assert str(exc) == 'hash'
else:
    raise AssertionError('hash error not propagated')
print('getitem direct, subclasses, arguments, and exceptions passed')
