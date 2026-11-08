import sqlite3

con = sqlite3.connect(':memory:')
for value in (None, 0, -1, 2**51, 2**63-1, -2**63, 1.5, True, 'x', b'x'):
    result = con.execute('select ?', (value,)).fetchone()[0]
    print('value', type(value).__name__, repr(result))
for value in (2**63, -2**63-1):
    try: con.execute('select ?', (value,))
    except Exception as e: print('overflow', type(e).__name__, str(e))
class IntSub(int): pass
class FloatSub(float): pass
for value in (IntSub(31), FloatSub(2.5)):
    print('subclass', repr(con.execute('select ?', (value,)).fetchone()[0]))
class Mapping(dict):
    def __getitem__(self, key):
        print('lookup', key)
        return super().__getitem__(key)
print('mapping', con.execute('select :x', Mapping(x=42)).fetchone())
class X:
    def __conform__(self, proto):
        print('conform', proto.__name__)
        return 7
print('conform result', con.execute('select ?', (X(),)).fetchone())
sqlite3.register_adapter(X, lambda x: 8)
print('registered', con.execute('select ?', (X(),)).fetchone())
for args in ((object(),), (object(), sqlite3.PrepareProtocol, None),
             (object(), sqlite3.PrepareProtocol, 19)):
    try: print('alternate', repr(sqlite3.adapt(*args)))
    except Exception as e: print('alternate', type(e).__name__, str(e))
class Proto:
    @staticmethod
    def __adapt__(obj):
        print('proto adapt')
        return 29
print('protocol', sqlite3.adapt(1, Proto))
class Raises:
    def __conform__(self, proto): raise ValueError('callback')
try: sqlite3.adapt(Raises())
except Exception as e: print('raises', type(e).__name__, str(e))
