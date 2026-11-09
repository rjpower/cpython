import sys, dis, pathlib, collections
names = sys.argv[1:]
cnt = collections.Counter(); ex = {}
def walk(co, path):
    for i in dis.get_instructions(co):
        if i.opname in names:
            cnt[i.opname] += 1; ex.setdefault(i.opname, (path, co.co_name, i.positions.lineno))
    for c in co.co_consts:
        if hasattr(c, 'co_code'): walk(c, path)
for p in pathlib.Path('Lib').rglob('*.py'):
    try: co = compile(p.read_bytes(), str(p), 'exec')
    except Exception: continue
    walk(co, str(p))
print(cnt); print(ex)
