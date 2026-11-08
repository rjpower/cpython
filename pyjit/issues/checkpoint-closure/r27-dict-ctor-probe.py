for cls in (dict, frozendict):
    for x in (1, 1.25):
        try:
            cls(x)
        except TypeError as exc:
            assert str(exc) == f"'{type(x).__name__}' object is not iterable", str(exc)
        else:
            raise AssertionError('accepted scalar')
    try:
        cls(iter([1]))
    except TypeError as exc:
        assert str(exc) == 'object is not iterable', str(exc)
        assert exc.__notes__ == ['Cannot convert dictionary update sequence element #0 to a sequence']
    else:
        raise AssertionError('accepted scalar pair')
    assert cls(a=1, b=1.25) == {'a': 1, 'b': 1.25}
    assert cls(iter([('x', 3)])) == {'x': 3}
f = frozendict(a=1)
assert frozendict(f) is f
seen = []
class Meta(type):
    def __call__(cls, *args, **kw):
        seen.append('meta')
        return super().__call__(*args, **kw)
class D(dict, metaclass=Meta):
    def __new__(cls, *args, **kw):
        seen.append('new')
        return super().__new__(cls)
    def __init__(self, *args, **kw):
        seen.append('init')
        super().__init__(*args, **kw)
assert D(a=1) == {'a': 1} and seen == ['meta', 'new', 'init']
seen.clear()
class F(frozendict, metaclass=Meta):
    def __new__(cls, *args, **kw):
        seen.append('new')
        return super().__new__(cls, *args, **kw)
    def __init__(self, *args, **kw):
        seen.append('init')
assert F(a=1) == {'a': 1} and seen == ['meta', 'new', 'init']
print('dict constructor probe passed')
