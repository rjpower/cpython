print('int', int.__new__(int, 17), int.__new__(int, 2**80))
print('float', repr(float.__new__(float,-0.0)))
class I(int): pass
class F(float): pass
print('sub', type(int.__new__(I,17)).__name__, type(float.__new__(F,1.5)).__name__)
class A:
    def __new__(cls,*args,**kwargs):
        print('A new',args,kwargs)
        return 17
    def __init__(self,*args,**kwargs): print('BAD INIT')
print('python new',A.__new__(A, 3, value=4))
class B:
    def __new__(cls): return 'different'
print('other', B.__new__(B))
class C: pass
C.__new__ = lambda cls, x: x
print('changed', C.__new__(C, 9))
for fn,args,kw in ((int.__new__,(),{}), (int.__new__,('bad',),{}), (int.__new__,(float,1),{}), (object.__new__,(dict,),{}), (int.__new__,(int,1,2,3),{}), (float.__new__,(float,),{'bad':1})):
    try: print('ok',repr(fn(*args,**kw)))
    except Exception as e: print('err',type(e).__name__,str(e))
