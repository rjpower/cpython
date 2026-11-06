class Names(str): pass
class Obj:
    @property
    def val(self): return 12.5
    @property
    def missing(self): raise AttributeError('missing')
    @property
    def fail(self): raise ValueError('failure')
obj=Obj()
for args in ((), (obj,), (obj, 'val'), (obj, Names('val')), (obj, 'missing'), (obj, 'missing', 17), (obj, 'fail', 17), (5, 'real'), (5, 'missing', 1.5), (obj, 5), (obj, 1.5, 0), (obj, None), (obj,'x',1,2)):
    try: print('ok',repr(getattr(*args)))
    except Exception as exc: print('err',type(exc).__name__,str(exc))
try: getattr(obj, name='val')
except Exception as exc: print('kw',type(exc).__name__,str(exc))
import gc, weakref
class Reentrant:
    def __getattribute__(self, name):
        holder.clear()
        gc.collect()
        print('reentrant alive', ref() is not None)
        return 21
holder = [Names('callback')]
ref = weakref.ref(holder[0])
print('reentrant result', getattr(Reentrant(), holder[0]))
