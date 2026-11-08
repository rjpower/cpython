#!/usr/bin/env python3
"""Attribute depth-one diagnostic samples to retained executable symbols."""

from __future__ import annotations

import argparse
import bisect
import collections
import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path


CASES = ("deltablue", "logging_silent", "comprehensions")
ROLES = ("upstream", "candidate")


@dataclass(frozen=True)
class MapRow:
    start: int
    end: int
    perms: str
    offset: int
    path: str


@dataclass(frozen=True)
class Symbol:
    start: int
    end: int
    names: tuple[str, ...]


def command_output(argv: list[str]) -> str:
    return subprocess.run(argv, check=True, capture_output=True, text=True).stdout


def read_capture(path: Path) -> tuple[list[int], list[MapRow]]:
    samples: list[int] = []
    maps: list[MapRow] = []
    section = "samples"
    with path.open(encoding="ascii") as stream:
        header = stream.readline().split()
        if header != ["depth", "1"]:
            raise ValueError(f"{path}: expected depth 1 header, got {header!r}")
        for raw in stream:
            line = raw.strip()
            if line == "maps":
                section = "maps"
                continue
            if not line:
                continue
            if section == "samples":
                columns = line.split()
                if len(columns) != 1:
                    raise ValueError(f"{path}: malformed depth-one sample {line!r}")
                samples.append(int(columns[0], 16))
                continue
            columns = line.split(None, 5)
            if len(columns) < 5:
                continue
            span, perms, offset, _device, _inode = columns[:5]
            mapped_path = columns[5] if len(columns) == 6 else ""
            if not mapped_path or mapped_path.startswith("["):
                continue
            lo, hi = (int(value, 16) for value in span.split("-", 1))
            maps.append(MapRow(lo, hi, perms, int(offset, 16), mapped_path))
    if section != "maps":
        raise ValueError(f"{path}: missing maps section")
    return samples, maps


def command_executable(path: Path) -> str:
    data = json.loads(path.read_text(encoding="utf-8"))
    command = data.get("command")
    if not isinstance(command, list) or not command or not isinstance(command[0], str):
        raise ValueError(f"{path}: command.json has no string command[0]")
    return command[0]


def build_id(path: Path) -> str:
    output = command_output(["readelf", "-n", str(path)])
    match = re.search(r"Build ID:\s*([0-9a-fA-F]+)", output)
    if match is None:
        raise ValueError(f"{path}: retained ELF has no GNU build ID")
    return match.group(1).lower()


def noted_build_id(path: Path) -> str:
    output = path.read_text(encoding="utf-8")
    match = re.search(r"Build ID:\s*([0-9a-fA-F]+)", output)
    if match is None:
        raise ValueError(f"{path}: saved ELF notes have no build ID")
    return match.group(1).lower()


def executable_loads(path: Path) -> list[tuple[int, int, int]]:
    output = command_output(["readelf", "-lW", str(path)])
    loads: list[tuple[int, int, int]] = []
    for line in output.splitlines():
        match = re.match(
            r"\s*LOAD\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+\S+"
            r"\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+([RWE]+(?:\s+[RWE]+)?)\s+0x",
            line,
        )
        if match is None or "E" not in match.group(5):
            continue
        offset, vaddr, filesz = (int(match.group(i), 16) for i in (1, 2, 3))
        if offset != vaddr:
            raise ValueError(
                f"{path}: executable PT_LOAD offset {offset:#x} != vaddr {vaddr:#x}"
            )
        loads.append((offset, vaddr, filesz))
    if not loads:
        raise ValueError(f"{path}: no executable PT_LOAD segments")
    return loads


