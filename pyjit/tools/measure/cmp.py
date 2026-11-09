import json, sys
a = json.load(open(sys.argv[1])); b = json.load(open(sys.argv[2]))
nd = 0; tot = 0; files = []
for f in a:
    if f not in b:
        print('missing', f); continue
    for k, h in a[f].items():
        tot += 1
        if b[f].get(k) != h:
            nd += 1
            files.append((f, k))
print('total', tot, 'differ', nd)
for x in files: print(*x)
