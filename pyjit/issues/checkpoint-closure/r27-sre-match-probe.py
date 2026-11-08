import re
m = re.match(r'(?P<word>a)(b)?', 'a')
class I(int): pass
class Index:
    def __index__(self): return 1
class Broken:
    def __index__(self): raise ValueError('index callback')
class Key:
    def __hash__(self): print('hash'); return hash('word')
    def __eq__(self, other): print('eq', other); return other == 'word'
for key in (0, 1, 2, True, I(1), Index(), -1, 2**80, 1.5, [], 'word', 'missing', Key(), Broken()):
    print('key', type(key).__name__)
    for method in (lambda: m[key], lambda: m.group(key), lambda: m.start(key), lambda: m.end(key), lambda: m.span(key)):
        try: print('result', repr(method()))
        except Exception as exc: print('error', type(exc).__name__, str(exc))
print('bytes', re.match(b'(a)', b'a')[1])