def load_symbols(path: Path) -> list[Symbol]:
    output = command_output(["nm", "-n", "-S", "--defined-only", str(path)])
    ranges: dict[tuple[int, int], set[str]] = collections.defaultdict(set)
    for line in output.splitlines():
        columns = line.split(None, 3)
        if len(columns) != 4 or columns[2] not in "tTwW":
            continue
        try:
            start, size = int(columns[0], 16), int(columns[1], 16)
        except ValueError:
            continue
        if size > 0:
            ranges[(start, start + size)].add(columns[3])
    return [
        Symbol(start, end, tuple(sorted(names)))
        for (start, end), names in sorted(ranges.items())
    ]


class SymbolIndex:
    def __init__(self, symbols: list[Symbol]):
        self.symbols = symbols
        self.starts = [symbol.start for symbol in symbols]
        max_end = 0
        self.prefix_max_end = []
        for symbol in symbols:
            max_end = max(max_end, symbol.end)
            self.prefix_max_end.append(max_end)

    def find(self, address: int) -> tuple[str, list[str]]:
        index = bisect.bisect_right(self.starts, address) - 1
        matches = []
        while index >= 0 and self.prefix_max_end[index] > address:
            symbol = self.symbols[index]
            if symbol.start <= address < symbol.end:
                matches.append(symbol)
            index -= 1
        if not matches:
            return "unresolved-no-sized-symbol", []
        ranges = {(symbol.start, symbol.end) for symbol in matches}
        if len(ranges) != 1:
            return "unresolved-overlapping-symbols", []
        symbol = matches[0]
        return f"{symbol.start:#x}-{symbol.end:#x}", list(symbol.names)


def addr2line(path: Path, addresses: list[int]) -> dict[int, list[dict[str, str]]]:
    if not addresses:
        return {}
    argv = ["addr2line", "-a", "-f", "-i", "-e", str(path)]
    argv.extend(hex(address) for address in addresses)
    lines = command_output(argv).splitlines()
    result: dict[int, list[dict[str, str]]] = {}
    i = 0
    while i < len(lines):
        try:
            address = int(lines[i], 16)
        except ValueError as exc:
            raise ValueError(f"addr2line: expected address line, got {lines[i]!r}") from exc
        i += 1
        chain = []
        while i + 1 < len(lines) and not lines[i].startswith("0x"):
            chain.append({"function": lines[i], "location": lines[i + 1]})
            i += 2
        result[address] = chain
    if set(result) != set(addresses):
        raise ValueError("addr2line returned a different address set")
    return result


def case_files(directory: Path, role: str, case: str) -> tuple[Path, Path]:
    command_path = directory / f"{role}-{case}-command.json"
    captures = sorted(directory.glob(f"{role}-{case}.*"))
    captures = [p for p in captures if p.is_file() and p.name.rsplit(".", 1)[-1].isdigit()]
    if not command_path.is_file() or len(captures) != 1:
        raise ValueError(
            f"{role}/{case}: expected one PID capture and command.json, got {captures}"
        )
    return command_path, captures[0]


def main_map_for_pc(pc: int, maps: list[MapRow], executable: str) -> MapRow | None:
    for row in maps:
        if row.start <= pc < row.end and row.path == executable:
            return row
    return None


def map_pc(
    pc: int, maps: list[MapRow], executable: str, loads: list[tuple[int, int, int]]
) -> tuple[str, int | None, str]:
    row = main_map_for_pc(pc, maps, executable)
    if row is None:
        for mapped in maps:
            if mapped.start <= pc < mapped.end:
                return "external", mapped.offset + pc - mapped.start, mapped.path
        return "unmapped", None, ""
    if "x" not in row.perms:
        return "main-non-executable-map", None, row.path
    file_offset = row.offset + pc - row.start
    if not any(start <= file_offset < start + size for start, _vaddr, size in loads):
        return "main-outside-executable-load", file_offset, row.path
    return "main", file_offset, row.path


