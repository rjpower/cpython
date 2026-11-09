class A: pass
for value in (5,1.5,2**55,None):
    A.x=value
    print('value',repr(A.x))
del A.x
for action in (lambda: delattr(A, 'missing'), lambda: setattr(int,'x',1), lambda: setattr(A, 1, 1)):
    try: action()
    except Exception as e: print('error',type(e).__name__,str(e))
default = None
class Name(str): pass
setattr(A,Name('x'),2)
print('name', A.x)
class Meta(type):
    @property
    def prop(cls): return 3
    @prop.setter
    def prop(cls,v): print('descriptor',repr(v))
class B(metaclass=Meta): pass
B.prop=4.5
class Old:
    def __del__(self):
        print('finalizer', C.x)
        C.y=42
class C: pass
C.x=Old()
C.x=19
print('final', C.x,C.y)
class D:
    def __len__(self): return 2
print('slot before',len(D()))
D.__len__=lambda self: 9
print('slot after',len(D()))
