"""Count the LOAD_FAST_BORROW words left over Lib, and for each, the first
op between it and its reader that the IR format marks as running code.

usage: BUILD/python -I boil/tools/measure/fold_count.py LIBDIR OUT.json
(compiler-copyprop-1.md: the folds that copy propagation does not make)"""
import collections, dis, json, os, sys, _opcode, warnings

RUNS_CODE = 128
formats = _opcode.get_ir_formats()

def runs_code(ins):
    fmt = formats[ins.opcode] if ins.opcode < len(formats) else None
    return fmt is not None and fmt[0] & RUNS_CODE

def walk(co):
    yield co
    for c in co.co_consts:
        if hasattr(c, 'co_code'):
            yield from walk(c)

def main(lib, out):
    loads = words = temps = 0
    killers = collections.Counter()
    files = 0
    for root, dirs, names in os.walk(lib):
        dirs[:] = sorted(d for d in dirs if d not in ('__pycache__', 'site-packages'))
        for n in sorted(names):
            if not n.endswith('.py'):
                continue
            path = os.path.join(root, n)
            try:
                with open(path, 'rb') as f:
                    src = f.read()
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore')
                    top = compile(src, path, 'exec')
            except (SyntaxError, ValueError, UnicodeDecodeError):
                continue
            files += 1
            for co in walk(top):
                ins = list(dis.get_instructions(co))
                words += len(ins)
                temps += co.co_stacksize
                for i, op in enumerate(ins):
                    if op.opname != 'LOAD_FAST_BORROW':
                        continue
                    loads += 1
                    for nxt in ins[i + 1:]:
                        if op.dst is not None and op.dst in (nxt.srcs or ()):
                            killers['(reader reached)'] += 1
                            break
                        if nxt.line_number != op.line_number:
                            killers['(line change)'] += 1
                            break
                        if runs_code(nxt):
                            killers[nxt.opname] += 1
                            break
                    else:
                        killers['(end)'] += 1
    json.dump({'files': files, 'loads': loads, 'words': words,
               'temporaries': temps,
               'first_stop': dict(killers.most_common())}, open(out, 'w'), indent=1)
    print(files, 'files', loads, 'LOAD_FAST_BORROW', words, 'words',
          temps, 'temporaries')

main(*sys.argv[1:])
