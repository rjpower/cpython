# boil/tools: measurement harness

Stdlib-only Python 3.13+ scripts. Run them with any Python; the interpreter
under test is `--interpreter` (default `build/baseline-default/python`).
Each writes JSON to `--json PATH` (else stdout) and a summary to stderr.

| Tool | Measures | Run |
|---|---|---|
| `code_stats.py` | Static code-object statistics over `Lib/` (instructions, slots = nlocalsplus + stacksize, consts, specialisable share, longest jump, co_code bytes). Reproduces 01-internal-ir.md section 0. | `python boil/tools/code_stats.py --tests --json out.json` (about 45 s with `--tests`) |
| `micro_loops.py` | User-mode instructions per iteration of the nine loops in 02-tagged-values.md section 1, via `perf stat -e instructions:u`. Two runs per loop cancel start-up; skeleton subtracted as in the note. | `python boil/tools/micro_loops.py --json out.json` |
| `pyperf_compare.py` | Geomean new/old ratio and per-benchmark table (sorted by ratio) of two pyperformance JSON files. Below 1 is faster. | `python boil/tools/pyperf_compare.py old.json new.json` |
| `rss.py` | Peak RSS (KiB) for startup, stdlib imports, 200k dicts and the nine loops. | `python boil/tools/rss.py --json out.json` |

Notes: `perf` must be allowed to count user-mode instructions. `rss.py` needs
GNU `/usr/bin/time`. `code_stats.py` runs the measured interpreter's own
`compile()`, so rebuilt interpreters report their own opcode metadata. Check
lint with `uvx ruff check --no-fix boil/tools` (the repo config has `fix=true`).