def analyze_case(
    role: str,
    case: str,
    samples: list[int],
    maps: list[MapRow],
    executable: str,
    loads: list[tuple[int, int, int]],
    symbols: SymbolIndex,
    locations: dict[int, list[dict[str, str]]],
) -> dict:
    pcs = collections.Counter(samples)
    functions: collections.Counter[str] = collections.Counter()
    sources: collections.Counter[str] = collections.Counter()
    externals: collections.Counter[tuple[str, int]] = collections.Counter()
    unresolved: collections.Counter[tuple[str, str, int | None]] = collections.Counter()
    pc_rows = []
    for pc, count in sorted(pcs.items(), key=lambda item: (-item[1], item[0])):
        kind, offset, mapped_path = map_pc(pc, maps, executable, loads)
        row = {"pc": hex(pc), "count": count}
        if kind == "main" and offset is not None:
            symbol_key, names = symbols.find(offset)
            chain = locations.get(offset, [])
            row.update({
                "file_offset": hex(offset),
                "symbol_range": symbol_key if names else None,
                "symbol_aliases": names,
                "inline_chain": chain,
            })
            if names:
                function = " | ".join(names)
                functions[function] += count
                source = chain[0]["location"] if chain else "??:0"
                sources[source] += count
            else:
                unresolved[(symbol_key, mapped_path, offset)] += count
                row["unresolved"] = symbol_key
        elif kind == "external" and offset is not None:
            externals[(mapped_path, offset)] += count
            row.update({"mapped_path": mapped_path, "file_offset": hex(offset)})
        else:
            unresolved[(kind, mapped_path, offset)] += count
            row.update({"mapped_path": mapped_path, "unresolved": kind})
            if offset is not None:
                row["file_offset"] = hex(offset)
        pc_rows.append(row)
    attributed = sum(functions.values()) + sum(externals.values()) + sum(unresolved.values())
    if attributed != len(samples):
        raise AssertionError(f"{role}/{case}: {attributed} attributed != {len(samples)} samples")
    if sum(sources.values()) != sum(functions.values()):
        raise AssertionError(f"{role}/{case}: source rows do not conserve uniquely symbolized samples")
    return {
        "role": role,
        "case": case,
        "total_samples": len(samples),
        "unique_pcs": len(pcs),
        "function_sample_count": sum(functions.values()),
        "external_sample_count": sum(externals.values()),
        "unresolved_sample_count": sum(unresolved.values()),
        "top_functions": [
            {"function_or_aliases": key, "count": value}
            for key, value in functions.most_common(30)
        ],
        "top_innermost_source_lines": [
            {"location": key, "count": value} for key, value in sources.most_common(30)
        ],
        "all_innermost_source_lines": [
            {"location": key, "count": value} for key, value in sources.most_common()
        ],
        "top_external_path_offsets": [
            {"path": key[0], "file_offset": hex(key[1]), "count": value}
            for key, value in externals.most_common(20)
        ],
        "unresolved": [
            {"reason": key[0], "path": key[1], "file_offset": hex(key[2]) if key[2] is not None else None, "count": value}
            for key, value in unresolved.most_common(20)
        ],
        "top_pcs": pc_rows[:50],
        "all_pcs": pc_rows,
    }


