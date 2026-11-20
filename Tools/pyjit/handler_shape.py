#!/usr/bin/env python3
"""Report the shape of an evaluator's handlers from a compiled object.

Two backends:

  tc  one C function per handler (`<prefix>NAME`, default `_TAIL_CALL_`).
  cg  one function (default `_PyEval_EvalFrameDefault`) whose handlers are
      labels.  The labels come from a static table of label addresses (in a
      `.o`, an array with R_X86_64_64 relocations to `.text+offset`).  The
      table symbol and the source of the index -> name mapping are options.

Per handler and in total it reports the frame, stack canaries, indirect jumps,
the shortest path from the handler's entry to its dispatch jump (the "fast
path": instructions, stack references, register-register moves, rsp
adjustments, calls) and every call with its target.  The same figures are
summed over a hot set read from kernel symbol files.

Fast path definition (from the PyJIT probe's fastpath.py): a path goes from the
entry to the first indirect jump (or ret).  A call costs 1000 extra, so a path
with a call is chosen only when all paths have one.  A jump whose target is
relocated (another section or function: deopt, cold exit, direct tail call)
ends no path.  nops are free.  In a linked executable there are no relocations;
jumps leaving the function end no path.

Usage:
  handler_shape.py OBJ [--json OUT.json] [options]            # tc (default)
  handler_shape.py OBJ --mode cg --names opcode-ids:Include/opcode_ids.h
  handler_shape.py --compare A.json B.json
  handler_shape.py --share KCAT_FILE...                       # handler share

Standard library plus `objdump`/`readelf`.
"""
import argparse
import collections
import heapq
import json
import re
import subprocess
import sys

E59 = ('/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/'
       'pyjit-link-wave1/register-semantics59')
KERNELS = ['deltablue', 'raytrace', 'coroutines', 'deepcopy_memo', 'richards']
DEFAULT_KCAT = [f'{E59}/execution79-call-ledger/raw/kcat-c78-{k}-symbols.txt'
                for k in KERNELS]

NOP_RE = re.compile(r'(nop|data16|cs nop|xchg\s+%ax,%ax)')
INDIRECT_RE = re.compile(r'(notrack )?(bnd )?jmp\s+\*')
JUMP_RE = re.compile(r'(j[a-z]+)\s+([0-9a-f]+)\b')
REGMOVE_RE = re.compile(r'mov\s+%r\w+,%r\w+$|mov\s+%e\w+,%e\w+$')
RSP_OP_RE = re.compile(r'(sub|add)\s+\$0x[0-9a-f]+,%rsp|push\s|pop\s')
STACK_REF_RE = re.compile(r'\((%rsp|%rbp)[,)]')
PROBE_STACK_REF_RE = re.compile(r'\((%rsp|%rbp)\)')
CANARY_TEXT = '%fs:0x28'
CANARY_SYM = '__stack_chk_fail'
CALL_COST = 1000


# ---------------------------------------------------------------------------
# Disassembly

class Func:
    """Instructions of one function: parallel lists, indexed by position."""

    def __init__(self, section, name):
        self.section = section
        self.name = name
        self.addr = []
        self.text = []
        self.relocs = []      # list of relocation targets per instruction
        self.index = {}       # address -> position

    def add(self, addr, text):
        self.index[addr] = len(self.addr)
        self.addr.append(addr)
        self.text.append(text)
        self.relocs.append([])


def disassemble(obj, probe_compat=False):
    """Return {(section, name): Func} for every function in `obj`.

    With `probe_compat`, relocation lines count as instructions and relocations
    are not attached, as probe/fastpath.py behaved (to reproduce its numbers).
    """
    out = subprocess.run(['objdump', '-dr', '--no-show-raw-insn', obj],
                         capture_output=True, text=True, check=True).stdout
    funcs = {}
    section = None
    cur = None
    for line in out.splitlines():
        m = re.match(r'^Disassembly of section (\S+):', line)
        if m:
            section = m.group(1)
            cur = None
            continue
        m = re.match(r'^([0-9a-f]+) <(.+)>:$', line)
        if m:
            cur = Func(section, m.group(2))
            funcs[(section, cur.name)] = cur
            continue
        if cur is None:
            continue
        m = re.match(r'^\s+([0-9a-f]+):\s+(\S.*)$', line)
        if not m:
            continue
        text = m.group(2).strip()
        if text.startswith('R_X86_64_') or text.startswith('R_'):
            if probe_compat:
                cur.add(int(m.group(1), 16), text)
            elif cur.addr:
                parts = text.split(None, 1)
                if len(parts) == 2:
                    cur.relocs[-1].append(parts[1].strip())
            continue
        cur.add(int(m.group(1), 16), text)
    return funcs


