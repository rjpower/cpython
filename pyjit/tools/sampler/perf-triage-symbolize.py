"""Symbolise sampler output: python3 symbolize.py <sampler.out.PID> [topN] [--callers]."""
import bisect, collections, subprocess, sys

def load(path):
    samples, maps = [], []
    with open(path) as f:
        depth = int(f.readline().split()[1])
        for line in f:
            if line.startswith("maps"):
                break
            samples.append([int(x, 16) for x in line.split()])
        for line in f:
            parts = line.split()
            if len(parts) >= 6 and "x" in parts[1]:
                lo, hi = (int(x, 16) for x in parts[0].split("-"))
                maps.append((lo, hi, int(parts[2], 16), parts[5]))
    return depth, samples, maps

_symcache = {}
def symtab(path):
    if path in _symcache:
        return _symcache[path]
    out = subprocess.run(["nm", "-n", "--defined-only", path], capture_output=True, text=True).stdout
    addrs, names = [], []
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "tTwW":
            addrs.append(int(p[0], 16)); names.append(p[2])
    if not addrs:
        out = subprocess.run(["nm", "-n", "-D", "--defined-only", path], capture_output=True, text=True).stdout
        for line in out.splitlines():
            p = line.split()
            if len(p) == 3 and p[1] in "tTwW":
                addrs.append(int(p[0], 16)); names.append(p[2])
    # load bias: first PT_LOAD vaddr
    _symcache[path] = (addrs, names)
    return addrs, names

def sym(pc, maps):
    for lo, hi, off, path in maps:
        if lo <= pc < hi:
            # Assume vaddr == file offset for PIE/shared objects (true for typical ld layouts).
            rel = pc - lo + off
            addrs, names = symtab(path)
            i = bisect.bisect_right(addrs, rel) - 1
            base = path.rsplit("/", 1)[-1]
            if i < 0:
                return f"{base}+{rel:#x}", rel
            # Non-PIE executable: absolute addresses
            return f"{names[i]}", rel
    return f"?{pc:#x}", pc

def main():
    path = sys.argv[1]
    top = int(sys.argv[2]) if len(sys.argv) > 2 else 15
    depth, samples, maps = load(path)
    total = len(samples)
    c = collections.Counter()
    callers = collections.defaultdict(collections.Counter)
    for s in samples:
        name, _ = sym(s[0], maps)
        c[name] += 1
        if depth > 1 and "--callers" in sys.argv:
            chain = " <- ".join(sym(x, maps)[0] for x in s[1:] if x)
            callers[name][chain] += 1
    print(f"total samples {total}")
    for name, k in c.most_common(top):
        print(f"{100*k/total:6.2f}% {k:7d} {name}")
        if callers.get(name):
            for ch, kk in callers[name].most_common(3):
                print(f"         {100*kk/k:5.1f}% <- {ch}")

if __name__ == "__main__":
    main()
