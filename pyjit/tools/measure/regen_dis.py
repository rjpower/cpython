"""Rewrite test_dis.py templates whose expected disassembly changed.
Run with the interpreter under test from the repository root:
    python /tmp/boil3b/regen_dis.py TESTNAME...  (unittest ids under test.test_dis)"""
import re, sys, unittest
import test.test_dis as td

pairs = []
orig = td.DisTestBase.do_disassembly_compare
def record(self, got, expected):
    if got != expected:
        got = self.strip_addresses(got)
    if got != expected:
        pairs.append((got, expected))
td.DisTestBase.do_disassembly_compare = record
suite = unittest.defaultTestLoader.loadTestsFromNames(sys.argv[1:], td)
unittest.TextTestRunner(verbosity=0).run(suite)

path = td.__file__
src = open(path).read()
num = re.compile(r'^( *)(\d+)(?= )')
ph = re.compile(r'^%-?\d*d')
done = set()
for got, expected in pairs:
    names = [k for k, v in vars(td).items() if isinstance(v, str) and v == expected]
    if not names:
        print('no template for expected starting', expected[:60].replace('\n', '|'))
        continue
    name = names[0]
    if name in done:
        continue
    m = re.search(r'^%s = """\\\n(.*?)"""' % re.escape(name), src, re.S | re.M)
    if not m:
        print('no literal for', name); continue
    tmpl = m.group(1).split('\n')
    holders = [ph.match(l).group(0) for l in tmpl if ph.match(l)]
    glines = got.split('\n')
    numbered = [i for i, l in enumerate(glines) if re.match(r'^ *\d+ ', l)
                and not re.match(r'^ +\d+ +[A-Z]', l[:0] + l) or re.match(r'^\s{0,2}\d+\s{2,}', l)]
    # A line-number field is the first token when it starts within 5 columns
    numbered = [i for i, l in enumerate(glines) if re.match(r'^ {0,4}\d+ ', l)]
    if len(numbered) != len(holders):
        print('line count mismatch for', name, len(numbered), len(holders)); continue
    for i, h in zip(numbered, holders):
        l = glines[i]
        n = re.match(r'^ {0,4}\d+', l).group(0)
        width = len(n)
        glines[i] = h + l[width:] if len(h) <= width else h + l[width:]
    new = '\n'.join(glines)
    # Template text keeps % escaped
    src = src[:m.start(1)] + new + src[m.end(1):]
    done.add(name)
    print('rewrote', name)
open(path, 'w').write(src)