def strip_addend(sym):
    return re.sub(r'[+-]0x[0-9a-f]+$', '', sym)


def call_target(f, i):
    if f.relocs[i]:
        return strip_addend(f.relocs[i][0])
    m = re.search(r'<([^>+]+)', f.text[i])
    return m.group(1) if m else '*indirect'


def is_call(t):
    return t.startswith('call') or t.startswith('notrack call')


def is_nop(t):
    return bool(NOP_RE.match(t))


# ---------------------------------------------------------------------------
# Per-function analyses

def successors(f, i, compat=False):
    """Positions that follow instruction i, or [] when it ends a path."""
    t = f.text[i]
    if INDIRECT_RE.match(t) or t.startswith('ret') or t.startswith('ud2'):
        return []
    succ = []
    m = JUMP_RE.match(t)
    if m:
        relocated = bool(f.relocs[i]) and not compat
        if not relocated:
            target = int(m.group(2), 16)
            if target in f.index:
                succ.append(f.index[target])
        if m.group(1) != 'jmp' and i + 1 < len(f.addr):
            succ.append(i + 1)
    elif i + 1 < len(f.addr):
        succ.append(i + 1)
    return succ


def fast_path(f, start, compat=False):
    """Shortest path from instruction `start` to a dispatch jump (or ret)."""
    dist = {start: 0}
    prev = {}
    pq = [(0, start)]
    best = None
    while pq:
        d, i = heapq.heappop(pq)
        if best is not None and d >= best[0]:
            break
        if d > dist.get(i, d):
            continue
        t = f.text[i]
        nd = d + (0 if is_nop(t) else 1) + (CALL_COST if is_call(t) else 0)
        if INDIRECT_RE.match(t) or t.startswith('ret'):
            if best is None or nd < best[0]:
                best = (nd, i)
            continue
        for s in successors(f, i, compat):
            if s not in dist or nd < dist[s]:
                dist[s] = nd
                prev[s] = i
                heapq.heappush(pq, (nd, s))
    if best is None:
        return None
    path = [best[1]]
    while path[-1] != start:
        path.append(prev[path[-1]])
    path.reverse()
    texts = [f.text[i] for i in path]
    stack_re = PROBE_STACK_REF_RE if compat else STACK_REF_RE
    return {
        'insns': sum(1 for t in texts if not is_nop(t)),
        'stack': sum(1 for t in texts
                     if stack_re.search(t) and (compat or not t.startswith('lea'))),
        'regmoves': sum(1 for t in texts if REGMOVE_RE.match(t)),
        'rsp_adj': sum(1 for t in texts if RSP_OP_RE.match(t)),
        'calls': sum(1 for t in texts if is_call(t)),
    }


def prologue(f, start=0):
    """Frame of the first basic block: pushes and `sub $N,%rsp`."""
    pushes = sub = 0
    for i in range(start, len(f.addr)):
        t = f.text[i]
        if t.startswith('push'):
            pushes += 1
        m = re.match(r'sub\s+\$(0x[0-9a-f]+),%rsp', t)
        if m:
            sub += int(m.group(1), 16)
        if JUMP_RE.match(t) or is_call(t) or t.startswith('ret') or INDIRECT_RE.match(t):
            break
    return {'pushes': pushes, 'sub': sub, 'bytes': sub + 8 * pushes}


def instr_stats(f, indices):
    """Indirect jumps, canary references, rsp ops and calls over `indices`."""
    ijmp = canary = rsp_ops = 0
    calls = collections.Counter()
    for i in indices:
        t = f.text[i]
        if INDIRECT_RE.match(t):
            ijmp += 1
        if CANARY_TEXT in t or any(CANARY_SYM in r for r in f.relocs[i]):
            canary += 1
        if RSP_OP_RE.match(t):
            rsp_ops += 1
        if is_call(t):
            calls[call_target(f, i)] += 1
    return {'ijumps': ijmp, 'canary_refs': canary, 'rsp_ops': rsp_ops,
            'calls': dict(sorted(calls.items()))}


