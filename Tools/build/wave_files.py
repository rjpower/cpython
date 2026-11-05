#!/usr/bin/env python3
"""Pack the files of a compiler-driven wave into batches for parallel workers.

A wave is a mechanical change whose call sites the compiler reports. Each
worker takes one batch of files, so the batches should hold about the same
number of call sites, and a file must never be split between workers.

Input is a TSV of sites with a header line. The columns are, tab-separated,
file, line, col, function, class, family and note. Only "file" is needed for
packing. "family" (for example list, tuple, dict, set, cell) and "class" are
optional strings used for grouping. Lines starting with "#" are comments.

Usage:

    wave_files.py --batches N [--group {dir,family,none}] [--never FILE]...
                  [--last FILE]... [--no-default-never] [--slack F]
                  [--tsv] [--summary] SITES.tsv

Files in NEVER_BATCHED (the interpreter unit, deferred in the wave protocol)
and files named by --never are reported as excluded and never batched.
Files named by --last go together into the final batch, and the other files
are packed into N-1 batches.

Packing is greedy longest-processing-time: files sorted by site count,
largest first (ties by path), each go to the batch with the smallest total.
Grouping is a preference. Among the batches whose total is within --slack
(a fraction of the ideal per-batch total) of the smallest, the batch holding
the most files of the file's group is chosen. The group of a file is its
directory (dir), its majority family (family) or itself (none).

Output is JSON on stdout:

    {"total": int, "excluded": [file, ...],
     "batches": [{"index": 1, "calls": int,
                  "files": [{"file": str, "calls": int}, ...],
                  "groups": {group: calls}}, ...]}

With --tsv the output is "batch", "file" and "calls" columns instead. With
--summary one line per batch goes to stderr in either mode. The output is
deterministic for a given input.
"""

import argparse
import collections
import csv
import json
import os
import sys

NEVER_BATCHED = (
    "Python/ceval.c",
    "Python/bytecodes.c",
    "Python/ceval_macros.h",
)
NEVER_BATCHED_PREFIX = "generated_"

GROUP_MODES = ("dir", "family", "none")


def read_sites(path_or_lines):
    """Parse a sites TSV into a list of dicts keyed by the header columns.

    Accepts a path or an iterable of lines. The first non-comment line is the
    header. Blank lines and lines starting with "#" are skipped. Missing
    trailing columns read as empty strings.
    """
    if isinstance(path_or_lines, (str, os.PathLike)):
        with open(path_or_lines, encoding="utf-8", newline="") as f:
            return read_sites(f.read().splitlines())
    lines = [
        line.rstrip("\r\n")
        for line in path_or_lines
        if line.strip() and not line.startswith("#")
    ]
    if not lines:
        return []
    header = lines[0].split("\t")
    if "file" not in header:
        raise SystemExit("wave_files: the sites header has no 'file' column")
    sites = []
    for row in csv.reader(lines[1:], delimiter="\t", quoting=csv.QUOTE_NONE):
        site = dict.fromkeys(header, "")
        site.update(zip(header, row))
        sites.append(site)
    return sites


def count_by_file(sites):
    """Return {file: number of sites}, in first-seen order."""
    counts = collections.Counter(site["file"] for site in sites)
    return dict(counts)


def group_key(file, sites, mode):
    """Return the group of FILE under MODE ("dir", "family" or "none").

    For "family" the group is the file's most common family among SITES, ties
    broken alphabetically, or "" when the file has no family.
    """
    if mode == "dir":
        return os.path.dirname(file) or "."
    if mode == "none":
        return file
    if mode == "family":
        families = collections.Counter(
            site.get("family", "") for site in sites if site["file"] == file
        )
        if not families:
            return ""
        return min(families, key=lambda fam: (-families[fam], fam))
    raise ValueError(f"unknown group mode {mode!r}")


def is_generated(file):
    return os.path.basename(file).startswith(NEVER_BATCHED_PREFIX)


