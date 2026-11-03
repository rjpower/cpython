"""Compute requested-byte forecasts from the frozen probe's measured inputs."""

import json
from pathlib import Path


root = Path(__file__).resolve().parent
rows = {}
for path in sorted((root / "default").glob("*.log")):
    records = [json.loads(line) for line in path.read_text().splitlines()
               if line.startswith("{")]
    compiled = next(row for row in records if row["phase"] == "compiled")
    nwords = len(compiled["words"])
    lines = [line for _, _, line in compiled["co_lines"] if line is not None]
    first = compiled["firstlineno"]
    old_adjusted_max = max(first + 1, *lines) - first + 2
    absolute_max = max(lines) + 1
    relative_width = max(1, (old_adjusted_max.bit_length() + 7) // 8)
    absolute_width = max(1, (absolute_max.bit_length() + 7) // 8)
    # Both proposed headers have sizeof 16 under the qualified x86-64 ABI.
    # These are source/layout forecasts, not new allocator measurements.
    rows[path.stem] = {
        "measured_nwords": nwords, "measured_firstlineno": first,
        "measured_lines": lines, "old_relative_width": relative_width,
        "absolute_width": absolute_width,
        "old_requested_LINE_bytes": 1 + nwords * (1 + relative_width),
        "proposed_variable_LINE_bytes": 16 + nwords * (2 + absolute_width),
        "proposed_fixed_LINE_bytes": 16 + nwords * (2 + 4),
        "fixed_minus_variable": nwords * (4 - absolute_width),
    }
print(json.dumps({"scope": "LINE blocks only; source/layout forecast using measured probe inputs",
                  "excludes": "allocator overhead, common monitoring header, executing copy and tool arrays",
                  "new_heap_measurement": "not run; required after implementation", "rows": rows}, indent=2))
