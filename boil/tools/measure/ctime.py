import pathlib, time
srcs = []
for p in sorted(pathlib.Path('Lib').rglob('*.py')):
    try: s = p.read_bytes(); compile(s, str(p), 'exec'); srcs.append((s, str(p)))
    except Exception: pass
best = 1e9
for _ in range(3):
    t = time.perf_counter()
    for s, p in srcs: compile(s, p, 'exec')
    best = min(best, time.perf_counter() - t)
print(f"{len(srcs)} files, best of 3: {best:.2f}s")
