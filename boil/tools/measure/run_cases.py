import asyncio, contextlib
exec(open('/tmp/boil3b/cases.py').read().split("ns = {}")[0])
ns = {}
exec(compile(src, 'cases', 'exec'), ns)
class O:
    x = 1; y = 2; a = 3; b = 0
    def bit_length(self): return 1
print(ns['f1'](O()), ns['f2']([1,2,3],[4,5,6],3), ns['f3'](1,2), ns['f4'](5))
o = O(); ns['f5'](o, 4); print(o.a, ns['f6'](1, 2), ns['f6'](0, 2), ns['f7'](O()), ns['f8'](O()))
class CM:
    def __enter__(self): return self
    def __exit__(self, *a): print('exit', a[0]); return False
    async def __aenter__(self): return self
    async def __aexit__(self, *a): print('aexit'); return False
    def go(self): print('go')
    async def ago(self): pass
ns['f9'](CM())
class ACM(CM):
    def go(self):
        async def g(): print('ago')
        return g()
asyncio.run(ns['f10'](ACM()))
class It:
    def close(self): print('closed')
g = ns['f11'](It()); print(next(g)); g.close()
print(ns['f12'](lambda *a: len(a), 1), ns['f13'](1, 2), ns['f14'](1, 2), ns['f15'](3, lambda z: z * 2))
try:
    ns['f15'](3, lambda z: 1 / 0)
except ZeroDivisionError as e:
    print('raised', type(e).__name__)
ns['len0'] = ns['len1'] = ns['len2'] = ns['len3'] = ns['len4'] = ns['len5'] = ns['len6'] = 9
l = ns['f17'](); print(len(l), sum(l))
print(ns['f16']("ab", 5))
