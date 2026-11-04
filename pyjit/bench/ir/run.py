"""Run the register-IR measurements of pyjit/design/register-ir.md §2.3–§2.5.

    uv run pyjit/bench/ir/run.py --ir-python IR/python --upstream-python UP/python \\
        [--corpus DIR ...] [--generated DIR] [--kernels NAME ...]

The workers run under the build interpreters, not under uv.  The IR build's
Lib cannot disassemble its own words unless dis.py, opcode.py and
_opcode_metadata.py come from the same commit, so this script extracts them
(and pycore_opcode_metadata.h, for window extents) with `git show
COMMIT:path` into a temporary directory and puts it on the IR worker's
PYTHONPATH only.

--corpus DIR: static counts over every .py below DIR, with both builds.
--generated DIR: static counts, IR build only, over every *_pb2.py and every
  .py larger than 200 KB below DIR, deduplicated by content and taken in hash
  order, stopping after --time-limit seconds.
--kernels: dynamic counts per kernel in pyjit/bench/internal/kernels.
Prints markdown tables and writes the raw JSON next to them with --out.
"""

import argparse, hashlib, json, os, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
KERNELS = HERE.parent / "internal" / "kernels"
FILES = ("Lib/dis.py", "Lib/opcode.py", "Lib/_opcode_metadata.py",
         "Include/internal/pycore_opcode_metadata.h")
DEFAULT_KERNELS = ("fannkuch raytrace go nbody spectral_norm nqueens float chaos "
                   "richards generators coroutines deltablue unpack_sequence comprehensions").split()


def worker(python, script, args, env):
    out = subprocess.run([python, "-s", "-W", "ignore", str(HERE / script), *args],
                         env=env, capture_output=True, text=True, check=True)
    return json.loads(out.stdout)


def generated_files(root, listing):
    seen = {}
    for dp, dn, fn in os.walk(root):
        for f in fn:
            p = os.path.join(dp, f)
            if f.endswith("_pb2.py") or (f.endswith(".py") and os.path.getsize(p) > 200_000):
                seen.setdefault(hashlib.sha1(Path(p).read_bytes()).hexdigest(), p)
    listing.write_text("\n".join(p for h, p in sorted(seen.items())) + "\n")
    return len(seen)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--ir-python", required=True)
    ap.add_argument("--upstream-python", required=True)
    ap.add_argument("--repo", default="/home/power/code/cpython")
    ap.add_argument("--commit", default="eba351fee7d", help="commit of the IR build")
    ap.add_argument("--corpus", action="append", default=[])
    ap.add_argument("--generated")
    ap.add_argument("--time-limit", default="420")
    ap.add_argument("--kernels", nargs="*", default=None)
    ap.add_argument("--out")
    a = ap.parse_args()
    results = {}
    with tempfile.TemporaryDirectory() as tmp:
        for f in FILES:
            blob = subprocess.run(["git", "-C", a.repo, "show", f"{a.commit}:{f}"],
                                  capture_output=True, check=True).stdout
            (Path(tmp) / Path(f).name).write_bytes(blob)
        ir_env = dict(os.environ, PYTHONPATH=tmp)
        up_env = {k: v for k, v in os.environ.items() if k != "PYTHONPATH"}
        for corpus in a.corpus:
            results["static " + corpus] = {
                "ir": worker(a.ir_python, "static_counts.py", [corpus], ir_env),
                "upstream": worker(a.upstream_python, "static_counts.py", [corpus], up_env)}
        if a.generated:
            listing = Path(tmp) / "generated.txt"
            n = generated_files(a.generated, listing)
            r = worker(a.ir_python, "static_counts.py",
                       ["--time-limit", a.time_limit, "@" + str(listing)], ir_env)
            r["unique_files"] = n
            results["generated " + a.generated] = {"ir": r}
        meta = str(Path(tmp) / "pycore_opcode_metadata.h")
        for k in (DEFAULT_KERNELS if a.kernels == [] else a.kernels or ()):
            path = str(KERNELS / f"{k}.py")
            results["kernel " + k] = {
                "ir": worker(a.ir_python, "dynamic_counts.py", [path, meta], ir_env),
                "upstream": worker(a.upstream_python, "dynamic_counts.py", [path], up_env)}
    if a.out:
        Path(a.out).write_text(json.dumps(results, indent=1))
    report(results)


def pct(x, total):
    return f"{100 * x / total:.1f}%" if total else "–"


def report(results):
    rows = [(k[7:], v) for k, v in results.items() if k.startswith("kernel ")]
    if rows:
        print("| Kernel | Upstream | IR | IR / upstream | Constant loads | of which scalars"
              " | Fusable compare → branch | CALL → POP_TOP | PUSH_NULL | STORE_FAST"
              " | NOT_TAKEN (excluded) | Reads / word | Writes / word | NULL stores / word |")
        print("|" + "---|" * 14)
        for name, v in rows:
            i, u = v["ir"]["counts"], v["upstream"]["counts"]
            t = i["total"]
            print(f"| {name} | {u['total']:,} | {t:,} | {t / u['total']:.2f} "
                  f"| {pct(i.get('const_loads', 0), t)} | {pct(i.get('const_scalar', 0), t)} "
                  f"| {pct(i.get('fusable_compare_branch', 0), t)} | {pct(i.get('call_pop_top', 0), t)} "
                  f"| {pct(i['op_PUSH_NULL'], t)} | {pct(i['op_STORE_FAST'], t)} "
                  f"| {pct(i.get('not_taken', 0), t)} | {i['slot_reads'] / t:.2f} "
                  f"| {i['slot_writes'] / t:.2f} | {i['null_stores'] / t:.2f} |")
    for key, v in results.items():
        if not key.startswith(("static ", "generated ")):
            continue
        r = v["ir"]
        print(f"\n{key}: {json.dumps(r['counts'], sort_keys=True)}")
        print("limit failures:", r["limit_failures"])
        print("| Per code object | max | p50 | p99 | p99.9 | p99.99 | > 255 | > 2,047 | > 4,095 |")
        print("|" + "---|" * 9)
        for m, s in sorted(r["per_code_object"].items()):
            print(f"| {m} | {s['max']:,} | {s['p50']} | {s['p99']} | {s['p99.9']} | {s['p99.99']} "
                  f"| {s['over255']} | {s['over2047']} | {s['over4095']} |")
        if "upstream" in v:
            print("upstream:", json.dumps(v["upstream"]["counts"], sort_keys=True))


if __name__ == "__main__":
    sys.exit(main())