# ---------------------------------------------------------------------------
# Tail-call mode

def analyse_tc(funcs, prefix, compat):
    handlers = {}
    for (sec, name), f in funcs.items():
        m = re.fullmatch(re.escape(prefix) + r'(\w+?)(\.cold)?', name)
        if not m:
            continue
        h = handlers.setdefault(m.group(1), {'parts': []})
        h['parts'].append((f, bool(m.group(2))))
    result = {}
    for name, h in handlers.items():
        main = [f for f, cold in h['parts'] if not cold]
        if not main:
            continue
        f = main[0]
        stats = instr_stats(f, range(len(f.addr)))
        for cf, cold in h['parts']:
            if not cold:
                continue
            cs = instr_stats(cf, range(len(cf.addr)))
            stats['ijumps'] += cs['ijumps']
            stats['canary_refs'] += cs['canary_refs']
            stats['rsp_ops'] += cs['rsp_ops']
            for k, v in cs['calls'].items():
                stats['calls'][k] = stats['calls'].get(k, 0) + v
        pro = prologue(f)
        result[name] = {
            'section': f.section,
            'insns': sum(1 for t in f.text if not is_nop(t)),
            'frame': pro,
            'canary': stats['canary_refs'] > 0,
            'ijumps': stats['ijumps'],
            'rsp_ops': stats['rsp_ops'],
            'calls': stats['calls'],
            'path': fast_path(f, 0, compat),
        }
    return result


# ---------------------------------------------------------------------------
# Computed-goto mode

def read_names(spec):
    """Index -> handler name from `kind:FILE`.

    opcode-ids:FILE  `#define NAME 123` lines (Include/opcode_ids.h).
    macro:FILE       the `#define HANDLERS(X) X(A) X(B) ...` list, in order.
    labels:FILE      `[N] = &&L_NAME` initialisers of the probe's table.
    ir-targets:FILE  `[N] = &&TARGET_NAME` initialisers of `opcode_targets`
                     in Python/ir_targets.h (the generator tables are skipped).
    """
    kind, _, path = spec.partition(':')
    text = open(path).read()
    names = {}
    if kind == 'opcode-ids':
        for name, num in re.findall(r'^#define\s+([A-Z][A-Z0-9_]*)\s+(\d+)\s*$',
                                    text, re.M):
            if re.match(r'(HAVE|MIN|MAX)_', name):
                continue
            names.setdefault(int(num), name)
    elif kind == 'macro':
        body = text.split('#define HANDLERS(X)')[1].split('\n#define')[0]
        for n, name in enumerate(re.findall(r'X\((\w+)\)', body)):
            names[n] = name
    elif kind == 'labels':
        for num, name in re.findall(r'\[(\d+)\]\s*=\s*&&L_(\w+)', text):
            names[int(num)] = name
    elif kind == 'ir-targets':
        body = text.split('opcode_targets[')[1].split('};')[0]
        for num, name in re.findall(r'\[(\d+)\]\s*=\s*&&TARGET_(\w+)', body):
            names.setdefault(int(num), name)
    else:
        sys.exit(f'unknown --names kind {kind!r}')
    return names