def pack(counts, n, groups=None, last=(), slack=0.15):
    """Pack the files of COUNTS into N batches of similar total.

    Returns a list of batches, each a list of file paths. When LAST names
    files, they form the final batch and the other files fill N-1 batches.
    GROUPS maps a file to its group; it only breaks near-ties (see the module
    docstring). A file is never split.
    """
    last = list(last)
    if n < 1:
        raise SystemExit("wave_files: --batches must be at least 1")
    if last and n < 2:
        raise SystemExit("wave_files: --last needs at least 2 batches")
    groups = groups or {}
    reserved = set(last)
    regular = n - 1 if last else n
    files = sorted(
        (f for f in counts if f not in reserved),
        key=lambda f: (-counts[f], f),
    )
    ideal = sum(counts[f] for f in files) / regular
    batches = [[] for _ in range(regular)]
    totals = [0] * regular
    members = [collections.Counter() for _ in range(regular)]
    for file in files:
        low = min(totals)
        near = [
            i for i in range(regular) if totals[i] <= low + slack * ideal
        ]
        group = groups.get(file, file)
        # Most files of the same group first, then the smallest total.
        best = min(near, key=lambda i: (-members[i][group], totals[i], i))
        batches[best].append(file)
        totals[best] += counts[file]
        members[best][group] += 1
    if last:
        batches.append(sorted(last))
    return batches


def build_report(counts, batches, excluded, groups):
    report_batches = []
    for index, files in enumerate(batches, 1):
        group_calls = collections.Counter()
        for file in files:
            group_calls[groups.get(file, file)] += counts[file]
        report_batches.append({
            "index": index,
            "calls": sum(counts[f] for f in files),
            "files": [{"file": f, "calls": counts[f]} for f in files],
            "groups": dict(
                sorted(group_calls.items(), key=lambda kv: (-kv[1], kv[0]))
            ),
        })
    return {
        "total": sum(counts.values()),
        "excluded": sorted(excluded),
        "batches": report_batches,
    }


def summary_lines(report):
    for batch in report["batches"]:
        groups = ", ".join(f"{g} ({c})" for g, c in batch["groups"].items())
        yield (
            f"batch {batch['index']}: {batch['calls']} calls, "
            f"{len(batch['files'])} files: {groups}"
        )


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Pack the files of a wave into batches of similar size."
    )
    parser.add_argument("sites", help="TSV of sites with a header line")
    parser.add_argument("--batches", type=int, required=True, metavar="N")
    parser.add_argument("--group", choices=GROUP_MODES, default="dir")
    parser.add_argument("--never", action="append", default=[], metavar="FILE",
                        help="file that is never batched (repeatable)")
    parser.add_argument("--no-default-never", action="store_true",
                        help="do not exclude the NEVER_BATCHED files")
    parser.add_argument("--last", action="append", default=[], metavar="FILE",
                        help="file for the reserved final batch (repeatable)")
    parser.add_argument("--slack", type=float, default=0.15,
                        help="near-tie width as a fraction of the ideal "
                             "batch total (default 0.15)")
    parser.add_argument("--tsv", action="store_true",
                        help="print batch, file and calls columns, not JSON")
    parser.add_argument("--summary", action="store_true",
                        help="print one line per batch to stderr")
    args = parser.parse_args(argv)

    if args.batches < 1:
        raise SystemExit("wave_files: --batches must be at least 1")
    if args.last and args.batches < 2:
        raise SystemExit("wave_files: --last needs at least 2 batches")

    sites = read_sites(args.sites)
    counts = count_by_file(sites)
    never = set(args.never)
    if not args.no_default_never:
        never.update(NEVER_BATCHED)
    excluded = [
        f for f in counts
        if f in never or (not args.no_default_never and is_generated(f))
    ]
    counts_kept = {f: c for f, c in counts.items() if f not in excluded}
    missing = [f for f in args.last if f not in counts_kept]
    if missing:
        raise SystemExit(
            "wave_files: --last files not among batchable files: "
            + ", ".join(missing)
        )
    groups = {f: group_key(f, sites, args.group) for f in counts_kept}
    batches = pack(counts_kept, args.batches, groups, args.last, args.slack)
    report = build_report(counts_kept, batches, excluded, groups)

    if args.tsv:
        for batch in report["batches"]:
            for item in batch["files"]:
                print(f"{batch['index']}\t{item['file']}\t{item['calls']}")
    else:
        json.dump(report, sys.stdout, indent=2)
        print()
    if args.summary:
        for line in summary_lines(report):
            print(line, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
