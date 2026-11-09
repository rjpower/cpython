# Regeneration make semantics

Reviewed at 2026-11-01 10:11:03 UTC, using the retained packaging-v3 regeneration row and the current debug Makefile. This is a source/log audit; no make, generator, compiler, or target was invoked.

The two `The Makefile was updated, you may need to re-run make.` lines do **not** establish that cases regeneration was skipped. The message is an informational echo at the end of the ordinary Makefile construction recipe, without an exit or conditional goal guard. `Makefile.pre` depends on `Makefile.pre.in` and `config.status`, and its recipe runs a subordinate make to construct `Makefile`. GNU make remakes its Makefile and re-reads it before processing the original goals. See the [GNU make manual, “How Makefiles Are Remade”](https://www.gnu.org/software/make/manual/html_node/Remaking-Makefiles.html).

The actual command was `/usr/bin/make -s regen-cases clinic`, from `build/debug`, with recorded native return code 0. The sanitized recorded environment has no `MAKEFLAGS` or `MFLAGS`. Neither the source template nor the inspected generated Makefile has a `MAKE_RESTARTS`/`MAKECMDGOALS` shortcut or `.IGNORE` rule. The relevant recipes do not ignore errors.

`regen-cases` and all six prerequisites are phony: opcode IDs, uop IDs, Python opcode metadata, C opcode metadata, IR cases/targets, and IR metadata. `clinic` is also phony and invokes Clinic with `--force --make` after the ordinary out-of-tree source-artifact check. None of these goals depends on the full interpreter build. The zero make exit therefore supports completion of both requested regeneration scopes, including all seven cases output files. This is an inference from the actual command/result and rule graph, rather than a per-generator PID or command census: `-s` suppresses recipe echo, and unchanged generator outputs are silent.

The row nevertheless **failed** its separate tracked-bytes/modes/paths check: only `Python/clinic/sysmodule.c.h` changed. Cases outputs remained byte-identical in the retained before/after snapshots. There is no accepted overall regeneration closure and no credit for later packaging/foreign-wheel/CPU rows, which did not run.

After the generated-header successor and its source/product bridge receive review, the same unchanged make invocation can earn both no-drift closures if its native exit is 0, the exact tracked snapshot is unchanged, and the ordinary post-command/cleanup checks pass. No additional cases-only invocation is necessary because of this informational message. This finding grants no rerun or source mutation. The landing draft stays bound to historical 8e until an actual successor is accepted.

Source locations: `Makefile.pre.in:975,1615,2141,3118`; inspected debug `Makefile:1119,1759,2285,3262`. Exact hashes and raw row identity are retained in `build/rc-regeneration-make-semantics-20261101-v1/receipt.json`.
