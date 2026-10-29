# boil/tools: measurement harness

Stdlib-only Python 3.13+ scripts. Run them with any Python; the interpreter
under test is `--interpreter` (default `build/baseline-default/python`).
Each writes JSON to `--json PATH` (else stdout) and a summary to stderr.

| Tool | Measures | Run |
|---|---|---|
| `code_stats.py` | Static code-object statistics over `Lib/` (instructions, slots = nlocalsplus + stacksize, consts, specialisable share, longest jump, co_code bytes). | `python boil/tools/code_stats.py --tests --json out.json` (about 45 s with `--tests`) |

Instruction counts, peak RSS and box counts are measured by the internal
benchmark set, `boil/bench/run.py` (boil/bench/README.md); boxing call
sites are in `measure/box_audit.py -v`.

Notes: `code_stats.py` runs the measured interpreter's own
`compile()`, so rebuilt interpreters report their own opcode metadata. Check
lint with `uvx ruff check --no-fix boil/tools` (the repo config has `fix=true`).

`gate_run.sh` holds its build lock through worker cleanup. On Linux,
`uv run --no-project --python /usr/bin/python3 python boil/tools/test_gate_run.py`
checks cleanup ownership, an interrupted lock waiter and command failure
status without killing real workers. An optional script path selects a
negative-control version of the runner.