def read_table(obj, symbol_re):
    """Return {index: (section, addend)} for the label-address table."""
    syms = subprocess.run(['readelf', '-sW', obj], capture_output=True,
                          text=True, check=True).stdout
    secs = subprocess.run(['readelf', '-SW', obj], capture_output=True,
                          text=True, check=True).stdout
    secname = {int(n): s for n, s in
               re.findall(r'^\s*\[\s*(\d+)\]\s+(\S+)', secs, re.M)}
    found = None
    for line in syms.splitlines():
        p = line.split()
        if len(p) >= 8 and re.fullmatch(symbol_re, p[7]) and p[3] == 'OBJECT':
            found = (int(p[1], 16), int(p[2]), secname.get(int(p[6])), p[7])
            break
    if not found:
        sys.exit(f'table symbol /{symbol_re}/ not found in {obj}')
    value, size, section, _ = found
    rel = subprocess.run(['objdump', '-r', obj], capture_output=True,
                         text=True, check=True).stdout
    table = {}
    cur = None
    for line in rel.splitlines():
        m = re.match(r'RELOCATION RECORDS FOR \[(.+)\]:', line)
        if m:
            cur = m.group(1)
            continue
        if cur != section:
            continue
        m = re.match(r'^([0-9a-f]+)\s+R_X86_64_64\s+(\S+?)(?:\+0x([0-9a-f]+))?$', line)
        if not m:
            continue
        off = int(m.group(1), 16)
        if value <= off < value + size:
            table[(off - value) // 8] = (m.group(2), int(m.group(3) or '0', 16))
    return table, section


def analyse_cg(funcs, obj, func_name, symbol_re, names):
    cands = [f for (s, n), f in funcs.items() if n == func_name]
    if not cands:
        sys.exit(f'function {func_name} not found in {obj}')
    f = cands[0]
    table, tsec = read_table(obj, symbol_re)
    # Group table indices by label address.
    labels = collections.OrderedDict()
    for idx in sorted(table):
        sec, off = table[idx]
        labels.setdefault((sec, off), []).append(idx)
    entries = {}     # name -> dict(entry position or None, section, aliases)
    for (sec, off), idxs in labels.items():
        named = [names[i] for i in idxs if i in names]
        name = named[0] if named else '_unnamed_%d' % idxs[0]
        if len(idxs) > 1 and not named:
            name = '_shared_unnamed'
        pos = f.index.get(off) if sec == f.section else None
        entries[name] = {'pos': pos, 'section': sec, 'aliases': named[1:],
                         'indices': idxs}
    all_idx = range(len(f.addr))
    total = instr_stats(f, all_idx)
    # Reach: positions reachable from each entry without passing a dispatch.
    reach = {}
    owners = collections.Counter()
    for name, e in entries.items():
        if e['pos'] is None:
            continue
        seen = {e['pos']}
        stack = [e['pos']]
        while stack:
            i = stack.pop()
            for s in successors(f, i):
                if s not in seen:
                    seen.add(s)
                    stack.append(s)
        reach[name] = seen
        owners.update(seen)
    result = {}
    for name, e in entries.items():
        rec = {'section': e['section'], 'cold': e['pos'] is None,
               'indices': e['indices'], 'aliases': e['aliases']}
        if e['pos'] is None:
            result[name] = rec
            continue
        own = [i for i in reach[name] if owners[i] == 1]
        st = instr_stats(f, own)
        rs = instr_stats(f, reach[name])
        rec.update({
            'insns': sum(1 for i in own if not is_nop(f.text[i])),
            'insns_reach': sum(1 for i in reach[name] if not is_nop(f.text[i])),
            'canary': st['canary_refs'] > 0,
            'ijumps': st['ijumps'],
            'ijumps_reach': rs['ijumps'],
            'rsp_ops': st['rsp_ops'],
            'calls': st['calls'],
            'path': fast_path(f, e['pos']),
        })
        result[name] = rec
    func = {
        'name': f.name,
        'table_section': tsec,
        'labels': len(entries),
        'labels_in_text': sum(1 for e in entries.values() if e['pos'] is not None),
        'frame': prologue(f),
        'canary_refs': total['canary_refs'],
        'ijumps': total['ijumps'],
        'insns': sum(1 for t in f.text if not is_nop(t)),
        'rsp_ops': total['rsp_ops'],
        'calls': total['calls'],
    }
    return result, func


# ---------------------------------------------------------------------------
# Hot set and handler share from kernel symbol files

KCAT_RE = re.compile(r'^\s*([\d.]+)%\s+(\d+)\s+(.*?)\s*(\S+)\s*$')


def read_kcat(path):
    rows = []
    for line in open(path):
        m = KCAT_RE.match(line)
        if m:
            rows.append((float(m.group(1)), int(m.group(2)), m.group(4)))
    return rows


def hot_set(files, fraction, tc_prefix='_TAIL_CALL_'):
    """Handler names covering `fraction` of handler instructions across files."""
    total = collections.Counter()
    for path in files:
        for share, instr, sym in read_kcat(path):
            if sym.startswith(tc_prefix):
                total[sym[len(tc_prefix):]] += instr
    grand = sum(total.values())
    names, acc = [], 0
    for name, n in total.most_common():
        if acc >= fraction * grand:
            break
        names.append(name)
        acc += n
    return sorted(names)


def share_report(files, tc_prefix='_TAIL_CALL_'):
    lines = ['%-14s %12s %8s %8s %8s' %
             ('kernel', 'instr/loop', 'handler%', 'helper%', 'listed%')]
    rows = []
    for path in files:
        k = re.search(r'kcat-\w+?-(\w+?)-symbols', path)
        rs = read_kcat(path)
        handler = sum(s for s, n, sym in rs if sym.startswith(tc_prefix))
        listed = sum(s for s, n, sym in rs)
        head = open(path).readline()
        m = re.search(r'share x (\d+)', head)
        rows.append({'kernel': k.group(1) if k else path,
                     'instr_per_loop': int(m.group(1)) if m else None,
                     'handler_pct': round(handler, 2),
                     'helper_pct': round(listed - handler, 2),
                     'listed_pct': round(listed, 2)})
    for r in rows:
        lines.append('%-14s %12s %8.2f %8.2f %8.2f' %
                     (r['kernel'], r['instr_per_loop'], r['handler_pct'],
                      r['helper_pct'], r['listed_pct']))
    mean = sum(r['handler_pct'] for r in rows) / len(rows)
    lines.append('mean handler share: %.2f%% (helpers = runtime functions '
                 'outside %s*)' % (mean, tc_prefix))
    return '\n'.join(lines), rows


# ---------------------------------------------------------------------------
# Summaries and output

PATH_KEYS = ['insns', 'stack', 'regmoves', 'rsp_adj', 'calls']


def summarise(handlers, names=None):
    s = {k: 0 for k in PATH_KEYS}
    s.update({'handlers': 0, 'no_path': 0, 'ijumps': 0, 'canaries': 0,
              'calls_total': 0, 'frame_bytes': 0, 'rsp_ops': 0})
    for name, h in handlers.items():
        if names is not None and name not in names:
            continue
        s['handlers'] += 1
        if h.get('cold'):
            continue
        s['ijumps'] += h['ijumps']
        s['canaries'] += 1 if h['canary'] else 0
        s['calls_total'] += sum(h['calls'].values())
        s['rsp_ops'] += h['rsp_ops']
        if 'frame' in h:
            s['frame_bytes'] += h['frame']['bytes']
        if h['path'] is None:
            s['no_path'] += 1
            continue
        for k in PATH_KEYS:
            s[k] += h['path'][k]
    return s


COLS = '%-36s %5s %4s %3s %5s %5s %5s %4s %5s %5s'
HEAD = COLS % ('handler', 'frame', 'rsp#', 'can', 'ijmp', 'path', 'stk', 'rr', 'adj',
               'calls')


def row(name, h, mode):
    if h.get('cold'):
        return '%-36s cold section (%s)' % (name, h['section'])
    p = h['path']
    frame = h['frame']['bytes'] if 'frame' in h else '-'
    ncalls = sum(h['calls'].values())
    if p is None:
        return COLS % (name, frame, h['rsp_ops'], 'Y' if h['canary'] else '.',
                       h['ijumps'], 'none', '-', '-', '-', '%d/%d' % (0, ncalls))[:200]
    return COLS % (name, frame, h['rsp_ops'], 'Y' if h['canary'] else '.',
                   h['ijumps'], p['insns'], p['stack'], p['regmoves'],
                   p['rsp_adj'], '%d/%d' % (p['calls'], ncalls))


def format_text(rep, show_calls):
    out = []
    out.append('object: %s' % rep['object'])
    out.append('mode: %s' % rep['mode'])
    if rep['mode'] == 'cg':
        fn = rep['function']
        fr = fn['frame']
        out.append('evaluator: %s' % fn['name'])
        out.append('  frame: sub $0x%x,%%rsp (%d bytes) + %d pushes = %d bytes '
                   '(+8 return address)' % (fr['sub'], fr['sub'], fr['pushes'],
                                            fr['bytes']))
        out.append('  canary references (%%fs:0x28 or %s): %d' %
                   (CANARY_SYM, fn['canary_refs']))
        out.append('  indirect jumps: %d; labels: %d (%d in .text, table in %s)' %
                   (fn['ijumps'], fn['labels'], fn['labels_in_text'],
                    fn['table_section']))
        out.append('  instructions: %d' % fn['insns'])
        out.append('  check: indirect jumps >= labels in .text: %s' %
                   ('OK' if fn['ijumps'] >= fn['labels_in_text'] else 'FAIL'))
        zero = [n for n, h in rep['handlers'].items()
                if not h.get('cold') and h['ijumps'] == 0]
        out.append('  handlers with no own indirect jump (shared dispatch): %d%s' %
                   (len(zero), (' (' + ', '.join(sorted(zero)) + ')')
                    if 0 < len(zero) <= 30 else ''))
    out.append('')
    out.append('columns: frame = bytes of pushes + sub $N,%rsp in the first block;'
               ' rsp# = rsp ops in the handler;')
    out.append('  can = canary reference; ijmp = indirect jumps (cg: reached only'
               ' from this handler);')
    out.append('  path = fast path instructions; stk = stack references on it;'
               ' rr = reg-reg moves;')
    out.append('  adj = push/pop/sub/add rsp on it; calls = on path / anywhere')
    out.append(HEAD)
    for name in sorted(rep['handlers']):
        out.append(row(name, rep['handlers'][name], rep['mode']))
        if show_calls and rep['handlers'][name].get('calls'):
            out.append('    calls: ' + ', '.join(
                '%s x%d' % kv for kv in rep['handlers'][name]['calls'].items()))
    out.append('')
    t = rep['totals']
    out.append('TOTAL  handlers %d (no path %d)  frame bytes %d  rsp ops %d  '
               'canary handlers %d  ijumps %d' %
               (t['handlers'], t['no_path'], t['frame_bytes'], t['rsp_ops'],
                t['canaries'], t['ijumps']))
    out.append('       path insns %d  stack refs %d  reg moves %d  rsp adj %d  '
               'calls on path %d  calls anywhere %d' %
               (t['insns'], t['stack'], t['regmoves'], t['rsp_adj'], t['calls'],
                t['calls_total']))
    hot = rep.get('hot')
    if hot:
        out.append('')
        out.append('HOT SET (%d names, %d found%s)' %
                   (len(hot['names']), hot['found'],
                    '; missing: ' + ', '.join(hot['missing']) if hot['missing'] else ''))
        out.append(HEAD)
        for name in hot['names']:
            if name in rep['handlers']:
                out.append(row(name, rep['handlers'][name], rep['mode']))
        s = hot['sums']
        out.append('HOT TOTAL  static fast path %d insns  stack refs %d  reg moves %d'
                   '  rsp adj %d  calls on path %d' %
                   (s['insns'], s['stack'], s['regmoves'], s['rsp_adj'], s['calls']))
        out.append('           frame bytes %d  rsp ops %d  canary handlers %d  '
                   'ijumps %d  calls anywhere %d  no path %d' %
                   (s['frame_bytes'], s['rsp_ops'], s['canaries'], s['ijumps'],
                    s['calls_total'], s['no_path']))
    return '\n'.join(out)


def compare(a_path, b_path):
    a, b = json.load(open(a_path)), json.load(open(b_path))
    out = ['A: %s (%s)' % (a['object'], a['mode']),
           'B: %s (%s)' % (b['object'], b['mode']), '']
    keys = ['path.insns', 'path.stack', 'path.regmoves', 'path.rsp_adj',
            'path.calls', 'ijumps']

    def get(h, k):
        if h is None or h.get('cold'):
            return None
        if k.startswith('path.'):
            return h['path'][k[5:]] if h.get('path') else None
        return h[k]

    out.append('%-36s' % 'handler' + ''.join('%14s' % k for k in keys))
    ha, hb = a['handlers'], b['handlers']
    for name in sorted(set(ha) & set(hb)):
        cells = []
        changed = False
        for k in keys:
            x, y = get(ha[name], k), get(hb[name], k)
            if x is None or y is None:
                cells.append('%14s' % '-')
                continue
            changed |= x != y
            cells.append('%14s' % ('%d>%d(%+d)' % (x, y, y - x)))
        if changed:
            out.append('%-36s' % name + ''.join(cells))
    only_a, only_b = sorted(set(ha) - set(hb)), sorted(set(hb) - set(ha))
    if only_a:
        out.append('only in A: %d' % len(only_a))
    if only_b:
        out.append('only in B: %d' % len(only_b))
    out.append('')
    sets = [('totals', a['totals'], b['totals'])]
    if a.get('hot') and b.get('hot'):
        # Sum over hot handlers present in both reports, so a handler that
        # exists on one side only does not look like a change.
        common = {n for n in a['hot']['names'] if n in ha and n in hb}
        sets.append(('hot set (%d common handlers)' % len(common),
                     summarise(ha, common), summarise(hb, common)))
    for label, sa, sb in sets:
        out.append(label + ':')
        for k in sorted(sa):
            if k in sb and sa[k] != sb[k]:
                pct = ' (%+.1f%%)' % (100.0 * (sb[k] - sa[k]) / sa[k]) if sa[k] else ''
                out.append('  %-14s %8d > %8d  %+d%s' % (k, sa[k], sb[k], sb[k] - sa[k], pct))
    if a['mode'] == 'cg' and b['mode'] == 'cg':
        for k in ('canary_refs', 'ijumps', 'insns'):
            out.append('evaluator %s: %d > %d' % (k, a['function'][k], b['function'][k]))
        out.append('evaluator frame bytes: %d > %d' %
                   (a['function']['frame']['bytes'], b['function']['frame']['bytes']))
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('obj', nargs='?', help='ceval.o or linked executable')
    ap.add_argument('--mode', choices=['tc', 'cg'], default='tc')
    ap.add_argument('--prefix', default='_TAIL_CALL_',
                    help='tc: handler function prefix')
    ap.add_argument('--function', default='_PyEval_EvalFrameDefault',
                    help='cg: the evaluator function')
    ap.add_argument('--table-symbol',
                    default=r'opcode_targets_table(\.\d+)?|opcode_targets(\.\d+)?|targets(\.\d+)?',
                    help='cg: regex for the label-address table symbol')
    ap.add_argument('--names', help='cg: index->name source, '
                    'opcode-ids:FILE | macro:FILE | labels:FILE | ir-targets:FILE')
    ap.add_argument('--hot-kcat', nargs='*', default=DEFAULT_KCAT,
                    help='kernel symbol files that define the hot set')
    ap.add_argument('--hot-fraction', type=float, default=0.9)
    ap.add_argument('--hot', help='comma-separated hot set, instead of --hot-kcat')
    ap.add_argument('--no-hot', action='store_true')
    ap.add_argument('--calls', action='store_true', help='list call targets in text')
    ap.add_argument('--json', metavar='OUT', help='write the report as JSON')
    ap.add_argument('--probe-compat', action='store_true',
                    help="tc: reproduce probe/fastpath.py's counting quirks")
    ap.add_argument('--compare', nargs=2, metavar=('A', 'B'))
    ap.add_argument('--share', nargs='*', metavar='KCAT',
                    help='report handler vs helper share per kernel (default files)')
    args = ap.parse_args()

    if args.compare:
        print(compare(*args.compare))
        return
    if args.share is not None:
        text, _ = share_report(args.share or DEFAULT_KCAT, args.prefix)
        print(text)
        return
    if not args.obj:
        ap.error('OBJ required')

    funcs = disassemble(args.obj, args.probe_compat)
    rep = {'object': args.obj, 'mode': args.mode}
    if args.mode == 'tc':
        rep['handlers'] = analyse_tc(funcs, args.prefix, args.probe_compat)
    else:
        if not args.names:
            ap.error('--names required for --mode cg')
        rep['handlers'], rep['function'] = analyse_cg(
            funcs, args.obj, args.function, args.table_symbol, read_names(args.names))
    rep['totals'] = summarise(rep['handlers'])
    if not args.no_hot:
        names = (args.hot.split(',') if args.hot
                 else hot_set(args.hot_kcat, args.hot_fraction))
        have = [n for n in names if n in rep['handlers']]
        rep['hot'] = {'names': names, 'found': len(have),
                      'missing': [n for n in names if n not in rep['handlers']],
                      'sums': summarise(rep['handlers'], set(have))}
    print(format_text(rep, args.calls))
    if args.json:
        with open(args.json, 'w') as fh:
            json.dump(rep, fh, indent=1, sort_keys=True)


if __name__ == '__main__':
    main()
