class Alien:
    def __new__(cls, n): return n
    def __init__(self,n): print('BAD INITIALIZED')
print('alien', Alien(3), Alien(1.5))
class Base:
    def __new__(cls): return object.__new__(Child)
    def __init__(self): print('BASE INIT')
class Child(Base):
    def __init__(self): print('CHILD INIT')
print('actual',type(Base()).__name__)
for result in (1,1.5):
    class Bad:
        def __init__(self): return result
    try: Bad()
    except Exception as e: print('bad init',type(e).__name__,str(e))
class M(type):
    def __call__(cls,*a,**kw): print('META CALL',a,kw); return 42
class A(metaclass=M): pass
print('meta',A(1,x=2))
class C: pass
C.__new__=lambda cls,*a: object.__new__(cls)
C.__init__=lambda self,*a: setattr(self,'x',a)
print('mutated', C(1).x)
del C.__init__
del C.__new__
print('restored',type(C()).__name__)
for f,args,kw in ((int,(),{}),(int,(12,),{}),(float,(),{}),(float,(1.5,),{}),(object,(1,),{}),(type,(),{})):
    try: print('builtin',f.__name__,repr(f(*args,**kw)))
    except Exception as e: print('error',type(e).__name__,str(e))
for invalid in (17, 1.5):
    class InvalidInit:
        __init__ = invalid
    try: InvalidInit()
    except Exception as e: print('noncallable init',type(e).__name__,str(e))
class InlineDescriptor:
    def __get__(self, obj, typ): return 17
class BadDescriptor:
    __init__ = InlineDescriptor()
try: BadDescriptor()
except Exception as e: print('descriptor init',type(e).__name__,str(e))
