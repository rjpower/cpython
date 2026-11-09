"""Diagnostic source/entry census; this is never an elapsed-time measurement.

Native capture (fresh output paths, clean environment, JIT disabled):
  taskset -c 15 BUILD/python -S -B -X pystats_dir=OUTPUT/stats-1000 \
    capture-boundary-census.py capture --kernel KERNEL/logging.py \
    --loops 1000 --cpu 15 --stats-dir OUTPUT/stats-1000 --output OUTPUT/1000.json
Repeat at 0 and 2000 loops with separate stats/output paths. Release-only source
capture omits --stats-dir. Host summary:
  uv run --no-project python capture-boundary-census.py summarize \
    OUTPUT/0.json OUTPUT/1000.json OUTPUT/2000.json --output OUTPUT/summary.json
"""

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys


def write_json(path, value):
    with path.open("x") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")


def capture(args):
    import logging
    import os
    import _testinternalcapi as capi

    assert sorted(os.sched_getaffinity(0)) == [args.cpu]
    assert not sys._jit.is_enabled()
    assert "jit_method_natural" not in vars(capi)
    assert sys.gettrace() is None and sys.getprofile() is None
    assert args.loops >= 0 and args.warmup > 0
    spec = importlib.util.spec_from_file_location("_boundary_logging", args.kernel)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    module.setup("logging_silent")
    module.run(args.warmup)
    functions = []
    for func in (module.bench_silent, logging.Logger.debug, logging.Logger.isEnabledFor):
        canonical = capi.get_ir(func.__code__)
        physical = capi.get_ir(func.__code__, True)
        assert len(canonical) == len(physical)
        functions.append({
            "name": func.__qualname__,
            "file": func.__code__.co_filename,
            "first_line": func.__code__.co_firstlineno,
            "canonical_words": canonical,
            "physical_words": physical,
            "canonical_counts": dict(Counter(word[0] for word in canonical)),
            "physical_counts": dict(Counter(word[0] for word in physical)),
            "selected_variants": [
                {"offset": i, "handler": word[0], "fields": word[1:]}
                for i, word in enumerate(physical)
                if ("_SKIP_" in word[0] or word[0].endswith("_DISCARD")
                    or word[0] == "LOAD_ATTR_SHAPE_LOCAL_TEMP")
            ],
        })
    result = {
        "git": sys._git,
        "executable": sys.executable,
        "affinity": sorted(os.sched_getaffinity(0)),
        "jit_enabled": sys._jit.is_enabled(),
        "kernel": str(args.kernel.resolve()),
        "kernel_sha256": hashlib.sha256(args.kernel.read_bytes()).hexdigest(),
        "loops": args.loops,
        "warmup_loops": args.warmup,
        "functions": functions,
        "accounting": (
            "Canonical words are retained logical/storage positions. Physical "
            "words prove warmed selection, not site hit counts. Existing entered "
            "handler counters charge execution aliases to canonical specialization "
            "names. A baseline inline NOP continuation already avoids NOP entry; "
            "its removal must not be credited as another saved dispatch."
        ),
    }
    if args.stats_dir is None:
        module.run(args.loops)
    else:
        stats_dir = args.stats_dir.resolve()
        assert str(sys._xoptions["pystats_dir"]) == str(stats_dir)
        stats_dir.mkdir(parents=True, exist_ok=True)
        before = set(stats_dir.glob("*.txt"))
        sys._stats_clear()
        sys._stats_on()
        module.run(args.loops)
        dumped = sys._stats_dump()
        sys._stats_off()
        sys._stats_clear()
        assert dumped
        new = set(stats_dir.glob("*.txt")) - before
        assert len(new) == 1, sorted(map(str, new))
        result["stats_file"] = str(new.pop())
    assert module._stream.getvalue() == ""
    write_json(args.output, result)


def read_stats(path):
    counters = {}
    pattern = re.compile(r"^\s*(.+?)\s*:\s*(\d+)\s*$")
    for line in path.read_text().splitlines():
        match = pattern.match(line)
        if match:
            counters[match[1]] = int(match[2])
    return counters


def summarize(args):
    samples = [json.loads(path.read_text()) for path in args.samples]
    samples.sort(key=lambda item: item["loops"])
    assert len(samples) == 3
    empty, first, second = samples
    assert empty["loops"] == 0 and first["loops"] > 0
    assert second["loops"] == 2 * first["loops"]
    assert len({item["kernel_sha256"] for item in samples}) == 1
    assert len({tuple(item["git"]) for item in samples}) == 1
    raw = [read_stats(Path(item["stats_file"])) for item in samples]
    keys = set().union(*(set(row) for row in raw))
    entered = sorted(key for key in keys if key.endswith(".execution_count"))
    frame_keys = ("Calls to Python functions inlined", "Frames pushed")
    rows = []
    for key in (*entered, *frame_keys):
        values = [row.get(key, 0) for row in raw]
        one = values[1] - values[0]
        two = values[2] - values[0]
        if one or two:
            rows.append({"counter": key, "empty": values[0],
                         "per_loop": one / first["loops"],
                         "linear_2x": two == 2 * one})
    totals = [sum(row.get(key, 0) for key in entered) for row in raw]
    count = totals[1] - totals[0]
    write_json(args.output, {
        "git": first["git"],
        "loops": [sample["loops"] for sample in samples],
        "entered_handlers_per_loop": count / first["loops"],
        "entered_handlers_linear_2x": totals[2] - totals[0] == 2 * count,
        "counts": rows,
        "static_and_selected_shapes": first["functions"],
        "accounting": first["accounting"],
        "limits": "No site-specific hit counters and no elapsed-speedup claim.",
    })


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    native = commands.add_parser("capture")
    native.add_argument("--kernel", required=True, type=Path)
    native.add_argument("--loops", required=True, type=int)
    native.add_argument("--warmup", default=20000, type=int)
    native.add_argument("--cpu", default=15, type=int)
    native.add_argument("--stats-dir", type=Path)
    native.add_argument("--output", required=True, type=Path)
    host = commands.add_parser("summarize")
    host.add_argument("samples", nargs=3, type=Path)
    host.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.command == "capture":
        capture(args)
    else:
        summarize(args)


if __name__ == "__main__":
    main()
