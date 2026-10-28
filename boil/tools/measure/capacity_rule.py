"""Block bytes of inline-slot objects under today's capacity and 6d's rule.

Usage: python capacity_rule.py [--spare N] OCC.json...
OCC.json is occupancy_types.py's output for one kernel, named after it.
Prints the block bytes summed over the samples under today's capacity and
under the rule with w = S + N (N is 2, 6d's rule, by default), with and
without generators' tree, and the sampled objects the rule would put over
capacity, which would go to dictionary mode.  The sums are time integrals,
so their ratio is what counts.
"""
import argparse, json

MAX_SLOTS = 30


def good(n):
    """mi_good_size(n) with MI_ALIGN2W: 16-byte steps to 64 bytes, then four
    size classes per power of two."""
    if n <= 64:
        return (n + 15) & ~15
    step = 1 << ((n - 1).bit_length() - 3)
    return (n + step - 1) // step * step


def capacity_6d(basicsize, s, spare=2):
    w = s + spare if s > 0 else 6
    return min(MAX_SLOTS, (good(basicsize + 8 * w) - basicsize) // 8)


def main(paths, spare):
    today = new = today_ng = new_ng = 0
    over = []
    for path in paths:
        kernel = path.rsplit("/", 1)[-1].removesuffix(".json")
        for row in json.load(open(path)):
            b, s, cap = row["basicsize"], row["S"], row["cap"]
            cap6d = capacity_6d(b, s, spare)
            for nslots, count in row["hist"].items():
                t, n = count * good(b + 8 * cap), count * good(b + 8 * cap6d)
                today += t; new += n
                if row["type"] != "kernel.Tree" or kernel != "generators":
                    today_ng += t; new_ng += n
                if int(nslots) > cap6d:
                    over.append((kernel, row["type"], s, int(nslots), count))
    print(f"all: {today // 1024:,} KiB today, {new // 1024:,} KiB 6d "
          f"({(new - today) / today:+.1%})")
    print(f"without generators' Tree: {today_ng // 1024:,} KiB today, "
          f"{new_ng // 1024:,} KiB 6d ({(new_ng - today_ng) / today_ng:+.1%})")
    print(f"objects over the 6d capacity: {sum(o[-1] for o in over)}")
    for o in over:
        print("  ", *o)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--spare", type=int, default=2)
    parser.add_argument("paths", nargs="+")
    args = parser.parse_args()
    main(args.paths, args.spare)
