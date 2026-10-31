"""For each code object of FILE matching the digests that differ, print the
offsets whose words differ from the oracle's, and whether every one lies
in code that neither control flow nor the exception table reaches.
Usage: python reach.py ORACLE_PICKLE FILE  (run twice: dump, then check)"""
import dis, sys, pickle, warnings
warnings.simplefilter('ignore')

def codes(co):
    yield co
    for c in co.co_consts:
        if hasattr(c, 'co_code'):
            yield from codes(c)

def reachable(co):
    instrs = list(dis.get_instructions(co))
    by_off = {i.offset: k for k, i in enumerate(instrs)}
    entries = [0] + [e.target for e in dis._parse_exception_table(co)]
    seen = set(); work = list(entries)
    stops = {'RETURN_VALUE', 'RAISE_VARARGS', 'RERAISE', 'JUMP_FORWARD',
             'JUMP_BACKWARD', 'JUMP_BACKWARD_NO_INTERRUPT', 'JUMP'}
    while work:
        off = work.pop()
        if off in seen or off not in by_off: continue
        seen.add(off)
        i = instrs[by_off[off]]
        if i.jump_target is not None:
            work.append(i.jump_target)
        if i.opname not in stops:
            k = by_off[off] + 1
            if k < len(instrs): work.append(instrs[k].offset)
    return seen, instrs

mode, path = sys.argv[1], sys.argv[2]
co = compile(open(path).read(), path, 'exec')
table = {}
for c in codes(co):
    table[c.co_qualname + ':' + str(c.co_firstlineno)] = (
        [bytes(c.co_code[k:k+8]) for k in range(0, len(c.co_code), 8)],
        c.co_exceptiontable, c.co_linetable)
if mode == 'dump':
    pickle.dump(table, open(sys.argv[3], 'wb'))
    sys.exit()
old = pickle.load(open(sys.argv[3], 'rb'))
for c in codes(co):
    key = c.co_qualname + ':' + str(c.co_firstlineno)
    a, b = old[key], table[key]
    if a == b: continue
    seen, _ = reachable(c)
    diffs = [k for k, (x, y) in enumerate(zip(a[0], b[0])) if x != y]
    ok = len(a[0]) == len(b[0]) and a[1] == b[1] and a[2] == b[2] and not (set(diffs) & seen)
    print('UNREACHABLE-ONLY' if ok else 'REACHABLE-DIFF', key, diffs[:8])
