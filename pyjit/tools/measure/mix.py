"""Summarise a raw Py_STATS file: total dispatches, the top opcodes, and
the successors of chosen opcodes.  Usage: mix.py FILE [OPNAME...]"""
import re, sys, collections
exe = collections.Counter(); pairs = collections.defaultdict(collections.Counter)
for line in open(sys.argv[1]):
    m = re.match(r'\s*opcode\[(\w+)\]\.execution_count : (\d+)', line)
    if m: exe[m[1]] += int(m[2]); continue
    m = re.match(r'\s*opcode\[(\w+)\]\.pair_count\[(\w+)\] : (\d+)', line)
    if m: pairs[m[1]][m[2]] += int(m[3])
tot = sum(exe.values())
print(f'total {tot/1e6:.2f}M')
for k, v in exe.most_common(25):
    print(f'  {k:32} {v/1e6:7.3f}M {100*v/tot:5.1f}%')
for name in sys.argv[2:]:
    print(name, 'followed by:')
    for k, v in pairs[name].most_common(8):
        print(f'    {k:30} {v/1e6:7.3f}M')
