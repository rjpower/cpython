# Frame slots (co_nlocalsplus + co_stacksize) and temporaries over Lib
import sys, pathlib, statistics, json
def walk(co, acc):
    acc.append((co.co_nlocals + len(co.co_cellvars) + len(co.co_freevars), co.co_stacksize, co.co_qualname, co.co_filename))
    for c in co.co_consts:
        if hasattr(c, 'co_code'): walk(c, acc)
acc = []
for p in sorted(pathlib.Path('Lib').rglob('*.py')):
    try: co = compile(p.read_bytes(), str(p), 'exec')
    except Exception as e:
        if isinstance(e, SystemError): print("SystemError", p, e)
        continue
    walk(co, acc)
tot = sorted(a + b for a, b, *_ in acc); st = sorted(b for _, b, *_ in acc)
def q(xs, f): return xs[min(len(xs) - 1, int(f * len(xs)))]
print("code objects", len(acc))
for name, xs in (("slots", tot), ("temps", st)):
    print(f"{name}: mean {statistics.mean(xs):.2f} median {q(xs,.5)} p90 {q(xs,.9)} p99 {q(xs,.99)} max {xs[-1]} sum {sum(xs)}")
big = max(acc, key=lambda a: a[0] + a[1]); print("largest", big)
if len(sys.argv) > 1: json.dump(acc, open(sys.argv[1], 'w'))
