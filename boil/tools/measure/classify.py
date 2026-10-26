import dis, pathlib, collections, sys
ctx = {"COPY": collections.Counter(), "SWAP": collections.Counter(), "POP_TOP": collections.Counter()}
def walk(co):
    ins = [i for i in dis.get_instructions(co)]
    for n, i in enumerate(ins):
        if i.opname in ctx:
            prev = ins[n-1].opname if n else "-"
            nxt = ins[n+1].opname if n + 1 < len(ins) else "-"
            key = (f"{i.opname} {i.arg}", "after " + prev, "before " + nxt)
            if i.opname == "POP_TOP": key = ("after " + prev,)
            ctx[i.opname][key] += 1
    for c in co.co_consts:
        if hasattr(c, 'co_code'): walk(c)
for p in pathlib.Path('Lib').rglob('*.py'):
    try: co = compile(p.read_bytes(), str(p), 'exec')
    except Exception: continue
    walk(co)
for k, c in ctx.items():
    print("==", k, sum(c.values()))
    for key, n in c.most_common(12): print(f"  {n:7d} {' / '.join(key)}")
