"""Dump a digest of every code object compiled from Lib, for comparing two
compilers.  Usage: python dump_code.py LIBDIR OUT"""
import hashlib, json, os, sys, warnings

def walk(co, out, prefix):
    h = hashlib.sha1()
    for part in (co.co_code, co.co_linetable, co.co_exceptiontable,
                 str(co.co_stacksize).encode(), repr(co.co_consts_digest if False else '').encode()):
        h.update(part)
    out[prefix + co.co_qualname + ':' + str(co.co_firstlineno)] = h.hexdigest()[:16]
    for c in co.co_consts:
        if hasattr(c, 'co_code'):
            walk(c, out, prefix)

def main():
    lib, outpath = sys.argv[1], sys.argv[2]
    warnings.simplefilter('ignore')
    res = {}
    for root, dirs, files in os.walk(lib):
        dirs.sort()
        for f in sorted(files):
            if not f.endswith('.py'):
                continue
            p = os.path.join(root, f)
            try:
                src = open(p, 'rb').read()
                co = compile(src, p, 'exec')
            except SystemError as e:
                print('SystemError', p, e, file=sys.stderr)
                continue
            except Exception:
                continue
            d = {}
            walk(co, d, '')
            res[os.path.relpath(p, lib)] = d
    json.dump(res, open(outpath, 'w'))
    print(len(res), 'files')

main()
