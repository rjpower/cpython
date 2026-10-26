import dis, sys, io
path, qual = sys.argv[1], sys.argv[2]
co = compile(open(path).read(), path, 'exec')
def find(co):
    if co.co_qualname == qual: return co
    for c in co.co_consts:
        if hasattr(c, 'co_code'):
            r = find(c)
            if r: return r
c = find(co)
dis.dis(c, depth=0, show_offsets=True)
print('stacksize', c.co_stacksize)