def render_markdown(result: dict) -> str:
    lines = [
        "# Leaf-PC analysis",
        "",
        "Counts use the original depth-one sample denominator per profile. Each sample is attributed once to a sized main-image symbol, an external path+offset, or an unresolved bucket.",
        "`addr2line -i` chains are compile-time inline metadata, not caller stacks; only the first (innermost) source location is included in source-line totals.",
        "",
    ]
    for case in result["cases"]:
        total = case["total_samples"]
        lines += [
            f"## {case['role']} / {case['case']}",
            "",
            f"Samples: {total}; unique PCs: {case['unique_pcs']}; symbolized: {case['function_sample_count']}; external: {case['external_sample_count']}; unresolved: {case['unresolved_sample_count']}.",
            "",
            "### Functions",
            "",
            "| Count | Share | Function / same-range aliases |",
            "| ---: | ---: | --- |",
        ]
        for row in case["top_functions"]:
            lines.append(f"| {row['count']} | {100 * row['count'] / total:.2f}% | `{row['function_or_aliases']}` |")
        lines += ["", "### Innermost source lines", "", "| Count | Share | Location |", "| ---: | ---: | --- |"]
        for row in case["top_innermost_source_lines"]:
            lines.append(f"| {row['count']} | {100 * row['count'] / total:.2f}% | `{row['location']}` |")
        lines += ["", "### PCs", "", "| Count | Share | PC | File offset | Symbol / disposition |", "| ---: | ---: | ---: | ---: | --- |"]
        for row in case["top_pcs"]:
            name = " | ".join(row.get("symbol_aliases", [])) or row.get("unresolved", row.get("mapped_path", ""))
            lines.append(f"| {row['count']} | {100 * row['count'] / total:.2f}% | `{row['pc']}` | `{row.get('file_offset', '')}` | `{name}` |")
        lines += ["", "### External path+offset buckets", "", "| Count | Path | File offset |", "| ---: | --- | ---: |"]
        for row in case["top_external_path_offsets"]:
            lines.append(f"| {row['count']} | `{row['path']}` | `{row['file_offset']}` |")
        lines.append("")
    return "\n".join(lines)


def run(profiles: Path, output: Path) -> None:
    if output.exists():
        raise FileExistsError(f"refusing to overwrite output directory: {output}")
    role_data = {}
    case_inputs = {}
    for role in ROLES:
        retained = profiles / f"{role}-python"
        notes = profiles / f"{role}-elf-notes.txt"
        actual_id = build_id(retained)
        expected_id = noted_build_id(notes)
        if actual_id != expected_id:
            raise ValueError(f"{role}: retained ELF build ID {actual_id} != saved {expected_id}")
        loads = executable_loads(retained)
        symbols = SymbolIndex(load_symbols(retained))
        role_data[role] = {"elf": retained, "build_id": actual_id, "loads": loads, "symbols": symbols}
        for case in CASES:
            command_path, capture = case_files(profiles, role, case)
            executable = command_executable(command_path)
            if not Path(executable).is_absolute():
                raise ValueError(f"{command_path}: command[0] is not an absolute executable path")
            samples, maps = read_capture(capture)
            if not samples:
                raise ValueError(f"{capture}: no samples")
            if not any(row.path == executable and "x" in row.perms for row in maps):
                raise ValueError(f"{capture}: maps do not contain executable exact command[0] path {executable}")
            main_offsets = set()
            for pc in samples:
                kind, offset, _path = map_pc(pc, maps, executable, loads)
                if kind == "main" and offset is not None:
                    key, names = symbols.find(offset)
                    if names:
                        main_offsets.add(offset)
            case_inputs[(role, case)] = (samples, maps, executable, command_path, capture, main_offsets)
    results = []
    for role in ROLES:
        info = role_data[role]
        all_offsets = sorted(set().union(*(case_inputs[(role, case)][5] for case in CASES)))
        locations = addr2line(info["elf"], all_offsets)
        for case in CASES:
            samples, maps, executable, command_path, capture, _offsets = case_inputs[(role, case)]
            case_result = analyze_case(
                role, case, samples, maps, executable, info["loads"], info["symbols"], locations
            )
            case_result["capture"] = capture.name
            case_result["command_metadata"] = command_path.name
            case_result["retained_elf"] = info["elf"].name
            case_result["build_id"] = info["build_id"]
            results.append(case_result)
    output.mkdir(parents=True)
    result = {
        "profiles": str(profiles),
        "method": "depth-one exact-PC mapping; sized symbols only; no nearest-symbol fallback",
        "inline_note": "addr2line inline chains are metadata, not caller stacks; source totals count the first location once per symbolized sample",
        "cases": results,
    }
    (output / "leaf-pc-analysis.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    (output / "leaf-pc-analysis.md").write_text(render_markdown(result), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profiles", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    run(args.profiles, args.output)


if __name__ == "__main__":
    main()
