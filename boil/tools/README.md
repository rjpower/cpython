# boil/tools: measurement harness

Stdlib-only Python 3.13+ scripts. Run them with any Python; the interpreter
under test is `--interpreter` (default `build/baseline-default/python`).
Each writes JSON to `--json PATH` (else stdout) and a summary to stderr.

| Tool | Measures | Run |
|---|---|---|
| `code_stats.py` | Static code-object statistics over `Lib/` (instructions, slots = nlocalsplus + stacksize, consts, specialisable share, longest jump, co_code bytes). Reproduces 01-internal-ir.md section 0. | `python boil/tools/code_stats.py --tests --json out.json` (about 45 s with `--tests`) |
| `pyperf_compare.py` | Geomean new/old ratio and per-benchmark table (sorted by ratio) of two pyperformance JSON files. Below 1 is faster. | `python boil/tools/pyperf_compare.py old.json new.json` |

Instruction counts, peak RSS and box counts are measured by the internal
benchmark set, `boil/bench/run.py` (boil/bench/README.md). It replaces four
scripts that design notes and the worklog cite: `micro_loops.py` is its
micro set (the same loop bodies, repeat counts and skeleton subtraction,
under the names in `boil/bench/internal/micro/`), `rss.py` its rss metric
and memory group, `measure/perloop.py` its instr metric on the kernels, and
`measure/boxes.py` its boxes metric; boxing call sites are in
`measure/box_audit.py -v`.

Notes: `code_stats.py` runs the measured interpreter's own
`compile()`, so rebuilt interpreters report their own opcode metadata. Check
lint with `uvx ruff check --no-fix boil/tools` (the repo config has `fix=true`).
